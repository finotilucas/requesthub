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

#include "../src/http/response.h"

#include <curl/curl.h>
#include <glib.h>
#include <stdint.h>
#include <string.h>

static void test_create_initializes_empty(void) {
  HttpResponse *r = http_response_create();
  g_assert_nonnull(r);
  g_assert_nonnull(r->body);
  g_assert_cmpstr(r->body, ==, "");
  g_assert_cmpuint(r->body_size, ==, 0);
  g_assert_cmpint(r->http_status, ==, 0);
  g_assert_cmpint(r->curl_code, ==, CURLE_OK);
  g_assert_null(r->content_type);
  g_assert_null(r->header_location);
  g_assert_null(r->etag);
  g_assert_null(r->all_headers);
  http_response_free(r);
}

static void test_free_null_safe(void) { http_response_free(NULL); }

static void test_write_callback_appends_chunks(void) {
  HttpResponse *r = http_response_create();
  const char *first = "hello, ";
  const char *second = "world!";

  size_t n1 = http_response_write_callback((void *)first, 1, strlen(first), r);
  size_t n2 = http_response_write_callback((void *)second, 1, strlen(second), r);

  g_assert_cmpuint(n1, ==, strlen(first));
  g_assert_cmpuint(n2, ==, strlen(second));
  g_assert_cmpuint(r->body_size, ==, strlen(first) + strlen(second));
  g_assert_cmpstr(r->body, ==, "hello, world!");

  http_response_free(r);
}

static void test_write_callback_handles_size_times_nmemb(void) {
  HttpResponse *r = http_response_create();
  const char *data = "abcdEFGHwxyz";
  size_t n = http_response_write_callback((void *)data, 4, 3, r);
  g_assert_cmpuint(n, ==, 12);
  g_assert_cmpuint(r->body_size, ==, 12);
  g_assert_cmpstr(r->body, ==, "abcdEFGHwxyz");
  http_response_free(r);
}

static void test_write_callback_zero_length_is_safe(void) {
  HttpResponse *r = http_response_create();
  size_t n = http_response_write_callback((void *)"x", 0, 0, r);
  g_assert_cmpuint(n, ==, 0);
  g_assert_cmpuint(r->body_size, ==, 0);
  g_assert_cmpstr(r->body, ==, "");
  http_response_free(r);
}

static void test_write_callback_preserves_binary_payload(void) {
  HttpResponse *r = http_response_create();
  const unsigned char data[] = {0x00, 0x01, 0xFF, 0x42, 0x00, 0xAB};
  size_t n = http_response_write_callback((void *)data, 1, sizeof(data), r);
  g_assert_cmpuint(n, ==, sizeof(data));
  g_assert_cmpuint(r->body_size, ==, sizeof(data));
  g_assert_cmpint(memcmp(r->body, data, sizeof(data)), ==, 0);
  http_response_free(r);
}

static void test_write_callback_rejects_payload_size_overflow(void) {
  HttpResponse *r = http_response_create();
  size_t n = http_response_write_callback((void *)"x", SIZE_MAX, 2, r);
  g_assert_cmpuint(n, ==, 0);
  g_assert_cmpuint(r->body_size, ==, 0);
  http_response_free(r);
}

static void test_write_callback_rejects_writes_past_max(void) {
  HttpResponse *r = http_response_create();
  r->body_size = HTTP_RESPONSE_MAX_BODY_SIZE;

  size_t n = http_response_write_callback((void *)"x", 1, 1, r);
  g_assert_cmpuint(n, ==, 0);
  g_assert_cmpuint(r->body_size, ==, (size_t)HTTP_RESPONSE_MAX_BODY_SIZE);

  r->body_size = 0;
  http_response_free(r);
}

static void feed_header_line(HttpResponse *r, const char *line) {
  size_t len = strlen(line);
  size_t n = http_response_header_callback((char *)line, 1, len, r);
  g_assert_cmpuint(n, ==, len);
}

static gboolean slist_contains(struct curl_slist *list, const char *value) {
  while (list != NULL) {
    if (g_strcmp0(list->data, value) == 0) {
      return TRUE;
    }
    list = list->next;
  }
  return FALSE;
}

static void test_header_callback_collects_headers(void) {
  HttpResponse *r = http_response_create();

  feed_header_line(r, "HTTP/1.1 200 OK\r\n");
  feed_header_line(r, "Content-Type: application/json\r\n");
  feed_header_line(r, "ETag: \"abc-123\"\r\n");
  feed_header_line(r, "Location: https://example.com/new\r\n");
  feed_header_line(r, "\r\n");

  g_assert_nonnull(r->all_headers);
  g_assert_true(slist_contains(r->all_headers, "Content-Type: application/json"));
  g_assert_true(slist_contains(r->all_headers, "ETag: \"abc-123\""));
  g_assert_cmpstr(r->etag, ==, "\"abc-123\"");
  g_assert_cmpstr(r->header_location, ==, "https://example.com/new");

  http_response_free(r);
}

static void test_header_callback_resets_on_new_status_line(void) {
  HttpResponse *r = http_response_create();

  feed_header_line(r, "HTTP/1.1 301 Moved Permanently\r\n");
  feed_header_line(r, "Location: https://first/redirect\r\n");
  feed_header_line(r, "ETag: \"first\"\r\n");
  feed_header_line(r, "\r\n");

  feed_header_line(r, "HTTP/1.1 200 OK\r\n");
  feed_header_line(r, "Location: https://second/final\r\n");
  feed_header_line(r, "\r\n");

  g_assert_cmpstr(r->header_location, ==, "https://second/final");
  g_assert_null(r->etag);

  http_response_free(r);
}

static void test_header_callback_blank_line_is_safe(void) {
  HttpResponse *r = http_response_create();
  feed_header_line(r, "HTTP/1.1 200 OK\r\n");
  feed_header_line(r, "\r\n");
  http_response_free(r);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);

  g_test_add_func("/http/response/create/empty",
                  test_create_initializes_empty);
  g_test_add_func("/http/response/free/null_safe", test_free_null_safe);
  g_test_add_func("/http/response/write_callback/appends",
                  test_write_callback_appends_chunks);
  g_test_add_func("/http/response/write_callback/size_nmemb",
                  test_write_callback_handles_size_times_nmemb);
  g_test_add_func("/http/response/write_callback/zero_length",
                  test_write_callback_zero_length_is_safe);
  g_test_add_func("/http/response/write_callback/binary",
                  test_write_callback_preserves_binary_payload);
  g_test_add_func("/http/response/write_callback/size_overflow",
                  test_write_callback_rejects_payload_size_overflow);
  g_test_add_func("/http/response/write_callback/exceeds_max",
                  test_write_callback_rejects_writes_past_max);
  g_test_add_func("/http/response/header_callback/collects",
                  test_header_callback_collects_headers);
  g_test_add_func("/http/response/header_callback/reset_on_new_status",
                  test_header_callback_resets_on_new_status_line);
  g_test_add_func("/http/response/header_callback/blank_line",
                  test_header_callback_blank_line_is_safe);

  return g_test_run();
}
