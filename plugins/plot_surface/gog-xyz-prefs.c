/*
 * gog-xyz-prefs.c
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
#include "gog-xyz.h"
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/app/go-plugin.h>

#include <string.h>

GtkWidget *gog_xyz_plot_pref   (GogXYZPlot *plot, GOCmdContext *cc);

static void
cb_transpose (GtkToggleButton *btn, GObject *plot)
{
	g_object_set (plot, "transposed", gtk_toggle_button_get_active (btn), NULL);
}

GtkWidget *
gog_xyz_plot_pref (GogXYZPlot *plot, GOCmdContext *cc)
{
	GtkWidget  *w;
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_surface/gog-xyz-prefs.ui",
				    GETTEXT_PACKAGE, cc);
        if (gui == NULL)
                return NULL;

	w = go_gtk_builder_get_widget (gui, "transpose");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), plot->transposed);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_transpose), plot);

	w = GTK_WIDGET (g_object_ref (gtk_builder_get_object (gui, "gog-xyz-prefs")));
	g_object_unref (gui);

	return w;
}
