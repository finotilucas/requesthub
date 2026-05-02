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

#include "history.h"

#include <cjson/cJSON.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include <string.h>

#define HISTORY_DIR_NAME "requesthub"
#define HISTORY_FILE_NAME "history.json"
#define HISTORY_DIR_PERM 0700
#define HISTORY_FILE_PERM 0600
#define HISTORY_MAX_RESPONSE_BODY (512 * 1024)

struct _HistoryStore {
  GPtrArray *entries;
  gsize max_entries;
  gchar *file_path;
};

static gboolean header_key_is_sensitive(const char *key) {
  return key != NULL && g_ascii_strcasecmp(key, "Authorization") == 0;
}

static gchar *resolve_default_path(void) {
  const gchar *data_home = g_get_user_data_dir();
  return g_build_filename(data_home, HISTORY_DIR_NAME, HISTORY_FILE_NAME, NULL);
}

static gboolean ensure_storage_dir(const gchar *file_path) {
  if (file_path == NULL) {
    return FALSE;
  }

  gchar *dir = g_path_get_dirname(file_path);
  gboolean ok = g_mkdir_with_parents(dir, HISTORY_DIR_PERM) == 0;
  if (ok) {
    g_chmod(dir, HISTORY_DIR_PERM);
  }
  g_free(dir);
  return ok;
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

void history_entry_take_payload(HistoryEntry *dst, HistoryEntry *src) {
  if (dst == NULL || src == NULL || dst == src) {
    return;
  }

  dst->method = src->method;

  g_free(dst->url);
  dst->url = src->url;
  src->url = NULL;

  g_free(dst->body);
  dst->body = src->body;
  src->body = NULL;

  if (dst->headers != NULL) {
    g_ptr_array_unref(dst->headers);
  }
  dst->headers = src->headers;
  src->headers = NULL;

  if (dst->query_params != NULL) {
    g_ptr_array_unref(dst->query_params);
  }
  dst->query_params = src->query_params;
  src->query_params = NULL;

  g_free(dst->response_body);
  dst->response_body = src->response_body;
  src->response_body = NULL;

  g_free(dst->response_content_type);
  dst->response_content_type = src->response_content_type;
  src->response_content_type = NULL;

  dst->timestamp_ms = src->timestamp_ms;
  dst->http_status = src->http_status;
  dst->total_time_s = src->total_time_s;
  dst->response_size = src->response_size;
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
    gsize cache_max = (gsize)HISTORY_MAX_RESPONSE_BODY;
    gsize cache_len =
        resp->body_size < cache_max ? resp->body_size : cache_max;

    const gchar *end_ptr = resp->body;
    g_utf8_validate(resp->body, (gssize)cache_len, &end_ptr);
    cache_len = (gsize)(end_ptr - resp->body);

    if (cache_len > 0) {
      entry->response_body = g_strndup(resp->body, cache_len);
    }
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
  if (entry == NULL || entry->headers == NULL) {
    return NULL;
  }
  guint slot = index * 2;
  if (slot >= entry->headers->len) {
    return NULL;
  }
  return g_ptr_array_index(entry->headers, slot);
}

const char *history_entry_header_value(const HistoryEntry *entry, guint index) {
  if (entry == NULL || entry->headers == NULL) {
    return NULL;
  }
  guint slot = index * 2 + 1;
  if (slot >= entry->headers->len) {
    return NULL;
  }
  return g_ptr_array_index(entry->headers, slot);
}

const char *history_entry_query_key(const HistoryEntry *entry, guint index) {
  if (entry == NULL || entry->query_params == NULL) {
    return NULL;
  }
  guint slot = index * 2;
  if (slot >= entry->query_params->len) {
    return NULL;
  }
  return g_ptr_array_index(entry->query_params, slot);
}

const char *history_entry_query_value(const HistoryEntry *entry, guint index) {
  if (entry == NULL || entry->query_params == NULL) {
    return NULL;
  }
  guint slot = index * 2 + 1;
  if (slot >= entry->query_params->len) {
    return NULL;
  }
  return g_ptr_array_index(entry->query_params, slot);
}

HistoryStore *history_store_new(gsize max_entries) {
  HistoryStore *store = g_new0(HistoryStore, 1);
  store->entries =
      g_ptr_array_new_with_free_func((GDestroyNotify)history_entry_free);
  store->max_entries =
      max_entries > 0 ? max_entries : HISTORY_DEFAULT_MAX_ENTRIES;
  store->file_path = resolve_default_path();
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
                                            HttpMethods method) {
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

gboolean history_store_evicted_after_prepend(gsize before, gsize after) {
  return after <= before;
}

void history_store_clear(HistoryStore *store) {
  if (store == NULL || store->entries == NULL) {
    return;
  }
  g_ptr_array_set_size(store->entries, 0);
}

static cJSON *entry_to_json(const HistoryEntry *entry) {
  cJSON *obj = cJSON_CreateObject();
  if (obj == NULL) {
    return NULL;
  }

  cJSON_AddStringToObject(obj, "id", entry->id != NULL ? entry->id : "");
  cJSON_AddNumberToObject(obj, "method", (double)entry->method);
  cJSON_AddStringToObject(obj, "url", entry->url != NULL ? entry->url : "");

  if (entry->body != NULL) {
    cJSON_AddStringToObject(obj, "body", entry->body);
  }

  cJSON *headers = cJSON_AddArrayToObject(obj, "headers");
  if (headers != NULL && entry->headers != NULL) {
    for (guint i = 0; i + 1 < entry->headers->len; i += 2) {
      cJSON *pair = cJSON_CreateObject();
      if (pair == NULL) {
        continue;
      }
      cJSON_AddStringToObject(pair, "k",
                              g_ptr_array_index(entry->headers, i));
      cJSON_AddStringToObject(pair, "v",
                              g_ptr_array_index(entry->headers, i + 1));
      cJSON_AddItemToArray(headers, pair);
    }
  }

  cJSON *queries = cJSON_AddArrayToObject(obj, "query");
  if (queries != NULL && entry->query_params != NULL) {
    for (guint i = 0; i + 1 < entry->query_params->len; i += 2) {
      cJSON *pair = cJSON_CreateObject();
      if (pair == NULL) {
        continue;
      }
      cJSON_AddStringToObject(pair, "k",
                              g_ptr_array_index(entry->query_params, i));
      cJSON_AddStringToObject(pair, "v",
                              g_ptr_array_index(entry->query_params, i + 1));
      cJSON_AddItemToArray(queries, pair);
    }
  }

  cJSON_AddNumberToObject(obj, "ts", (double)entry->timestamp_ms);
  cJSON_AddNumberToObject(obj, "status", (double)entry->http_status);
  cJSON_AddNumberToObject(obj, "time", entry->total_time_s);
  cJSON_AddNumberToObject(obj, "size", (double)entry->response_size);

  if (entry->response_body != NULL) {
    cJSON_AddStringToObject(obj, "rbody", entry->response_body);
  }
  if (entry->response_content_type != NULL) {
    cJSON_AddStringToObject(obj, "rct", entry->response_content_type);
  }

  return obj;
}

static void load_pairs(cJSON *array, HistoryEntry *entry, gboolean is_header) {
  if (array == NULL || !cJSON_IsArray(array)) {
    return;
  }

  cJSON *pair = NULL;
  cJSON_ArrayForEach(pair, array) {
    if (!cJSON_IsObject(pair)) {
      continue;
    }
    cJSON *k = cJSON_GetObjectItemCaseSensitive(pair, "k");
    cJSON *v = cJSON_GetObjectItemCaseSensitive(pair, "v");
    if (!cJSON_IsString(k) || k->valuestring == NULL) {
      continue;
    }
    const char *value_str = (cJSON_IsString(v) && v->valuestring != NULL)
                                ? v->valuestring
                                : "";
    if (is_header) {
      history_entry_add_header(entry, k->valuestring, value_str);
    } else {
      history_entry_add_query_param(entry, k->valuestring, value_str);
    }
  }
}

static HistoryEntry *entry_from_json(const cJSON *obj) {
  if (!cJSON_IsObject(obj)) {
    return NULL;
  }

  HistoryEntry *entry = history_entry_new();
  if (entry == NULL) {
    return NULL;
  }

  cJSON *id = cJSON_GetObjectItemCaseSensitive(obj, "id");
  if (cJSON_IsString(id) && id->valuestring != NULL) {
    g_free(entry->id);
    entry->id = g_strdup(id->valuestring);
  }

  cJSON *method = cJSON_GetObjectItemCaseSensitive(obj, "method");
  if (cJSON_IsNumber(method)) {
    int raw = method->valueint;
    if (raw >= HTTP_GET && raw <= HTTP_OPTIONS) {
      entry->method = (HttpMethods)raw;
    }
  }

  cJSON *url = cJSON_GetObjectItemCaseSensitive(obj, "url");
  if (cJSON_IsString(url) && url->valuestring != NULL) {
    entry->url = g_strdup(url->valuestring);
  }

  cJSON *body = cJSON_GetObjectItemCaseSensitive(obj, "body");
  if (cJSON_IsString(body) && body->valuestring != NULL) {
    entry->body = g_strdup(body->valuestring);
  }

  load_pairs(cJSON_GetObjectItemCaseSensitive(obj, "headers"), entry, TRUE);
  load_pairs(cJSON_GetObjectItemCaseSensitive(obj, "query"), entry, FALSE);

  cJSON *ts = cJSON_GetObjectItemCaseSensitive(obj, "ts");
  if (cJSON_IsNumber(ts)) {
    entry->timestamp_ms = (gint64)ts->valuedouble;
  }

  cJSON *status = cJSON_GetObjectItemCaseSensitive(obj, "status");
  if (cJSON_IsNumber(status)) {
    entry->http_status = (glong)status->valuedouble;
  }

  cJSON *total_time = cJSON_GetObjectItemCaseSensitive(obj, "time");
  if (cJSON_IsNumber(total_time)) {
    entry->total_time_s = total_time->valuedouble;
  }

  cJSON *size = cJSON_GetObjectItemCaseSensitive(obj, "size");
  if (cJSON_IsNumber(size) && size->valuedouble >= 0) {
    entry->response_size = (gsize)size->valuedouble;
  }

  cJSON *response_body = cJSON_GetObjectItemCaseSensitive(obj, "rbody");
  if (cJSON_IsString(response_body) && response_body->valuestring != NULL) {
    entry->response_body = g_strdup(response_body->valuestring);
  }

  cJSON *response_ct = cJSON_GetObjectItemCaseSensitive(obj, "rct");
  if (cJSON_IsString(response_ct) && response_ct->valuestring != NULL) {
    entry->response_content_type = g_strdup(response_ct->valuestring);
  }

  return entry;
}

gboolean history_store_load(HistoryStore *store) {
  if (store == NULL || store->file_path == NULL) {
    return FALSE;
  }

  if (!g_file_test(store->file_path, G_FILE_TEST_EXISTS)) {
    return TRUE;
  }

  gchar *contents = NULL;
  gsize length = 0;
  GError *error = NULL;

  if (!g_file_get_contents(store->file_path, &contents, &length, &error)) {
    if (error != NULL) {
      g_error_free(error);
    }
    return FALSE;
  }

  cJSON *root = cJSON_ParseWithLength(contents, length);
  g_free(contents);

  if (root == NULL || !cJSON_IsArray(root)) {
    if (root != NULL) {
      cJSON_Delete(root);
    }
    return FALSE;
  }

  cJSON *item = NULL;
  cJSON_ArrayForEach(item, root) {
    if (store->entries->len >= store->max_entries) {
      break;
    }
    HistoryEntry *entry = entry_from_json(item);
    if (entry != NULL) {
      g_ptr_array_add(store->entries, entry);
    }
  }

  cJSON_Delete(root);
  return TRUE;
}

gboolean history_store_save(const HistoryStore *store) {
  if (store == NULL || store->file_path == NULL) {
    return FALSE;
  }

  if (!ensure_storage_dir(store->file_path)) {
    return FALSE;
  }

  cJSON *root = cJSON_CreateArray();
  if (root == NULL) {
    return FALSE;
  }

  for (guint i = 0; i < store->entries->len; i++) {
    const HistoryEntry *entry = g_ptr_array_index(store->entries, i);
    cJSON *json = entry_to_json(entry);
    if (json != NULL) {
      cJSON_AddItemToArray(root, json);
    }
  }

  char *serialized = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  if (serialized == NULL) {
    return FALSE;
  }

  GError *error = NULL;
  gboolean ok = g_file_set_contents_full(
      store->file_path, serialized, -1, G_FILE_SET_CONTENTS_CONSISTENT,
      HISTORY_FILE_PERM, &error);

  free(serialized);

  if (!ok) {
    if (error != NULL) {
      g_error_free(error);
    }
    return FALSE;
  }

  g_chmod(store->file_path, HISTORY_FILE_PERM);
  return TRUE;
}
