#include "src/http/http.h"
#include "src/http/http_pool.h"
#include "src/http/request.h"
#include "src/ui/components/request_top_bar.h"
#include "src/ui/components/response_content.h"
#include "src/ui/components/response_top_bar.h"
#include "src/utils/css_loader.h"

#include <curl/curl.h>
#include <gtk/gtk.h>

typedef struct {
  ResponseTopBar *top_bar;
  ResponseContent *view;
} ResponseContext;

static void on_send_clicked(RequestTopBar *bar, gpointer user_data) {
  ResponseContext *ctx = (ResponseContext *)user_data;

  const char *url = request_top_bar_get_url(bar);
  HttpMethods method = request_top_bar_get_method(bar);

  HttpRequest *req = http_request_new(url, method);
  HttpResponse *resp = http_request_perform(req);

  response_top_bar_update(ctx->top_bar, resp);
  response_content_set_response(ctx->view, resp);

  http_response_free(resp);
  http_request_free(req);
}

static void on_activate(GtkApplication *app) {
  load_css();
  watch_css_file("src/ui/styles/app.css");

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "RequestHub");
  gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);

  GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

  RequestTopBar *request_bar = request_top_bar_new();
  GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_box_append(GTK_BOX(left_box), GTK_WIDGET(request_bar));
  gtk_widget_set_size_request(left_box, 300, -1);

  ResponseTopBar *res_top_bar = response_top_bar_new();
  ResponseContent *res_view = response_content_new();

  GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append(GTK_BOX(right_box), GTK_WIDGET(res_top_bar));
  gtk_box_append(GTK_BOX(right_box), GTK_WIDGET(res_view));
  gtk_widget_set_size_request(right_box, 300, -1);

  gtk_paned_set_start_child(GTK_PANED(paned), left_box);
  gtk_paned_set_end_child(GTK_PANED(paned), right_box);
  gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
  gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);

  gtk_paned_set_position(GTK_PANED(paned), 400);

  ResponseContext *ctx = g_new0(ResponseContext, 1);
  ctx->top_bar = res_top_bar;
  ctx->view = res_view;

  g_signal_connect(request_bar, "send-clicked", G_CALLBACK(on_send_clicked),
                   ctx);

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
