#include "src/http/http.h"
#include "src/http/http_pool.h"
#include "src/http/request.h"
#include "src/ui/components/request_top_bar.h"
#include "src/ui/components/response_view.h"
#include "src/utils/css_loader.h"

#include <curl/curl.h>
#include <gtk/gtk.h>

static void on_send_clicked(RequestTopBar *bar, gpointer user_data) {
  ResponseView *view = user_data;

  const char *url = request_top_bar_get_url(bar);
  HttpMethods method = request_top_bar_get_method(bar);

  HttpRequest *req = http_request_new(url, method);

  HttpResponse *resp = http_request_perform(req);

  response_view_set_response(view, resp);

  http_response_free(resp);
  http_request_free(req);
}

static void on_activate(GtkApplication *app) {
  load_css();
  watch_css_file("src/ui/styles/app.css");

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "RequestHub");
  gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

  RequestTopBar *top_bar = request_top_bar_new();
  ResponseView *response_view = response_view_new();

  GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

  gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(top_bar));
  gtk_box_append(GTK_BOX(main_box), GTK_WIDGET(response_view));

  g_signal_connect(top_bar, "send-clicked", G_CALLBACK(on_send_clicked),
                   response_view);

  gtk_window_set_child(GTK_WINDOW(window), main_box);
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
