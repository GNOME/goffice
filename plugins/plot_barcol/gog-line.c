/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-line.c
 *
 * Copyright (C) 2003-2004 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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
#include "gog-line.h"
#include "gog-1.5d.h"
#include <goffice/graph/gog-series-lines.h>
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/data/go-data.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-marker.h>
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-style.h>
#include <goffice/utils/go-styled-object.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

struct _GogLinePlot {
	GogPlot1_5d	base;
	gboolean	default_style_has_markers;
};

static GType gog_line_view_get_type (void);

enum {
	GOG_LINE_PROP_0,
	GOG_LINE_PROP_DEFAULT_STYLE_HAS_MARKERS
};

/*****************************************************************************/

typedef GogSeriesElement GogLineSeriesElement;
typedef GogSeriesElementClass GogLineSeriesElementClass;

#define GOG_TYPE_LINE_SERIES_ELEMENT	(gog_line_series_element_get_type ())
#define GOG_LINE_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_LINE_SERIES_ELEMENT, GogLinelSeriesElement))
#define GOG_IS_LINE_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_LINE_SERIES_ELEMENT))
GType gog_line_series_element_get_type (void);

static void
gog_line_series_element_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogSeries const *series = GOG_SERIES (GOG_OBJECT (gso)->parent);

	g_return_if_fail (series != NULL);

	style->interesting_fields = GO_STYLE_MARKER;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), GOG_SERIES_ELEMENT (gso)->index, GO_STYLE_MARKER);
}

static void
gog_line_series_element_class_init (GogLineSeriesElementClass *klass)
{
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) klass;
	style_klass->init_style	    	= gog_line_series_element_init_style;
}

GSF_DYNAMIC_CLASS (GogLineSeriesElement, gog_line_series_element,
	gog_line_series_element_class_init, NULL,
	GOG_TYPE_SERIES_ELEMENT)

/******************************************************************************/

typedef GogView		GogLineSeriesView;
typedef GogViewClass	GogLineSeriesViewClass;

#define GOG_TYPE_LINE_SERIES_VIEW	(gog_line_series_view_get_type ())
#define GOG_LINE_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_LINE_SERIES_VIEW, GogLineSeriesView))
#define GOG_IS_LINE_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_LINE_SERIES_VIEW))

static void
gog_line_series_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr;
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);
}

static void
gog_line_series_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;

	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
}

static void
gog_line_series_view_class_init (GogLineSeriesViewClass *gview_klass)
{
	GogViewClass *view_klass = GOG_VIEW_CLASS (gview_klass);
	view_klass->render = gog_line_series_view_render;
	view_klass->size_allocate = gog_line_series_view_size_allocate;
	view_klass->build_toolkit = NULL;
}

GSF_DYNAMIC_CLASS (GogLineSeriesView, gog_line_series_view,
	gog_line_series_view_class_init, NULL,
	GOG_TYPE_VIEW)

/*****************************************************************************/

typedef struct {
	GogSeries1_5d base;
	double *x;
} GogLineSeries;
typedef GogSeries1_5dClass	GogLineSeriesClass;

static GogStyledObjectClass *series_parent_klass;

GType gog_line_series_get_type (void);
#define GOG_TYPE_LINE_SERIES	(gog_line_series_get_type ())
#define GOG_LINE_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_LINE_SERIES, GogLineSeries))
#define GOG_IS_LINE_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_LINE_SERIES))

static void
gog_line_series_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogSeries *series = GOG_SERIES (gso);
	GogLinePlot const *plot;

	series_parent_klass->init_style (gso, style);
	if (series->plot == NULL)
		return;

	plot = GOG_LINE_PLOT (series->plot);

	if (!plot->default_style_has_markers) {
		style->disable_theming |= GO_STYLE_MARKER;
		if (style->marker.auto_shape) {
			GOMarker *m = go_marker_new ();
			go_marker_set_shape (m, GO_MARKER_NONE);
			go_style_set_marker (style, m);
		}
	}
}

