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

/* On-disk format (FROZEN — changes break existing histories): a compact
 * JSON array; each object uses the fields id/method/url/body/headers/query/
 * ts/status/time/size/rbody/rct, with "method" stored as the HttpMethod
 * ORDINAL (see the comment in src/http/methods.h). */

#include "internal.h"

#include <cjson/cJSON.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <stdlib.h>

#define HISTORY_DIR_NAME "requesthub"
#define HISTORY_FILE_NAME "history.json"
#define HISTORY_DIR_PERM 0700
#define HISTORY_FILE_PERM 0600

gchar *history_default_file_path(void) {
  return g_build_filename(g_get_user_data_dir(), HISTORY_DIR_NAME,
                          HISTORY_FILE_NAME, NULL);
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

static void add_kv_array(cJSON *obj, const char *name,
                         const GPtrArray *pairs) {
  cJSON *array = cJSON_AddArrayToObject(obj, name);
  if (array == NULL || pairs == NULL) {
    return;
  }

  for (guint i = 0; i + 1 < pairs->len; i += 2) {
    cJSON *pair = cJSON_CreateObject();
    if (pair == NULL) {
      continue;
    }
    cJSON_AddStringToObject(pair, "k", g_ptr_array_index(pairs, i));
    cJSON_AddStringToObject(pair, "v", g_ptr_array_index(pairs, i + 1));
    cJSON_AddItemToArray(array, pair);
  }
}

static cJSON *entry_to_json(const HistoryEntry *entry) {
  cJSON *obj = cJSON_CreateObject();
  if (obj == NULL) {
    return NULL;
  }

  cJSON_AddStringToObject(obj, "id", entry->id != NULL ? entry->id : "");
  cJSON_AddNumberToObject(obj, "method", (double)entry->request.method);
  cJSON_AddStringToObject(obj, "url",
                          entry->request.url != NULL ? entry->request.url : "");

  if (entry->request.body != NULL) {
    cJSON_AddStringToObject(obj, "body", entry->request.body);
  }

  add_kv_array(obj, "headers", entry->request.headers);
  add_kv_array(obj, "query", entry->request.query_params);

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

static void add_header_filtered(RequestData *request, const char *key,
                                const char *value) {
  if (!history_header_key_is_sensitive(key)) {
    request_data_add_header(request, key, value);
  }
}

static void request_add_pairs_from_json(RequestData *request, cJSON *array,
                                        void (*add_pair)(RequestData *,
                                                         const char *,
                                                         const char *)) {
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
    const char *value_str =
        (cJSON_IsString(v) && v->valuestring != NULL) ? v->valuestring : "";
    add_pair(request, k->valuestring, value_str);
  }
}

static HistoryEntry *entry_from_json(const cJSON *obj) {
  if (!cJSON_IsObject(obj)) {
    return NULL;
  }

  HistoryEntry *entry = history_entry_new();

  cJSON *id = cJSON_GetObjectItemCaseSensitive(obj, "id");
  if (cJSON_IsString(id) && id->valuestring != NULL) {
    g_free(entry->id);
    entry->id = g_strdup(id->valuestring);
  }

  cJSON *method = cJSON_GetObjectItemCaseSensitive(obj, "method");
  if (cJSON_IsNumber(method)) {
    int raw = method->valueint;
    if (raw >= 0 && raw < HTTP_METHOD_COUNT) {
      entry->request.method = (HttpMethod)raw;
    }
  }

  cJSON *url = cJSON_GetObjectItemCaseSensitive(obj, "url");
  if (cJSON_IsString(url) && url->valuestring != NULL) {
    request_data_set_url(&entry->request, url->valuestring);
  }

  cJSON *body = cJSON_GetObjectItemCaseSensitive(obj, "body");
  if (cJSON_IsString(body) && body->valuestring != NULL) {
    request_data_set_body(&entry->request, body->valuestring);
  }

  request_add_pairs_from_json(&entry->request,
                              cJSON_GetObjectItemCaseSensitive(obj, "headers"),
                              add_header_filtered);
  request_add_pairs_from_json(&entry->request,
                              cJSON_GetObjectItemCaseSensitive(obj, "query"),
                              request_data_add_query);

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
    g_warning("failed to read history file %s: %s", store->file_path,
              error != NULL ? error->message : "unknown error");
    g_clear_error(&error);
    return FALSE;
  }

  cJSON *root = cJSON_ParseWithLength(contents, length);
  g_free(contents);

  if (root == NULL || !cJSON_IsArray(root)) {
    g_warning("history file %s is not a valid JSON array; ignoring it",
              store->file_path);
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

gchar *history_store_serialize(const HistoryStore *store) {
  if (store == NULL || store->entries == NULL) {
    return NULL;
  }

  cJSON *root = cJSON_CreateArray();
  if (root == NULL) {
    return NULL;
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
    return NULL;
  }

  gchar *result = g_strdup(serialized);
  free(serialized);
  return result;
}

gboolean history_store_write_serialized(const HistoryStore *store,
                                        const char *payload) {
  if (store == NULL || store->file_path == NULL || payload == NULL) {
    return FALSE;
  }

  if (!ensure_storage_dir(store->file_path)) {
    g_warning("failed to create history directory for %s", store->file_path);
    return FALSE;
  }

  GError *error = NULL;
  gboolean ok = g_file_set_contents_full(
      store->file_path, payload, -1, G_FILE_SET_CONTENTS_CONSISTENT,
      HISTORY_FILE_PERM, &error);

  if (!ok) {
    g_warning("failed to write history file %s: %s", store->file_path,
              error != NULL ? error->message : "unknown error");
    g_clear_error(&error);
    return FALSE;
  }

  g_chmod(store->file_path, HISTORY_FILE_PERM);
  return TRUE;
}

gboolean history_store_save(const HistoryStore *store) {
  gchar *payload = history_store_serialize(store);
  if (payload == NULL) {
    return FALSE;
  }
  gboolean ok = history_store_write_serialized(store, payload);
  g_free(payload);
  return ok;
}
