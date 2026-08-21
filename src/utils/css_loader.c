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

#include "css_loader.h"

#include <gtk/gtk.h>

#define APP_CSS_RESOURCE_PATH "/io/github/finotilucas/requesthub/app.css"
#define APP_CSS_DEV_PATH "src/ui/styles/app.css"

static GtkCssProvider *css_provider = NULL;

static void load_app_css(void) {
#ifdef NDEBUG
  gtk_css_provider_load_from_resource(css_provider, APP_CSS_RESOURCE_PATH);
#else
  GFile *file = g_file_new_for_path(APP_CSS_DEV_PATH);
  gtk_css_provider_load_from_file(css_provider, file);
  g_object_unref(file);
#endif
}

#ifndef NDEBUG
static void on_css_file_changed(GFileMonitor *monitor, GFile *file,
                                GFile *other_file,
                                GFileMonitorEvent event_type,
                                gpointer user_data) {
  (void)monitor;
  (void)file;
  (void)other_file;
  (void)user_data;

  if (event_type == G_FILE_MONITOR_EVENT_CHANGED ||
      event_type == G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT) {
    load_app_css();
  }
}

/* Hot-reload do CSS em builds de debug; depende de rodar a partir da raiz do
 * repositorio. O monitor vive deliberadamente ate o fim do processo. */
static void watch_dev_css(void) {
  GFile *file = g_file_new_for_path(APP_CSS_DEV_PATH);
  GFileMonitor *monitor =
      g_file_monitor_file(file, G_FILE_MONITOR_NONE, NULL, NULL);
  g_object_unref(file);

  if (monitor == NULL) {
    return;
  }
  g_signal_connect(monitor, "changed", G_CALLBACK(on_css_file_changed), NULL);
}
#endif

void css_loader_init(void) {
  css_provider = gtk_css_provider_new();
  load_app_css();

  gtk_style_context_add_provider_for_display(
      gdk_display_get_default(), GTK_STYLE_PROVIDER(css_provider),
      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

#ifndef NDEBUG
  watch_dev_css();
#endif
}
