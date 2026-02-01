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

#pragma once

#include "../../http/methods.h"
#include <gtk/gtk.h>

#define REQUEST_TYPE_TOP_BAR (request_top_bar_get_type())
G_DECLARE_FINAL_TYPE(RequestTopBar, request_top_bar, REQUEST, TOP_BAR, GtkBox)

RequestTopBar *request_top_bar_new(void);
const char *request_top_bar_get_url(RequestTopBar *self);
HttpMethods request_top_bar_get_method(RequestTopBar *self);
void request_top_bar_set_loading(RequestTopBar *self, gboolean is_loading);
