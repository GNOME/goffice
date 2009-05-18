/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-histogram.c
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-histogram.h"
#include <goffice/data/go-data.h>
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-series-lines.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-format.h>
#include <goffice/utils/go-path.h>
#include <goffice/utils/go-styled-object.h>
#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#define GOG_TYPE_HISTOGRAM_PLOT_SERIES	(gog_histogram_plot_series_get_type ())
#define GOG_HISTOGRAM_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_HISTOGRAM_PLOT_SERIES, GogHistogramPlotSeries))
#define GOG_IS_HISTOGRAM_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_HISTOGRAM_PLOT_SERIES))

typedef struct {
	GogSeries base;
	GogObject *droplines;
	double *x, *y;
} GogHistogramPlotSeries;
typedef GogSeriesClass GogHistogramPlotSeriesClass;

GType gog_histogram_plot_series_get_type (void);
GType gog_histogram_plot_view_get_type (void);

static GogObjectClass *histogram_plot_parent_klass;

static void
gog_histogram_plot_clear_formats (GogHistogramPlot *model)
{
	if (model->x.fmt != NULL) {
		go_format_unref (model->x.fmt);
		model->x.fmt = NULL;
	}
	if (model->y.fmt != NULL) {
		go_format_unref (model->y.fmt);
		model->y.fmt = NULL;
	}
}

static char const *
gog_histogram_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name histogram-plot objects
	 * eg The 2nd histogram-plot in a chart will be called
	 * 	Histogram2 */
	return N_("Histogram");
}

static void
gog_histogram_plot_update (GogObject *obj)
{
	GogHistogramPlot *model = GOG_HISTOGRAM_PLOT (obj);
	GogHistogramPlotSeries *series = GOG_HISTOGRAM_PLOT_SERIES (
		model->base.series->data);
	double x_min, x_max, y_min = DBL_MAX, y_max = -DBL_MAX, *x_vals, *y_vals, val;
	unsigned i;

	if (!gog_series_is_valid (GOG_SERIES (series)) || series->base.num_elements == 0)
			return;
	
	g_free (series->x);
	series->x = g_new (double, series->base.num_elements);
	if (series->base.values[0].data != NULL) {
		x_vals = go_data_get_values (series->base.values[0].data);
		x_min = x_vals[0];
		x_max = x_vals[series->base.num_elements];
		if (model->x.fmt == NULL)
			model->x.fmt = go_data_preferred_fmt (series->base.values[0].data);
		model->x.date_conv = go_data_date_conv (series->base.values[0].data);
		for (i = 0; i < series->base.num_elements; i++)
			series->x[i] = (x_vals[i] + x_vals[i+1]) / 2;
	} else {
		x_vals = NULL;
		x_min = 0;
		x_max = series->base.num_elements;
		for (i = 0; i < series->base.num_elements; i++)
			series->x[i] = (double) i + 0.5;
	}
	if (model->x.minima != x_min || model->x.maxima != x_max) {
		model->x.minima = x_min;
		model->x.maxima = x_max;
		gog_axis_bound_changed (model->base.axis[0], GOG_OBJECT (model));
	}
	g_free (series->y);
	series->y = NULL;
	if (series->base.values[1].data != NULL) {
		if (x_vals) {
			series->y = g_new (double, series->base.num_elements);
			y_vals = go_data_get_values (series->base.values[1].data);
			for (i = 0; i < series->base.num_elements; i++)
				if (go_finite (y_vals[i])) {
					series->y[i] = val = y_vals[i] / (x_vals[i+1] - x_vals[i]);
					if (val < y_min)
						y_min = val;
					if (val > y_max)
						y_max = val;
				} else
					series->y[i] = 0.;
		} else
			go_data_get_bounds (series->base.values[1].data, &y_min, &y_max);
		if (model->y.fmt == NULL)
			model->y.fmt = go_data_preferred_fmt (series->base.values[1].data);
		model->y.date_conv = go_data_date_conv (series->base.values[1].data);
	}
	if (y_min > y_max)
		y_min = y_max = go_nan;
	if (model->y.minima != y_min || model->y.maxima != y_max) {
		model->y.minima = y_min;
		model->y.maxima = y_max;
		gog_axis_bound_changed (model->base.axis[1], GOG_OBJECT (model));
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static GOData *
gog_histogram_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			      GogPlotBoundInfo *bounds)
{
	GogHistogramPlot *model = GOG_HISTOGRAM_PLOT (plot);


	if (axis == GOG_AXIS_X) {
		bounds->val.minima = model->x.minima;
		bounds->val.maxima = model->x.maxima;
		if (bounds->fmt == NULL && model->x.fmt != NULL)
			bounds->fmt = go_format_ref (model->x.fmt);
		if (model->x.date_conv)
			bounds->date_conv = model->x.date_conv;
	} else {
		bounds->val.minima = model->y.minima;
		bounds->val.maxima = model->y.maxima;
		if (bounds->fmt == NULL && model->y.fmt != NULL)
			bounds->fmt = go_format_ref (model->y.fmt);
		if (model->y.date_conv)
			bounds->date_conv = model->y.date_conv;
	}
	bounds->is_discrete = FALSE;
	return NULL;
}

static void
gog_histogram_plot_finalize (GObject *obj)
{
	gog_histogram_plot_clear_formats (GOG_HISTOGRAM_PLOT (obj));
	G_OBJECT_CLASS (histogram_plot_parent_klass)->finalize (obj);
}

static void
gog_histogram_plot_class_init (GogPlotClass *gog_plot_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) gog_plot_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) gog_plot_klass;
	GogPlotClass   *plot_klass = (GogPlotClass *) gog_plot_klass;
	
	histogram_plot_parent_klass = g_type_class_peek_parent (gog_plot_klass);

	gobject_klass->finalize = gog_histogram_plot_finalize;

	gog_object_klass->type_name	= gog_histogram_plot_type_name;
	gog_object_klass->view_type	= gog_histogram_plot_view_get_type ();
	gog_object_klass->update	= gog_histogram_plot_update;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Limits"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("Values"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
	}
	plot_klass->desc.num_series_max = 1;
	plot_klass->series_type = gog_histogram_plot_series_get_type ();
	plot_klass->axis_set = GOG_AXIS_SET_XY;
	plot_klass->desc.series.style_fields	= GO_STYLE_OUTLINE | GO_STYLE_FILL;
	plot_klass->axis_get_bounds   		= gog_histogram_plot_axis_get_bounds;
}

