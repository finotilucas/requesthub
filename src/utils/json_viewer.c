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

#include <cjson/cJSON.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>
#include <stdlib.h>

static void json_view_setup_language(GtkSourceBuffer *buffer) {
  g_return_if_fail(GTK_SOURCE_IS_BUFFER(buffer));

  GtkSourceLanguageManager *lm = gtk_source_language_manager_get_default();

  GtkSourceLanguage *lang =
      gtk_source_language_manager_get_language(lm, "json");

  if (lang) {
    gtk_source_buffer_set_language(buffer, lang);
    gtk_source_buffer_set_highlight_syntax(buffer, TRUE);
  }
}

static void json_view_setup_theme(GtkSourceBuffer *buffer) {
  g_return_if_fail(GTK_SOURCE_IS_BUFFER(buffer));

  GtkSourceStyleSchemeManager *sm =
      gtk_source_style_scheme_manager_get_default();

  GtkSourceStyleScheme *scheme =
      gtk_source_style_scheme_manager_get_scheme(sm, "Adwaita-dark");

  if (scheme)
    gtk_source_buffer_set_style_scheme(buffer, scheme);
}

GtkWidget *json_view_new(GtkSourceBuffer **out_buffer) {
  GtkSourceBuffer *buffer = gtk_source_buffer_new(NULL);
  GtkWidget *view = gtk_source_view_new_with_buffer(buffer);

  json_view_setup_language(buffer);
  json_view_setup_theme(buffer);

  gtk_source_view_set_show_line_numbers(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_auto_indent(GTK_SOURCE_VIEW(view), TRUE);
  gtk_source_view_set_tab_width(GTK_SOURCE_VIEW(view), 2);
  gtk_source_view_set_insert_spaces_instead_of_tabs(GTK_SOURCE_VIEW(view),
                                                    TRUE);

  if (out_buffer)
    *out_buffer = buffer;
  else
    g_object_unref(buffer);

  return view;
}

void json_buffer_set_from_cjson(GtkTextBuffer *buffer, const cJSON *item) {
  g_return_if_fail(GTK_IS_TEXT_BUFFER(buffer));
  g_return_if_fail(item != NULL);

  char *json = cJSON_PrintBuffered(item, 1024, 1);
  if (!json) {
    return;
  }

  gtk_text_buffer_set_text(buffer, json, -1);

  free(json);
}
