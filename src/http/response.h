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

#ifndef RESPONSE_H
#define RESPONSE_H

#include <curl/curl.h>
#include <stddef.h>

typedef struct {
  char *body;
  char *content_type;
  char *header_location;
  char *etag;
  struct curl_slist *all_headers;
  size_t body_size;
  double total_time;
  double download_size;
  long http_status;
  long content_length;
  CURLcode curl_code;
} HttpResponse;

HttpResponse *http_response_create(void);
void http_response_free(HttpResponse *response);
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);

#endif
