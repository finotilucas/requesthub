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

#include "history_view.h"

#include "../../http/methods.h"
#include "../../utils/format_response.h"

#define HISTORY_VIEW_DEFAULT_WIDTH 260
#define PAGE_EMPTY "empty"
#define PAGE_LIST "list"
#define TRASH_ICON_NAME "user-trash-symbolic"

struct _HistoryView {
  GtkBox parent_instance;
  HistoryService *service;
  GListStore *items; /* de HistoryItem */
  GtkStack *stack;
  GtkLabel *count_label;
  gulong service_handler;
};

enum { ENTRY_SELECTED, N_SIGNALS };
static guint signals[N_SIGNALS];

enum { PROP_SERVICE = 1 };

/* ---- HistoryItem: wrapper GObject de um HistoryEntry emprestado, para o
 * GListStore. O entry pertence ao HistoryService. ---- */

#define HISTORY_TYPE_ITEM (history_item_get_type())
G_DECLARE_FINAL_TYPE(HistoryItem, history_item, HISTORY, ITEM, GObject)

struct _HistoryItem {
  GObject parent_instance;
  HistoryEntry *entry;
};

G_DEFINE_FINAL_TYPE(HistoryItem, history_item, G_TYPE_OBJECT)

static HistoryItem *history_item_new(HistoryEntry *entry) {
  HistoryItem *self = g_object_new(HISTORY_TYPE_ITEM, NULL);
  self->entry = entry;
  return self;
}

static void history_item_class_init(HistoryItemClass *klass) { (void)klass; }
static void history_item_init(HistoryItem *self) { (void)self; }

/* ---- HistoryRow: uma linha da lista, com os filhos como campos ---- */

#define HISTORY_TYPE_ROW (history_row_get_type())
G_DECLARE_FINAL_TYPE(HistoryRow, history_row, HISTORY, ROW, GtkBox)

struct _HistoryRow {
  GtkBox parent_instance;
  GtkLabel *method_label;
  GtkLabel *url_label;
  GtkLabel *status_label;
  GtkLabel *time_label;
  GtkLabel *size_label;
  GtkLabel *time_ago_label;
  HistoryView *view;   /* emprestado; a linha vive dentro da view */
  HistoryEntry *entry; /* emprestado; valido enquanto bound */
};

G_DEFINE_FINAL_TYPE(HistoryRow, history_row, GTK_TYPE_BOX)

static const char *const METHOD_CSS_CLASSES[] = {
    [HTTP_GET] = "method-get",         [HTTP_POST] = "method-post",
    [HTTP_PUT] = "method-put",         [HTTP_DELETE] = "method-delete",
    [HTTP_PATCH] = "method-patch",     [HTTP_HEAD] = "method-head",
    [HTTP_OPTIONS] = "method-options",
};

static const char *method_css_class(HttpMethod method) {
  if ((gsize)method < G_N_ELEMENTS(METHOD_CSS_CLASSES) &&
      METHOD_CSS_CLASSES[method] != NULL) {
    return METHOD_CSS_CLASSES[method];
  }
  return "method-unknown";
}

static const char *const STATUS_CSS_CLASSES[] = {
    "badge-success",
    "badge-warning",
    "badge-error",
    "badge-neutral",
};

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
    if (all[i] != NULL) {
      gtk_widget_remove_css_class(widget, all[i]);
    }
  }
  gtk_widget_remove_css_class(widget, "method-unknown");
  if (new_class != NULL) {
    gtk_widget_add_css_class(widget, new_class);
  }
}

static gchar *relative_time_string(gint64 timestamp_ms) {
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

static void on_row_delete_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;
  HistoryRow *row = HISTORY_ROW(user_data);
  if (row->entry == NULL || row->view == NULL || row->view->service == NULL) {
    return;
  }
  history_service_remove(row->view->service, row->entry);
}

