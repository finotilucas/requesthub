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

#pragma once

#include <curl/curl.h>
#include <stddef.h>

#define HTTP_RESPONSE_MAX_BODY_SIZE (10 * 1024 * 1024)

typedef struct {
  char *body;
  char *content_type;
  size_t body_size;
  double total_time;
  long http_status;
  CURLcode curl_code;
} HttpResponse;

HttpResponse *http_response_new(void);

/* Reconstroi uma resposta a partir de dados cacheados (replay do historico).
 * body/content_type podem ser NULL; mantem body_size == strlen(body). */
HttpResponse *http_response_new_snapshot(long http_status, double total_time,
                                         const char *body,
                                         const char *content_type);

void http_response_free(HttpResponse *response);
/* Callback de escrita do curl; userp e um GString acumulando o corpo
 * (crescimento amortizado). O perform materializa em body/body_size. */
size_t http_response_write_callback(void *contents, size_t size, size_t nmemb,
                                    void *userp);
