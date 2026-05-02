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

#include "response_view.h"
#include "../components/response_content.h"
#include "../components/response_top_bar.h"

struct _ResponseView {
  GtkBox parent_instance;
  ResponseTopBar *top_bar;
  ResponseContent *content;
};

G_DEFINE_TYPE(ResponseView, response_view, GTK_TYPE_BOX)

static void response_view_init(ResponseView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_size_request(GTK_WIDGET(self), RESPONSE_VIEW_MIN_WIDTH, -1);

  self->top_bar = response_top_bar_new();
  self->content = response_content_new();

  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->top_bar));
  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->content));
}

static void response_view_class_init(ResponseViewClass *klass) { (void)klass; }

ResponseView *response_view_new(void) {
  return g_object_new(RESPONSE_TYPE_VIEW, NULL);
}

void response_view_update(ResponseView *self, HttpResponse *resp) {
  response_top_bar_update(self->top_bar, resp);
  response_content_set_response(self->content, resp);
}
