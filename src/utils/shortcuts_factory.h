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

#ifndef SHORTCUTS_FACTORY_H
#define SHORTCUTS_FACTORY_H

#include <gtk/gtk.h>

typedef void (*ShortcutCallback)(GSimpleAction *action, GVariant *parameter,
                                 gpointer user_data);

typedef struct {
  const char *name;
  const char *accel;
  ShortcutCallback cb;
} ShortcutEntry;

void setup_application_shortcuts(GtkApplication *app,
                                 GtkApplicationWindow *window,
                                 const ShortcutEntry *entries, int n_entries);

#endif
