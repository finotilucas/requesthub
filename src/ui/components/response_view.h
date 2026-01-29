#pragma once

#include "../../http/response.h"
#include <gtk/gtk.h>

#define RESPONSE_TYPE_VIEW (response_view_get_type())

G_DECLARE_FINAL_TYPE(ResponseView, response_view, RESPONSE, VIEW, GtkBox)

ResponseView *response_view_new(void);

void response_view_clear(ResponseView *self);

void response_view_set_response(ResponseView *self, HttpResponse *resp);
