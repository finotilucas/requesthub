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

#include "params_panel.h"
#include "../components/kv_list_view.h"

struct _ParamsPanel {
  GtkBox parent_instance;
  KvListView *kv;
};

G_DEFINE_TYPE(ParamsPanel, params_panel, GTK_TYPE_BOX)

static void params_panel_class_init(ParamsPanelClass *klass) { (void)klass; }

static void params_panel_init(ParamsPanel *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  KvListViewConfig config = {
      .title = "Query parameters",
      .add_button_label = "Add",
      .clear_button_label = "Delete all",
      .key_placeholder = "name",
      .value_placeholder = "value",
      .add_prepends = TRUE,
  };

  self->kv = kv_list_view_new(&config);
  gtk_widget_set_vexpand(GTK_WIDGET(self->kv), TRUE);
  gtk_widget_set_hexpand(GTK_WIDGET(self->kv), TRUE);
  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->kv));
}

ParamsPanel *params_panel_new(void) {
  return g_object_new(TYPE_PARAMS_PANEL, NULL);
}

static void apply_param_to_request(const char *key, const char *value,
                                   gpointer user_data) {
  http_request_add_query_param((HttpRequest *)user_data, key, value);
}

void params_panel_apply_to_request(ParamsPanel *self, HttpRequest *request) {
  g_return_if_fail(PARAMS_IS_PANEL(self));
  g_return_if_fail(request != NULL);

  kv_list_view_for_each(self->kv, KV_LIST_ITER_ALL, apply_param_to_request,
                        request);
}

void params_panel_clear_all(ParamsPanel *self) {
  g_return_if_fail(PARAMS_IS_PANEL(self));
  kv_list_view_clear_editable(self->kv);
}

void params_panel_for_each(ParamsPanel *self,
                          void (*func)(const char *key, const char *value,
                                       gpointer user_data),
                          gpointer user_data) {
  g_return_if_fail(PARAMS_IS_PANEL(self));
  kv_list_view_for_each(self->kv, KV_LIST_ITER_ALL, func, user_data);
}

void params_panel_add_pair(ParamsPanel *self, const char *key,
                          const char *value) {
  g_return_if_fail(PARAMS_IS_PANEL(self));
  kv_list_view_add_editable(self->kv, key, value);
}
