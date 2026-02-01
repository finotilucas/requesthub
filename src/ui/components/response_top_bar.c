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

#include "response_top_bar.h"
#include "../../utils/format.h"
#include <gtk/gtk.h>

struct _ResponseTopBar {
  GtkBox parent_instance;

  GtkLabel *status_value_label;
  GtkLabel *time_label;
  GtkLabel *size_label;
};

G_DEFINE_TYPE(ResponseTopBar, response_top_bar, GTK_TYPE_BOX)

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

static void response_top_bar_class_init(ResponseTopBarClass *klass) {
  (void)klass;
}

static void response_top_bar_init(ResponseTopBar *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_HORIZONTAL);

  gtk_widget_set_margin_start(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_end(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_top(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_bottom(GTK_WIDGET(self), 10);
  gtk_box_set_spacing(GTK_BOX(self), 8);

  self->status_value_label = GTK_LABEL(gtk_label_new("---"));
  self->time_label = GTK_LABEL(gtk_label_new("0ms"));
  self->size_label = GTK_LABEL(gtk_label_new("0B"));

  gtk_widget_add_css_class(GTK_WIDGET(self->status_value_label), "badge");
  gtk_widget_add_css_class(GTK_WIDGET(self->time_label), "badge");
  gtk_widget_add_css_class(GTK_WIDGET(self->size_label), "badge");

  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->status_value_label));
  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->time_label));
  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->size_label));
}

ResponseTopBar *response_top_bar_new(void) {
  return g_object_new(RESPONSE_TYPE_TOP_BAR, NULL);
}

void response_top_bar_update(ResponseTopBar *self, HttpResponse *resp) {
  g_return_if_fail(RESPONSE_IS_TOP_BAR(self));
  if (!resp)
    return;

  char buf[128];
  g_snprintf(buf, sizeof(buf), "%ld", resp->http_status);
  gtk_label_set_text(self->status_value_label, buf);
  apply_status_style(GTK_WIDGET(self->status_value_label),
                     (int)resp->http_status);

  char *time_str = format_response_time(resp->total_time * 1000.0);
  gtk_label_set_text(self->time_label, time_str);
  g_free(time_str);

  size_t body_len = resp->body ? strlen(resp->body) : 0;
  char *size_str = format_response_size(body_len);
  gtk_label_set_text(self->size_label, size_str);
  g_free(size_str);
}
