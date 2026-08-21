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

#include "request_state_codec.h"

RequestState *request_state_from_history_entry(const HistoryEntry *entry) {
  if (entry == NULL) {
    return NULL;
  }

  RequestState *state = request_state_new();
  request_state_set_method(state, entry->method);
  request_state_set_url(state, entry->url);
  request_state_set_body(state, entry->body);

  guint header_count = history_entry_headers_count(entry);
  for (guint i = 0; i < header_count; i++) {
    const char *key = history_entry_header_key(entry, i);
    const char *value = history_entry_header_value(entry, i);
    if (key != NULL) {
      request_state_add_header(state, key, value != NULL ? value : "");
    }
  }

  guint query_count = history_entry_query_count(entry);
  for (guint i = 0; i < query_count; i++) {
    const char *key = history_entry_query_key(entry, i);
    const char *value = history_entry_query_value(entry, i);
    if (key != NULL) {
      request_state_add_query(state, key, value != NULL ? value : "");
    }
  }

  return state;
}

HistoryEntry *history_entry_from_request_state(const RequestState *state) {
  if (state == NULL) {
    return NULL;
  }

  HistoryEntry *entry = history_entry_new();

  entry->method = request_state_get_method(state);
  history_entry_set_url(entry, request_state_get_url(state));

  const char *body = request_state_get_body(state);
  if (body != NULL && *body != '\0') {
    history_entry_set_body(entry, body);
  }

  guint header_count = request_state_headers_count(state);
  for (guint i = 0; i < header_count; i++) {
    const char *key = request_state_header_key(state, i);
    const char *value = request_state_header_value(state, i);
    if (key != NULL) {
      history_entry_add_header(entry, key, value != NULL ? value : "");
    }
  }

  guint query_count = request_state_query_count(state);
  for (guint i = 0; i < query_count; i++) {
    const char *key = request_state_query_key(state, i);
    const char *value = request_state_query_value(state, i);
    if (key != NULL) {
      history_entry_add_query_param(entry, key, value != NULL ? value : "");
    }
  }

  return entry;
}
