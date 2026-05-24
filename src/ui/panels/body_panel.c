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

#include "body_panel.h"
#include "../../utils/body_validator.h"
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

struct _BodyPanel {
  GtkBox parent_instance;

  GtkDropDown *type_dropdown;
  GtkSourceView *source_view;
  GtkSourceBuffer *source_buffer;
  GtkWidget *status_label;

  BodyContentType current_type;
  GtkTextTag *error_tag;
  gboolean error_tag_created;
  gboolean is_valid;
};

G_DEFINE_TYPE(BodyPanel, body_panel, GTK_TYPE_BOX)

static void force_highlight_immediate(GtkSourceBuffer *buffer) {
  GtkTextIter start, end;

  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
  gtk_text_buffer_insert(GTK_TEXT_BUFFER(buffer), &end, " ", 1);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
  gtk_text_buffer_get_iter_at_offset(GTK_TEXT_BUFFER(buffer), &start,
                                     gtk_text_iter_get_offset(&end) - 1);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(buffer), &start, &end);

  gtk_source_buffer_ensure_highlight(buffer, &start, &end);

  g_signal_emit_by_name(buffer, "changed");
}

static void update_validation_status(BodyPanel *self, gboolean is_valid,
                                     const char *message) {
  self->is_valid = is_valid;

  if (self->status_label) {
    if (is_valid) {
      gtk_label_set_markup(GTK_LABEL(self->status_label),
                           "<span foreground='#26a269'>✓ Valid</span>");
    } else {
      char markup[512];
      snprintf(markup, sizeof(markup), "<span foreground='#c01c28'>✗ %s</span>",
               message ? message : "Invalid syntax");
      gtk_label_set_markup(GTK_LABEL(self->status_label), markup);
    }
  }
}

static void validate_current_content(BodyPanel *self) {
  GtkTextIter start, end;
  char *text;
  gboolean is_valid = TRUE;
  const char *error_msg = NULL;

  gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->source_buffer), &start,
                             &end);
  text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(self->source_buffer), &start,
                                  &end, FALSE);

  if (self->error_tag_created) {
    gtk_text_buffer_remove_tag(GTK_TEXT_BUFFER(self->source_buffer),
                               self->error_tag, &start, &end);
  }

  if (text && strlen(text) > 0) {
    switch (self->current_type) {
    case BODY_TYPE_JSON:
      is_valid = body_validator_validate_json(text, &error_msg);
      break;
    case BODY_TYPE_XML:
      is_valid = body_validator_validate_xml(text, &error_msg);
      break;
    case BODY_TYPE_YAML:
      is_valid = body_validator_validate_yaml(text, &error_msg);
      break;
    case BODY_TYPE_TEXT:
      is_valid = TRUE;
      break;
    }

    if (!is_valid && self->error_tag_created) {
      gtk_text_buffer_apply_tag(GTK_TEXT_BUFFER(self->source_buffer),
                                self->error_tag, &start, &end);
    }
  } else {
    is_valid = TRUE;
  }

  update_validation_status(self, is_valid, error_msg);

  g_free(text);
}

static void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
  (void)buffer;
  BodyPanel *self = BODY_PANEL(user_data);

  if (self->current_type != BODY_TYPE_TEXT) {
    validate_current_content(self);
  }
}

static void on_type_changed(GtkDropDown *dropdown, GParamSpec *pspec,
                            gpointer user_data) {
  (void)pspec;
  BodyPanel *self = BODY_PANEL(user_data);
  guint selected = gtk_drop_down_get_selected(dropdown);

  body_panel_set_content_type(self, (BodyContentType)selected);
}

static void on_clear_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;
  body_panel_clear(BODY_PANEL(user_data));
}

static void on_format_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;
  BodyPanel *self = BODY_PANEL(user_data);

  char *content = body_panel_get_content(self);
  if (!content || strlen(content) == 0) {
    g_free(content);
    return;
  }

  char *formatted = NULL;

  switch (self->current_type) {
  case BODY_TYPE_JSON:
    formatted = body_validator_format_json(content);
    break;
  case BODY_TYPE_XML:
    formatted = body_validator_format_xml(content);
    break;
  case BODY_TYPE_YAML:
    break;
  case BODY_TYPE_TEXT:
    break;
  }

  if (formatted) {
    body_panel_set_content(self, formatted);
    g_free(formatted);
  }

  g_free(content);
}

