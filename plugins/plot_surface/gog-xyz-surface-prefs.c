/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-xyz-surface-prefs.c
 *
 * Copyright (C) 2004-2008 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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

#include <gtk/gtktogglebutton.h>

#include <string.h>

GtkWidget *gog_xyz_surface_plot_pref   (GogXYZSurfacePlot *plot, GOCmdContext *cc);

static void
cb_rows_changed (GtkAdjustment *adj, GObject *plot)
{
	g_object_set (plot, "rows", (int) adj->value, NULL);
}

static void
cb_columns_changed (GtkAdjustment *adj, GObject *plot)
{
	g_object_set (plot, "columns", (int) adj->value, NULL);
}

GtkWidget *
gog_xyz_surface_plot_pref (GogXYZSurfacePlot *plot, GOCmdContext *cc)
{
	GogXYZPlot *xyz = GOG_XYZ_PLOT (plot);
	GtkWidget  *w;
	char const *dir = go_plugin_get_dir_name (
		go_plugins_get_plugin_by_id ("GOffice_plot_surface"));
	char	 *path = g_build_filename (dir, "gog-xyz-surface-prefs.glade", NULL);
	GladeXML *gui = go_libglade_new (path, "gog_xyz_surface_prefs", GETTEXT_PACKAGE, cc);

	g_free (path);
        if (gui == NULL)
                return NULL;

	w = glade_xml_get_widget (gui, "rows_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), xyz->rows);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_rows_changed), plot);

	w = glade_xml_get_widget (gui, "columns_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), xyz->columns);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_columns_changed), plot);

	w = glade_xml_get_widget (gui, "gog_xyz_surface_prefs");
	g_object_set_data_full (G_OBJECT (w),
		"state", gui, (GDestroyNotify)g_object_unref);

	return w;
}

