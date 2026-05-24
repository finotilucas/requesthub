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

#ifndef PARAMS_PANEL_H
#define PARAMS_PANEL_H

#include "../../http/request.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define TYPE_PARAMS_PANEL (params_panel_get_type())
G_DECLARE_FINAL_TYPE(ParamsPanel, params_panel, PARAMS, PANEL, GtkBox)

ParamsPanel *params_panel_new(void);
void params_panel_apply_to_request(ParamsPanel *self, HttpRequest *request);
void params_panel_clear_all(ParamsPanel *self);
void params_panel_for_each(ParamsPanel *self,
                          void (*func)(const char *key, const char *value,
                                       gpointer user_data),
                          gpointer user_data);
void params_panel_add_pair(ParamsPanel *self, const char *key, const char *value);

G_END_DECLS

#endif
