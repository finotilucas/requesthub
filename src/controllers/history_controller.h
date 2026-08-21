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

#include "../ui/views/history_view.h"
#include "../ui/views/request_view.h"
#include "../ui/views/response_view.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define HISTORY_TYPE_CONTROLLER (history_controller_get_type())
G_DECLARE_FINAL_TYPE(HistoryController, history_controller, HISTORY, CONTROLLER,
                     GObject)

HistoryController *history_controller_new(HistoryView *history_view,
                                          RequestView *request_view,
                                          ResponseView *response_view);

G_END_DECLS