static void
gog_histogram_plot_init (GogHistogramPlot *hist)
{
	GogPlot *plot = GOG_PLOT (hist);

	plot->render_before_axes = TRUE;
}

GSF_DYNAMIC_CLASS (GogHistogramPlot, gog_histogram_plot,
	gog_histogram_plot_class_init, gog_histogram_plot_init,
	GOG_TYPE_PLOT)

/*****************************************************************************/
typedef GogPlotView		GogHistogramPlotView;
typedef GogPlotViewClass	GogHistogramPlotViewClass;

static void
gog_histogram_plot_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogHistogramPlot const *model = GOG_HISTOGRAM_PLOT (view->model);
	GogChart *chart = GOG_CHART (view->model->parent);
	GogChartMap *chart_map;
	GogAxisMap *x_map, *y_map;
	GogViewAllocation const *area;
	GogHistogramPlotSeries const *series;
	double *x_vals = NULL, *y_vals, curx, cury, y0;
	unsigned i, j, nb;
	GOPath *path ;
	GSList *ptr;
	GOStyle *style;

	if (model->base.series == NULL)
		return;
	series = GOG_HISTOGRAM_PLOT_SERIES (model->base.series->data);
	nb = (series->base.num_elements + 1) * 2 + 2;
	style = GOG_STYLED_OBJECT (series)->style;
	if (series->base.num_elements < 1)
		return;
	area = gog_chart_view_get_plot_area (view->parent);
	chart_map = gog_chart_map_new (chart, area, 
				       GOG_PLOT (model)->axis[GOG_AXIS_X], 
				       GOG_PLOT (model)->axis[GOG_AXIS_Y],
				       NULL, FALSE);
	if (!gog_chart_map_is_valid (chart_map)) {
		gog_chart_map_free (chart_map);
		return;
	}

	x_map = gog_chart_map_get_axis_map (chart_map, 0);
	y_map = gog_chart_map_get_axis_map (chart_map, 1);

	if (series->base.values[0].data)
		x_vals = go_data_get_values (series->base.values[0].data);
	y_vals = (x_vals)? series->y: go_data_get_values (series->base.values[1].data);

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);
	curx = gog_axis_map_to_view (x_map, ((x_vals)? x_vals[0]: 0.));
	go_path_move_to (path, curx, y0 = gog_axis_map_get_baseline (y_map));
	for (i = 0, j = 1; i < series->base.num_elements; i++) {
		cury = gog_axis_map_to_view (y_map, y_vals[i]);
		go_path_line_to (path, curx, cury);
		curx = gog_axis_map_to_view (x_map, ((x_vals)? x_vals[i+1]: 0.));
		go_path_line_to (path, curx, cury);
	}
	go_path_line_to (path, curx, y0);
	gog_renderer_push_style (view->renderer, style);
	gog_renderer_fill_shape (view->renderer, path);

	if (series->droplines) {
		GOPath *drop_path = go_path_new ();
		go_path_set_options (drop_path, GO_PATH_OPTIONS_SHARP);
		gog_renderer_push_style (view->renderer,
			go_styled_object_get_style (GO_STYLED_OBJECT (series->droplines)));
		cury = y0;
		for (i = 1; i < series->base.num_elements; i++) {
			curx = gog_axis_map_to_view (x_map, ((x_vals)? x_vals[i]: 0.));
			if (y_vals[i-1] * y_vals[i] > 0.) {
				go_path_move_to (drop_path, curx, y0);
				if (y_vals[i] > 0.)
					cury = gog_axis_map_to_view (y_map,
						MIN (y_vals[i-1], y_vals[i]));
				else
					cury = gog_axis_map_to_view (y_map,
						MAX (y_vals[i-1], y_vals[i]));
				
			} else {
				go_path_move_to (drop_path, curx, cury);
				cury = gog_axis_map_to_view (y_map, y_vals[i]);
			}
			go_path_line_to (drop_path, curx, cury);
		}
		gog_renderer_stroke_serie (view->renderer, drop_path);
		go_path_free (drop_path);
		gog_renderer_pop_style (view->renderer);
	}
	gog_renderer_stroke_shape (view->renderer, path);
	gog_renderer_pop_style (view->renderer);
	go_path_free (path);
	/* Now render children */
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);

	gog_chart_map_free (chart_map);
}

