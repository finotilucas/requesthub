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

#include "shortcuts_factory.h"

void setup_application_shortcuts(GtkApplication *app,
                                 GtkApplicationWindow *window,
                                 const ShortcutEntry *entries, int n_entries) {
  for (int i = 0; i < n_entries; i++) {
    GSimpleAction *action = g_simple_action_new(entries[i].name, NULL);
    g_signal_connect(action, "activate", G_CALLBACK(entries[i].cb), window);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(action));

    char *full_action_name = g_strdup_printf("win.%s", entries[i].name);
    const char *accels[] = {entries[i].accel, NULL};
    gtk_application_set_accels_for_action(app, full_action_name, accels);

    g_free(full_action_name);
    g_object_unref(action);
  }
}
