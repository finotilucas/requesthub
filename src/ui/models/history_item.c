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

#include "history_item.h"

struct _HistoryItem {
  GObject parent_instance;
  HistoryEntry *entry;
};

G_DEFINE_FINAL_TYPE(HistoryItem, history_item, G_TYPE_OBJECT)

HistoryItem *history_item_new(HistoryEntry *entry) {
  HistoryItem *self = g_object_new(HISTORY_TYPE_ITEM, NULL);
  self->entry = entry;
  return self;
}

HistoryEntry *history_item_get_entry(HistoryItem *self) {
  g_return_val_if_fail(HISTORY_IS_ITEM(self), NULL);
  return self->entry;
}

static void history_item_class_init(HistoryItemClass *klass) { (void)klass; }
static void history_item_init(HistoryItem *self) { (void)self; }
