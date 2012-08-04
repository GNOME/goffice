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
data_as_y_toggled_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "data-as-y-values", gtk_toggle_button_get_active (btn), NULL);
}

static void
gog_probability_plot_populate_editor (GogObject *item,
			      GOEditor *editor,
			      GogDataAllocator *dalloc,
			      GOCmdContext *cc)
{
	GtkWidget *w, *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (box), 12);
	w = gtk_check_button_new_with_label (_("Use data as Y-values"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), GOG_PROBABILITY_PLOT (item)->data_as_yvals);
	g_signal_connect (G_OBJECT (w), "toggled", G_CALLBACK (data_as_y_toggled_cb), item);
	gtk_box_pack_start (GTK_BOX (box), w, FALSE, TRUE, 0);
	gtk_widget_show_all (box);
	go_editor_add_page (editor, box, _("Details"));
	w = go_distribution_pref_new (G_OBJECT (item), dalloc, cc);
	go_editor_add_page (editor, w, _("Distribution"));

	(GOG_OBJECT_CLASS(probability_plot_parent_klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

static void
gog_probability_plot_update (GogObject *obj)
{
	GogProbabilityPlot *plot = GOG_PROBABILITY_PLOT (obj);
	GogProbabilityPlotSeries *series;
	double x_min, x_max, y_min, y_max;
	GSList *ptr;

	x_min = y_min =  DBL_MAX;
	x_max = y_max = -DBL_MAX;
	for (ptr = plot->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = GOG_PROBABILITY_PLOT_SERIES (ptr->data);
		if (!gog_series_is_valid (GOG_SERIES (series)) || series->base.num_elements == 0)
			continue;

		if (plot->data_as_yvals) {
			if (x_min > series->y[0])
				x_min = series->y[0];
			if (x_max < series->y[series->base.num_elements - 1])
				x_max = series->y[series->base.num_elements - 1];
			if (y_min > series->x[0])
				y_min = series->x[0];
			if (y_max < series->x[series->base.num_elements - 1])
				y_max = series->x[series->base.num_elements - 1];
		} else {
			if (x_min > series->x[0])
				x_min = series->x[0];
			if (x_max < series->x[series->base.num_elements - 1])
				x_max = series->x[series->base.num_elements - 1];
			if (y_min > series->y[0])
				y_min = series->y[0];
			if (y_max < series->y[series->base.num_elements - 1])
				y_max = series->y[series->base.num_elements - 1];
		}
	}
	if (plot->x.minima != x_min || plot->x.maxima != x_max) {
		plot->x.minima = x_min;
		plot->x.maxima = x_max;
		gog_axis_bound_changed (plot->base.axis[0], GOG_OBJECT (plot));
	}
	if (plot->y.minima != y_min || plot->y.maxima != y_max) {
		plot->y.minima = y_min;
		plot->y.maxima = y_max;
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
	PROBABILITY_PLOT_PROP_SHAPE_PARAM2,
	PROBABILITY_PLOT_PROP_DATA_AS_YVALS
};

static void
gog_probability_plot_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GogProbabilityPlot *plot = GOG_PROBABILITY_PLOT (obj);

	switch (param_id) {
	case PROBABILITY_PLOT_PROP_DISTRIBUTION: {
		GODistribution *dist = GO_DISTRIBUTION (g_value_get_object (value));
		GSList *series;
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
			for (series = plot->base.series; series; series = series->next)
				gog_object_request_update (GOG_OBJECT (series->data));
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
	case PROBABILITY_PLOT_PROP_DATA_AS_YVALS:
		plot->data_as_yvals = g_value_get_boolean (value);
		gog_object_request_update (GOG_OBJECT (obj));
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
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
	case PROBABILITY_PLOT_PROP_DATA_AS_YVALS:
		g_value_set_boolean (value, plot->data_as_yvals);
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
	g_object_class_install_property (gobject_klass, PROBABILITY_PLOT_PROP_DATA_AS_YVALS,
		g_param_spec_boolean ("data-as-y-values",
			_("Data as Y values"),
			_("whether the data should be mapped to the Y axis."),
			FALSE,
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
	plot_klass->desc.num_series_max = G_MAXINT;
	plot_klass->series_type = gog_probability_plot_series_get_type ();
	plot_klass->axis_set = GOG_AXIS_SET_XY;
	plot_klass->desc.series.style_fields	= GO_STYLE_MARKER;
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
			g_value_set_double (&value, go_data_get_scalar_value (plot->shape_params[dim_i].elem->data));
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
	GOStyle *style;
	GSList *ptr;

	if (model->base.series == NULL)
		return;

	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);

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

	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = GOG_PROBABILITY_PLOT_SERIES (ptr->data);
		if (!gog_series_is_valid (GOG_SERIES (series))
		    || series->base.num_elements == 0
		    || series->x == NULL || series->y == NULL)
			continue;
		nb = series->base.num_elements;
		style = GOG_STYLED_OBJECT (series)->style;
		gog_renderer_push_style (view->renderer, style);
		if (model->data_as_yvals) {
			for (i = 0; i < nb; i++) {
				gog_renderer_draw_marker (view->renderer,
							  gog_axis_map_to_view (x_map, series->y[i]),
							  gog_axis_map_to_view (y_map, series->x[i]));
			}
		} else {
			for (i = 0; i < nb; i++) {
				gog_renderer_draw_marker (view->renderer,
							  gog_axis_map_to_view (x_map, series->x[i]),
							  gog_axis_map_to_view (y_map, series->y[i]));
			}
		}
		gog_renderer_pop_style (view->renderer);
	}
	gog_chart_map_free (chart_map);
}

static GogViewClass *probability_plot_view_parent_klass;

static void
gog_probability_plot_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;
	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
	(probability_plot_view_parent_klass->size_allocate) (view, allocation);
}

static void
gog_probability_plot_view_class_init (GogViewClass *view_klass)
{
	probability_plot_view_parent_klass = (GogViewClass*) g_type_class_peek_parent (view_klass);
	view_klass->render	  = gog_probability_plot_view_render;
	view_klass->size_allocate = gog_probability_plot_view_size_allocate;
	view_klass->clip	  = FALSE;
}

GSF_DYNAMIC_CLASS (GogProbabilityPlotView, gog_probability_plot_view,
	gog_probability_plot_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)


/*****************************************************************************/

static gboolean
regression_curve_can_add (GogObject const *parent)
{
	return (gog_object_get_child_by_name (parent, "Regression line") == NULL);
}

static void
regression_curve_post_add (GogObject *parent, GogObject *child)
{
	gog_object_request_update (child);
}

static void
regression_curve_pre_remove (GogObject *parent, GogObject *child)
{
}

/*****************************************************************************/

typedef GogView		GogProbabilityPlotSeriesView;
typedef GogViewClass	GogProbabilityPlotSeriesViewClass;

#define GOG_TYPE_PROBABILITY_SERIES_VIEW	(gog_probability_plot_series_view_get_type ())
#define GOG__PROBABILITY_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE__PROBABILITY_SERIES_VIEW, GogProbabilityPlotSeriesView))
#define GOG_IS__PROBABILITY_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE__PROBABILITY_SERIES_VIEW))

GType gog_probability_plot_series_view_get_type (void);

static void
gog_probability_plot_series_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr;
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);
}

static void
gog_probability_plot_series_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;

	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
}

