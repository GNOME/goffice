/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-surface.c
 *
 * Copyright (C) 2004-2005 Jean Brefort (jean.brefort@normalesup.org)
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

#include <goffice/goffice-config.h>
#include "gog-surface.h"

#include <goffice/data/go-data.h>
#include <goffice/graph/gog-chart-map-3d.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/utils/go-path.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

/*****************************************************************************/

static GType gog_surface_view_get_type (void);

static double *
gog_surface_plot_build_matrix (GogXYZPlot const *plot, gboolean *cardinality_changed)
{
	unsigned i, j;
	double val;
	GogSeries *series = GOG_SERIES (plot->base.series->data);
	GODataMatrix *mat = GO_DATA_MATRIX (series->values[2].data);
	unsigned n = plot->rows * plot->columns;
	double *data;

	data = g_new (double, n);

	for (i = 0; i < plot->rows; i++)
		for (j = 0; j < plot->columns; j++) {
			val = go_data_matrix_get_value (mat, i, j);
			if (plot->transposed)
				data[j * plot->rows + i] = val;
			else
				data[i * plot->columns + j] = val;
		}
	*cardinality_changed = FALSE;
	return data;
}

static char const *
gog_surface_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name surface plot objects
	*/
	return N_("PlotSurface");
}

static void
gog_surface_plot_class_init (GogSurfacePlotClass *klass)
{
	GogXYZPlotClass *gog_xyz_plot_klass = (GogXYZPlotClass *) klass;
	GogPlotClass *gog_plot_klass = (GogPlotClass*) klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;

	gog_object_klass->type_name	= gog_surface_plot_type_name;
	gog_object_klass->view_type	= gog_surface_view_get_type ();

	/* Fill in GogPlotClass methods */
	gog_plot_klass->axis_set = GOG_AXIS_SET_XYZ;
	gog_plot_klass->desc.series.style_fields = GOG_STYLE_LINE | GOG_STYLE_FILL;

	gog_xyz_plot_klass->third_axis = GOG_AXIS_Z;
	gog_xyz_plot_klass->build_matrix = gog_surface_plot_build_matrix;
}

static void
gog_surface_plot_init (GogSurfacePlot *surface)
{
	GogPlot *plot = GOG_PLOT (surface);

	plot->render_before_axes = FALSE;
}

GSF_DYNAMIC_CLASS (GogSurfacePlot, gog_surface_plot,
	gog_surface_plot_class_init, gog_surface_plot_init,
	GOG_XYZ_PLOT_TYPE)

/*****************************************************************************/

typedef GogPlotView		GogSurfaceView;
typedef GogPlotViewClass	GogSurfaceViewClass;

