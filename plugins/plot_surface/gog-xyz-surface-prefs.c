/*
 * gog-xyz-surface-prefs.c
 *
 * Copyright (C) 2004-2008 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-xyz-surface.h"
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/app/go-plugin.h>

#include <glib/gi18n-lib.h>
#include <string.h>

GtkWidget *gog_xyz_surface_plot_pref   (GogXYZPlot *plot, GogDataAllocator *dalloc, GOCmdContext *cc);

typedef struct {
	GogXYZPlot *plot;
	GtkWidget *x_spin, *y_spin, *x_label, *y_label, *x_entry, *y_entry;
} XYZSurfPrefsState;

static void
cb_rows_changed (GtkAdjustment *adj, GObject *plot)
{
	g_object_set (plot, "rows", (int) gtk_adjustment_get_value (adj), NULL);
}

static void
cb_columns_changed (GtkAdjustment *adj, GObject *plot)
{
	g_object_set (plot, "columns", (int) gtk_adjustment_get_value (adj), NULL);
}

static void
cb_cols_toggled (GtkToggleButton *btn, XYZSurfPrefsState *state)
{
	if (gtk_toggle_button_get_active (btn)) {
		gtk_widget_show (state->x_spin);
		gtk_widget_show (state->x_label);
		gtk_widget_hide (state->x_entry);
		g_object_set (state->plot, "auto-columns", TRUE, NULL);
	} else {
		gtk_widget_hide (state->x_spin);
		gtk_widget_hide (state->x_label);
		gtk_widget_show (state->x_entry);
		g_object_set (state->plot, "auto-columns", FALSE, NULL);
	}
}

static void
cb_rows_toggled (GtkToggleButton *btn, XYZSurfPrefsState *state)
{
	if (gtk_toggle_button_get_active (btn)) {
		gtk_widget_show (state->y_spin);
		gtk_widget_show (state->y_label);
		gtk_widget_hide (state->y_entry);
		g_object_set (state->plot, "auto-rows", TRUE, NULL);
	} else {
		gtk_widget_hide (state->y_spin);
		gtk_widget_hide (state->y_label);
		gtk_widget_show (state->y_entry);
		g_object_set (state->plot, "auto-rows", FALSE, NULL);
	}
}

static void
cb_missing_as_changed (GtkComboBox *box, XYZSurfPrefsState *state)
{
	g_object_set (state->plot,
	              "missing-as", missing_as_string (gtk_combo_box_get_active (box)),
	              NULL);

}

static void
cb_as_density_toggled (GtkToggleButton *btn, XYZSurfPrefsState *state)
{
	g_object_set (state->plot,
	              "as_density", gtk_toggle_button_get_active (btn),
	              NULL);

}
GtkWidget *
gog_xyz_surface_plot_pref (GogXYZPlot *plot, GogDataAllocator *dalloc, GOCmdContext *cc)
{
	GogDataset *set = GOG_DATASET (plot);
	XYZSurfPrefsState *state;
	GtkWidget  *w, *grid;
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_surface/gog-xyz-surface-prefs.ui",
				    GETTEXT_PACKAGE, cc);

        if (gui == NULL)
                return NULL;

	state = g_new (XYZSurfPrefsState, 1);
	state->plot = plot;

	state->x_spin = w = go_gtk_builder_get_widget (gui, "columns_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), plot->columns);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_columns_changed), plot);
	state->x_label = go_gtk_builder_get_widget (gui, "cols-nb-lbl");

	grid = go_gtk_builder_get_widget (gui, "gog-xyz-surface-prefs");
	state->x_entry = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, 0, GOG_DATA_VECTOR));
	gtk_widget_show_all (state->x_entry);
	gtk_widget_set_margin_left (state->x_entry, 12);
	gtk_grid_attach (GTK_GRID (grid), state->x_entry, 0, 2, 3, 1);
	w = go_gtk_builder_get_widget (gui, "preset-cols-btn");
	if (!state->plot->auto_x) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
		gtk_widget_hide (state->x_spin);
		gtk_widget_hide (state->x_label);
	} else
		gtk_widget_hide (state->x_entry);
	w = go_gtk_builder_get_widget (gui, "calc-cols-btn");
	g_signal_connect (G_OBJECT (w), "toggled", G_CALLBACK (cb_cols_toggled), state);

	state->y_spin = w = go_gtk_builder_get_widget (gui, "rows_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), plot->rows);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_rows_changed), plot);
	state->y_label = go_gtk_builder_get_widget (gui, "rows-nb-lbl");

	state->y_entry = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, 1, GOG_DATA_VECTOR));
	gtk_widget_show_all (state->y_entry);
	gtk_widget_set_margin_left (state->y_entry, 12);
	gtk_grid_attach (GTK_GRID (grid), state->y_entry, 0, 5, 3, 1);
	w = go_gtk_builder_get_widget (gui, "preset-rows-btn");
	if (!state->plot->auto_y) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), TRUE);
		gtk_widget_hide (state->y_spin);
		gtk_widget_hide (state->y_label);
	} else
		gtk_widget_hide (state->y_entry);
	w = go_gtk_builder_get_widget (gui, "calc-rows-btn");
	g_signal_connect (G_OBJECT (w), "toggled", G_CALLBACK (cb_rows_toggled), state);

	w = go_gtk_builder_get_widget (gui, "missing-as-btn");
	if (GOG_PLOT (plot)->desc.series.num_dim == 2) {
		gboolean as_density;
		gtk_widget_hide (w);
		gtk_widget_hide (go_gtk_builder_get_widget (gui, "missing-lbl"));
		w = gtk_check_button_new_with_label (_("Display population density"));
		gtk_container_add (GTK_CONTAINER (grid), w);
		gtk_widget_show (w);
		g_object_get (plot, "as-density", &as_density, NULL);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), as_density);
		g_signal_connect (G_OBJECT (w), "toggled", G_CALLBACK (cb_as_density_toggled), state);
	} else {
		char const *missing;
		g_object_get (plot, "missing-as", &missing, NULL);
		gtk_combo_box_set_active (GTK_COMBO_BOX (w), missing_as_value (missing));
		g_signal_connect (G_OBJECT (w), "changed", G_CALLBACK (cb_missing_as_changed), state);
	}

	w = GTK_WIDGET (g_object_ref (grid));
	g_object_set_data_full (G_OBJECT (w), "state", state, g_free);
	g_object_unref (gui);

	return w;
}

