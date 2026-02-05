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

#ifndef BODY_VALIDATOR_H
#define BODY_VALIDATOR_H

#include <glib.h>

gboolean body_validator_validate_json(const char *content,
                                      const char **error_msg);

gboolean body_validator_validate_xml(const char *content,
                                     const char **error_msg);

gboolean body_validator_validate_yaml(const char *content,
                                      const char **error_msg);

char *body_validator_format_json(const char *content);

char *body_validator_format_xml(const char *content);

#endif
