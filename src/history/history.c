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

#include "internal.h"

#include <glib.h>

#define HISTORY_DEFAULT_MAX_ENTRIES 200
#define HISTORY_MAX_RESPONSE_BODY (512 * 1024)

static gboolean header_key_is_sensitive(const char *key) {
  return key != NULL && g_ascii_strcasecmp(key, "Authorization") == 0;
}

static const char *kv_array_get(const GPtrArray *pairs, guint index,
                                gboolean want_value) {
  if (pairs == NULL) {
    return NULL;
  }
  guint slot = index * 2 + (want_value ? 1 : 0);
  if (slot >= pairs->len) {
    return NULL;
  }
  return g_ptr_array_index(pairs, slot);
}

HistoryEntry *history_entry_new(void) {
  HistoryEntry *entry = g_new0(HistoryEntry, 1);
  entry->id = g_uuid_string_random();
  entry->method = HTTP_GET;
  entry->headers = g_ptr_array_new_with_free_func(g_free);
  entry->query_params = g_ptr_array_new_with_free_func(g_free);
  entry->timestamp_ms = g_get_real_time() / 1000;
  return entry;
}

void history_entry_free(HistoryEntry *entry) {
  if (entry == NULL) {
    return;
  }

  g_free(entry->id);
  g_free(entry->url);
  g_free(entry->body);
  g_free(entry->response_body);
  g_free(entry->response_content_type);

  if (entry->headers != NULL) {
    g_ptr_array_unref(entry->headers);
  }
  if (entry->query_params != NULL) {
    g_ptr_array_unref(entry->query_params);
  }

  g_free(entry);
}

void history_entry_set_url(HistoryEntry *entry, const char *url) {
  if (entry == NULL) {
    return;
  }

  g_free(entry->url);
  entry->url = (url != NULL) ? g_strdup(url) : NULL;
}

void history_entry_set_body(HistoryEntry *entry, const char *body) {
  if (entry == NULL) {
    return;
  }

  g_free(entry->body);
  entry->body = (body != NULL) ? g_strdup(body) : NULL;
}

void history_entry_add_header(HistoryEntry *entry, const char *key,
                              const char *value) {
  if (entry == NULL || key == NULL || *key == '\0') {
    return;
  }
  if (header_key_is_sensitive(key)) {
    return;
  }

  g_ptr_array_add(entry->headers, g_strdup(key));
  g_ptr_array_add(entry->headers, g_strdup(value != NULL ? value : ""));
}

void history_entry_add_query_param(HistoryEntry *entry, const char *key,
                                   const char *value) {
  if (entry == NULL || key == NULL || *key == '\0') {
    return;
  }

  g_ptr_array_add(entry->query_params, g_strdup(key));
  g_ptr_array_add(entry->query_params, g_strdup(value != NULL ? value : ""));
}

static void take_str(gchar **dst, gchar **src) {
  g_free(*dst);
  *dst = *src;
  *src = NULL;
}

static void take_array(GPtrArray **dst, GPtrArray **src) {
  if (*dst != NULL) {
    g_ptr_array_unref(*dst);
  }
  *dst = *src;
  *src = NULL;
}

void history_entry_move_content_from(HistoryEntry *dst, HistoryEntry *src) {
  if (dst == NULL || src == NULL || dst == src) {
    return;
  }

  dst->method = src->method;
  take_str(&dst->url, &src->url);
  take_str(&dst->body, &src->body);
  take_array(&dst->headers, &src->headers);
  take_array(&dst->query_params, &src->query_params);
  take_str(&dst->response_body, &src->response_body);
  take_str(&dst->response_content_type, &src->response_content_type);

  dst->timestamp_ms = src->timestamp_ms;
  dst->http_status = src->http_status;
  dst->total_time_s = src->total_time_s;
  dst->response_size = src->response_size;
}

/* Corta em no maximo max_len recuando ate o ultimo code point UTF-8 completo:
 * um corte no meio de um code point geraria string invalida que quebraria a
 * serializacao cJSON e os widgets GTK. */
static gchar *copy_utf8_prefix(const char *text, gsize text_len,
                               gsize max_len) {
  gsize len = text_len < max_len ? text_len : max_len;

  const gchar *end_ptr = text;
  g_utf8_validate(text, (gssize)len, &end_ptr);
  len = (gsize)(end_ptr - text);

  return len > 0 ? g_strndup(text, len) : NULL;
}

