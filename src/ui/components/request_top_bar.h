#pragma once

#include "../../http/methods.h"
#include <gtk/gtk.h>

#define REQUEST_TYPE_TOP_BAR (request_top_bar_get_type())
G_DECLARE_FINAL_TYPE(RequestTopBar, request_top_bar, REQUEST, TOP_BAR, GtkBox)

RequestTopBar *request_top_bar_new(void);

const char *request_top_bar_get_url(RequestTopBar *self);
HttpMethods request_top_bar_get_method(RequestTopBar *self);
