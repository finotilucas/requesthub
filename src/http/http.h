#ifndef HTTP_H
#define HTTP_H

#include "request.h"
#include "response.h"

HttpResponse *http_request_perform(HttpRequest *request);

#endif
