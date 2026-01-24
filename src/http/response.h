#include <curl/curl.h>
#include <stdio.h>

#ifndef RESPONSE_H
#define RESPONSE_H

typedef struct {
  CURLcode curl_code;
  long http_status;

  char *body;
  size_t body_size;

  char *content_type;
  double total_time;
  double download_size;

  char *header_location;
  char *etag;
  long content_length;

  struct curl_slist *all_headers;
} HttpResponse;

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
void http_response_free(HttpResponse *response);

#endif
