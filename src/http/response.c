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

#include "response.h"

#include <glib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_RESPONSE_SIZE (10 * 1024 * 1024)

HttpResponse *http_response_create(void) {
  HttpResponse *response = g_new0(HttpResponse, 1);

  response->body = g_malloc(1);
  response->body[0] = '\0';
  response->body_size = 0;
  response->curl_code = CURLE_OK;
  response->http_status = 0;

  return response;
}

void http_response_free(HttpResponse *response) {
  if (response == NULL) {
    return;
  }

  g_free(response->body);
  g_free(response->content_type);
  g_free(response->header_location);
  g_free(response->etag);

  if (response->all_headers != NULL) {
    curl_slist_free_all(response->all_headers);
  }

  g_free(response);
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  if (size > 0 && nmemb > SIZE_MAX / size) {
    return 0;
  }

  size_t realsize = size * nmemb;
  HttpResponse *response = (HttpResponse *)userp;

  if (response->body_size + realsize > MAX_RESPONSE_SIZE) {
    fprintf(stderr, "Error: Response exceeds the limit of %d bytes\n",
            MAX_RESPONSE_SIZE);
    return 0;
  }

  response->body =
      g_realloc(response->body, response->body_size + realsize + 1);
  memcpy(&(response->body[response->body_size]), contents, realsize);
  response->body_size += realsize;
  response->body[response->body_size] = '\0';

  return realsize;
}
