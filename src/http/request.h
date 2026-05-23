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

#ifndef REQUEST_H
#define REQUEST_H

#include "methods.h"

#include <glib.h>

typedef struct {
  char *url;
  char *body;
  GPtrArray *headers;
  char **query_params;
  char *auth_header;
  long timeout;
  long connect_timeout;
  long follow_redirects;
  long max_redirects;
  long verify_ssl;
  int query_count;
  HttpMethods method;
} HttpRequest;

HttpRequest *http_request_new(const char *url, HttpMethods method);

void http_request_free(HttpRequest *request);

HttpRequest *http_request_add_header(HttpRequest *request, const char *key,
                                     const char *value);

HttpRequest *http_request_remove_header(HttpRequest *request, const char *key);

guint http_request_headers_count(const HttpRequest *request);

const char *http_request_header_key(const HttpRequest *request, guint index);

const char *http_request_header_value(const HttpRequest *request, guint index);

HttpRequest *http_request_set_body(HttpRequest *request, const char *body);

HttpRequest *http_request_add_query_param(HttpRequest *request, const char *key,
                                          const char *value);

HttpRequest *http_request_remove_query_param(HttpRequest *request,
                                             const char *key);

HttpRequest *http_request_set_timeout(HttpRequest *request, long seconds);

HttpRequest *http_request_set_connect_timeout(HttpRequest *request,
                                              long seconds);

HttpRequest *http_request_set_verify_ssl(HttpRequest *request, int verify);

HttpRequest *http_request_set_bearer_token(HttpRequest *request,
                                           const char *token);

HttpRequest *http_request_follow_redirects(HttpRequest *request, int follow,
                                           int max);

#endif
