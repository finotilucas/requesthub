#include "request.h"
#include "response.h"

#include <curl/curl.h>
#include <stdlib.h>

HttpResponse *http_get(HttpRequest *request) {
  CURL *curl;
  CURLcode result;

  HttpResponse *response = calloc(1, sizeof(HttpResponse));
  if (response == NULL) {
    return NULL;
  }

  response->body = malloc(1);
  if (response->body == NULL) {
    free(response);
    return NULL;
  }

  response->body[0] = '\0';
  response->body_size = 0;
  response->curl_code = CURLE_OK;
  response->http_status = 0;

  curl = curl_easy_init();
  if (!curl) {
    free(response->body);
    free(response);
    return NULL;
  }

  curl_easy_setopt(curl, CURLOPT_URL, request->url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);

  result = curl_easy_perform(curl);
  response->curl_code = result;

  if (result != CURLE_OK) {
    fprintf(stderr, "Erro: %s\n", curl_easy_strerror(result));
  }

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status);

  curl_easy_cleanup(curl);

  return response;
}
