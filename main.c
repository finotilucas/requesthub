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

#include "src/controllers/app_controller.h"
#include "src/http/http_pool.h"

#include <adwaita.h>
#include <curl/curl.h>

#define REQUESTHUB_APP_ID "io.github.finotilucas.requesthub"

static void on_app_activate(GtkApplication *app, gpointer user_data) {
  (void)user_data;
  app_controller_present(app_controller_new(ADW_APPLICATION(app)));
}

int main(int argc, char **argv) {
  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
    g_printerr("requesthub: failed to initialize libcurl\n");
    return 1;
  }
  http_pool_init();

  gtk_init();
  g_object_set(gtk_settings_get_default(),
               "gtk-application-prefer-dark-theme", FALSE, NULL);

  AdwApplication *app =
      adw_application_new(REQUESTHUB_APP_ID, G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);

  int status = g_application_run(G_APPLICATION(app), argc, argv);

  g_object_unref(app);

  http_pool_cleanup();
  curl_global_cleanup();

  return status;
}
