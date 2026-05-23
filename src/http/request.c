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

#include "request.h"

#include <curl/curl.h>
#include <glib.h>
#include <string.h>

HttpRequest *http_request_new(const char *url, HttpMethods method) {
  if (url == NULL || *url == '\0') {
    return NULL;
  }

  HttpRequest *request = g_new0(HttpRequest, 1);
  request->url = g_strdup(url);
  request->headers = g_ptr_array_new_with_free_func(g_free);
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
  g_free(request->auth_header);

  if (request->headers != NULL) {
    g_ptr_array_unref(request->headers);
  }

  if (request->query_params != NULL) {
    for (int i = 0; i < request->query_count; i++) {
      g_free(request->query_params[i]);
    }
    g_free(request->query_params);
  }

  g_free(request);
}

HttpRequest *http_request_add_header(HttpRequest *request, const char *key,
                                     const char *value) {
  if (request == NULL || key == NULL || *key == '\0') {
    return request;
  }

  g_ptr_array_add(request->headers, g_strdup(key));
  g_ptr_array_add(request->headers, g_strdup(value != NULL ? value : ""));

  return request;
}

HttpRequest *http_request_remove_header(HttpRequest *request, const char *key) {
  if (request == NULL || key == NULL || request->headers == NULL ||
      request->headers->len == 0) {
    return request;
  }

  for (guint i = 0; i + 1 < request->headers->len; i += 2) {
    const char *stored_key = g_ptr_array_index(request->headers, i);
    if (g_ascii_strcasecmp(stored_key, key) == 0) {
      g_ptr_array_remove_index(request->headers, i + 1);
      g_ptr_array_remove_index(request->headers, i);
      return request;
    }
  }

  return request;
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

HttpRequest *http_request_set_body(HttpRequest *request, const char *body) {
  if (request == NULL) {
    return request;
  }

  g_free(request->body);
  request->body = (body != NULL) ? g_strdup(body) : NULL;

  return request;
}

HttpRequest *http_request_add_query_param(HttpRequest *request, const char *key,
                                          const char *value) {
  if (request == NULL || key == NULL || value == NULL) {
    return request;
  }

  char *encoded_key = curl_easy_escape(NULL, key, 0);
  char *encoded_value = curl_easy_escape(NULL, value, 0);

  if (encoded_key == NULL || encoded_value == NULL) {
    curl_free(encoded_key);
    curl_free(encoded_value);
    return request;
  }

  gchar *param = g_strdup_printf("%s=%s", encoded_key, encoded_value);

  curl_free(encoded_key);
  curl_free(encoded_value);

  request->query_params = g_realloc(request->query_params,
                                    sizeof(char *) * (request->query_count + 1));
  request->query_params[request->query_count] = param;
  request->query_count++;

  return request;
}

HttpRequest *http_request_remove_query_param(HttpRequest *request,
                                             const char *key) {
  if (request == NULL || key == NULL || request->query_params == NULL ||
      request->query_count == 0) {
    return request;
  }

  char *encoded_key = curl_easy_escape(NULL, key, 0);
  if (encoded_key == NULL) {
    return request;
  }

  size_t key_len = strlen(encoded_key);
  int found_index = -1;

  for (int i = 0; i < request->query_count; i++) {
    if (strncmp(request->query_params[i], encoded_key, key_len) == 0 &&
        request->query_params[i][key_len] == '=') {
      found_index = i;
      break;
    }
  }

  curl_free(encoded_key);

  if (found_index == -1) {
    return request;
  }

  g_free(request->query_params[found_index]);

  size_t num_elements_to_move = request->query_count - found_index - 1;
  if (num_elements_to_move > 0) {
    memmove(&request->query_params[found_index],
            &request->query_params[found_index + 1],
            sizeof(char *) * num_elements_to_move);
  }

  request->query_count--;

  if (request->query_count == 0) {
    g_free(request->query_params);
    request->query_params = NULL;
  } else {
    request->query_params =
        g_realloc(request->query_params, sizeof(char *) * request->query_count);
  }

  return request;
}

HttpRequest *http_request_set_timeout(HttpRequest *request, long seconds) {
  if (request == NULL || seconds < 0) {
    return request;
  }

  request->timeout = seconds;
  return request;
}

HttpRequest *http_request_set_connect_timeout(HttpRequest *request,
                                              long seconds) {
  if (request == NULL || seconds < 0) {
    return request;
  }

  request->connect_timeout = seconds;
  return request;
}

HttpRequest *http_request_set_verify_ssl(HttpRequest *request, int verify) {
  if (request == NULL) {
    return request;
  }

  request->verify_ssl = verify ? 1 : 0;
  return request;
}

HttpRequest *http_request_set_bearer_token(HttpRequest *request,
                                           const char *token) {
  if (request == NULL || token == NULL) {
    return request;
  }

  g_free(request->auth_header);
  request->auth_header = g_strdup_printf("Authorization: Bearer %s", token);

  return request;
}

HttpRequest *http_request_follow_redirects(HttpRequest *request, int follow,
                                           int max) {
  if (request == NULL) {
    return request;
  }

  request->follow_redirects = follow ? 1 : 0;
  request->max_redirects = (max > 0) ? max : 5;

  return request;
}
