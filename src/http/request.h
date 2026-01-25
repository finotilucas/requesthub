#ifndef REQUEST_H
#define REQUEST_H

#include "methods.h"

typedef struct {
  char *url;
  HttpMethods method;
  char *body;
  char **headers;
  int headers_count;
  char **query_params;
  int query_count;
  long timeout;
  long connect_timeout;
  long follow_redirects;
  long max_redirects;
  char *auth_header;
  long verify_ssl;
} HttpRequest;

HttpRequest *http_request_new(const char *url, HttpMethods method);

void http_request_free(HttpRequest *request);

HttpRequest *http_request_add_header(HttpRequest *request, const char *header);

HttpRequest *http_request_set_body(HttpRequest *request, const char *body);

HttpRequest *http_request_add_query_param(HttpRequest *request, const char *key,
                                          const char *value);

HttpRequest *http_request_set_timeout(HttpRequest *request, long seconds);

HttpRequest *http_request_set_bearer_token(HttpRequest *request,
                                           const char *token);

HttpRequest *http_request_follow_redirects(HttpRequest *request, int follow,
                                           int max);

#endif
