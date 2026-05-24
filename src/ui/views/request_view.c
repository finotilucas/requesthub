/*******************************************************************************
 * REQUEST HUB
 * =============================================================================
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

#include "request_view.h"
#include "../panels/request_tabs.h"
#include "../panels/body_panel.h"
#include "../panels/headers_panel.h"
#include "../panels/params_panel.h"

struct _RequestView {
  GtkBox parent_instance;
  RequestTopBar *top_bar;
  RequestTabs *tabs_component;
  ParamsPanel *params_panel;
  HeadersPanel *headers_panel;
  BodyPanel *body_panel;
};

G_DEFINE_TYPE(RequestView, request_view, GTK_TYPE_BOX)

static void request_view_init(RequestView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_size_request(GTK_WIDGET(self), REQUEST_VIEW_MIN_WIDTH, -1);

  self->top_bar = request_top_bar_new();
  if (self->top_bar) {
    gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->top_bar));
  }

  self->tabs_component = request_tabs_new();
  if (self->tabs_component != NULL) {
    gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->tabs_component));

    ParamsPanel *params = params_panel_new();
    if (params != NULL) {
      request_tabs_add_view(self->tabs_component, GTK_WIDGET(params), "params",
                            "Params", "view-list-symbolic");
      self->params_panel = params;
    }

    BodyPanel *body = body_panel_new();
    if (body != NULL) {
      request_tabs_add_view(self->tabs_component, GTK_WIDGET(body), "body",
                            "Body", "text-x-generic-symbolic");
      self->body_panel = body;
    }

    HeadersPanel *headers = headers_panel_new();
    if (headers != NULL) {
      request_tabs_add_view(self->tabs_component, GTK_WIDGET(headers),
                            "headers", "Headers",
                            "preferences-system-symbolic");
      self->headers_panel = headers;
    }
  } else {
    g_warning("Can not load RequestView");
  }
}

static void request_view_class_init(RequestViewClass *klass) { (void)klass; }

RequestView *request_view_new(void) {
  return g_object_new(REQUEST_TYPE_VIEW, NULL);
}

RequestTopBar *request_view_get_top_bar(RequestView *self) {
  g_return_val_if_fail(REQUEST_IS_VIEW(self), NULL);

  return self->top_bar;
}

ParamsPanel *request_view_get_params_panel(RequestView *self) {
  g_return_val_if_fail(REQUEST_IS_VIEW(self), NULL);
  return self->params_panel;
}

HeadersPanel *request_view_get_headers_panel(RequestView *self) {
  g_return_val_if_fail(REQUEST_IS_VIEW(self), NULL);
  return self->headers_panel;
}

BodyPanel *request_view_get_body_panel(RequestView *self) {
  g_return_val_if_fail(REQUEST_IS_VIEW(self), NULL);
  return self->body_panel;
}

void request_view_apply_state(RequestView *self, const RequestState *state) {
  g_return_if_fail(REQUEST_IS_VIEW(self));
  if (state == NULL) {
    return;
  }

  if (self->top_bar != NULL) {
    request_top_bar_set_url(self->top_bar, request_state_get_url(state));
    request_top_bar_set_method(self->top_bar, request_state_get_method(state));
  }

  if (self->body_panel != NULL) {
    body_panel_clear(self->body_panel);
    const char *body = request_state_get_body(state);
    if (body != NULL && *body != '\0') {
      body_panel_set_content(self->body_panel, body);
    }
  }

  if (self->params_panel != NULL) {
    params_panel_clear_all(self->params_panel);
    guint count = request_state_query_count(state);
    for (guint i = 0; i < count; i++) {
      const char *k = request_state_query_key(state, i);
      const char *v = request_state_query_value(state, i);
      if (k != NULL) {
        params_panel_add_pair(self->params_panel, k, v != NULL ? v : "");
      }
    }
  }

  if (self->headers_panel != NULL) {
    headers_panel_clear_all(self->headers_panel);
    guint count = request_state_headers_count(state);
    for (guint i = 0; i < count; i++) {
      const char *k = request_state_header_key(state, i);
      const char *v = request_state_header_value(state, i);
      if (k != NULL) {
        headers_panel_add_pair(self->headers_panel, k, v != NULL ? v : "");
      }
    }
  }
}

static void capture_header_to_state(const char *key, const char *value,
                                    gpointer user_data) {
  request_state_add_header((RequestState *)user_data, key, value);
}

static void capture_param_to_state(const char *key, const char *value,
                                   gpointer user_data) {
  request_state_add_query((RequestState *)user_data, key, value);
}

RequestState *request_view_capture_state(RequestView *self) {
  g_return_val_if_fail(REQUEST_IS_VIEW(self), NULL);

  RequestState *state = request_state_new();

  if (self->top_bar != NULL) {
    request_state_set_url(state, request_top_bar_get_url(self->top_bar));
    request_state_set_method(state, request_top_bar_get_method(self->top_bar));
  }

  if (self->headers_panel != NULL) {
    headers_panel_for_each(self->headers_panel, capture_header_to_state, state);
  }

  if (self->params_panel != NULL) {
    params_panel_for_each(self->params_panel, capture_param_to_state, state);
  }

  if (self->body_panel != NULL) {
    char *body = body_panel_get_content(self->body_panel);
    if (body != NULL && *body != '\0') {
      request_state_set_body(state, body);
    }
    g_free(body);
  }

  return state;
}
