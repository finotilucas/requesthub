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

#include "format.h"

#include <stdio.h>

char *format_size(size_t bytes) {
  const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  int i = 0;
  double size = (double)bytes;

  while (size >= 1024 && i < 4) {
    size /= 1024;
    i++;
  }

  if (i == 0)
    return g_strdup_printf("%zu %s", bytes, units[i]);
  return g_strdup_printf("%.2f %s", size, units[i]);
}

char *format_time(double milliseconds) {
  if (milliseconds >= 1000.0) {
    return g_strdup_printf("%.2f s", milliseconds / 1000.0);
  }
  return g_strdup_printf("%.0f ms", milliseconds);
}
