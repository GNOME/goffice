/*
 * gog-plot-engine.h :
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
#ifndef GOG_PLOT_ENGINE_H
#define GOG_PLOT_ENGINE_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GogPlotType {
	GogPlotFamily 	*family;
	char 		*engine;

	char 		*name, *sample_image_file;
	char 		*description; /* untranslated */
	int 		 col, row;

	GHashTable 	*properties;
};

struct _GogPlotFamily {
	char *name, *sample_image_file;
	int priority;

	GogAxisSet	 axis_set;

	GHashTable *types;
};

/* GogPlotFamily hashed by name */
GHashTable const *gog_plot_families (void);
GogPlotFamily *gog_plot_family_by_name  (char const *name);
GogPlotFamily *gog_plot_family_register (char const *name, char const *sample_image_file,
					 int priority, GogAxisSet axis_set);
void gog_plot_family_unregister (GogPlotFamily *family);
GogPlotType   *gog_plot_type_register   (GogPlotFamily *family, int col, int row,
					 char const *name, char const *sample_image_file,
					 char const *description, char const *engine);

struct _GogTrendLineType {
	char *engine;

	char *name;
	char *description; /* untranslated */

	GHashTable *properties;
};

GHashTable const *gog_trend_line_types (void);

void _gog_plugin_services_init (void);
void _gog_plugin_services_shutdown (void);

G_END_DECLS

#endif /* GOG_PLOT_ENGINE_H */
