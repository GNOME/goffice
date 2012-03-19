/*
 * plot_distrib/plugin.c :
 *
 * Copyright (C) 2005-2008 Jean Brefort (jean.brefort@normalesup.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <goffice/goffice-config.h>
#include <goffice/app/module-plugin-defs.h>

#include "embedded-stuff.c"

GOFFICE_PLUGIN_MODULE_HEADER;

/* Plugin initialization */
void gog_box_plot_register_type (GTypeModule *module);
void gog_box_plot_view_register_type (GTypeModule *module);
void gog_box_plot_series_register_type (GTypeModule *module);
void gog_double_histogram_plot_register_type (GTypeModule *module);
void gog_histogram_plot_register_type (GTypeModule *module);
void gog_histogram_plot_series_register_type (GTypeModule *module);
void gog_histogram_plot_view_register_type (GTypeModule *module);
void gog_histogram_series_view_register_type (GTypeModule *module);
void gog_probability_plot_register_type (GTypeModule *module);
void gog_probability_plot_series_register_type (GTypeModule *module);
void gog_probability_plot_view_register_type (GTypeModule *module);
void gog_probability_plot_series_view_register_type (GTypeModule *module);

G_MODULE_EXPORT void
go_plugin_init (GOPlugin *plugin, GOCmdContext *cc)
{
	GTypeModule *module = go_plugin_get_type_module (plugin);
	gog_box_plot_register_type (module);
	gog_box_plot_view_register_type (module);
	gog_box_plot_series_register_type (module);
	gog_histogram_plot_register_type (module);
	gog_histogram_plot_view_register_type (module);
	gog_histogram_plot_series_register_type (module);
	gog_histogram_series_view_register_type (module);
	gog_double_histogram_plot_register_type (module);
	gog_probability_plot_register_type (module);
	gog_probability_plot_view_register_type (module);
	gog_probability_plot_series_register_type (module);
	gog_probability_plot_series_view_register_type (module);

	register_embedded_stuff ();
}

G_MODULE_EXPORT void
go_plugin_shutdown (GOPlugin *plugin, GOCmdContext *cc)
{
	unregister_embedded_stuff ();
}
