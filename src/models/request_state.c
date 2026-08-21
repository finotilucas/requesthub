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

#include "request_state.h"

struct _RequestState {
  HttpMethod method;
  gchar *url;
  gchar *body;
  GPtrArray *headers;
  GPtrArray *query_params;
};

RequestState *request_state_new(void) {
  RequestState *self = g_new0(RequestState, 1);
  self->method = HTTP_GET;
  self->headers = g_ptr_array_new_with_free_func(g_free);
  self->query_params = g_ptr_array_new_with_free_func(g_free);
  return self;
}

void request_state_free(RequestState *self) {
  if (self == NULL) {
    return;
  }
  g_free(self->url);
  g_free(self->body);
  if (self->headers != NULL) {
    g_ptr_array_unref(self->headers);
  }
  if (self->query_params != NULL) {
    g_ptr_array_unref(self->query_params);
  }
  g_free(self);
}

void request_state_set_method(RequestState *self, HttpMethod method) {
  if (self != NULL) {
    self->method = method;
  }
}

HttpMethod request_state_get_method(const RequestState *self) {
  return self != NULL ? self->method : HTTP_GET;
}

void request_state_set_url(RequestState *self, const char *url) {
  if (self == NULL) {
    return;
  }
  g_free(self->url);
  self->url = (url != NULL) ? g_strdup(url) : NULL;
}

const char *request_state_get_url(const RequestState *self) {
  return self != NULL ? self->url : NULL;
}

void request_state_set_body(RequestState *self, const char *body) {
  if (self == NULL) {
    return;
  }
  g_free(self->body);
  self->body = (body != NULL) ? g_strdup(body) : NULL;
}

const char *request_state_get_body(const RequestState *self) {
  return self != NULL ? self->body : NULL;
}

static void kv_array_add(GPtrArray *arr, const char *key, const char *value) {
  g_ptr_array_add(arr, g_strdup(key));
  g_ptr_array_add(arr, g_strdup(value != NULL ? value : ""));
}

static const char *kv_array_get(GPtrArray *arr, guint index, gboolean is_value) {
  if (arr == NULL) {
    return NULL;
  }
  guint slot = index * 2 + (is_value ? 1u : 0u);
  if (slot >= arr->len) {
    return NULL;
  }
  return g_ptr_array_index(arr, slot);
}

void request_state_add_header(RequestState *self, const char *key,
                              const char *value) {
  if (self == NULL || key == NULL || *key == '\0') {
    return;
  }
  kv_array_add(self->headers, key, value);
}

guint request_state_headers_count(const RequestState *self) {
  if (self == NULL || self->headers == NULL) {
    return 0;
  }
  return self->headers->len / 2;
}

const char *request_state_header_key(const RequestState *self, guint index) {
  return self != NULL ? kv_array_get(self->headers, index, FALSE) : NULL;
}

const char *request_state_header_value(const RequestState *self, guint index) {
  return self != NULL ? kv_array_get(self->headers, index, TRUE) : NULL;
}

void request_state_add_query(RequestState *self, const char *key,
                             const char *value) {
  if (self == NULL || key == NULL || *key == '\0') {
    return;
  }
  kv_array_add(self->query_params, key, value);
}

guint request_state_query_count(const RequestState *self) {
  if (self == NULL || self->query_params == NULL) {
    return 0;
  }
  return self->query_params->len / 2;
}

const char *request_state_query_key(const RequestState *self, guint index) {
  return self != NULL ? kv_array_get(self->query_params, index, FALSE) : NULL;
}

const char *request_state_query_value(const RequestState *self, guint index) {
  return self != NULL ? kv_array_get(self->query_params, index, TRUE) : NULL;
}
