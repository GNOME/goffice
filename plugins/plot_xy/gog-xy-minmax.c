/*
 * gog-xy-minmax.c
 *
 * Copyright (C) 2012
 *	Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-xy-minmax.h"

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

struct _GogXYMinMaxPlot {
	GogPlot base;

	gboolean horizontal;
	gboolean default_style_has_markers;
	struct axes_data {
		double minima, maxima;
		GOFormat const *fmt;
		GODateConventions const *date_conv;
	} x, y;
};

enum {
	XY_MINMAX_PROP_0,
	XY_MINMAX_PROP_HORIZONTAL,
	XY_MINMAX_PROP_DEFAULT_STYLE_HAS_MARKERS
};

static GogObjectClass *gog_xy_minmax_parent_klass;

static GType gog_xy_minmax_series_get_type (void);
static GType gog_xy_minmax_view_get_type (void);

typedef GogSeries		GogXYMinMaxSeries;
typedef GogSeriesClass	GogXYMinMaxSeriesClass;

static GogObjectClass *series_parent_klass;

static void
gog_xy_minmax_series_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogSeries *series = GOG_SERIES (gso);
	GogXYMinMaxPlot const *plot;

	((GogStyledObjectClass *)series_parent_klass)->init_style (gso, style);
	if (series->plot == NULL)
		return;

	plot = GOG_XY_MINMAX_PLOT (series->plot);
	if (!plot->default_style_has_markers) {
		style->disable_theming |= GO_STYLE_MARKER;
		if (style->marker.auto_shape) {
			GOMarker *m = go_marker_dup (style->marker.mark);
			go_marker_set_shape (m, GO_MARKER_NONE);
			go_style_set_marker (style, m);
		}
	}
}

static void
gog_xy_minmax_series_update (GogObject *obj)
{
	const double *x_vals, *y_vals, *z_vals;
	GogSeries *series = GOG_SERIES (obj);
	unsigned old_num = series->num_elements;

	series->num_elements = gog_series_get_xyz_data (series,
							&x_vals, &y_vals, &z_vals);
	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->plot));
	if (old_num != series->num_elements)
		gog_plot_request_cardinality_update (series->plot);

	if (series_parent_klass->update)
		series_parent_klass->update (obj);
}

static void
gog_xy_minmax_series_class_init (GogObjectClass *gog_klass)
{
	GogStyledObjectClass *gso_klass = (GogStyledObjectClass *) gog_klass;
	series_parent_klass = g_type_class_peek_parent (gso_klass);
	gog_klass->update = gog_xy_minmax_series_update;
	gso_klass->init_style = gog_xy_minmax_series_init_style;
}

GSF_DYNAMIC_CLASS (GogXYMinMaxSeries, gog_xy_minmax_series,
	gog_xy_minmax_series_class_init, NULL,
	GOG_TYPE_SERIES)

/*****************************************************************************/

static void
gog_xy_minmax_plot_clear_formats (GogXYMinMaxPlot *plot)
{
	go_format_unref (plot->x.fmt);
	plot->x.fmt = NULL;

	go_format_unref (plot->y.fmt);
	plot->y.fmt = NULL;
}

static void
gog_xy_minmax_plot_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GogXYMinMaxPlot *minmax = GOG_XY_MINMAX_PLOT (obj);

	switch (param_id) {
	case XY_MINMAX_PROP_HORIZONTAL:
		minmax->horizontal = g_value_get_boolean (value);
		break;

	case XY_MINMAX_PROP_DEFAULT_STYLE_HAS_MARKERS:
		minmax->default_style_has_markers = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
}

