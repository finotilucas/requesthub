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

#ifndef APP_H
#define APP_H

#include "../history/history.h"
#include "../http/request.h"
#include "../ui/components/history_sidebar.h"
#include "../ui/views/request_view.h"
#include "../ui/views/response_view.h"
#include <gtk/gtk.h>

typedef struct {
  ResponseView *response_view;
  RequestView *request_view;
  HistorySidebar *history_sidebar;
} AppContext;

typedef struct {
  HttpRequest *request;
  RequestTopBar *bar;
  AppContext *ctx;
  HistoryEntry *history_entry;
} AsyncRequestData;

void async_request_data_free(AsyncRequestData *data);

void request_worker_thread(GTask *task, gpointer source_obj, gpointer task_data,
                           GCancellable *cancellable);

void on_request_finished(GObject *source, GAsyncResult *res,
                         gpointer user_data);

void on_send_clicked(RequestTopBar *bar, gpointer user_data);

void on_shortcut_send_wrapper(GSimpleAction *action, GVariant *parameter,
                              gpointer user_data);

void on_shortcut_focus_url_wrapper(GSimpleAction *action, GVariant *parameter,
                                   gpointer user_data);

void on_activate(GtkApplication *app, gpointer user_data);

#endif
