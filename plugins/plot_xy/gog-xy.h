/*
 * go-xy.h
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

#ifndef GOG_XY_PLOT_H
#define GOG_XY_PLOT_H

#include <goffice/graph/gog-plot-impl.h>
#include <goffice/graph/gog-series-impl.h>

G_BEGIN_DECLS

typedef struct {
	GogPlot	base;

	struct {
		double minima, maxima;
		GOFormat const *fmt;
		GODateConventions const *date_conv;
	} x, y;
} Gog2DPlot;

typedef struct {
	Gog2DPlot	base;
	gboolean	default_style_has_markers;
	gboolean	default_style_has_lines;
	gboolean	default_style_has_fill;
	gboolean	use_splines;			/* for compatibility with goffice 0.2.x */
} GogXYPlot;

typedef struct {
	Gog2DPlot	base;
	gboolean size_as_area;
	gboolean in_3d;
	gboolean show_negatives;
	double bubble_scale;
} GogBubblePlot;

typedef struct {
	Gog2DPlot	base;
	gboolean	default_style_has_lines;
	gboolean	default_style_has_fill;
	gboolean	hide_outliers;
	struct {
		double minima, maxima;
		GOFormat const *fmt;
		GODateConventions const *date_conv;
	} z;
} GogXYColorPlot;

#define GOG_2D_PLOT_TYPE	(gog_2d_plot_get_type ())
#define GOG_2D_PLOT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_2D_PLOT_TYPE, Gog2DPlot))
#define GOG_IS_2D_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_2D_PLOT_TYPE))

GType gog_2d_plot_get_type (void);

#define GOG_TYPE_XY_PLOT	(gog_xy_plot_get_type ())
#define GOG_XY_PLOT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XY_PLOT, GogXYPlot))
#define GOG_IS_XY_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XY_PLOT))

GType gog_xy_plot_get_type (void);

#define GOG_TYPE_BUBBLE_PLOT	(gog_bubble_plot_get_type ())
#define GOG_BUBBLE_PLOT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_BUBBLE_PLOT, GogBubblePlot))
#define GOG_IS_BUBBLE_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_BUBBLE_PLOT))

GType gog_bubble_plot_get_type (void);

#define GOG_TYPE_XY_COLOR_PLOT	(gog_xy_color_plot_get_type ())
#define GOG_XY_COLOR_PLOT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XY_COLOR_PLOT, GogXYColorPlot))
#define GOG_IS_XY_COLOR_PLOT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XY_COLOR_PLOT))

GType gog_xy_color_plot_get_type (void);

typedef struct {
	GogSeries 	   base;
	GogErrorBar 	  *x_errors;
	GogErrorBar 	  *y_errors;
	GogObject 	  *hdroplines;
    GogObject	  *vdroplines;
	gboolean 	   invalid_as_zero;
	double		   clamped_derivs[2]; /* start and slopes for clamped cubic splines */
	GogDataset	  *interpolation_props;
} GogXYSeries;

#define GOG_TYPE_XY_SERIES	(gog_xy_series_get_type ())
#define GOG_XY_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XY_SERIES, GogXYSeries))
#define GOG_IS_XY_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XY_SERIES))

G_END_DECLS

#endif /* GOG_XY_PLOT_H */
