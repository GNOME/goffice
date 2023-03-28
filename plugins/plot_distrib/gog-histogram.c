/*
 * gog-histogram.c
 *
 * Copyright (C) 2005-2010 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-styled-object.h>
#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#define GOG_TYPE_HISTOGRAM_PLOT_SERIES	(gog_histogram_plot_series_get_type ())
#define GOG_HISTOGRAM_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_HISTOGRAM_PLOT_SERIES, GogHistogramPlotSeries))
#define GOG_IS_HISTOGRAM_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_HISTOGRAM_PLOT_SERIES))

typedef struct {
	GogSeries base;
	GogObject *droplines;
	double *x, *y, *y_; /* y_ is the second data for double histograms. */
	double *real_x, *real_y, *real_y_;
} GogHistogramPlotSeries;
typedef GogSeriesClass GogHistogramPlotSeriesClass;

GType gog_histogram_plot_series_get_type (void);
GType gog_histogram_plot_view_get_type (void);

static GogObjectClass *histogram_plot_parent_klass;

/* Tests if product is strictly positive without causing overflow.  */
static gboolean
pos_product (double x1, double x2)
{
	return ((x1 > 0 && x2 > 0) ||
		(x1 < 0 && x2 < 0));
}


static void
gog_histogram_plot_clear_formats (GogHistogramPlot *model)
{
	go_format_unref (model->x.fmt);
	model->x.fmt = NULL;

	go_format_unref (model->y.fmt);
	model->y.fmt = NULL;
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
	GogHistogramPlotSeries *series = model->base.series
		? GOG_HISTOGRAM_PLOT_SERIES (model->base.series->data)
		: NULL;
	double x_min, x_max, y_min = DBL_MAX, y_max = -DBL_MAX, *x_vals = NULL, *y_vals = NULL, val;
	unsigned i, y_len = 0;

	if (!series || !gog_series_is_valid (GOG_SERIES (series)) || series->base.num_elements == 0)
			return;

	g_free (series->x);
	series->x = g_new (double, series->base.num_elements);
	if (series->real_x != NULL)
		x_vals = series->real_x;
	else if (series->base.values[0].data != NULL)
		x_vals = go_data_get_values (series->base.values[0].data);
	if (x_vals != NULL) {
		x_min = x_vals[0];
		x_max = x_vals[series->base.num_elements];
		if (series->base.values[0].data) {
			if (model->x.fmt == NULL)
				model->x.fmt = go_data_preferred_fmt (series->base.values[0].data);
			model->x.date_conv = go_data_date_conv (series->base.values[0].data);
		}
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
		gog_axis_bound_changed (model->base.axis[model->vertical? 0: 1], GOG_OBJECT (model));
	}
	g_free (series->y);
	series->y = NULL;
	if (series->real_y != NULL) {
		y_vals = series->real_y;
		y_len = series->base.num_elements;
	} else if (series->base.values[1].data != NULL) {
		y_vals = go_data_get_values (series->base.values[1].data);
		y_len = go_data_get_vector_size (series->base.values[1].data);
		if (y_len > series->base.num_elements)
			y_len = series->base.num_elements;
	}
	if (y_vals != NULL) {
		double sum = 0.;
		series->y = g_new0 (double, series->base.num_elements);
		for (i = 0; i < y_len; i++)
			if (go_finite (y_vals[i])) {
				if (model->cumulative)
					sum += y_vals[i];
				else
					sum = y_vals[i];
				series->y[i] = val = sum / (x_vals[i+1] - x_vals[i]);
				if (val < y_min)
					y_min = val;
				if (val > y_max)
					y_max = val;
			} else
				series->y[i] = model->cumulative ? sum : 0.;
		if (model->y.fmt == NULL)
			model->y.fmt = go_data_preferred_fmt (series->base.values[1].data);
		model->y.date_conv = go_data_date_conv (series->base.values[1].data);
	}
	if (GOG_IS_DOUBLE_HISTOGRAM_PLOT (model) && series->base.values[2].data != NULL) {
		double max = 0.;
		g_free (series->y_);
		series->y_ = NULL;
		if (series->real_y_ != NULL) {
			y_vals = series->real_y_;
			y_len = series->base.num_elements;
		} else if (series->base.values[1].data != NULL) {
			y_vals = go_data_get_values (series->base.values[1].data);
			y_len = go_data_get_vector_size (series->base.values[1].data);
			if (y_len > series->base.num_elements)
				y_len = series->base.num_elements;
		} else
			y_vals = NULL;
		if (y_vals != NULL) {
			double sum = 0.;
			y_min = 0.;
			series->y_ = g_new0 (double, series->base.num_elements);
			for (i = 0; i < y_len; i++)
				if (go_finite (y_vals[i])) {
					if (model->cumulative)
						sum += y_vals[i];
					else
						sum = y_vals[i];
					series->y_[i] = val = -sum / (x_vals[i+1] - x_vals[i]);
					if (val < y_min)
						y_min = val;
					if (val > max)
						max = val;
				} else
					series->y_[i] = model->cumulative ? sum : 0.;
		}
		if (y_max < 0)
			y_max = max;
	}
	if (y_min > y_max)
		y_min = y_max = go_nan;
	if (model->y.minima != y_min || model->y.maxima != y_max) {
		model->y.minima = y_min;
		model->y.maxima = y_max;
		gog_axis_bound_changed (model->base.axis[model->vertical? 1: 0], GOG_OBJECT (model));
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static GOData *
gog_histogram_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			      GogPlotBoundInfo *bounds)
{
	GogHistogramPlot *model = GOG_HISTOGRAM_PLOT (plot);


	if ((axis == GOG_AXIS_X && model->vertical) || (axis == GOG_AXIS_Y && !model->vertical)) {
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

enum {
	HISTOGRAM_PROP_0,
	HISTOGRAM_PROP_VERTICAL,
	HISTOGRAM_PROP_CUMULATIVE,
	HISTOGRAM_PROP_BEFORE_GRID
};

static void
gog_histogram_plot_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogHistogramPlot *model = GOG_HISTOGRAM_PLOT (obj);
	switch (param_id) {
	case HISTOGRAM_PROP_VERTICAL:
		g_value_set_boolean (value, model->vertical);
		break;
	case HISTOGRAM_PROP_CUMULATIVE:
		g_value_set_boolean (value, model->cumulative);
		break;
	case HISTOGRAM_PROP_BEFORE_GRID:
		g_value_set_boolean (value, GOG_PLOT (obj)->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_histogram_plot_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogHistogramPlot *model = GOG_HISTOGRAM_PLOT (obj);
	switch (param_id) {
	case HISTOGRAM_PROP_VERTICAL:
		if (g_value_get_boolean (value) != model->vertical) {
			model->vertical = !model->vertical;
			/* force axis bounds reevaluation */
			model->x.minima = model->y.minima = G_MAXDOUBLE;
			gog_object_request_update (GOG_OBJECT (model));
		}
		break;
	case HISTOGRAM_PROP_CUMULATIVE:
		if (g_value_get_boolean (value) != model->cumulative) {
			model->cumulative = !model->cumulative;
			gog_object_request_update (GOG_OBJECT (model));
		}
		break;
	case HISTOGRAM_PROP_BEFORE_GRID:
		GOG_PLOT (obj)->rendering_order =
			(g_value_get_boolean (value)
			 ? GOG_PLOT_RENDERING_BEFORE_GRID
			 : GOG_PLOT_RENDERING_LAST);
		gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>

static void
vertical_changed_cb (GtkToggleButton *btn, GogHistogramPlot *model)
{
	if (gtk_toggle_button_get_active (btn) != model->vertical) {
		model->vertical = !model->vertical;
		gog_object_request_update (GOG_OBJECT (model));
		/* force axis bounds reevaluation */
		model->x.minima = model->y.minima = G_MAXDOUBLE;
	}
}

static void
cumulative_changed_cb (GtkToggleButton *btn, GogHistogramPlot *model)
{
	if (gtk_toggle_button_get_active (btn) != model->cumulative) {
		model->cumulative = !model->cumulative;
		gog_object_request_update (GOG_OBJECT (model));
	}
}

static void
display_before_grid_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "before-grid", gtk_toggle_button_get_active (btn), NULL);
}

static void
gog_histogram_plot_populate_editor (GogObject *item,
			      GOEditor *editor,
			      G_GNUC_UNUSED GogDataAllocator *dalloc,
			      GOCmdContext *cc)
{
	GtkWidget  *w;
	GogHistogramPlot *hist = GOG_HISTOGRAM_PLOT (item);
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_distrib/gog-histogram-prefs.ui",
				    GETTEXT_PACKAGE, cc);
        if (gui != NULL) {
		w = go_gtk_builder_get_widget (gui, "vertical");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), hist->vertical);
		g_signal_connect (w, "toggled", G_CALLBACK (vertical_changed_cb), hist);

		w = go_gtk_builder_get_widget (gui, "cumulative");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), hist->cumulative);
		g_signal_connect (w, "toggled", G_CALLBACK (cumulative_changed_cb), hist);

		w = go_gtk_builder_get_widget (gui, "before-grid");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				(GOG_PLOT (item))->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (display_before_grid_cb), item);

		w = go_gtk_builder_get_widget (gui, "histogram-prefs");
		go_editor_add_page (editor, w , _("Properties"));
		g_object_unref (gui);
	}

	(GOG_OBJECT_CLASS(histogram_plot_parent_klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

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

	histogram_plot_parent_klass = g_type_class_peek_parent (gog_plot_klass);

	gobject_klass->finalize = gog_histogram_plot_finalize;
	gobject_klass->get_property = gog_histogram_plot_get_property;
	gobject_klass->set_property = gog_histogram_plot_set_property;

	g_object_class_install_property (gobject_klass, HISTOGRAM_PROP_VERTICAL,
		g_param_spec_boolean ("vertical",
			_("Vertical"),
			_("Draw the histogram vertically or horizontally"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, HISTOGRAM_PROP_CUMULATIVE,
		g_param_spec_boolean ("cumulative",
			_("Cumulative"),
			_("Use cumulated data"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, HISTOGRAM_PROP_BEFORE_GRID,
		g_param_spec_boolean ("before-grid",
			_("Displayed under the grids"),
			_("Should the plot be displayed before the grids"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_klass->type_name	= gog_histogram_plot_type_name;
	gog_object_klass->view_type	= gog_histogram_plot_view_get_type ();
	gog_object_klass->update	= gog_histogram_plot_update;
#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor = gog_histogram_plot_populate_editor;
#endif

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Limits"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("Values"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
		};
		gog_plot_klass->desc.series.dim = dimensions;
		gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
	}
	gog_plot_klass->desc.num_series_max = 1;
	gog_plot_klass->series_type = gog_histogram_plot_series_get_type ();
	gog_plot_klass->axis_set = GOG_AXIS_SET_XY;
	gog_plot_klass->desc.series.style_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	gog_plot_klass->axis_get_bounds   	 = gog_histogram_plot_axis_get_bounds;
}

static void
gog_histogram_plot_init (GogHistogramPlot *hist)
{
	GogPlot *plot = GOG_PLOT (hist);

	plot->rendering_order = GOG_PLOT_RENDERING_BEFORE_AXIS;
	hist->vertical = TRUE;
}

GSF_DYNAMIC_CLASS (GogHistogramPlot, gog_histogram_plot,
	gog_histogram_plot_class_init, gog_histogram_plot_init,
	GOG_TYPE_PLOT)

/*****************************************************************************/
/*   Double histograms                                                       */

static GObjectClass *double_histogram_plot_parent_klass;

#ifdef GOFFICE_WITH_GTK
static void
gog_double_histogram_plot_populate_editor (GogObject	*gobj,
			       GOEditor	*editor,
			       GogDataAllocator	*dalloc,
			       GOCmdContext	*cc)
{
	GtkGrid *grid;
	GtkWidget *w;
	GogDataset *set = GOG_DATASET (gobj);
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_distrib/gog-double-histogram-prefs.ui",
				    GETTEXT_PACKAGE, cc);
	if (gui != NULL) {
		grid = GTK_GRID (gtk_builder_get_object (gui, "double-histogram-prefs"));
		w = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, 0, GOG_DATA_SCALAR));
		gtk_widget_set_tooltip_text (w, _("Label for the first Y category. If not set or empty, \"First values\" will be used."));
		gtk_widget_show (w);
		gtk_widget_set_hexpand (w, TRUE);
		gtk_grid_attach (grid, w, 1, 0, 1, 1);
		w = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, 1, GOG_DATA_SCALAR));
		gtk_widget_set_tooltip_text (w, _("Label for the second Y category. If not set or empty, \"Second values\" will be used."));
		gtk_widget_show (w);
		gtk_widget_set_hexpand (w, TRUE);
		gtk_grid_attach (grid, w, 1, 1, 1, 1);
		go_editor_add_page (editor,
				     go_gtk_builder_get_widget (gui, "double-histogram-prefs"),
				     _("Categories labels"));
	}
	(GOG_OBJECT_CLASS (double_histogram_plot_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
}
#endif

static void
gog_double_histogram_plot_finalize (GObject *obj)
{
	GogDoubleHistogramPlot *plot = GOG_DOUBLE_HISTOGRAM_PLOT (obj);
	if (plot->labels != NULL) {
		gog_dataset_finalize (GOG_DATASET (obj));
		g_free (plot->labels);
		plot->labels = NULL;
	}
	(*double_histogram_plot_parent_klass->finalize) (obj);
}

static void
gog_double_histogram_plot_class_init (GogPlotClass *gog_plot_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) gog_plot_klass;
	GogObjectClass *gog_klass = (GogObjectClass *) gog_plot_klass;
	double_histogram_plot_parent_klass = g_type_class_peek_parent (gog_klass);
	gobject_klass->finalize	    = gog_double_histogram_plot_finalize;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor  = gog_double_histogram_plot_populate_editor;
#endif
	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Limits"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("First values"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
			{ N_("Second values"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_EXTRA1 },
		};
		gog_plot_klass->desc.series.dim = dimensions;
		gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		gog_plot_klass->desc.series.style_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL | GO_STYLE_FONT;
	}
}

static void
gog_double_histogram_plot_init (GogDoubleHistogramPlot *plot)
{
	plot->labels = g_new0 (GogDatasetElement, 2);
}

static void
gog_double_histogram_plot_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = 0;
	*last = 1;
}

