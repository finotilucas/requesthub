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

#include "history_view.h"

#include "../../http/methods.h"
#include "../../utils/format_response.h"
#include "../models/history_item.h"
#include "../models/history_list_model.h"
#include "../../utils/format_time.h"

#define ROW_METHOD_KEY "history-row-method"
#define ROW_URL_KEY "history-row-url"
#define ROW_STATUS_KEY "history-row-status"
#define ROW_TIME_KEY "history-row-time"
#define ROW_SIZE_KEY "history-row-size"
#define ROW_TIME_AGO_KEY "history-row-time-ago"
#define ROW_DELETE_KEY "history-row-delete"
#define DELETE_ENTRY_KEY "history-delete-entry"

struct _HistoryView {
  GtkBox parent_instance;
  HistoryService *service;
  HistoryListModel *model;
  GtkListView *list_view;
  GtkStack *stack;
  GtkLabel *count_label;
  gulong service_handler;
};

G_DEFINE_FINAL_TYPE(HistoryView, history_view, GTK_TYPE_BOX)

enum { ENTRY_SELECTED, N_SIGNALS };
static guint signals[N_SIGNALS];

static const char *const METHOD_CSS_CLASSES[] = {
    "method-get",   "method-post",    "method-put",     "method-delete",
    "method-patch", "method-head",    "method-options", "method-unknown",
};

static const char *const STATUS_CSS_CLASSES[] = {
    "badge-success", "badge-warning", "badge-error", "badge-neutral",
};

static const char *method_css_class(HttpMethods method) {
  switch (method) {
  case HTTP_GET:
    return "method-get";
  case HTTP_POST:
    return "method-post";
  case HTTP_PUT:
    return "method-put";
  case HTTP_DELETE:
    return "method-delete";
  case HTTP_PATCH:
    return "method-patch";
  case HTTP_HEAD:
    return "method-head";
  case HTTP_OPTIONS:
    return "method-options";
  }
  return "method-unknown";
}

static const char *status_css_class(glong status) {
  if (status >= 200 && status < 300) {
    return "badge-success";
  }
  if (status >= 400 && status < 500) {
    return "badge-warning";
  }
  if (status >= 500) {
    return "badge-error";
  }
  return "badge-neutral";
}

static void replace_css_class(GtkWidget *widget, const char *const *all,
                              gsize n_all, const char *new_class) {
  for (gsize i = 0; i < n_all; i++) {
    gtk_widget_remove_css_class(widget, all[i]);
  }
  if (new_class != NULL) {
    gtk_widget_add_css_class(widget, new_class);
  }
}

static void update_count_label(HistoryView *self) {
  if (self->count_label == NULL) {
    return;
  }

  gsize count = history_service_count(self->service);
  if (count == 0) {
    gtk_widget_set_visible(GTK_WIDGET(self->count_label), FALSE);
    if (self->stack != NULL) {
      gtk_stack_set_visible_child_name(self->stack, "empty");
    }
    return;
  }

  char buf[32];
  g_snprintf(buf, sizeof(buf), "%zu", count);
  gtk_label_set_text(self->count_label, buf);
  gtk_widget_set_visible(GTK_WIDGET(self->count_label), TRUE);

  if (self->stack != NULL) {
    gtk_stack_set_visible_child_name(self->stack, "list");
  }
}

static void on_row_delete_clicked(GtkButton *btn, gpointer user_data) {
  HistoryView *self = HISTORY_VIEW(user_data);
  HistoryEntry *entry = g_object_get_data(G_OBJECT(btn), DELETE_ENTRY_KEY);
  if (entry == NULL || self->service == NULL) {
    return;
  }
  history_service_remove(self->service, entry);
}

