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

#include "request_tabs.h"

struct _RequestTabs {
  GtkBox parent_instance;
  AdwViewStack *stack;
};

G_DEFINE_FINAL_TYPE(RequestTabs, request_tabs, GTK_TYPE_BOX)

static void request_tabs_class_init(RequestTabsClass *klass) { (void)klass; }

static void request_tabs_init(RequestTabs *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  gtk_widget_set_margin_start(GTK_WIDGET(self), 5);
  gtk_widget_set_margin_end(GTK_WIDGET(self), 5);
  gtk_widget_set_margin_top(GTK_WIDGET(self), 5);
  gtk_widget_set_margin_bottom(GTK_WIDGET(self), 5);

  self->stack = ADW_VIEW_STACK(adw_view_stack_new());
  gtk_widget_set_vexpand(GTK_WIDGET(self->stack), TRUE);
  gtk_widget_set_hexpand(GTK_WIDGET(self->stack), TRUE);

  AdwViewSwitcher *switcher = ADW_VIEW_SWITCHER(adw_view_switcher_new());
  adw_view_switcher_set_policy(switcher, ADW_VIEW_SWITCHER_POLICY_WIDE);
  adw_view_switcher_set_stack(switcher, self->stack);
  gtk_widget_set_halign(GTK_WIDGET(switcher), GTK_ALIGN_CENTER);

  gtk_box_append(GTK_BOX(self), GTK_WIDGET(switcher));
  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->stack));
}

RequestTabs *request_tabs_new(void) {
  return g_object_new(REQUEST_TYPE_TABS, NULL);
}

void request_tabs_add_view(RequestTabs *self, GtkWidget *content,
                           const char *name, const char *title,
                           const char *icon_name) {
  g_return_if_fail(REQUEST_IS_TABS(self));
  g_return_if_fail(GTK_IS_WIDGET(content));
  g_return_if_fail(name != NULL);

  AdwViewStackPage *page =
      adw_view_stack_add_titled(self->stack, content, name, title);
  if (icon_name != NULL) {
    adw_view_stack_page_set_icon_name(page, icon_name);
  }
}
