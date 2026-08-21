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

#include "../src/http/response.h"

#include <curl/curl.h>
#include <glib.h>
#include <stdint.h>
#include <string.h>

static void test_create_initializes_empty(void) {
  HttpResponse *r = http_response_new();
  g_assert_nonnull(r);
  g_assert_nonnull(r->body);
  g_assert_cmpstr(r->body, ==, "");
  g_assert_cmpuint(r->body_size, ==, 0);
  g_assert_cmpint(r->http_status, ==, 0);
  g_assert_cmpint(r->curl_code, ==, CURLE_OK);
  g_assert_null(r->content_type);
  http_response_free(r);
}

static void test_free_null_safe(void) { http_response_free(NULL); }

static void test_write_callback_appends_chunks(void) {
  GString *body = g_string_new(NULL);
  const char *first = "hello, ";
  const char *second = "world!";

  size_t n1 =
      http_response_write_callback((void *)first, 1, strlen(first), body);
  size_t n2 =
      http_response_write_callback((void *)second, 1, strlen(second), body);

  g_assert_cmpuint(n1, ==, strlen(first));
  g_assert_cmpuint(n2, ==, strlen(second));
  g_assert_cmpuint(body->len, ==, strlen(first) + strlen(second));
  g_assert_cmpstr(body->str, ==, "hello, world!");

  g_string_free(body, TRUE);
}

static void test_write_callback_handles_size_times_nmemb(void) {
  GString *body = g_string_new(NULL);
  const char *data = "abcdEFGHwxyz";
  size_t n = http_response_write_callback((void *)data, 4, 3, body);
  g_assert_cmpuint(n, ==, 12);
  g_assert_cmpuint(body->len, ==, 12);
  g_assert_cmpstr(body->str, ==, "abcdEFGHwxyz");
  g_string_free(body, TRUE);
}

static void test_write_callback_zero_length_is_safe(void) {
  GString *body = g_string_new(NULL);
  size_t n = http_response_write_callback((void *)"x", 0, 0, body);
  g_assert_cmpuint(n, ==, 0);
  g_assert_cmpuint(body->len, ==, 0);
  g_assert_cmpstr(body->str, ==, "");
  g_string_free(body, TRUE);
}

static void test_write_callback_preserves_binary_payload(void) {
  GString *body = g_string_new(NULL);
  const unsigned char data[] = {0x00, 0x01, 0xFF, 0x42, 0x00, 0xAB};
  size_t n = http_response_write_callback((void *)data, 1, sizeof(data), body);
  g_assert_cmpuint(n, ==, sizeof(data));
  g_assert_cmpuint(body->len, ==, sizeof(data));
  g_assert_cmpint(memcmp(body->str, data, sizeof(data)), ==, 0);
  g_string_free(body, TRUE);
}

static void test_write_callback_rejects_payload_size_overflow(void) {
  GString *body = g_string_new(NULL);
  size_t n = http_response_write_callback((void *)"x", SIZE_MAX, 2, body);
  g_assert_cmpuint(n, ==, 0);
  g_assert_cmpuint(body->len, ==, 0);
  g_string_free(body, TRUE);
}

static void test_write_callback_rejects_writes_past_max(void) {
  GString *body = g_string_new(NULL);
  g_string_set_size(body, HTTP_RESPONSE_MAX_BODY_SIZE);

  size_t n = http_response_write_callback((void *)"x", 1, 1, body);
  g_assert_cmpuint(n, ==, 0);
  g_assert_cmpuint(body->len, ==, (gsize)HTTP_RESPONSE_MAX_BODY_SIZE);

  g_string_free(body, TRUE);
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

  return g_test_run();
}