void history_entry_apply_response(HistoryEntry *entry,
                                  const HttpResponse *resp) {
  if (entry == NULL) {
    return;
  }

  g_free(entry->response_body);
  entry->response_body = NULL;
  g_free(entry->response_content_type);
  entry->response_content_type = NULL;

  if (resp == NULL) {
    entry->http_status = 0;
    entry->total_time_s = 0.0;
    entry->response_size = 0;
    return;
  }

  entry->http_status = resp->http_status;
  entry->total_time_s = resp->total_time;
  entry->response_size = resp->body_size;

  if (resp->content_type != NULL) {
    entry->response_content_type = g_strdup(resp->content_type);
  }

  if (resp->body != NULL && resp->body_size > 0) {
    entry->response_body = copy_utf8_prefix(resp->body, resp->body_size,
                                            HISTORY_MAX_RESPONSE_BODY);
  }
}

guint history_entry_headers_count(const HistoryEntry *entry) {
  if (entry == NULL || entry->headers == NULL) {
    return 0;
  }
  return entry->headers->len / 2;
}

guint history_entry_query_count(const HistoryEntry *entry) {
  if (entry == NULL || entry->query_params == NULL) {
    return 0;
  }
  return entry->query_params->len / 2;
}

const char *history_entry_header_key(const HistoryEntry *entry, guint index) {
  return entry != NULL ? kv_array_get(entry->headers, index, FALSE) : NULL;
}

const char *history_entry_header_value(const HistoryEntry *entry, guint index) {
  return entry != NULL ? kv_array_get(entry->headers, index, TRUE) : NULL;
}

const char *history_entry_query_key(const HistoryEntry *entry, guint index) {
  return entry != NULL ? kv_array_get(entry->query_params, index, FALSE) : NULL;
}

const char *history_entry_query_value(const HistoryEntry *entry, guint index) {
  return entry != NULL ? kv_array_get(entry->query_params, index, TRUE) : NULL;
}

HistoryStore *history_store_new(gsize max_entries) {
  HistoryStore *store = g_new0(HistoryStore, 1);
  store->entries =
      g_ptr_array_new_with_free_func((GDestroyNotify)history_entry_free);
  store->max_entries =
      max_entries > 0 ? max_entries : HISTORY_DEFAULT_MAX_ENTRIES;
  store->file_path = history_default_file_path();
  return store;
}

void history_store_free(HistoryStore *store) {
  if (store == NULL) {
    return;
  }

  if (store->entries != NULL) {
    g_ptr_array_unref(store->entries);
  }
  g_free(store->file_path);
  g_free(store);
}

gsize history_store_count(const HistoryStore *store) {
  if (store == NULL || store->entries == NULL) {
    return 0;
  }
  return store->entries->len;
}

HistoryEntry *history_store_get(const HistoryStore *store, gsize index) {
  if (store == NULL || store->entries == NULL ||
      index >= store->entries->len) {
    return NULL;
  }
  return g_ptr_array_index(store->entries, index);
}

void history_store_prepend(HistoryStore *store, HistoryEntry *entry) {
  if (store == NULL || entry == NULL) {
    return;
  }

  g_ptr_array_insert(store->entries, 0, entry);

  while (store->entries->len > store->max_entries) {
    g_ptr_array_remove_index(store->entries, store->entries->len - 1);
  }
}

gboolean history_store_remove(HistoryStore *store, HistoryEntry *entry) {
  if (store == NULL || entry == NULL || store->entries == NULL) {
    return FALSE;
  }
  return g_ptr_array_remove(store->entries, entry);
}

HistoryEntry *history_store_find_by_request(const HistoryStore *store,
                                            const char *url,
                                            HttpMethod method) {
  if (store == NULL || store->entries == NULL || url == NULL) {
    return NULL;
  }

  for (guint i = 0; i < store->entries->len; i++) {
    HistoryEntry *candidate = g_ptr_array_index(store->entries, i);
    if (candidate == NULL || candidate->method != method) {
      continue;
    }
    if (g_strcmp0(candidate->url, url) == 0) {
      return candidate;
    }
  }
  return NULL;
}

gboolean history_store_promote(HistoryStore *store, HistoryEntry *entry) {
  if (store == NULL || entry == NULL || store->entries == NULL) {
    return FALSE;
  }

  guint pos = 0;
  if (!g_ptr_array_find(store->entries, entry, &pos)) {
    return FALSE;
  }
  if (pos == 0) {
    return TRUE;
  }

  gpointer stolen = g_ptr_array_steal_index(store->entries, pos);
  g_ptr_array_insert(store->entries, 0, stolen);
  return TRUE;
}

void history_store_clear(HistoryStore *store) {
  if (store == NULL || store->entries == NULL) {
    return;
  }
  g_ptr_array_set_size(store->entries, 0);
}
