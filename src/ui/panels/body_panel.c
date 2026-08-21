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

#include "body_panel.h"
#include "../../utils/body_syntax.h"
#include <gtk/gtk.h>
#include <gtksourceview/gtksource.h>

#define VALIDATE_DEBOUNCE_MS 300

typedef enum {
  BODY_TYPE_JSON = 0,
  BODY_TYPE_XML,
  BODY_TYPE_YAML,
  BODY_TYPE_TEXT,
  BODY_TYPE_COUNT
} BodyContentType;

typedef struct {
  const char *label;
  const char *language_id;
  const char *mime_type;
  gboolean (*validate)(const char *content, const char **error_msg);
  char *(*format)(const char *content);
} BodyTypeSpec;

static const BodyTypeSpec BODY_TYPES[BODY_TYPE_COUNT] = {
    [BODY_TYPE_JSON] = {"JSON", "json", "application/json",
                        body_syntax_validate_json, body_syntax_format_json},
    [BODY_TYPE_XML] = {"XML", "xml", "application/xml",
                       body_syntax_validate_xml, body_syntax_format_xml},
    [BODY_TYPE_YAML] = {"YAML", "yaml", "application/x-yaml",
                        body_syntax_validate_yaml, NULL},
    [BODY_TYPE_TEXT] = {"TEXT", NULL, "text/plain", NULL, NULL},
};

static void body_panel_set_content_type(BodyPanel *self, BodyContentType type);

struct _BodyPanel {
  GtkBox parent_instance;

  GtkDropDown *type_dropdown;
  GtkSourceView *source_view;
  GtkSourceBuffer *source_buffer;
  GtkWidget *status_label;

  BodyContentType current_type;
  GtkTextTag *error_tag;
  guint validate_source_id;
  gboolean is_valid;
};

G_DEFINE_FINAL_TYPE(BodyPanel, body_panel, GTK_TYPE_BOX)

/* Workaround GtkSourceView: trocar a linguagem nao re-highlighta o conteudo
 * ja presente no buffer; um insert+delete no-op forca o refresh. A acao
 * irreversivel evita poluir o undo do usuario com a edicao fantasma. */
static void refresh_highlight(GtkSourceBuffer *buffer) {
  GtkTextIter start, end;

  gtk_text_buffer_begin_irreversible_action(GTK_TEXT_BUFFER(buffer));
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
  gtk_text_buffer_insert(GTK_TEXT_BUFFER(buffer), &end, " ", 1);
  gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
  start = end;
  gtk_text_iter_backward_char(&start);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(buffer), &start, &end);
  gtk_text_buffer_end_irreversible_action(GTK_TEXT_BUFFER(buffer));
}

static void update_validation_status(BodyPanel *self, gboolean is_valid,
                                     const char *message) {
  self->is_valid = is_valid;

  if (self->status_label == NULL) {
    return;
  }

  if (is_valid) {
    gtk_label_set_text(GTK_LABEL(self->status_label), "✓ Valid");
  } else {
    gchar *text =
        g_strdup_printf("✗ %s", message != NULL ? message : "Invalid syntax");
    gtk_label_set_text(GTK_LABEL(self->status_label), text);
    g_free(text);
  }

  gtk_widget_remove_css_class(self->status_label, is_valid
                                                      ? "validation-error"
                                                      : "validation-success");
  gtk_widget_add_css_class(self->status_label, is_valid ? "validation-success"
                                                        : "validation-error");
}

static void validate_current_content(BodyPanel *self) {
  GtkTextIter start, end;
  gboolean is_valid = TRUE;
  const char *error_msg = NULL;

  gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->source_buffer), &start,
                             &end);
  char *text = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(self->source_buffer),
                                        &start, &end, FALSE);

  gtk_text_buffer_remove_tag(GTK_TEXT_BUFFER(self->source_buffer),
                             self->error_tag, &start, &end);

  const BodyTypeSpec *spec = &BODY_TYPES[self->current_type];
  if (spec->validate != NULL && text != NULL && *text != '\0') {
    is_valid = spec->validate(text, &error_msg);
    if (!is_valid) {
      gtk_text_buffer_apply_tag(GTK_TEXT_BUFFER(self->source_buffer),
                                self->error_tag, &start, &end);
    }
  }

  update_validation_status(self, is_valid, error_msg);

  g_free(text);
}

