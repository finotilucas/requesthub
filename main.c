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

#include "src/config/app_config.h"
#include "src/core/app.h"
#include "src/http/http_pool.h"

#include <curl/curl.h>
#include <gtk/gtk.h>

int main(int argc, char **argv) {
  curl_global_init(CURL_GLOBAL_ALL);
  http_pool_init();

  AppConfig *cfg = app_config_new();

  GtkApplication *app =
      gtk_application_new(cfg->app_id, G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect(app, "activate", G_CALLBACK(on_activate), cfg);

  int status = g_application_run(G_APPLICATION(app), argc, argv);

  g_object_unref(app);
  app_config_free(cfg);

  http_pool_cleanup();
  curl_global_cleanup();

  return status;
}