static void body_panel_class_init(BodyPanelClass *klass) { (void)klass; }

static void body_panel_init(BodyPanel *self) {
  GtkSourceLanguageManager *lang_manager;
  GtkSourceStyleSchemeManager *style_manager;
  GtkSourceStyleScheme *scheme;
  GdkRGBA red_color = {1.0, 0.0, 0.0, 1.0};

  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  gtk_widget_set_margin_start(GTK_WIDGET(self), 16);
  gtk_widget_set_margin_end(GTK_WIDGET(self), 16);
  gtk_widget_set_margin_top(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_bottom(GTK_WIDGET(self), 10);

  GtkWidget *label = gtk_label_new("Request Body");
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_widget_set_margin_top(label, 10);
  gtk_widget_set_margin_bottom(label, 10);
  gtk_widget_add_css_class(label, "title-4");
  gtk_box_append(GTK_BOX(self), label);

  GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_bottom(header, 10);

  const char *types[] = {"JSON", "XML", "YAML", "TEXT", NULL};
  GtkStringList *string_list = gtk_string_list_new(types);
  self->type_dropdown =
      GTK_DROP_DOWN(gtk_drop_down_new(G_LIST_MODEL(string_list), NULL));
  gtk_drop_down_set_selected(self->type_dropdown, 0);
  self->current_type = BODY_TYPE_JSON;

  gtk_box_append(GTK_BOX(header), GTK_WIDGET(self->type_dropdown));

  GtkWidget *format_btn = gtk_button_new_with_label("Format");
  gtk_widget_add_css_class(format_btn, "flat");
  gtk_box_append(GTK_BOX(header), format_btn);

  GtkWidget *clear_btn = gtk_button_new_with_label("Clear");
  gtk_widget_add_css_class(clear_btn, "flat");
  gtk_widget_add_css_class(clear_btn, "error");
  gtk_box_append(GTK_BOX(header), clear_btn);

  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(header), spacer);

  self->status_label = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(self->status_label), 1.0);
  gtk_box_append(GTK_BOX(header), self->status_label);

  gtk_box_append(GTK_BOX(self), header);

  self->source_buffer = gtk_source_buffer_new(NULL);

  lang_manager = gtk_source_language_manager_get_default();
  GtkSourceLanguage *language =
      gtk_source_language_manager_get_language(lang_manager, "json");
  gtk_source_buffer_set_language(self->source_buffer, language);

  style_manager = gtk_source_style_scheme_manager_get_default();
  scheme = gtk_source_style_scheme_manager_get_scheme(style_manager, "Adwaita-dark");
  if (scheme) {
    gtk_source_buffer_set_style_scheme(self->source_buffer, scheme);
  }

  self->error_tag = gtk_text_buffer_create_tag(
      GTK_TEXT_BUFFER(self->source_buffer), "syntax-error", "underline", 4,
      "underline-rgba", &red_color, NULL);
  self->error_tag_created = TRUE;

  self->source_view =
      GTK_SOURCE_VIEW(gtk_source_view_new_with_buffer(self->source_buffer));

  gtk_source_view_set_show_line_numbers(self->source_view, TRUE);
  gtk_source_view_set_highlight_current_line(self->source_view, TRUE);
  gtk_source_view_set_show_right_margin(self->source_view, TRUE);
  gtk_source_view_set_right_margin_position(self->source_view, 100);
  gtk_source_view_set_auto_indent(self->source_view, TRUE);
  gtk_source_view_set_indent_on_tab(self->source_view, TRUE);
  gtk_source_view_set_tab_width(self->source_view, 4);
  gtk_source_view_set_insert_spaces_instead_of_tabs(self->source_view, TRUE);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(self->source_view));
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_widget_add_css_class(scrolled, "body-editor");

  gtk_box_append(GTK_BOX(self), scrolled);

  g_signal_connect(self->type_dropdown, "notify::selected",
                   G_CALLBACK(on_type_changed), self);
  g_signal_connect(self->source_buffer, "changed",
                   G_CALLBACK(on_buffer_changed), self);
  g_signal_connect(format_btn, "clicked", G_CALLBACK(on_format_clicked), self);
  g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_clicked), self);

  force_highlight_immediate(self->source_buffer);

  self->is_valid = TRUE;
  update_validation_status(self, TRUE, NULL);
}

