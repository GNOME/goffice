/*
 * gog-axis-color-map.c
 *
 * Copyright (C) 2012 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#endif

struct _GogAxisColorMap {
	GObject base;
	char *name;
	unsigned size; /* colors number */
	unsigned *limits;
	GOColor *colors;
};
typedef GObjectClass GogAxisColorMapClass;

static GObjectClass *parent_klass;

static void
gog_axis_color_map_finalize (GObject *obj)
{
	GogAxisColorMap *map = GOG_AXIS_COLOR_MAP (obj);
	g_free (map->name);
	map->name = NULL;
	g_free (map->limits);
	map->limits = NULL;
	g_free (map->colors);
	map->colors = NULL;
	parent_klass->finalize (obj);
}

static void
gog_axis_color_map_class_init (GObjectClass *gobject_klass)
{
	parent_klass = g_type_class_peek_parent (gobject_klass);
	/* GObjectClass */
	gobject_klass->finalize = gog_axis_color_map_finalize;
}

static void
gog_axis_color_map_init (GogAxisColorMap *map)
{
}

GSF_CLASS (GogAxisColorMap, gog_axis_color_map,
	   gog_axis_color_map_class_init, gog_axis_color_map_init,
	   G_TYPE_OBJECT)

GOColor
gog_axis_color_map_get_color (GogAxisColorMap const *map, double x)
{
	unsigned n = 1;
	double t;
	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), (GOColor) 0x00000000);
	if (x < 0. || map->size == 0)
		return (GOColor) 0x00000000;
	if (map->size == 1)
		return map->colors[0];
	if (x > map->limits[map->size-1])
		x -= floor (x / map->limits[map->size-1]) * map->limits[map->size-1];
	while (n < map->size && x > map->limits[n])
		n++;
	t = (x - map->limits[n-1]) / (map->limits[n] - map->limits[n-1]);
	return GO_COLOR_INTERPOLATE (map->colors[n-1], map->colors[n], t);
}

unsigned
gog_axis_color_map_get_max (GogAxisColorMap const *map)
{
	g_return_val_if_fail (GOG_IS_AXIS_COLOR_MAP (map), 0);
	return (map->size > 0)? map->limits[map->size-1]: 0;
}

/**
 * gog_axis_color_map_from_colors:
 * @name: color map name
 * @nb: colors number
 * @colors: the colors.
 *
 * Creates a color map using @colors.
 * Returns: (transfer full): the newly created color map.
 **/
GogAxisColorMap *
gog_axis_color_map_from_colors (char const *name, unsigned nb, GOColor const *colors)
{
	unsigned i;
	GogAxisColorMap *color_map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, NULL);
	color_map->name = g_strdup (name);
	color_map->size = nb;
	color_map->limits = g_new (unsigned, nb);
	color_map->colors = g_new (GOColor, nb);
	for (i = 0; i < nb; i++) {
		color_map->limits[i] = i;
		color_map->colors[i] = colors[i];
	}
	return color_map;
}

static GogAxisColorMap *color_map = NULL;

GogAxisColorMap const *
_gog_axis_color_map_get_default ()
{
	return color_map;
}

void
_gog_axis_color_maps_init (void)
{
	/* Default color map */
	color_map = g_object_new (GOG_TYPE_AXIS_COLOR_MAP, NULL);
	color_map->name = g_strdup ("Default");
	color_map->size = 5;
	color_map->limits = g_new (unsigned, 5);
	color_map->colors = g_new (GOColor, 5);
	color_map->limits[0] = 0;
	color_map->colors[0] = GO_COLOR_BLUE;
	color_map->limits[1] = 1;
	color_map->colors[1] = GO_COLOR_FROM_RGB (0, 0xff, 0xff);
	color_map->limits[2] = 2;
	color_map->colors[2] = GO_COLOR_GREEN;
	color_map->limits[3] = 4;
	color_map->colors[3] = GO_COLOR_YELLOW;
	color_map->limits[4] = 6;
	color_map->colors[4] = GO_COLOR_RED;
}

void
_gog_axis_color_maps_shutdown (void)
{
	g_object_unref (color_map);
}
