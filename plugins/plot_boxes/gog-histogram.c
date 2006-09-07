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
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-series-lines.h>
#include <goffice/utils/go-format.h>
#include <goffice/utils/go-math.h>
#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <libart_lgpl/libart.h>

#define GOG_HISTOGRAM_PLOT_SERIES_TYPE	(gog_histogram_plot_series_get_type ())
#define GOG_HISTOGRAM_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_HISTOGRAM_PLOT_SERIES_TYPE, GogHistogramPlotSeries))
#define IS_GOG_HISTOGRAM_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_HISTOGRAM_PLOT_SERIES_TYPE))

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
	
	if (series->x) {
		g_free (series->x);
		series->x = NULL;
	}
	series->x = g_new (double, series->base.num_elements);
	if (series->base.values[0].data != NULL) {
		x_vals = go_data_vector_get_values (GO_DATA_VECTOR (series->base.values[0].data));
		x_min = x_vals[0];
		x_max = x_vals[series->base.num_elements];
		if (model->x.fmt == NULL)
			model->x.fmt = go_data_preferred_fmt (series->base.values[0].data);
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
	if (series->y) {
		g_free (series->y);
		series->y = NULL;
	}
	if (series->base.values[1].data != NULL) {
		if (x_vals) {
			series->y = g_new (double, series->base.num_elements);
			y_vals = go_data_vector_get_values (GO_DATA_VECTOR (series->base.values[1].data));
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
			go_data_vector_get_minmax (
				GO_DATA_VECTOR (series->base.values[1].data), &y_min, &y_max);
		if (model->y.fmt == NULL)
			model->y.fmt = go_data_preferred_fmt (series->base.values[1].data);
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
	} else {
		bounds->val.minima = model->y.minima;
		bounds->val.maxima = model->y.maxima;
		if (bounds->fmt == NULL && model->y.fmt != NULL)
			bounds->fmt = go_format_ref (model->y.fmt);
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
	plot_klass->desc.num_series_min = 1;
	plot_klass->desc.num_series_max = 1;
	plot_klass->series_type = gog_histogram_plot_series_get_type ();
	plot_klass->axis_set = GOG_AXIS_SET_XY;
	plot_klass->desc.series.style_fields	= GOG_STYLE_LINE | GOG_STYLE_FILL;
	plot_klass->axis_get_bounds   		= gog_histogram_plot_axis_get_bounds;
}

GSF_DYNAMIC_CLASS (GogHistogramPlot, gog_histogram_plot,
	gog_histogram_plot_class_init, NULL,
	GOG_PLOT_TYPE)

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
	double *x_vals = NULL, *y_vals, curx, cury;
	unsigned i, j, nb;
	ArtVpath *path ;
	GSList *ptr;
	GogStyle *style;

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
		x_vals = go_data_vector_get_values (
			GO_DATA_VECTOR (series->base.values[0].data));
	y_vals = (x_vals)? series->y: go_data_vector_get_values (
		GO_DATA_VECTOR (series->base.values[1].data));

	path = art_new (ArtVpath, nb);
	path[0].code = ART_MOVETO;
	curx = path[0].x = gog_axis_map_to_view (x_map, ((x_vals)? x_vals[0]: 0.));
	path[0].y = gog_axis_map_get_baseline (y_map);
	for (i = 0, j = 1; i < series->base.num_elements; i++) {
		path[j].code = ART_LINETO;
		path[j].x = curx;
		cury = path[j++].y = gog_axis_map_to_view (y_map, y_vals[i]);
		path[j].code = ART_LINETO;
		curx = path[j].x = gog_axis_map_to_view (x_map, ((x_vals)? x_vals[i+1]: 0.));
		path[j++].y = cury;
	}
	path[j].code = ART_LINETO;
	path[j].x = curx;
	path[j++].y = path[0].y;
	path[j].code = ART_LINETO;
	path[j].x = path[0].x;
	path[j++].y = path[0].y;
	path[j].code = ART_END;
	gog_renderer_push_style (view->renderer, style);
	gog_renderer_draw_sharp_polygon (view->renderer, path, FALSE);

	if (series->droplines) {
		ArtVpath droppath[3];
		droppath[0].code = ART_MOVETO;
		droppath[1].code = ART_LINETO;
		droppath[2].code = ART_END;
		gog_renderer_push_style (view->renderer,
			gog_styled_object_get_style (GOG_STYLED_OBJECT (series->droplines)));
		for (i = 1; i < series->base.num_elements; i++) {
			droppath[0].x = droppath[1].x =
				gog_axis_map_to_view (x_map, ((x_vals)? x_vals[i]: 0.));
			if (y_vals[i-1] * y_vals[i] > 0.) {
				droppath[0].y = path[0].y;
				if (y_vals[i] > 0.)
					droppath[1].y = gog_axis_map_to_view (y_map,
						MIN (y_vals[i-1], y_vals[i]));
				else
					droppath[1].y = gog_axis_map_to_view (y_map,
						MAX (y_vals[i-1], y_vals[i]));
			} else {
				droppath[0].y = gog_axis_map_to_view (y_map, y_vals[i-1]);
				droppath[1].y = gog_axis_map_to_view (y_map, y_vals[i]);
			}
			gog_renderer_draw_path (view->renderer, droppath);
		}
		gog_renderer_pop_style (view->renderer);
	}
/*	int nb = series->base.num_elements - 1;
	x = g_new (double, nb);
	y = g_new (double, nb);

	g_free (x);
	g_free (y);*/
	j--;
	path[j].code = ART_END;
	gog_renderer_draw_path (view->renderer, path);
	gog_renderer_pop_style (view->renderer);
	art_free (path);
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
	GOG_PLOT_VIEW_TYPE)

/*****************************************************************************/

typedef GogView		GogHistogramSeriesView;
typedef GogViewClass	GogHistogramSeriesViewClass;

#define GOG_HISTOGRAM_SERIES_VIEW_TYPE	(gog_histogram_series_view_get_type ())
#define GOG_HISTOGRAM_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_HISTOGRAM_SERIES_VIEW_TYPE, GogHistogramSeriesView))
#define IS_GOG_HISTOGRAM_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_HISTOGRAM_SERIES_VIEW_TYPE))

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
	GOG_VIEW_TYPE)

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
		y_vals = go_data_vector_get_values (GO_DATA_VECTOR (series->base.values[1].data));
		y_len = go_data_vector_get_len (
			GO_DATA_VECTOR (series->base.values[1].data));
	}
	if (series->base.values[0].data != NULL) {
		x_vals = go_data_vector_get_values (GO_DATA_VECTOR (series->base.values[0].data));
		max = go_data_vector_get_len 
			(GO_DATA_VECTOR (series->base.values[0].data));
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
		if (!IS_GOG_SERIES_LINES (ptr->data))
			gog_object_request_update (GOG_OBJECT (ptr->data));

	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));
	if (old_num != series->base.num_elements)
		gog_plot_request_cardinality_update (series->base.plot);

	if (gog_histogram_plot_series_parent_klass->update)
		gog_histogram_plot_series_parent_klass->update (obj);
}

