#include "http.h"
#include "request.h"
#include "response.h"

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void configure_method(CURL *curl, HttpRequest *request) {
  switch (request->method) {
  case GET:
    break;

  case POST:
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    if (request->body) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
    }
    break;

  case PUT:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    if (request->body) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
    }
    break;

  case DELETE:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    break;

  case PATCH:
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    if (request->body) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request->body);
    }
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

  if (request->headers != NULL) {
    for (int i = 0; i < request->headers_count; i++) {
      list = curl_slist_append(list, request->headers[i]);
    }
  }

  if (request->auth_header != NULL) {
    list = curl_slist_append(list, request->auth_header);
  }

  return list;
}

static void configure_curl_options(CURL *curl, HttpRequest *request,
                                   HttpResponse *response,
                                   struct curl_slist *headers) {
  curl_easy_setopt(curl, CURLOPT_URL, request->url);

  configure_method(curl, request);

  if (headers != NULL) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  if (request->timeout > 0) {
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, request->timeout);
  }

  if (request->connect_timeout > 0) {
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, request->connect_timeout);
  }

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,
                   (long)request->follow_redirects);
  if (request->max_redirects > 0) {
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, (long)request->max_redirects);
  }

  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)request->verify_ssl);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, request->verify_ssl ? 2L : 0L);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
}

static void extract_response_info(CURL *curl, HttpResponse *response) {
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status);

  char *content_type = NULL;
  curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
  if (content_type != NULL) {
    response->content_type = strdup(content_type);
  }

  curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &response->total_time);
}

static char *build_query_string(HttpRequest *request) {
  if (request->query_params == NULL || request->query_count == 0) {
    return NULL;
  }

  size_t total_len = 2;
  for (int i = 0; i < request->query_count; i++) {
    total_len += strlen(request->query_params[i]) + 1;
  }

  char *query = malloc(total_len);
  if (query == NULL) {
    return NULL;
  }

  strcpy(query, "?");

  for (int i = 0; i < request->query_count; i++) {
    if (i > 0) {
      strcat(query, "&");
    }
    strcat(query, request->query_params[i]);
  }

  return query;
}

HttpResponse *http_request_perform(HttpRequest *request) {
  if (request == NULL || request->url == NULL) {
    return NULL;
  }

  char *final_url = request->url;
  char *query_string = build_query_string(request);

  if (query_string != NULL) {
    size_t url_len = strlen(request->url) + strlen(query_string) + 1;
    final_url = malloc(url_len);
    if (final_url != NULL) {
      snprintf(final_url, url_len, "%s%s", request->url, query_string);
    }
    free(query_string);
  }

  HttpResponse *response = http_response_create();
  if (response == NULL) {
    return NULL;
  }

  CURL *curl = curl_easy_init();
  if (curl == NULL) {
    http_response_free(response);
    return NULL;
  }

  curl_easy_setopt(curl, CURLOPT_URL, final_url);

  if (final_url != request->url) {
    free(final_url);
  }

  struct curl_slist *headers = build_headers_list(request);

  configure_curl_options(curl, request, response, headers);

  CURLcode result = curl_easy_perform(curl);
  response->curl_code = result;

  if (result != CURLE_OK) {
    fprintf(stderr, "curl error: %s\n", curl_easy_strerror(result));
  }

  extract_response_info(curl, response);

  if (headers != NULL) {
    curl_slist_free_all(headers);
  }
  curl_easy_cleanup(curl);

  return response;
}
