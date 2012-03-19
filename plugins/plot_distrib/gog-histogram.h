/*
 * gog-histogram.h
 *
 * Copyright (C) 2005-2010 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_HISTOGRAM_H
#define GOG_HISTOGRAM_H

#include <goffice/graph/gog-plot-impl.h>

G_BEGIN_DECLS

typedef struct {
	GogPlot	base;

	struct {
		double minima, maxima;
		GOFormat const *fmt;
		GODateConventions const *date_conv;
	} x, y;
	gboolean vertical;
	gboolean cumulative;
} GogHistogramPlot;
typedef GogPlotClass GogHistogramPlotClass;

#define GOG_TYPE_HISTOGRAM_PLOT	(gog_histogram_plot_get_type ())
#define GOG_HISTOGRAM_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_HISTOGRAM_PLOT, GogHistogramPlot))
#define GOG_IS_HISTOGRAM_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_HISTOGRAM_PLOT))

GType gog_histogram_plot_get_type (void);

typedef struct {
	GogHistogramPlot base;
	GogDatasetElement *labels; /* labels for the two y categories */
} GogDoubleHistogramPlot;
typedef GogHistogramPlotClass GogDoubleHistogramPlotClass;

#define GOG_DOUBLE_HISTOGRAM_PLOT_TYPE	(gog_double_histogram_plot_get_type ())
#define GOG_DOUBLE_HISTOGRAM_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_DOUBLE_HISTOGRAM_PLOT_TYPE, GogDoubleHistogramPlot))
#define GOG_IS_DOUBLE_HISTOGRAM_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_DOUBLE_HISTOGRAM_PLOT_TYPE))

GType gog_double_histogram_plot_get_type (void);

G_END_DECLS

#endif /* GOG_HISTOGRAM_H */
