/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-probability-plot.c
 *
 * Copyright (C) 2007 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
#include "gog-probability-plot.h"
#ifdef GOFFICE_WITH_GTK
#include "go-distribution-prefs.h"
#endif
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/math/go-rangefunc.h>
#include <goffice/utils/go-persist.h>
#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <math.h>
#include <string.h>

#define GOG_TYPE_PROBABILITY_PLOT_SERIES	(gog_probability_plot_series_get_type ())
#define GOG_PROBABILITY_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_PROBABILITY_PLOT_SERIES, GogProbabilityPlotSeries))
#define GOG_IS_PROBABILITY_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_PROBABILITY_PLOT_SERIES))

typedef struct {
	GogSeries base;
	double *x, *y;
} GogProbabilityPlotSeries;
typedef GogSeriesClass GogProbabilityPlotSeriesClass;

GType gog_probability_plot_series_get_type (void);
GType gog_probability_plot_view_get_type (void);

typedef GogPlotClass GogProbabilityPlotClass;

static GogObjectClass *probability_plot_parent_klass;

#ifdef GOFFICE_WITH_GTK
static void
gog_probability_plot_populate_editor (GogObject *item,
			      GogEditor *editor,
			      GogDataAllocator *dalloc,
			      GOCmdContext *cc)
{
	gog_editor_add_page (editor, go_distribution_pref_new (G_OBJECT (item), dalloc, cc), _("Distribution"));
	
	(GOG_OBJECT_CLASS(probability_plot_parent_klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

static void
gog_probability_plot_update (GogObject *obj)
{
	GogProbabilityPlot *plot = GOG_PROBABILITY_PLOT (obj);
	GogProbabilityPlotSeries *series = GOG_PROBABILITY_PLOT_SERIES (
		plot->base.series->data);


	if (!gog_series_is_valid (GOG_SERIES (series)) || series->base.num_elements == 0)
			return;
	if (plot->x.minima != series->x[0] || plot->x.maxima != series->x[series->base.num_elements - 1]) {
		plot->x.minima = series->x[0];
		plot->x.maxima = series->x[series->base.num_elements - 1];
		gog_axis_bound_changed (plot->base.axis[0], GOG_OBJECT (plot));
	}
	if (plot->y.minima != series->y[0] || plot->y.maxima != series->y[series->base.num_elements - 1]) {
		plot->y.minima = series->y[0];
		plot->y.maxima = series->y[series->base.num_elements - 1];
		gog_axis_bound_changed (plot->base.axis[1], GOG_OBJECT (plot));
	}
}

static GOData *
gog_probability_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			      GogPlotBoundInfo *bounds)
{
	GogProbabilityPlot *model = GOG_PROBABILITY_PLOT (plot);


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

static char const *
gog_probability_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name probability-plot objects
	 * eg The 2nd probability-plot in a chart will be called
	 * 	ProbabilityPlot2 */
	return N_("ProbabilityPlot");
}

/* We do not support distributions with more than two shape parameters */
enum {
	PROBABILITY_PLOT_PROP_0,
	PROBABILITY_PLOT_PROP_DISTRIBUTION,
	PROBABILITY_PLOT_PROP_SHAPE_PARAM1,
	PROBABILITY_PLOT_PROP_SHAPE_PARAM2
};

static void
gog_probability_plot_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GogProbabilityPlot *plot = GOG_PROBABILITY_PLOT (obj);

	switch (param_id) {
	case PROBABILITY_PLOT_PROP_DISTRIBUTION: {
		GODistribution *dist = GO_DISTRIBUTION (g_value_get_object (value));
		if (dist) {
			GParamSpec **props;
			int i, j, n;
			if (plot->dist)
				g_object_unref (plot->dist);
			plot->dist = g_object_ref (dist);
			/* search the properties */
			props = g_object_class_list_properties (G_OBJECT_GET_CLASS (dist), &n);
			for (i = j = 0; j < n; j++)
				if (props[j]->flags & GO_PARAM_PERSISTENT) {
					g_free (plot->shape_params[i].prop_name);
					plot->shape_params[i].prop_name = g_strdup (g_param_spec_get_name (props[j]));
					/* set the property */
					i++;
				}
			while (i < 2) {
				g_free (plot->shape_params[i].prop_name);
				plot->shape_params[i].prop_name = NULL;
				i++;
			}
			g_free (props);
			if (plot->base.series)
				gog_object_request_update (GOG_OBJECT (plot->base.series->data));
			gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
		}
		break;
	}
	case PROBABILITY_PLOT_PROP_SHAPE_PARAM1: {

		char const *name = g_value_get_string (value);
		g_free (plot->shape_params[0].prop_name);
		plot->shape_params[0].prop_name =
			(name && *name && strcmp (name, "none"))?
			g_strdup (name): NULL;
		break;
	}
	case PROBABILITY_PLOT_PROP_SHAPE_PARAM2: {
		char const *name = g_value_get_string (value);
		g_free (plot->shape_params[1].prop_name);
		plot->shape_params[1].prop_name =
			(name && *name && strcmp (name, "none"))?
			g_strdup (name): NULL;
		break;
	}
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_probability_plot_get_property (GObject *obj, guint param_id,
			      GValue *value, GParamSpec *pspec)
{
	GogProbabilityPlot *plot = GOG_PROBABILITY_PLOT (obj);

	switch (param_id) {
	case PROBABILITY_PLOT_PROP_DISTRIBUTION:
		g_value_set_object (value, plot->dist);
		break;
	case PROBABILITY_PLOT_PROP_SHAPE_PARAM1:
		g_value_set_string (value, ((plot->shape_params[0].prop_name)? plot->shape_params[0].prop_name: NULL));
		break;
	case PROBABILITY_PLOT_PROP_SHAPE_PARAM2:
		g_value_set_string (value, ((plot->shape_params[1].prop_name)? plot->shape_params[1].prop_name: NULL));
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		break;
	}
}

static void
gog_probability_plot_finalize (GObject *obj)
{
	GogProbabilityPlot *plot = GOG_PROBABILITY_PLOT (obj);
	g_return_if_fail (plot != NULL);
	if (plot->dist != NULL)
		g_object_unref (plot->dist);
	gog_dataset_finalize (GOG_DATASET (plot));
	g_free (plot->shape_params[0].prop_name);
	g_free (plot->shape_params[0].elem);
	g_free (plot->shape_params[1].prop_name);
	g_free (plot->shape_params[1].elem);
	G_OBJECT_CLASS (probability_plot_parent_klass)->finalize (obj);
}

static void
gog_probability_plot_class_init (GogPlotClass *gog_plot_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) gog_plot_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) gog_plot_klass;
	GogPlotClass   *plot_klass = (GogPlotClass *) gog_plot_klass;

	probability_plot_parent_klass = g_type_class_peek_parent (gog_plot_klass);

	gobject_klass->set_property = gog_probability_plot_set_property;
	gobject_klass->get_property = gog_probability_plot_get_property;
	gobject_klass->finalize = gog_probability_plot_finalize;

	g_object_class_install_property (gobject_klass, PROBABILITY_PLOT_PROP_DISTRIBUTION,
		g_param_spec_object ("distribution", 
			_("Distribution"),
			_("A pointer to the GODistribution used by this plot"),
			GO_TYPE_DISTRIBUTION, 
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, PROBABILITY_PLOT_PROP_SHAPE_PARAM1,
		g_param_spec_string ("param1", 
			_("Shape parameter"),
			_("Name of the first shape parameter if any"),
			"none", 
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, PROBABILITY_PLOT_PROP_SHAPE_PARAM2,
		g_param_spec_string ("param2", 
			_("Second shape parameter"),
			_("Name of the second shape parameter if any"),
			"none", 
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_klass->type_name	= gog_probability_plot_type_name;
	gog_object_klass->view_type	= gog_probability_plot_view_get_type ();
	gog_object_klass->update	= gog_probability_plot_update;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Values"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_VALUES },
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
	}
	plot_klass->desc.num_series_max = 1;
	plot_klass->series_type = gog_probability_plot_series_get_type ();
	plot_klass->axis_set = GOG_AXIS_SET_XY;
	plot_klass->desc.series.style_fields	= GOG_STYLE_MARKER;
	plot_klass->axis_get_bounds   		= gog_probability_plot_axis_get_bounds;
#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor = gog_probability_plot_populate_editor;
#endif
}

static void
gog_probability_plot_init (GogProbabilityPlot *plot)
{
	plot->dist = go_distribution_new (plot->dist_type = GO_DISTRIBUTION_NORMAL);
	plot->shape_params[0].elem = g_new0 (GogDatasetElement, 1);
	plot->shape_params[1].elem = g_new0 (GogDatasetElement, 1);
};

static void
gog_probability_plot_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = 0;
	*last = 1;
}

static GogDatasetElement *
gog_probability_plot_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogProbabilityPlot const *plot = GOG_PROBABILITY_PLOT (set);
	g_return_val_if_fail (2 > dim_i, NULL);
	g_return_val_if_fail (dim_i >= 0, NULL);
	return plot->shape_params[dim_i].elem;
}

static void
gog_probability_plot_dataset_dim_changed (GogDataset *set, int dim_i)
{
	GogProbabilityPlot const *plot = GOG_PROBABILITY_PLOT (set);
	GParamSpec *spec;
	GType prop_type;

	if (!plot->shape_params[dim_i].prop_name)
		return;
	spec = g_object_class_find_property (G_OBJECT_GET_CLASS (plot->dist), plot->shape_params[dim_i].prop_name);
	prop_type = G_PARAM_SPEC_VALUE_TYPE (spec);
	switch G_TYPE_FUNDAMENTAL (prop_type) {
	case G_TYPE_DOUBLE: {
		GValue value = {0};
		g_value_init (&value, G_TYPE_DOUBLE);
		if (plot->shape_params[dim_i].elem->data)
			g_value_set_double (&value, go_data_scalar_get_value (GO_DATA_SCALAR (plot->shape_params[dim_i].elem->data)));
		else
			g_param_value_set_default (spec, &value);
		g_param_value_validate (spec, &value);
		g_object_set_property (G_OBJECT (plot->dist), plot->shape_params[dim_i].prop_name, &value);
		g_value_unset (&value);
		break;
	}
	default:
		g_critical ("Unsupported property type. Please report.");
		break;
	}
	if (plot->base.series)
		gog_object_request_update (GOG_OBJECT (plot->base.series->data));
	gog_object_request_update (GOG_OBJECT (set));
}

static void
gog_probability_plot_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_probability_plot_dataset_get_elem;
	iface->dims	   = gog_probability_plot_dataset_dims;
	iface->dim_changed = gog_probability_plot_dataset_dim_changed;
}

GSF_DYNAMIC_CLASS_FULL (GogProbabilityPlot, gog_probability_plot,
	NULL, NULL, gog_probability_plot_class_init, NULL,
	gog_probability_plot_init, GOG_TYPE_PLOT, 0,
	GSF_INTERFACE (gog_probability_plot_dataset_init, GOG_TYPE_DATASET))

/************************************************,*****************************/
typedef GogPlotView		GogProbabilityPlotView;
typedef GogPlotViewClass	GogProbabilityPlotViewClass;

static void
gog_probability_plot_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogProbabilityPlot const *model = GOG_PROBABILITY_PLOT (view->model);
	GogChart *chart = GOG_CHART (view->model->parent);
	GogChartMap *chart_map;
	GogAxisMap *x_map, *y_map;
	GogViewAllocation const *area;
	GogProbabilityPlotSeries const *series;
	unsigned i, nb;
	GogStyle *style;

	if (model->base.series == NULL)
		return;
	series = GOG_PROBABILITY_PLOT_SERIES (model->base.series->data);
	nb = series->base.num_elements;
	style = GOG_STYLED_OBJECT (series)->style;
	if (nb == 0 || series->x == NULL || series->y == NULL)
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

	gog_renderer_push_style (view->renderer, style);
	for (i = 0; i < nb; i++) {
		gog_renderer_draw_marker (view->renderer,
					  gog_axis_map_to_view (x_map, series->x[i]),
					  gog_axis_map_to_view (y_map, series->y[i]));
	}
	gog_renderer_pop_style (view->renderer);
	gog_chart_map_free (chart_map);
}