static void
gog_histogram_plot_series_init_style (GogStyledObject *gso, GogStyle *style)
{
	((GogStyledObjectClass*) gog_histogram_plot_series_parent_klass)->init_style (gso, style);

	style->outline.dash_type = GO_LINE_NONE;
}

static void
gog_histogram_plot_series_finalize (GObject *obj)
{
	GogHistogramPlotSeries *series = GOG_HISTOGRAM_PLOT_SERIES (obj);

	if (series->y != NULL) {
		g_free (series->y); 
		series->y = NULL;
	}
	if (series->x != NULL) {
		g_free (series->x); 
		series->x = NULL;
	}

	G_OBJECT_CLASS (series_parent_klass)->finalize (obj);
}

static unsigned
gog_histogram_plot_series_get_xy_data (GogSeries const *series,
					double const **x, double const **y)
{
	GogHistogramPlotSeries *hist_ser = GOG_HISTOGRAM_PLOT_SERIES (series);
	*x = hist_ser->x;
	*y = (hist_ser->y)? hist_ser->y:
		go_data_vector_get_values (GO_DATA_VECTOR (series->values[1].data));
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
	GogStyledObjectClass *gso_klass = (GogStyledObjectClass*) obj_klass;
	GogObjectClass *gog_klass = (GogObjectClass *)gso_klass;
	GObjectClass *gobject_klass = (GObjectClass *) gso_klass;

	series_parent_klass = g_type_class_peek_parent (obj_klass);
	gobject_klass->finalize		= gog_histogram_plot_series_finalize;

	gog_histogram_plot_series_parent_klass = g_type_class_peek_parent (obj_klass);
	obj_klass->update = gog_histogram_plot_series_update;
	gog_klass->view_type	= gog_histogram_series_view_get_type ();
	gso_klass->init_style = gog_histogram_plot_series_init_style;

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
	GOG_SERIES_TYPE)
