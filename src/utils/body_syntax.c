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

#include "body_syntax.h"

#include <cjson/cJSON.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <stdlib.h>
#include <yaml.h>

gboolean body_syntax_validate_json(const char *content,
                                   const char **error_msg) {
  if (error_msg != NULL) {
    *error_msg = NULL;
  }
  if (content == NULL || *content == '\0') {
    return TRUE;
  }

  cJSON *json = cJSON_Parse(content);
  if (json == NULL) {
    if (error_msg != NULL) {
      *error_msg = "Invalid JSON syntax";
    }
    return FALSE;
  }

  cJSON_Delete(json);
  return TRUE;
}

gboolean body_syntax_validate_xml(const char *content,
                                  const char **error_msg) {
  if (error_msg != NULL) {
    *error_msg = NULL;
  }
  if (content == NULL || *content == '\0') {
    return TRUE;
  }

  xmlDocPtr doc = xmlReadMemory(content, (int)strlen(content), "noname.xml",
                                NULL, XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
  if (doc == NULL) {
    if (error_msg != NULL) {
      *error_msg = "Invalid XML syntax";
    }
    return FALSE;
  }

  xmlFreeDoc(doc);
  return TRUE;
}

gboolean body_syntax_validate_yaml(const char *content,
                                   const char **error_msg) {
  if (error_msg != NULL) {
    *error_msg = NULL;
  }
  if (content == NULL || *content == '\0') {
    return TRUE;
  }

  yaml_parser_t parser;
  if (!yaml_parser_initialize(&parser)) {
    if (error_msg != NULL) {
      *error_msg = "Failed to initialize YAML parser";
    }
    return FALSE;
  }

  yaml_parser_set_input_string(&parser, (const unsigned char *)content,
                               strlen(content));

  gboolean valid = TRUE;
  yaml_event_t event;
  do {
    if (!yaml_parser_parse(&parser, &event)) {
      valid = FALSE;
      if (error_msg != NULL) {
        *error_msg = "Invalid YAML syntax";
      }
      break;
    }
    gboolean at_stream_end = event.type == YAML_STREAM_END_EVENT;
    yaml_event_delete(&event);
    if (at_stream_end) {
      break;
    }
  } while (1);

  yaml_parser_delete(&parser);
  return valid;
}

/* cJSON/libxml2 alocam com malloc; copiamos para o alocador GLib porque os
 * chamadores liberam com g_free. */
static char *steal_to_glib(char *malloced) {
  if (malloced == NULL) {
    return NULL;
  }
  char *result = g_strdup(malloced);
  free(malloced);
  return result;
}

char *body_syntax_format_json(const char *content) {
  if (content == NULL || *content == '\0') {
    return NULL;
  }

  cJSON *json = cJSON_Parse(content);
  if (json == NULL) {
    return NULL;
  }

  char *formatted = cJSON_Print(json);
  cJSON_Delete(json);
  return steal_to_glib(formatted);
}

char *body_syntax_format_xml(const char *content) {
  if (content == NULL || *content == '\0') {
    return NULL;
  }

  xmlDocPtr doc = xmlReadMemory(content, (int)strlen(content), "noname.xml",
                                NULL, XML_PARSE_NOWARNING | XML_PARSE_NOERROR);
  if (doc == NULL) {
    return NULL;
  }

  xmlChar *formatted = NULL;
  int size = 0;
  xmlDocDumpFormatMemory(doc, &formatted, &size, 1);
  xmlFreeDoc(doc);

  if (formatted == NULL) {
    return NULL;
  }
  char *result = g_strdup((char *)formatted);
  xmlFree(formatted);
  return result;
}
