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

#ifndef HTTP_POOL_H
#define HTTP_POOL_H

#include <curl/curl.h>
#include <pthread.h>
#include <time.h>

#define MAX_POOL_SIZE 100
#define CONNECTION_TIMEOUT_SECONDS 60

typedef struct {
  CURL *handle;
  time_t last_used;
  int in_use;
} PooledConnection;

typedef struct {
  PooledConnection connections[MAX_POOL_SIZE];
  pthread_mutex_t lock;
  int initialized;
} ConnectionPool;

void http_pool_init(void);
void http_pool_cleanup(void);
CURL *http_pool_acquire(void);
void http_pool_release(CURL *handle);

#endif