static void
gog_line_series_update (GogObject *obj)
{
	GogLineSeries *series = GOG_LINE_SERIES (obj);
	unsigned i, nb = series->base.base.num_elements;
	GSList *ptr;
	(GOG_OBJECT_CLASS (series_parent_klass))->update (obj);
	if (nb != series->base.base.num_elements) {
		nb = series->base.base.num_elements;
		g_free (series->x);
		series->x = g_new (double, nb);
		for (i = 0; i < nb; i++)
			series->x[i] = i + 1;
	}
	/* update children */
	for (ptr = obj->children; ptr != NULL; ptr = ptr->next)
		if (!GOG_IS_SERIES_LINES (ptr->data))
			gog_object_request_update (GOG_OBJECT (ptr->data));
}

static unsigned
gog_line_series_get_xy_data (GogSeries const *series,
					double const **x, double const **y)
{
	GogLineSeries *line_ser = GOG_LINE_SERIES (series);
	*x = line_ser->x;
	*y = go_data_get_values (series->values[1].data);
	return series->num_elements;
}

static void
gog_line_series_finalize (GObject *obj)
{
	GogLineSeries *series = GOG_LINE_SERIES (obj);

	g_free (series->x);
	series->x = NULL;

	G_OBJECT_CLASS (series_parent_klass)->finalize (obj);
}

static void
gog_line_series_class_init (GogStyledObjectClass *gso_klass)
{
	GObjectClass *obj_klass = (GObjectClass *)gso_klass;
	GogObjectClass *gog_klass = (GogObjectClass *)gso_klass;
	GogSeriesClass *series_klass = (GogSeriesClass*) gso_klass;
	series_parent_klass = g_type_class_peek_parent (gso_klass);
	obj_klass->finalize = gog_line_series_finalize;
	gso_klass->init_style = gog_line_series_init_style;
	gog_klass->view_type = gog_line_series_view_get_type ();
	gog_klass->update = gog_line_series_update;
	series_klass->get_xy_data = gog_line_series_get_xy_data;
	series_klass->series_element_type = GOG_TYPE_LINE_SERIES_ELEMENT;
}

GSF_DYNAMIC_CLASS (GogLineSeries, gog_line_series,
	gog_line_series_class_init, NULL,
	GOG_SERIES1_5D_TYPE)

static void
child_added_cb (GogLinePlot *plot, GogObject *obj)
{
	/* we only accept regression curves for not stacked plots */
	if (GOG_IS_SERIES (obj) && plot->base.type == GOG_1_5D_NORMAL)
		(GOG_SERIES (obj))->acceptable_children =
					GOG_SERIES_ACCEPT_TREND_LINE;
}

static char const *
gog_line_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name bar/col plot objects
	 * eg The 2nd line plot in a chart will be called
	 * 	PlotLine2
	 */
	return N_("PlotLine");
}

static void
gog_line_update_stacked_and_percentage (GogPlot1_5d *model,
					double **vals, GogErrorBar **errors, unsigned const *lengths)
{
	unsigned i, j;
	double abs_sum, minima, maxima, sum, tmp, errplus, errminus;

	for (i = model->num_elements ; i-- > 0 ; ) {
		abs_sum = sum = 0.;
		minima =  DBL_MAX;
		maxima = -DBL_MAX;
		for (j = 0 ; j < model->num_series ; j++) {
			if (i >= lengths[j])
				continue;
			tmp = vals[j][i];
			if (!go_finite (tmp))
				continue;
			if (gog_error_bar_is_visible (errors[j])) {
				gog_error_bar_get_bounds (errors[j], i, &errminus, &errplus);
				errminus = errminus > 0. ? errminus: 0.;
				errplus = errplus > 0. ? errplus : 0.;
			} else
				errplus = errminus = 0.;
			sum += tmp;
			abs_sum += fabs (tmp);
			if (minima > sum - errminus)
				minima = sum - errminus;
			if (maxima < sum + errplus)
				maxima = sum + errplus;
		}
		if ((model->type == GOG_1_5D_AS_PERCENTAGE) &&
		    (go_sub_epsilon (abs_sum) > 0.)) {
			if (model->minima > minima / abs_sum)
				model->minima = minima / abs_sum;
			if (model->maxima < maxima / abs_sum)
				model->maxima = maxima / abs_sum;
		} else {
			if (model->minima > minima)
				model->minima = minima;
			if (model->maxima < maxima)
				model->maxima = maxima;
		}
	}
}

