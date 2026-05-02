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

#include "history_sidebar.h"

#include "../../http/methods.h"
#include "../../utils/format_response.h"

#define HISTORY_ENTRY_DATA_KEY "history-entry-ptr"
#define HISTORY_MAX_ENTRIES 200

struct _HistorySidebar {
  GtkBox parent_instance;
  HistoryStore *store;
  GtkListBox *list_box;
  GtkStack *stack;
  GtkLabel *count_label;
};

G_DEFINE_TYPE(HistorySidebar, history_sidebar, GTK_TYPE_BOX)

enum { ENTRY_SELECTED, N_SIGNALS };
static guint signals[N_SIGNALS];

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

static gchar *format_relative_time(gint64 timestamp_ms) {
  if (timestamp_ms <= 0) {
    return g_strdup("");
  }

  gint64 now_ms = g_get_real_time() / 1000;
  gint64 diff_s = (now_ms - timestamp_ms) / 1000;
  if (diff_s < 0) {
    diff_s = 0;
  }

  if (diff_s < 60) {
    return g_strdup_printf("%lds ago", (long)diff_s);
  }
  if (diff_s < 3600) {
    return g_strdup_printf("%ldm ago", (long)(diff_s / 60));
  }
  if (diff_s < 86400) {
    return g_strdup_printf("%ldh ago", (long)(diff_s / 3600));
  }
  return g_strdup_printf("%ldd ago", (long)(diff_s / 86400));
}

static void update_count_label(HistorySidebar *self) {
  if (self->count_label == NULL) {
    return;
  }

  gsize count = history_store_count(self->store);
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
  HistorySidebar *self = HISTORY_SIDEBAR(user_data);

  GtkWidget *row_container =
      gtk_widget_get_ancestor(GTK_WIDGET(btn), GTK_TYPE_LIST_BOX_ROW);
  if (row_container == NULL) {
    return;
  }

  GtkWidget *child =
      gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row_container));
  if (child == NULL) {
    return;
  }

  HistoryEntry *entry =
      g_object_get_data(G_OBJECT(child), HISTORY_ENTRY_DATA_KEY);
  if (entry == NULL) {
    return;
  }

  gtk_list_box_remove(self->list_box, row_container);
  history_store_remove(self->store, entry);
  history_store_save(self->store);
  update_count_label(self);
}

static GtkWidget *build_row_widget(HistorySidebar *self, HistoryEntry *entry) {
  GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_widget_set_margin_start(outer, 8);
  gtk_widget_set_margin_end(outer, 8);
  gtk_widget_set_margin_top(outer, 6);
  gtk_widget_set_margin_bottom(outer, 6);
  gtk_widget_add_css_class(outer, "history-row");

  GtkWidget *top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

  GtkWidget *method_label = gtk_label_new(method_to_string(entry->method));
  gtk_widget_add_css_class(method_label, "method-badge");
  gtk_widget_add_css_class(method_label, method_css_class(entry->method));
  gtk_widget_set_halign(method_label, GTK_ALIGN_START);
  gtk_widget_set_valign(method_label, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(top), method_label);

  GtkWidget *url_label = gtk_label_new(entry->url != NULL ? entry->url : "");
  gtk_label_set_xalign(GTK_LABEL(url_label), 0.0);
  gtk_label_set_ellipsize(GTK_LABEL(url_label), PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand(url_label, TRUE);
  gtk_widget_set_halign(url_label, GTK_ALIGN_FILL);
  gtk_widget_set_tooltip_text(url_label, entry->url != NULL ? entry->url : "");
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

  char status_buf[16];
  if (entry->http_status > 0) {
    g_snprintf(status_buf, sizeof(status_buf), "%ld", entry->http_status);
  } else {
    g_strlcpy(status_buf, "—", sizeof(status_buf));
  }

  GtkWidget *status_label = gtk_label_new(status_buf);
  gtk_widget_add_css_class(status_label, "history-status");
  gtk_widget_add_css_class(status_label, status_css_class(entry->http_status));
  gtk_box_append(GTK_BOX(bottom), status_label);

  gchar *time_str = format_response_time(entry->total_time_s * 1000.0);
  GtkWidget *time_label = gtk_label_new(time_str);
  gtk_widget_add_css_class(time_label, "history-meta");
  g_free(time_str);
  gtk_box_append(GTK_BOX(bottom), time_label);

  gchar *size_str = format_response_size(entry->response_size);
  GtkWidget *size_label = gtk_label_new(size_str);
  gtk_widget_add_css_class(size_label, "history-meta");
  g_free(size_str);
  gtk_box_append(GTK_BOX(bottom), size_label);

  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(bottom), spacer);

  gchar *relative = format_relative_time(entry->timestamp_ms);
  GtkWidget *time_ago_label = gtk_label_new(relative);
  gtk_widget_add_css_class(time_ago_label, "history-meta");
  g_free(relative);
  gtk_box_append(GTK_BOX(bottom), time_ago_label);

  gtk_box_append(GTK_BOX(outer), bottom);

  g_object_set_data(G_OBJECT(outer), HISTORY_ENTRY_DATA_KEY, entry);
  return outer;
}

static void prepend_row_for_entry(HistorySidebar *self, HistoryEntry *entry) {
  GtkWidget *row = build_row_widget(self, entry);
  gtk_list_box_prepend(self->list_box, row);
}

static void append_row_for_entry(HistorySidebar *self, HistoryEntry *entry) {
  GtkWidget *row = build_row_widget(self, entry);
  gtk_list_box_append(self->list_box, row);
}

