/*
 * gog-bubble-prefs.c
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <goffice/goffice-config.h>
#include "gog-xy.h"
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/app/go-plugin.h>

#include <string.h>

GtkWidget *gog_bubble_plot_pref   (GogBubblePlot *bubble, GOCmdContext *cc);

static void
cb_type_changed (GtkToggleButton* button, GObject *bubble)
{
	if (gtk_toggle_button_get_active (button))
		g_object_set (bubble, "size-as-area",
			strcmp (gtk_buildable_get_name (GTK_BUILDABLE (button)), "area")? FALSE: TRUE, NULL);
}

static void
cb_style_changed (GtkToggleButton* button, GObject *bubble)
{
	g_object_set (bubble, "vary-style-by-element",
		gtk_toggle_button_get_active (button), NULL);
}

static void
cb_3d_changed (GtkToggleButton* button, GObject *bubble)
{
	g_object_set (bubble, "in-3d",
		gtk_toggle_button_get_active (button), NULL);
}

static void
cb_negatives_changed (GtkToggleButton* button, GObject *bubble)
{
	g_object_set (bubble, "show-negatives",
		gtk_toggle_button_get_active (button), NULL);
}

static void
cb_scale_changed (GtkAdjustment *adj, GObject *bubble)
{
	g_object_set (bubble, "bubble-scale", gtk_adjustment_get_value (adj) / 100., NULL);
}

GtkWidget *
gog_bubble_plot_pref (GogBubblePlot *bubble, GOCmdContext *cc)
{
	GtkWidget  *w;
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_xy/gog-bubble-prefs.ui",
				    GETTEXT_PACKAGE, cc);

        if (gui == NULL)
                return NULL;

	w = go_gtk_builder_get_widget (gui, "area");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), bubble->size_as_area);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_type_changed), bubble);

	w = go_gtk_builder_get_widget (gui, "diameter");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), !bubble->size_as_area);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_type_changed), bubble);

	w = go_gtk_builder_get_widget (gui, "vary_style_by_element");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), bubble->base.base.vary_style_by_element);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_style_changed), bubble);

	w = go_gtk_builder_get_widget (gui, "3d");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), bubble->in_3d);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_3d_changed), bubble);

	/* TODO Add support for 3D bubbles. Hide the button for now. */
	gtk_widget_hide (w);

	w = go_gtk_builder_get_widget (gui, "scale");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), bubble->bubble_scale * 100.);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_scale_changed), bubble);

	w = go_gtk_builder_get_widget (gui, "show_negative_values");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), bubble->show_negatives);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_negatives_changed), bubble);

	w = GTK_WIDGET (g_object_ref (gtk_builder_get_object (gui, "gog-bubble-prefs")));
	g_object_unref (gui);

	return w;
}