static GtkWidget *build_row_template(HistoryView *self) {
  GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(outer, 8);
  gtk_widget_set_margin_end(outer, 8);
  gtk_widget_set_margin_top(outer, 6);
  gtk_widget_set_margin_bottom(outer, 6);
  gtk_widget_add_css_class(outer, "history-row");

  GtkWidget *top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

  GtkWidget *method_label = gtk_label_new("");
  gtk_widget_add_css_class(method_label, "method-badge");
  gtk_widget_set_halign(method_label, GTK_ALIGN_START);
  gtk_widget_set_valign(method_label, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(top), method_label);

  GtkWidget *url_label = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(url_label), 0.0);
  gtk_label_set_ellipsize(GTK_LABEL(url_label), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand(url_label, TRUE);
  gtk_widget_set_halign(url_label, GTK_ALIGN_FILL);
  gtk_widget_add_css_class(url_label, "history-url");
  gtk_box_append(GTK_BOX(top), url_label);

  GtkWidget *delete_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
  gtk_widget_add_css_class(delete_btn, "flat");
  gtk_widget_add_css_class(delete_btn, "history-row-delete");
  gtk_widget_set_tooltip_text(delete_btn, "Remove entry");
  gtk_widget_set_valign(delete_btn, GTK_ALIGN_CENTER);
  g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_row_delete_clicked),
                   self);
  gtk_box_append(GTK_BOX(top), delete_btn);

  gtk_box_append(GTK_BOX(outer), top);

  GtkWidget *bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

  GtkWidget *status_label = gtk_label_new("");
  gtk_widget_add_css_class(status_label, "history-status");
  gtk_box_append(GTK_BOX(bottom), status_label);

  GtkWidget *time_label = gtk_label_new("");
  gtk_widget_add_css_class(time_label, "history-meta");
  gtk_box_append(GTK_BOX(bottom), time_label);

  GtkWidget *size_label = gtk_label_new("");
  gtk_widget_add_css_class(size_label, "history-meta");
  gtk_box_append(GTK_BOX(bottom), size_label);

  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(bottom), spacer);

  GtkWidget *time_ago_label = gtk_label_new("");
  gtk_widget_add_css_class(time_ago_label, "history-meta");
  gtk_box_append(GTK_BOX(bottom), time_ago_label);

  gtk_box_append(GTK_BOX(outer), bottom);

  g_object_set_data(G_OBJECT(outer), ROW_METHOD_KEY, method_label);
  g_object_set_data(G_OBJECT(outer), ROW_URL_KEY, url_label);
  g_object_set_data(G_OBJECT(outer), ROW_STATUS_KEY, status_label);
  g_object_set_data(G_OBJECT(outer), ROW_TIME_KEY, time_label);
  g_object_set_data(G_OBJECT(outer), ROW_SIZE_KEY, size_label);
  g_object_set_data(G_OBJECT(outer), ROW_TIME_AGO_KEY, time_ago_label);
  g_object_set_data(G_OBJECT(outer), ROW_DELETE_KEY, delete_btn);

  return outer;
}

static void on_factory_setup(GtkSignalListItemFactory *factory,
                             GObject *list_item, gpointer user_data) {
  (void)factory;
  HistoryView *self = HISTORY_VIEW(user_data);
  GtkWidget *row = build_row_template(self);
  gtk_list_item_set_child(GTK_LIST_ITEM(list_item), row);
}

