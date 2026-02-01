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

struct _RequestView {
  GtkBox parent_instance;
  RequestTopBar *top_bar;
};

G_DEFINE_TYPE(RequestView, request_view, GTK_TYPE_BOX)

static void request_view_init(RequestView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_size_request(GTK_WIDGET(self), 300, -1);

  self->top_bar = request_top_bar_new();
  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->top_bar));
}

static void request_view_class_init(RequestViewClass *klass) { (void)klass; }

RequestView *request_view_new(void) {
  return g_object_new(REQUEST_TYPE_VIEW, NULL);
}

RequestTopBar *request_view_get_top_bar(RequestView *self) {
  return self->top_bar;
}
