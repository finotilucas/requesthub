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

/* The order of these values is frozen: history.json stores the method as
 * the enum ordinal. New methods may only be appended at the end. */
typedef enum {
  HTTP_GET,
  HTTP_POST,
  HTTP_PUT,
  HTTP_DELETE,
  HTTP_PATCH,
  HTTP_HEAD,
  HTTP_OPTIONS,
} HttpMethod;

#define HTTP_METHOD_COUNT 7

const char *http_method_to_string(HttpMethod method);

const char **http_methods_get_list(void);
