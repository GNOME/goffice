/*
 * go-pie.h
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

#ifndef GOG_PIE_PLOT_H
#define GOG_PIE_PLOT_H

#include <goffice/graph/gog-plot-impl.h>
#include <goffice/graph/gog-series-impl.h>

G_BEGIN_DECLS

typedef struct  {
	GogSeriesElement base;

	double separation;
} GogPieSeriesElement;

#define GOG_TYPE_PIE_SERIES_ELEMENT	(gog_pie_series_element_get_type ())
#define GOG_PIE_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_PIE_SERIES_ELEMENT, GogPieSeriesElement))
#define GOG_IS_PIE_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_PIE_SERIES_ELEMENT))

GType gog_pie_series_element_get_type (void);

typedef struct {
	GogPlot	base;

	double	 initial_angle;	 	/* degrees counter-clockwise from 3 o'clock */
	double   span;
	double	 default_separation;	/* as a percentage of the radius */
	gboolean in_3d;
	unsigned show_negatives;
} GogPiePlot;

#define GOG_TYPE_PIE_PLOT	(gog_pie_plot_get_type ())
#define GOG_PIE_PLOT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_PIE_PLOT, GogPiePlot))
#define GOG_IS_PIE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_PIE_PLOT))

GType gog_pie_plot_get_type (void);

typedef struct {
	GogPiePlot	base;

	double	 center_size;
} GogRingPlot;

#define GOG_TYPE_RING_PLOT	(gog_ring_plot_get_type ())
#define GOG_RING_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_RING_PLOT, GogRingPlot))
#define GOG_IS_RING_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_RING_PLOT))

GType gog_ring_plot_get_type (void);

typedef struct {
	GogSeries base;

	double	 initial_angle;	/* degrees counterclockwise from 3 o'clock */
	double	 separation;	/* as a percentage of the radius */

	double   total;
	double	*extensions;
} GogPieSeries;

#define GOG_TYPE_PIE_SERIES	(gog_pie_series_get_type ())
#define GOG_PIE_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_PIE_SERIES, GogPieSeries))
#define GOG_IS_PIE_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_PIE_SERIES))

GType gog_pie_series_get_type (void);

G_END_DECLS

#endif /* GOG_PIE_SERIES_H */
