/*
 * gog-xyz.h
 *
 * Copyright (C) 2004-2007 Jean Brefort (jean.brefort@normalesup.org)
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
	gboolean data_xyz;
	gboolean auto_x, auto_y; /* automatic limits for xyz plots */
	struct {
		double minima, maxima;
		GOFormat const *fmt;
		GODateConventions const *date_conv;
	} x, y, z;
	double *plotted_data;
	GOData *x_vals, *y_vals;
} GogXYZPlot;

#define GOG_TYPE_XYZ_PLOT	(gog_xyz_plot_get_type ())
#define GOG_XYZ_PLOT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XYZ_PLOT, GogXYZPlot))
#define GOG_IS_PLOT_XYZ(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XYZ_PLOT))

GType gog_xyz_plot_get_type (void);

typedef struct {
	GogPlotClass	base;

	GogAxisType third_axis;

	double * (*build_matrix) (GogXYZPlot *plot, gboolean *cardinality_changed);
	GOData * (*get_x_vals) (GogXYZPlot *plot);
	GOData * (*get_y_vals) (GogXYZPlot *plot);
} GogXYZPlotClass;

#define GOG_XYZ_PLOT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_XYZ_PLOT, GogXYZPlotClass))

double *gog_xyz_plot_build_matrix (GogXYZPlot *plot, gboolean *cardinality_changed);
GOData *gog_xyz_plot_get_x_vals (GogXYZPlot *plot);
GOData *gog_xyz_plot_get_y_vals (GogXYZPlot *plot);

typedef struct {
	GogSeries base;

	unsigned rows, columns;
} GogXYZSeries;
typedef GogSeriesClass GogXYZSeriesClass;

#define GOG_TYPE_XYZ_SERIES	(gog_xyz_series_get_type ())
#define GOG_XYZ_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XYZ_SERIES, GogXYZSeries))
#define GOG_IS_XYZ_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XYZ_SERIES))

GType gog_xyz_series_get_type (void);

G_END_DECLS

#endif /* GOG_XYZ_H */