static void cancel_pending_validation(BodyPanel *self) {
  if (self->validate_source_id != 0) {
    g_source_remove(self->validate_source_id);
    self->validate_source_id = 0;
  }
}

static gboolean on_validate_timeout(gpointer user_data) {
  BodyPanel *self = BODY_PANEL(user_data);
  self->validate_source_id = 0;
  validate_current_content(self);
  return G_SOURCE_REMOVE;
}

/* Validar a cada tecla copia e parseia o buffer inteiro; o debounce limita o
 * custo a uma validacao por pausa de digitacao. */
static void on_buffer_changed(GtkTextBuffer *buffer, gpointer user_data) {
  (void)buffer;
  BodyPanel *self = BODY_PANEL(user_data);

  if (self->current_type == BODY_TYPE_TEXT) {
    return;
  }
  cancel_pending_validation(self);
  self->validate_source_id =
      g_timeout_add(VALIDATE_DEBOUNCE_MS, on_validate_timeout, self);
}

static void on_type_changed(GtkDropDown *dropdown, GParamSpec *pspec,
                            gpointer user_data) {
  (void)pspec;
  BodyPanel *self = BODY_PANEL(user_data);
  guint selected = gtk_drop_down_get_selected(dropdown);

  if (selected < BODY_TYPE_COUNT) {
    body_panel_set_content_type(self, (BodyContentType)selected);
  }
}

static void on_clear_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;
  body_panel_clear(BODY_PANEL(user_data));
}

static void on_format_clicked(GtkButton *btn, gpointer user_data) {
  (void)btn;
  BodyPanel *self = BODY_PANEL(user_data);

  const BodyTypeSpec *spec = &BODY_TYPES[self->current_type];
  if (spec->format == NULL) {
    return;
  }

  char *content = body_panel_get_content(self);
  char *formatted = spec->format(content);
  if (formatted != NULL) {
    body_panel_set_content(self, formatted);
    g_free(formatted);
  }
  g_free(content);
}

static GtkWidget *build_toolbar(BodyPanel *self) {
  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_margin_bottom(toolbar, 10);

  const char *labels[BODY_TYPE_COUNT + 1] = {0};
  for (guint i = 0; i < BODY_TYPE_COUNT; i++) {
    labels[i] = BODY_TYPES[i].label;
  }
  GtkStringList *string_list = gtk_string_list_new(labels);
  self->type_dropdown =
      GTK_DROP_DOWN(gtk_drop_down_new(G_LIST_MODEL(string_list), NULL));
  gtk_drop_down_set_selected(self->type_dropdown, BODY_TYPE_JSON);
  gtk_box_append(GTK_BOX(toolbar), GTK_WIDGET(self->type_dropdown));

  GtkWidget *format_btn = gtk_button_new_with_label("Format");
  gtk_widget_add_css_class(format_btn, "flat");
  gtk_box_append(GTK_BOX(toolbar), format_btn);

  GtkWidget *clear_btn = gtk_button_new_with_label("Clear");
  gtk_widget_add_css_class(clear_btn, "flat");
  gtk_widget_add_css_class(clear_btn, "error");
  gtk_box_append(GTK_BOX(toolbar), clear_btn);

  GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_hexpand(spacer, TRUE);
  gtk_box_append(GTK_BOX(toolbar), spacer);

  self->status_label = gtk_label_new("");
  gtk_label_set_xalign(GTK_LABEL(self->status_label), 1.0);
  gtk_box_append(GTK_BOX(toolbar), self->status_label);

  g_signal_connect(self->type_dropdown, "notify::selected",
                   G_CALLBACK(on_type_changed), self);
  g_signal_connect(format_btn, "clicked", G_CALLBACK(on_format_clicked), self);
  g_signal_connect(clear_btn, "clicked", G_CALLBACK(on_clear_clicked), self);

  return toolbar;
}

