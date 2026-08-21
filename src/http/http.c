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

#include "http.h"
#include "http_pool.h"
#include "request.h"
#include "response.h"

#include "../config/version.h"

#include <curl/curl.h>
#include <glib.h>
#include <string.h>

#define TCP_KEEPIDLE_SECONDS 120L
#define TCP_KEEPINTVL_SECONDS 60L
#define DNS_CACHE_SECONDS 120L
#define RECEIVE_BUFFER_BYTES 102400L

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
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
                     http_method_to_string(HTTP_PUT));
    apply_request_body(curl, request);
    break;
  case HTTP_DELETE:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
                     http_method_to_string(HTTP_DELETE));
    break;
  case HTTP_PATCH:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
                     http_method_to_string(HTTP_PATCH));
    apply_request_body(curl, request);
    break;
  case HTTP_HEAD:
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    break;
  case HTTP_OPTIONS:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,
                     http_method_to_string(HTTP_OPTIONS));
    break;
  }
}

static gboolean headers_contain_key(struct curl_slist *list, const char *key) {
  size_t key_len = strlen(key);
  while (list) {
    if (g_ascii_strncasecmp(list->data, key, key_len) == 0 &&
        list->data[key_len] == ':') {
      return TRUE;
    }
    list = list->next;
  }
  return FALSE;
}

static void add_default_headers(struct curl_slist **headers,
                                const HttpRequest *request) {
  if (!headers_contain_key(*headers, "User-Agent")) {
    struct curl_slist *appended =
        curl_slist_append(*headers, "User-Agent: " REQUESTHUB_USER_AGENT);
    if (appended != NULL) {
      *headers = appended;
    }
  }
  if (!headers_contain_key(*headers, "Accept")) {
    struct curl_slist *appended = curl_slist_append(*headers, "Accept: */*");
    if (appended != NULL) {
      *headers = appended;
    }
  }
  if (request->body != NULL && request->body_content_type != NULL &&
      !headers_contain_key(*headers, "Content-Type")) {
    gchar *line =
        g_strdup_printf("Content-Type: %s", request->body_content_type);
    struct curl_slist *appended = curl_slist_append(*headers, line);
    g_free(line);
    if (appended != NULL) {
      *headers = appended;
    }
  }
}

static struct curl_slist *build_headers_list(HttpRequest *request) {
  struct curl_slist *list = NULL;
  guint count = http_request_headers_count(request);
  for (guint i = 0; i < count; i++) {
    const char *key = http_request_header_key(request, i);
    const char *value = http_request_header_value(request, i);
    gchar *line = g_strdup_printf("%s: %s", key, value != NULL ? value : "");
    struct curl_slist *appended = curl_slist_append(list, line);
    g_free(line);
    if (!appended) {
      curl_slist_free_all(list);
      return NULL;
    }
    list = appended;
  }
  return list;
}

static void configure_curl_options(CURL *curl, HttpRequest *request,
                                   GString *body_acc,
                                   struct curl_slist **headers,
                                   const char *url) {
  curl_easy_setopt(curl, CURLOPT_URL, url);
  configure_method(curl, request);

  add_default_headers(headers, request);

  if (*headers) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, *headers);
  }

  /* Setar tudo incondicionalmente: handles reusados do pool nao podem herdar
   * opcoes do request anterior. */
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, request->timeout);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, request->connect_timeout);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,
                   (long)request->follow_redirects);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, (long)request->max_redirects);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)request->verify_ssl);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, request->verify_ssl ? 2L : 0L);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_response_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, body_acc);

  curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_0);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, TCP_KEEPIDLE_SECONDS);
  curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, TCP_KEEPINTVL_SECONDS);
  curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
  curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, DNS_CACHE_SECONDS);
  curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, RECEIVE_BUFFER_BYTES);
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

static gchar *build_query_suffix(HttpRequest *request) {
  if (request->query_params == NULL || request->query_params->len == 0) {
    return NULL;
  }

  const char *existing_query = strchr(request->url, '?');
  size_t url_len = strlen(request->url);
  gboolean url_ends_with_separator =
      existing_query != NULL &&
      (url_len > 0 &&
       (request->url[url_len - 1] == '?' || request->url[url_len - 1] == '&'));

  GString *buffer = g_string_new(NULL);
  if (!url_ends_with_separator) {
    g_string_append_c(buffer, existing_query != NULL ? '&' : '?');
  }
  for (guint i = 0; i < request->query_params->len; i++) {
    if (i > 0) {
      g_string_append_c(buffer, '&');
    }
    g_string_append(buffer, g_ptr_array_index(request->query_params, i));
  }

  return g_string_free(buffer, FALSE);
}

HttpResponse *http_request_perform(HttpRequest *request) {
  if (!request || !request->url) {
    return NULL;
  }

  /* CURLOPT_URL copia a string, entao so alocamos quando ha sufixo. */
  gchar *query_suffix = build_query_suffix(request);
  gchar *final_url = (query_suffix != NULL)
                         ? g_strconcat(request->url, query_suffix, NULL)
                         : NULL;
  g_free(query_suffix);
  const char *effective_url = final_url != NULL ? final_url : request->url;

  HttpResponse *response = http_response_new();
  CURL *curl = http_pool_acquire();

  if (!curl) {
    g_free(final_url);
    http_response_free(response);
    return NULL;
  }

  struct curl_slist *headers = build_headers_list(request);
  GString *body_acc = g_string_new(NULL);

  configure_curl_options(curl, request, body_acc, &headers, effective_url);

  response->curl_code = curl_easy_perform(curl);

  extract_response_info(curl, response);

  g_free(response->body);
  response->body_size = body_acc->len;
  response->body = g_string_free(body_acc, FALSE);

  if (headers) {
    curl_slist_free_all(headers);
  }

  http_pool_release(curl);

  g_free(final_url);

  return response;
}