static GogViewClass *histogram_plot_view_parent_klass;

static void
gog_histogram_plot_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;
	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
	(histogram_plot_view_parent_klass->size_allocate) (view, allocation);
}

static void
gog_histogram_plot_view_class_init (GogViewClass *view_klass)
{
	histogram_plot_view_parent_klass = (GogViewClass*) g_type_class_peek_parent (view_klass);
	view_klass->render	  = gog_histogram_plot_view_render;
	view_klass->size_allocate = gog_histogram_plot_view_size_allocate;
	view_klass->clip	  = FALSE;
}

GSF_DYNAMIC_CLASS (GogHistogramPlotView, gog_histogram_plot_view,
	gog_histogram_plot_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)

/*****************************************************************************/

typedef GogView		GogHistogramSeriesView;
typedef GogViewClass	GogHistogramSeriesViewClass;

#define GOG_TYPE_HISTOGRAM_SERIES_VIEW	(gog_histogram_series_view_get_type ())
#define GOG_HISTOGRAM_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_HISTOGRAM_SERIES_VIEW, GogHistogramSeriesView))
#define GOG_IS_HISTOGRAM_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_HISTOGRAM_SERIES_VIEW))

static void
gog_histogram_series_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr;
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);
}

static void
gog_histogram_series_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;

	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
}

static void
gog_histogram_series_view_class_init (GogHistogramSeriesViewClass *gview_klass)
{
	GogViewClass *view_klass = GOG_VIEW_CLASS (gview_klass);
	view_klass->render = gog_histogram_series_view_render;
	view_klass->size_allocate = gog_histogram_series_view_size_allocate;
	view_klass->build_toolkit = NULL;
}

GSF_DYNAMIC_CLASS (GogHistogramSeriesView, gog_histogram_series_view,
	gog_histogram_series_view_class_init, NULL,
	GOG_TYPE_VIEW)

/*****************************************************************************/

static gboolean
drop_lines_can_add (GogObject const *parent)
{
	GogHistogramPlotSeries *series = GOG_HISTOGRAM_PLOT_SERIES (parent);
	return (series->droplines == NULL);
}

static void
drop_lines_post_add (GogObject *parent, GogObject *child)
{
	GogHistogramPlotSeries *series = GOG_HISTOGRAM_PLOT_SERIES (parent);
	series->droplines = child;
	gog_object_request_update (child);
}

