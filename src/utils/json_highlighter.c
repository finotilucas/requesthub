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
#include <stdio.h>

static void json_buffer_append_with_tag(GtkTextBuffer *buffer, const char *text,
                                        const char *tag_name) {
  GtkTextIter iter;
  gtk_text_buffer_get_end_iter(buffer, &iter);

  if (tag_name) {
    gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, text, -1, tag_name,
                                             NULL);
  } else {
    gtk_text_buffer_insert(buffer, &iter, text, -1);
  }
}

static void setup_json_tags(GtkTextBuffer *buffer) {
  GtkTextTagTable *table = gtk_text_buffer_get_tag_table(buffer);

  if (gtk_text_tag_table_lookup(table, "json_key"))
    return;

  gtk_text_buffer_create_tag(buffer, "json_key", "foreground", "#b0da09", NULL);
  gtk_text_buffer_create_tag(buffer, "json_string", "foreground", "#ffdd00",
                             NULL);
  gtk_text_buffer_create_tag(buffer, "json_number", "foreground", "#AE81FF",
                             NULL);
  gtk_text_buffer_create_tag(buffer, "json_bool", "foreground", "#AE81FF",
                             NULL);
  gtk_text_buffer_create_tag(buffer, "json_null", "foreground", "#E06C75",
                             NULL);
}

void json_highlighted_to_buffer(cJSON *item, GtkTextBuffer *buffer,
                                int indent) {
  setup_json_tags(buffer);

  char indent_buf[64];
  snprintf(indent_buf, sizeof(indent_buf), "%*s", indent, "");

  if (cJSON_IsObject(item) || cJSON_IsArray(item)) {
    gboolean is_obj = cJSON_IsObject(item);
    json_buffer_append_with_tag(buffer, is_obj ? "{\n" : "[\n", NULL);

    cJSON *child = item->child;
    while (child) {
      char child_indent[64];
      snprintf(child_indent, sizeof(child_indent), "%*s", indent + 2, "");
      json_buffer_append_with_tag(buffer, child_indent, NULL);

      if (is_obj && child->string) {
        json_buffer_append_with_tag(buffer, "\"", "json_key");
        json_buffer_append_with_tag(buffer, child->string, "json_key");
        json_buffer_append_with_tag(buffer, "\"", "json_key");
        json_buffer_append_with_tag(buffer, ": ", NULL);
      }

      json_highlighted_to_buffer(child, buffer, indent + 2);

      if (child->next)
        json_buffer_append_with_tag(buffer, ",", NULL);

      json_buffer_append_with_tag(buffer, "\n", NULL);
      child = child->next;
    }

    json_buffer_append_with_tag(buffer, indent_buf, NULL);
    json_buffer_append_with_tag(buffer, is_obj ? "}" : "]", NULL);

  } else if (cJSON_IsString(item)) {
    json_buffer_append_with_tag(buffer, "\"", "json_string");
    json_buffer_append_with_tag(buffer, item->valuestring, "json_string");
    json_buffer_append_with_tag(buffer, "\"", "json_string");

  } else if (cJSON_IsNumber(item)) {
    char num_buf[64];
    snprintf(num_buf, sizeof(num_buf), "%g", item->valuedouble);
    json_buffer_append_with_tag(buffer, num_buf, "json_number");

  } else if (cJSON_IsBool(item)) {
    json_buffer_append_with_tag(buffer, cJSON_IsTrue(item) ? "true" : "false",
                                "json_bool");

  } else if (cJSON_IsNull(item)) {
    json_buffer_append_with_tag(buffer, "null", "json_null");
  }
}
