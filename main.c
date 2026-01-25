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
 * (at your option) any later version.
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


#include "src/http/http.h"
#include "src/http/request.h"
#include <curl/curl.h>
#include <stdio.h>

int main(void) {
  clock_t start = clock();

  curl_global_init(CURL_GLOBAL_ALL);

  const char *url = "https://jsonplaceholder.typicode.com/comments";

  HttpRequest *req = http_request_new(url, GET);

  http_request_add_query_param(req, "postId", "10");

  if (req == NULL) {
    fprintf(stderr, "Erro ao criar request\n");
    return 1;
  }

  HttpResponse *resp = http_request_perform(req);

  if (resp != NULL) {
    printf("Status: %ld\n", resp->http_status);
    printf("Body: %s\n", resp->body);
    http_response_free(resp);
  }

  http_request_free(req);
  curl_global_cleanup();

  clock_t stop = clock();
  double elapsed = (double)(stop - start) * 1000.0 / CLOCKS_PER_SEC;
  printf("Time elapsed in ms: %.2f\n", elapsed);
  return 0;
}
