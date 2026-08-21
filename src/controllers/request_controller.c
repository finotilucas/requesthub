/*******************************************************************************
 * Copyright (C) 2026 Lucas Finoti <lucas.finoti@protonmail.com>
 *
 * This file is part of RequestHub.
 *
 * RequestHub is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * any later version.
 *
 * RequestHub is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RequestHub. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************************/

#include "request_controller.h"

#include "../http/request.h"
#include "../http/response.h"
#include "../models/request_state.h"
#include "../models/request_state_codec.h"
#include "../ui/panels/body_panel.h"
#include "../ui/panels/request_top_bar.h"

struct _RequestController {
  GObject parent_instance;
  RequestView *request_view;
  ResponseView *response_view;
  HttpService *http_service;
  HistoryService *history_service;
  GCancellable *cancellable;
};

G_DEFINE_FINAL_TYPE(RequestController, request_controller, G_TYPE_OBJECT)

typedef struct {
  HistoryEntry *history_entry;
  GWeakRef controller_ref;
} AsyncRequestData;

static void async_request_data_free(AsyncRequestData *data) {
  if (data == NULL) {
    return;
  }
  if (data->history_entry != NULL) {
    history_entry_free(data->history_entry);
  }
  g_weak_ref_clear(&data->controller_ref);
  g_free(data);
}

static HttpRequest *http_request_from_state(const RequestState *state) {
  if (state == NULL) {
    return NULL;
  }
  HttpRequest *req = http_request_new(request_state_get_url(state),
                                      request_state_get_method(state));
  if (req == NULL) {
    return NULL;
  }

  guint header_count = request_state_headers_count(state);
  for (guint i = 0; i < header_count; i++) {
    http_request_add_header(req, request_state_header_key(state, i),
                            request_state_header_value(state, i));
  }

  guint query_count = request_state_query_count(state);
  for (guint i = 0; i < query_count; i++) {
    http_request_add_query_param(req, request_state_query_key(state, i),
                                 request_state_query_value(state, i));
  }

  return req;
}

static void on_request_finished(GObject *source, GAsyncResult *res,
                                gpointer user_data) {
  AsyncRequestData *async_data = user_data;
  GError *error = NULL;
  HttpResponse *resp =
      http_service_send_finish(HTTP_SERVICE(source), res, &error);

  RequestController *self = g_weak_ref_get(&async_data->controller_ref);

  if (error != NULL) {
    if (!g_error_matches(error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
      g_warning("request failed: %s", error->message);
      if (self != NULL && self->request_view != NULL) {
        RequestTopBar *bar = request_view_get_top_bar(self->request_view);
        if (bar != NULL) {
          request_top_bar_set_loading(bar, FALSE);
        }
      }
    }
    g_clear_error(&error);
    g_clear_object(&self);
    if (resp != NULL) {
      http_response_free(resp);
    }
    async_request_data_free(async_data);
    return;
  }

  if (self == NULL) {
    if (resp != NULL) {
      http_response_free(resp);
    }
    async_request_data_free(async_data);
    return;
  }

  if (self->response_view != NULL) {
    response_view_update(self->response_view, resp);
  }

  if (self->request_view != NULL) {
    RequestTopBar *bar = request_view_get_top_bar(self->request_view);
    if (bar != NULL) {
      request_top_bar_set_loading(bar, FALSE);
    }
  }

  if (async_data->history_entry != NULL && self->history_service != NULL) {
    history_entry_apply_response(async_data->history_entry, resp);
    history_service_record(self->history_service, async_data->history_entry);
    async_data->history_entry = NULL;
  }

  g_object_unref(self);
  if (resp != NULL) {
    http_response_free(resp);
  }
  async_request_data_free(async_data);
}

void request_controller_send(RequestController *self) {
  g_return_if_fail(REQUEST_IS_CONTROLLER(self));
  if (self->request_view == NULL || self->http_service == NULL) {
    return;
  }

  RequestState *state = request_view_capture_state(self->request_view);
  if (state == NULL) {
    g_warning("send aborted: could not capture request state");
    return;
  }

  HttpRequest *req = http_request_from_state(state);
  if (req == NULL) {
    g_warning("send aborted: request needs a URL");
    request_state_free(state);
    return;
  }

  BodyPanel *body_panel = request_view_get_body_panel(self->request_view);
  if (body_panel != NULL) {
    body_panel_apply_to_request(body_panel, req);
  }

  HistoryEntry *entry = history_entry_from_request_state(state);
  request_state_free(state);

  if (self->cancellable != NULL) {
    g_cancellable_cancel(self->cancellable);
    g_clear_object(&self->cancellable);
  }
  self->cancellable = g_cancellable_new();

  RequestTopBar *bar = request_view_get_top_bar(self->request_view);
  if (bar != NULL) {
    request_top_bar_set_loading(bar, TRUE);
  }

  AsyncRequestData *async_data = g_new0(AsyncRequestData, 1);
  async_data->history_entry = entry;
  g_weak_ref_init(&async_data->controller_ref, self);

  http_service_send_async(self->http_service, req, self->cancellable,
                          on_request_finished, async_data);
}

static void on_top_bar_send_clicked(RequestTopBar *bar, gpointer user_data) {
  (void)bar;
  request_controller_send(REQUEST_CONTROLLER(user_data));
}

RequestController *request_controller_new(RequestView *request_view,
                                          ResponseView *response_view,
                                          HttpService *http_service,
                                          HistoryService *history_service) {
  g_return_val_if_fail(REQUEST_IS_VIEW(request_view), NULL);
  g_return_val_if_fail(HTTP_IS_SERVICE(http_service), NULL);
  g_return_val_if_fail(HISTORY_IS_SERVICE(history_service), NULL);

  RequestController *self = g_object_new(REQUEST_TYPE_CONTROLLER, NULL);
  self->request_view = request_view;
  self->response_view = response_view;
  self->http_service = g_object_ref(http_service);
  self->history_service = g_object_ref(history_service);

  RequestTopBar *bar = request_view_get_top_bar(request_view);
  if (bar != NULL) {
    g_signal_connect_object(bar, "send-clicked",
                            G_CALLBACK(on_top_bar_send_clicked), self, 0);
  }

  return self;
}

static void request_controller_dispose(GObject *obj) {
  RequestController *self = REQUEST_CONTROLLER(obj);
  if (self->cancellable != NULL) {
    g_cancellable_cancel(self->cancellable);
    g_clear_object(&self->cancellable);
  }
  g_clear_object(&self->http_service);
  g_clear_object(&self->history_service);
  G_OBJECT_CLASS(request_controller_parent_class)->dispose(obj);
}

static void request_controller_class_init(RequestControllerClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = request_controller_dispose;
}

static void request_controller_init(RequestController *self) { (void)self; }
