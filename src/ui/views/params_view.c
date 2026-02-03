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

#include "params_view.h"

struct _ParamsView {
  GtkBox parent_instance;
};

G_DEFINE_TYPE(ParamsView, params_view, GTK_TYPE_BOX)

static void params_view_class_init(ParamsViewClass *klass) { (void)klass; }

static void params_view_init(ParamsView *self) {
  gtk_orientable_set_orientation(GTK_ORIENTABLE(self),
                                 GTK_ORIENTATION_VERTICAL);

  gtk_widget_set_halign(GTK_WIDGET(self), GTK_ALIGN_CENTER);
  gtk_widget_set_valign(GTK_WIDGET(self), GTK_ALIGN_CENTER);

  GtkWidget *label = gtk_label_new("Params page");
  gtk_widget_add_css_class(label, "title-1");

  gtk_box_append(GTK_BOX(self), label);
}

ParamsView *params_view_new(void) {
  return g_object_new(PARAMS_TYPE_VIEW, NULL);
}