static void
drop_lines_pre_remove (GogObject *parent, GogObject *child)
{
	GogHistogramPlotSeries *series = GOG_HISTOGRAM_PLOT_SERIES (parent);
	series->droplines = NULL;
}

/****************************************************************************/

static GogObjectClass *gog_histogram_plot_series_parent_klass;

static GObjectClass *series_parent_klass;

static void
gog_histogram_plot_series_update (GogObject *obj)
{
	double *x_vals = NULL, *y_vals = NULL, cur;
	int x_len = 1, y_len = 0, i, max;
	GogHistogramPlotSeries *series = GOG_HISTOGRAM_PLOT_SERIES (obj);
	unsigned old_num = series->base.num_elements;
	GSList *ptr;

	if (series->base.values[1].data != NULL) {
		y_vals = go_data_get_values (series->base.values[1].data);
		y_len = go_data_get_vector_size (series->base.values[1].data);
	}
	if (series->base.values[0].data != NULL) {
		x_vals = go_data_get_values (series->base.values[0].data);
		max = go_data_get_vector_size (series->base.values[0].data);
		if (max > 0 && go_finite (x_vals[0])) {
			cur = x_vals[0];
			for (i = 1; i< max; i++) {
				if (go_finite (x_vals[i]) && x_vals[i] > cur) {
					cur = x_vals[i];
					x_len++;
				} else
					break;
			}
		}
	} else
		x_len = y_len + 1;
	series->base.num_elements = MIN (x_len - 1, y_len);

	/* update children */
	for (ptr = obj->children; ptr != NULL; ptr = ptr->next)
		if (!GOG_IS_SERIES_LINES (ptr->data))
			gog_object_request_update (GOG_OBJECT (ptr->data));

	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));
	if (old_num != series->base.num_elements)
		gog_plot_request_cardinality_update (series->base.plot);

	if (gog_histogram_plot_series_parent_klass->update)
		gog_histogram_plot_series_parent_klass->update (obj);
}

static void
gog_histogram_plot_series_finalize (GObject *obj)
{
	GogHistogramPlotSeries *series = GOG_HISTOGRAM_PLOT_SERIES (obj);

	g_free (series->y);
	series->y = NULL;
	g_free (series->x);
	series->x = NULL;

	G_OBJECT_CLASS (series_parent_klass)->finalize (obj);
}

static unsigned
gog_histogram_plot_series_get_xy_data (GogSeries const *series,
					double const **x, double const **y)
{
	GogHistogramPlotSeries *hist_ser = GOG_HISTOGRAM_PLOT_SERIES (series);

	*x = hist_ser->x;
	*y = (hist_ser->y)? hist_ser->y: go_data_get_values (series->values[1].data);

	return series->num_elements;
}

static void
gog_histogram_plot_series_class_init (GogObjectClass *obj_klass)
{
	static GogObjectRole const roles[] = {
		{ N_("Drop lines"), "GogSeriesLines", 0,
			GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
			drop_lines_can_add,
			NULL,
			NULL,
			drop_lines_post_add,
			drop_lines_pre_remove,
			NULL },
	};
	GogSeriesClass *series_klass = (GogSeriesClass*) obj_klass;
	GogObjectClass *gog_klass = (GogObjectClass *)obj_klass;
	GObjectClass *gobject_klass = (GObjectClass *) obj_klass;

	series_parent_klass = g_type_class_peek_parent (obj_klass);
	gobject_klass->finalize		= gog_histogram_plot_series_finalize;

	gog_histogram_plot_series_parent_klass = g_type_class_peek_parent (obj_klass);
	obj_klass->update = gog_histogram_plot_series_update;
	gog_klass->view_type	= gog_histogram_series_view_get_type ();

	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));

	series_klass->get_xy_data = gog_histogram_plot_series_get_xy_data;
}

static void
gog_histogram_plot_series_init (GObject *obj)
{
	GogSeries *series = GOG_SERIES (obj);

	series->acceptable_children = GOG_SERIES_ACCEPT_TREND_LINE;
}

GSF_DYNAMIC_CLASS (GogHistogramPlotSeries, gog_histogram_plot_series,
	gog_histogram_plot_series_class_init, gog_histogram_plot_series_init,
	GOG_TYPE_SERIES)
