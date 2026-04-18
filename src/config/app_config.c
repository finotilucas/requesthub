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

#include "app_config.h"

static const gchar *DEFAULT_APP_ID = "com.requesthub.app";
static const gchar *DEFAULT_WINDOW_TITLE = "Request Hub";
static const gint DEFAULT_WIDTH = 1024;
static const gint DEFAULT_HEIGHT = 768;

AppConfig *app_config_new(const gchar *app_id) {
  AppConfig *self = g_new0(AppConfig, 1);

  self->app_id = g_strdup(app_id && *app_id ? app_id : DEFAULT_APP_ID);
  self->window_title = g_strdup(DEFAULT_WINDOW_TITLE);
  self->default_width = DEFAULT_WIDTH;
  self->default_height = DEFAULT_HEIGHT;
  self->maximize_on_start = FALSE;

  return self;
}

void app_config_free(AppConfig *self) {
  if (self == NULL) {
    return;
  }

  g_free(self->app_id);
  g_free(self->window_title);
  g_free(self);
}

gboolean app_config_apply_to_window(const AppConfig *self, GtkWindow *window) {
  if (self == NULL || window == NULL) {
    return FALSE;
  }

  gtk_window_set_title(window, self->window_title);
  gtk_window_set_default_size(window, self->default_width,
                              self->default_height);

  if (self->maximize_on_start) {
    gtk_window_maximize(window);
  }

  return TRUE;
}
