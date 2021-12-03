/*
 * go-barcol.c
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#include "gog-barcol.h"
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-series-lines.h>
#include <goffice/graph/gog-view.h>
#include <goffice/data/go-data.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-style.h>
#include <goffice/utils/go-styled-object.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

/******************************************************************************/

typedef GogSeriesElement GogBarColSeriesElement;
typedef GogSeriesElementClass GogBarColSeriesElementClass;

#define GOG_TYPE_BARCOL_SERIES_ELEMENT	(gog_barcol_series_element_get_type ())
#define GOG_BARCOL_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_BARCOL_SERIES_ELEMENT, GogBarColSeriesElement))
#define GOG_IS_BARCOL_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_BARCOL_SERIES_ELEMENT))
GType gog_barcol_series_element_get_type (void);

GSF_DYNAMIC_CLASS (GogBarColSeriesElement, gog_barcol_series_element,
	NULL, NULL,
	GOG_TYPE_SERIES_ELEMENT)

/******************************************************************************/

typedef GogSeries1_5d GogBarColSeries;
typedef GogSeries1_5dClass GogBarColSeriesClass;

#define GOG_TYPE_BARCOL_SERIES	(gog_barcol_series_eget_type ())
#define GOG_BARCOL_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_BARCOL_SERIES, GogBarColSeries))
#define GOG_IS_BARCOL_SERIEST(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_BARCOL_SERIES))
GType gog_barcol_series_get_type (void);

static void
gog_barcol_series_class_init (GogSeriesClass *series_klass)
{
	series_klass->series_element_type = GOG_TYPE_BARCOL_SERIES_ELEMENT;
}

static void
gog_barcol_series_init (GogSeries *series)
{
	series->allowed_pos = GOG_SERIES_LABELS_CENTERED | GOG_SERIES_LABELS_OUTSIDE
		| GOG_SERIES_LABELS_INSIDE | GOG_SERIES_LABELS_NEAR_ORIGIN;
}

GSF_DYNAMIC_CLASS (GogBarColSeries, gog_barcol_series,
	gog_barcol_series_class_init, gog_barcol_series_init,
	GOG_SERIES1_5D_TYPE)

/******************************************************************************/

enum {
	BARCOL_PROP_0,
	BARCOL_PROP_GAP_PERCENTAGE,
	BARCOL_PROP_OVERLAP_PERCENTAGE,
	BARCOL_PROP_HORIZONTAL,
	BARCOL_PROP_FILL_BEFORE_GRID
};

static GogObjectClass *gog_barcol_parent_klass;

static GType gog_barcol_view_get_type (void);

static void
gog_barcol_plot_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GogBarColPlot *barcol = GOG_BARCOL_PLOT (obj);

	switch (param_id) {
	case BARCOL_PROP_GAP_PERCENTAGE:
		barcol->gap_percentage = g_value_get_int (value);
		break;

	case BARCOL_PROP_OVERLAP_PERCENTAGE:
		barcol->overlap_percentage = g_value_get_int (value);
		break;
	case BARCOL_PROP_HORIZONTAL:
		barcol->horizontal = g_value_get_boolean (value);
		break;
	case BARCOL_PROP_FILL_BEFORE_GRID:
		GOG_PLOT (obj)->rendering_order = (g_value_get_boolean (value))?
						GOG_PLOT_RENDERING_BEFORE_GRID:
						GOG_PLOT_RENDERING_BEFORE_AXIS;
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
}

