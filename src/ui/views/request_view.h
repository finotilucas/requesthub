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

#include "../../history/history.h"
#include "../components/request_top_bar.h"
#include "body_view.h"
#include "headers_view.h"
#include "params_view.h"
#include <gtk/gtk.h>

#define REQUEST_TYPE_VIEW (request_view_get_type())
G_DECLARE_FINAL_TYPE(RequestView, request_view, REQUEST, VIEW, GtkBox)

RequestView *request_view_new(void);
RequestTopBar *request_view_get_top_bar(RequestView *self);
ParamsView *request_view_get_params_view(RequestView *self);
HeadersView *request_view_get_headers_view(RequestView *self);
BodyView *request_view_get_body_view(RequestView *self);
void request_view_load_history_entry(RequestView *self,
                                     const HistoryEntry *entry);
