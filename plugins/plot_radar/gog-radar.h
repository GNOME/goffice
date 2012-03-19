/*
 * gog-radar.h
 *
 * Copyright (C) 2004 Michael Devine (mdevine@cs.stanford.edu)
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

#ifndef GOG_RADAR_H
#define GOG_RADAR_H

#include <goffice/graph/gog-plot-impl.h>

G_BEGIN_DECLS

typedef struct {
	GogPlot	base;
	gboolean default_style_has_markers;
	gboolean default_style_has_fill;
	unsigned num_elements;
	struct {
		double minima, maxima;
	} r, t;
} GogRTPlot;

typedef GogRTPlot GogRadarPlot;

typedef GogRTPlot GogPolarPlot;

typedef struct {
	GogPolarPlot	base;
	struct {
		double minima, maxima;
	} z;
	gboolean	hide_outliers;
} GogColorPolarPlot;

#define GOG_TYPE_RT_PLOT	(gog_rt_plot_get_type ())
#define GOG_RT_PLOT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_RT_PLOT, GogRTPlot))
#define GOG_IS_PLOT_RT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_RT_PLOT))

GType gog_rt_plot_get_type (void);

#define GOG_TYPE_RADAR_PLOT	(gog_radar_plot_get_type ())
#define GOG_RADAR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_RADAR_PLOT, GogRadarPlot))
#define GOG_IS_PLOT_RADAR(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_RADAR_PLOT))

GType gog_radar_plot_get_type (void);

#define GOG_TYPE_POLAR_PLOT	(gog_polar_plot_get_type ())
#define GOG_POLAR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_POLAR_PLOT, GogPolarPlot))
#define GOG_IS_PLOT_POLAR(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_POLAR_PLOT))

GType gog_polar_plot_get_type (void);

#define GOG_TYPE_COLOR_POLAR_PLOT	(gog_color_polar_plot_get_type ())
#define GOG_COLOR_POLAR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_COLOR_POLAR_PLOT, GogColorPolarPlot))
#define GOG_IS_PLOT_COLOR_POLAR(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_COLOR_POLAR_PLOT))

GType gog_color_polar_plot_get_type (void);

G_END_DECLS

#endif /* GOG_RADAR_H */
