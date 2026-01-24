#include "request.h"
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
  CURL *curl;
  CURLcode result;

  result = curl_global_init(CURL_GLOBAL_ALL);
  if (result) {
    return;
  }

  curl = curl_easy_init();
  if (!curl)
    return;

  HttpResponse response = {.body = malloc(1), .body_size = 0};
  response.body[0] = '\0';

  curl_easy_setopt(curl, CURLOPT_URL, request->url);

  if (request->method == POST) {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
  }

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  result = curl_easy_perform(curl);

  if (result != CURLE_OK) {
    fprintf(stderr, "Erro: %s\n", curl_easy_strerror(result));
  } else {
    printf("Resposta da API:\n%s\n", response.body);
  }

  free(response.body);
  curl_easy_cleanup(curl);
}
