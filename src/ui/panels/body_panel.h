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

#ifndef BODY_PANEL_H
#define BODY_PANEL_H

#include "../../http/request.h"
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

#define TYPE_BODY_PANEL (body_panel_get_type())
G_DECLARE_FINAL_TYPE(BodyPanel, body_panel, BODY, PANEL, GtkBox)

typedef enum {
  BODY_TYPE_JSON = 0,
  BODY_TYPE_XML = 1,
  BODY_TYPE_YAML = 2,
  BODY_TYPE_TEXT = 3
} BodyContentType;

BodyPanel *body_panel_new(void);

void body_panel_apply_to_request(BodyPanel *self, HttpRequest *request);

void body_panel_clear(BodyPanel *self);

void body_panel_set_content_type(BodyPanel *self, BodyContentType type);

BodyContentType body_panel_get_content_type(BodyPanel *self);

void body_panel_set_content(BodyPanel *self, const char *content);

char *body_panel_get_content(BodyPanel *self);

gboolean body_panel_is_valid(BodyPanel *self);

G_END_DECLS

#endif
