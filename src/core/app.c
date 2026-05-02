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

#include "app.h"
#include "../config/app_config.h"
#include "../history/history.h"
#include "../http/http.h"
#include "../http/request.h"
#include "../http/response.h"
#include "../ui/components/history_sidebar.h"
#include "../ui/views/body_view.h"
#include "../ui/views/headers_view.h"
#include "../ui/views/params_view.h"
#include "../ui/views/request_view.h"
#include "../ui/views/response_view.h"
#include "../utils/css_loader.h"
#include "../utils/shortcuts_factory.h"

#include <curl/curl.h>
#include <gtk/gtk.h>
#include <string.h>

#define INITIAL_REQUEST_PANE_WIDTH 450

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

static void async_request_data_free(AsyncRequestData *data) {
  if (data == NULL) {
    return;
  }
  if (data->history_entry != NULL) {
    history_entry_free(data->history_entry);
  }
  g_free(data);
}

static void capture_header_to_entry(const char *key, const char *value,
                                    gpointer user_data) {
  HistoryEntry *entry = user_data;
  history_entry_add_header(entry, key, value);
}

static void capture_param_to_entry(const char *key, const char *value,
                                   gpointer user_data) {
  HistoryEntry *entry = user_data;
  history_entry_add_query_param(entry, key, value);
}

static HistoryEntry *build_history_entry_from_views(RequestView *view,
                                                    const char *url,
                                                    HttpMethods method) {
  HistoryEntry *entry = history_entry_new();
  if (entry == NULL) {
    return NULL;
  }

  entry->method = method;
  history_entry_set_url(entry, url);

  ParamsView *pv = request_view_get_params_view(view);
  if (pv != NULL) {
    params_view_for_each(pv, capture_param_to_entry, entry);
  }

  HeadersView *hv = request_view_get_headers_view(view);
  if (hv != NULL) {
    headers_view_for_each(hv, capture_header_to_entry, entry);
  }

  BodyView *bv = request_view_get_body_view(view);
  if (bv != NULL) {
    char *content = body_view_get_content(bv);
    if (content != NULL && *content != '\0') {
      history_entry_set_body(entry, content);
    }
    g_free(content);
  }

  return entry;
}

static HttpResponse *response_snapshot_from_entry(const HistoryEntry *entry) {
  if (entry == NULL || entry->http_status <= 0) {
    return NULL;
  }

  HttpResponse *resp = http_response_create();
  resp->http_status = entry->http_status;
  resp->total_time = entry->total_time_s;

  if (entry->response_body != NULL) {
    g_free(resp->body);
    resp->body = g_strdup(entry->response_body);
    resp->body_size = strlen(resp->body);
  } else {
    resp->body_size = entry->response_size;
  }

  if (entry->response_content_type != NULL) {
    resp->content_type = g_strdup(entry->response_content_type);
  }

  return resp;
}

static void on_history_entry_selected(HistorySidebar *sidebar,
                                      gpointer entry_ptr, gpointer user_data) {
  (void)sidebar;
  AppContext *ctx = user_data;
  HistoryEntry *entry = entry_ptr;
  if (ctx == NULL || ctx->request_view == NULL || entry == NULL) {
    return;
  }

  request_view_load_history_entry(ctx->request_view, entry);

  if (ctx->response_view != NULL) {
    HttpResponse *snapshot = response_snapshot_from_entry(entry);
    response_view_update(ctx->response_view, snapshot);
    if (snapshot != NULL) {
      http_response_free(snapshot);
    }
  }
}

static void request_worker_thread(GTask *task, gpointer source_obj,
                                  gpointer task_data,
                                  GCancellable *cancellable) {
  (void)cancellable;
  (void)source_obj;
  AsyncRequestData *rd = (AsyncRequestData *)task_data;

  HttpResponse *resp = http_request_perform(rd->request);

  g_task_return_pointer(task, resp, (GDestroyNotify)http_response_free);

  http_request_free(rd->request);
}

static void on_request_finished(GObject *source, GAsyncResult *res,
                                gpointer user_data) {
  (void)source;
  AsyncRequestData *rd = (AsyncRequestData *)user_data;
  HttpResponse *resp = g_task_propagate_pointer(G_TASK(res), NULL);

  response_view_update(rd->ctx->response_view, resp);
  request_top_bar_set_loading(rd->bar, FALSE);

  if (rd->history_entry != NULL && rd->ctx != NULL &&
      rd->ctx->history_sidebar != NULL) {
    history_entry_apply_response(rd->history_entry, resp);
    history_sidebar_record(rd->ctx->history_sidebar, rd->history_entry);
    rd->history_entry = NULL;
  }

  async_request_data_free(rd);
}

