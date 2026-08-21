/*******************************************************************************
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

#include "app_controller.h"

#include "../services/history_service.h"
#include "../services/http_service.h"
#include "../ui/panels/request_top_bar.h"
#include "../ui/views/history_view.h"
#include "../ui/views/request_view.h"
#include "../ui/views/response_view.h"
#include "../utils/css_loader.h"
#include "history_controller.h"
#include "request_controller.h"

#define APP_CTRL_DATA_KEY "app-controller"
#define WINDOW_TITLE "Request Hub"
#define WINDOW_DEFAULT_WIDTH 1280
#define WINDOW_DEFAULT_HEIGHT 768
#define INITIAL_REQUEST_PANE_WIDTH 450
#define SIDEBAR_MIN_WIDTH 240
#define SIDEBAR_MAX_WIDTH 360
#define SIDEBAR_WIDTH_FRACTION 0.25
#define WINDOW_NARROW_BREAKPOINT "max-width: 900px"

struct _AppController {
  GObject parent_instance;
  GtkWindow *window;
  HttpService *http_service;
  HistoryService *history_service;
  RequestController *request_controller;
  HistoryController *history_controller;
  RequestView *request_view;
};

G_DEFINE_FINAL_TYPE(AppController, app_controller, G_TYPE_OBJECT)

static void setup_appearance(void) {
  g_object_set(gtk_settings_get_default(),
               "gtk-theme-name", "Adwaita",
               "gtk-icon-theme-name", "Adwaita", NULL);

  AdwStyleManager *style_manager = adw_style_manager_get_default();
  adw_style_manager_set_color_scheme(style_manager,
                                     ADW_COLOR_SCHEME_FORCE_DARK);

  css_loader_init();
}

static AdwOverlaySplitView *
build_workspace_split(RequestView *request_view, ResponseView *response_view,
                      HistoryView *history_view) {
  GtkWidget *inner_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_paned_set_start_child(GTK_PANED(inner_paned), GTK_WIDGET(request_view));
  gtk_paned_set_end_child(GTK_PANED(inner_paned), GTK_WIDGET(response_view));
  gtk_paned_set_position(GTK_PANED(inner_paned), INITIAL_REQUEST_PANE_WIDTH);
  gtk_paned_set_shrink_start_child(GTK_PANED(inner_paned), FALSE);
  gtk_paned_set_shrink_end_child(GTK_PANED(inner_paned), FALSE);

  AdwOverlaySplitView *split =
      ADW_OVERLAY_SPLIT_VIEW(adw_overlay_split_view_new());
  adw_overlay_split_view_set_sidebar(split, GTK_WIDGET(history_view));
  adw_overlay_split_view_set_content(split, inner_paned);
  adw_overlay_split_view_set_min_sidebar_width(split, SIDEBAR_MIN_WIDTH);
  adw_overlay_split_view_set_max_sidebar_width(split, SIDEBAR_MAX_WIDTH);
  adw_overlay_split_view_set_sidebar_width_fraction(split,
                                                    SIDEBAR_WIDTH_FRACTION);
  adw_overlay_split_view_set_show_sidebar(split, TRUE);

  return split;
}

static GtkWidget *build_window_chrome(AdwOverlaySplitView *split) {
  GtkWidget *toolbar_view = adw_toolbar_view_new();
  GtkWidget *header_bar = adw_header_bar_new();

  GtkWidget *sidebar_toggle = gtk_toggle_button_new();
  gtk_button_set_icon_name(GTK_BUTTON(sidebar_toggle), "sidebar-show-symbolic");
  gtk_widget_set_tooltip_text(sidebar_toggle, "Toggle Sidebar");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(sidebar_toggle), TRUE);
  adw_header_bar_pack_start(ADW_HEADER_BAR(header_bar), sidebar_toggle);

  g_object_bind_property(sidebar_toggle, "active", split, "show-sidebar",
                         G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  adw_toolbar_view_add_top_bar(ADW_TOOLBAR_VIEW(toolbar_view), header_bar);
  adw_toolbar_view_set_content(ADW_TOOLBAR_VIEW(toolbar_view),
                               GTK_WIDGET(split));

  return toolbar_view;
}

static void install_breakpoints(AdwApplicationWindow *window,
                                AdwOverlaySplitView *split) {
  AdwBreakpointCondition *condition =
      adw_breakpoint_condition_parse(WINDOW_NARROW_BREAKPOINT);
  AdwBreakpoint *breakpoint = adw_breakpoint_new(condition);

  GValue collapsed = G_VALUE_INIT;
  g_value_init(&collapsed, G_TYPE_BOOLEAN);
  g_value_set_boolean(&collapsed, TRUE);
  adw_breakpoint_add_setter(breakpoint, G_OBJECT(split), "collapsed", &collapsed);
  g_value_unset(&collapsed);

  adw_application_window_add_breakpoint(window, breakpoint);
}

static void on_shortcut_send(GSimpleAction *action, GVariant *parameter,
                             gpointer user_data) {
  (void)action;
  (void)parameter;
  AppController *self = APP_CONTROLLER(user_data);
  if (self->request_controller != NULL) {
    request_controller_send(self->request_controller);
  }
}

static void on_shortcut_focus_url(GSimpleAction *action, GVariant *parameter,
                                  gpointer user_data) {
  (void)action;
  (void)parameter;
  AppController *self = APP_CONTROLLER(user_data);
  if (self->request_view != NULL) {
    request_top_bar_focus_url(request_view_get_top_bar(self->request_view));
  }
}

static void install_shortcuts(AppController *self, GtkApplication *app,
                              GtkApplicationWindow *window) {
  static const struct {
    const char *name;
    const char *accel;
    GCallback callback;
  } shortcuts[] = {
      {"send_request", "<Control>Return", G_CALLBACK(on_shortcut_send)},
      {"focus_url", "<Control>l", G_CALLBACK(on_shortcut_focus_url)},
  };

  for (gsize i = 0; i < G_N_ELEMENTS(shortcuts); i++) {
    GSimpleAction *action = g_simple_action_new(shortcuts[i].name, NULL);
    g_signal_connect_object(action, "activate", shortcuts[i].callback, self, 0);
    g_action_map_add_action(G_ACTION_MAP(window), G_ACTION(action));
    g_object_unref(action);

    gchar *detailed_name = g_strdup_printf("win.%s", shortcuts[i].name);
    const char *accels[] = {shortcuts[i].accel, NULL};
    gtk_application_set_accels_for_action(app, detailed_name, accels);
    g_free(detailed_name);
  }
}

AppController *app_controller_new(AdwApplication *application) {
  g_return_val_if_fail(ADW_IS_APPLICATION(application), NULL);

  AppController *self = g_object_new(APP_TYPE_CONTROLLER, NULL);

  setup_appearance();

  self->http_service = http_service_new();
  self->history_service = history_service_new(0);

  GtkWidget *window =
      adw_application_window_new(GTK_APPLICATION(application));
  gtk_window_set_title(GTK_WINDOW(window), WINDOW_TITLE);
  gtk_window_set_default_size(GTK_WINDOW(window), WINDOW_DEFAULT_WIDTH,
                              WINDOW_DEFAULT_HEIGHT);
  self->window = GTK_WINDOW(window);

  RequestView *request_view = request_view_new();
  ResponseView *response_view = response_view_new();
  HistoryView *history_view = history_view_new(self->history_service);

  self->request_view = request_view;

  AdwOverlaySplitView *split =
      build_workspace_split(request_view, response_view, history_view);
  GtkWidget *chrome = build_window_chrome(split);

  self->request_controller = request_controller_new(
      request_view, response_view, self->http_service, self->history_service);
  self->history_controller =
      history_controller_new(history_view, request_view, response_view);

  install_shortcuts(self, GTK_APPLICATION(application),
                    GTK_APPLICATION_WINDOW(window));
  install_breakpoints(ADW_APPLICATION_WINDOW(window), split);

  adw_application_window_set_content(ADW_APPLICATION_WINDOW(window), chrome);

  g_object_set_data_full(G_OBJECT(window), APP_CTRL_DATA_KEY, self,
                         g_object_unref);

  return self;
}

void app_controller_present(AppController *self) {
  g_return_if_fail(APP_IS_CONTROLLER(self));
  if (self->window != NULL) {
    gtk_window_present(self->window);
  }
}


static void app_controller_dispose(GObject *obj) {
  AppController *self = APP_CONTROLLER(obj);
  g_clear_object(&self->request_controller);
  g_clear_object(&self->history_controller);
  g_clear_object(&self->http_service);
  g_clear_object(&self->history_service);
  G_OBJECT_CLASS(app_controller_parent_class)->dispose(obj);
}

static void app_controller_class_init(AppControllerClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = app_controller_dispose;
}

static void app_controller_init(AppController *self) { (void)self; }
