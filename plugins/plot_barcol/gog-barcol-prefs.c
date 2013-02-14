/*
 * gog-barcol-prefs.c
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
#include "gog-barcol.h"
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/app/go-plugin.h>

GtkWidget *gog_barcol_plot_pref (GogBarColPlot *plot, GOCmdContext *cc);

static void
cb_gap_changed (GtkAdjustment *adj, GObject *barcol)
{
	g_object_set (barcol, "gap-percentage", (int) gtk_adjustment_get_value (adj), NULL);
}

static void
cb_overlap_changed (GtkAdjustment *adj, GObject *barcol)
{
	g_object_set (barcol, "overlap-percentage", (int) gtk_adjustment_get_value (adj), NULL);
}

static void
display_before_grid_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "before-grid", gtk_toggle_button_get_active (btn), NULL);
}

GtkWidget *
gog_barcol_plot_pref (GogBarColPlot *barcol, GOCmdContext *cc)
{
	GtkWidget  *w;
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_barcol/gog-barcol-prefs.ui",
				    GETTEXT_PACKAGE, cc);
        if (gui == NULL)
                return NULL;

	w = go_gtk_builder_get_widget (gui, "gap_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), barcol->gap_percentage);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_gap_changed), barcol);

	w = go_gtk_builder_get_widget (gui, "overlap_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), barcol->overlap_percentage);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_overlap_changed), barcol);

	w = go_gtk_builder_get_widget (gui, "before-grid");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
			(GOG_PLOT (barcol))->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (display_before_grid_cb), barcol);

	w = GTK_WIDGET (g_object_ref (gtk_builder_get_object (gui, "gog-barcol-prefs")));
	g_object_unref (gui);

	return w;
}
