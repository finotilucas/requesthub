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

#include "request_tabs.h"

struct _RequestTabs {
  GtkBox parent_instance;
  GtkWidget *notebook;
};

G_DEFINE_TYPE(RequestTabs, request_tabs, GTK_TYPE_BOX)

static void request_tabs_class_init(RequestTabsClass *klass) { (void)klass; }

static void request_tabs_init(RequestTabs *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  gtk_widget_set_margin_start(GTK_WIDGET(self), 5);
  gtk_widget_set_margin_end(GTK_WIDGET(self), 5);
  gtk_widget_set_margin_top(GTK_WIDGET(self), 5);
  gtk_widget_set_margin_bottom(GTK_WIDGET(self), 5);

  self->notebook = gtk_notebook_new();
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(self->notebook), TRUE);
  gtk_notebook_set_show_border(GTK_NOTEBOOK(self->notebook), TRUE);

  gtk_widget_set_vexpand(self->notebook, TRUE);
  gtk_widget_set_hexpand(self->notebook, TRUE);

  gtk_box_append(GTK_BOX(self), self->notebook);
}

RequestTabs *request_tabs_new(void) {
  return g_object_new(REQUEST_TYPE_TABS, NULL);
}

void request_tabs_add_view(RequestTabs *self, GtkWidget *view_content,
                           const char *title) {
  g_return_if_fail(REQUEST_IS_TABS(self));
  g_return_if_fail(GTK_IS_WIDGET(view_content));

  GtkWidget *label = gtk_label_new(title);
  gtk_notebook_append_page(GTK_NOTEBOOK(self->notebook), view_content, label);
}
