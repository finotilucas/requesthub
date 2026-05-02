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

#include "format_time.h"

gchar *format_relative_time(gint64 timestamp_ms) {
  if (timestamp_ms <= 0) {
    return g_strdup("");
  }

  gint64 now_ms = g_get_real_time() / 1000;
  gint64 diff_s = (now_ms - timestamp_ms) / 1000;
  if (diff_s < 0) {
    diff_s = 0;
  }

  if (diff_s < 60) {
    return g_strdup_printf("%lds ago", (long)diff_s);
  }
  if (diff_s < 3600) {
    return g_strdup_printf("%ldm ago", (long)(diff_s / 60));
  }
  if (diff_s < 86400) {
    return g_strdup_printf("%ldh ago", (long)(diff_s / 3600));
  }
  return g_strdup_printf("%ldd ago", (long)(diff_s / 86400));
}
