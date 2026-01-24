#include "request.h"
#include "response.h"

#ifndef HTTP_H
#define HTTP_H

// HttpResponse *http_request_perform(HttpRequest *req);
HttpResponse *http_get(HttpRequest *request);
// HttpResponse *http_post(const char *url, const char *body);
// HttpResponse *http_put(const char *url, const char *body);
// HttpResponse *http_delete (const char *url);

#endif
