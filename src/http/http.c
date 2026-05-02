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

#include "http.h"
#include "http_pool.h"
#include "request.h"
#include "response.h"

#include <curl/curl.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

static void apply_request_body(CURL *curl, const HttpRequest *request) {
  if (request->body != NULL && *request->body != '\0') {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                     (long)strlen(request->body));
    curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, request->body);
  } else {
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, 0L);
  }
}

static void configure_method(CURL *curl, HttpRequest *request) {
  curl_easy_setopt(curl, CURLOPT_POST, 0L);
  curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, NULL);
  curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 0L);

  switch (request->method) {
  case HTTP_GET:
    break;
  case HTTP_POST:
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    apply_request_body(curl, request);
    break;
  case HTTP_PUT:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    apply_request_body(curl, request);
    break;
  case HTTP_DELETE:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    break;
  case HTTP_PATCH:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    apply_request_body(curl, request);
    break;
  case HTTP_HEAD:
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    break;
  case HTTP_OPTIONS:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
    break;
  }
}

static int header_exists(struct curl_slist *list, const char *key) {
  size_t key_len = strlen(key);
  while (list) {
    if (strncasecmp(list->data, key, key_len) == 0 &&
        list->data[key_len] == ':') {
      return 1;
    }
    list = list->next;
  }
  return 0;
}

static void add_default_headers(struct curl_slist **headers) {
  if (!header_exists(*headers, "User-Agent")) {
    *headers = curl_slist_append(*headers, "User-Agent: requesthub/0.0.1");
  }
  if (!header_exists(*headers, "Accept")) {
    *headers = curl_slist_append(*headers, "Accept: */*");
  }
}

static struct curl_slist *build_headers_list(HttpRequest *request) {
  struct curl_slist *list = NULL;
  for (int i = 0; i < request->headers_count; i++) {
    struct curl_slist *tmp = curl_slist_append(list, request->headers[i]);
    if (!tmp) {
      curl_slist_free_all(list);
      return NULL;
    }
    list = tmp;
  }
  if (request->auth_header) {
    struct curl_slist *tmp = curl_slist_append(list, request->auth_header);
    if (!tmp) {
      curl_slist_free_all(list);
      return NULL;
    }
    list = tmp;
  }
  return list;
}

static void configure_curl_options(CURL *curl, HttpRequest *request,
                                   HttpResponse *response,
                                   struct curl_slist **headers, char *url) {
  curl_easy_setopt(curl, CURLOPT_URL, url);
  configure_method(curl, request);

  add_default_headers(headers);

  if (*headers) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headers);
  }

  if (request->timeout > 0) {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, request->timeout);
  }

  if (request->connect_timeout > 0) {
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, request->connect_timeout);
  }

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,
                   (long)request->follow_redirects);

  if (request->max_redirects > 0) {
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, (long)request->max_redirects);
  }

  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)request->verify_ssl);

  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, request->verify_ssl ? 2L : 0L);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

  curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);

  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);

  curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);

  curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);

  curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

  curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 120L);

  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, 102400L);

  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

  curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 1L);
}

static void extract_response_info(CURL *curl, HttpResponse *response) {
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status);

  char *content_type = NULL;
  curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);

  g_free(response->content_type);
  response->content_type = (content_type != NULL) ? g_strdup(content_type) : NULL;

  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &response->total_time);
}

static gchar *build_query_string(HttpRequest *request) {
  if (!request->query_params || request->query_count == 0) {
    return NULL;
  }

  GString *buffer = g_string_new("?");
  for (int i = 0; i < request->query_count; i++) {
    if (i > 0) {
      g_string_append_c(buffer, '&');
    }
    g_string_append(buffer, request->query_params[i]);
  }

  return g_string_free(buffer, FALSE);
}

HttpResponse *http_request_perform(HttpRequest *request) {
  if (!request || !request->url) {
    return NULL;
  }

  gchar *query_string = build_query_string(request);
  gchar *final_url =
      (query_string != NULL)
          ? g_strconcat(request->url, query_string, NULL)
          : g_strdup(request->url);
  g_free(query_string);

  HttpResponse *response = http_response_create();
  CURL *curl = http_pool_acquire();

  if (!curl) {
    g_free(final_url);
    http_response_free(response);
    return NULL;
  }

  struct curl_slist *headers = build_headers_list(request);

  configure_curl_options(curl, request, response, &headers, final_url);

  response->curl_code = curl_easy_perform(curl);

  extract_response_info(curl, response);

  if (headers) {
    curl_slist_free_all(headers);
  }

  http_pool_release(curl);

  g_free(final_url);

  return response;
}