static void history_row_init(HistoryRow *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_box_set_spacing(GTK_BOX(self), 4);
  gtk_widget_set_margin_start(GTK_WIDGET(self), 8);
  gtk_widget_set_margin_end(GTK_WIDGET(self), 8);
  gtk_widget_set_margin_top(GTK_WIDGET(self), 6);
  gtk_widget_set_margin_bottom(GTK_WIDGET(self), 6);
  gtk_widget_add_css_class(GTK_WIDGET(self), "history-row");

  GtkWidget *top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

  self->method_label = GTK_LABEL(gtk_label_new(""));
  gtk_widget_add_css_class(GTK_WIDGET(self->method_label), "method-badge");
  gtk_widget_set_halign(GTK_WIDGET(self->method_label), GTK_ALIGN_START);
  gtk_widget_set_valign(GTK_WIDGET(self->method_label), GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(top), GTK_WIDGET(self->method_label));

  self->url_label = GTK_LABEL(gtk_label_new(""));
  gtk_label_set_xalign(self->url_label, 0.0);
  gtk_label_set_ellipsize(self->url_label, PANGO_ELLIPSIZE_END);
  gtk_widget_set_hexpand(GTK_WIDGET(self->url_label), TRUE);
  gtk_widget_set_halign(GTK_WIDGET(self->url_label), GTK_ALIGN_FILL);
  gtk_widget_add_css_class(GTK_WIDGET(self->url_label), "history-url");
  gtk_box_append(GTK_BOX(top), GTK_WIDGET(self->url_label));

  GtkWidget *delete_btn = gtk_button_new_from_icon_name(TRASH_ICON_NAME);
  gtk_widget_add_css_class(delete_btn, "flat");
  gtk_widget_add_css_class(delete_btn, "history-row-delete");
  gtk_widget_set_tooltip_text(delete_btn, "Remove entry");
  gtk_widget_set_valign(delete_btn, GTK_ALIGN_CENTER);
  g_signal_connect(delete_btn, "clicked", G_CALLBACK(on_row_delete_clicked),
                   self);
  gtk_box_append(GTK_BOX(top), delete_btn);

  gtk_box_append(GTK_BOX(self), top);

  GtkWidget *bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

  self->status_label = GTK_LABEL(gtk_label_new(""));
  gtk_widget_add_css_class(GTK_WIDGET(self->status_label), "history-status");
  gtk_box_append(GTK_BOX(bottom), GTK_WIDGET(self->status_label));

  self->time_label = GTK_LABEL(gtk_label_new(""));
  gtk_widget_add_css_class(GTK_WIDGET(self->time_label), "history-meta");
  gtk_box_append(GTK_BOX(bottom), GTK_WIDGET(self->time_label));

  self->size_label = GTK_LABEL(gtk_label_new(""));
  gtk_widget_add_css_class(GTK_WIDGET(self->size_label), "history-meta");
  gtk_box_append(GTK_BOX(bottom), GTK_WIDGET(self->size_label));

  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(bottom), spacer);

  self->time_ago_label = GTK_LABEL(gtk_label_new(""));
  gtk_widget_add_css_class(GTK_WIDGET(self->time_ago_label), "history-meta");
  gtk_box_append(GTK_BOX(bottom), GTK_WIDGET(self->time_ago_label));

  gtk_box_append(GTK_BOX(self), bottom);
}

static void history_row_class_init(HistoryRowClass *klass) { (void)klass; }

static void history_row_bind(HistoryRow *self, HistoryEntry *entry) {
  self->entry = entry;

  gtk_label_set_text(self->method_label, http_method_to_string(entry->method));
  replace_css_class(GTK_WIDGET(self->method_label), METHOD_CSS_CLASSES,
                    G_N_ELEMENTS(METHOD_CSS_CLASSES),
                    method_css_class(entry->method));

  const char *url_text = entry->url != NULL ? entry->url : "";
  gtk_label_set_text(self->url_label, url_text);
  gtk_widget_set_tooltip_text(GTK_WIDGET(self->url_label), url_text);

  char status_buf[16];
  if (entry->http_status > 0) {
    g_snprintf(status_buf, sizeof(status_buf), "%ld", entry->http_status);
  } else {
    g_strlcpy(status_buf, "—", sizeof(status_buf));
  }
  gtk_label_set_text(self->status_label, status_buf);
  replace_css_class(GTK_WIDGET(self->status_label), STATUS_CSS_CLASSES,
                    G_N_ELEMENTS(STATUS_CSS_CLASSES),
                    status_css_class(entry->http_status));

  gchar *time_str = format_response_time(entry->total_time_s);
  gtk_label_set_text(self->time_label, time_str);
  g_free(time_str);

  gchar *size_str = format_response_size(entry->response_size);
  gtk_label_set_text(self->size_label, size_str);
  g_free(size_str);

  gchar *relative = relative_time_string(entry->timestamp_ms);
  gtk_label_set_text(self->time_ago_label, relative);
  g_free(relative);
}

