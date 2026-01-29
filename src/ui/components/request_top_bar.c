#include "request_top_bar.h"
#include "../../http/methods.h"

struct _RequestTopBar {
  GtkBox parent_instance;
  GtkDropDown *method_dropdown;
  GtkEntry *url_entry;
  GtkButton *send_button;
};

G_DEFINE_TYPE(RequestTopBar, request_top_bar, GTK_TYPE_BOX)

enum { SEND_CLICKED, N_SIGNALS };

static guint signals[N_SIGNALS];

static void on_send_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;
  RequestTopBar *self = REQUEST_TOP_BAR(user_data);
  g_signal_emit(self, signals[SEND_CLICKED], 0);
}

static void request_top_bar_init(RequestTopBar *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_HORIZONTAL);

  gtk_widget_set_margin_start(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_end(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_top(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_bottom(GTK_WIDGET(self), 10);
  gtk_box_set_spacing(GTK_BOX(self), 8);

  const char **methods = http_methods_get_list();
  GtkStringList *list = gtk_string_list_new(methods);

  self->method_dropdown =
      GTK_DROP_DOWN(gtk_drop_down_new(G_LIST_MODEL(list), NULL));
  g_object_unref(list);

  self->url_entry = GTK_ENTRY(gtk_entry_new());
  gtk_entry_set_placeholder_text(self->url_entry, "https://api.example.com");
  gtk_widget_set_hexpand(GTK_WIDGET(self->url_entry), TRUE);

  self->send_button = GTK_BUTTON(gtk_button_new_with_label("Enviar"));
  gtk_widget_add_css_class(GTK_WIDGET(self->send_button), "suggested-action");

  g_signal_connect(self->send_button, "clicked", G_CALLBACK(on_send_clicked),
                   self);

  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->method_dropdown));
  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->url_entry));
  gtk_box_append(GTK_BOX(self), GTK_WIDGET(self->send_button));
}

static void request_top_bar_class_init(RequestTopBarClass *klass) {
  signals[SEND_CLICKED] =
      g_signal_new("send-clicked", G_TYPE_FROM_CLASS(klass), G_SIGNAL_RUN_FIRST,
                   0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

RequestTopBar *request_top_bar_new(void) {
  return g_object_new(REQUEST_TYPE_TOP_BAR, NULL);
}

const char *request_top_bar_get_url(RequestTopBar *self) {
  return gtk_editable_get_text(GTK_EDITABLE(self->url_entry));
}

HttpMethods request_top_bar_get_method(RequestTopBar *self) {
  return (HttpMethods)gtk_drop_down_get_selected(self->method_dropdown);
}
