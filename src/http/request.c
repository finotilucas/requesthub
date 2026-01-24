#include "request.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

HttpRequest *http_request_new(const char *url, HttpMethods method) {
  if (url == NULL || strlen(url) == 0) {
    return NULL;
  }

  HttpRequest *request = calloc(1, sizeof(HttpRequest));
  if (request == NULL) {
    return NULL;
  }

  request->url = strdup(url);
  if (request->url == NULL) {
    free(request);
    return NULL;
  }

  request->method = method;

  request->timeout = 30;
  request->connect_timeout = 10;
  request->follow_redirects = 1;
  request->max_redirects = 5;
  request->verify_ssl = 1;

  return request;
}

void http_request_free(HttpRequest *request) {
  if (request == NULL) {
    return;
  }

  free(request->url);
  free(request->body);
  free(request->auth_header);

  if (request->headers != NULL) {
    for (int i = 0; i < request->headers_count; i++) {
      free(request->headers[i]);
    }
    free(request->headers);
  }

  if (request->query_params != NULL) {
    for (int i = 0; i < request->query_count; i++) {
      free(request->query_params[i]);
    }
    free(request->query_params);
  }

  free(request);
}

HttpRequest *http_request_add_header(HttpRequest *request, const char *header) {
  if (request == NULL || header == NULL) {
    return request;
  }

  char **new_headers =
      realloc(request->headers, sizeof(char *) * (request->headers_count + 1));
  if (new_headers == NULL) {
    return request;
  }

  request->headers = new_headers;
  request->headers[request->headers_count] = strdup(header);

  if (request->headers[request->headers_count] == NULL) {
    return request;
  }

  request->headers_count++;

  return request;
}

HttpRequest *http_request_set_body(HttpRequest *request, const char *body) {
  if (request == NULL) {
    return request;
  }

  if (request->body != NULL) {
    free(request->body);
    request->body = NULL;
  }

  if (body != NULL) {
    request->body = strdup(body);
  }

  return request;
}

HttpRequest *http_request_add_query_param(HttpRequest *request, const char *key,
                                          const char *value) {
  if (request == NULL || key == NULL || value == NULL) {
    return request;
  }

  char *encoded_key = curl_easy_escape(NULL, key, 0);
  char *encoded_value = curl_easy_escape(NULL, value, 0);

  if (encoded_key == NULL || encoded_value == NULL) {
    curl_free(encoded_key);
    curl_free(encoded_value);
    return request;
  }

  size_t param_len = strlen(encoded_key) + strlen(encoded_value) + 2;
  char *param = malloc(param_len);
  if (param == NULL) {
    curl_free(encoded_key);
    curl_free(encoded_value);
    return request;
  }

  snprintf(param, param_len, "%s=%s", encoded_key, encoded_value);

  curl_free(encoded_key);
  curl_free(encoded_value);

  char **new_params = realloc(request->query_params,
                              sizeof(char *) * (request->query_count + 1));
  if (new_params == NULL) {
    free(param);
    return request;
  }

  request->query_params = new_params;
  request->query_params[request->query_count] = param;
  request->query_count++;

  return request;
}

HttpRequest *http_request_set_timeout(HttpRequest *request, long seconds) {
  if (request == NULL || seconds < 0) {
    return request;
  }

  request->timeout = seconds;
  return request;
}

HttpRequest *http_request_set_bearer_token(HttpRequest *request,
                                           const char *token) {
  if (request == NULL || token == NULL) {
    return request;
  }

  if (request->auth_header != NULL) {
    free(request->auth_header);
  }

  size_t header_len = strlen("Authorization: Bearer ") + strlen(token) + 1;
  request->auth_header = malloc(header_len);

  if (request->auth_header != NULL) {
    snprintf(request->auth_header, header_len, "Authorization: Bearer %s",
             token);
  }

  return request;
}

HttpRequest *http_request_follow_redirects(HttpRequest *request, int follow,
                                           int max) {
  if (request == NULL) {
    return request;
  }

  request->follow_redirects = follow ? 1 : 0;
  request->max_redirects = (max > 0) ? max : 5;

  return request;
}
