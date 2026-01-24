#include "request.h"
#include "response.h"

#include <curl/curl.h>
#include <stdlib.h>

HttpResponse *get(HttpRequest *request) {
  CURL *curl;
  CURLcode result;

  result = curl_global_init(CURL_GLOBAL_ALL);
  if (result) {
    return NULL;
  }

  curl = curl_easy_init();
  if (!curl)
    return NULL;

  HttpResponse *response = malloc(sizeof(*response));
  response->body[0] = '\0';

  curl_easy_setopt(curl, CURLOPT_URL, request->url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  result = curl_easy_perform(curl);

  if (result != CURLE_OK) {
    fprintf(stderr, "Erro: %s\n", curl_easy_strerror(result));
  } else {
    printf("Resposta da API:\n%s\n", response->body);
  }

  curl_easy_cleanup(curl);

  return response;
}