static void
gog_xy_minmax_plot_get_property (GObject *obj, guint param_id,
			      GValue *value, GParamSpec *pspec)
{
	GogXYMinMaxPlot *minmax = GOG_XY_MINMAX_PLOT (obj);

	switch (param_id) {
	case XY_MINMAX_PROP_HORIZONTAL:
		g_value_set_boolean (value, minmax->horizontal);
		break;
	case XY_MINMAX_PROP_DEFAULT_STYLE_HAS_MARKERS:
		g_value_set_boolean (value, minmax->default_style_has_markers);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static char const *
gog_xy_minmax_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name min/max line plot objects
	 * eg The 2nd min/max line plot in a chart will be called
	 * 	PlotMinMax2 */
	return N_("PlotXYMinMax");
}

static void
gog_xy_minmax_plot_update (GogObject *obj)
{
	GogXYMinMaxPlot *model = GOG_XY_MINMAX_PLOT (obj);
	GogSeries const *series = NULL;
	double x_min, x_max, y_min, y_max, tmp_min, tmp_max;
	GSList *ptr;
	unsigned xaxis, yaxis;

	if (model->horizontal) {
		xaxis = 1;
		yaxis = 0;
	} else {
		xaxis = 0;
		yaxis = 1;
	}
	x_min = y_min =  DBL_MAX;
	x_max = y_max = -DBL_MAX;
	gog_xy_minmax_plot_clear_formats (model);
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;

		go_data_get_bounds (series->values[1].data, &tmp_min, &tmp_max);
		if (y_min > tmp_min) y_min = tmp_min;
		if (y_max < tmp_max) y_max = tmp_max;
		if (model->y.fmt == NULL) {
			model->y.fmt = go_data_preferred_fmt (series->values[1].data);
			model->y.date_conv = go_data_date_conv (series->values[1].data);
		}
		go_data_get_bounds (series->values[2].data, &tmp_min, &tmp_max);
		if (y_min > tmp_min)
			y_min = tmp_min;
		if (y_max < tmp_max)
			y_max = tmp_max;

		if (series->values[0].data != NULL) {
			go_data_get_bounds (series->values[0].data, &tmp_min, &tmp_max);

			if (!go_finite (tmp_min) || !go_finite (tmp_max) ||
			    tmp_min > tmp_max) {
				tmp_min = 0;
				tmp_max = go_data_get_vector_size (series->values[1].data);

			} else if (model->x.fmt == NULL) {
				model->x.fmt = go_data_preferred_fmt (series->values[0].data);
				model->x.date_conv = go_data_date_conv (series->values[0].data);
			}
		} else {
			tmp_min = 0;
			tmp_max = go_data_get_vector_size (series->values[1].data);
		}

		if (x_min > tmp_min)
			x_min = tmp_min;
		if (x_max < tmp_max)
			x_max = tmp_max;
	}
	if (model->x.minima != x_min || model->x.maxima != x_max) {
		model->x.minima = x_min;
		model->x.maxima = x_max;
		gog_axis_bound_changed (model->base.axis[xaxis], GOG_OBJECT (model));
	}
	if (model->y.minima != y_min || model->y.maxima != y_max) {
		model->y.minima = y_min;
		model->y.maxima = y_max;
		gog_axis_bound_changed (model->base.axis[yaxis], GOG_OBJECT (model));
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
	if (gog_xy_minmax_parent_klass->update)
		gog_xy_minmax_parent_klass->update (obj);
}

static GOData *
gog_xy_minmax_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			    GogPlotBoundInfo *bounds)
{
	GogXYMinMaxPlot *model = GOG_XY_MINMAX_PLOT (plot);

	if ((model->horizontal && axis == GOG_AXIS_Y) || (!model->horizontal && axis == GOG_AXIS_X)) {
		GSList *ptr;

		bounds->val.minima = model->x.minima;
		bounds->val.maxima = model->x.maxima;
		bounds->is_discrete = model->x.minima > model->x.maxima ||
			!go_finite (model->x.minima) ||
			!go_finite (model->x.maxima);
		if (bounds->fmt == NULL && model->x.fmt != NULL)
			bounds->fmt = go_format_ref (model->x.fmt);
		if (model->x.date_conv)
			bounds->date_conv = model->x.date_conv;

		for (ptr = plot->series; ptr != NULL ; ptr = ptr->next)
			if (gog_series_is_valid (GOG_SERIES (ptr->data)))
				return GOG_SERIES (ptr->data)->values[0].data;
		return NULL;
	}

	if ((model->horizontal && axis == GOG_AXIS_X) || (!model->horizontal && axis == GOG_AXIS_Y)) {
		bounds->val.minima = model->y.minima;
		bounds->val.maxima = model->y.maxima;
		if (bounds->fmt == NULL && model->y.fmt != NULL)
			bounds->fmt = go_format_ref (model->y.fmt);
		if (model->y.date_conv)
			bounds->date_conv = model->y.date_conv;
	}
	return NULL;
}