static void on_factory_bind(GtkSignalListItemFactory *factory,
                            GObject *list_item, gpointer user_data) {
  (void)factory;
  (void)user_data;

  GObject *item = gtk_list_item_get_item(GTK_LIST_ITEM(list_item));
  if (!HISTORY_IS_ITEM(item)) {
    return;
  }
  HistoryEntry *entry = history_item_get_entry(HISTORY_ITEM(item));
  if (entry == NULL) {
    return;
  }

  GtkWidget *row = gtk_list_item_get_child(GTK_LIST_ITEM(list_item));
  if (row == NULL) {
    return;
  }

  GtkWidget *method_label = g_object_get_data(G_OBJECT(row), ROW_METHOD_KEY);
  GtkWidget *url_label = g_object_get_data(G_OBJECT(row), ROW_URL_KEY);
  GtkWidget *status_label = g_object_get_data(G_OBJECT(row), ROW_STATUS_KEY);
  GtkWidget *time_label = g_object_get_data(G_OBJECT(row), ROW_TIME_KEY);
  GtkWidget *size_label = g_object_get_data(G_OBJECT(row), ROW_SIZE_KEY);
  GtkWidget *time_ago_label =
      g_object_get_data(G_OBJECT(row), ROW_TIME_AGO_KEY);
  GtkWidget *delete_btn = g_object_get_data(G_OBJECT(row), ROW_DELETE_KEY);

  gtk_label_set_text(GTK_LABEL(method_label), method_to_string(entry->method));
  replace_css_class(method_label, METHOD_CSS_CLASSES,
                    G_N_ELEMENTS(METHOD_CSS_CLASSES),
                    method_css_class(entry->method));

  const char *url_text = entry->url != NULL ? entry->url : "";
  gtk_label_set_text(GTK_LABEL(url_label), url_text);
  gtk_widget_set_tooltip_text(url_label, url_text);

  char status_buf[16];
  if (entry->http_status > 0) {
    g_snprintf(status_buf, sizeof(status_buf), "%ld", entry->http_status);
  } else {
    g_strlcpy(status_buf, "—", sizeof(status_buf));
  }
  gtk_label_set_text(GTK_LABEL(status_label), status_buf);
  replace_css_class(status_label, STATUS_CSS_CLASSES,
                    G_N_ELEMENTS(STATUS_CSS_CLASSES),
                    status_css_class(entry->http_status));

  gchar *time_str = format_response_time(entry->total_time_s * 1000.0);
  gtk_label_set_text(GTK_LABEL(time_label), time_str);
  g_free(time_str);

  gchar *size_str = format_response_size(entry->response_size);
  gtk_label_set_text(GTK_LABEL(size_label), size_str);
  g_free(size_str);

  gchar *relative = format_relative_time(entry->timestamp_ms);
  gtk_label_set_text(GTK_LABEL(time_ago_label), relative);
  g_free(relative);

  /* Delete-button needs to know the current entry. Cleared on unbind. */
  g_object_set_data(G_OBJECT(delete_btn), DELETE_ENTRY_KEY, entry);
}

static void on_factory_unbind(GtkSignalListItemFactory *factory,
                              GObject *list_item, gpointer user_data) {
  (void)factory;
  (void)user_data;
  GtkWidget *row = gtk_list_item_get_child(GTK_LIST_ITEM(list_item));
  if (row == NULL) {
    return;
  }
  GtkWidget *delete_btn = g_object_get_data(G_OBJECT(row), ROW_DELETE_KEY);
  if (delete_btn != NULL) {
    g_object_set_data(G_OBJECT(delete_btn), DELETE_ENTRY_KEY, NULL);
  }
}

static void on_list_view_activate(GtkListView *view, guint position,
                                  gpointer user_data) {
  (void)view;
  HistoryView *self = HISTORY_VIEW(user_data);
  GObject *item = g_list_model_get_item(G_LIST_MODEL(self->model), position);
  if (item == NULL) {
    return;
  }
  HistoryEntry *entry = history_item_get_entry(HISTORY_ITEM(item));
  if (entry != NULL) {
    g_signal_emit(self, signals[ENTRY_SELECTED], 0, entry);
  }
  g_object_unref(item);
}

static void on_clear_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;
  HistoryView *self = HISTORY_VIEW(user_data);
  history_service_clear(self->service);
}

static void on_service_changed(HistoryService *service, gpointer user_data) {
  (void)service;
  HistoryView *self = HISTORY_VIEW(user_data);
  update_count_label(self);
}

static void history_view_dispose(GObject *object) {
  HistoryView *self = HISTORY_VIEW(object);
  if (self->service != NULL && self->service_handler != 0) {
    g_signal_handler_disconnect(self->service, self->service_handler);
    self->service_handler = 0;
  }
  g_clear_object(&self->model);
  g_clear_object(&self->service);
  G_OBJECT_CLASS(history_view_parent_class)->dispose(object);
}

