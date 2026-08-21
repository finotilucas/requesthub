/*******************************************************************************
 * Copyright (C) 2026 Lucas Finoti <lucas.finoti@protonmail.com>
 *
 * This file is part of RequestHub.
 *
 * RequestHub is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * any later version.
 *
 * RequestHub is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RequestHub. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 ******************************************************************************/

#include "../src/http/request.h"

#include <curl/curl.h>
#include <glib.h>
#include <string.h>

static void test_new_rejects_null_or_empty_url(void) {
  g_assert_null(http_request_new(NULL, HTTP_GET));
  g_assert_null(http_request_new("", HTTP_GET));
}

static void test_new_initializes_defaults(void) {
  HttpRequest *req = http_request_new("https://example.com/api", HTTP_POST);
  g_assert_nonnull(req);
  g_assert_cmpstr(req->url, ==, "https://example.com/api");
  g_assert_cmpint(req->method, ==, HTTP_POST);
  g_assert_cmpint(req->timeout, ==, 30);
  g_assert_cmpint(req->connect_timeout, ==, 10);
  g_assert_cmpint(req->follow_redirects, ==, 1);
  g_assert_cmpint(req->max_redirects, ==, 5);
  g_assert_cmpint(req->verify_ssl, ==, 1);
  g_assert_cmpuint(http_request_headers_count(req), ==, 0);
  g_assert_cmpuint(req->query_params->len, ==, 0);
  g_assert_null(req->body);
  http_request_free(req);
}

static void test_free_null_safe(void) { http_request_free(NULL); }

static void test_add_header_appends_in_order(void) {
  HttpRequest *req = http_request_new("https://example.com", HTTP_GET);
  http_request_add_header(req, "X-One", "1");
  http_request_add_header(req, "X-Two", "2");
  http_request_add_header(req, "X-Three", "3");

  g_assert_cmpuint(http_request_headers_count(req), ==, 3);
  g_assert_cmpstr(http_request_header_key(req, 0), ==, "X-One");
  g_assert_cmpstr(http_request_header_value(req, 0), ==, "1");
  g_assert_cmpstr(http_request_header_key(req, 1), ==, "X-Two");
  g_assert_cmpstr(http_request_header_value(req, 1), ==, "2");
  g_assert_cmpstr(http_request_header_key(req, 2), ==, "X-Three");
  g_assert_cmpstr(http_request_header_value(req, 2), ==, "3");

  http_request_free(req);
}

static void test_add_header_rejects_empty_or_null_keys(void) {
  HttpRequest *req = http_request_new("https://example.com", HTTP_GET);

  http_request_add_header(req, NULL, "v");
  http_request_add_header(req, "", "v");
  http_request_add_header(NULL, "X-Foo", "bar");

  g_assert_cmpuint(http_request_headers_count(req), ==, 0);

  http_request_add_header(req, "X-Allow-Null-Value", NULL);
  g_assert_cmpuint(http_request_headers_count(req), ==, 1);
  g_assert_cmpstr(http_request_header_value(req, 0), ==, "");

  http_request_free(req);
}

static void test_header_accessors_out_of_range(void) {
  HttpRequest *req = http_request_new("https://example.com", HTTP_GET);
  http_request_add_header(req, "X-A", "a");

  g_assert_null(http_request_header_key(req, 5));
  g_assert_null(http_request_header_value(req, 5));
  g_assert_null(http_request_header_key(NULL, 0));
  g_assert_null(http_request_header_value(NULL, 0));
  g_assert_cmpuint(http_request_headers_count(NULL), ==, 0);

  http_request_free(req);
}

static void test_set_body_replaces_and_clears(void) {
  HttpRequest *req = http_request_new("https://example.com", HTTP_POST);
  http_request_set_body(req, "{\"a\":1}", NULL);
  g_assert_cmpstr(req->body, ==, "{\"a\":1}");

  http_request_set_body(req, "{\"a\":2}", NULL);
  g_assert_cmpstr(req->body, ==, "{\"a\":2}");

  http_request_set_body(req, NULL, NULL);
  g_assert_null(req->body);

  http_request_set_body(NULL, "x", NULL);

  http_request_free(req);
}

static void test_add_query_param_url_encodes_special_chars(void) {
  HttpRequest *req = http_request_new("https://example.com", HTTP_GET);
  http_request_add_query_param(req, "q", "hello world");
  http_request_add_query_param(req, "tag", "a/b&c");

  g_assert_cmpuint(req->query_params->len, ==, 2);
  g_assert_cmpstr(g_ptr_array_index(req->query_params, 0), ==,
                  "q=hello%20world");
  const char *tag = g_ptr_array_index(req->query_params, 1);
  g_assert_nonnull(strstr(tag, "tag="));
  g_assert_nonnull(strstr(tag, "%2F"));
  g_assert_nonnull(strstr(tag, "%26"));

  http_request_free(req);
}

static void test_add_query_param_null_safe(void) {
  HttpRequest *req = http_request_new("https://example.com", HTTP_GET);
  http_request_add_query_param(req, NULL, "v");
  http_request_add_query_param(req, "k", NULL);
  http_request_add_query_param(NULL, "k", "v");
  g_assert_cmpuint(req->query_params->len, ==, 0);
  http_request_free(req);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  curl_global_init(CURL_GLOBAL_DEFAULT);

  g_test_add_func("/http/request/new/null_or_empty_url",
                  test_new_rejects_null_or_empty_url);
  g_test_add_func("/http/request/new/defaults", test_new_initializes_defaults);
  g_test_add_func("/http/request/free/null_safe", test_free_null_safe);
  g_test_add_func("/http/request/headers/add_appends",
                  test_add_header_appends_in_order);
  g_test_add_func("/http/request/headers/rejects_empty_or_null_keys",
                  test_add_header_rejects_empty_or_null_keys);
  g_test_add_func("/http/request/headers/accessors_out_of_range",
                  test_header_accessors_out_of_range);
  g_test_add_func("/http/request/body/set_replaces_and_clears",
                  test_set_body_replaces_and_clears);
  g_test_add_func("/http/request/query/add_encodes",
                  test_add_query_param_url_encodes_special_chars);
  g_test_add_func("/http/request/query/add_null_safe",
                  test_add_query_param_null_safe);

  int ret = g_test_run();
  curl_global_cleanup();
  return ret;
}
