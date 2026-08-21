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

#pragma once

#include "../http/response.h"
#include "../models/request_data.h"

#include <glib.h>

typedef struct {
  gchar *id;
  RequestData request;
  gint64 timestamp_ms;
  glong http_status;
  gdouble total_time_s;
  gsize response_size;
  gchar *response_body;
  gchar *response_content_type;
} HistoryEntry;

typedef struct _HistoryStore HistoryStore;

HistoryEntry *history_entry_new(void);

/* Copies request into a new entry, filtering sensitive headers
 * (Authorization) and skipping an empty body. */
HistoryEntry *history_entry_new_from_request(const RequestData *request);

void history_entry_free(HistoryEntry *entry);

void history_entry_apply_response(HistoryEntry *entry, const HttpResponse *resp);
/* Moves all content from src to dst (request, response, timestamps),
 * preserving dst->id; src is left empty but valid. */
void history_entry_move_content_from(HistoryEntry *dst, HistoryEntry *src);

HistoryStore *history_store_new(gsize max_entries);
void history_store_free(HistoryStore *store);

gboolean history_store_load(HistoryStore *store);

/* Serializes all entries to the on-disk format; g_free the result. */
gchar *history_store_serialize(const HistoryStore *store);

/* Writes an already-serialized payload to the store file (atomic, 0600).
 * Does not touch the entries — safe off the main thread. */
gboolean history_store_write_serialized(const HistoryStore *store,
                                        const char *payload);
gboolean history_store_save(const HistoryStore *store);

void history_store_prepend(HistoryStore *store, HistoryEntry *entry);
gboolean history_store_remove(HistoryStore *store, HistoryEntry *entry);
gsize history_store_count(const HistoryStore *store);
HistoryEntry *history_store_get(const HistoryStore *store, gsize index);
void history_store_clear(HistoryStore *store);
HistoryEntry *history_store_find_by_request(const HistoryStore *store,
                                            const char *url,
                                            HttpMethod method);
gboolean history_store_promote(HistoryStore *store, HistoryEntry *entry);
