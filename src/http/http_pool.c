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

#include "http_pool.h"

static ConnectionPool global_pool = {0};
static CURLSH *global_share = NULL;

static void init_curlshare(void) {
  if (global_share)
    return;

  global_share = curl_share_init();
  if (!global_share)
    return;

  curl_share_setopt(global_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);

  curl_share_setopt(global_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_SSL_SESSION);

  curl_share_setopt(global_share, CURLSHOPT_SHARE, CURL_LOCK_DATA_CONNECT);
}

static void cleanup_curlshare(void) {
  if (global_share) {
    curl_share_cleanup(global_share);
    global_share = NULL;
  }
}

static void selective_cleanup(CURL *handle) {
  if (!handle)
    return;

  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, NULL);
  curl_easy_setopt(handle, CURLOPT_POSTFIELDS, NULL);
  curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, 0L);
  curl_easy_setopt(handle, CURLOPT_POST, 0L);
  curl_easy_setopt(handle, CURLOPT_NOBODY, 0L);
  curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, NULL);
}

void http_pool_init(void) {
  if (global_pool.initialized)
    return;

  pthread_mutex_init(&global_pool.lock, NULL);

  for (int i = 0; i < MAX_POOL_SIZE; i++) {
    global_pool.connections[i].handle = NULL;
    global_pool.connections[i].last_used = 0;
    global_pool.connections[i].in_use = 0;
  }

  init_curlshare();
  global_pool.initialized = 1;
}

void http_pool_cleanup(void) {
  if (!global_pool.initialized)
    return;

  pthread_mutex_lock(&global_pool.lock);

  for (int i = 0; i < MAX_POOL_SIZE; i++) {
    if (global_pool.connections[i].handle) {
      curl_easy_cleanup(global_pool.connections[i].handle);
      global_pool.connections[i].handle = NULL;
    }
  }

  pthread_mutex_unlock(&global_pool.lock);
  pthread_mutex_destroy(&global_pool.lock);

  cleanup_curlshare();
  global_pool.initialized = 0;
}

CURL *http_pool_acquire(void) {
  if (!global_pool.initialized) {
    http_pool_init();
  }

  pthread_mutex_lock(&global_pool.lock);

  time_t now = time(NULL);

  for (int i = 0; i < MAX_POOL_SIZE; i++) {
    if (global_pool.connections[i].handle &&
        !global_pool.connections[i].in_use) {

      if ((now - global_pool.connections[i].last_used) <
          CONNECTION_TIMEOUT_SECONDS) {

        global_pool.connections[i].in_use = 1;
        global_pool.connections[i].last_used = now;

        CURL *handle = global_pool.connections[i].handle;
        pthread_mutex_unlock(&global_pool.lock);

        selective_cleanup(handle);

        return handle;
      } else {
        curl_easy_cleanup(global_pool.connections[i].handle);
        global_pool.connections[i].handle = NULL;
      }
    }
  }

  for (int i = 0; i < MAX_POOL_SIZE; i++) {
    if (!global_pool.connections[i].handle) {
      CURL *handle = curl_easy_init();
      if (handle) {
        global_pool.connections[i].handle = handle;
        global_pool.connections[i].in_use = 1;
        global_pool.connections[i].last_used = now;

        if (global_share) {
          curl_easy_setopt(handle, CURLOPT_SHARE, global_share);
        }
      }

      pthread_mutex_unlock(&global_pool.lock);
      return handle;
    }
  }

  pthread_mutex_unlock(&global_pool.lock);

  CURL *temp_handle = curl_easy_init();
  if (temp_handle && global_share) {
    curl_easy_setopt(temp_handle, CURLOPT_SHARE, global_share);
  }

  return temp_handle;
}

void http_pool_release(CURL *handle) {
  if (!handle || !global_pool.initialized) {
    if (handle)
      curl_easy_cleanup(handle);
    return;
  }

  pthread_mutex_lock(&global_pool.lock);

  for (int i = 0; i < MAX_POOL_SIZE; i++) {
    if (global_pool.connections[i].handle == handle) {
      global_pool.connections[i].in_use = 0;
      global_pool.connections[i].last_used = time(NULL);
      pthread_mutex_unlock(&global_pool.lock);
      return;
    }
  }

  pthread_mutex_unlock(&global_pool.lock);

  curl_easy_cleanup(handle);
}
