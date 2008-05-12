/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-xyz.h
 *
 * Copyright (C) 2004-2007 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_XYZ_H
#define GOG_XYZ_H

#include <goffice/graph/gog-plot-impl.h>

G_BEGIN_DECLS

/*-----------------------------------------------------------------------------
 *
 * GogContourPlot
 *
 *-----------------------------------------------------------------------------
 */

typedef struct {
	GogPlot	base;
	
	unsigned rows, columns;
	gboolean transposed;
	struct {
		double minima, maxima;
		GOFormat *fmt;
	} x, y, z;
	double *plotted_data;
} GogXYZPlot;

#define GOG_XYZ_PLOT_TYPE	(gog_xyz_plot_get_type ())
#define GOG_XYZ_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_XYZ_PLOT_TYPE, GogXYZPlot))
#define GOG_IS_PLOT_XYZ(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_XYZ_PLOT_TYPE))

GType gog_xyz_plot_get_type (void);

typedef struct {
	GogPlotClass	base;

	GogAxisType third_axis;

	double * (*build_matrix) (GogXYZPlot const *plot, gboolean *cardinality_changed);
} GogXYZPlotClass;

#define GOG_XYZ_PLOT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_XYZ_PLOT_TYPE, GogXYZPlotClass))

double *gog_xyz_plot_build_matrix (GogXYZPlot const *plot, gboolean *cardinality_changed);

typedef struct {
	GogSeries base;
	
	unsigned rows, columns;
} GogXYZSeries;
typedef GogSeriesClass GogXYZSeriesClass;

#define GOG_XYZ_SERIES_TYPE	(gog_xyz_series_get_type ())
#define GOG_XYZ_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_XYZ_SERIES_TYPE, GogXYZSeries))
#define GOG_IS_XYZ_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_XYZ_SERIES_TYPE))

GType gog_xyz_series_get_type (void);

G_END_DECLS

#endif /* GOG_XYZ_H */
