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

#include "kv_list_view.h"

#define KV_ROW_DATA_KEY "kv-row-data"
#define KV_ACTION_MIN_WIDTH 36

struct _KvListView {
  GtkBox parent_instance;
  GtkWidget *list_box;
  gchar *key_placeholder;
  gchar *value_placeholder;
  gboolean add_prepends;
};

G_DEFINE_TYPE(KvListView, kv_list_view, GTK_TYPE_BOX)

typedef struct {
  GtkWidget *key_entry;
  GtkWidget *value_entry;
  GtkWidget *action_widget;
  gboolean editable;
} KvRow;

static void kv_row_free(gpointer data) { g_free(data); }

static void row_validate(KvRow *row) {
  if (!row->editable) {
    return;
  }
  const char *text = gtk_editable_get_text(GTK_EDITABLE(row->key_entry));
  if (text == NULL || *text == '\0') {
    gtk_widget_add_css_class(row->action_widget, "warning");
  } else {
    gtk_widget_remove_css_class(row->action_widget, "warning");
  }
}

static void on_key_entry_changed(GtkEditable *editable, gpointer user_data) {
  (void)editable;
  row_validate((KvRow *)user_data);
}

static void on_remove_button_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  GtkWidget *hbox = GTK_WIDGET(user_data);
  GtkWidget *list_box_row =
      gtk_widget_get_ancestor(hbox, GTK_TYPE_LIST_BOX_ROW);
  if (list_box_row == NULL) {
    return;
  }
  GtkWidget *list_box =
      gtk_widget_get_ancestor(list_box_row, GTK_TYPE_LIST_BOX);
  if (GTK_IS_LIST_BOX(list_box)) {
    gtk_list_box_remove(GTK_LIST_BOX(list_box), list_box_row);
  }
}

static GtkWidget *build_remove_button(GtkWidget *hbox) {
  GtkWidget *button = gtk_button_new_from_icon_name("user-trash-symbolic");
  gtk_widget_add_css_class(button, "flat");
  gtk_widget_add_css_class(button, "kv-remove-button");
  g_signal_connect(button, "clicked", G_CALLBACK(on_remove_button_clicked),
                   hbox);
  return button;
}

static GtkWidget *build_info_icon(const char *tooltip) {
  GtkWidget *icon = gtk_image_new_from_icon_name("dialog-information-symbolic");
  gtk_widget_add_css_class(icon, "kv-info-icon");
  gtk_widget_set_sensitive(icon, FALSE);
  if (tooltip != NULL) {
    gtk_widget_set_tooltip_text(icon, tooltip);
  }
  return icon;
}

static void append_row(KvListView *self, const char *key, const char *value,
                       gboolean editable, const char *fixed_tooltip,
                       gboolean prepend) {
  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_bottom(hbox, 6);
  gtk_widget_set_margin_start(hbox, 2);
  gtk_widget_set_margin_end(hbox, 2);
  gtk_widget_add_css_class(hbox, "kv-row");

  GtkWidget *entries_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_hexpand(entries_box, TRUE);

  GtkWidget *key_entry = gtk_entry_new();
  GtkWidget *value_entry = gtk_entry_new();

  gtk_entry_set_placeholder_text(GTK_ENTRY(key_entry), self->key_placeholder);
  gtk_entry_set_placeholder_text(GTK_ENTRY(value_entry),
                                 self->value_placeholder);

  if (key != NULL) {
    gtk_editable_set_text(GTK_EDITABLE(key_entry), key);
  }
  if (value != NULL) {
    gtk_editable_set_text(GTK_EDITABLE(value_entry), value);
  }

  gtk_editable_set_editable(GTK_EDITABLE(key_entry), editable);
  gtk_editable_set_editable(GTK_EDITABLE(value_entry), editable);

  gtk_widget_set_hexpand(key_entry, TRUE);
  gtk_widget_set_hexpand(value_entry, TRUE);
  gtk_widget_set_halign(key_entry, GTK_ALIGN_FILL);
  gtk_widget_set_halign(value_entry, GTK_ALIGN_FILL);
  gtk_widget_set_margin_end(key_entry, 4);

  gtk_box_append(GTK_BOX(entries_box), key_entry);
  gtk_box_append(GTK_BOX(entries_box), value_entry);

  GtkWidget *action_widget =
      editable ? build_remove_button(hbox) : build_info_icon(fixed_tooltip);
  gtk_widget_set_size_request(action_widget, KV_ACTION_MIN_WIDTH, -1);
  gtk_widget_set_hexpand(action_widget, FALSE);
  gtk_widget_set_halign(action_widget, GTK_ALIGN_END);

  gtk_box_append(GTK_BOX(hbox), entries_box);
  gtk_box_append(GTK_BOX(hbox), action_widget);

  KvRow *row = g_new0(KvRow, 1);
  row->key_entry = key_entry;
  row->value_entry = value_entry;
  row->action_widget = action_widget;
  row->editable = editable;

  g_object_set_data_full(G_OBJECT(hbox), KV_ROW_DATA_KEY, row, kv_row_free);

  if (editable) {
    g_signal_connect(key_entry, "changed",
                     G_CALLBACK(on_key_entry_changed), row);
  }

  if (prepend) {
    gtk_list_box_prepend(GTK_LIST_BOX(self->list_box), hbox);
  } else {
    gtk_list_box_append(GTK_LIST_BOX(self->list_box), hbox);
  }

  row_validate(row);
}

