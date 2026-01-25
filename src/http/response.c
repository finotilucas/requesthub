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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HttpResponse *http_response_create(void) {
  HttpResponse *response = calloc(1, sizeof(HttpResponse));
  if (response == NULL) {
    return NULL;
  }

  response->body = malloc(1);
  if (response->body == NULL) {
    free(response);
    return NULL;
  }

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

  free(response->body);
  free(response->content_type);
  free(response->header_location);
  free(response->etag);

  if (response->all_headers != NULL) {
    curl_slist_free_all(response->all_headers);
  }

  free(response);
}

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  HttpResponse *response = (HttpResponse *)userp;

  char *ptr = realloc(response->body, response->body_size + realsize + 1);
  if (ptr == NULL) {
    fprintf(stderr, "Erro: no memory\n");
    return 0;
  }

  response->body = ptr;
  memcpy(&(response->body[response->body_size]), contents, realsize);
  response->body_size += realsize;
  response->body[response->body_size] = '\0';

  return realsize;
}