static GtkWidget *build_editor(BodyPanel *self) {
  self->source_buffer = gtk_source_buffer_new(NULL);

  GtkSourceLanguage *language = gtk_source_language_manager_get_language(
      gtk_source_language_manager_get_default(),
      BODY_TYPES[BODY_TYPE_JSON].language_id);
  gtk_source_buffer_set_language(self->source_buffer, language);

  GtkSourceStyleScheme *scheme = gtk_source_style_scheme_manager_get_scheme(
      gtk_source_style_scheme_manager_get_default(), "Adwaita-dark");
  if (scheme != NULL) {
    gtk_source_buffer_set_style_scheme(self->source_buffer, scheme);
  }

  GdkRGBA red_color = {1.0, 0.0, 0.0, 1.0};
  self->error_tag = gtk_text_buffer_create_tag(
      GTK_TEXT_BUFFER(self->source_buffer), "syntax-error", "underline",
      PANGO_UNDERLINE_ERROR, "underline-rgba", &red_color, NULL);

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

  g_signal_connect(self->source_buffer, "changed",
                   G_CALLBACK(on_buffer_changed), self);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled),
                                GTK_WIDGET(self->source_view));
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_add_css_class(scrolled, "body-editor");

  return scrolled;
}

static void body_panel_dispose(GObject *object) {
  cancel_pending_validation(BODY_PANEL(object));
  G_OBJECT_CLASS(body_panel_parent_class)->dispose(object);
}

static void body_panel_class_init(BodyPanelClass *klass) {
  G_OBJECT_CLASS(klass)->dispose = body_panel_dispose;
}

static void body_panel_init(BodyPanel *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_margin_start(GTK_WIDGET(self), 16);
  gtk_widget_set_margin_end(GTK_WIDGET(self), 16);
  gtk_widget_set_margin_top(GTK_WIDGET(self), 10);
  gtk_widget_set_margin_bottom(GTK_WIDGET(self), 10);

  GtkWidget *title = gtk_label_new("Request Body");
  gtk_label_set_xalign(GTK_LABEL(title), 0.0);
  gtk_widget_set_margin_top(title, 10);
  gtk_widget_set_margin_bottom(title, 10);
  gtk_widget_add_css_class(title, "title-4");
  gtk_box_append(GTK_BOX(self), title);

  self->current_type = BODY_TYPE_JSON;
  gtk_box_append(GTK_BOX(self), build_toolbar(self));
  gtk_box_append(GTK_BOX(self), build_editor(self));

  update_validation_status(self, TRUE, NULL);
}

BodyPanel *body_panel_new(void) { return g_object_new(BODY_TYPE_PANEL, NULL); }

void body_panel_apply_to_request(BodyPanel *self, HttpRequest *request) {
  g_return_if_fail(BODY_IS_PANEL(self));
  g_return_if_fail(request != NULL);

  if (self->validate_source_id != 0) {
    cancel_pending_validation(self);
    validate_current_content(self);
  }

  char *content = body_panel_get_content(self);

  if (content != NULL && *content != '\0' && self->is_valid) {
    http_request_set_body(request, content,
                          BODY_TYPES[self->current_type].mime_type);
  } else if (content != NULL && *content == '\0') {
    http_request_set_body(request, NULL, NULL);
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

static void body_panel_set_content_type(BodyPanel *self, BodyContentType type) {
  g_return_if_fail(BODY_IS_PANEL(self));

  self->current_type = type;

  const char *language_id = BODY_TYPES[type].language_id;
  GtkSourceLanguage *language =
      language_id != NULL
          ? gtk_source_language_manager_get_language(
                gtk_source_language_manager_get_default(), language_id)
          : NULL;
  gtk_source_buffer_set_language(self->source_buffer, language);

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->source_buffer), &start,
                             &end);
  gtk_text_buffer_remove_tag(GTK_TEXT_BUFFER(self->source_buffer),
                             self->error_tag, &start, &end);

  refresh_highlight(self->source_buffer);

  if (type != BODY_TYPE_TEXT) {
    validate_current_content(self);
  } else {
    update_validation_status(self, TRUE, NULL);
  }

  if (gtk_drop_down_get_selected(self->type_dropdown) != (guint)type) {
    gtk_drop_down_set_selected(self->type_dropdown, (guint)type);
  }
}

void body_panel_set_content(BodyPanel *self, const char *content) {
  g_return_if_fail(BODY_IS_PANEL(self));

  GtkTextIter start, end;
  gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(self->source_buffer), &start,
                             &end);
  gtk_text_buffer_delete(GTK_TEXT_BUFFER(self->source_buffer), &start, &end);

  if (content != NULL && *content != '\0') {
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
