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

#include "request.h"

#include <curl/curl.h>
#include <glib.h>

HttpRequest *http_request_new(const char *url, HttpMethod method) {
  if (url == NULL || *url == '\0') {
    return NULL;
  }

  HttpRequest *request = g_new0(HttpRequest, 1);
  request->url = g_strdup(url);
  request->headers = g_ptr_array_new_with_free_func(g_free);
  request->query_params = g_ptr_array_new_with_free_func(g_free);
  request->method = method;
  request->timeout = 30;
  request->connect_timeout = 10;
  request->follow_redirects = 1;
  request->max_redirects = 5;
  request->verify_ssl = 1;

  return request;
}

void http_request_free(HttpRequest *request) {
  if (request == NULL) {
    return;
  }

  g_free(request->url);
  g_free(request->body);
  g_free(request->body_content_type);

  if (request->headers != NULL) {
    g_ptr_array_unref(request->headers);
  }
  if (request->query_params != NULL) {
    g_ptr_array_unref(request->query_params);
  }

  g_free(request);
}

void http_request_add_header(HttpRequest *request, const char *key,
                             const char *value) {
  if (request == NULL || key == NULL || *key == '\0') {
    return;
  }

  g_ptr_array_add(request->headers, g_strdup(key));
  g_ptr_array_add(request->headers, g_strdup(value != NULL ? value : ""));
}

guint http_request_headers_count(const HttpRequest *request) {
  if (request == NULL || request->headers == NULL) {
    return 0;
  }
  return request->headers->len / 2;
}

const char *http_request_header_key(const HttpRequest *request, guint index) {
  if (request == NULL || request->headers == NULL) {
    return NULL;
  }
  guint slot = index * 2;
  if (slot >= request->headers->len) {
    return NULL;
  }
  return g_ptr_array_index(request->headers, slot);
}

const char *http_request_header_value(const HttpRequest *request, guint index) {
  if (request == NULL || request->headers == NULL) {
    return NULL;
  }
  guint slot = index * 2 + 1;
  if (slot >= request->headers->len) {
    return NULL;
  }
  return g_ptr_array_index(request->headers, slot);
}

void http_request_set_body(HttpRequest *request, const char *body,
                           const char *content_type) {
  if (request == NULL) {
    return;
  }

  g_free(request->body);
  request->body = (body != NULL) ? g_strdup(body) : NULL;

  g_free(request->body_content_type);
  request->body_content_type =
      (content_type != NULL) ? g_strdup(content_type) : NULL;
}

void http_request_add_query_param(HttpRequest *request, const char *key,
                                  const char *value) {
  if (request == NULL || key == NULL || value == NULL) {
    return;
  }

  char *encoded_key = curl_easy_escape(NULL, key, 0);
  char *encoded_value = curl_easy_escape(NULL, value, 0);

  if (encoded_key == NULL || encoded_value == NULL) {
    curl_free(encoded_key);
    curl_free(encoded_value);
    return;
  }

  gchar *param = g_strdup_printf("%s=%s", encoded_key, encoded_value);

  curl_free(encoded_key);
  curl_free(encoded_value);

  g_ptr_array_add(request->query_params, param);
}
