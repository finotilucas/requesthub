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

#include <gtk/gtk.h>

static GtkCssProvider *css_provider = NULL;

void load_css(void) {
  if (!css_provider)
    css_provider = gtk_css_provider_new();

  GFile *file = g_file_new_for_path("src/ui/styles/app.css");

  gtk_css_provider_load_from_file(css_provider, file);

  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  g_object_unref(file);
}

static void css_file_changed(GFileMonitor *monitor, GFile *file,
                             GFile *other_file, GFileMonitorEvent event_type,
                             gpointer user_data) {

  (void)monitor;
  (void)file;
  (void)other_file;
  (void)user_data;

  if (event_type == G_FILE_MONITOR_EVENT_CHANGED ||
      event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
    g_print("CSS changed, reloading...\n");
    load_css();
  }
}

void watch_css_file(const char *path) {
  GFile *file = g_file_new_for_path(path);
  GFileMonitor *monitor =
      g_file_monitor_file(file, G_FILE_MONITOR_NONE, NULL, NULL);
  g_signal_connect(monitor, "changed", G_CALLBACK(css_file_changed), NULL);
  g_object_unref(file);
}