static void
gog_xy_minmax_plot_finalize (GObject *obj)
{
	gog_xy_minmax_plot_clear_formats (GOG_XY_MINMAX_PLOT (obj));
	((GObjectClass *) gog_xy_minmax_parent_klass)->finalize (obj);
}

static void
gog_xy_minmax_plot_class_init (GogPlotClass *plot_klass)
{
	GObjectClass   *gobject_klass = (GObjectClass *) plot_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) plot_klass;
	gog_xy_minmax_parent_klass = g_type_class_peek_parent (plot_klass);

	gobject_klass->set_property = gog_xy_minmax_plot_set_property;
	gobject_klass->get_property = gog_xy_minmax_plot_get_property;
	gobject_klass->finalize = gog_xy_minmax_plot_finalize;

	g_object_class_install_property (gobject_klass, XY_MINMAX_PROP_HORIZONTAL,
		g_param_spec_boolean ("horizontal",
			_("Horizontal"),
			_("Horizontal or vertical lines"),
			FALSE,
			G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, XY_MINMAX_PROP_DEFAULT_STYLE_HAS_MARKERS,
		g_param_spec_boolean ("default-style-has-markers",
			_("Default markers"),
			_("Should the default style of a series include markers"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_klass->type_name	= gog_xy_minmax_plot_type_name;
	gog_object_klass->update	= gog_xy_minmax_plot_update;
	gog_object_klass->view_type	= gog_xy_minmax_view_get_type ();

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Positions"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("Min"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_LOW },
			{ N_("Max"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_HIGH },
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
	}
	plot_klass->desc.series.style_fields = GO_STYLE_LINE | GO_STYLE_MARKER;
	plot_klass->axis_set	      	= GOG_AXIS_SET_XY;
	plot_klass->axis_get_bounds   		= gog_xy_minmax_axis_get_bounds;
	plot_klass->series_type = gog_xy_minmax_series_get_type ();

}

static void
gog_xy_minmax_plot_init (GogXYMinMaxPlot *minmax)
{
	minmax->default_style_has_markers = FALSE;
}

GSF_DYNAMIC_CLASS (GogXYMinMaxPlot, gog_xy_minmax_plot,
	gog_xy_minmax_plot_class_init, gog_xy_minmax_plot_init,
	GOG_TYPE_PLOT)

/*****************************************************************************/
typedef GogPlotView		GogXYMinMaxView;
typedef GogPlotViewClass	GogXYMinMaxViewClass;


static void
path_move_to (void *closure, GOPathPoint const *point)
{
	gog_renderer_draw_marker (GOG_RENDERER (closure), point->x, point->y);
}

static void
path_curve_to (void *closure,
	       GOPathPoint const *point0,
	       GOPathPoint const *point1,
	       GOPathPoint const *point2)
{
	gog_renderer_draw_marker (GOG_RENDERER (closure), point2->x, point2->y);
}

static void
path_close_path (void *closure)
{
}

static void
gog_xy_minmax_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogXYMinMaxPlot const *model = GOG_XY_MINMAX_PLOT (view->model);
	GogPlot *plot = GOG_PLOT (model);
	GogXYMinMaxSeries const *series;
	GogSeries const *base_series;
	GogAxisMap *x_map, *y_map;
	gboolean is_vertical = ! (model->horizontal);
	double *max_vals, *min_vals, *pos;
	double x, xmapped, minmapped, maxmapped;
	unsigned i, j;
	unsigned num_series;
	GSList *ptr;
	unsigned n, tmp;
	GOPath *path, *Mpath, *mpath;
	GOStyle * style;
	gboolean prec_valid;

	for (num_series = 0, ptr = plot->series ; ptr != NULL ; ptr = ptr->next, num_series++);
	if (num_series < 1)
		return;

	if (num_series <= 0)
		return;

	x_map = gog_axis_map_new (GOG_PLOT (model)->axis[0],
				  view->allocation.x, view->allocation.w);
	y_map = gog_axis_map_new (GOG_PLOT (model)->axis[1], view->allocation.y + view->allocation.h,
				  -view->allocation.h);

	if (!(gog_axis_map_is_valid (x_map) &&
	      gog_axis_map_is_valid (y_map))) {
		gog_axis_map_free (x_map);
		gog_axis_map_free (y_map);
		return;
	}


	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);

	for (ptr = plot->series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		base_series = GOG_SERIES (series);
		if (!gog_series_is_valid (base_series))
			continue;
		style = go_styled_object_get_style (GO_STYLED_OBJECT (series));
		pos = go_data_get_values (base_series->values[0].data);
		n = go_data_get_vector_size (base_series->values[0].data);
		min_vals = go_data_get_values (base_series->values[1].data);
		tmp = go_data_get_vector_size (base_series->values[1].data);
		if (n > tmp)
			n = tmp;
		max_vals = go_data_get_values (base_series->values[2].data);
		tmp = go_data_get_vector_size (base_series->values[2].data);
		if (n > tmp)
			n = tmp;
		mpath = go_path_new ();
		Mpath = go_path_new ();
		go_path_set_options (mpath, GO_PATH_OPTIONS_SHARP);
		go_path_set_options (Mpath, GO_PATH_OPTIONS_SHARP);
		gog_renderer_push_style (view->renderer, style);
		j = 0;
		prec_valid = FALSE;

		for (i = 0; i < n; i++) {
			x = (pos)? pos[i]: i;
			if (is_vertical) {
				if (!gog_axis_map_finite (x_map, x) ||
				    !gog_axis_map_finite (y_map, min_vals[i]) ||
				    !gog_axis_map_finite (y_map, max_vals[i])) {
					prec_valid = FALSE;
					continue;
				    }
				xmapped = gog_axis_map_to_view (x_map, x);
				minmapped = gog_axis_map_to_view (y_map, min_vals[i]);
				maxmapped = gog_axis_map_to_view (y_map, max_vals[i]);
				go_path_move_to (path, xmapped, minmapped);
				go_path_line_to (path, xmapped, maxmapped);
				if (prec_valid) {
					go_path_line_to (mpath, xmapped, minmapped);
					go_path_line_to (Mpath, xmapped, maxmapped);
				} else {
					go_path_move_to (mpath, xmapped, minmapped);
					go_path_move_to (Mpath, xmapped, maxmapped);
				}
			} else {
				if (!gog_axis_map_finite (y_map, x) ||
				    !gog_axis_map_finite (x_map, min_vals[i]) ||
				    !gog_axis_map_finite (x_map, max_vals[i])) {
					prec_valid = FALSE;
					continue;
				}
				xmapped = gog_axis_map_to_view (y_map, x);
				minmapped = gog_axis_map_to_view (x_map, min_vals[i]);
				maxmapped = gog_axis_map_to_view (x_map, max_vals[i]);
				go_path_move_to (path, minmapped, xmapped);
				go_path_line_to (path, maxmapped, xmapped);
				if (prec_valid) {
					go_path_line_to (mpath, minmapped, xmapped);
					go_path_line_to (Mpath, maxmapped, xmapped);
				} else {
					go_path_move_to (mpath, minmapped, xmapped);
					go_path_move_to (Mpath, maxmapped, xmapped);
				}
			}
			gog_renderer_stroke_serie (view->renderer, path);
			go_path_clear (path);
			prec_valid = TRUE;
			j++;
		}
		if (go_style_is_marker_visible (style))
			for (i = 0; i < j; i++) {
				go_path_interpret (mpath, GO_PATH_DIRECTION_FORWARD,
						   path_move_to,
						   path_move_to,
						   path_curve_to,
						   path_close_path,
						   view->renderer);
				go_path_interpret (Mpath, GO_PATH_DIRECTION_FORWARD,
						   path_move_to,
						   path_move_to,
						   path_curve_to,
						   path_close_path,
						   view->renderer);
			}
		gog_renderer_pop_style (view->renderer);
		go_path_free (Mpath);
		go_path_free (mpath);
	}

	go_path_free (path);
	gog_axis_map_free (x_map);
	gog_axis_map_free (y_map);
}

static void
gog_xy_minmax_view_class_init (GogViewClass *view_klass)
{
	view_klass->render	  = gog_xy_minmax_view_render;
	view_klass->clip	  = TRUE;
}

GSF_DYNAMIC_CLASS (GogXYMinMaxView, gog_xy_minmax_view,
	gog_xy_minmax_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)
