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

#include "../src/history/history.h"

#include <glib.h>

static HistoryEntry *make_entry(HttpMethod method, const char *url) {
  HistoryEntry *entry = history_entry_new();
  entry->request.method = method;
  request_data_set_url(&entry->request, url);
  return entry;
}

static void test_entry_lifecycle(void) {
  HistoryEntry *entry = history_entry_new();
  g_assert_nonnull(entry);
  g_assert_nonnull(entry->id);
  g_assert_cmpint(entry->request.method, ==, HTTP_GET);
  g_assert_null(entry->request.url);
  g_assert_null(entry->request.body);
  g_assert_null(entry->response_body);
  g_assert_cmpuint(request_data_headers_count(&entry->request), ==, 0);
  g_assert_cmpuint(request_data_query_count(&entry->request), ==, 0);
  g_assert_cmpint(entry->http_status, ==, 0);
  history_entry_free(entry);
  history_entry_free(NULL);
}

static void test_request_data_set_url_replaces(void) {
  RequestData *data = request_data_new();

  request_data_set_url(data, "https://example.com/v1");
  g_assert_cmpstr(data->url, ==, "https://example.com/v1");

  request_data_set_url(data, "https://example.com/v2");
  g_assert_cmpstr(data->url, ==, "https://example.com/v2");

  request_data_set_url(data, NULL);
  g_assert_null(data->url);

  request_data_free(data);
  request_data_free(NULL);
}

static void test_request_data_rejects_empty_keys(void) {
  RequestData *data = request_data_new();
  request_data_add_header(data, "", "value");
  request_data_add_header(data, NULL, "value");
  request_data_add_query(data, "", "value");
  request_data_add_query(data, NULL, "value");

  g_assert_cmpuint(request_data_headers_count(data), ==, 0);
  g_assert_cmpuint(request_data_query_count(data), ==, 0);

  request_data_free(data);
}

static void test_entry_from_request_copies_and_filters(void) {
  RequestData *data = request_data_new();
  data->method = HTTP_POST;
  request_data_set_url(data, "https://api.example.com/users");
  request_data_set_body(data, "{\"name\":\"Alice\"}");
  request_data_add_header(data, "X-Foo", "bar");
  request_data_add_header(data, "Authorization", "Bearer secret");
  request_data_add_header(data, "authorization", "Bearer also-filtered");
  request_data_add_header(data, "Content-Type", "application/json");
  request_data_add_query(data, "page", "1");

  HistoryEntry *entry = history_entry_new_from_request(data);
  g_assert_nonnull(entry);
  g_assert_cmpint(entry->request.method, ==, HTTP_POST);
  g_assert_cmpstr(entry->request.url, ==, "https://api.example.com/users");
  g_assert_cmpstr(entry->request.body, ==, "{\"name\":\"Alice\"}");

  g_assert_cmpuint(request_data_headers_count(&entry->request), ==, 2);
  g_assert_cmpstr(request_data_header_key(&entry->request, 0), ==, "X-Foo");
  g_assert_cmpstr(request_data_header_value(&entry->request, 0), ==, "bar");
  g_assert_cmpstr(request_data_header_key(&entry->request, 1), ==,
                  "Content-Type");

  g_assert_cmpuint(request_data_query_count(&entry->request), ==, 1);
  g_assert_cmpstr(request_data_query_key(&entry->request, 0), ==, "page");

  history_entry_free(entry);
  request_data_free(data);
}

static void test_entry_from_request_skips_empty_body(void) {
  RequestData *data = request_data_new();
  request_data_set_url(data, "https://example.com");
  request_data_set_body(data, "");

  HistoryEntry *entry = history_entry_new_from_request(data);
  g_assert_nonnull(entry);
  g_assert_null(entry->request.body);

  history_entry_free(entry);
  request_data_free(data);

  g_assert_null(history_entry_new_from_request(NULL));
}

static void test_entry_move_content_transfers_ownership(void) {
  HistoryEntry *src = history_entry_new();
  src->request.method = HTTP_POST;
  request_data_set_url(&src->request, "https://example.com/api");
  request_data_set_body(&src->request, "{\"hello\":\"world\"}");
  request_data_add_header(&src->request, "X-Test", "value");
  request_data_add_query(&src->request, "q", "search term");
  src->http_status = 201;
  src->total_time_s = 0.5;
  src->response_size = 42;

  HistoryEntry *dst = history_entry_new();
  gchar *original_dst_id = g_strdup(dst->id);

  history_entry_move_content_from(dst, src);

  g_assert_cmpstr(dst->id, ==, original_dst_id);
  g_assert_cmpint(dst->request.method, ==, HTTP_POST);
  g_assert_cmpstr(dst->request.url, ==, "https://example.com/api");
  g_assert_cmpstr(dst->request.body, ==, "{\"hello\":\"world\"}");
  g_assert_cmpuint(request_data_headers_count(&dst->request), ==, 1);
  g_assert_cmpstr(request_data_header_key(&dst->request, 0), ==, "X-Test");
  g_assert_cmpuint(request_data_query_count(&dst->request), ==, 1);
  g_assert_cmpstr(request_data_query_key(&dst->request, 0), ==, "q");
  g_assert_cmpint(dst->http_status, ==, 201);

  g_assert_null(src->request.url);
  g_assert_null(src->request.body);
  g_assert_null(src->request.headers);
  g_assert_null(src->request.query_params);

  g_free(original_dst_id);
  history_entry_free(src);
  history_entry_free(dst);
}

