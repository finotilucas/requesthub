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

#include "source_editor.h"

#define SOURCE_EDITOR_STYLE_SCHEME "Adwaita-dark"

void source_editor_set_language(GtkSourceBuffer *buffer,
                                const char *language_id) {
  GtkSourceLanguage *language =
      language_id != NULL
          ? gtk_source_language_manager_get_language(
                gtk_source_language_manager_get_default(), language_id)
          : NULL;
  gtk_source_buffer_set_language(buffer, language);
}

GtkSourceView *source_editor_new(const char *language_id, guint tab_width) {
  GtkSourceBuffer *buffer = gtk_source_buffer_new(NULL);
  source_editor_set_language(buffer, language_id);

  GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(
      gtk_source_style_scheme_manager_get_default(),
      SOURCE_EDITOR_STYLE_SCHEME);
  if (scheme != NULL) {
    gtk_source_buffer_set_style_scheme(buffer, scheme);
  }

  GtkWidget *view = gtk_source_view_new_with_buffer(buffer);
  g_object_unref(buffer);

  gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(view), tab_width);
  gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(view),
                                                    TRUE);

  return GTK_SOURCE_VIEW(view);
}
