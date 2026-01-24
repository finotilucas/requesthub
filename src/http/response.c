#include "response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  HttpResponse *response = (HttpResponse *)userp;

  char *ptr = realloc(response->body, response->body_size + realsize + 1);
  if (ptr == NULL) {
    fprintf(stderr, "Erro: sem memória\n");
    return 0;
  }

  response->body = ptr;

  memcpy(&(response->body[response->body_size]), contents, realsize);
  response->body_size += realsize;
  response->body[response->body_size] = '\0';

  return realsize;
}

void http_response_free(HttpResponse *response) {
  if (response == NULL) {
    return;
  }

  if (response->body != NULL) {
    free(response->body);
  }

  if (response->content_type != NULL) {
    free(response->content_type);
  }

  free(response);
}
