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

#include "../src/history/history.h"

#include <glib.h>

static HistoryEntry *make_entry(HttpMethods method, const char *url) {
  HistoryEntry *entry = history_entry_new();
  entry->method = method;
  history_entry_set_url(entry, url);
  return entry;
}

static void test_entry_lifecycle(void) {
  HistoryEntry *entry = history_entry_new();
  g_assert_nonnull(entry);
  g_assert_nonnull(entry->id);
  g_assert_cmpint(entry->method, ==, HTTP_GET);
  g_assert_null(entry->url);
  g_assert_null(entry->body);
  g_assert_null(entry->response_body);
  g_assert_cmpuint(history_entry_headers_count(entry), ==, 0);
  g_assert_cmpuint(history_entry_query_count(entry), ==, 0);
  g_assert_cmpint(entry->http_status, ==, 0);
  history_entry_free(entry);
  history_entry_free(NULL);
}

static void test_entry_set_url_replaces(void) {
  HistoryEntry *entry = history_entry_new();

  history_entry_set_url(entry, "https://example.com/v1");
  g_assert_cmpstr(entry->url, ==, "https://example.com/v1");

  history_entry_set_url(entry, "https://example.com/v2");
  g_assert_cmpstr(entry->url, ==, "https://example.com/v2");

  history_entry_set_url(entry, NULL);
  g_assert_null(entry->url);

  history_entry_free(entry);
}

static void test_entry_filters_authorization_header(void) {
  HistoryEntry *entry = history_entry_new();
  history_entry_add_header(entry, "X-Foo", "bar");
  history_entry_add_header(entry, "Authorization", "Bearer secret");
  history_entry_add_header(entry, "authorization", "Bearer also-filtered");
  history_entry_add_header(entry, "Content-Type", "application/json");

  g_assert_cmpuint(history_entry_headers_count(entry), ==, 2);
  g_assert_cmpstr(history_entry_header_key(entry, 0), ==, "X-Foo");
  g_assert_cmpstr(history_entry_header_value(entry, 0), ==, "bar");
  g_assert_cmpstr(history_entry_header_key(entry, 1), ==, "Content-Type");

  history_entry_free(entry);
}

static void test_entry_rejects_empty_keys(void) {
  HistoryEntry *entry = history_entry_new();
  history_entry_add_header(entry, "", "value");
  history_entry_add_header(entry, NULL, "value");
  history_entry_add_query_param(entry, "", "value");
  history_entry_add_query_param(entry, NULL, "value");

  g_assert_cmpuint(history_entry_headers_count(entry), ==, 0);
  g_assert_cmpuint(history_entry_query_count(entry), ==, 0);

  history_entry_free(entry);
}

static void test_entry_take_payload_transfers_ownership(void) {
  HistoryEntry *src = history_entry_new();
  src->method = HTTP_POST;
  history_entry_set_url(src, "https://example.com/api");
  history_entry_set_body(src, "{\"hello\":\"world\"}");
  history_entry_add_header(src, "X-Test", "value");
  history_entry_add_query_param(src, "q", "search term");
  src->http_status = 201;
  src->total_time_s = 0.5;
  src->response_size = 42;

  HistoryEntry *dst = history_entry_new();
  gchar *original_dst_id = g_strdup(dst->id);

  history_entry_take_payload(dst, src);

  g_assert_cmpstr(dst->id, ==, original_dst_id);
  g_assert_cmpint(dst->method, ==, HTTP_POST);
  g_assert_cmpstr(dst->url, ==, "https://example.com/api");
  g_assert_cmpstr(dst->body, ==, "{\"hello\":\"world\"}");
  g_assert_cmpuint(history_entry_headers_count(dst), ==, 1);
  g_assert_cmpstr(history_entry_header_key(dst, 0), ==, "X-Test");
  g_assert_cmpuint(history_entry_query_count(dst), ==, 1);
  g_assert_cmpstr(history_entry_query_key(dst, 0), ==, "q");
  g_assert_cmpint(dst->http_status, ==, 201);

  g_assert_null(src->url);
  g_assert_null(src->body);
  g_assert_null(src->headers);
  g_assert_null(src->query_params);

  g_free(original_dst_id);
  history_entry_free(src);
  history_entry_free(dst);
}

static void test_entry_take_payload_self_is_noop(void) {
  HistoryEntry *entry = history_entry_new();
  history_entry_set_url(entry, "https://example.com");
  history_entry_take_payload(entry, entry);
  g_assert_cmpstr(entry->url, ==, "https://example.com");
  history_entry_free(entry);
}

