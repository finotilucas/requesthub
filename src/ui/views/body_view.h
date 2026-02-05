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

#ifndef BODY_VIEW_H
#define BODY_VIEW_H

#include "../../http/request.h"
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

#define TYPE_BODY_VIEW (body_view_get_type())
G_DECLARE_FINAL_TYPE(BodyView, body_view, BODY, VIEW, GtkBox)

typedef enum {
  BODY_TYPE_JSON = 0,
  BODY_TYPE_XML = 1,
  BODY_TYPE_YAML = 2,
  BODY_TYPE_TEXT = 3
} BodyContentType;

BodyView *body_view_new(void);

void body_view_apply_to_request(BodyView *self, HttpRequest *request);

void body_view_clear(BodyView *self);

void body_view_set_content_type(BodyView *self, BodyContentType type);

BodyContentType body_view_get_content_type(BodyView *self);

void body_view_set_content(BodyView *self, const char *content);

char *body_view_get_content(BodyView *self);

gboolean body_view_is_valid(BodyView *self);

G_END_DECLS

#endif
