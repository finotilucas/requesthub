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

#include "../http/methods.h"

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
  HttpMethod method;
  gchar *url;
  gchar *body;
  GPtrArray *headers;
  GPtrArray *query_params;
} RequestData;

RequestData *request_data_new(void);
void request_data_free(RequestData *data);

void request_data_init(RequestData *data);
void request_data_clear(RequestData *data);

void request_data_set_url(RequestData *data, const char *url);
void request_data_set_body(RequestData *data, const char *body);
void request_data_add_header(RequestData *data, const char *key,
                             const char *value);
void request_data_add_query(RequestData *data, const char *key,
                            const char *value);

guint request_data_headers_count(const RequestData *data);
const char *request_data_header_key(const RequestData *data, guint index);
const char *request_data_header_value(const RequestData *data, guint index);
guint request_data_query_count(const RequestData *data);
const char *request_data_query_key(const RequestData *data, guint index);
const char *request_data_query_value(const RequestData *data, guint index);

G_END_DECLS
