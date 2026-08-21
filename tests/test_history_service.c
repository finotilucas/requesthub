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

#include "../src/services/history_service.h"

#include <glib.h>

static HistoryEntry *make_entry(HttpMethod method, const char *url,
                                const char *body) {
  HistoryEntry *entry = history_entry_new();
  entry->request.method = method;
  request_data_set_url(&entry->request, url);
  if (body != NULL) {
    request_data_set_body(&entry->request, body);
  }
  return entry;
}

static void on_changed_count(HistoryService *service, gpointer user_data) {
  (void)service;
  guint *count = user_data;
  (*count)++;
}

static void test_record_prepends_new_entries(void) {
  HistoryService *service = history_service_new(10);

  HistoryEntry *first = make_entry(HTTP_GET, "https://a", NULL);
  history_service_record(service, first);
  g_assert_cmpuint(history_service_count(service), ==, 1);
  g_assert_true(history_service_get(service, 0) == first);

  HistoryEntry *second = make_entry(HTTP_GET, "https://b", NULL);
  history_service_record(service, second);
  g_assert_cmpuint(history_service_count(service), ==, 2);
  g_assert_true(history_service_get(service, 0) == second);
  g_assert_true(history_service_get(service, 1) == first);

  g_object_unref(service);
}

static void test_record_dedupes_same_url_and_method(void) {
  HistoryService *service = history_service_new(10);

  HistoryEntry *original = make_entry(HTTP_POST, "https://a", "v1");
  history_service_record(service, original);
  history_service_record(service, make_entry(HTTP_GET, "https://b", NULL));
  g_assert_true(history_service_get(service, 0) != original);

  history_service_record(service, make_entry(HTTP_POST, "https://a", "v2"));

  g_assert_cmpuint(history_service_count(service), ==, 2);
  HistoryEntry *stored = history_service_get(service, 0);
  g_assert_true(stored == original);
  g_assert_cmpstr(stored->request.body, ==, "v2");

  g_object_unref(service);
}

static void test_record_does_not_dedupe_across_methods(void) {
  HistoryService *service = history_service_new(10);

  history_service_record(service, make_entry(HTTP_GET, "https://a", NULL));
  history_service_record(service, make_entry(HTTP_POST, "https://a", NULL));

  g_assert_cmpuint(history_service_count(service), ==, 2);

  g_object_unref(service);
}

static void test_record_same_pointer_promotes(void) {
  HistoryService *service = history_service_new(10);

  HistoryEntry *first = make_entry(HTTP_GET, "https://a", NULL);
  history_service_record(service, first);
  history_service_record(service, make_entry(HTTP_GET, "https://b", NULL));
  g_assert_true(history_service_get(service, 0) != first);

  history_service_record(service, first);

  g_assert_cmpuint(history_service_count(service), ==, 2);
  g_assert_true(history_service_get(service, 0) == first);

  g_object_unref(service);
}

static void test_remove_and_clear_update_count(void) {
  HistoryService *service = history_service_new(10);

  HistoryEntry *first = make_entry(HTTP_GET, "https://a", NULL);
  history_service_record(service, first);
  history_service_record(service, make_entry(HTTP_GET, "https://b", NULL));

  history_service_remove(service, first);
  g_assert_cmpuint(history_service_count(service), ==, 1);

  history_service_clear(service);
  g_assert_cmpuint(history_service_count(service), ==, 0);

  g_object_unref(service);
}

static void test_changed_signal_per_mutation(void) {
  HistoryService *service = history_service_new(10);
  guint changed = 0;
  g_signal_connect(service, "changed", G_CALLBACK(on_changed_count), &changed);

  HistoryEntry *entry = make_entry(HTTP_GET, "https://a", NULL);
  history_service_record(service, entry);
  g_assert_cmpuint(changed, ==, 1);

  history_service_remove(service, entry);
  g_assert_cmpuint(changed, ==, 2);

  history_service_clear(service);
  g_assert_cmpuint(changed, ==, 2);

  g_object_unref(service);
}

static void test_persists_on_finalize_and_reloads(void) {
  HistoryService *service = history_service_new(10);
  history_service_record(service, make_entry(HTTP_POST, "https://a", "body"));
  history_service_record(service, make_entry(HTTP_GET, "https://b", NULL));
  g_object_unref(service);

  HistoryService *reloaded = history_service_new(10);
  g_assert_cmpuint(history_service_count(reloaded), ==, 2);
  g_assert_cmpstr(history_service_get(reloaded, 0)->request.url, ==,
                  "https://b");
  g_assert_cmpstr(history_service_get(reloaded, 1)->request.url, ==,
                  "https://a");
  g_assert_cmpstr(history_service_get(reloaded, 1)->request.body, ==, "body");

  g_object_unref(reloaded);
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func("/history/service/record_prepends",
                  test_record_prepends_new_entries);
  g_test_add_func("/history/service/record_dedupes",
                  test_record_dedupes_same_url_and_method);
  g_test_add_func("/history/service/record_distinct_methods",
                  test_record_does_not_dedupe_across_methods);
  g_test_add_func("/history/service/record_same_pointer_promotes",
                  test_record_same_pointer_promotes);
  g_test_add_func("/history/service/remove_and_clear",
                  test_remove_and_clear_update_count);
  g_test_add_func("/history/service/changed_signal",
                  test_changed_signal_per_mutation);
  g_test_add_func("/history/service/persists_on_finalize",
                  test_persists_on_finalize_and_reloads);

  return g_test_run();
}
