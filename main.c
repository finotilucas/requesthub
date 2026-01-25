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

#include "src/http/methods.h"
#include <gtk/gtk.h>

typedef struct {
  GtkDropDown *method_dropdown;
  GtkEntry *url_entry;
} AppWidgets;

static void on_send_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;

  AppWidgets *widgets = (AppWidgets *)user_data;

  if (!widgets || !GTK_IS_DROP_DOWN(widgets->method_dropdown)) {
    g_printerr("Erro: Inválid Reference.\n");
    return;
  }

  guint index = gtk_drop_down_get_selected(widgets->method_dropdown);
  HttpMethods method = (HttpMethods)index;

  const char *method_str = method_to_string(method);
  const char *url = gtk_editable_get_text(GTK_EDITABLE(widgets->url_entry));

  g_print("Request send: %s %s\n", method_str, url);
}

static void on_window_destroy(gpointer data, GObject *where_the_object_was) {
  (void)where_the_object_was;

  g_free(data);
}

static GtkWidget *create_top_bar(AppWidgets *widgets) {
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

  gtk_widget_set_margin_start(box, 10);
  gtk_widget_set_margin_end(box, 10);
  gtk_widget_set_margin_top(box, 10);
  gtk_widget_set_margin_bottom(box, 10);

  const char **methods = http_methods_get_list();
  GtkStringList *list = gtk_string_list_new(methods);

  GtkWidget *dropdown = gtk_drop_down_new(G_LIST_MODEL(list), NULL);

  g_object_unref(list);

  gtk_widget_set_size_request(dropdown, 120, -1);
  widgets->method_dropdown = GTK_DROP_DOWN(dropdown);

  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "https://api.example.com");

  gtk_widget_set_hexpand(entry, TRUE);
  widgets->url_entry = GTK_ENTRY(entry);

  GtkWidget *button = gtk_button_new_with_label("Enviar");

  gtk_widget_add_css_class(button, "suggested-action");

  gtk_widget_set_size_request(button, 100, -1);

  g_signal_connect(button, "clicked", G_CALLBACK(on_send_clicked), widgets);

  gtk_box_append(GTK_BOX(box), dropdown);
  gtk_box_append(GTK_BOX(box), entry);
  gtk_box_append(GTK_BOX(box), button);

  return box;
}

static void on_activate(GtkApplication *app) {
  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "RequestHub");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  AppWidgets *widgets = g_new0(AppWidgets, 1);

  g_object_weak_ref(G_OBJECT(window), on_window_destroy, widgets);
  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_append(GTK_BOX(main_box), create_top_bar(widgets));

  gtk_window_set_child(GTK_WINDOW(window), main_box);
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  GtkApplication *app;
  int status;

  app = gtk_application_new("com.requesthub.app", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

  status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  return status;
}
