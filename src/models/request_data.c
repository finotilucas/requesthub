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

#include "request_data.h"

void request_data_init(RequestData *data) {
  data->method = HTTP_GET;
  data->url = NULL;
  data->body = NULL;
  data->headers = g_ptr_array_new_with_free_func(g_free);
  data->query_params = g_ptr_array_new_with_free_func(g_free);
}

void request_data_clear(RequestData *data) {
  if (data == NULL) {
    return;
  }
  g_free(data->url);
  g_free(data->body);
  if (data->headers != NULL) {
    g_ptr_array_unref(data->headers);
  }
  if (data->query_params != NULL) {
    g_ptr_array_unref(data->query_params);
  }
  *data = (RequestData){0};
}

RequestData *request_data_new(void) {
  RequestData *data = g_new0(RequestData, 1);
  request_data_init(data);
  return data;
}

void request_data_free(RequestData *data) {
  if (data == NULL) {
    return;
  }
  request_data_clear(data);
  g_free(data);
}

void request_data_set_url(RequestData *data, const char *url) {
  if (data == NULL) {
    return;
  }
  g_free(data->url);
  data->url = (url != NULL) ? g_strdup(url) : NULL;
}

void request_data_set_body(RequestData *data, const char *body) {
  if (data == NULL) {
    return;
  }
  g_free(data->body);
  data->body = (body != NULL) ? g_strdup(body) : NULL;
}

static void kv_array_add(GPtrArray *pairs, const char *key,
                         const char *value) {
  g_ptr_array_add(pairs, g_strdup(key));
  g_ptr_array_add(pairs, g_strdup(value != NULL ? value : ""));
}

static const char *kv_array_get(const GPtrArray *pairs, guint index,
                                gboolean want_value) {
  if (pairs == NULL) {
    return NULL;
  }
  guint slot = index * 2 + (want_value ? 1u : 0u);
  if (slot >= pairs->len) {
    return NULL;
  }
  return g_ptr_array_index(pairs, slot);
}

void request_data_add_header(RequestData *data, const char *key,
                             const char *value) {
  if (data == NULL || key == NULL || *key == '\0') {
    return;
  }
  kv_array_add(data->headers, key, value);
}

void request_data_add_query(RequestData *data, const char *key,
                            const char *value) {
  if (data == NULL || key == NULL || *key == '\0') {
    return;
  }
  kv_array_add(data->query_params, key, value);
}

guint request_data_headers_count(const RequestData *data) {
  if (data == NULL || data->headers == NULL) {
    return 0;
  }
  return data->headers->len / 2;
}

const char *request_data_header_key(const RequestData *data, guint index) {
  return data != NULL ? kv_array_get(data->headers, index, FALSE) : NULL;
}

const char *request_data_header_value(const RequestData *data, guint index) {
  return data != NULL ? kv_array_get(data->headers, index, TRUE) : NULL;
}

guint request_data_query_count(const RequestData *data) {
  if (data == NULL || data->query_params == NULL) {
    return 0;
  }
  return data->query_params->len / 2;
}

const char *request_data_query_key(const RequestData *data, guint index) {
  return data != NULL ? kv_array_get(data->query_params, index, FALSE) : NULL;
}

const char *request_data_query_value(const RequestData *data, guint index) {
  return data != NULL ? kv_array_get(data->query_params, index, TRUE) : NULL;
}
