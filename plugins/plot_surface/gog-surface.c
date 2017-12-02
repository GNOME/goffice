/*
 * gog-surface.c
 *
 * Copyright (C) 2004-2005 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-surface.h"

#include <goffice/data/go-data.h>
#include <goffice/graph/gog-chart-map-3d.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-path.h>
#include <goffice/utils/go-styled-object.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

/*****************************************************************************/

static GType gog_surface_view_get_type (void);

static double *
gog_surface_plot_build_matrix (GogXYZPlot *plot, gboolean *cardinality_changed)
{
	unsigned i, j;
	double val;
	GogSeries *series = GOG_SERIES (plot->base.series->data);
	GOData *mat = series->values[2].data;
	unsigned n = plot->rows * plot->columns;
	double *data;

	data = g_new (double, n);

	for (i = 0; i < plot->rows; i++)
		for (j = 0; j < plot->columns; j++) {
			val = go_data_get_matrix_value (mat, i, j);
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
	gog_plot_klass->desc.series.style_fields = GO_STYLE_LINE | GO_STYLE_FILL;

	gog_xyz_plot_klass->third_axis = GOG_AXIS_Z;
	gog_xyz_plot_klass->build_matrix = gog_surface_plot_build_matrix;
}

static void
gog_surface_plot_init (GogSurfacePlot *surface)
{
	GogPlot *plot = GOG_PLOT (surface);

	plot->rendering_order = GOG_PLOT_RENDERING_LAST;
}

GSF_DYNAMIC_CLASS (GogSurfacePlot, gog_surface_plot,
	gog_surface_plot_class_init, gog_surface_plot_init,
	GOG_TYPE_XYZ_PLOT)

/*****************************************************************************/

typedef GogPlotView		GogSurfaceView;
typedef GogPlotViewClass	GogSurfaceViewClass;

typedef struct {
	GOPath *path;
	double distance;
} GogSurfaceTile;

static int
tile_cmp (GogSurfaceTile *t1, GogSurfaceTile *t2)
{
	if (t1->distance > t2->distance)
		return -1;
	if (t1->distance == t2->distance)
		return 0;
	return 1;
}

static void
gog_surface_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogSurfacePlot *plot = GOG_SURFACE_PLOT (view->model);
	GogSeries const *series;
	GogChartMap3D *chart_map;
	GogViewAllocation const *area;
	int i, imax, j, jmax, nbvalid;
	double x, y, z, x0, y0, x1, y1;
	GogRenderer *rend = view->renderer;
	GOStyle *style;
	double *data;
	GOData *x_vec = NULL, *y_vec = NULL;
	gboolean xdiscrete, ydiscrete;
	GSList *tiles = NULL, *cur;
	GogSurfaceTile *tile;

	if (plot->base.series == NULL)
		return;
	series = GOG_SERIES (plot->base.series->data);
	if (plot->transposed) {
		imax = plot->rows;
		jmax = plot->columns;
	} else {
		imax = plot->columns;
		jmax = plot->rows;
	}
	if (imax == 0 || jmax == 0)
		return;
	area = gog_chart_view_get_plot_area (view->parent);

	if (plot->plotted_data)
		data = plot->plotted_data;
	else
		return;

	chart_map = gog_chart_map_3d_new (view, area,
				       GOG_PLOT (plot)->axis[GOG_AXIS_X],
				       GOG_PLOT (plot)->axis[GOG_AXIS_Y],
				       GOG_PLOT (plot)->axis[GOG_AXIS_Z]);
	if (!gog_chart_map_3d_is_valid (chart_map)) {
		gog_chart_map_3d_free (chart_map);
		return;
	}

	style = go_styled_object_get_style (GO_STYLED_OBJECT (series));

	/* Build the tiles list */
	x_vec = gog_xyz_plot_get_x_vals (plot);
	y_vec = gog_xyz_plot_get_y_vals (plot);
	xdiscrete = gog_axis_is_discrete (plot->base.axis[0]) ||
			x_vec == NULL;
	ydiscrete = gog_axis_is_discrete (plot->base.axis[1]) ||
			y_vec == NULL;
	for (i = 1; i < imax; i++)
		for (j = 1; j < jmax; j++) {
			tile = g_new0 (GogSurfaceTile, 1);
			tile->path = go_path_new ();
			if (xdiscrete) {
				x0 = i;
				x1 = i + 1;
			} else {
				x0 = go_data_get_vector_value (x_vec, i - 1);
				x1 = go_data_get_vector_value (x_vec, i);
			}
			if (ydiscrete) {
				y0 = j;
				y1 = j + 1;
			} else {
				y0 = go_data_get_vector_value (y_vec, j - 1);
				y1 = go_data_get_vector_value (y_vec, j);
			}
			nbvalid = 0;
			z = data[(j - 1) * imax + i - 1];
			if (!isnan (z) && go_finite (z)) {
				gog_chart_map_3d_to_view (chart_map, x0, y0, z, &x, &y, &z);
				go_path_move_to (tile->path, x, y);
				nbvalid = 1;
				tile->distance = z;
			}
			z = data[(j - 1) * imax + i];
			if (!isnan (z) && go_finite (z)) {
				gog_chart_map_3d_to_view (chart_map, x1, y0, z, &x, &y, &z);
				if (nbvalid)
					go_path_line_to (tile->path, x, y);
				else
					go_path_move_to (tile->path, x, y);
				nbvalid++;
				tile->distance += z;
			}
			z = data[j * imax + i];
			if (!isnan (z) && go_finite (z)) {
				gog_chart_map_3d_to_view (chart_map, x1, y1, z, &x, &y, &z);
				if (nbvalid)
					go_path_line_to (tile->path, x, y);
				else
					go_path_move_to (tile->path, x, y);
				nbvalid++;
				tile->distance += z;
			}
			z = data[j * imax + i - 1];
			if (!isnan (z) && go_finite (z)) {
				gog_chart_map_3d_to_view (chart_map, x0, y1, z, &x, &y, &z);
				if (nbvalid)
					go_path_line_to (tile->path, x, y);
				else
					go_path_move_to (tile->path, x, y);
				nbvalid++;
				tile->distance += z;
			}
			if (nbvalid) {
				go_path_close (tile->path);
				tile->distance /= nbvalid;
				tiles = g_slist_prepend (tiles, tile);
			} else {
				go_path_free (tile->path);
				g_free (tile);
			}
		}

	/* Sort the tiles */
	tiles = g_slist_sort (tiles, (GCompareFunc) tile_cmp);

	/* Render the tiles and free memory */
	cur = tiles;
	gog_renderer_push_style (rend, style);
	while (cur) {
		tile = (GogSurfaceTile *) cur->data;
		gog_renderer_draw_shape (rend, tile->path);
		go_path_free (tile->path);
		g_free (tile);

		cur = cur->next;
	}
	g_slist_free (tiles);

	gog_renderer_pop_style (rend);
	gog_chart_map_3d_free (chart_map);
	if (!plot->plotted_data)
		g_free (data);
}

static void
gog_surface_view_class_init (GogViewClass *view_klass)
{
	view_klass->render = gog_surface_view_render;
}

GSF_DYNAMIC_CLASS (GogSurfaceView, gog_surface_view,
	gog_surface_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)
