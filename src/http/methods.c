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

#include "methods.h"
#include <stddef.h>

static const char *methods_strings[] = {"GET",   "POST", "PUT",     "DELETE",
                                        "PATCH", "HEAD", "OPTIONS", NULL};

const char *http_method_to_string(HttpMethod method) {
  if ((unsigned)method <= (unsigned)HTTP_OPTIONS) {
    return methods_strings[method];
  }
  return "UNKNOWN";
}

const char **http_methods_get_list(void) { return methods_strings; }
