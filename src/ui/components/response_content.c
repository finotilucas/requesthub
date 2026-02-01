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

#include "response_content.h"
#include "source_view.h"
#include <cjson/cJSON.h>
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

struct _ResponseContent {
  GtkBox parent_instance;
  GtkSourceView *body_view;
  GtkSourceBuffer *body_buffer;
};

G_DEFINE_TYPE(ResponseContent, response_content, GTK_TYPE_BOX)

static void response_content_finalize(GObject *object) {
  ResponseContent *self = RESPONSE_CONTENT(object);

  if (self->body_buffer)
    g_object_unref(self->body_buffer);

  G_OBJECT_CLASS(response_content_parent_class)->finalize(object);
}

static void response_content_class_init(ResponseContentClass *klass) {
  G_OBJECT_CLASS(klass)->finalize = response_content_finalize;
}

static void response_content_init(ResponseContent *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  self->body_view = GTK_SOURCE_VIEW(source_view_new(&self->body_buffer));

  gtk_text_view_set_editable(GTK_TEXT_VIEW(self->body_view), FALSE);
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(self->body_view), TRUE);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(self->body_view));
  gtk_widget_set_vexpand(scrolled, TRUE);

  gtk_box_append(GTK_BOX(self), scrolled);
}

ResponseContent *response_content_new(void) {
  return g_object_new(RESPONSE_TYPE_VIEW, NULL);
}

void response_content_clear(ResponseContent *self) {
  g_return_if_fail(RESPONSE_IS_CONTENT(self));
  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->body_buffer), "", -1);
}

void response_content_set_response(ResponseContent *self, HttpResponse *resp) {
  g_return_if_fail(RESPONSE_IS_CONTENT(self));
  if (!resp)
    return;

  if (resp->body && g_utf8_validate(resp->body, -1, NULL)) {
    cJSON *json = cJSON_Parse(resp->body);

    if (json) {
      json_buffer_set_from_cjson(GTK_TEXT_BUFFER(self->body_buffer), json);
      cJSON_Delete(json);
    } else {
      gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->body_buffer), resp->body,
                               -1);
    }
  } else {
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(self->body_buffer),
                             "[Binary or Invalid Content]", -1);
  }
}
