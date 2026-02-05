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

#include "src/http/http.h"
#include "src/http/http_pool.h"
#include "src/http/request.h"
#include "src/ui/views/body_view.h"
#include "src/ui/views/headers_view.h"
#include "src/ui/views/params_view.h"
#include "src/ui/views/request_view.h"
#include "src/ui/views/response_view.h"
#include "src/utils/css_loader.h"
#include "src/utils/shortcuts_factory.h"
#include <curl/curl.h>
#include <gtk/gtk.h>

typedef struct {
  ResponseView *response_view;
  RequestView *request_view;
} AppContext;

typedef struct {
  HttpRequest *request;
  RequestTopBar *bar;
  AppContext *ctx;
} AsyncRequestData;

static void async_request_data_free(AsyncRequestData *data) { g_free(data); }

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

  async_request_data_free(rd);
}

static void on_send_clicked(RequestTopBar *bar, gpointer user_data) {
  AppContext *ctx = (AppContext *)user_data;

  const char *url = request_top_bar_get_url(bar);
  HttpMethods method = request_top_bar_get_method(bar);

  HttpRequest *req = http_request_new(url, method);

  ParamsView *pv = request_view_get_params_view(ctx->request_view);
  if (pv && req) {
    params_view_apply_to_request(pv, req);
  }

  HeadersView *hv = request_view_get_headers_view(ctx->request_view);
  if (hv && req) {
    headers_view_apply_to_request(hv, req);
  }

  BodyView *bv = request_view_get_body_view(ctx->request_view);
  if (bv && req) {
    body_view_apply_to_request(bv, req);
  }

  request_top_bar_set_loading(bar, TRUE);

  AsyncRequestData *rd = g_new0(AsyncRequestData, 1);
  rd->request = req;
  rd->bar = bar;
  rd->ctx = ctx;

  GTask *task = g_task_new(NULL, NULL, on_request_finished, rd);
  g_task_set_task_data(task, rd, NULL);
  g_task_run_in_thread(task, request_worker_thread);
  g_object_unref(task);
}

static void on_shortcut_send_wrapper(GSimpleAction *action, GVariant *parameter,
                                     gpointer user_data) {
  GtkWidget *window = GTK_WIDGET(user_data);
  AppContext *ctx = g_object_get_data(G_OBJECT(window), "app-ctx");

  if (ctx && ctx->request_view) {
    RequestTopBar *bar = request_view_get_top_bar(ctx->request_view);
    on_send_clicked(bar, ctx);
  }

  (void)action;
  (void)parameter;
}

static void on_shortcut_focus_url_wrapper(GSimpleAction *action,
                                          GVariant *parameter,
                                          gpointer user_data) {
  GtkWidget *window = GTK_WIDGET(user_data);
  AppContext *ctx = g_object_get_data(G_OBJECT(window), "app-ctx");

  if (ctx && ctx->request_view) {
    RequestTopBar *bar = request_view_get_top_bar(ctx->request_view);
    request_top_bar_focus_url(bar);
  }

  (void)action;
  (void)parameter;
}

static void on_activate(GtkApplication *app) {
  load_css();
  watch_css_file("src/ui/styles/app.css");

  GtkSettings *settings = gtk_settings_get_default();
  g_object_set(settings, "gtk-theme-name", "Adwaita-dark",
               "gtk-application-prefer-dark-theme", TRUE, NULL);

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  RequestView *request_view = request_view_new();
  ResponseView *response_view = response_view_new();

  gtk_paned_set_start_child(GTK_PANED(paned), GTK_WIDGET(request_view));
  gtk_paned_set_end_child(GTK_PANED(paned), GTK_WIDGET(response_view));
  gtk_paned_set_position(GTK_PANED(paned), 450);

  AppContext *ctx = g_new0(AppContext, 1);
  ctx->response_view = response_view;
  ctx->request_view = request_view;

  g_signal_connect(request_view_get_top_bar(request_view), "send-clicked",
                   G_CALLBACK(on_send_clicked), ctx);

  g_object_set_data_full(G_OBJECT(window), "app-ctx", ctx, g_free);

  static const ShortcutEntry app_shortcuts[] = {
      {"send_request", "<Control>Return", on_shortcut_send_wrapper},
      {"focus_url", "<Control>l", on_shortcut_focus_url_wrapper},
  };

  setup_application_shortcuts(app, GTK_APPLICATION_WINDOW(window),
                              app_shortcuts, G_N_ELEMENTS(app_shortcuts));

  gtk_window_set_child(GTK_WINDOW(window), paned);
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  curl_global_init(CURL_GLOBAL_ALL);
  http_pool_init();

  GtkApplication *app =
      gtk_application_new("com.requesthub.app", G_APPLICATION_DEFAULT_FLAGS);

  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

  int status = g_application_run(G_APPLICATION(app), argc, argv);

  g_object_unref(app);

  http_pool_cleanup();
  curl_global_cleanup();

  return status;
}