static void test_store_prepend_and_count(void) {
  HistoryStore *store = history_store_new(10);
  g_assert_cmpuint(history_store_count(store), ==, 0);

  history_store_prepend(store, make_entry(HTTP_GET, "https://a"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://b"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://c"));

  g_assert_cmpuint(history_store_count(store), ==, 3);
  g_assert_cmpstr(history_store_get(store, 0)->url, ==, "https://c");
  g_assert_cmpstr(history_store_get(store, 1)->url, ==, "https://b");
  g_assert_cmpstr(history_store_get(store, 2)->url, ==, "https://a");

  history_store_free(store);
}

static void test_store_eviction_at_max(void) {
  HistoryStore *store = history_store_new(3);

  for (int i = 0; i < 5; i++) {
    char url[32];
    g_snprintf(url, sizeof(url), "https://e%d", i);
    history_store_prepend(store, make_entry(HTTP_GET, url));
  }

  g_assert_cmpuint(history_store_count(store), ==, 3);
  g_assert_cmpstr(history_store_get(store, 0)->url, ==, "https://e4");
  g_assert_cmpstr(history_store_get(store, 1)->url, ==, "https://e3");
  g_assert_cmpstr(history_store_get(store, 2)->url, ==, "https://e2");

  history_store_free(store);
}

static void test_store_eviction_helper(void) {
  g_assert_true(history_store_evicted_after_prepend(5, 5));
  g_assert_false(history_store_evicted_after_prepend(3, 4));
  g_assert_false(history_store_evicted_after_prepend(0, 1));
}

static void test_store_find_by_request_matches_url_and_method(void) {
  HistoryStore *store = history_store_new(10);

  history_store_prepend(store, make_entry(HTTP_GET, "https://a"));
  history_store_prepend(store, make_entry(HTTP_POST, "https://a"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://b"));

  HistoryEntry *post_a =
      history_store_find_by_request(store, "https://a", HTTP_POST);
  g_assert_nonnull(post_a);
  g_assert_cmpint(post_a->method, ==, HTTP_POST);

  HistoryEntry *get_a =
      history_store_find_by_request(store, "https://a", HTTP_GET);
  g_assert_nonnull(get_a);
  g_assert_true(get_a != post_a);

  g_assert_null(history_store_find_by_request(store, "https://x", HTTP_GET));
  g_assert_null(history_store_find_by_request(store, "https://a", HTTP_DELETE));

  history_store_free(store);
}

static void test_store_promote_moves_to_front(void) {
  HistoryStore *store = history_store_new(10);

  history_store_prepend(store, make_entry(HTTP_GET, "https://a"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://b"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://c"));

  HistoryEntry *a = history_store_get(store, 2);
  g_assert_true(history_store_promote(store, a));

  g_assert_cmpuint(history_store_count(store), ==, 3);
  g_assert_true(history_store_get(store, 0) == a);
  g_assert_cmpstr(history_store_get(store, 1)->url, ==, "https://c");
  g_assert_cmpstr(history_store_get(store, 2)->url, ==, "https://b");

  g_assert_true(history_store_promote(store, a));
  g_assert_true(history_store_get(store, 0) == a);

  history_store_free(store);
}

static void test_store_remove_specific_entry(void) {
  HistoryStore *store = history_store_new(10);

  history_store_prepend(store, make_entry(HTTP_GET, "https://a"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://b"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://c"));

  HistoryEntry *b = history_store_get(store, 1);
  g_assert_true(history_store_remove(store, b));

  g_assert_cmpuint(history_store_count(store), ==, 2);
  g_assert_cmpstr(history_store_get(store, 0)->url, ==, "https://c");
  g_assert_cmpstr(history_store_get(store, 1)->url, ==, "https://a");

  history_store_free(store);
}

static void test_store_clear(void) {
  HistoryStore *store = history_store_new(10);

  history_store_prepend(store, make_entry(HTTP_GET, "https://a"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://b"));

  history_store_clear(store);
  g_assert_cmpuint(history_store_count(store), ==, 0);

  history_store_free(store);
}

static void test_store_save_and_load_roundtrip(void) {
  HistoryStore *store = history_store_new(10);

  HistoryEntry *entry = make_entry(HTTP_POST, "https://api.example.com/v1");
  history_entry_set_body(entry, "{\"name\":\"Alice\"}");
  history_entry_add_header(entry, "Content-Type", "application/json");
  history_entry_add_query_param(entry, "page", "1");
  entry->http_status = 200;
  entry->total_time_s = 0.123;
  entry->response_size = 17;

  history_store_prepend(store, entry);
  history_store_prepend(store, make_entry(HTTP_GET, "https://example.com"));

  g_assert_true(history_store_save(store));
  history_store_free(store);

  HistoryStore *loaded = history_store_new(10);
  g_assert_true(history_store_load(loaded));

  g_assert_cmpuint(history_store_count(loaded), ==, 2);

  HistoryEntry *recovered = history_store_get(loaded, 1);
  g_assert_cmpint(recovered->method, ==, HTTP_POST);
  g_assert_cmpstr(recovered->url, ==, "https://api.example.com/v1");
  g_assert_cmpstr(recovered->body, ==, "{\"name\":\"Alice\"}");
  g_assert_cmpint(recovered->http_status, ==, 200);
  g_assert_cmpuint(history_entry_headers_count(recovered), ==, 1);
  g_assert_cmpstr(history_entry_header_key(recovered, 0), ==, "Content-Type");
  g_assert_cmpuint(history_entry_query_count(recovered), ==, 1);
  g_assert_cmpstr(history_entry_query_key(recovered, 0), ==, "page");

  history_store_free(loaded);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func("/history/entry/lifecycle", test_entry_lifecycle);
  g_test_add_func("/history/entry/set_url_replaces",
                  test_entry_set_url_replaces);
  g_test_add_func("/history/entry/filters_authorization",
                  test_entry_filters_authorization_header);
  g_test_add_func("/history/entry/rejects_empty_keys",
                  test_entry_rejects_empty_keys);
  g_test_add_func("/history/entry/take_payload",
                  test_entry_take_payload_transfers_ownership);
  g_test_add_func("/history/entry/take_payload_self",
                  test_entry_take_payload_self_is_noop);

  g_test_add_func("/history/store/prepend_and_count",
                  test_store_prepend_and_count);
  g_test_add_func("/history/store/eviction_at_max",
                  test_store_eviction_at_max);
  g_test_add_func("/history/store/eviction_helper", test_store_eviction_helper);
  g_test_add_func("/history/store/find_by_request",
                  test_store_find_by_request_matches_url_and_method);
  g_test_add_func("/history/store/promote", test_store_promote_moves_to_front);
  g_test_add_func("/history/store/remove", test_store_remove_specific_entry);
  g_test_add_func("/history/store/clear", test_store_clear);
  g_test_add_func("/history/store/save_load_roundtrip",
                  test_store_save_and_load_roundtrip);

  return g_test_run();
}
