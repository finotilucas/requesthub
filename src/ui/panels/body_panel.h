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

#include "../../http/request.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BODY_TYPE_PANEL (body_panel_get_type())
G_DECLARE_FINAL_TYPE(BodyPanel, body_panel, BODY, PANEL, GtkBox)


BodyPanel *body_panel_new(void);

void body_panel_apply_to_request(BodyPanel *self, HttpRequest *request);

void body_panel_clear(BodyPanel *self);

void body_panel_set_content(BodyPanel *self, const char *content);

char *body_panel_get_content(BodyPanel *self);

G_END_DECLS
