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

#include "../src/http/http_pool.h"

#include <curl/curl.h>
#include <glib.h>

static void test_init_is_idempotent(void) {
  http_pool_init();
  http_pool_init();
  http_pool_cleanup();
}

static void test_acquire_without_init_initializes_pool(void) {
  CURL *handle = http_pool_acquire();
  g_assert_nonnull(handle);
  http_pool_release(handle);
  http_pool_cleanup();
}

static void test_acquire_distinct_when_in_use(void) {
  http_pool_init();
  CURL *a = http_pool_acquire();
  CURL *b = http_pool_acquire();
  CURL *c = http_pool_acquire();
  g_assert_nonnull(a);
  g_assert_nonnull(b);
  g_assert_nonnull(c);
  g_assert_true(a != b);
  g_assert_true(b != c);
  g_assert_true(a != c);
  http_pool_release(a);
  http_pool_release(b);
  http_pool_release(c);
  http_pool_cleanup();
}

static void test_release_then_acquire_reuses_handle(void) {
  http_pool_init();
  CURL *first = http_pool_acquire();
  http_pool_release(first);
  CURL *second = http_pool_acquire();
  g_assert_true(first == second);
  http_pool_release(second);
  http_pool_cleanup();
}

static void test_release_null_is_safe(void) {
  http_pool_init();
  http_pool_release(NULL);
  http_pool_cleanup();
}

static void test_release_unknown_handle_is_freed(void) {
  http_pool_init();
  CURL *outside = curl_easy_init();
  g_assert_nonnull(outside);
  http_pool_release(outside);
  http_pool_cleanup();
}

static void test_acquire_release_many_handles(void) {
  http_pool_init();
  enum { COUNT = 16 };
  CURL *handles[COUNT];

  for (int i = 0; i < COUNT; i++) {
    handles[i] = http_pool_acquire();
    g_assert_nonnull(handles[i]);
    for (int j = 0; j < i; j++) {
      g_assert_true(handles[i] != handles[j]);
    }
  }

  for (int i = 0; i < COUNT; i++) {
    http_pool_release(handles[i]);
  }

  CURL *reused = http_pool_acquire();
  g_assert_nonnull(reused);
  gboolean is_known = FALSE;
  for (int i = 0; i < COUNT; i++) {
    if (handles[i] == reused) {
      is_known = TRUE;
      break;
    }
  }
  g_assert_true(is_known);
  http_pool_release(reused);

  http_pool_cleanup();
}

static void test_cleanup_then_reinit_reallocates(void) {
  http_pool_init();
  CURL *a = http_pool_acquire();
  http_pool_release(a);
  http_pool_cleanup();

  http_pool_init();
  CURL *b = http_pool_acquire();
  g_assert_nonnull(b);
  http_pool_release(b);
  http_pool_cleanup();
}

int main(int argc, char **argv) {
  g_test_init(&argc, &argv, NULL);
  curl_global_init(CURL_GLOBAL_DEFAULT);

  g_test_add_func("/http/pool/init/idempotent", test_init_is_idempotent);
  g_test_add_func("/http/pool/acquire/auto_init",
                  test_acquire_without_init_initializes_pool);
  g_test_add_func("/http/pool/acquire/distinct_in_use",
                  test_acquire_distinct_when_in_use);
  g_test_add_func("/http/pool/acquire/reuses_after_release",
                  test_release_then_acquire_reuses_handle);
  g_test_add_func("/http/pool/release/null_safe", test_release_null_is_safe);
  g_test_add_func("/http/pool/release/unknown_handle_freed",
                  test_release_unknown_handle_is_freed);
  g_test_add_func("/http/pool/acquire/many_handles",
                  test_acquire_release_many_handles);
  g_test_add_func("/http/pool/cleanup/reinit_works",
                  test_cleanup_then_reinit_reallocates);

  int ret = g_test_run();
  curl_global_cleanup();
  return ret;
}
