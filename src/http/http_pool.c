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

#include "http_pool.h"

#include <glib.h>
#include <time.h>

#define MAX_POOL_SIZE 16
#define CONNECTION_TIMEOUT_SECONDS 60

typedef struct {
  CURL *handle;
  time_t last_used;
  int in_use;
} PooledConnection;

typedef struct {
  PooledConnection connections[MAX_POOL_SIZE];
  int initialized;
} ConnectionPool;

static ConnectionPool global_pool = {0};
static GMutex pool_lock;
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

static void reset_request_options(CURL *handle) {
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

  g_mutex_lock(&pool_lock);

  for (int i = 0; i < MAX_POOL_SIZE; i++) {
    if (global_pool.connections[i].handle) {
      curl_easy_cleanup(global_pool.connections[i].handle);
      global_pool.connections[i].handle = NULL;
    }
  }

  g_mutex_unlock(&pool_lock);

  cleanup_curlshare();
  global_pool.initialized = 0;
}

/* Both require pool_lock. */
static CURL *reuse_idle_handle(time_t now) {
  for (int i = 0; i < MAX_POOL_SIZE; i++) {
    PooledConnection *slot = &global_pool.connections[i];
    if (slot->handle == NULL || slot->in_use) {
      continue;
    }
    if (now - slot->last_used >= CONNECTION_TIMEOUT_SECONDS) {
      curl_easy_cleanup(slot->handle);
      slot->handle = NULL;
      continue;
    }
    slot->in_use = 1;
    slot->last_used = now;
    return slot->handle;
  }
  return NULL;
}

static CURL *create_pooled_handle(time_t now) {
  for (int i = 0; i < MAX_POOL_SIZE; i++) {
    PooledConnection *slot = &global_pool.connections[i];
    if (slot->handle != NULL) {
      continue;
    }
    CURL *handle = curl_easy_init();
    if (handle != NULL) {
      slot->handle = handle;
      slot->in_use = 1;
      slot->last_used = now;
      if (global_share) {
        curl_easy_setopt(handle, CURLOPT_SHARE, global_share);
      }
    }
    return handle;
  }
  return NULL;
}

CURL *http_pool_acquire(void) {
  if (!global_pool.initialized) {
    http_pool_init();
  }

  time_t now = time(NULL);

  g_mutex_lock(&pool_lock);
  CURL *idle = reuse_idle_handle(now);
  if (idle != NULL) {
    g_mutex_unlock(&pool_lock);
    reset_request_options(idle);
    return idle;
  }
  CURL *created = create_pooled_handle(now);
  g_mutex_unlock(&pool_lock);
  if (created != NULL) {
    return created;
  }

  /* Pool full: untracked handle, destroyed on release. */
  CURL *unpooled_handle = curl_easy_init();
  if (unpooled_handle && global_share) {
    curl_easy_setopt(unpooled_handle, CURLOPT_SHARE, global_share);
  }
  return unpooled_handle;
}

void http_pool_release(CURL *handle) {
  if (!handle || !global_pool.initialized) {
    if (handle)
      curl_easy_cleanup(handle);
    return;
  }

  g_mutex_lock(&pool_lock);

  for (int i = 0; i < MAX_POOL_SIZE; i++) {
    if (global_pool.connections[i].handle == handle) {
      global_pool.connections[i].in_use = 0;
      global_pool.connections[i].last_used = time(NULL);
      g_mutex_unlock(&pool_lock);
      return;
    }
  }

  g_mutex_unlock(&pool_lock);

  curl_easy_cleanup(handle);
}
