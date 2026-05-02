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

#ifndef HISTORY_H
#define HISTORY_H

#include "../http/methods.h"
#include "../http/response.h"

#include <glib.h>

#define HISTORY_DEFAULT_MAX_ENTRIES 200

typedef struct {
  gchar *id;
  HttpMethods method;
  gchar *url;
  gchar *body;
  GPtrArray *headers;
  GPtrArray *query_params;
  gint64 timestamp_ms;
  glong http_status;
  gdouble total_time_s;
  gsize response_size;
  gchar *response_body;
  gchar *response_content_type;
} HistoryEntry;

typedef struct _HistoryStore HistoryStore;

HistoryEntry *history_entry_new(void);
void history_entry_free(HistoryEntry *entry);

void history_entry_set_url(HistoryEntry *entry, const char *url);
void history_entry_set_body(HistoryEntry *entry, const char *body);
void history_entry_add_header(HistoryEntry *entry, const char *key,
                              const char *value);
void history_entry_add_query_param(HistoryEntry *entry, const char *key,
                                   const char *value);
void history_entry_apply_response(HistoryEntry *entry, const HttpResponse *resp);
void history_entry_take_payload(HistoryEntry *dst, HistoryEntry *src);

guint history_entry_headers_count(const HistoryEntry *entry);
guint history_entry_query_count(const HistoryEntry *entry);
const char *history_entry_header_key(const HistoryEntry *entry, guint index);
const char *history_entry_header_value(const HistoryEntry *entry, guint index);
const char *history_entry_query_key(const HistoryEntry *entry, guint index);
const char *history_entry_query_value(const HistoryEntry *entry, guint index);

HistoryStore *history_store_new(gsize max_entries);
void history_store_free(HistoryStore *store);

gboolean history_store_load(HistoryStore *store);
gboolean history_store_save(const HistoryStore *store);

void history_store_prepend(HistoryStore *store, HistoryEntry *entry);
gboolean history_store_remove(HistoryStore *store, HistoryEntry *entry);
gsize history_store_count(const HistoryStore *store);
HistoryEntry *history_store_get(const HistoryStore *store, gsize index);
gboolean history_store_evicted_after_prepend(gsize before, gsize after);
void history_store_clear(HistoryStore *store);
HistoryEntry *history_store_find_by_request(const HistoryStore *store,
                                            const char *url,
                                            HttpMethods method);
gboolean history_store_promote(HistoryStore *store, HistoryEntry *entry);

#endif