/* ---- HistoryView ---- */

G_DEFINE_FINAL_TYPE(HistoryView, history_view, GTK_TYPE_BOX)

static void update_count_and_empty_state(HistoryView *self) {
  if (self->count_label == NULL) {
    return;
  }

  gsize count = history_service_count(self->service);
  if (count == 0) {
    gtk_widget_set_visible(GTK_WIDGET(self->count_label), FALSE);
    gtk_stack_set_visible_child_name(self->stack, PAGE_EMPTY);
    return;
  }

  char buf[32];
  g_snprintf(buf, sizeof(buf), "%zu", count);
  gtk_label_set_text(self->count_label, buf);
  gtk_widget_set_visible(GTK_WIDGET(self->count_label), TRUE);
  gtk_stack_set_visible_child_name(self->stack, PAGE_LIST);
}

/* Reaproveita os wrappers existentes (por ponteiro de entry) para que a
 * identidade dos itens sobreviva a mudancas — e assim a selecao do
 * GtkSingleSelection tambem. */
static void refresh_items(HistoryView *self) {
  GListModel *model = G_LIST_MODEL(self->items);
  guint old_n = g_list_model_get_n_items(model);

  GHashTable *by_entry =
      g_hash_table_new_full(NULL, NULL, NULL, g_object_unref);
  for (guint i = 0; i < old_n; i++) {
    HistoryItem *item = g_list_model_get_item(model, i);
    g_hash_table_insert(by_entry, item->entry, item);
  }

  gsize new_n = history_service_count(self->service);
  GPtrArray *fresh = g_ptr_array_new_full((guint)new_n, g_object_unref);
  for (gsize i = 0; i < new_n; i++) {
    HistoryEntry *entry = history_service_get(self->service, i);
    HistoryItem *existing = g_hash_table_lookup(by_entry, entry);
    g_ptr_array_add(fresh, existing != NULL ? g_object_ref(existing)
                                            : history_item_new(entry));
  }

  g_list_store_splice(self->items, 0, old_n, fresh->pdata, fresh->len);

  g_ptr_array_unref(fresh);
  g_hash_table_destroy(by_entry);
}

static void on_factory_setup(GtkSignalListItemFactory *factory,
                             GObject *list_item, gpointer user_data) {
  (void)factory;
  HistoryView *self = HISTORY_VIEW(user_data);
  HistoryRow *row = g_object_new(HISTORY_TYPE_ROW, NULL);
  row->view = self;
  gtk_list_item_set_child(GTK_LIST_ITEM(list_item), GTK_WIDGET(row));
}

static void on_factory_bind(GtkSignalListItemFactory *factory,
                            GObject *list_item, gpointer user_data) {
  (void)factory;
  (void)user_data;

  HistoryItem *item =
      HISTORY_ITEM(gtk_list_item_get_item(GTK_LIST_ITEM(list_item)));
  HistoryRow *row =
      HISTORY_ROW(gtk_list_item_get_child(GTK_LIST_ITEM(list_item)));
  if (item == NULL || row == NULL || item->entry == NULL) {
    return;
  }
  history_row_bind(row, item->entry);
}

static void on_factory_unbind(GtkSignalListItemFactory *factory,
                              GObject *list_item, gpointer user_data) {
  (void)factory;
  (void)user_data;
  HistoryRow *row =
      HISTORY_ROW(gtk_list_item_get_child(GTK_LIST_ITEM(list_item)));
  if (row != NULL) {
    row->entry = NULL;
  }
}

