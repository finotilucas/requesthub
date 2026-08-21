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

#include "history_controller.h"

#include "../http/response.h"

struct _HistoryController {
  GObject parent_instance;
  RequestView *request_view;
  ResponseView *response_view;
};

G_DEFINE_FINAL_TYPE(HistoryController, history_controller, G_TYPE_OBJECT)

static HttpResponse *response_snapshot_from_entry(const HistoryEntry *entry) {
  if (entry == NULL || entry->http_status <= 0) {
    return NULL;
  }
  return http_response_new_snapshot(entry->http_status, entry->total_time_s,
                                    entry->response_body,
                                    entry->response_content_type);
}

static void on_history_entry_selected(HistoryView *view, gpointer entry_ptr,
                                      gpointer user_data) {
  (void)view;
  HistoryController *self = HISTORY_CONTROLLER(user_data);
  HistoryEntry *entry = entry_ptr;

  if (entry == NULL) {
    return;
  }

  if (self->request_view != NULL) {
    request_view_apply_request(self->request_view, &entry->request);
  }

  if (self->response_view != NULL) {
    HttpResponse *snapshot = response_snapshot_from_entry(entry);
    response_view_update(self->response_view, snapshot);
    if (snapshot != NULL) {
      http_response_free(snapshot);
    }
  }
}

HistoryController *history_controller_new(HistoryView *history_view,
                                          RequestView *request_view,
                                          ResponseView *response_view) {
  g_return_val_if_fail(HISTORY_IS_VIEW(history_view), NULL);

  HistoryController *self = g_object_new(HISTORY_TYPE_CONTROLLER, NULL);
  self->request_view = request_view;
  self->response_view = response_view;

  g_signal_connect_object(history_view, "entry-selected",
                          G_CALLBACK(on_history_entry_selected), self, 0);
  return self;
}

static void history_controller_class_init(HistoryControllerClass *klass) {
  (void)klass;
}

static void history_controller_init(HistoryController *self) { (void)self; }
