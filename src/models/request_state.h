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

typedef struct _RequestState RequestState;

RequestState *request_state_new(void);
void          request_state_free(RequestState *self);

void          request_state_set_method(RequestState *self, HttpMethod method);
HttpMethod   request_state_get_method(const RequestState *self);

void          request_state_set_url(RequestState *self, const char *url);
const char   *request_state_get_url(const RequestState *self);

void          request_state_set_body(RequestState *self, const char *body);
const char   *request_state_get_body(const RequestState *self);

void          request_state_add_header  (RequestState *self,
                                         const char *key, const char *value);
guint         request_state_headers_count(const RequestState *self);
const char   *request_state_header_key  (const RequestState *self, guint index);
const char   *request_state_header_value(const RequestState *self, guint index);

void          request_state_add_query   (RequestState *self,
                                         const char *key, const char *value);
guint         request_state_query_count (const RequestState *self);
const char   *request_state_query_key   (const RequestState *self, guint index);
const char   *request_state_query_value (const RequestState *self, guint index);

G_END_DECLS
