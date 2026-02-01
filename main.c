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
#include "src/ui/views/request_view.h"
#include "src/ui/views/response_view.h"
#include "src/utils/css_loader.h"

#include <curl/curl.h>
#include <gtk/gtk.h>

typedef struct {
  ResponseView *response_view;
} AppContext;

static void on_send_clicked(RequestTopBar *bar, gpointer user_data) {
  AppContext *ctx = (AppContext *)user_data;

  const char *url = request_top_bar_get_url(bar);
  HttpMethods method = request_top_bar_get_method(bar);

  HttpRequest *req = http_request_new(url, method);
  HttpResponse *resp = http_request_perform(req);

  response_view_update(ctx->response_view, resp);

  http_response_free(resp);
  http_request_free(req);
}

static void on_activate(GtkApplication *app) {
  load_css();
  watch_css_file("src/ui/styles/app.css");

  GtkSettings *settings = gtk_settings_get_default();
  g_object_set(settings, "gtk-theme-name", "Adwaita", NULL);
  g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

  RequestView *request_view = request_view_new();
  ResponseView *response_view = response_view_new();

  gtk_paned_set_start_child(GTK_PANED(paned), GTK_WIDGET(request_view));
  gtk_paned_set_end_child(GTK_PANED(paned), GTK_WIDGET(response_view));

  gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
  gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);
  gtk_paned_set_position(GTK_PANED(paned), 450);

  AppContext *ctx = g_new0(AppContext, 1);
  ctx->response_view = response_view;

  g_signal_connect(request_view_get_top_bar(request_view), "send-clicked",
                   G_CALLBACK(on_send_clicked), ctx);

  g_object_set_data_full(G_OBJECT(window), "app-ctx", ctx, g_free);
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
