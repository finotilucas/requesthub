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

#include <gtksourceview/gtksource.h>

G_BEGIN_DECLS

/* GtkSourceView with the app style scheme and shared editing defaults; the
 * view owns its buffer — fetch it with gtk_text_view_get_buffer(). */
GtkSourceView *source_editor_new(const char *language_id, guint tab_width);

void source_editor_set_language(GtkSourceBuffer *buffer,
                                const char *language_id);

G_END_DECLS
