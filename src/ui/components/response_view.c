#include "response_view.h"
#include <string.h>

struct _ResponseView {
  GtkBox parent_instance;

  GtkLabel *status_label;
  GtkLabel *time_label;
  GtkLabel *size_label;

  GtkTextView *body_view;
};

G_DEFINE_TYPE(ResponseView, response_view, GTK_TYPE_BOX)

static void response_view_init(ResponseView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_box_set_spacing(GTK_BOX(self), 8);



  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);

  self->status_label = GTK_LABEL(gtk_label_new("Status: "));
  self->time_label = GTK_LABEL(gtk_label_new("Time: "));
  self->size_label = GTK_LABEL(gtk_label_new("Size: "));

  gtk_widget_set_halign(GTK_WIDGET(self->status_label), GTK_ALIGN_START);
  gtk_widget_set_halign(GTK_WIDGET(self->time_label), GTK_ALIGN_START);
  gtk_widget_set_halign(GTK_WIDGET(self->size_label), GTK_ALIGN_START);

  gtk_widget_set_margin_start(GTK_WIDGET(self->status_label), 10);


  gtk_box_append(GTK_BOX(header), GTK_WIDGET(self->status_label));
  gtk_box_append(GTK_BOX(header), GTK_WIDGET(self->time_label));
  gtk_box_append(GTK_BOX(header), GTK_WIDGET(self->size_label));

  gtk_box_append(GTK_BOX(self), header);

  self->body_view = GTK_TEXT_VIEW(gtk_text_view_new());
  gtk_text_view_set_editable(self->body_view, FALSE);
  gtk_text_view_set_monospace(self->body_view, TRUE);
  gtk_text_view_set_wrap_mode(self->body_view, GTK_WRAP_WORD_CHAR);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(self->body_view));

  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_widget_set_hexpand(scrolled, TRUE);

  gtk_box_append(GTK_BOX(self), scrolled);
}

static void response_view_class_init(ResponseViewClass *klass) { (void)klass; }

ResponseView *response_view_new(void) {
  return g_object_new(RESPONSE_TYPE_VIEW, NULL);
}

void response_view_clear(ResponseView *self) {
  gtk_label_set_text(self->status_label, "Status: -");
  gtk_label_set_text(self->time_label, "Time: -");
  gtk_label_set_text(self->size_label, "Size: -");

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(self->body_view);
  gtk_text_buffer_set_text(buffer, "", -1);
}

void response_view_set_response(ResponseView *self, HttpResponse *resp) {
  if (!resp)
    return;

  char buf[128];

  g_snprintf(buf, sizeof(buf), "Status: %ld", resp->http_status);
  gtk_label_set_text(self->status_label, buf);

  g_snprintf(buf, sizeof(buf), "Time: %.f ms", resp->total_time * 1000.0);
  gtk_label_set_text(self->time_label, buf);

  g_snprintf(buf, sizeof(buf), "Size: %zu bytes",
             resp->body ? strlen(resp->body) : 0);
  gtk_label_set_text(self->size_label, buf);

  GtkTextBuffer *buffer = gtk_text_view_get_buffer(self->body_view);
  if (resp->body && g_utf8_validate(resp->body, -1, NULL)) {
    gtk_text_buffer_set_text(buffer, resp->body, -1);
  } else {
    gtk_text_buffer_set_text(
        buffer, "[inválid]", -1);
  }
}
