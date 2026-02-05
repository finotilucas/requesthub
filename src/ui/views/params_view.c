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

#include "params_view.h"
#include <gtk/gtk.h>

typedef struct {
  GtkWidget *key_entry;
  GtkWidget *val_entry;
  GtkWidget *del_btn;
} QueryRow;

struct _ParamsView {
  GtkBox parent_instance;
  GtkWidget *list_box;
};

G_DEFINE_TYPE(ParamsView, params_view, GTK_TYPE_BOX)

static void params_view_class_init(ParamsViewClass *klass) { (void)klass; }

static void query_row_free(gpointer data) { g_free(data); }

static void validate_row_state(QueryRow *row) {
  const char *text = gtk_editable_get_text(GTK_EDITABLE(row->key_entry));
  if (text == NULL || *text == '\0') {
    gtk_widget_add_css_class(row->del_btn, "warning");
  } else {
    gtk_widget_remove_css_class(row->del_btn, "warning");
  }
}

static void on_key_changed(GtkEditable *editable, gpointer user_data) {
  (void)editable;
  validate_row_state((QueryRow *)user_data);
}

static void on_remove_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;
  GtkWidget *hbox = GTK_WIDGET(user_data);
  GtkWidget *row_container =
      gtk_widget_get_ancestor(hbox, GTK_TYPE_LIST_BOX_ROW);
  GtkWidget *list_box =
      gtk_widget_get_ancestor(row_container, GTK_TYPE_LIST_BOX);

  if (GTK_IS_LIST_BOX(list_box) && GTK_IS_LIST_BOX_ROW(row_container)) {
    gtk_list_box_remove(GTK_LIST_BOX(list_box), row_container);
  }
}

static void add_param_row(ParamsView *self, const char *key, const char *val) {
  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

  gtk_widget_set_margin_bottom(hbox, 6);
  gtk_widget_set_margin_start(hbox, 2);
  gtk_widget_set_margin_end(hbox, 2);

  GtkWidget *key_entry = gtk_entry_new();
  GtkWidget *val_entry = gtk_entry_new();
  GtkWidget *del_btn = gtk_button_new_from_icon_name("user-trash-symbolic");

  gtk_widget_add_css_class(del_btn, "flat");
  gtk_entry_set_placeholder_text(GTK_ENTRY(key_entry), "name");
  gtk_entry_set_placeholder_text(GTK_ENTRY(val_entry), "value");

  gtk_widget_set_margin_end(key_entry, 4);

  if (key)
    gtk_editable_set_text(GTK_EDITABLE(key_entry), key);
  if (val)
    gtk_editable_set_text(GTK_EDITABLE(val_entry), val);

  gtk_widget_set_hexpand(key_entry, TRUE);
  gtk_widget_set_hexpand(val_entry, TRUE);

  gtk_box_append(GTK_BOX(hbox), key_entry);
  gtk_box_append(GTK_BOX(hbox), val_entry);
  gtk_box_append(GTK_BOX(hbox), del_btn);

  QueryRow *row_data = g_new0(QueryRow, 1);
  row_data->key_entry = key_entry;
  row_data->val_entry = val_entry;
  row_data->del_btn = del_btn;

  g_object_set_data_full(G_OBJECT(hbox), "query-data", row_data,
                         query_row_free);

  g_signal_connect(del_btn, "clicked", G_CALLBACK(on_remove_clicked), hbox);
  g_signal_connect(key_entry, "changed", G_CALLBACK(on_key_changed), row_data);

  gtk_widget_add_css_class(hbox, "query-row");

  gtk_list_box_prepend(GTK_LIST_BOX(self->list_box), hbox);

  validate_row_state(row_data);
}

static void on_add_btn_clicked(GtkButton *btn, gpointer user_data) {
  add_param_row(PARAMS_VIEW(user_data), NULL, NULL);
  (void)btn;
}

static void on_clear_all_clicked(GtkButton *btn, gpointer user_data) {
  params_view_clear_all(PARAMS_VIEW(user_data));
  (void)btn;
}

static void params_view_init(ParamsView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  gtk_widget_set_margin_start(GTK_WIDGET(self), 16);
  gtk_widget_set_margin_end(GTK_WIDGET(self), 16);
  gtk_widget_set_margin_top(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_bottom(GTK_WIDGET(self), 10);

  GtkWidget *label = gtk_label_new("Query parameters");
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_widget_set_margin_top(label, 10);
  gtk_widget_set_margin_bottom(label, 20);
  gtk_widget_add_css_class(label, "title-4");
  gtk_box_append(GTK_BOX(self), label);

  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_bottom(header, 15);

  GtkWidget *add_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *add_icon = gtk_image_new_from_icon_name("list-add-symbolic");
  GtkWidget *add_text = gtk_label_new("Add");
  gtk_box_append(GTK_BOX(add_content), add_icon);
  gtk_box_append(GTK_BOX(add_content), add_text);

  GtkWidget *add_btn = gtk_button_new();
  gtk_button_set_child(GTK_BUTTON(add_btn), add_content);
  gtk_widget_add_css_class(add_btn, "flat");

  GtkWidget *clear_btn = gtk_button_new_with_label("Delete all");
  gtk_widget_add_css_class(clear_btn, "flat");
  gtk_widget_add_css_class(clear_btn, "error");

  gtk_box_append(GTK_BOX(header), add_btn);
  gtk_box_append(GTK_BOX(header), clear_btn);
  gtk_box_append(GTK_BOX(self), header);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolled), FALSE);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  self->list_box = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(self->list_box),
                                  GTK_SELECTION_NONE);
  gtk_list_box_set_show_separators(GTK_LIST_BOX(self->list_box), FALSE);

  gtk_widget_add_css_class(self->list_box, "background");
  gtk_widget_add_css_class(self->list_box, "params-list");

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), self->list_box);
  gtk_box_append(GTK_BOX(self), scrolled);

  g_signal_connect(add_btn, "clicked", G_CALLBACK(on_add_btn_clicked), self);
  g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_all_clicked),
                   self);
}

ParamsView *params_view_new(void) {
  return g_object_new(TYPE_PARAMS_VIEW, NULL);
}

void params_view_clear_all(ParamsView *self) {
  g_return_if_fail(PARAMS_IS_VIEW(self));
  GtkWidget *child;

  while ((child = gtk_widget_get_first_child(self->list_box)) != NULL) {
    gtk_list_box_remove(GTK_LIST_BOX(self->list_box), child);
  }
}

void params_view_apply_to_request(ParamsView *self, HttpRequest *request) {
  g_return_if_fail(PARAMS_IS_VIEW(self));
  g_return_if_fail(request != NULL);

  GtkWidget *row = gtk_widget_get_first_child(self->list_box);
  while (row != NULL) {
    GtkWidget *hbox = gtk_widget_get_first_child(row);
    if (hbox) {
      QueryRow *data = g_object_get_data(G_OBJECT(hbox), "query-data");
      if (data) {
        const char *k = gtk_editable_get_text(GTK_EDITABLE(data->key_entry));
        const char *v = gtk_editable_get_text(GTK_EDITABLE(data->val_entry));

        if (k && *k != '\0') {
          http_request_add_query_param(request, k, v);
        }
      }
    }
    row = gtk_widget_get_next_sibling(row);
  }
}