BodyPanel *body_panel_new(void) { return g_object_new(TYPE_BODY_PANEL, NULL); }

void body_panel_apply_to_request(BodyPanel *self, HttpRequest *request) {
  g_return_if_fail(BODY_IS_PANEL(self));
  g_return_if_fail(request != NULL);

  char *content = body_panel_get_content(self);

  if (content && strlen(content) > 0 && self->is_valid) {
    http_request_set_body(request, content);

    const char *content_type_value = NULL;
    switch (self->current_type) {
    case BODY_TYPE_JSON:
      content_type_value = "application/json";
      break;
    case BODY_TYPE_XML:
      content_type_value = "application/xml";
      break;
    case BODY_TYPE_YAML:
      content_type_value = "application/x-yaml";
      break;
    case BODY_TYPE_TEXT:
      content_type_value = "text/plain";
      break;
    }

    if (content_type_value) {
      gboolean has_content_type = FALSE;
      guint header_count = http_request_headers_count(request);
      for (guint i = 0; i < header_count; i++) {
        if (g_ascii_strcasecmp(http_request_header_key(request, i),
                               "Content-Type") == 0) {
          has_content_type = TRUE;
          break;
        }
      }

      if (!has_content_type) {
        http_request_add_header(request, "Content-Type", content_type_value);
      }
    }
  } else if (content && strlen(content) == 0) {
    http_request_set_body(request, NULL);
  }

  g_free(content);
}

void body_panel_clear(BodyPanel *self) {
  g_return_if_fail(BODY_IS_PANEL(self));

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->source_buffer), &start,
                             &end);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(self->source_buffer), &start, &end);

  update_validation_status(self, TRUE, NULL);
}

void body_panel_set_content_type(BodyPanel *self, BodyContentType type) {
  g_return_if_fail(BODY_IS_PANEL(self));

  GtkSourceLanguageManager *lang_manager;
  GtkSourceLanguage *language = NULL;
  const char *lang_id = NULL;
  GtkTextIter start, end;

  self->current_type = type;

  switch (type) {
  case BODY_TYPE_JSON:
    lang_id = "json";
    break;
  case BODY_TYPE_XML:
    lang_id = "xml";
    break;
  case BODY_TYPE_YAML:
    lang_id = "yaml";
    break;
  case BODY_TYPE_TEXT:
    lang_id = NULL;
    break;
  }

  lang_manager = gtk_source_language_manager_get_default();
  if (lang_id) {
    language = gtk_source_language_manager_get_language(lang_manager, lang_id);
  }
  gtk_source_buffer_set_language(self->source_buffer, language);

  if (self->error_tag_created) {
    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->source_buffer), &start,
                               &end);
    gtk_text_buffer_remove_tag(GTK_TEXT_BUFFER(self->source_buffer),
                               self->error_tag, &start, &end);
  }

  force_highlight_immediate(self->source_buffer);

  if (type != BODY_TYPE_TEXT) {
    validate_current_content(self);
  } else {
    update_validation_status(self, TRUE, NULL);
  }

  if (gtk_drop_down_get_selected(self->type_dropdown) != (guint)type) {
    gtk_drop_down_set_selected(self->type_dropdown, (guint)type);
  }
}

BodyContentType body_panel_get_content_type(BodyPanel *self) {
  g_return_val_if_fail(BODY_IS_PANEL(self), BODY_TYPE_TEXT);
  return self->current_type;
}

void body_panel_set_content(BodyPanel *self, const char *content) {
  g_return_if_fail(BODY_IS_PANEL(self));

  GtkTextIter start, end;

  gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->source_buffer), &start,
                             &end);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(self->source_buffer), &start, &end);

  if (content && strlen(content) > 0) {
    gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(self->source_buffer),
                                   &start);
    gtk_text_buffer_insert(GTK_TEXT_BUFFER(self->source_buffer), &start,
                           content, -1);
  }

  validate_current_content(self);
}

char *body_panel_get_content(BodyPanel *self) {
  g_return_val_if_fail(BODY_IS_PANEL(self), NULL);

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->source_buffer), &start,
                             &end);
  return gtk_text_buffer_get_text(GTK_TEXT_BUFFER(self->source_buffer), &start,
                                  &end, FALSE);
}

gboolean body_panel_is_valid(BodyPanel *self) {
  g_return_val_if_fail(BODY_IS_PANEL(self), FALSE);
  return self->is_valid;
}
