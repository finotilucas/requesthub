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

#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "../config/app_config.h"

#include <adwaita.h>

G_BEGIN_DECLS

#define APP_TYPE_CONTROLLER (app_controller_get_type())
G_DECLARE_FINAL_TYPE(AppController, app_controller, APP, CONTROLLER, GObject)

AppController *app_controller_new(AdwApplication *application,
                                  AppConfig *config);
void app_controller_present(AppController *self);
void app_controller_trigger_send(AppController *self);
void app_controller_focus_url(AppController *self);

G_END_DECLS

#endif