static GogViewClass *probability_plot_view_parent_klass;

static void
gog_probability_plot_view_class_init (GogViewClass *view_klass)
{
	probability_plot_view_parent_klass = (GogViewClass*) g_type_class_peek_parent (view_klass);
	view_klass->render	  = gog_probability_plot_view_render;
	view_klass->clip	  = FALSE;
}

GSF_DYNAMIC_CLASS (GogProbabilityPlotView, gog_probability_plot_view,
	gog_probability_plot_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)

/****************************************************************************/

static GogObjectClass *gog_probability_plot_series_parent_klass;

static GObjectClass *series_parent_klass;

static void
gog_probability_plot_series_update (GogObject *obj)
{
	double *x_vals = NULL;
	GogProbabilityPlotSeries *series = GOG_PROBABILITY_PLOT_SERIES (obj);
	double mn, d;
	unsigned i;
	GODistribution *dist = GO_DISTRIBUTION (((GogProbabilityPlot *) series->base.plot)->dist);

	g_free (series->x);
	series->x = NULL;
	if (series->base.values[0].data != NULL) {
		x_vals = go_data_vector_get_values (GO_DATA_VECTOR (series->base.values[0].data));
		series->base.num_elements = go_data_vector_get_len 
			(GO_DATA_VECTOR (series->base.values[0].data));
		if (x_vals)
			series->x = go_range_sort (x_vals, series->base.num_elements);
	}
	mn = pow (0.5, 1. / series->base.num_elements);
	d = series->base.num_elements + .365;
	g_free (series->y);
	if (series->base.num_elements > 0) {
		series->y = g_new0 (double, series->base.num_elements);
		series->y[0] = go_distribution_get_ppf (dist, 1. - mn);
		if (series->base.num_elements > 1) {
			for (i = 1; i < series->base.num_elements - 1; i++)
				series->y[i] = go_distribution_get_ppf (dist, (i + .6825) / d);
			series->y[i] = go_distribution_get_ppf (dist, mn);
		}
			
	} else
		series->y = NULL;

	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));

	if (gog_probability_plot_series_parent_klass->update)
		gog_probability_plot_series_parent_klass->update (obj);
}

static void
gog_probability_plot_series_finalize (GObject *obj)
{
	GogProbabilityPlotSeries *series = GOG_PROBABILITY_PLOT_SERIES (obj);

	g_free (series->y);
	series->y = NULL;
	g_free (series->x);
	series->x = NULL;

	G_OBJECT_CLASS (series_parent_klass)->finalize (obj);
}

static void
gog_probability_plot_series_class_init (GogObjectClass *obj_klass)
{
	GogStyledObjectClass *gso_klass = (GogStyledObjectClass*) obj_klass;
	GObjectClass *gobject_klass = (GObjectClass *) gso_klass;

	series_parent_klass = g_type_class_peek_parent (obj_klass);
	gobject_klass->finalize		= gog_probability_plot_series_finalize;

	gog_probability_plot_series_parent_klass = g_type_class_peek_parent (obj_klass);
	obj_klass->update = gog_probability_plot_series_update;
}

GSF_DYNAMIC_CLASS (GogProbabilityPlotSeries, gog_probability_plot_series,
	gog_probability_plot_series_class_init, NULL,
	GOG_TYPE_SERIES)
