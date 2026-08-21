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

#include "history_service.h"

#include <gio/gio.h>

#define HISTORY_SAVE_DEBOUNCE_SECONDS 2

struct _HistoryService {
  GObject parent_instance;
  HistoryStore *store;
  guint pending_save_id;
  gboolean save_in_flight;
};

G_DEFINE_FINAL_TYPE(HistoryService, history_service, G_TYPE_OBJECT)

enum { SIG_CHANGED, N_SIGNALS };
static guint signals[N_SIGNALS];

static void write_history_thread_func(GTask *task, gpointer source_object,
                                      gpointer task_data,
                                      GCancellable *cancellable) {
  (void)task;
  (void)cancellable;
  HistoryService *self = HISTORY_SERVICE(source_object);
  history_store_write_serialized(self->store, task_data);
}

static void on_save_finished(GObject *source, GAsyncResult *result,
                             gpointer user_data) {
  (void)result;
  (void)user_data;
  HISTORY_SERVICE(source)->save_in_flight = FALSE;
}

/* Serializa na main thread (o store so e mutado aqui) e despacha apenas a
 * escrita em disco para um worker; o refresh do timer garante uma escrita em
 * voo por vez. */
static gboolean on_pending_save(gpointer user_data) {
  HistoryService *self = HISTORY_SERVICE(user_data);

  if (self->save_in_flight) {
    return G_SOURCE_CONTINUE;
  }
  self->pending_save_id = 0;

  gchar *payload = history_store_serialize(self->store);
  if (payload == NULL) {
    return G_SOURCE_REMOVE;
  }

  self->save_in_flight = TRUE;
  GTask *task = g_task_new(self, NULL, on_save_finished, NULL);
  g_task_set_task_data(task, payload, g_free);
  g_task_run_in_thread(task, write_history_thread_func);
  g_object_unref(task);

  return G_SOURCE_REMOVE;
}

static void schedule_save(HistoryService *self) {
  if (self->pending_save_id != 0) {
    return;
  }
  self->pending_save_id = g_timeout_add_seconds(HISTORY_SAVE_DEBOUNCE_SECONDS,
                                                on_pending_save, self);
}

static void emit_changed(HistoryService *self) {
  g_signal_emit(self, signals[SIG_CHANGED], 0);
}

HistoryService *history_service_new(gsize max_entries) {
  HistoryService *self = g_object_new(HISTORY_TYPE_SERVICE, NULL);
  self->store = history_store_new(max_entries);
  history_store_load(self->store);
  return self;
}

gsize history_service_count(HistoryService *self) {
  g_return_val_if_fail(HISTORY_IS_SERVICE(self), 0);
  return history_store_count(self->store);
}

HistoryEntry *history_service_get(HistoryService *self, gsize index) {
  g_return_val_if_fail(HISTORY_IS_SERVICE(self), NULL);
  return history_store_get(self->store, index);
}


void history_service_record(HistoryService *self, HistoryEntry *entry) {
  g_return_if_fail(HISTORY_IS_SERVICE(self));
  if (entry == NULL) {
    return;
  }

  HistoryEntry *existing =
      history_store_find_by_request(self->store, entry->url, entry->method);

  if (existing == entry) {
    history_store_promote(self->store, entry);
  } else if (existing != NULL) {
    history_entry_move_content_from(existing, entry);
    history_entry_free(entry);
    history_store_promote(self->store, existing);
  } else {
    history_store_prepend(self->store, entry);
  }

  schedule_save(self);
  emit_changed(self);
}

void history_service_remove(HistoryService *self, HistoryEntry *entry) {
  g_return_if_fail(HISTORY_IS_SERVICE(self));
  if (entry == NULL) {
    return;
  }
  if (history_store_remove(self->store, entry)) {
    schedule_save(self);
    emit_changed(self);
  }
}

void history_service_clear(HistoryService *self) {
  g_return_if_fail(HISTORY_IS_SERVICE(self));
  if (history_store_count(self->store) == 0) {
    return;
  }
  history_store_clear(self->store);
  schedule_save(self);
  emit_changed(self);
}

static void history_service_dispose(GObject *obj) {
  HistoryService *self = HISTORY_SERVICE(obj);
  if (self->pending_save_id != 0) {
    g_source_remove(self->pending_save_id);
    self->pending_save_id = 0;
  }
  G_OBJECT_CLASS(history_service_parent_class)->dispose(obj);
}

static void history_service_finalize(GObject *obj) {
  HistoryService *self = HISTORY_SERVICE(obj);
  if (self->store != NULL) {
    history_store_save(self->store);
    history_store_free(self->store);
    self->store = NULL;
  }
  G_OBJECT_CLASS(history_service_parent_class)->finalize(obj);
}

static void history_service_class_init(HistoryServiceClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = history_service_dispose;
  G_OBJECT_CLASS(klass)->finalize = history_service_finalize;

  signals[SIG_CHANGED] =
      g_signal_new("changed", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_LAST, 0,
                   NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void history_service_init(HistoryService *self) { (void)self; }
