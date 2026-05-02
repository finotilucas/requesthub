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
    for (int i = 0; i < request->headers_count; i++) {
      g_free(request->headers[i]);
    }
    g_free(request->headers);
  }

  if (request->query_params != NULL) {
    for (int i = 0; i < request->query_count; i++) {
      g_free(request->query_params[i]);
    }
    g_free(request->query_params);
  }

  g_free(request);
}

HttpRequest *http_request_add_header(HttpRequest *request, const char *header) {
  if (request == NULL || header == NULL) {
    return request;
  }

  request->headers =
      g_realloc(request->headers, sizeof(char *) * (request->headers_count + 1));
  request->headers[request->headers_count] = g_strdup(header);
  request->headers_count++;

  return request;
}

HttpRequest *http_request_remove_header(HttpRequest *request, const char *key) {
  if (request == NULL || key == NULL || request->headers == NULL ||
      request->headers_count == 0) {
    return request;
  }

  size_t key_len = strlen(key);
  int found_index = -1;

  for (int i = 0; i < request->headers_count; i++) {
    if (strncasecmp(request->headers[i], key, key_len) == 0 &&
        request->headers[i][key_len] == ':') {
      found_index = i;
      break;
    }
  }

  if (found_index == -1) {
    return request;
  }

  g_free(request->headers[found_index]);

  size_t num_elements_to_move = request->headers_count - found_index - 1;
  if (num_elements_to_move > 0) {
    memmove(&request->headers[found_index], &request->headers[found_index + 1],
            sizeof(char *) * num_elements_to_move);
  }

  request->headers_count--;

  if (request->headers_count == 0) {
    g_free(request->headers);
    request->headers = NULL;
  } else {
    request->headers =
        g_realloc(request->headers, sizeof(char *) * request->headers_count);
  }

  return request;
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
