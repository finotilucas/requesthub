#ifndef RESPONSE_H
#define RESPONSE_H

#include <curl/curl.h>
#include <stddef.h>

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

HttpResponse *http_response_create(void);
void http_response_free(HttpResponse *response);
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);

#endif
