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
#include "../components/request_tabs.h"
#include "body_view.h"
#include "headers_view.h"
#include "params_view.h"

struct _RequestView {
  GtkBox parent_instance;
  RequestTopBar *top_bar;
  RequestTabs *tabs_component;
  ParamsView *params_view;
  HeadersView *headers_view;
  BodyView *body_view;
};

G_DEFINE_TYPE(RequestView, request_view, GTK_TYPE_BOX)

static void request_view_init(RequestView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  self->top_bar = request_top_bar_new();
  if (self->top_bar) {
    gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->top_bar));
  }

  self->tabs_component = request_tabs_new();
  if (self->tabs_component != NULL) {
    gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->tabs_component));

    ParamsView *params = params_view_new();
    if (params != NULL) {
      request_tabs_add_view(self->tabs_component, GTK_WIDGET(params), "Params");
      self->params_view = params;
    }

    BodyView *body = body_view_new();
    if (body != NULL) {
      request_tabs_add_view(self->tabs_component, GTK_WIDGET(body), "Body");
      self->body_view = body;
    }

    HeadersView *headers = headers_view_new();
    if (headers != NULL) {
      request_tabs_add_view(self->tabs_component, GTK_WIDGET(headers),
                            "Headers");
      self->headers_view = headers;
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

ParamsView *request_view_get_params_view(RequestView *self) {
  g_return_val_if_fail(REQUEST_IS_VIEW(self), NULL);
  return self->params_view;
}

HeadersView *request_view_get_headers_view(RequestView *self) {
  g_return_val_if_fail(REQUEST_IS_VIEW(self), NULL);
  return self->headers_view;
}

BodyView *request_view_get_body_view(RequestView *self) {
  g_return_val_if_fail(REQUEST_IS_VIEW(self), NULL);
  return self->body_view;
}