static void history_view_class_init(HistoryViewClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = history_view_dispose;

  signals[ENTRY_SELECTED] = g_signal_new(
      "entry-selected", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void history_view_init(HistoryView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_size_request(GTK_WIDGET(self), HISTORY_VIEW_DEFAULT_WIDTH, -1);
  gtk_widget_add_css_class(GTK_WIDGET(self), "history-view");

  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(header, 12);
  gtk_widget_set_margin_end(header, 12);
  gtk_widget_set_margin_top(header, 12);
  gtk_widget_set_margin_bottom(header, 8);

  GtkWidget *title = gtk_label_new("Requests");
  gtk_label_set_xalign(GTK_LABEL(title), 0.0);
  gtk_widget_add_css_class(title, "title-4");
  gtk_widget_set_hexpand(title, TRUE);
  gtk_widget_set_halign(title, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(header), title);

  self->count_label = GTK_LABEL(gtk_label_new(""));
  gtk_widget_add_css_class(GTK_WIDGET(self->count_label), "history-count");
  gtk_label_set_xalign(self->count_label, 0.5);
  gtk_box_append(GTK_BOX(header), GTK_WIDGET(self->count_label));

  GtkWidget *clear_btn = gtk_button_new_from_icon_name("user-trash-symbolic");
  gtk_widget_add_css_class(clear_btn, "flat");
  gtk_widget_set_tooltip_text(clear_btn, "Clear history");
  g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_clicked), self);
  gtk_box_append(GTK_BOX(header), clear_btn);

  gtk_box_append(GTK_BOX(self), header);

  GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_box_append(GTK_BOX(self), separator);

  self->stack = GTK_STACK(gtk_stack_new());
  gtk_widget_set_vexpand(GTK_WIDGET(self->stack), TRUE);
  gtk_widget_set_hexpand(GTK_WIDGET(self->stack), TRUE);

  GtkWidget *empty_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_valign(empty_box, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(empty_box, GTK_ALIGN_CENTER);
  GtkWidget *empty_label = gtk_label_new("No requests yet");
  gtk_widget_add_css_class(empty_label, "dim-label");
  gtk_box_append(GTK_BOX(empty_box), empty_label);
  gtk_stack_add_named(self->stack, empty_box, "empty");

  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->stack));
}

HistoryView *history_view_new(HistoryService *service) {
  g_return_val_if_fail(HISTORY_IS_SERVICE(service), NULL);

  HistoryView *self = g_object_new(HISTORY_TYPE_VIEW, NULL);
  self->service = g_object_ref(service);
  self->model = history_list_model_new(service);

  GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(on_factory_setup), self);
  g_signal_connect(factory, "bind", G_CALLBACK(on_factory_bind), self);
  g_signal_connect(factory, "unbind", G_CALLBACK(on_factory_unbind), self);

  /* gtk_single_selection_new() takes ownership of the model ref it is given,
   * so we ref again to keep our own pointer alive. */
  GtkSingleSelection *selection = gtk_single_selection_new(
      G_LIST_MODEL(g_object_ref(self->model)));
  gtk_single_selection_set_autoselect(selection, FALSE);
  gtk_single_selection_set_can_unselect(selection, TRUE);

  self->list_view = GTK_LIST_VIEW(
      gtk_list_view_new(GTK_SELECTION_MODEL(selection), factory));
  gtk_list_view_set_single_click_activate(self->list_view, TRUE);
  gtk_list_view_set_show_separators(self->list_view, TRUE);
  gtk_widget_add_css_class(GTK_WIDGET(self->list_view), "history-list");
  g_signal_connect(self->list_view, "activate",
                   G_CALLBACK(on_list_view_activate), self);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolled), FALSE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(self->list_view));
  gtk_stack_add_named(self->stack, scrolled, "list");

  self->service_handler = g_signal_connect(
      self->service, "changed", G_CALLBACK(on_service_changed), self);

  update_count_label(self);
  return self;
}
