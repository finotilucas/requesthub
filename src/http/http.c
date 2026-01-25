#include "http.h"
#include "request.h"
#include "response.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

static void configure_method(CURL *curl, HttpRequest *request) {
  switch (request->method) {
  case GET:
    break;
  case POST:
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    if (request->body)
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
    break;
  case PUT:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    if (request->body)
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
    break;
  case DELETE:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    break;
  case PATCH:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    if (request->body)
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
    break;
  case HEAD:
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "HEAD");
    break;
  case OPTIONS:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
    break;
  }
}

static struct curl_slist *build_headers_list(HttpRequest *request) {
  struct curl_slist *list = NULL;
  for (int i = 0; i < request->headers_count; i++) {
    struct curl_slist *tmp = curl_slist_append(list, request->headers[i]);
    if (!tmp) {
      curl_slist_free_all(list);
      return NULL;
    }
    list = tmp;
  }
  if (request->auth_header) {
    struct curl_slist *tmp = curl_slist_append(list, request->auth_header);
    if (!tmp) {
      curl_slist_free_all(list);
      return NULL;
    }
    list = tmp;
  }
  return list;
}

static void configure_curl_options(CURL *curl, HttpRequest *request,
                                   HttpResponse *response,
                                   struct curl_slist *headers, char *url) {
  curl_easy_setopt(curl, CURLOPT_URL, url);
  configure_method(curl, request);
  if (headers) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }
  if (request->timeout > 0) {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, request->timeout);
  }
  if (request->connect_timeout > 0) {
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, request->connect_timeout);
  }
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, request->follow_redirects);
  if (request->max_redirects > 0) {
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, request->max_redirects);
  }
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, request->verify_ssl);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, request->verify_ssl ? 2L : 0L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
}

static void extract_response_info(CURL *curl, HttpResponse *response) {
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status);
  char *content_type = NULL;
  curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
  if (content_type)
    response->content_type = strdup(content_type);
  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &response->total_time);
}

static char *build_query_string(HttpRequest *request) {
  if (!request->query_params || request->query_count == 0)
    return NULL;

  size_t total_len = 2;
  for (int i = 0; i < request->query_count; i++)
    total_len += strlen(request->query_params[i]) + 1;

  char *query = malloc(total_len);
  if (!query)
    return NULL;

  query[0] = '?';
  query[1] = '\0';

  for (int i = 0; i < request->query_count; i++) {
    if (i > 0)
      strcat(query, "&");
    strcat(query, request->query_params[i]);
  }

  return query;
}

HttpResponse *http_request_perform(HttpRequest *request) {
  if (!request || !request->url)
    return NULL;

  char *query_string = build_query_string(request);
  size_t base_len = strlen(request->url);
  size_t query_len = query_string ? strlen(query_string) : 0;

  char *final_url = malloc(base_len + query_len + 1);
  if (!final_url) {
    free(query_string);
    return NULL;
  }

  memcpy(final_url, request->url, base_len);
  if (query_string)
    memcpy(final_url + base_len, query_string, query_len);
  final_url[base_len + query_len] = '\0';
  free(query_string);

  HttpResponse *response = http_response_create();
  if (!response) {
    free(final_url);
    return NULL;
  }

  CURL *curl = curl_easy_init();
  if (!curl) {
    free(final_url);
    http_response_free(response);
    return NULL;
  }

  struct curl_slist *headers = build_headers_list(request);
  configure_curl_options(curl, request, response, headers, final_url);

  CURLcode result = curl_easy_perform(curl);
  response->curl_code = result;

  extract_response_info(curl, response);

  if (headers)
    curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  free(final_url);

  return response;
}
