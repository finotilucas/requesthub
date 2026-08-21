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

#include "http_service.h"

#include "../http/http.h"

struct _HttpService {
  GObject parent_instance;
};

G_DEFINE_FINAL_TYPE(HttpService, http_service, G_TYPE_OBJECT)

typedef struct {
  HttpRequest *request;
} HttpServiceTaskData;

static void http_service_task_data_free(HttpServiceTaskData *data) {
  if (data == NULL) {
    return;
  }
  if (data->request != NULL) {
    http_request_free(data->request);
  }
  g_free(data);
}

static void send_request_thread_func(GTask *task, gpointer source_object,
                          gpointer task_data, GCancellable *cancellable) {
  (void)source_object;
  (void)cancellable;
  HttpServiceTaskData *data = task_data;

  HttpResponse *resp = http_request_perform(data->request);

  if (g_task_return_error_if_cancelled(task)) {
    if (resp != NULL) {
      http_response_free(resp);
    }
    return;
  }

  if (resp == NULL) {
    g_task_return_new_error(task, G_IO_ERROR, G_IO_ERROR_FAILED,
                            "failed to perform HTTP request");
    return;
  }

  g_task_return_pointer(task, resp, (GDestroyNotify)http_response_free);
}

void http_service_send_async(HttpService *self, HttpRequest *request,
                             GCancellable *cancellable,
                             GAsyncReadyCallback callback, gpointer user_data) {
  g_return_if_fail(HTTP_IS_SERVICE(self));
  g_return_if_fail(request != NULL);

  HttpServiceTaskData *data = g_new0(HttpServiceTaskData, 1);
  data->request = request;

  GTask *task = g_task_new(self, cancellable, callback, user_data);
  g_task_set_task_data(task, data,
                       (GDestroyNotify)http_service_task_data_free);
  g_task_run_in_thread(task, send_request_thread_func);
  g_object_unref(task);
}

HttpResponse *http_service_send_finish(HttpService *self, GAsyncResult *result,
                                       GError **error) {
  g_return_val_if_fail(HTTP_IS_SERVICE(self), NULL);
  g_return_val_if_fail(g_task_is_valid(result, self), NULL);

  return g_task_propagate_pointer(G_TASK(result), error);
}

HttpService *http_service_new(void) {
  return g_object_new(HTTP_TYPE_SERVICE, NULL);
}

static void http_service_class_init(HttpServiceClass *klass) { (void)klass; }
static void http_service_init(HttpService *self) { (void)self; }
