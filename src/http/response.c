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

#include "response.h"

#include <glib.h>
#include <stdint.h>
#include <string.h>

HttpResponse *http_response_new(void) {
  HttpResponse *response = g_new0(HttpResponse, 1);

  response->body = g_malloc(1);
  response->body[0] = '\0';
  response->body_size = 0;
  response->curl_code = CURLE_OK;
  response->http_status = 0;

  return response;
}

HttpResponse *http_response_new_snapshot(long http_status, double total_time,
                                         const char *body,
                                         const char *content_type) {
  HttpResponse *response = http_response_new();
  response->http_status = http_status;
  response->total_time = total_time;

  if (body != NULL) {
    g_free(response->body);
    response->body = g_strdup(body);
    response->body_size = strlen(response->body);
  }
  if (content_type != NULL) {
    response->content_type = g_strdup(content_type);
  }

  return response;
}

void http_response_free(HttpResponse *response) {
  if (response == NULL) {
    return;
  }

  g_free(response->body);
  g_free(response->content_type);
  g_free(response);
}

size_t http_response_write_callback(void *contents, size_t size, size_t nmemb,
                                    void *userp) {
  if (size > 0 && nmemb > SIZE_MAX / size) {
    return 0;
  }

  size_t realsize = size * nmemb;
  GString *body = (GString *)userp;

  if (body->len + realsize > HTTP_RESPONSE_MAX_BODY_SIZE) {
    g_message("response body exceeds the %d byte limit; transfer aborted",
              HTTP_RESPONSE_MAX_BODY_SIZE);
    return 0;
  }

  g_string_append_len(body, contents, (gssize)realsize);
  return realsize;
}
