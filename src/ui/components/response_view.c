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

#include "response_view.h"
#include "../../utils/format.h"
#include "../../utils/json_highlighter.h"
#include "glib.h"

#include <stdio.h>
#include <string.h>

struct _ResponseView {
  GtkBox parent_instance;

  GtkWidget *header_container;
  GtkLabel *status_value_label;
  GtkLabel *time_label;
  GtkLabel *size_label;

  GtkTextView *body_view;
};

G_DEFINE_TYPE(ResponseView, response_view, GTK_TYPE_BOX)

static void apply_status_style(GtkWidget *label, int status) {
  static const char *classes[] = {"badge-success", "badge-warning",
                                  "badge-error", "badge-neutral", NULL};

  for (int i = 0; classes[i]; i++)
    gtk_widget_remove_css_class(label, classes[i]);

  if (status >= 200 && status < 300)
    gtk_widget_add_css_class(label, "badge-success");
  else if (status >= 400 && status < 500)
    gtk_widget_add_css_class(label, "badge-warning");
  else if (status >= 500)
    gtk_widget_add_css_class(label, "badge-error");
  else
    gtk_widget_add_css_class(label, "badge-neutral");
}

static void response_view_init(ResponseView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_box_set_spacing(GTK_BOX(self), 8);

  /* Header */
  self->header_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_widget_set_visible(self->header_container, FALSE);

  GtkWidget *status_prefix = gtk_label_new("");

  self->status_value_label = GTK_LABEL(gtk_label_new(""));
  self->time_label = GTK_LABEL(gtk_label_new(""));
  self->size_label = GTK_LABEL(gtk_label_new(""));

  gtk_box_append(GTK_BOX(self->header_container), status_prefix);
  gtk_box_append(GTK_BOX(self->header_container),
                 GTK_WIDGET(self->status_value_label));
  gtk_box_append(GTK_BOX(self->header_container), GTK_WIDGET(self->time_label));
  gtk_box_append(GTK_BOX(self->header_container), GTK_WIDGET(self->size_label));
  gtk_box_append(GTK_BOX(self), self->header_container);

  gtk_widget_add_css_class(GTK_WIDGET(self->status_value_label), "badge");
  gtk_widget_add_css_class(GTK_WIDGET(self->time_label), "badge");
  gtk_widget_add_css_class(GTK_WIDGET(self->size_label), "badge");

  gtk_widget_set_valign(GTK_WIDGET(self->status_value_label), GTK_ALIGN_CENTER);
  gtk_widget_set_valign(GTK_WIDGET(self->time_label), GTK_ALIGN_CENTER);
  gtk_widget_set_valign(GTK_WIDGET(self->size_label), GTK_ALIGN_CENTER);

  /* Body */
  self->body_view = GTK_TEXT_VIEW(gtk_text_view_new());
  gtk_text_view_set_editable(self->body_view, FALSE);
  gtk_text_view_set_monospace(self->body_view, TRUE);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(self->body_view));
  gtk_widget_set_vexpand(scrolled, TRUE);

  gtk_box_append(GTK_BOX(self), scrolled);
}

static void response_view_class_init(ResponseViewClass *klass) { (void)klass; }

ResponseView *response_view_new(void) {
  return g_object_new(RESPONSE_TYPE_VIEW, NULL);
}

void response_view_clear(ResponseView *self) {
  g_return_if_fail(RESPONSE_IS_VIEW(self));

  gtk_widget_set_visible(self->header_container, FALSE);

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(self->body_view);
  gtk_text_buffer_set_text(buffer, "", -1);
}

void response_view_set_response(ResponseView *self, HttpResponse *resp) {
  g_return_if_fail(RESPONSE_IS_VIEW(self));
  if (!resp)
    return;

  char buf[128];

  g_snprintf(buf, sizeof(buf), "%ld", resp->http_status);
  gtk_label_set_text(self->status_value_label, buf);
  apply_status_style(GTK_WIDGET(self->status_value_label),
                     (int)resp->http_status);

  char *time_str = format_time(resp->total_time * 1000.0);
  gtk_label_set_text(self->time_label, time_str);
  g_free(time_str);

  size_t body_len = resp->body ? strlen(resp->body) : 0;
  char *size_str = format_size(body_len);
  gtk_label_set_text(self->size_label, size_str);
  g_free(size_str);

  gtk_widget_set_visible(self->header_container, TRUE);

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(self->body_view);

  if (resp->body && g_utf8_validate(resp->body, -1, NULL)) {
    cJSON *json = cJSON_Parse(resp->body);

    if (json) {
      gtk_text_buffer_set_text(buffer, "", -1);
      json_highlighted_to_buffer(json, buffer, 0);
      cJSON_Delete(json);
    } else {
      gtk_text_buffer_set_text(buffer, resp->body, -1);
    }
  } else {
    gtk_text_buffer_set_text(buffer, "[Binary or Invalid Content]", -1);
  }
}