static void
gog_surface_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogSurfacePlot const *plot = GOG_SURFACE_PLOT (view->model);
	GogSeries const *series;
	GogChartMap3D *chart_map;
	GogChart *chart = GOG_CHART (view->model->parent);
	GogViewAllocation const *area;
	int i, imax, j, jmax, max, istep, jstep, jstart, iend, jend;
	double x[2], y[2], z[3], x0, y0, x1, y1;
	GogRenderer *rend = view->renderer;
	GogStyle *style;
	double *data;
	GODataVector *x_vec = NULL, *y_vec = NULL;
	gboolean xdiscrete, ydiscrete;
	GOPath *path;
	gboolean cw;

	if (plot->base.series == NULL)
		return;
	series = GOG_SERIES (plot->base.series->data);
	max = series->num_elements;
	if (plot->transposed) {
		imax = plot->rows;
		jmax = plot->columns;
	} else {
		imax = plot->columns;
		jmax = plot->rows;
	}
	if (imax ==0 || jmax == 0)
		return;
	area = gog_chart_view_get_plot_area (view->parent);
	chart_map = gog_chart_map_3d_new (chart, area,
				       GOG_PLOT (plot)->axis[GOG_AXIS_X],
				       GOG_PLOT (plot)->axis[GOG_AXIS_Y],
				       GOG_PLOT (plot)->axis[GOG_AXIS_Z]);
	if (!gog_chart_map_3d_is_valid (chart_map)) {
		gog_chart_map_3d_free (chart_map);
		return;
	}


	style = gog_styled_object_get_style (GOG_STYLED_OBJECT (series));

	if (plot->plotted_data)
		data = plot->plotted_data;
	else
		data = gog_xyz_plot_build_matrix (plot, &cw);

	/* check the drawing order */
	xdiscrete = gog_axis_is_discrete (plot->base.axis[0]) ||
			series->values[(plot->transposed)? 1: 0].data == NULL;
	if (xdiscrete) {
		x[0] = 0.;
		x[1] = 1.;
	}else {
		x_vec = GO_DATA_VECTOR (series->values[(plot->transposed)? 1: 0].data);
		x[0] = go_data_vector_get_value (x_vec, 0);
		x[1] = go_data_vector_get_value (x_vec, 1);
	}
	ydiscrete = gog_axis_is_discrete (plot->base.axis[1]) ||
			series->values[(plot->transposed)? 0: 1].data == NULL;
	if (ydiscrete) {
		y[0] = 0.;
		y[1] = 1.;
	}else {
		y_vec = GO_DATA_VECTOR (series->values[(plot->transposed)? 0: 1].data);
		y[0] = go_data_vector_get_value (y_vec, 0);
		y[1] = go_data_vector_get_value (y_vec, 1);
	}
	gog_chart_map_3d_to_view (chart_map, x[0], y[0], data[0], NULL, NULL, z + 1); 
	gog_chart_map_3d_to_view (chart_map, x[1], y[0], data[0], NULL, NULL, z + 2);
	if (z[2] > z[1]) {
		i = imax - 1;
		iend = 0;
		istep = -1;
	} else {
		i = 1;
		iend = imax;
		istep = 1;
	}
	gog_chart_map_3d_to_view (chart_map, x[0], y[1], data[0], NULL, NULL, z + 2);
	if (z[2] > z[1]) {
		jstart = jmax - 1;
		jend = 0;
		jstep = -1;
	} else {
		jstart = 1;
		jstep = 1;
		jend = jmax;
	}
	gog_renderer_push_style (rend, style);
	for (; i != iend; i +=istep)
		for (j = jstart; j != jend; j += jstep) {
			path = go_path_new ();
			if (xdiscrete) {
				x0 = i;
				x1 = i + 1;
			} else {
				x0 = go_data_vector_get_value (x_vec, i - 1);
				x1 = go_data_vector_get_value (x_vec, i);
			}
			if (ydiscrete) {
				y0 = j;
				y1 = j + 1;
			} else {
				y0 = go_data_vector_get_value (y_vec, j - 1);
				y1 = go_data_vector_get_value (y_vec, j);
			}
			gog_chart_map_3d_to_view (chart_map, x0, y0,
						  data[(j - 1) * imax + i - 1], x, y, NULL);
			go_path_move_to (path, *x, *y);
			gog_chart_map_3d_to_view (chart_map, x1, y0,
						  data[(j - 1) * imax + i], x, y, NULL);
			go_path_line_to (path, *x, *y);
			gog_chart_map_3d_to_view (chart_map, x1, y1,
						  data[j * imax + i], x, y, NULL);
			go_path_line_to (path, *x, *y);
			gog_chart_map_3d_to_view (chart_map, x0, y1,
						  data[j * imax + i - 1], x, y, NULL);
			go_path_line_to (path, *x, *y);
			go_path_close (path);
			gog_renderer_draw_shape (rend, path);
			go_path_free (path);
			//break;
		}
	gog_renderer_pop_style (rend);
	gog_chart_map_3d_free (chart_map);
}

static void
gog_surface_view_class_init (GogViewClass *view_klass)
{
	view_klass->render = gog_surface_view_render;
}

GSF_DYNAMIC_CLASS (GogSurfaceView, gog_surface_view,
	gog_surface_view_class_init, NULL,
	GOG_PLOT_VIEW_TYPE)
