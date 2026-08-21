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

#include "../services/history_service.h"
#include "../services/http_service.h"
#include "../ui/views/request_view.h"
#include "../ui/views/response_view.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define REQUEST_TYPE_CONTROLLER (request_controller_get_type())
G_DECLARE_FINAL_TYPE(RequestController, request_controller, REQUEST, CONTROLLER,
                     GObject)

RequestController *request_controller_new(RequestView *request_view,
                                          ResponseView *response_view,
                                          HttpService *http_service,
                                          HistoryService *history_service);

void request_controller_send(RequestController *self);

G_END_DECLS