static void
gog_line_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogLinePlot *line = GOG_LINE_PLOT (obj);
	switch (param_id) {
	case GOG_LINE_PROP_DEFAULT_STYLE_HAS_MARKERS:
		line->default_style_has_markers = g_value_get_boolean (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}
static void
gog_line_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogLinePlot const *line = GOG_LINE_PLOT (obj);
	switch (param_id) {
	case GOG_LINE_PROP_DEFAULT_STYLE_HAS_MARKERS:
		g_value_set_boolean (value, line->default_style_has_markers);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_line_plot_class_init (GogPlot1_5dClass *gog_plot_1_5d_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) gog_plot_1_5d_klass;
	GogObjectClass *gog_klass = (GogObjectClass *) gog_plot_1_5d_klass;
	GogPlotClass *plot_klass = (GogPlotClass *) gog_plot_1_5d_klass;

	gobject_klass->set_property = gog_line_set_property;
	gobject_klass->get_property = gog_line_get_property;

	g_object_class_install_property (gobject_klass, GOG_LINE_PROP_DEFAULT_STYLE_HAS_MARKERS,
		g_param_spec_boolean ("default-style-has-markers",
			_("Default markers"),
			_("Should the default style of a series include markers"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_klass->type_name	= gog_line_plot_type_name;
	gog_klass->view_type	= gog_line_view_get_type ();

	plot_klass->desc.series.style_fields = GO_STYLE_LINE | GO_STYLE_MARKER;
	plot_klass->series_type = gog_line_series_get_type ();

	gog_plot_1_5d_klass->update_stacked_and_percentage =
		gog_line_update_stacked_and_percentage;
}

static void
gog_line_plot_init (GogLinePlot *plot)
{
	plot->default_style_has_markers = TRUE;
	g_signal_connect (G_OBJECT (plot), "child-added",
					G_CALLBACK (child_added_cb), NULL);
	GOG_PLOT1_5D (plot)->support_drop_lines = TRUE;
}

GSF_DYNAMIC_CLASS (GogLinePlot, gog_line_plot,
	gog_line_plot_class_init, gog_line_plot_init,
	GOG_PLOT1_5D_TYPE)

/*****************************************************************************/

static GogObjectClass *gog_area_plot_parent_klass;

enum {
	AREA_PROP_FILL_0,
	AREA_PROP_FILL_BEFORE_GRID
};

static void
gog_area_plot_set_property (GObject *obj, guint param_id,
		     GValue const *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);
	switch (param_id) {
	case AREA_PROP_FILL_BEFORE_GRID:
		plot->rendering_order = (g_value_get_boolean (value))?
					 GOG_PLOT_RENDERING_BEFORE_GRID:
					 GOG_PLOT_RENDERING_LAST;
		gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_area_plot_get_property (GObject *obj, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);

	switch (param_id) {
	case AREA_PROP_FILL_BEFORE_GRID:
		g_value_set_boolean (value, plot->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
static void
display_before_grid_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "before-grid", gtk_toggle_button_get_active (btn), NULL);
}
#endif

static void
gog_area_plot_populate_editor (GogObject *obj,
			     GOEditor *editor,
                             GogDataAllocator *dalloc,
                             GOCmdContext *cc)
{
#ifdef GOFFICE_WITH_GTK
	GtkBuilder *gui;
	char const *dir;
	char *path;

	dir = go_plugin_get_dir_name (go_plugins_get_plugin_by_id ("GOffice_plot_barcol"));
	path = g_build_filename (dir, "gog-area-prefs.ui", NULL);
	gui = go_gtk_builder_new (path, GETTEXT_PACKAGE, cc);
	g_free (path);

	if (gui != NULL) {
		GtkWidget *w = go_gtk_builder_get_widget (gui, "before-grid");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				(GOG_PLOT (obj))->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (display_before_grid_cb), obj);
		w = go_gtk_builder_get_widget (gui, "gog-area-prefs");
		go_editor_add_page (editor, w, _("Properties"));
		g_object_unref (gui);
	}

#endif
	gog_area_plot_parent_klass->populate_editor (obj, editor, dalloc, cc);
};

static char const *
gog_area_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name bar/col plot objects
	 * eg The 2nd line plot in a chart will be called
	 * 	PlotArea2
	 */
	return N_("PlotArea");
}

static void
gog_area_plot_class_init (GObjectClass *gobject_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *) gobject_klass;
	GogPlotClass   *plot_klass = (GogPlotClass *) gobject_klass;
	gog_area_plot_parent_klass = g_type_class_peek_parent (gobject_klass);

	gobject_klass->set_property = gog_area_plot_set_property;
	gobject_klass->get_property = gog_area_plot_get_property;
	g_object_class_install_property (gobject_klass, AREA_PROP_FILL_BEFORE_GRID,
		g_param_spec_boolean ("before-grid",
			_("Displayed under the grids"),
			_("Should the plot be displayed before the grids"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	plot_klass->desc.series.style_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	plot_klass->series_type = gog_series1_5d_get_type ();

	gog_klass->populate_editor = gog_area_plot_populate_editor;
	gog_klass->type_name	= gog_area_plot_type_name;
}

static void
gog_area_plot_init (GogPlot *plot)
{
	plot->rendering_order = GOG_PLOT_RENDERING_BEFORE_AXIS;
	GOG_PLOT1_5D (plot)->support_drop_lines = TRUE;
}

GSF_DYNAMIC_CLASS (GogAreaPlot, gog_area_plot,
	gog_area_plot_class_init, gog_area_plot_init,
	GOG_TYPE_LINE_PLOT)

/*****************************************************************************/

typedef struct {
	double 		x;
	double 		y;
	double		plus;
	double 		minus;
} ErrorBarData;

typedef struct {
	double x;
	double y;
} Point;

typedef GogPlotView		GogLineView;
typedef GogPlotViewClass	GogLineViewClass;

static void
gog_line_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogPlot1_5d const *model = GOG_PLOT1_5D (view->model);
	GogPlot1_5dType const type = model->type;
	GogSeries1_5d const *series;
	GogChart *chart = GOG_CHART (view->model->parent);
	GogChartMap *chart_map;
	GogViewAllocation const *area;
	unsigned i, j;
	unsigned num_elements = model->num_elements;
	unsigned num_series = model->num_series;
	GSList *ptr;
	double plus, minus;

	double **vals;
	ErrorBarData **error_data;
	GOStyle **styles;
	unsigned *lengths;
	GOPath **paths;
	GOPath **drop_paths;
	Point **points = NULL;
	GogErrorBar **errors;
	GogObjectRole const *role = NULL;
	GogSeriesLines **lines;

	double y_zero, y_top, y_bottom, drop_lines_y_min, drop_lines_y_max;
	double abs_sum, sum, value, x, y = 0.;
	gboolean is_null, is_area_plot;

	GogAxisMap *x_map, *y_map;

	GogSeriesElement *gse;
	GList const *overrides;

	is_area_plot = GOG_IS_PLOT_AREA (model);

	if (num_elements <= 0 || num_series <= 0)
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

	/* Draw drop lines from point to axis start. See comment in
	 * GogXYPlotView::render */

	gog_axis_map_get_extents (y_map, &drop_lines_y_min, &drop_lines_y_max);
	drop_lines_y_min = gog_axis_map_to_view (y_map, drop_lines_y_min);
	drop_lines_y_max = gog_axis_map_to_view (y_map, drop_lines_y_max);
	gog_axis_map_get_extents (y_map, &y_bottom, &y_top);
	y_bottom = gog_axis_map_to_view (y_map, y_bottom);
	y_top = gog_axis_map_to_view (y_map, y_top);
	y_zero = gog_axis_map_get_baseline (y_map);

	vals    = g_alloca (num_series * sizeof (double *));
	error_data = g_alloca (num_series * sizeof (ErrorBarData *));
	lengths = g_alloca (num_series * sizeof (unsigned));
	styles  = g_alloca (num_series * sizeof (GOStyle *));
	paths	= g_alloca (num_series * sizeof (GOPath *));
	if (!is_area_plot)
		points  = g_alloca (num_series * sizeof (Point *));
	errors	= g_alloca (num_series * sizeof (GogErrorBar *));
	lines	= g_alloca (num_series * sizeof (GogSeriesLines *));
	drop_paths = g_alloca (num_series * sizeof (GOPath *));

	i = 0;
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;

		if (!gog_series_is_valid (GOG_SERIES (series))) {
			vals[i] = NULL;
			lengths[i] = 0;
			continue;
		}

		vals[i] = go_data_get_values (series->base.values[1].data);
		lengths[i] = go_data_get_vector_size (series->base.values[1].data);
		styles[i] = GOG_STYLED_OBJECT (series)->style;

		paths[i] = go_path_new ();
		if (!is_area_plot)
			points[i] = g_malloc (sizeof (Point) * (lengths[i]));

		errors[i] = series->errors;
		if (gog_error_bar_is_visible (series->errors))
			error_data[i] = g_malloc (sizeof (ErrorBarData) * lengths[i]);
		else
			error_data[i] = NULL;
		if (series->has_drop_lines) {
			if (!role)
				role = gog_object_find_role_by_name (
							GOG_OBJECT (series), "Drop lines");
			lines[i] = GOG_SERIES_LINES (
					gog_object_get_child_by_role (GOG_OBJECT (series), role));
			drop_paths [i] = go_path_new ();
		} else
			lines[i] = NULL;
		i++;
	}

	for (j = 0; j < num_elements; j++) {
		sum = abs_sum = 0.0;
		if (type == GOG_1_5D_AS_PERCENTAGE) {
			for (i = 0; i < num_series; i++)
				if (vals[i] && gog_axis_map_finite (y_map, vals[i][j]))
					abs_sum += fabs (vals[i][j]);
			is_null = (go_sub_epsilon (abs_sum) <= 0.);
		} else
			is_null = TRUE;

		for (i = 0; i < num_series; i++) {
			if (j >= lengths[i])
				continue;

			if (vals[i] && gog_axis_map_finite (y_map, vals[i][j])) {
				value = vals[i][j];
				if (gog_error_bar_is_visible (errors[i])) {
					gog_error_bar_get_bounds (errors[i], j, &minus, &plus);
				}
			} else if (type == GOG_1_5D_NORMAL && !is_area_plot) {
				value = go_nan;
				minus = -1.0;
				plus = -1.;
			} else {
				value = 0.0;
				minus = -1.0;
				plus = -1.;
			}

			x = gog_axis_map_to_view (x_map, j + 1);
			sum += value;

			if (gog_error_bar_is_visible (errors[i]))
				error_data[i][j].x = j + 1;

			switch (type) {
				case GOG_1_5D_NORMAL :
					y = gog_axis_map_finite (y_map, value) ?
						gog_axis_map_to_view (y_map, value) :
					((is_area_plot)? y_zero: go_nan);
					if (gog_error_bar_is_visible (errors[i])) {
						error_data[i][j].y = value;
						error_data[i][j].minus = minus;
						error_data[i][j].plus = plus;
					}
					if (isnan (y))
						break;
					if (!j || (!is_area_plot && isnan (points[i][j-1].y)))
						go_path_move_to (paths[i], x, y);
					else
						go_path_line_to (paths[i], x, y);
					break;

				case GOG_1_5D_STACKED :
					y = gog_axis_map_finite (y_map, sum) ?
						gog_axis_map_to_view (y_map, sum) :
						y_zero;
					if (gog_error_bar_is_visible (errors[i])) {
						error_data[i][j].y = sum;
						error_data[i][j].minus = minus;
						error_data[i][j].plus = plus;
					}
					if (!j)
						go_path_move_to (paths[i], x, y);
					else
						go_path_line_to (paths[i], x, y);
					break;

				case GOG_1_5D_AS_PERCENTAGE :
					y = is_null ?
						y_zero :
						(gog_axis_map_finite (y_map, sum) ?
						 gog_axis_map_to_view (y_map, sum  / abs_sum) :
						 y_zero);
					if (gog_error_bar_is_visible (errors[i])) {
						error_data[i][j].y = is_null ? 0. : sum / abs_sum;
						error_data[i][j].minus = is_null ? -1. : minus / abs_sum;
						error_data[i][j].plus = is_null ? -1. : plus / abs_sum;
					}
					if (!j)
						go_path_move_to (paths[i], x, y);
					else
						go_path_line_to (paths[i], x, y);
					break;
			}
			if (!is_area_plot){
				points[i][j].x = x;
				points[i][j].y = y;
			}
			if (lines[i] && go_finite (y)) {
				double y_target;
				GogAxis *axis = GOG_PLOT (model)->axis[GOG_AXIS_Y];
				GogAxisPosition pos = gog_axis_base_get_clamped_position (GOG_AXIS_BASE (axis));
				switch (pos) {
				case GOG_AXIS_AT_LOW:
					y_target = gog_axis_map_is_inverted (y_map)? y_top: y_bottom;
					break;
				case GOG_AXIS_CROSS: {
					GogChartMap *c_map;
					GogAxisMap *a_map;
					c_map = gog_chart_map_new (chart, area, axis,
							gog_axis_base_get_crossed_axis (GOG_AXIS_BASE (axis)),
							NULL, FALSE);
					a_map = gog_chart_map_get_axis_map (c_map, 1);
					y_target = gog_axis_map_to_view (a_map, gog_axis_base_get_cross_location (GOG_AXIS_BASE (axis)));
					gog_chart_map_free (c_map);
					break;
				}
				case GOG_AXIS_AT_HIGH:
					y_target = gog_axis_map_is_inverted (y_map)? y_bottom: y_top;
					break;
				default:
					/* this should not occur */
					y_target = gog_axis_map_to_view (y_map, 0);
					break;
				}
				go_path_move_to (drop_paths[i], x, y);
				go_path_line_to (drop_paths[i], x, y_target);
			}

		}
	}

	gog_renderer_push_clip_rectangle (view->renderer, view->allocation.x, view->allocation.y,
					  view->allocation.w, view->allocation.h);

	for (i = 0; i < num_series; i++) {
		if (lengths[i] == 0)
			continue;

		gog_renderer_push_style (view->renderer, styles[i]);

		if (!is_area_plot)
			gog_renderer_stroke_serie (view->renderer, paths[i]);
		else {
			if (type == GOG_1_5D_NORMAL || i == 0) {
				GOPath *close_path = go_path_new ();
				go_path_move_to (close_path, gog_axis_map_to_view (x_map, 1), y_zero);
				go_path_line_to (close_path, gog_axis_map_to_view (x_map, lengths[i] + 1), y_zero);
				gog_renderer_fill_serie (view->renderer, paths[i], close_path);
				gog_renderer_stroke_serie (view->renderer, close_path);
				go_path_free (close_path);
			} else {
				gog_renderer_fill_serie (view->renderer, paths[i], paths[i-1]);
			}
			gog_renderer_stroke_serie (view->renderer, paths[i]);
		}

		gog_renderer_pop_style (view->renderer);
	}

	/*Now draw drop lines */
	for (i = 0; i < num_series; i++)
		if (lines[i] != NULL) {
			gog_renderer_push_style (view->renderer,
				go_styled_object_get_style (GO_STYLED_OBJECT (lines[i])));
			gog_series_lines_stroke (lines[i], view->renderer, bbox, drop_paths[i], FALSE);
			gog_renderer_pop_style (view->renderer);
			go_path_free (drop_paths[i]);
		}

	/*Now draw error bars */
	for (i = 0; i < num_series; i++)
		if (gog_error_bar_is_visible (errors[i]))
			for (j = 0; j < lengths[i]; j++)
				gog_error_bar_render (errors[i], view->renderer, chart_map,
						      error_data[i][j].x, error_data[i][j].y,
						      error_data[i][j].minus, error_data[i][j].plus,
						      GOG_ERROR_BAR_DIRECTION_VERTICAL);

	gog_renderer_pop_clip (view->renderer);

	/*Now draw markers*/
	if (!is_area_plot) {
		double x_margin_min, x_margin_max, y_margin_min, y_margin_max, margin;

		margin = gog_renderer_line_size (view->renderer, 1.0);
		x_margin_min = view->allocation.x - margin;
		x_margin_max = view->allocation.x + view->allocation.w + margin;
		y_margin_min = view->allocation.y - margin;
		y_margin_max = view->allocation.y + view->allocation.h + margin;

		ptr = model->base.series;
		for (i = 0; i < num_series; i++) {
			overrides = gog_series_get_overrides (GOG_SERIES (ptr->data));
			if (lengths[i] == 0)
				continue;

				gog_renderer_push_style (view->renderer, styles[i]);

			for (j = 0; j < lengths[i]; j++) {
				x = points[i][j].x;
				y = points[i][j].y;
				if (isnan (y)) {
					if ((overrides != NULL) &&
						(GOG_SERIES_ELEMENT (overrides->data)->index == j))
						overrides = overrides->next;
					continue;
				}
				gse = NULL;
				if ((overrides != NULL) &&
					(GOG_SERIES_ELEMENT (overrides->data)->index == j)) {
						gse = GOG_SERIES_ELEMENT (overrides->data);
						overrides = overrides->next;
						gog_renderer_push_style (view->renderer,
							go_styled_object_get_style (
								GO_STYLED_OBJECT (gse)));
				}
				if (x_margin_min <= x && x <= x_margin_max &&
				    y_margin_min <= y && y <= y_margin_max)
					gog_renderer_draw_marker (view->renderer, x, y);
				if (gse)
					gog_renderer_pop_style (view->renderer);
			}
			gog_renderer_pop_style (view->renderer);
			ptr = ptr->next;
		}
	}

	for (i = 0; i < num_series; i++) {
		if (!is_area_plot)
			g_free (points[i]);
		g_free (error_data[i]);
		go_path_free (paths[i]);
	}

	/* Now render children */
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);

	gog_chart_map_free (chart_map);
}

static GogViewClass *line_view_parent_klass;

static void
gog_line_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;
	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
	(line_view_parent_klass->size_allocate) (view, allocation);
}

static void
gog_line_view_class_init (GogViewClass *view_klass)
{
	line_view_parent_klass = (GogViewClass*) g_type_class_peek_parent (view_klass);
	view_klass->render	  = gog_line_view_render;
	view_klass->size_allocate = gog_line_view_size_allocate;
}

GSF_DYNAMIC_CLASS (GogLineView, gog_line_view,
	gog_line_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)
