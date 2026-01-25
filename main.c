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
 * (at your option) any later version.
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

#include <curl/curl.h>
#include <gtk/gtk.h>

static void on_activate(GtkApplication *app, gpointer user_data) {
  (void)user_data;

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "RequestHub");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  GtkWidget *label = gtk_label_new("RequestHub");
  gtk_window_set_child(GTK_WINDOW(window), label);

  gtk_window_present(GTK_WINDOW(window));
}

int main(void) {
  GtkApplication *app;
  int status;

  app = gtk_application_new("com.requesthub.app", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

  status = g_application_run(G_APPLICATION(app), 0, NULL);

  g_object_unref(app);
  return status;
}