static void test_entry_move_content_self_is_noop(void) {
  HistoryEntry *entry = history_entry_new();
  request_data_set_url(&entry->request, "https://example.com");
  history_entry_move_content_from(entry, entry);
  g_assert_cmpstr(entry->request.url, ==, "https://example.com");
  history_entry_free(entry);
}

static void test_store_prepend_and_count(void) {
  HistoryStore *store = history_store_new(10);
  g_assert_cmpuint(history_store_count(store), ==, 0);

  history_store_prepend(store, make_entry(HTTP_GET, "https://a"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://b"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://c"));

  g_assert_cmpuint(history_store_count(store), ==, 3);
  g_assert_cmpstr(history_store_get(store, 0)->request.url, ==, "https://c");
  g_assert_cmpstr(history_store_get(store, 1)->request.url, ==, "https://b");
  g_assert_cmpstr(history_store_get(store, 2)->request.url, ==, "https://a");

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
  g_assert_cmpstr(history_store_get(store, 0)->request.url, ==, "https://e4");
  g_assert_cmpstr(history_store_get(store, 1)->request.url, ==, "https://e3");
  g_assert_cmpstr(history_store_get(store, 2)->request.url, ==, "https://e2");

  history_store_free(store);
}

static void test_store_find_by_request_matches_url_and_method(void) {
  HistoryStore *store = history_store_new(10);

  history_store_prepend(store, make_entry(HTTP_GET, "https://a"));
  history_store_prepend(store, make_entry(HTTP_POST, "https://a"));
  history_store_prepend(store, make_entry(HTTP_GET, "https://b"));

  HistoryEntry *post_a =
      history_store_find_by_request(store, "https://a", HTTP_POST);
  g_assert_nonnull(post_a);
  g_assert_cmpint(post_a->request.method, ==, HTTP_POST);

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
  g_assert_cmpstr(history_store_get(store, 1)->request.url, ==, "https://c");
  g_assert_cmpstr(history_store_get(store, 2)->request.url, ==, "https://b");

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
  g_assert_cmpstr(history_store_get(store, 0)->request.url, ==, "https://c");
  g_assert_cmpstr(history_store_get(store, 1)->request.url, ==, "https://a");

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
  request_data_set_body(&entry->request, "{\"name\":\"Alice\"}");
  request_data_add_header(&entry->request, "Content-Type", "application/json");
  request_data_add_query(&entry->request, "page", "1");
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
  g_assert_cmpint(recovered->request.method, ==, HTTP_POST);
  g_assert_cmpstr(recovered->request.url, ==, "https://api.example.com/v1");
  g_assert_cmpstr(recovered->request.body, ==, "{\"name\":\"Alice\"}");
  g_assert_cmpint(recovered->http_status, ==, 200);
  g_assert_cmpuint(request_data_headers_count(&recovered->request), ==, 1);
  g_assert_cmpstr(request_data_header_key(&recovered->request, 0), ==,
                  "Content-Type");
  g_assert_cmpuint(request_data_query_count(&recovered->request), ==, 1);
  g_assert_cmpstr(request_data_query_key(&recovered->request, 0), ==, "page");

  history_store_free(loaded);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func("/history/entry/lifecycle", test_entry_lifecycle);
  g_test_add_func("/history/request_data/set_url_replaces",
                  test_request_data_set_url_replaces);
  g_test_add_func("/history/request_data/rejects_empty_keys",
                  test_request_data_rejects_empty_keys);
  g_test_add_func("/history/entry/from_request_copies_and_filters",
                  test_entry_from_request_copies_and_filters);
  g_test_add_func("/history/entry/from_request_skips_empty_body",
                  test_entry_from_request_skips_empty_body);
  g_test_add_func("/history/entry/move_content",
                  test_entry_move_content_transfers_ownership);
  g_test_add_func("/history/entry/move_content_self",
                  test_entry_move_content_self_is_noop);

  g_test_add_func("/history/store/prepend_and_count",
                  test_store_prepend_and_count);
  g_test_add_func("/history/store/eviction_at_max",
                  test_store_eviction_at_max);
  g_test_add_func("/history/store/find_by_request",
                  test_store_find_by_request_matches_url_and_method);
  g_test_add_func("/history/store/promote", test_store_promote_moves_to_front);
  g_test_add_func("/history/store/remove", test_store_remove_specific_entry);
  g_test_add_func("/history/store/clear", test_store_clear);
  g_test_add_func("/history/store/save_load_roundtrip",
                  test_store_save_and_load_roundtrip);

  return g_test_run();
}
