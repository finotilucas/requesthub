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

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <glib.h>
#include <gtk/gtk.h>

#define APP_ID "com.requesthub.app"

typedef struct _AppConfig AppConfig;

struct _AppConfig {
  gchar *app_id;
  gchar *window_title;
  gint default_width;
  gint default_height;
  gboolean maximize_on_start;
};

_Static_assert(sizeof(AppConfig) > 0, "AppConfig must not be empty");

AppConfig *app_config_new(const gchar *app_id);
void app_config_free(AppConfig *self);
gboolean app_config_apply_to_window(const AppConfig *self, GtkWindow *window);

#endif
