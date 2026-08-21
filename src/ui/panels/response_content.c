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

#include "response_content.h"
#include "../../utils/body_syntax.h"
#include "../components/source_editor.h"
#include <curl/curl.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

struct _ResponseContent {
  GtkBox parent_instance;
  GtkStack *stack;
  GtkSourceView *body_view;
  GtkSourceBuffer *body_buffer;
};

G_DEFINE_FINAL_TYPE(ResponseContent, response_content, GTK_TYPE_BOX)

static GtkWidget *create_shortcut_row(const char *action, const char *keys) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 40);
  gtk_widget_set_margin_bottom(box, 10);

  GtkWidget *action_label = gtk_label_new(action);
  GtkWidget *keys_label = gtk_label_new(keys);

  gtk_widget_set_hexpand(action_label, TRUE);
  gtk_widget_set_halign(action_label, GTK_ALIGN_START);
  gtk_widget_set_halign(keys_label, GTK_ALIGN_END);

  gtk_widget_add_css_class(keys_label, "dim-label");

  gtk_box_append(GTK_BOX(box), action_label);
  gtk_box_append(GTK_BOX(box), keys_label);

  return box;
}

static gboolean content_type_is_json(const char *content_type) {
  if (content_type == NULL) {
    return FALSE;
  }
  return g_str_has_prefix(content_type, "application/json") ||
         g_strstr_len(content_type, -1, "+json") != NULL;
}

static void response_content_class_init(ResponseContentClass *klass) {
  (void)klass;
}

static void response_content_init(ResponseContent *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  self->stack = GTK_STACK(gtk_stack_new());
  gtk_stack_set_transition_type(self->stack,
                                GTK_STACK_TRANSITION_TYPE_CROSSFADE);
  gtk_stack_set_transition_duration(self->stack, 200);
  gtk_widget_set_vexpand(GTK_WIDGET(self->stack), TRUE);

  GtkWidget *empty_center_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_valign(empty_center_box, GTK_ALIGN_CENTER);
  gtk_widget_set_halign(empty_center_box, GTK_ALIGN_CENTER);

  GtkWidget *shortcuts_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_set_size_request(shortcuts_container, 350, -1);

  gtk_box_append(GTK_BOX(shortcuts_container),
                 create_shortcut_row("Send Request", "Ctrl + Enter"));
  gtk_box_append(GTK_BOX(shortcuts_container),
                 create_shortcut_row("Focus URL", "Ctrl + L"));
  gtk_box_append(GTK_BOX(empty_center_box), shortcuts_container);

  self->body_view = source_editor_new("json", 2);
  self->body_buffer = GTK_SOURCE_BUFFER(
      gtk_text_view_get_buffer(GTK_TEXT_VIEW(self->body_view)));
  gtk_text_view_set_editable(GTK_TEXT_VIEW(self->body_view), FALSE);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(self->body_view), TRUE);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(self->body_view));
  gtk_widget_set_vexpand(scrolled, TRUE);

  gtk_stack_add_named(self->stack, empty_center_box, "empty");
  gtk_stack_add_named(self->stack, scrolled, "data");

  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->stack));

  gtk_stack_set_visible_child_name(self->stack, "empty");
}

ResponseContent *response_content_new(void) {
  return g_object_new(RESPONSE_TYPE_CONTENT, NULL);
}


void response_content_set_response(ResponseContent *self, HttpResponse *resp) {
  g_return_if_fail(RESPONSE_IS_CONTENT(self));

  if (!resp) {
    gtk_stack_set_visible_child_name(self->stack, "empty");
    return;
  }

  if (resp->curl_code != CURLE_OK) {
    gchar *message =
        g_strdup_printf("Network error (%d): %s\n\n%s", resp->curl_code,
                        curl_easy_strerror(resp->curl_code),
                        "The request did not reach a successful response. "
                        "Check the URL, your connection, and TLS settings.");
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->body_buffer), message, -1);
    g_free(message);
    gtk_stack_set_visible_child_name(self->stack, "data");
    return;
  }

  if (resp->body && g_utf8_validate(resp->body, -1, NULL)) {
    if (content_type_is_json(resp->content_type)) {
      gchar *pretty = body_syntax_format_json(resp->body);
      if (pretty != NULL) {
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->body_buffer), pretty,
                                 -1);
        g_free(pretty);
      } else {
        gchar *annotated = g_strdup_printf(
            "/* Server advertised %s but body did not parse as JSON. */\n%s",
            resp->content_type, resp->body);
        gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->body_buffer), annotated,
                                 -1);
        g_free(annotated);
      }
    } else {
      gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->body_buffer), resp->body,
                               -1);
    }
  } else {
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->body_buffer),
                             "[Binary or Invalid Content]", -1);
  }

  gtk_stack_set_visible_child_name(self->stack, "data");
}