static GtkListBoxRow *find_row_for_entry(HistorySidebar *self,
                                         const HistoryEntry *entry) {
  GtkWidget *row = gtk_widget_get_first_child(GTK_WIDGET(self->list_box));
  while (row != NULL) {
    if (GTK_IS_LIST_BOX_ROW(row)) {
      GtkWidget *child = gtk_list_box_row_get_child(GTK_LIST_BOX_ROW(row));
      if (child != NULL) {
        const HistoryEntry *bound =
            g_object_get_data(G_OBJECT(child), HISTORY_ENTRY_DATA_KEY);
        if (bound == entry) {
          return GTK_LIST_BOX_ROW(row);
        }
      }
    }
    row = gtk_widget_get_next_sibling(row);
  }
  return NULL;
}

static void rebuild_rows(HistorySidebar *self) {
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(GTK_WIDGET(self->list_box))) !=
         NULL) {
    gtk_list_box_remove(self->list_box, child);
  }

  gsize count = history_store_count(self->store);
  for (gsize i = 0; i < count; i++) {
    HistoryEntry *entry = history_store_get(self->store, i);
    if (entry != NULL) {
      append_row_for_entry(self, entry);
    }
  }

  update_count_label(self);
}

static void on_row_activated(GtkListBox *list_box, GtkListBoxRow *row,
                             gpointer user_data) {
  (void)list_box;
  HistorySidebar *self = HISTORY_SIDEBAR(user_data);

  if (row == NULL) {
    return;
  }

  GtkWidget *child = gtk_list_box_row_get_child(row);
  if (child == NULL) {
    return;
  }

  HistoryEntry *entry =
      g_object_get_data(G_OBJECT(child), HISTORY_ENTRY_DATA_KEY);
  if (entry != NULL) {
    g_signal_emit(self, signals[ENTRY_SELECTED], 0, entry);
  }
}

static void on_clear_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;
  HistorySidebar *self = HISTORY_SIDEBAR(user_data);

  history_store_clear(self->store);

  GtkWidget *child;
  while ((child = gtk_widget_get_first_child(GTK_WIDGET(self->list_box))) !=
         NULL) {
    gtk_list_box_remove(self->list_box, child);
  }

  history_store_save(self->store);
  update_count_label(self);
}

static void history_sidebar_finalize(GObject *object) {
  HistorySidebar *self = HISTORY_SIDEBAR(object);
  if (self->store != NULL) {
    history_store_save(self->store);
    history_store_free(self->store);
    self->store = NULL;
  }
  G_OBJECT_CLASS(history_sidebar_parent_class)->finalize(object);
}

static void history_sidebar_class_init(HistorySidebarClass *klass) {
  G_OBJECT_CLASS(klass)->finalize = history_sidebar_finalize;

  signals[ENTRY_SELECTED] = g_signal_new(
      "entry-selected", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST, 0, NULL,
      NULL, NULL, G_TYPE_NONE, 1, G_TYPE_POINTER);
}

static void history_sidebar_init(HistorySidebar *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_size_request(GTK_WIDGET(self), HISTORY_SIDEBAR_WIDTH, -1);
  gtk_widget_add_css_class(GTK_WIDGET(self), "history-sidebar");

  self->store = history_store_new(HISTORY_MAX_ENTRIES);

  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_start(header, 12);
  gtk_widget_set_margin_end(header, 12);
  gtk_widget_set_margin_top(header, 12);
  gtk_widget_set_margin_bottom(header, 8);

  GtkWidget *title = gtk_label_new("History");
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

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolled), FALSE);

  self->list_box = GTK_LIST_BOX(gtk_list_box_new());
  gtk_list_box_set_selection_mode(self->list_box, GTK_SELECTION_SINGLE);
  gtk_list_box_set_show_separators(self->list_box, TRUE);
  gtk_list_box_set_activate_on_single_click(self->list_box, TRUE);
  gtk_widget_add_css_class(GTK_WIDGET(self->list_box), "history-list");
  g_signal_connect(self->list_box, "row-activated",
                   G_CALLBACK(on_row_activated), self);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(self->list_box));
  gtk_stack_add_named(self->stack, scrolled, "list");

  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->stack));

  if (history_store_load(self->store)) {
    rebuild_rows(self);
  } else {
    update_count_label(self);
  }
}

HistorySidebar *history_sidebar_new(void) {
  return g_object_new(HISTORY_TYPE_SIDEBAR, NULL);
}

void history_sidebar_record(HistorySidebar *self, HistoryEntry *entry) {
  g_return_if_fail(HISTORY_IS_SIDEBAR(self));
  if (entry == NULL) {
    return;
  }

  HistoryEntry *existing =
      history_store_find_by_request(self->store, entry->url, entry->method);

  if (existing != NULL && existing != entry) {
    history_entry_take_payload(existing, entry);
    history_entry_free(entry);

    GtkListBoxRow *old_row = find_row_for_entry(self, existing);
    if (old_row != NULL) {
      gtk_list_box_remove(self->list_box, GTK_WIDGET(old_row));
    }

    history_store_promote(self->store, existing);
    prepend_row_for_entry(self, existing);
  } else {
    gsize before = history_store_count(self->store);
    history_store_prepend(self->store, entry);
    gsize after = history_store_count(self->store);

    prepend_row_for_entry(self, entry);

    if (history_store_evicted_after_prepend(before, after)) {
      GtkWidget *last = gtk_widget_get_last_child(GTK_WIDGET(self->list_box));
      if (last != NULL) {
        gtk_list_box_remove(self->list_box, last);
      }
    }
  }

  history_store_save(self->store);
  update_count_label(self);
}

HistoryStore *history_sidebar_get_store(HistorySidebar *self) {
  g_return_val_if_fail(HISTORY_IS_SIDEBAR(self), NULL);
  return self->store;
}
