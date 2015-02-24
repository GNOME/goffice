/*
 * gog-axis-line.h :
 *
 * Copyright (C) 2005 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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

#ifndef GOG_AXIS_BASE_H
#define GOG_AXIS_BASE_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GOG_TYPE_AXIS_BASE	(gog_axis_base_get_type ())
#define GOG_AXIS_BASE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_AXIS_BASE, GogAxisBase))
#define GOG_IS_AXIS_BASE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_AXIS_BASE))

GType gog_axis_base_get_type (void);

GogAxisType gog_axis_base_get_crossed_axis_type (GogAxisBase *axis_base);
GogAxis *gog_axis_base_get_crossed_axis (GogAxisBase *axis_base);
GogAxis *gog_axis_base_get_crossed_axis_for_plot (GogAxisBase *axis_base, GogPlot *plot);
double gog_axis_base_get_cross_location (GogAxisBase *axis_base);

typedef enum {
	GOG_AXIS_AT_LOW,
	GOG_AXIS_CROSS,
	GOG_AXIS_AT_HIGH,
	GOG_AXIS_AUTO
} GogAxisPosition;

GogAxisPosition 	gog_axis_base_get_position 	   (GogAxisBase *axis_base);
void		    	gog_axis_base_set_position 	   (GogAxisBase *axis_base, GogAxisPosition position);
GogAxisPosition 	gog_axis_base_get_clamped_position (GogAxisBase *axis_base);

typedef enum {
	GOG_AXIS_TICK_NONE,
	GOG_AXIS_TICK_MAJOR,
	GOG_AXIS_TICK_MINOR
} GogAxisTickTypes;

typedef struct {
	double		 position;
	GogAxisTickTypes type;
	GOString        *str;
} GogAxisTick;

#define GOG_TYPE_AXIS_LINE	(gog_axis_line_get_type ())
#define GOG_AXIS_LINE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_AXIS_LINE, GogAxisLine))
#define GOG_IS_AXIS_LINE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_AXIS_LINE))

GType gog_axis_line_get_type (void);

unsigned	 gog_axis_line_get_ticks (GogAxisLine *axis_line, GogAxisTick **ticks);

G_END_DECLS

#endif /*GOG_AXIS_BASE_H*/