static GogDatasetElement *
gog_double_histogram_plot_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogDoubleHistogramPlot const *plot = GOG_DOUBLE_HISTOGRAM_PLOT (set);
	g_return_val_if_fail (2 > dim_i, NULL);
	g_return_val_if_fail (dim_i >= 0, NULL);
	return plot->labels + dim_i;
}

static void
gog_double_histogram_plot_dataset_dim_changed (GogDataset *set, int dim_i)
{
	gog_object_request_update (GOG_OBJECT (set));
}

static void
gog_double_histogram_plot_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_double_histogram_plot_dataset_get_elem;
	iface->dims	   = gog_double_histogram_plot_dataset_dims;
	iface->dim_changed = gog_double_histogram_plot_dataset_dim_changed;
}

GSF_DYNAMIC_CLASS_FULL (GogDoubleHistogramPlot, gog_double_histogram_plot,
	NULL, NULL, gog_double_histogram_plot_class_init, NULL,
        gog_double_histogram_plot_init, GOG_TYPE_HISTOGRAM_PLOT, 0,
        GSF_INTERFACE (gog_double_histogram_plot_dataset_init, GOG_TYPE_DATASET))

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
	double *x_vals = NULL, *y_vals, y0;
	unsigned i;
	GOPath *path ;
	GSList *ptr;
	GOStyle *style;

	if (model->base.series == NULL)
		return;
	series = GOG_HISTOGRAM_PLOT_SERIES (model->base.series->data);
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

	/* clip the plot to it's allocated rectangle, see #684195 */
	gog_renderer_push_clip_rectangle (view->renderer, area->x, area->y, area->w, area->h);

	x_map = gog_chart_map_get_axis_map (chart_map, 0);
	y_map = gog_chart_map_get_axis_map (chart_map, 1);

	if (series->real_x)
		x_vals = series->real_x;
	else if (series->base.values[0].data)
		x_vals = go_data_get_values (series->base.values[0].data);
	y_vals = series->y ? series->y : go_data_get_values (series->base.values[1].data);

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);
	if (model->vertical) {
		double curx = gog_axis_map_to_view (x_map, x_vals ? x_vals[0] : 0);
		go_path_move_to (path, curx, y0 = gog_axis_map_get_baseline (y_map));
		for (i = 0; i < series->base.num_elements; i++) {
			double cury = gog_axis_map_to_view (y_map, y_vals[i]);
			go_path_line_to (path, curx, cury);
			curx = gog_axis_map_to_view (x_map, x_vals ? x_vals[i+1] : 0);
			go_path_line_to (path, curx, cury);
		}
		go_path_line_to (path, curx, y0);
	} else {
		double curx = gog_axis_map_to_view (y_map, x_vals ? x_vals[0] : 0);
		go_path_move_to (path, y0 = gog_axis_map_get_baseline (x_map), curx);
		for (i = 0; i < series->base.num_elements; i++) {
			double cury = gog_axis_map_to_view (x_map, y_vals[i]);
			go_path_line_to (path, cury, curx);
			curx = gog_axis_map_to_view (y_map, x_vals ? x_vals[i+1] : 0);
			go_path_line_to (path, cury, curx);
		}
		go_path_line_to (path, y0, curx);
	}
	go_path_close (path);
	gog_renderer_push_style (view->renderer, style);
	gog_renderer_fill_shape (view->renderer, path);

	if (series->droplines) {
		GOPath *drop_path = go_path_new ();
		double cury = y0;
		go_path_set_options (drop_path, GO_PATH_OPTIONS_SHARP);
		gog_renderer_push_style (view->renderer,
			go_styled_object_get_style (GO_STYLED_OBJECT (series->droplines)));
		if (model->vertical) {
			for (i = 1; i < series->base.num_elements; i++) {
				double x = x_vals ? x_vals[i] : 0;
				double y = y_vals[i];
				double y_prev = y_vals[i - 1];
				double curx = gog_axis_map_to_view (x_map, x);

				if (pos_product (y, y_prev)) {
					go_path_move_to (drop_path, curx, y0);
					cury = gog_axis_map_to_view
						(y_map,
						 y > 0 ? MIN (y_prev, y) : MAX (y_prev, y));
				} else {
					go_path_move_to (drop_path, curx, cury);
					cury = gog_axis_map_to_view (y_map, y);
				}
				go_path_line_to (drop_path, curx, cury);
			}
		} else {
			for (i = 1; i < series->base.num_elements; i++) {
				double x = x_vals ? x_vals[i] : 0;
				double y = y_vals[i];
				double y_prev = y_vals[i - 1];
				double curx = gog_axis_map_to_view (y_map, x);

				if (pos_product (y, y_prev)) {
					go_path_move_to (drop_path, y0, curx);
					cury = gog_axis_map_to_view
						(x_map,
						 y > 0 ? MIN (y_prev, y) : MAX (y_prev, y));
				} else {
					go_path_move_to (drop_path, cury, curx);
					cury = gog_axis_map_to_view (x_map, y);
				}
				go_path_line_to (drop_path, cury, curx);
			}
		}
		gog_renderer_stroke_serie (view->renderer, drop_path);
		go_path_free (drop_path);
		gog_renderer_pop_style (view->renderer);
	}
	gog_renderer_stroke_shape (view->renderer, path);
	gog_renderer_pop_style (view->renderer);
	go_path_free (path);
	/* Redo the same for the other side of a double histogram. */
	if (GOG_IS_DOUBLE_HISTOGRAM_PLOT (model) && series->base.values[2].data != NULL) {
		y_vals = series->y_ ? series->y_ : go_data_get_values (series->base.values[2].data);
		path = go_path_new ();
		go_path_set_options (path, GO_PATH_OPTIONS_SHARP);
		if (model->vertical) {
			double curx = gog_axis_map_to_view (x_map, x_vals ? x_vals[0]: 0);
			go_path_move_to (path, curx, y0);
			for (i = 0; i < series->base.num_elements; i++) {
				double cury = gog_axis_map_to_view (y_map, y_vals[i]);
				go_path_line_to (path, curx, cury);
				curx = gog_axis_map_to_view (x_map, (x_vals ? x_vals[i+1]: 0.));
				go_path_line_to (path, curx, cury);
			}
			go_path_line_to (path, curx, y0);
		} else {
			double curx = gog_axis_map_to_view (y_map, x_vals ? x_vals[0]: 0);
			go_path_move_to (path, y0, curx);
			for (i = 0; i < series->base.num_elements; i++) {
				double cury = gog_axis_map_to_view (x_map, y_vals[i]);
				go_path_line_to (path, cury, curx);
				curx = gog_axis_map_to_view (y_map, (x_vals ? x_vals[i+1]: 0.));
				go_path_line_to (path, cury, curx);
			}
			go_path_line_to (path, y0, curx);
		}
		go_path_close (path);
		gog_renderer_push_style (view->renderer, style);
		gog_renderer_fill_shape (view->renderer, path);

		if (series->droplines) {
			GOPath *drop_path = go_path_new ();
			double cury = y0;
			go_path_set_options (drop_path, GO_PATH_OPTIONS_SHARP);
			gog_renderer_push_style (view->renderer,
				go_styled_object_get_style (GO_STYLED_OBJECT (series->droplines)));
			if (model->vertical) {
				for (i = 1; i < series->base.num_elements; i++) {
					double x = x_vals ? x_vals[i] : 0;
					double y = y_vals[i];
					double y_prev = y_vals[i - 1];
					double curx = gog_axis_map_to_view (x_map, x);

					if (pos_product (y_prev, y)) {
						go_path_move_to (drop_path, curx, y0);
						cury = gog_axis_map_to_view
							(y_map,
							 y > 0 ? MIN (y_prev, y) : MAX (y_prev, y));
					} else {
						go_path_move_to (drop_path, curx, cury);
						cury = gog_axis_map_to_view (y_map, y);
					}
					go_path_line_to (drop_path, curx, cury);
				}
			} else {
				for (i = 1; i < series->base.num_elements; i++) {
					double x = x_vals ? x_vals[i] : 0;
					double y = y_vals[i];
					double y_prev = y_vals[i - 1];
					double curx = gog_axis_map_to_view (y_map, x);

					if (pos_product (y_prev, y)) {
						go_path_move_to (drop_path, y0, curx);
						cury = gog_axis_map_to_view
							(x_map,
							 y > 0 ? MIN (y_prev, y) : MAX (y_prev, y));
					} else {
						go_path_move_to (drop_path, cury, curx);
						cury = gog_axis_map_to_view (x_map, y);
					}
					go_path_line_to (drop_path, cury, curx);
				}
			}
			gog_renderer_stroke_serie (view->renderer, drop_path);
			go_path_free (drop_path);
			gog_renderer_pop_style (view->renderer);
		}
		gog_renderer_stroke_shape (view->renderer, path);
		go_path_free (path);

		/* Now display labels FIXME: might be optional */
		{
			char *text1 = NULL, *text2 = NULL;
			char const *text;
			GogViewAllocation alloc;
			GogDoubleHistogramPlot *dbl = GOG_DOUBLE_HISTOGRAM_PLOT (model);
			if (dbl->labels[0].data) {
				text1 = go_data_get_scalar_string (dbl->labels[0].data);
				if (text1 && !*text1) {
					g_free (text1);
					text1 = NULL;
				}
			}
			if (dbl->labels[1].data) {
				text2 = go_data_get_scalar_string (dbl->labels[1].data);
				if (text2 && !*text2) {
					g_free (text2);
					text2 = NULL;
				}
			}
			text = text1 ? text1 : _("First values");
			if (model->cumulative) {
				if (model->vertical) {
					alloc.x = area->x + 2; /* FIXME: replace 2 by something configurable */
					alloc.y = area->y + 2;
					gog_renderer_draw_text (view->renderer, text, &alloc,
							        GO_ANCHOR_NORTH_WEST, FALSE,
					                        GO_JUSTIFY_LEFT, -1.);
					text =  text2 ? text2 : _("Second values");
					alloc.y = area->y + area->h - 2;
					gog_renderer_draw_text (view->renderer, text, &alloc,
							        GO_ANCHOR_SOUTH_WEST, FALSE,
					                        GO_JUSTIFY_LEFT, -1.);
				} else {
					alloc.x = area->x + area->w - 2; /* FIXME: replace 2 by something configurable */
					alloc.y = area->y + area->h - 2;
					gog_renderer_draw_text (view->renderer, text, &alloc,
							        GO_ANCHOR_SOUTH_EAST, FALSE,
					                        GO_JUSTIFY_LEFT, -1.);
					text = text2 ? text2 : _("Second values");
					alloc.x = area->x + 2;
					gog_renderer_draw_text (view->renderer, text, &alloc,
							        GO_ANCHOR_SOUTH_WEST, FALSE,
					                        GO_JUSTIFY_LEFT, -1.);
				}
			} else {
				alloc.x = area->x + area->w - 2; /* FIXME: replace 2 by something configurable */
				alloc.y = area->y + 2;
				gog_renderer_draw_text (view->renderer, text, &alloc,
					                GO_ANCHOR_NORTH_EAST, FALSE,
				                        GO_JUSTIFY_LEFT, -1.);
				text = text2 ? text2 : _("Second values");
				if (model->vertical) {
					alloc.y = area->y + area->h - 2;
					gog_renderer_draw_text (view->renderer, text, &alloc,
							        GO_ANCHOR_SOUTH_EAST, FALSE,
					                        GO_JUSTIFY_LEFT, -1.);
				} else {
					alloc.x = area->x + 2;
					gog_renderer_draw_text (view->renderer, text, &alloc,
							        GO_ANCHOR_NORTH_WEST, FALSE,
					                        GO_JUSTIFY_LEFT, -1.);
				}
			}
			g_free (text1);
			g_free (text2);
		}
		gog_renderer_pop_style (view->renderer);
	}
	gog_renderer_pop_clip (view->renderer);

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
	double *x_vals = NULL, *y_vals = NULL, *y__vals = NULL, cur;
	int x_len = 1, y_len = 0, y__len = 0, max, nb = 0, i, y_0 = 0, y__0 = 0;
	GogHistogramPlotSeries *series = GOG_HISTOGRAM_PLOT_SERIES (obj);
	unsigned old_num = series->base.num_elements;
	GSList *ptr;
	gboolean y_as_raw = FALSE;
	double width = -1., origin = 0.;

	g_free (series->real_x);
	series->real_x = NULL;
	g_free (series->real_y);
	series->real_y = NULL;
	g_free (series->real_y_);
	series->real_y_ = NULL;
	if (series->base.values[1].data != NULL) {
		y_vals = go_data_get_values (series->base.values[1].data);
		nb = y_len = go_data_get_vector_size (series->base.values[1].data);
	}
	if (GOG_IS_DOUBLE_HISTOGRAM_PLOT (series->base.plot) && series->base.values[2].data != NULL) {
		y__vals = go_data_get_values (series->base.values[2].data);
		y__len = go_data_get_vector_size (series->base.values[2].data);
		if (y__len > y_len)
			nb = y__len;
	}
	if (series->base.values[0].data != NULL) {
		x_vals = go_data_get_values (series->base.values[0].data);
		max = go_data_get_vector_size (series->base.values[0].data);
		if (max == 2) {
			origin = x_vals[1];
			max = 1;
		}
		if (max == 1) {
			/* use the value as bin width */
			width = x_vals[0];
			if (!go_finite (width))
				width = -1.;
			y_as_raw = TRUE;
		} else if (max > nb) {
			if (max > 0 && go_finite (x_vals[0])) {
				cur = x_vals[0];
				for (i = 1; i < max; i++) {
					if (go_finite (x_vals[i]) && x_vals[i] > cur) {
						cur = x_vals[i];
						x_len++;
					} else
						break;
				}
			}
		} else {
			x_len = max;
			y_as_raw = TRUE;
		}
	} else
		y_as_raw = TRUE;
	if (y_as_raw) {
		double *y = NULL, *y_ = NULL;
		if (y_vals) {
			y = go_range_sort (y_vals, y_len);
			while (y_0 < y_len && !go_finite (y[y_0]))
				y_0++;
			while (y_len > y_0 && !go_finite (y[y_len-1]))
				y_len--;
		}
		if (y__vals) {
			y_ = go_range_sort (y__vals, y__len);
			while (y__0 < y__len && !go_finite (y[y__0]))
				y__0++;
			while (y__len > y__0 && !go_finite (y_[y__len-1]))
				y__len--;
		}
		if (!x_vals || x_len <= 1) {
			/* guess reasonable values */
			if (width <= 0) {
				max = go_fake_round (sqrt (MAX (y_len, y__len)));
				if (y && y_len - y_0 > 2)
					width = (y[y_len-1] - y[y_0]) / max;
				if (y_ && y__len - y__0 > 2) {
					double w_ = (y_[y__len-1] - y_[y__0]) / max;
					width = MAX (width, w_);
				}
				if (width > 0.) {
					width = log10 (width);
					max = floor (width);
					width -= max;
					if (width < 0.15)
						width = pow (10, max);
					else if (width < 0.45)
						width = 2 * pow (10, max);
					else if (width < 0.85)
						width = 2 * pow (10, max);
					else
						width = pow (10, max + 1);
				}
			}
			if (width > 0. && (y || y_)) {
				double m, M, nm;
				/* ignore nans */
				if (y) {
					m = y_ ? (MIN (y[y_0], y_[y__0])) : y[y_0];
					M = y_ ? MAX (y[y_len-1], y_[y__len-1]) : y[y_len-1];
				} else {
					m = y_[y__0];
					M = y_[y__len-1];
				}
				if (!go_finite (m) || !go_finite (M))
					return;
				/* round m */
				nm = floor ((m - origin)/ width) * width + origin;
				/* we need to include all data so we must ensure that the
				 * first bin must be lower than the first value, see  #742996 */
				m = (nm == m)? nm - width: nm;
				x_len = ceil ((M - m) / width) + 1;
				series->real_x = g_new (double, x_len);
				for (max = 0; max < x_len; max++) {
					series->real_x[max] = m;
					m += width;
				}
			}
		} else
			series->real_x = go_range_sort (x_vals, x_len);
		if (x_len > 1) {
			series->real_y = g_new0 (double, x_len - 1);
			for (i = 0; i < y_len && y[i] <= series->real_x[0]; i++);
			for (max = 1; i < y_len; i++)
				if (go_finite (y[i])) {
					while (max < x_len - 1 && y[i] > series->real_x[max])
						max++;
					if (y[i] >  series->real_x[max])
						break;
					series->real_y[max-1]++;
			}
			if (y__len > 0) {
				series->real_y_ = g_new0 (double, x_len - 1);
				for (i = 0; i < y__len && y_[i] <= series->real_x[0]; i++);
				for (max = 1;  i < y__len; i++)
					if (go_finite (y_[i])) {
						while (y_[i] > series->real_x[max]) {
							max++;
							if (max == x_len - 1)
								break;
						}
						series->real_y_[max-1]++;
			}
			}
		}
	}
	series->base.num_elements = (x_len > 0 ? MIN (x_len - 1, nb) : 0);

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
	g_free (series->y_);
	series->y_ = NULL;
	g_free (series->x);
	series->x = NULL;
	g_free (series->real_x);
	series->real_x = NULL;
	g_free (series->real_y);
	series->real_y = NULL;
	g_free (series->real_y_);
	series->real_y_ = NULL;

	G_OBJECT_CLASS (series_parent_klass)->finalize (obj);
}

static unsigned
gog_histogram_plot_series_get_xy_data (GogSeries const *series,
					double const **x, double const **y)
{
	GogHistogramPlotSeries *hist_ser = GOG_HISTOGRAM_PLOT_SERIES (series);

	*x = hist_ser->x;
	*y = hist_ser->y ? hist_ser->y : go_data_get_values (series->values[1].data);

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
