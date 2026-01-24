#include "request.h"
#include "http.h"
#include "methods.h"
#include "response.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

HttpRequest *http_request_new(const char *url, HttpMethods method) {
  HttpRequest *request = NULL;

  request = malloc(sizeof(*request));
  if (request == NULL) {
    return NULL;
  }

  request->url = malloc(strlen(url) + 1);
  if (request->url == NULL) {
    http_request_free(request);
  }
  strcpy(request->url, url);

  request->method = method;

  return request;
}

void http_request_free(HttpRequest *request) {
  if (request != NULL) {
    free(request->url);
    free(request);
  }
}

void http_request_perform(HttpRequest *request) {
    HttpResponse *response = http_get(request);

    if (response == NULL) {
        fprintf(stderr, "Falha na requisição\n");
        return;
    }

    printf("Status: %ld\n", response->http_status);
    if (response->body) {
        printf("Body: %s\n", response->body);
    }

    http_response_free(response);
}
