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

#include "../../models/request_state.h"
#include "../panels/request_top_bar.h"
#include "../panels/body_panel.h"
#include "../panels/headers_panel.h"
#include "../panels/params_panel.h"
#include <gtk/gtk.h>

#define REQUEST_VIEW_MIN_WIDTH 380

#define REQUEST_TYPE_VIEW (request_view_get_type())
G_DECLARE_FINAL_TYPE(RequestView, request_view, REQUEST, VIEW, GtkBox)

RequestView *request_view_new(void);
RequestTopBar *request_view_get_top_bar(RequestView *self);
ParamsPanel *request_view_get_params_panel(RequestView *self);
HeadersPanel *request_view_get_headers_panel(RequestView *self);
BodyPanel *request_view_get_body_panel(RequestView *self);

/* Loads neutral state into the view's widgets. The state is borrowed. */
void request_view_apply_state(RequestView *self, const RequestState *state);

/* Captures the current widget state as a freshly allocated RequestState.
 * Caller owns the returned pointer (transfer: full). */
RequestState *request_view_capture_state(RequestView *self);