static void
gog_probability_plot_series_view_class_init (GogProbabilityPlotSeriesViewClass *gview_klass)
{
	GogViewClass *view_klass = GOG_VIEW_CLASS (gview_klass);
	view_klass->render = gog_probability_plot_series_view_render;
	view_klass->size_allocate = gog_probability_plot_series_view_size_allocate;
	view_klass->build_toolkit = NULL;
}

GSF_DYNAMIC_CLASS (GogProbabilityPlotSeriesView, gog_probability_plot_series_view,
	gog_probability_plot_series_view_class_init, NULL,
	GOG_TYPE_VIEW)
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
	GSList *ptr;

	g_free (series->x);
	series->x = NULL;
	if (series->base.values[0].data != NULL) {
		double *x;
		unsigned i, max = 0;
		x_vals = go_data_get_values (series->base.values[0].data);
		series->base.num_elements = go_data_get_vector_size (series->base.values[0].data);
		if (x_vals) {
		x = g_new (double, series->base.num_elements);
			for (i = 0; i < series->base.num_elements; i++) {
				if (go_finite (x_vals[i]))
				    x[max++] = x_vals[i];
			}
			series->base.num_elements = max;
			series->x = go_range_sort (x, series->base.num_elements);
			g_free (x);
		}
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

	/* update children */
	for (ptr = obj->children; ptr != NULL; ptr = ptr->next)
		if (!GOG_IS_SERIES_LINES (ptr->data))
			gog_object_request_update (GOG_OBJECT (ptr->data));

	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));

	if (gog_probability_plot_series_parent_klass->update)
		gog_probability_plot_series_parent_klass->update (obj);
}

static unsigned
gog_probability_plot_series_get_xy_data (GogSeries const  *series,
			double const 	**x,
			double const 	**y)
{
	GogProbabilityPlotSeries *ppseries = GOG_PROBABILITY_PLOT_SERIES (series);
	*x = ppseries->x;
	*y = ppseries->y;
	return series->num_elements;
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
	GObjectClass *gobject_klass = (GObjectClass *) obj_klass;
	GogSeriesClass *series_klass = (GogSeriesClass *) obj_klass;

	static GogObjectRole const roles[] = {
		{ N_("Regression line"), "GogLinRegCurve",	0,
		  GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
		  regression_curve_can_add,
		  NULL,
		  NULL,
		  regression_curve_post_add,
		  regression_curve_pre_remove,
		  NULL }
	};

	series_parent_klass = g_type_class_peek_parent (obj_klass);
	gobject_klass->finalize		= gog_probability_plot_series_finalize;

	gog_probability_plot_series_parent_klass = g_type_class_peek_parent (obj_klass);
	obj_klass->update = gog_probability_plot_series_update;
	obj_klass->view_type	= gog_probability_plot_series_view_get_type ();
	gog_object_register_roles (obj_klass, roles, G_N_ELEMENTS (roles));

	series_klass->get_xy_data = gog_probability_plot_series_get_xy_data;
}

GSF_DYNAMIC_CLASS (GogProbabilityPlotSeries, gog_probability_plot_series,
	gog_probability_plot_series_class_init, NULL,
	GOG_TYPE_SERIES)
