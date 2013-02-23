/*
 * gog-pie-prefs.c
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
#include "gog-pie.h"

#include <goffice/gtk/goffice-gtk.h>
#include <goffice/app/go-plugin.h>

GtkWidget *gog_pie_series_element_pref   (GogPieSeriesElement *element, GOCmdContext *cc);

static void
cb_element_separation_changed (GtkAdjustment *adj, GObject *element)
{
	g_object_set (element, "separation", gtk_adjustment_get_value (adj) / 100., NULL);
}

GtkWidget *
gog_pie_series_element_pref (GogPieSeriesElement *element, GOCmdContext *cc)
{
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_pie/gog-pie-series.ui",
				    GETTEXT_PACKAGE, cc);
	GtkWidget  *w;
        if (gui == NULL)
                return NULL;

	w = go_gtk_builder_get_widget (gui, "separation_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), element->separation * 100.);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_element_separation_changed), element);

	w = GTK_WIDGET (g_object_ref (gtk_builder_get_object (gui, "gog-pie-series-element-prefs")));
	g_object_unref (gui);

	return w;
}

/****************************************************************************/
GtkWidget *gog_pie_plot_pref   (GogPiePlot *plot, GOCmdContext *cc);

typedef struct {
	GtkWidget	*separation_spinner;
	GogObject	*gobj;
	gulong		 update_editor_handler;
} PiePrefState;

static void
pie_pref_state_free (PiePrefState *state)
{
	g_signal_handler_disconnect (state->gobj, state->update_editor_handler);
	g_object_unref (state->gobj);
	g_free (state);
}

static void
cb_default_separation_changed (GtkAdjustment *adj, GObject *pie)
{
	g_object_set (pie, "default-separation", gtk_adjustment_get_value (adj) / 100., NULL);
}

static void
cb_rotation_changed (GtkAdjustment *adj, GObject *pie)
{
	g_object_set (pie, "initial-angle", gtk_adjustment_get_value (adj), NULL);
}


static void
cb_use_style_toggled (GtkToggleButton *button, GObject *series)
{
	g_object_set (series, "vary-style-by-element",
		gtk_toggle_button_get_active (button), NULL);
}

static void
cb_show_negs_changed (GtkComboBox *box, GogPiePlot *pie)
{
	GSList *ptr = GOG_PLOT (pie)->series;
	pie->show_negatives = gtk_combo_box_get_active (box);
	while (ptr) {
		gog_object_request_update (GOG_OBJECT (ptr->data));
		ptr = ptr->next;
	}
	gog_object_emit_changed (GOG_OBJECT (pie), FALSE);
}

static void
gog_pie_plot_pref_signal_connect (GogPiePlot *pie, GtkBuilder *gui)
{
	GtkWidget *w;

	w = go_gtk_builder_get_widget (gui, "rotation_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), pie->initial_angle);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_rotation_changed), pie);

	w = go_gtk_builder_get_widget (gui, "separation_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), pie->default_separation * 100.);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_default_separation_changed), pie);

	w = go_gtk_builder_get_widget (gui, "vary_style_by_element");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), pie->base.vary_style_by_element);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_use_style_toggled), pie);

	w = go_gtk_builder_get_widget (gui, "neg-box");
	gtk_combo_box_set_active (GTK_COMBO_BOX (w), pie->show_negatives);
	g_signal_connect (G_OBJECT (w),
		"changed",
		G_CALLBACK (cb_show_negs_changed), pie);
}

static void
cb_update_editor (GogPiePlot *pie, PiePrefState *state)
{
	if (state->separation_spinner != NULL) {
		double value;
		g_object_get (G_OBJECT (pie), "default-separation", &value, NULL);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->separation_spinner), value * 100.0);
	}
}

/****************************************************************************/

GtkWidget *
gog_pie_plot_pref (GogPiePlot *pie, GOCmdContext *cc)
{
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_pie/gog-pie-prefs.ui",
				    GETTEXT_PACKAGE, cc);
	GtkWidget  *w;
	PiePrefState *state;

        if (gui == NULL)
                return NULL;

	state = g_new0 (PiePrefState, 1);
	state->gobj = GOG_OBJECT (pie);
	state->separation_spinner = go_gtk_builder_get_widget (gui, "separation_spinner");
	g_object_ref (pie);

	gog_pie_plot_pref_signal_connect (pie, gui);

	state->update_editor_handler = g_signal_connect (G_OBJECT (pie),
							 "update-editor",
							 G_CALLBACK (cb_update_editor), state);

	w = GTK_WIDGET (g_object_ref (gtk_builder_get_object (gui, "gog-pie-prefs")));
	g_object_set_data_full (G_OBJECT (w), "state", state, (GDestroyNotify) pie_pref_state_free);
	g_object_unref (gui);

	return w;
}

/****************************************************************************/

GtkWidget *gog_ring_plot_pref   (GogRingPlot *ring, GOCmdContext *cc);

static void
cb_center_size_changed (GtkAdjustment *adj, GObject *ring)
{
	g_object_set (ring, "center-size", gtk_adjustment_get_value (adj) / 100., NULL);
}


GtkWidget *
gog_ring_plot_pref (GogRingPlot *ring, GOCmdContext *cc)
{
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_pie/gog-ring-prefs.ui",
				    GETTEXT_PACKAGE, cc);
	GtkWidget  *w;
	PiePrefState *state;

        if (gui == NULL)
                return NULL;

	state = g_new0 (PiePrefState, 1);
	state->gobj = GOG_OBJECT (ring);
	state->separation_spinner = go_gtk_builder_get_widget (gui, "separation_spinner");
	g_object_ref (ring);

	gog_pie_plot_pref_signal_connect (GOG_PIE_PLOT (ring), gui);

	w = go_gtk_builder_get_widget (gui, "center_size_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), ring->center_size * 100);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_center_size_changed), ring);

	state->update_editor_handler = g_signal_connect (G_OBJECT (ring),
							 "update-editor",
							 G_CALLBACK (cb_update_editor), state);

	w = GTK_WIDGET (g_object_ref (gtk_builder_get_object (gui, "gog-ring-prefs")));
	g_object_set_data_full (G_OBJECT (w), "state", state, (GDestroyNotify) pie_pref_state_free);
	g_object_unref (gui);

	return w;
}
