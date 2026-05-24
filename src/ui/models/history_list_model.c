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

#include "history_list_model.h"

#include "history_item.h"

struct _HistoryListModel {
  GObject parent_instance;
  HistoryService *service;
  guint last_n_items;
  gulong service_handler;
};

static void history_list_model_iface_init(GListModelInterface *iface);

G_DEFINE_FINAL_TYPE_WITH_CODE(HistoryListModel, history_list_model, G_TYPE_OBJECT,
                              G_IMPLEMENT_INTERFACE(G_TYPE_LIST_MODEL,
                                                    history_list_model_iface_init))

static GType history_list_model_get_item_type(GListModel *model) {
  (void)model;
  return HISTORY_TYPE_ITEM;
}

static guint history_list_model_get_n_items(GListModel *model) {
  HistoryListModel *self = HISTORY_LIST_MODEL(model);
  return (guint)history_service_count(self->service);
}

static gpointer history_list_model_get_item(GListModel *model, guint position) {
  HistoryListModel *self = HISTORY_LIST_MODEL(model);
  HistoryEntry *entry = history_service_get(self->service, position);
  if (entry == NULL) {
    return NULL;
  }
  return history_item_new(entry);
}

static void history_list_model_iface_init(GListModelInterface *iface) {
  iface->get_item_type = history_list_model_get_item_type;
  iface->get_n_items = history_list_model_get_n_items;
  iface->get_item = history_list_model_get_item;
}

static void on_service_changed(HistoryService *service, gpointer user_data) {
  (void)service;
  HistoryListModel *self = HISTORY_LIST_MODEL(user_data);

  guint new_n = (guint)history_service_count(self->service);
  guint old_n = self->last_n_items;
  self->last_n_items = new_n;
  g_list_model_items_changed(G_LIST_MODEL(self), 0, old_n, new_n);
}

HistoryListModel *history_list_model_new(HistoryService *service) {
  g_return_val_if_fail(HISTORY_IS_SERVICE(service), NULL);

  HistoryListModel *self = g_object_new(HISTORY_TYPE_LIST_MODEL, NULL);
  self->service = g_object_ref(service);
  self->last_n_items = (guint)history_service_count(service);
  self->service_handler = g_signal_connect(
      service, "changed", G_CALLBACK(on_service_changed), self);
  return self;
}

static void history_list_model_dispose(GObject *obj) {
  HistoryListModel *self = HISTORY_LIST_MODEL(obj);
  if (self->service != NULL && self->service_handler != 0) {
    g_signal_handler_disconnect(self->service, self->service_handler);
    self->service_handler = 0;
  }
  g_clear_object(&self->service);
  G_OBJECT_CLASS(history_list_model_parent_class)->dispose(obj);
}

static void history_list_model_class_init(HistoryListModelClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = history_list_model_dispose;
}

static void history_list_model_init(HistoryListModel *self) { (void)self; }
