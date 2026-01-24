#include "request.h"
#include "response.h"

#ifndef HTTP_H
#define HTTP_H

// HttpResponse *http_request_perform(HttpRequest *req);
HttpResponse *get(const char *url);
// HttpResponse *post(const char *url, const char *body);
// HttpResponse *put(const char *url, const char *body);
// HttpResponse *delete (const char *url);

#endif
