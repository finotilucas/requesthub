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

#include "body_validator.h"
#include <cjson/cJSON.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <yaml.h>

static const char *last_error_msg = NULL;

gboolean body_validator_validate_json(const char *content,
                                      const char **error_msg) {
  cJSON *json;
  gboolean valid = FALSE;

  last_error_msg = NULL;

  if (!content || strlen(content) == 0) {
    return TRUE;
  }

  json = cJSON_Parse(content);

  if (json) {
    valid = TRUE;
    cJSON_Delete(json);
  } else {
    last_error_msg = "Invalid JSON syntax";
    const char *err = cJSON_GetErrorPtr();
    if (err != NULL) {
      last_error_msg = "JSON parse error";
    }
  }

  if (error_msg) {
    *error_msg = last_error_msg;
  }

  return valid;
}

gboolean body_validator_validate_xml(const char *content,
                                     const char **error_msg) {
  xmlDocPtr doc;
  gboolean valid = FALSE;

  last_error_msg = NULL;

  if (!content || strlen(content) == 0) {
    return TRUE;
  }

  xmlSetGenericErrorFunc(NULL, NULL);

  doc = xmlReadMemory(content, strlen(content), "noname.xml", NULL,
                      XML_PARSE_NOWARNING | XML_PARSE_NOERROR);

  if (doc) {
    valid = TRUE;
    xmlFreeDoc(doc);
  } else {
    last_error_msg = "Invalid XML syntax";
  }

  xmlCleanupParser();

  if (error_msg) {
    *error_msg = last_error_msg;
  }

  return valid;
}

gboolean body_validator_validate_yaml(const char *content,
                                      const char **error_msg) {
  yaml_parser_t parser;
  yaml_event_t event;
  gboolean valid = TRUE;

  last_error_msg = NULL;

  if (!content || strlen(content) == 0) {
    return TRUE;
  }

  if (!yaml_parser_initialize(&parser)) {
    last_error_msg = "Failed to initialize YAML parser";
    if (error_msg) {
      *error_msg = last_error_msg;
    }
    return FALSE;
  }

  yaml_parser_set_input_string(&parser, (const unsigned char *)content,
                               strlen(content));

  do {
    if (!yaml_parser_parse(&parser, &event)) {
      valid = FALSE;
      last_error_msg = "Invalid YAML syntax";
      yaml_event_delete(&event);
      break;
    }

    if (event.type == YAML_STREAM_END_EVENT) {
      yaml_event_delete(&event);
      break;
    }

    yaml_event_delete(&event);
  } while (1);

  yaml_parser_delete(&parser);

  if (error_msg) {
    *error_msg = last_error_msg;
  }

  return valid;
}

char *body_validator_format_json(const char *content) {
  cJSON *json;
  char *formatted = NULL;

  if (!content || strlen(content) == 0) {
    return NULL;
  }

  json = cJSON_Parse(content);
  if (!json) {
    return NULL;
  }

  formatted = cJSON_Print(json);
  cJSON_Delete(json);

  if (formatted) {
    char *result = g_strdup(formatted);
    free(formatted);
    return result;
  }

  return NULL;
}

char *body_validator_format_xml(const char *content) {
  xmlDocPtr doc;
  xmlChar *formatted = NULL;
  int size = 0;
  char *result = NULL;

  if (!content || strlen(content) == 0) {
    return NULL;
  }

  xmlSetGenericErrorFunc(NULL, NULL);

  doc = xmlReadMemory(content, strlen(content), "noname.xml", NULL,
                      XML_PARSE_NOWARNING | XML_PARSE_NOERROR);

  if (!doc) {
    return NULL;
  }

  xmlDocDumpFormatMemory(doc, &formatted, &size, 1);

  if (formatted) {
    result = g_strdup((char *)formatted);
    xmlFree(formatted);
  }

  xmlFreeDoc(doc);
  xmlCleanupParser();

  return result;
}