static void on_send_clicked(RequestTopBar *bar, gpointer user_data) {
  AppContext *ctx = (AppContext *)user_data;

  const char *url = request_top_bar_get_url(bar);
  HttpMethods method = request_top_bar_get_method(bar);

  HttpRequest *req = http_request_new(url, method);
  if (req == NULL) {
    return;
  }

  HistoryEntry *entry =
      build_history_entry_from_views(ctx->request_view, url, method);

  ParamsView *pv = request_view_get_params_view(ctx->request_view);
  if (pv)
    params_view_apply_to_request(pv, req);

  HeadersView *hv = request_view_get_headers_view(ctx->request_view);
  if (hv)
    headers_view_apply_to_request(hv, req);

  BodyView *bv = request_view_get_body_view(ctx->request_view);
  if (bv)
    body_view_apply_to_request(bv, req);

  request_top_bar_set_loading(bar, TRUE);

  AsyncRequestData *rd = g_new0(AsyncRequestData, 1);
  rd->request = req;
  rd->bar = bar;
  rd->ctx = ctx;
  rd->history_entry = entry;

  GTask *task = g_task_new(NULL, NULL, on_request_finished, rd);
  g_task_set_task_data(task, rd, NULL);
  g_task_run_in_thread(task, request_worker_thread);
  g_object_unref(task);
}

static void on_shortcut_send_wrapper(GSimpleAction *action, GVariant *parameter,
                                     gpointer user_data) {
  (void)action;
  (void)parameter;

  GtkWidget *window = GTK_WIDGET(user_data);
  AppContext *ctx = g_object_get_data(G_OBJECT(window), "app-ctx");

  if (ctx && ctx->request_view) {
    RequestTopBar *bar = request_view_get_top_bar(ctx->request_view);
    on_send_clicked(bar, ctx);
  }
}

static void on_shortcut_focus_url_wrapper(GSimpleAction *action,
                                          GVariant *parameter,
                                          gpointer user_data) {
  (void)action;
  (void)parameter;

  GtkWidget *window = GTK_WIDGET(user_data);
  AppContext *ctx = g_object_get_data(G_OBJECT(window), "app-ctx");

  if (ctx && ctx->request_view) {
    RequestTopBar *bar = request_view_get_top_bar(ctx->request_view);
    request_top_bar_focus_url(bar);
  }
}

static void apply_global_theming(void) {
  load_css();
  watch_css_file("src/ui/styles/app.css");

  GtkSettings *settings = gtk_settings_get_default();
  g_object_set(settings, "gtk-theme-name", "Adwaita-dark",
               "gtk-application-prefer-dark-theme", TRUE, NULL);
}

static GtkWidget *build_main_layout(RequestView *request_view,
                                    ResponseView *response_view,
                                    HistorySidebar *history_sidebar) {
  GtkWidget *inner_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_start_child(GTK_PANED(inner_paned), GTK_WIDGET(request_view));
  gtk_paned_set_end_child(GTK_PANED(inner_paned), GTK_WIDGET(response_view));
  gtk_paned_set_position(GTK_PANED(inner_paned), INITIAL_REQUEST_PANE_WIDTH);
  gtk_paned_set_shrink_end_child(GTK_PANED(inner_paned), FALSE);

  GtkWidget *outer_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_start_child(GTK_PANED(outer_paned),
                            GTK_WIDGET(history_sidebar));
  gtk_paned_set_end_child(GTK_PANED(outer_paned), inner_paned);
  gtk_paned_set_position(GTK_PANED(outer_paned), HISTORY_SIDEBAR_WIDTH);
  gtk_paned_set_resize_start_child(GTK_PANED(outer_paned), FALSE);
  gtk_paned_set_shrink_start_child(GTK_PANED(outer_paned), FALSE);

  return outer_paned;
}

static void wire_app_signals(GtkWidget *window, AppContext *ctx) {
  g_signal_connect(request_view_get_top_bar(ctx->request_view), "send-clicked",
                   G_CALLBACK(on_send_clicked), ctx);
  g_signal_connect(ctx->history_sidebar, "entry-selected",
                   G_CALLBACK(on_history_entry_selected), ctx);

  g_object_set_data_full(G_OBJECT(window), "app-ctx", ctx, g_free);
}

static void install_shortcuts(GtkApplication *app,
                              GtkApplicationWindow *window) {
  static const ShortcutEntry app_shortcuts[] = {
      {"send_request", "<Control>Return", on_shortcut_send_wrapper},
      {"focus_url", "<Control>l", on_shortcut_focus_url_wrapper},
  };

  setup_application_shortcuts(app, window, app_shortcuts,
                              G_N_ELEMENTS(app_shortcuts));
}

void on_activate(GtkApplication *app, gpointer user_data) {
  AppConfig *cfg = (AppConfig *)user_data;

  apply_global_theming();

  GtkWidget *window = gtk_application_window_new(app);
  app_config_apply_to_window(cfg, GTK_WINDOW(window));

  RequestView *request_view = request_view_new();
  ResponseView *response_view = response_view_new();
  HistorySidebar *history_sidebar = history_sidebar_new();

  GtkWidget *layout =
      build_main_layout(request_view, response_view, history_sidebar);

  AppContext *ctx = g_new0(AppContext, 1);
  ctx->response_view = response_view;
  ctx->request_view = request_view;
  ctx->history_sidebar = history_sidebar;

  wire_app_signals(window, ctx);
  install_shortcuts(app, GTK_APPLICATION_WINDOW(window));

  gtk_window_set_child(GTK_WINDOW(window), layout);
  gtk_window_present(GTK_WINDOW(window));
}
