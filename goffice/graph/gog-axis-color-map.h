/*
 * gog-axis-color-map.h
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

#ifndef GOG_AXIS_COLOR_MAP_H
#define GOG_AXIS_COLOR_MAP_H

#include <goffice/goffice.h>
#include <gsf/gsf-libxml.h>

G_BEGIN_DECLS

#define GOG_TYPE_AXIS_COLOR_MAP	(gog_axis_color_map_get_type ())
#define GOG_AXIS_COLOR_MAP(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_AXIS_COLOR_MAP, GogAxisColorMap))
#define GOG_IS_AXIS_COLOR_MAP(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_AXIS_COLOR_MAP))

GType gog_axis_color_map_get_type (void);

GOColor gog_axis_color_map_get_color (GogAxisColorMap const *map, double x);
unsigned gog_axis_color_map_get_max (GogAxisColorMap const *map);
GogAxisColorMap *gog_axis_color_map_from_colors (char const *name, unsigned nb,
                                                 GOColor const *colors,
                                                 GoResourceType type);
GogAxisColorMap *gog_axis_color_map_dup (GogAxisColorMap const *map);
GdkPixbuf *gog_axis_color_map_get_snapshot (GogAxisColorMap const *map,
                                            gboolean discrete,
                                            gboolean horizontal,
                                            unsigned width,
                                            unsigned height);
void gog_axis_color_map_to_cairo (GogAxisColorMap const *map, cairo_t *cr,
                                  unsigned discrete, gboolean horizontal,
                                  double width, double height);
char const *gog_axis_color_map_get_id (GogAxisColorMap const *map);
char const *gog_axis_color_map_get_name (GogAxisColorMap const *map);
GoResourceType gog_axis_color_map_get_resource_type (GogAxisColorMap const *map);
#ifdef GOFFICE_WITH_GTK
GogAxisColorMap *gog_axis_color_map_edit (GogAxisColorMap *map, GOCmdContext *cc);
#endif
typedef void (*GogAxisColorMapHandler) (GogAxisColorMap const *map, gpointer user_data);
void gog_axis_color_map_foreach (GogAxisColorMapHandler handler, gpointer user_data);
GogAxisColorMap const *gog_axis_color_map_get_from_id (char const *id);
gboolean gog_axis_color_map_delete (GogAxisColorMap *map);

/* private */
GogAxisColorMap const *_gog_axis_color_map_get_default (void);
void _gog_axis_color_maps_init	 (void);
void _gog_axis_color_maps_shutdown (void);

G_END_DECLS

#endif /* GOG_AXIS_COLOR_MAP_H */
