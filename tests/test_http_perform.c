/*******************************************************************************
 * REQUEST HUB
 * =============================================================================
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

#include "../src/http/http.h"
#include "../src/http/http_pool.h"
#include "../src/http/methods.h"
#include "../src/http/request.h"
#include "../src/http/response.h"

#include <curl/curl.h>
#include <glib.h>
#include <string.h>

#define BASE_URL "https://jsonplaceholder.typicode.com"

static gboolean network_enabled(void) {
  const char *flag = g_getenv("REQUESTHUB_NETWORK_TESTS");
  return flag != NULL && *flag != '\0' && g_strcmp0(flag, "0") != 0;
}

#define SKIP_IF_NO_NETWORK()                                                   \
  do {                                                                         \
    if (!network_enabled()) {                                                  \
      g_test_skip("set REQUESTHUB_NETWORK_TESTS=1 to run network tests");      \
      return;                                                                  \
    }                                                                          \
  } while (0)

static void test_perform_rejects_null_request(void) {
  g_assert_null(http_request_perform(NULL));
}

static void test_perform_rejects_request_without_url(void) {
  HttpRequest req = {0};
  g_assert_null(http_request_perform(&req));
}

static void test_get_returns_200_with_json_body(void) {
  SKIP_IF_NO_NETWORK();
  HttpRequest *req = http_request_new(BASE_URL "/todos/1", HTTP_GET);
  HttpResponse *res = http_request_perform(req);

  g_assert_nonnull(res);
  g_assert_cmpint(res->curl_code, ==, CURLE_OK);
  g_assert_cmpint(res->http_status, ==, 200);
  g_assert_nonnull(res->content_type);
  g_assert_true(g_str_has_prefix(res->content_type, "application/json"));
  g_assert_nonnull(res->body);
  g_assert_cmpuint(res->body_size, >, 0);
  g_assert_nonnull(strstr(res->body, "\"id\""));
  g_assert_nonnull(res->all_headers);

  http_response_free(res);
  http_request_free(req);
}

static void test_get_with_query_params_filters_results(void) {
  SKIP_IF_NO_NETWORK();
  HttpRequest *req = http_request_new(BASE_URL "/comments", HTTP_GET);
  http_request_add_query_param(req, "postId", "1");
  HttpResponse *res = http_request_perform(req);

  g_assert_nonnull(res);
  g_assert_cmpint(res->http_status, ==, 200);
  g_assert_nonnull(res->body);
  g_assert_nonnull(strstr(res->body, "\"postId\""));
  g_assert_null(strstr(res->body, "\"postId\": 2"));

  http_response_free(res);
  http_request_free(req);
}

static void test_get_unknown_resource_returns_404(void) {
  SKIP_IF_NO_NETWORK();
  HttpRequest *req =
      http_request_new(BASE_URL "/this-resource-does-not-exist", HTTP_GET);
  HttpResponse *res = http_request_perform(req);

  g_assert_nonnull(res);
  g_assert_cmpint(res->curl_code, ==, CURLE_OK);
  g_assert_cmpint(res->http_status, ==, 404);

  http_response_free(res);
  http_request_free(req);
}

static void test_post_creates_resource(void) {
  SKIP_IF_NO_NETWORK();
  HttpRequest *req = http_request_new(BASE_URL "/posts", HTTP_POST);
  http_request_add_header(req, "Content-Type", "application/json");
  http_request_set_body(
      req, "{\"title\":\"foo\",\"body\":\"bar\",\"userId\":1}");
  HttpResponse *res = http_request_perform(req);

  g_assert_nonnull(res);
  g_assert_cmpint(res->http_status, ==, 201);
  g_assert_nonnull(res->body);
  g_assert_nonnull(strstr(res->body, "\"id\""));

  http_response_free(res);
  http_request_free(req);
}

static void test_body_methods_send_payload(void) {
  SKIP_IF_NO_NETWORK();

  const struct {
    const char *label;
    HttpMethods method;
    const char *body;
    const char *expected_echo;
  } cases[] = {
      {"PUT", HTTP_PUT,
       "{\"id\":1,\"title\":\"updated\",\"body\":\"x\",\"userId\":1}",
       "updated"},
      {"PATCH", HTTP_PATCH, "{\"title\":\"patched\"}", "patched"},
  };

  for (size_t i = 0; i < G_N_ELEMENTS(cases); i++) {
    HttpRequest *req = http_request_new(BASE_URL "/posts/1", cases[i].method);
    http_request_add_header(req, "Content-Type", "application/json");
    http_request_set_body(req, cases[i].body);
    HttpResponse *res = http_request_perform(req);

    g_assert_nonnull(res);
    if (res->http_status != 200) {
      g_error("%s: expected status 200, got %ld", cases[i].label,
              res->http_status);
    }
    g_assert_nonnull(res->body);
    if (strstr(res->body, cases[i].expected_echo) == NULL) {
      g_error("%s: response body did not echo %s", cases[i].label,
              cases[i].expected_echo);
    }

    http_response_free(res);
    http_request_free(req);
  }
}

static void test_delete_removes_resource(void) {
  SKIP_IF_NO_NETWORK();
  HttpRequest *req = http_request_new(BASE_URL "/posts/1", HTTP_DELETE);
  HttpResponse *res = http_request_perform(req);

  g_assert_nonnull(res);
  g_assert_cmpint(res->http_status, ==, 200);

  http_response_free(res);
  http_request_free(req);
}

static void test_head_returns_no_body(void) {
  SKIP_IF_NO_NETWORK();
  HttpRequest *req = http_request_new(BASE_URL "/todos/1", HTTP_HEAD);
  HttpResponse *res = http_request_perform(req);

  g_assert_nonnull(res);
  g_assert_cmpint(res->http_status, ==, 200);
  g_assert_cmpuint(res->body_size, ==, 0);

  http_response_free(res);
  http_request_free(req);
}

static void test_unsupported_protocol_reports_curl_error(void) {
  HttpRequest *req = http_request_new("nope://example.invalid/", HTTP_GET);
  HttpResponse *res = http_request_perform(req);

  g_assert_nonnull(res);
  g_assert_cmpint(res->curl_code, !=, CURLE_OK);
  g_assert_cmpint(res->http_status, ==, 0);

  http_response_free(res);
  http_request_free(req);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  curl_global_init(CURL_GLOBAL_DEFAULT);
  http_pool_init();

  g_test_add_func("/http/perform/null_request",
                  test_perform_rejects_null_request);
  g_test_add_func("/http/perform/request_without_url",
                  test_perform_rejects_request_without_url);
  g_test_add_func("/http/perform/get_200",
                  test_get_returns_200_with_json_body);
  g_test_add_func("/http/perform/get_with_query_params",
                  test_get_with_query_params_filters_results);
  g_test_add_func("/http/perform/get_404",
                  test_get_unknown_resource_returns_404);
  g_test_add_func("/http/perform/post_201", test_post_creates_resource);
  g_test_add_func("/http/perform/body_methods_send_payload",
                  test_body_methods_send_payload);
  g_test_add_func("/http/perform/delete_200", test_delete_removes_resource);
  g_test_add_func("/http/perform/head_no_body", test_head_returns_no_body);
  g_test_add_func("/http/perform/unsupported_protocol",
                  test_unsupported_protocol_reports_curl_error);

  int ret = g_test_run();
  http_pool_cleanup();
  curl_global_cleanup();
  return ret;
}
