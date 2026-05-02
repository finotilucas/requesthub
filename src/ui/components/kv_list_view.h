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

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define KV_TYPE_LIST_VIEW (kv_list_view_get_type())
G_DECLARE_FINAL_TYPE(KvListView, kv_list_view, KV, LIST_VIEW, GtkBox)

typedef struct {
  const char *title;
  const char *add_button_label;
  const char *clear_button_label;
  const char *key_placeholder;
  const char *value_placeholder;
  gboolean add_prepends;
} KvListViewConfig;

typedef enum {
  KV_LIST_ITER_ALL,
  KV_LIST_ITER_EDITABLE,
} KvListIterMode;

typedef void (*KvListIterFunc)(const char *key, const char *value,
                               gpointer user_data);

KvListView *kv_list_view_new(const KvListViewConfig *config);

void kv_list_view_add_fixed(KvListView *self, const char *key,
                            const char *value, const char *info_tooltip);

void kv_list_view_add_editable(KvListView *self, const char *key,
                               const char *value);

void kv_list_view_clear_editable(KvListView *self);

void kv_list_view_for_each(KvListView *self, KvListIterMode mode,
                           KvListIterFunc func, gpointer user_data);

G_END_DECLS