static void on_list_view_activate(GtkListView *view, guint position,
                                  gpointer user_data) {
  (void)view;
  HistoryView *self = HISTORY_VIEW(user_data);
  HistoryItem *item =
      g_list_model_get_item(G_LIST_MODEL(self->items), position);
  if (item == NULL) {
    return;
  }
  if (item->entry != NULL) {
    g_signal_emit(self, signals[ENTRY_SELECTED], 0, item->entry);
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
  refresh_items(self);
  update_count_and_empty_state(self);
}

static void history_view_set_property(GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec) {
  HistoryView *self = HISTORY_VIEW(object);
  switch (property_id) {
  case PROP_SERVICE:
    self->service = g_value_dup_object(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
  }
}

static void history_view_constructed(GObject *object) {
  HistoryView *self = HISTORY_VIEW(object);
  G_OBJECT_CLASS(history_view_parent_class)->constructed(object);

  g_return_if_fail(HISTORY_IS_SERVICE(self->service));

  self->items = g_list_store_new(HISTORY_TYPE_ITEM);

  GtkListItemFactory *factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", G_CALLBACK(on_factory_setup), self);
  g_signal_connect(factory, "bind", G_CALLBACK(on_factory_bind), self);
  g_signal_connect(factory, "unbind", G_CALLBACK(on_factory_unbind), self);

  /* gtk_single_selection_new() toma posse da ref do modelo passada, entao
   * damos uma ref extra para manter self->items vivo. */
  GtkSingleSelection *selection =
      gtk_single_selection_new(G_LIST_MODEL(g_object_ref(self->items)));
  gtk_single_selection_set_autoselect(selection, FALSE);
  gtk_single_selection_set_can_unselect(selection, TRUE);

  GtkListView *list_view =
      GTK_LIST_VIEW(gtk_list_view_new(GTK_SELECTION_MODEL(selection), factory));
  gtk_list_view_set_single_click_activate(list_view, TRUE);
  gtk_list_view_set_show_separators(list_view, TRUE);
  gtk_widget_add_css_class(GTK_WIDGET(list_view), "history-list");
  g_signal_connect(list_view, "activate", G_CALLBACK(on_list_view_activate),
                   self);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(scrolled), FALSE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(list_view));
  gtk_stack_add_named(self->stack, scrolled, PAGE_LIST);

  self->service_handler = g_signal_connect(
      self->service, "changed", G_CALLBACK(on_service_changed), self);

  refresh_items(self);
  update_count_and_empty_state(self);
}

static void history_view_dispose(GObject *object) {
  HistoryView *self = HISTORY_VIEW(object);
  if (self->service != NULL && self->service_handler != 0) {
    g_signal_handler_disconnect(self->service, self->service_handler);
    self->service_handler = 0;
  }
  g_clear_object(&self->items);
  g_clear_object(&self->service);
  G_OBJECT_CLASS(history_view_parent_class)->dispose(object);
}

static void history_view_class_init(HistoryViewClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  object_class->set_property = history_view_set_property;
  object_class->constructed = history_view_constructed;
  object_class->dispose = history_view_dispose;

  g_object_class_install_property(
      object_class, PROP_SERVICE,
      g_param_spec_object("service", NULL, NULL, HISTORY_TYPE_SERVICE,
                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY |
                              G_PARAM_STATIC_STRINGS));

  /* O HistoryEntry* emitido e emprestado do HistoryService e so e valido
   * durante a emissao do sinal. */
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

  GtkWidget *clear_btn = gtk_button_new_from_icon_name(TRASH_ICON_NAME);
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
  gtk_stack_add_named(self->stack, empty_box, PAGE_EMPTY);

  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->stack));
}

HistoryView *history_view_new(HistoryService *service) {
  g_return_val_if_fail(HISTORY_IS_SERVICE(service), NULL);
  return g_object_new(HISTORY_TYPE_VIEW, "service", service, NULL);
}