static void
gog_barcol_plot_get_property (GObject *obj, guint param_id,
			      GValue *value, GParamSpec *pspec)
{
	GogBarColPlot *barcol = GOG_BARCOL_PLOT (obj);

	switch (param_id) {
	case BARCOL_PROP_GAP_PERCENTAGE:
		g_value_set_int (value, barcol->gap_percentage);
		break;
	case BARCOL_PROP_OVERLAP_PERCENTAGE:
		g_value_set_int (value, barcol->overlap_percentage);
		break;
	case BARCOL_PROP_HORIZONTAL:
		g_value_set_boolean (value, barcol->horizontal);
		break;
	case BARCOL_PROP_FILL_BEFORE_GRID:
		g_value_set_boolean (value, GOG_PLOT (obj)->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static char const *
gog_barcol_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name bar/col plot objects
	 * eg The 2nd bar/col plot in a chart will be called
	 * 	PlotBarCol2 */
	return N_("PlotBarCol");
}

#ifdef GOFFICE_WITH_GTK
extern gpointer gog_barcol_plot_pref (GogBarColPlot *barcol, GOCmdContext *cc);
static void
gog_barcol_plot_populate_editor (GogObject *item,
				 GOEditor *editor,
			G_GNUC_UNUSED GogDataAllocator *dalloc,
			GOCmdContext *cc)
{
	GtkWidget *w = gog_barcol_plot_pref (GOG_BARCOL_PLOT (item), cc);
	go_editor_add_page (editor, w, _("Properties"));
	g_object_unref (w);
	(GOG_OBJECT_CLASS(gog_barcol_parent_klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

static gboolean
gog_barcol_swap_x_and_y (GogPlot1_5d *model)
{
	return GOG_BARCOL_PLOT (model)->horizontal;
}

static void
gog_barcol_update_stacked_and_percentage (GogPlot1_5d *model,
					  double **vals, GogErrorBar **errors, unsigned const *lengths)
{
	unsigned i, j;
	double neg_sum, pos_sum, tmp, errplus, errminus, tmpmin, tmpmax;

	for (i = model->num_elements ; i-- > 0 ; ) {
		neg_sum = pos_sum = 0.;
		tmpmin = DBL_MAX;
		tmpmax = -DBL_MAX;
		for (j = 0 ; j < model->num_series ; j++) {
			if (i >= lengths[j])
				continue;
			tmp = vals[j][i];
			if (!go_finite (tmp))
				continue;
			if (gog_error_bar_is_visible (errors[j])) {
					gog_error_bar_get_bounds (errors[j], i, &errminus, &errplus);
					errminus = errminus > 0. ? errminus : 0.;
					errplus = errplus > 0. ? errplus : 0.;
				} else
					errplus = errminus = 0.;
			if (tmp > 0.) {
				pos_sum += tmp;
				errminus = (pos_sum - errminus < neg_sum)? neg_sum - pos_sum + errminus: 0.;
			} else {
				neg_sum += tmp;
				errplus = (neg_sum + errplus > pos_sum)? neg_sum - pos_sum + errplus: 0.;
			}
				if (tmpmin > neg_sum - errminus)
					tmpmin = neg_sum - errminus;
				if (tmpmax < pos_sum + errplus)
					tmpmax = pos_sum + errplus;
		}
		if (GOG_1_5D_STACKED == model->type) {
			if (model->minima > tmpmin)
				model->minima = tmpmin;
			if (model->maxima < tmpmax)
				model->maxima = tmpmax;
		} else {
			if (model->minima > tmpmin / (pos_sum - neg_sum))
				model->minima = tmpmin / (pos_sum - neg_sum);
			if (model->maxima < tmpmax / (pos_sum - neg_sum))
				model->maxima = tmpmax / (pos_sum - neg_sum);
		}
	}
}

static GOData *
gog_barcol_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			    GogPlotBoundInfo *bounds)
{
	GogPlot1_5d *model = GOG_PLOT1_5D (plot);
	GogPlot1_5dClass *plot1_5d_klass = GOG_PLOT1_5D_CLASS (gog_barcol_parent_klass);
	GOData *data;

	data = (plot1_5d_klass->base.axis_get_bounds) (plot, axis, bounds);

	if (axis == gog_axis_get_atype (gog_plot1_5d_get_index_axis (model))) {
		bounds->val.minima -= .5;
		bounds->val.maxima += .5;
		bounds->logical.minima = -.5;
		bounds->center_on_ticks = FALSE;
	}

	return data;
}

static void
gog_barcol_plot_class_init (GogPlot1_5dClass *gog_plot_1_5d_klass)
{
	GObjectClass   *gobject_klass = (GObjectClass *) gog_plot_1_5d_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) gog_plot_1_5d_klass;
	GogPlotClass   *plot_klass = (GogPlotClass *) gog_plot_1_5d_klass;

	gog_barcol_parent_klass = g_type_class_peek_parent (gog_plot_1_5d_klass);
	gobject_klass->set_property = gog_barcol_plot_set_property;
	gobject_klass->get_property = gog_barcol_plot_get_property;

	g_object_class_install_property (gobject_klass, BARCOL_PROP_GAP_PERCENTAGE,
		g_param_spec_int ("gap-percentage",
			_("Gap percentage"),
			_("The padding around each group as a percentage of their width"),
			0, 500, 150,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BARCOL_PROP_OVERLAP_PERCENTAGE,
		g_param_spec_int ("overlap-percentage",
			_("Overlap percentage"),
			_("The distance between series as a percentage of their width"),
			-100, 100, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BARCOL_PROP_HORIZONTAL,
		g_param_spec_boolean ("horizontal",
			_("horizontal"),
			_("horizontal bars or vertical columns"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BARCOL_PROP_FILL_BEFORE_GRID,
		g_param_spec_boolean ("before-grid",
			_("Displayed under the grids"),
			_("Should the plot be displayed before the grids"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_klass->type_name	= gog_barcol_plot_type_name;
#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor	= gog_barcol_plot_populate_editor;
#endif
	gog_object_klass->view_type	= gog_barcol_view_get_type ();

	plot_klass->desc.series.style_fields	= GO_STYLE_OUTLINE | GO_STYLE_FILL;
	plot_klass->series_type = gog_barcol_series_get_type ();
	plot_klass->axis_get_bounds   		= gog_barcol_axis_get_bounds;
	plot_klass->get_percent			= _gog_plot1_5d_get_percent_value;

	gog_plot_1_5d_klass->swap_x_and_y = gog_barcol_swap_x_and_y;
	gog_plot_1_5d_klass->update_stacked_and_percentage =
		gog_barcol_update_stacked_and_percentage;
}

static void
gog_barcol_plot_init (GogBarColPlot *model)
{
	model->gap_percentage = 150;
	GOG_PLOT1_5D (model)->support_series_lines = TRUE;
	GOG_PLOT (model)->rendering_order = GOG_PLOT_RENDERING_BEFORE_AXIS;
}

GSF_DYNAMIC_CLASS (GogBarColPlot, gog_barcol_plot,
	gog_barcol_plot_class_init, gog_barcol_plot_init,
	GOG_PLOT1_5D_TYPE)

/*****************************************************************************/
typedef GogPlotView		GogBarColView;
typedef GogPlotViewClass	GogBarColViewClass;

/**
 * FIXME FIXME FIXME Wrong description
 * barcol_draw_rect :
 * @rend: #GogRenderer
 * @flip:
 * @base: #GogViewAllocation
 * @rect: #GogViewAllocation
 *
 * A utility routine to build a vpath in @rect.  @rect is assumed to be in
 * coordinates relative to @base with 0,0 as the upper left.  @flip transposes
 * @rect and rotates it to put the origin in the bottom left.  Play fast and
 * loose with coordinates to keep widths >= 1.  If we allow things to be less
 * the background bleeds through.
 **/
static void
barcol_draw_rect (GogRenderer *rend, gboolean flip,
		  GogAxisMap *x_map,
		  GogAxisMap *y_map,
		  GogViewAllocation const *rect)
{
	GOPath *path = go_path_new ();
	double x0, x1, y0, y1;

	if (flip) {
		x0 = gog_axis_map_to_view (x_map, rect->y);
		x1 = gog_axis_map_to_view (x_map, rect->y + rect->h);
		if (gog_axis_map_finite (y_map, rect->x))
			y0 = gog_axis_map_to_view (y_map, rect->x);
		else
			y0 = gog_axis_map_get_baseline (y_map);
		if (gog_axis_map_finite (y_map, rect->x + rect->w))
			y1 = gog_axis_map_to_view (y_map, rect->x + rect->w);
		else
			y1 = gog_axis_map_get_baseline (y_map);
	} else {
		if (gog_axis_map_finite (x_map, rect->x))
			x0 = gog_axis_map_to_view (x_map, rect->x);
		else
			x0 = gog_axis_map_get_baseline (x_map);
		if (gog_axis_map_finite (x_map, rect->x + rect->w))
			x1 = gog_axis_map_to_view (x_map, rect->x + rect->w);
		else
			x1 = gog_axis_map_get_baseline (x_map);
		y0 = gog_axis_map_to_view (y_map, rect->y);
		y1 = gog_axis_map_to_view (y_map, rect->y + rect->h);
	}

	go_path_move_to (path, x0, y0);
	go_path_line_to (path, x1, y0);
	go_path_line_to (path, x1, y1);
	go_path_line_to (path, x0, y1);
	go_path_close (path);
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);

	gog_renderer_draw_shape (rend, path);
	go_path_free (path);
}

typedef struct {
	double		plus;
	double 		minus;
	double		x;
	double		y;
} ErrorBarData;

typedef struct {
	double		 x;
	double 		 y;
	GogSeriesLabelElt const	*elt;
	GOAnchorType     anchor;
} LabelData;

static int
gog_barcol_view_get_data_at_point (GogPlotView *view, double x, double y, GogSeries **series)
{
	GogBarColPlot const *model = GOG_BARCOL_PLOT (view->base.model);
	GogPlot1_5d const *gog_1_5d_model = GOG_PLOT1_5D (model);
	GogSeries1_5d const *pseries;
	GogSeries const *base_series;
	GogChart *chart = GOG_CHART (view->base.model->parent);
	GogChartMap *chart_map;
	GogAxisMap *x_map, *y_map, *map;
	GogViewAllocation work;
	GogViewAllocation const *area;
	unsigned num_elements = gog_1_5d_model->num_elements;
	unsigned num_series = gog_1_5d_model->num_series;
	gboolean is_vertical = ! (model->horizontal);
	double **vals, sum, neg_base, pos_base, tmp;
	double col_step, group_step, offset, data_scale;
	unsigned i, j, jinit, jend, jstep;
	GogPlot1_5dType const type = gog_1_5d_model->type;
	GSList *ptr;
	unsigned *lengths;

	if (num_elements <= 0 || num_series <= 0)
		return -1;

	area = gog_chart_view_get_plot_area (view->base.parent);
	chart_map = gog_chart_map_new (chart, area,
				       GOG_PLOT (model)->axis[GOG_AXIS_X],
				       GOG_PLOT (model)->axis[GOG_AXIS_Y],
				       NULL, FALSE);
	if (!gog_chart_map_is_valid (chart_map)) {
		gog_chart_map_free (chart_map);
		return -1;
	}

	x_map = gog_chart_map_get_axis_map (chart_map, 0);
	y_map = gog_chart_map_get_axis_map (chart_map, 1);

	map = is_vertical ? y_map : x_map;

	vals = g_alloca (num_series * sizeof (double *));
	lengths = g_alloca (num_series * sizeof (unsigned));

	i = 0;
	for (ptr = gog_1_5d_model->base.series ; ptr != NULL ; ptr = ptr->next) {
		pseries = ptr->data;
		base_series = GOG_SERIES (pseries);
		if (!gog_series_is_valid (base_series))
			continue;
		vals[i] = go_data_get_values (base_series->values[1].data);
		lengths[i] = go_data_get_vector_size (base_series->values[1].data);
		i++;
	}

	/* work in coordinates drawing bars from the top */
	col_step = 1. - model->overlap_percentage / 100.;
	group_step = model->gap_percentage / 100.;
	work.h = 1.0 / (1. + ((num_series - 1.0) * col_step) + group_step);
	col_step *= work.h;
	offset = (col_step * (num_series - 1.0) + work.h) / 2.0;
	data_scale = 1.0;
	if (type == GOG_1_5D_NORMAL) {
		/* we need to start from last series because they may overlap */
		jinit = num_series - 1;
		jstep = jend = -1;
	} else {
		jinit = 0;
		jend = num_series;
		jstep = 1;
	}

	for (i = 0 ; i < num_elements ; i++) {
		if (type == GOG_1_5D_AS_PERCENTAGE) {
			sum = 0.;
			for (j = num_series ; j-- > 0 ; ) {
				if (i >= lengths[j])
					continue;
				tmp = vals[j][i];
				if (!gog_axis_map_finite (map, tmp))
					continue;
				if (tmp > 0.)
					sum += tmp;
				else
					sum -= tmp;
			}

			data_scale = (fabs (go_sub_epsilon (sum)) > 0.0) ? 1.0 / sum : 1.0;
		}

		pos_base = neg_base = 0.0;
		for (j = jinit; j != jend; j += jstep) {
			double x0, y0, x1, y1;
			gboolean valid;

			work.y = (double) j * col_step + (double) i - offset + 1.0;

			if (i >= lengths[j])
				continue;

			tmp = vals[j][i];
			valid = gog_axis_map_finite (map, tmp);
			if (!valid)
				tmp = 0;
			tmp *= data_scale;
			if (tmp >= 0.) {
				work.x = pos_base;
				work.w = tmp;
				if (GOG_1_5D_NORMAL != type)
					pos_base += tmp;
			} else {
				work.x = neg_base + tmp;
				work.w = -tmp;
				if (GOG_1_5D_NORMAL != type)
					neg_base += tmp;
			}

			if (is_vertical) {
				x0 = gog_axis_map_to_view (x_map, work.y);
				x1 = gog_axis_map_to_view (x_map, work.y + work.h);
				if (gog_axis_map_finite (y_map, work.x))
					y0 = gog_axis_map_to_view (y_map, work.x);
				else
					y0 = gog_axis_map_get_baseline (y_map);
				if (gog_axis_map_finite (y_map, work.x + work.w))
					y1 = gog_axis_map_to_view (y_map, work.x + work.w);
				else
					y1 = gog_axis_map_get_baseline (y_map);
			} else {
				if (gog_axis_map_finite (x_map, work.x))
					x0 = gog_axis_map_to_view (x_map, work.x);
				else
					x0 = gog_axis_map_get_baseline (x_map);
				if (gog_axis_map_finite (x_map, work.x + work.w))
					x1 = gog_axis_map_to_view (x_map, work.x + work.w);
				else
					x1 = gog_axis_map_get_baseline (x_map);
				y0 = gog_axis_map_to_view (y_map, work.y);
				y1 = gog_axis_map_to_view (y_map, work.y + work.h);
			}
			if (x0 > x1) {
				double tmp = x0;
				x0 = x1;
				x1 = tmp;
			}
			if (y0 > y1) {
				double tmp = y0;
				y0 = y1;
				y1 = tmp;
			}
			if (x >= x0 && x <= x1 && y >= y0 && y <= y1) {
				*series = GOG_SERIES (g_slist_nth_data (gog_1_5d_model->base.series, j));
				gog_chart_map_free (chart_map);
				return i;
			}
		}
	}

	gog_chart_map_free (chart_map);
	return -1;
}

static void
gog_barcol_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogBarColPlot const *model = GOG_BARCOL_PLOT (view->model);
	GogPlot1_5d const *gog_1_5d_model = GOG_PLOT1_5D (view->model);
	GogSeries1_5d const *series;
	GogSeries const *base_series;
	GogChart *chart = GOG_CHART (view->model->parent);
	GogChartMap *chart_map;
	GogViewAllocation work;
	GogViewAllocation const *area;
	GogRenderer *rend = view->renderer;
	GogAxisMap *x_map, *y_map, *map;
	gboolean is_vertical = ! (model->horizontal), valid, inverted;
	double **vals, sum, neg_base, pos_base, tmp;
	double x;
	double col_step, group_step, offset, data_scale;
	unsigned i, j;
	unsigned num_elements = gog_1_5d_model->num_elements;
	unsigned num_series = gog_1_5d_model->num_series;
	GogPlot1_5dType const type = gog_1_5d_model->type;
	GOStyle **styles;
	ErrorBarData **error_data;
	GogErrorBar **errors;
	GogSeriesLines **lines;
	GOPath **paths;
	GSList *ptr;
	unsigned *lengths;
	double plus, minus;
	GogObjectRole const *role = NULL, *lbl_role = NULL;
	GogSeriesElement *gse;
	GList const **overrides;
	GogSeriesLabels **labels;
	LabelData **label_pos;

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

	map = is_vertical ? y_map : x_map;
	inverted = gog_axis_is_inverted (is_vertical?
	                                 GOG_PLOT (model)->axis[GOG_AXIS_Y]:
		                         GOG_PLOT (model)->axis[GOG_AXIS_X]);

	vals = g_alloca (num_series * sizeof (double *));
	lengths = g_alloca (num_series * sizeof (unsigned));
	styles = g_alloca (num_series * sizeof (GOStyle *));
	errors = g_alloca (num_series * sizeof (GogErrorBar *));
	error_data = g_alloca (num_series * sizeof (ErrorBarData *));
	lines = g_alloca (num_series * sizeof (GogSeriesLines *));
	paths = g_alloca (num_series * sizeof (GOPath *));
	overrides = g_alloca (num_series * sizeof (GSList *));
	labels = g_alloca (num_series * sizeof (GogSeriesLabels *));
	label_pos = g_alloca (num_series * sizeof (gpointer));

	i = 0;
	for (ptr = gog_1_5d_model->base.series ; ptr != NULL && i < num_series ; ptr = ptr->next, i++) {
		series = ptr->data;
		base_series = GOG_SERIES (series);
		if (!gog_series_is_valid (base_series)) {
			lengths[i] = 0;
			errors[i] = NULL;
			lines[i] = NULL;
			labels[i] = NULL;
			continue;
		}
		vals[i] = go_data_get_values (base_series->values[1].data);
		lengths[i] = go_data_get_vector_size (base_series->values[1].data);
		styles[i] = GOG_STYLED_OBJECT (series)->style;
		errors[i] = series->errors;
		overrides[i] = gog_series_get_overrides (GOG_SERIES (series));
		if (gog_error_bar_is_visible (series->errors))
			error_data[i] = g_new (ErrorBarData, lengths[i]);
		else
			error_data[i] = NULL;
		if (series->has_series_lines && lengths[i] > 0) {
			if (!role)
				role = gog_object_find_role_by_name (
							GOG_OBJECT (series), "Series lines");
			lines[i] = GOG_SERIES_LINES (
					gog_object_get_child_by_role (GOG_OBJECT (series), role));
			paths[i] = go_path_new ();
		} else
			lines[i] = NULL;
		if (!lbl_role)
			lbl_role = gog_object_find_role_by_name (GOG_OBJECT (series), "Data labels");
		labels[i] = (GogSeriesLabels *) gog_object_get_child_by_role (GOG_OBJECT (series), lbl_role);
		if (labels[i]) {
			label_pos[i] = g_new (LabelData, lengths[i]);
			for (j = 0; j < lengths[i]; j++)
				label_pos[i][j].elt = gog_series_labels_vector_get_element (labels[i] , j);
		} else
			label_pos[i] = NULL;
	}
	if (ptr != NULL || i != num_series) {
		g_warning ("Wrong series number in bar/col plot");
		num_series = i;
	}

	/* work in coordinates drawing bars from the top */
	col_step = 1. - model->overlap_percentage / 100.;
	group_step = model->gap_percentage / 100.;
	work.h = 1.0 / (1. + ((num_series - 1.0) * col_step) + group_step);
	col_step *= work.h;
	offset = (col_step * (num_series - 1.0) + work.h) / 2.0;
	data_scale = 1.0;

	for (i = 0 ; i < num_elements ; i++) {
		if (type == GOG_1_5D_AS_PERCENTAGE) {
			sum = 0.;
			for (j = num_series ; j-- > 0 ; ) {
				if (i >= lengths[j])
					continue;
				tmp = vals[j][i];
				if (!gog_axis_map_finite (map, tmp))
					continue;
				if (tmp > 0.)
					sum += tmp;
				else
					sum -= tmp;
			}

			data_scale = (fabs (go_sub_epsilon (sum)) > 0.0) ? 1.0 / sum : 1.0;
		}

		pos_base = neg_base = 0.0;
		for (j = 0 ; j < num_series ; j++) {
			work.y = (double) j * col_step + (double) i - offset + 1.0;

			if (i >= lengths[j])
				continue;
			tmp = vals[j][i];
			valid = TRUE;
			if (!gog_axis_map_finite (map, tmp)) {
				tmp = 0;
				valid = FALSE;
			} else if (gog_error_bar_is_visible (errors[j])) {
				gog_error_bar_get_bounds (errors[j], i, &minus, &plus);
			}
			tmp *= data_scale;
			if (tmp >= 0.) {
				work.x = pos_base;
				work.w = tmp;
				if (GOG_1_5D_NORMAL != type)
					pos_base += tmp;
			} else {
				work.x = neg_base + tmp;
				work.w = -tmp;
				if (GOG_1_5D_NORMAL != type)
					neg_base += tmp;
			}

			gse = NULL;
			if ((overrides[j] != NULL) &&
				(GOG_SERIES_ELEMENT (overrides[j]->data)->index == i)) {
					gse = GOG_SERIES_ELEMENT (overrides[j]->data);
					overrides[j] = overrides[j]->next;
					gog_renderer_push_style (view->renderer,
						go_styled_object_get_style (
							GO_STYLED_OBJECT (gse)));
			} else
				gog_renderer_push_style (view->renderer, styles[j]);
			barcol_draw_rect (rend, is_vertical, x_map, y_map, &work);
			gog_renderer_pop_style (view->renderer);

			if (valid && gog_error_bar_is_visible (errors[j])) {
				x = tmp > 0 ? work.x + work.w: work.x;
				error_data[j][i].plus = plus * data_scale;
				error_data[j][i].minus = minus * data_scale;
				if (is_vertical) {
					error_data[j][i].x = work.y + work.h / 2.0;
					error_data[j][i].y = x;
				} else {
					error_data[j][i].x = x;
					error_data[j][i].y = work.y + work.h / 2.0;
				}
			}
			if (lines[j] != NULL) {
				x = tmp > 0 ? work.x + work.w: work.x;
				if (is_vertical) {
					if (i > 0) {
						go_path_line_to (paths[j],
								 gog_axis_map_to_view (x_map, work.y),
								 gog_axis_map_to_view (y_map, x));
					}
					go_path_move_to (paths[j],
							 gog_axis_map_to_view (x_map, work.y + work.h),
							 gog_axis_map_to_view (y_map, x));
				} else {
					if (i > 0) {
						go_path_line_to (paths[j],
								 gog_axis_map_to_view (x_map, x),
								 gog_axis_map_to_view (y_map, work.y));
					}
					go_path_move_to (paths[j],
							 gog_axis_map_to_view (x_map, x),
							 gog_axis_map_to_view (y_map, work.y + work.h));
				}
			}
			if (labels[j] != NULL) {
				unsigned offset;
				GogSeriesLabelsPos position;
				GogObject *point = label_pos[j][i].elt->point;
				if (point) {
					g_object_get (point, "offset", &offset, NULL);
					position = gog_data_label_get_position (GOG_DATA_LABEL (point));
				} else {
					g_object_get (labels[j], "offset", &offset, NULL);
					position = gog_series_labels_get_position (labels[j]);
				}
				offset *= gog_renderer_get_scale (view->renderer);
				switch (position) {
				default:
				case GOG_SERIES_LABELS_CENTERED:
					label_pos[j][i].anchor = GO_ANCHOR_CENTER;
					if (is_vertical) {
						label_pos[j][i].x =  gog_axis_map_to_view (x_map, work.y + work.h / 2.);
						label_pos[j][i].y =  gog_axis_map_to_view (y_map, work.x + work.w / 2.);
					} else {
						label_pos[j][i].y =  gog_axis_map_to_view (y_map, work.y + work.h / 2.);
						label_pos[j][i].x =  gog_axis_map_to_view (x_map, work.x + work.w / 2.);
					}
					break;
				case GOG_SERIES_LABELS_OUTSIDE:
					if (is_vertical) {
						label_pos[j][i].x =  gog_axis_map_to_view (x_map, work.y + work.h / 2.);
						label_pos[j][i].y =  gog_axis_map_to_view (y_map, work.x + work.w);
						if (inverted) {
							label_pos[j][i].anchor =  GO_ANCHOR_NORTH;
							label_pos[j][i].y += offset;
						} else {
							label_pos[j][i].anchor = GO_ANCHOR_SOUTH;
							label_pos[j][i].y -= offset;
						}
					} else {
						label_pos[j][i].y =  gog_axis_map_to_view (y_map, work.y + work.h / 2.);
						label_pos[j][i].x =  gog_axis_map_to_view (x_map, work.x + work.w);
						if (inverted) {
							label_pos[j][i].anchor = GO_ANCHOR_EAST;
							label_pos[j][i].x -= offset;
						} else {
							label_pos[j][i].anchor = GO_ANCHOR_WEST;
							label_pos[j][i].x += offset;
						}
					}
					break;
				case GOG_SERIES_LABELS_INSIDE:
					if (is_vertical) {
						label_pos[j][i].x =  gog_axis_map_to_view (x_map, work.y + work.h / 2.);
						label_pos[j][i].y =  gog_axis_map_to_view (y_map, work.x + work.w);
						if (inverted) {
							label_pos[j][i].anchor = GO_ANCHOR_SOUTH;
							label_pos[j][i].y -= offset;
						} else {
							label_pos[j][i].anchor =  GO_ANCHOR_NORTH;
							label_pos[j][i].y += offset;
						}
					} else {
						label_pos[j][i].y =  gog_axis_map_to_view (y_map, work.y + work.h / 2.);
						label_pos[j][i].x =  gog_axis_map_to_view (x_map, work.x + work.w);
						if (inverted) {
							label_pos[j][i].anchor = GO_ANCHOR_WEST;
							label_pos[j][i].x += offset;
						} else {
							label_pos[j][i].anchor = GO_ANCHOR_EAST;
							label_pos[j][i].x -= offset;
						}
					}
					break;
				case GOG_SERIES_LABELS_NEAR_ORIGIN:
					if (is_vertical) {
						label_pos[j][i].x =  gog_axis_map_to_view (x_map, work.y + work.h / 2.);
						label_pos[j][i].y =  gog_axis_map_to_view (y_map, work.x);
						if (inverted) {
							label_pos[j][i].anchor =  GO_ANCHOR_NORTH;
							label_pos[j][i].y += offset;
						} else {
							label_pos[j][i].anchor = GO_ANCHOR_SOUTH;
							label_pos[j][i].y -= offset;
						}
					} else {
						label_pos[j][i].y =  gog_axis_map_to_view (y_map, work.y + work.h / 2.);
						label_pos[j][i].x =  gog_axis_map_to_view (x_map, work.x);
						if (inverted) {
							label_pos[j][i].anchor = GO_ANCHOR_EAST;
							label_pos[j][i].x -= offset;
						} else {
							label_pos[j][i].anchor = GO_ANCHOR_WEST;
							label_pos[j][i].x += offset;
						}
					}
					break;
				}
			}
		}
	}
	/*Now draw error bars and clean*/
	for (i = 0; i < num_series; i++)
		if (gog_error_bar_is_visible (errors[i])) {
			for (j = 0; j < lengths[i]; j++)
				gog_error_bar_render (errors[i], view->renderer,
						      chart_map,
						      error_data[i][j].x , error_data[i][j].y,
						      error_data[i][j].minus, error_data[i][j].plus,
						      model->horizontal? GOG_ERROR_BAR_DIRECTION_HORIZONTAL: GOG_ERROR_BAR_DIRECTION_VERTICAL);
			g_free (error_data[i]);
		}

	/* Draw series lines if any */
	for (i = 0; i < num_series; i++)
		if (lines[i] != NULL) {
			gog_series_lines_stroke (lines[i], view->renderer, bbox, paths[i], FALSE);
			go_path_free (paths[i]);
		}

	/* Draw data labels if any */
	for (i = 0; i < num_series; i++)
		if (labels[i] != NULL) {
			GogViewAllocation alloc;
			gog_renderer_push_style (view->renderer, go_styled_object_get_style (GO_STYLED_OBJECT (labels[i])));
			for (j = 0; j < lengths[i]; j++) {
				alloc.x = label_pos[i][j].x;
				alloc.y = label_pos[i][j].y;
				gog_renderer_draw_data_label (view->renderer,
				                              label_pos[i][j].elt,
				                              &alloc,
				                              label_pos[i][j].anchor,
				                              styles[i]);
			}
			gog_renderer_pop_style (view->renderer);
		}

	gog_chart_map_free (chart_map);
}

static void
gog_barcol_view_class_init (GogViewClass *view_klass)
{
	GogPlotViewClass *pv_klass = (GogPlotViewClass *) view_klass;

	view_klass->render	  = gog_barcol_view_render;
	view_klass->clip	  = TRUE;
	pv_klass->get_data_at_point = gog_barcol_view_get_data_at_point;
}

GSF_DYNAMIC_CLASS (GogBarColView, gog_barcol_view,
	gog_barcol_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)