static KvRow *row_from_list_box_row(GtkWidget *list_box_row) {
  if (!GTK_IS_LIST_BOX_ROW(list_box_row)) {
    return NULL;
  }
  GtkWidget *hbox =
      gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(list_box_row));
  if (hbox == NULL) {
    return NULL;
  }
  return g_object_get_data(G_OBJECT(hbox), KV_ROW_DATA_KEY);
}

static void on_add_button_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  KvListView *self = KV_LIST_VIEW(user_data);
  append_row(self, NULL, NULL, TRUE, NULL, self->add_prepends);
}

static void on_clear_button_clicked(GtkButton *button, gpointer user_data) {
  (void)button;
  kv_list_view_clear_editable(KV_LIST_VIEW(user_data));
}

void kv_list_view_add_fixed(KvListView *self, const char *key,
                            const char *value, const char *info_tooltip) {
  g_return_if_fail(KV_IS_LIST_VIEW(self));
  append_row(self, key, value, FALSE, info_tooltip, FALSE);
}

void kv_list_view_add_editable(KvListView *self, const char *key,
                               const char *value) {
  g_return_if_fail(KV_IS_LIST_VIEW(self));
  append_row(self, key, value, TRUE, NULL, FALSE);
}

void kv_list_view_clear_editable(KvListView *self) {
  g_return_if_fail(KV_IS_LIST_VIEW(self));

  GtkWidget *list_box_row =
      gtk_widget_get_first_child(GTK_WIDGET(self->list_box));
  while (list_box_row != NULL) {
    GtkWidget *next = gtk_widget_get_next_sibling(list_box_row);
    KvRow *row = row_from_list_box_row(list_box_row);
    if (row != NULL && row->editable) {
      gtk_list_box_remove(GTK_LIST_BOX(self->list_box), list_box_row);
    }
    list_box_row = next;
  }
}

void kv_list_view_for_each(KvListView *self, KvListIterMode mode,
                           KvListIterFunc func, gpointer user_data) {
  g_return_if_fail(KV_IS_LIST_VIEW(self));
  g_return_if_fail(func != NULL);

  GtkWidget *list_box_row =
      gtk_widget_get_first_child(GTK_WIDGET(self->list_box));
  while (list_box_row != NULL) {
    KvRow *row = row_from_list_box_row(list_box_row);
    if (row != NULL) {
      gboolean include =
          (mode == KV_LIST_ITER_ALL) || row->editable;
      if (include) {
        const char *k =
            gtk_editable_get_text(GTK_EDITABLE(row->key_entry));
        const char *v =
            gtk_editable_get_text(GTK_EDITABLE(row->value_entry));
        if (k != NULL && *k != '\0') {
          func(k, v != NULL ? v : "", user_data);
        }
      }
    }
    list_box_row = gtk_widget_get_next_sibling(list_box_row);
  }
}

static void kv_list_view_finalize(GObject *object) {
  KvListView *self = KV_LIST_VIEW(object);
  g_free(self->key_placeholder);
  g_free(self->value_placeholder);
  G_OBJECT_CLASS(kv_list_view_parent_class)->finalize(object);
}

static void kv_list_view_class_init(KvListViewClass *klass) {
  G_OBJECT_CLASS(klass)->finalize = kv_list_view_finalize;
}

static void kv_list_view_init(KvListView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  gtk_widget_set_margin_start(GTK_WIDGET(self), 16);
  gtk_widget_set_margin_end(GTK_WIDGET(self), 16);
  gtk_widget_set_margin_top(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_bottom(GTK_WIDGET(self), 10);
}

static void build_chrome(KvListView *self, const KvListViewConfig *config) {
  GtkWidget *title_label = gtk_label_new(config->title);
  gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
  gtk_widget_set_margin_top(title_label, 10);
  gtk_widget_set_margin_bottom(title_label, 20);
  gtk_widget_add_css_class(title_label, "title-4");
  gtk_box_append(GTK_BOX(self), title_label);

  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_bottom(header, 15);

  GtkWidget *add_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(add_content),
                 gtk_image_new_from_icon_name("list-add-symbolic"));
  gtk_box_append(GTK_BOX(add_content), gtk_label_new(config->add_button_label));

  GtkWidget *add_button = gtk_button_new();
  gtk_button_set_child(GTK_BUTTON(add_button), add_content);
  gtk_widget_add_css_class(add_button, "flat");

  GtkWidget *clear_button =
      gtk_button_new_with_label(config->clear_button_label);
  gtk_widget_add_css_class(clear_button, "flat");
  gtk_widget_add_css_class(clear_button, "error");

  gtk_box_append(GTK_BOX(header), add_button);
  gtk_box_append(GTK_BOX(header), clear_button);
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
  gtk_widget_add_css_class(self->list_box, "kv-list");

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), self->list_box);
  gtk_box_append(GTK_BOX(self), scrolled);

  g_signal_connect(add_button, "clicked", G_CALLBACK(on_add_button_clicked),
                   self);
  g_signal_connect(clear_button, "clicked",
                   G_CALLBACK(on_clear_button_clicked), self);
}

KvListView *kv_list_view_new(const KvListViewConfig *config) {
  g_return_val_if_fail(config != NULL, NULL);
  g_return_val_if_fail(config->title != NULL, NULL);
  g_return_val_if_fail(config->add_button_label != NULL, NULL);
  g_return_val_if_fail(config->clear_button_label != NULL, NULL);

  KvListView *self = g_object_new(KV_TYPE_LIST_VIEW, NULL);

  self->key_placeholder =
      g_strdup(config->key_placeholder != NULL ? config->key_placeholder : "");
  self->value_placeholder = g_strdup(
      config->value_placeholder != NULL ? config->value_placeholder : "");
  self->add_prepends = config->add_prepends;

  build_chrome(self, config);

  return self;
}
