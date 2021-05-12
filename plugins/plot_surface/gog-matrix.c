/*
 * gog-matrix.c
 *
 * Copyright (C) 2013 Jean Brefort (jean.brefort@normalesup.org)
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

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include "gog-matrix.h"

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

static GType gog_matrix_view_get_type (void);

/*-----------------------------------------------------------------------------
 *
 *  GogMatrixPlot
 *
 *-----------------------------------------------------------------------------
 */

static double *
gog_matrix_plot_build_matrix (GogXYZPlot *plot, gboolean *cardinality_changed)
{
	unsigned i, j;
	double val;
	GOData *mat = GOG_SERIES (plot->base.series->data)->values[2].data;
	unsigned n = plot->rows * plot->columns;
	double *data;

	if (cardinality_changed)
		*cardinality_changed = FALSE;
	if (n == 0)
		return NULL;
	data = g_new (double, n);

	for (i = 0; i < plot->rows; i++)
		for (j = 0; j < plot->columns; j++) {
			val = go_data_get_matrix_value (mat, i, j);
			if (plot->transposed)
				data[j * plot->rows + i] = val;
			else
				data[i * plot->columns + j] = val;
		}

	return data;
}

static char const *
gog_matrix_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name matrix objects
	*/
	return N_("PlotMatrix");
}

static void
gog_matrix_plot_class_init (GogMatrixPlotClass *klass)
{
	GogXYZPlotClass *gog_xyz_plot_klass = (GogXYZPlotClass *) klass;
	GogPlotClass *gog_plot_klass = (GogPlotClass*) klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;

	/* Fill in GogObject superclass values */
	gog_object_klass->type_name	= gog_matrix_plot_type_name;
	gog_object_klass->view_type	= gog_matrix_view_get_type ();

	gog_plot_klass->axis_set = GOG_AXIS_SET_XY_COLOR;
	gog_plot_klass->desc.series.style_fields = 0;

	gog_xyz_plot_klass->third_axis = GOG_AXIS_COLOR;
	gog_xyz_plot_klass->build_matrix = gog_matrix_plot_build_matrix;
}

static void
gog_matrix_plot_init (GogMatrixPlot *matrix)
{
	GogPlot *plot = GOG_PLOT (matrix);

	plot->rendering_order = GOG_PLOT_RENDERING_BEFORE_GRID;
}

GSF_DYNAMIC_CLASS (GogMatrixPlot, gog_matrix_plot,
	gog_matrix_plot_class_init, gog_matrix_plot_init,
	GOG_TYPE_XYZ_PLOT)

/*-----------------------------------------------------------------------------
 *
 *  GogMatrixView
 *
 *-----------------------------------------------------------------------------
 */

typedef GogPlotView		GogMatrixView;
typedef GogPlotViewClass	GogMatrixViewClass;

static void
gog_matrix_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogXYZPlot const *plot = GOG_XYZ_PLOT (view->model);
	GogSeries const *series;
	GOData *x_vec = NULL, *y_vec = NULL;
	GogAxisMap *x_map, *y_map, *z_map;
	GogAxisColorMap const *color_map = gog_axis_get_color_map (gog_plot_get_axis (GOG_PLOT (view->model), GOG_AXIS_COLOR));
	unsigned i, imax, j, jmax;
	double max, *data, z;
	GogRenderer *rend = view->renderer;
	GOStyle *style;
	gboolean xdiscrete, ydiscrete, hide_outliers = TRUE;
	GogViewAllocation rect;

	if (plot->base.series == NULL)
		return;
	series = GOG_SERIES (plot->base.series->data);
	if (plot->transposed) {
		imax = plot->columns;
		jmax = plot->rows;
	} else {
		imax = plot->rows;
		jmax = plot->columns;
	}
	if (imax == 0 || jmax == 0)
		return;

	if (plot->plotted_data)
		data = plot->plotted_data;
	else
		data = GOG_XYZ_PLOT (plot)->plotted_data = gog_xyz_plot_build_matrix (GOG_XYZ_PLOT (plot), NULL);

	x_map = gog_axis_map_new (plot->base.axis[0],
				  view->residual.x , view->residual.w);
	y_map = gog_axis_map_new (plot->base.axis[1],
				  view->residual.y + view->residual.h,
				  -view->residual.h);

	if (!(gog_axis_map_is_valid (x_map) &&
	      gog_axis_map_is_valid (y_map))) {
		gog_axis_map_free (x_map);
		gog_axis_map_free (y_map);
		return;
	}

	max = gog_axis_color_map_get_max (color_map);
	z_map = gog_axis_map_new (plot->base.axis[GOG_AXIS_COLOR], 0, max);

	xdiscrete = gog_axis_is_discrete (plot->base.axis[0]) ||
			series->values[(plot->transposed)? 1: 0].data == NULL;
	if (!xdiscrete)
		x_vec = gog_xyz_plot_get_x_vals (GOG_XYZ_PLOT (plot));
	ydiscrete = gog_axis_is_discrete (plot->base.axis[1]) ||
			series->values[(plot->transposed)? 0: 1].data == NULL;
	if (!ydiscrete)
		y_vec = gog_xyz_plot_get_y_vals (GOG_XYZ_PLOT (plot));
	/* clip to avoid problems with logarithmic axes */
	gog_renderer_push_clip_rectangle (rend, view->residual.x, view->residual.y,
					  view->residual.w, view->residual.h);

	style = go_style_new ();
	style->interesting_fields = GO_STYLE_FILL;
	style->disable_theming = GO_STYLE_ALL;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	style->fill.pattern.pattern = GO_PATTERN_SOLID;
	gog_renderer_push_style (rend, style);

	for (j = 0; j < jmax; j++) {
		if (xdiscrete) {
			rect.x = gog_axis_map_to_view (x_map, j + 1);
			rect.w = gog_axis_map_to_view (x_map, j + 2) - rect.x;
		} else {
			rect.x = gog_axis_map_to_view (x_map, go_data_get_vector_value (x_vec, j));
			rect.w = gog_axis_map_to_view (x_map, go_data_get_vector_value (x_vec, j + 1)) - rect.x;
		}

		for (i = 0; i < imax; i++) {
			if (ydiscrete) {
				rect.y = gog_axis_map_to_view (y_map, i + 1);
				rect.h = gog_axis_map_to_view (y_map, i + 2) - rect.y;
			} else {
				rect.y = gog_axis_map_to_view (y_map, go_data_get_vector_value (y_vec, i));
				rect.h = gog_axis_map_to_view (y_map, go_data_get_vector_value (y_vec, i + 1)) - rect.y;
			}
			z = data[(i) * jmax + j];
			if (gog_axis_map_finite (z_map, z)) {
				double zc = gog_axis_map_to_view (z_map, z);
				if (hide_outliers && (zc < 0 || zc > max))
					style->fill.pattern.back = 0;
				else
					style->fill.pattern.back = gog_axis_color_map_get_color (color_map, CLAMP (zc, 0, max));
			} else
				style->fill.pattern.back = 0;
			gog_renderer_draw_rectangle (rend, &rect);
		}
	}

	gog_renderer_pop_style (rend);
	gog_renderer_pop_clip (rend);
	g_object_unref (style);
	gog_axis_map_free (x_map);
	gog_axis_map_free (y_map);
	gog_axis_map_free (z_map);
	if (!plot->plotted_data)
		g_free (data);
}

static void
gog_matrix_view_class_init (GogViewClass *view_klass)
{
	view_klass->render = gog_matrix_view_render;
}

GSF_DYNAMIC_CLASS (GogMatrixView, gog_matrix_view,
	gog_matrix_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)
