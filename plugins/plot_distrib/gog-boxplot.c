/*
 * gog-boxplot.c
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-boxplot.h"
#include <goffice/app/go-plugin.h>
#include <goffice/graph/gog-series-impl.h>
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/utils/go-style.h>
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/data/go-data-simple.h>
#include <goffice/math/go-rangefunc.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-marker.h>
#include <goffice/utils/go-path.h>
#include <goffice/utils/go-persist.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <string.h>

struct _GogBoxPlot {
	GogPlot	base;

	unsigned  num_series;
	double min, max;
	int gap_percentage;
	gboolean vertical, outliers;
	char const **names;
	double radius_ratio;
};

static GogObjectClass *gog_box_plot_parent_klass;

static GType gog_box_plot_view_get_type (void);

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>

static void
cb_gap_changed (GtkAdjustment *adj, GObject *boxplot)
{
	g_object_set (boxplot, "gap-percentage", (int) gtk_adjustment_get_value (adj), NULL);
}

static void
cb_layout_changed (GtkComboBox *box, GObject *boxplot)
{
	g_object_set (boxplot, "vertical", gtk_combo_box_get_active (box), NULL);
}

static void
cb_outliers_changed (GtkToggleButton *btn, GObject *boxplot)
{
	GtkBuilder *gui = GTK_BUILDER (g_object_get_data (G_OBJECT (btn), "state"));
	GtkWidget *w;
	gboolean outliers = gtk_toggle_button_get_active (btn);
	if (outliers) {
		w = go_gtk_builder_get_widget (gui, "diameter-label");
		gtk_widget_show (w);
		w = go_gtk_builder_get_widget (gui, "diameter");
		gtk_widget_show (w);
		w = go_gtk_builder_get_widget (gui, "diam-pc-label");
		gtk_widget_show (w);
	} else {
		w = go_gtk_builder_get_widget (gui, "diameter-label");
		gtk_widget_hide (w);
		w = go_gtk_builder_get_widget (gui, "diameter");
		gtk_widget_hide (w);
		w = go_gtk_builder_get_widget (gui, "diam-pc-label");
		gtk_widget_hide (w);
	}
	g_object_set (boxplot, "outliers", outliers, NULL);
}

static void
cb_ratio_changed (GtkAdjustment *adj, GObject *boxplot)
{
	g_object_set (boxplot, "radius-ratio", gtk_adjustment_get_value (adj) / 200., NULL);
}

static void
display_before_grid_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "before-grid", gtk_toggle_button_get_active (btn), NULL);
}

static gpointer
gog_box_plot_pref (GogObject *obj,
		   GogDataAllocator *dalloc, GOCmdContext *cc)
{
	GogBoxPlot *boxplot = GOG_BOX_PLOT (obj);
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_distrib/gog-boxplot-prefs.ui",
				    GETTEXT_PACKAGE, cc);
	GtkWidget *w;

        if (gui == NULL)
                return NULL;

	w = go_gtk_builder_get_widget (gui, "gap_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), boxplot->gap_percentage);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_gap_changed), boxplot);

	w = go_gtk_builder_get_widget (gui, "layout");
	gtk_combo_box_set_active (GTK_COMBO_BOX (w), boxplot->vertical);
	g_signal_connect (w, "changed", G_CALLBACK (cb_layout_changed), boxplot);

	w = go_gtk_builder_get_widget (gui, "show-outliers");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), boxplot->outliers);
	g_object_set_data (G_OBJECT (w), "state", gui);
	g_signal_connect (w, "toggled", G_CALLBACK (cb_outliers_changed), boxplot);

	w = go_gtk_builder_get_widget (gui, "diameter");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), boxplot->radius_ratio * 200.);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_ratio_changed), boxplot);

	if (!boxplot->outliers) {
		gtk_widget_hide (w);
		w = go_gtk_builder_get_widget (gui, "diameter-label");
		gtk_widget_hide (w);
		w = go_gtk_builder_get_widget (gui, "diam-pc-label");
		gtk_widget_hide (w);
	}

	w = go_gtk_builder_get_widget (gui, "before-grid");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
			(GOG_PLOT (obj))->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (display_before_grid_cb), obj);

	w = go_gtk_builder_get_widget (gui, "gog-box-plot-prefs");
	g_object_set_data (G_OBJECT (w), "state", gui);
	g_signal_connect_swapped (G_OBJECT (w), "destroy", G_CALLBACK (g_object_unref), gui);

	return w;
}

static void
gog_box_plot_populate_editor (GogObject *item,
			      GOEditor *editor,
			      G_GNUC_UNUSED GogDataAllocator *dalloc,
			      GOCmdContext *cc)
{
	GtkWidget *w =  gog_box_plot_pref (item, dalloc, cc);
	go_editor_add_page (editor,w, _("Properties"));

	(GOG_OBJECT_CLASS(gog_box_plot_parent_klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

enum {
	BOX_PLOT_PROP_0,
	BOX_PLOT_PROP_GAP_PERCENTAGE,
	BOX_PLOT_PROP_VERTICAL,
	BOX_PLOT_PROP_OUTLIERS,
	BOX_PLOT_PROP_RADIUS_RATIO,
	BOX_PLOT_PROP_BEFORE_GRID
};

typedef struct {
	GogSeries base;
	int	 gap_percentage;
	double vals[5];
	double *svals; /* sorted data */
	int nb_valid;
} GogBoxPlotSeries;
typedef GogSeriesClass GogBoxPlotSeriesClass;

#define GOG_TYPE_BOX_PLOT_SERIES	(gog_box_plot_series_get_type ())
#define GOG_BOX_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_BOX_PLOT_SERIES, GogBoxPlotSeries))
#define GOG_IS_BOX_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_BOX_PLOT_SERIES))

GType gog_box_plot_series_get_type (void);

static char const *
gog_box_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name box-plot objects
	 * eg The 2nd box-plot in a chart will be called
	 * 	BoxPlot2 */
	return N_("Box-Plot");
}

static void
gog_box_plot_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GogBoxPlot *boxplot = GOG_BOX_PLOT (obj);

	switch (param_id) {
	case BOX_PLOT_PROP_GAP_PERCENTAGE:
		boxplot->gap_percentage = g_value_get_int (value);
		break;
	case BOX_PLOT_PROP_VERTICAL:
		boxplot->vertical = g_value_get_boolean (value);
		if (boxplot->base.axis[0])
			gog_axis_bound_changed (boxplot->base.axis[0], GOG_OBJECT (boxplot));
		if (boxplot->base.axis[1])
			gog_axis_bound_changed (boxplot->base.axis[1], GOG_OBJECT (boxplot));
		break;
	case BOX_PLOT_PROP_OUTLIERS:
		boxplot->outliers = g_value_get_boolean (value);
		break;
	case BOX_PLOT_PROP_RADIUS_RATIO:
		boxplot->radius_ratio = g_value_get_double (value);
		break;
	case BOX_PLOT_PROP_BEFORE_GRID:
		GOG_PLOT (obj)->rendering_order = (g_value_get_boolean (value))?
						GOG_PLOT_RENDERING_BEFORE_GRID:
						GOG_PLOT_RENDERING_LAST;
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
}

static void
gog_box_plot_get_property (GObject *obj, guint param_id,
			      GValue *value, GParamSpec *pspec)
{
	GogBoxPlot *boxplot = GOG_BOX_PLOT (obj);

	switch (param_id) {
	case BOX_PLOT_PROP_GAP_PERCENTAGE:
		g_value_set_int (value, boxplot->gap_percentage);
		break;
	case BOX_PLOT_PROP_VERTICAL:
		g_value_set_boolean (value, boxplot->vertical);
		break;
	case BOX_PLOT_PROP_OUTLIERS:
		g_value_set_boolean (value, boxplot->outliers);
		break;
	case BOX_PLOT_PROP_RADIUS_RATIO:
		g_value_set_double (value, boxplot->radius_ratio);
		break;
	case BOX_PLOT_PROP_BEFORE_GRID:
		g_value_set_boolean (value, GOG_PLOT (obj)->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		break;
	}
}

static void
gog_box_plot_update (GogObject *obj)
{
	GogBoxPlot *model = GOG_BOX_PLOT (obj);
	GogBoxPlotSeries *series;
	GSList *ptr;
	double min, max;
	unsigned num_series = 0;

	min =  DBL_MAX;
	max = -DBL_MAX;
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = GOG_BOX_PLOT_SERIES (ptr->data);
		if (!gog_series_is_valid (GOG_SERIES (series)) ||
			!go_data_get_vector_size (series->base.values[0].data))
			continue;
		num_series++;
		if (series->vals[0] < min)
			min = series->vals[0];
		if (series->vals[4] > max)
			max = series->vals[4];
	}
	if (min == DBL_MAX)
		min = 0.;
	if (max == -DBL_MAX)
		max = 1.;
	if (model->min != min || model->max != max) {
		model->min = min;
		model->max = max;
		gog_axis_bound_changed (model->base.axis[(model->vertical)? 1: 0],
								GOG_OBJECT (model));
	}
	if (model->num_series != num_series) {
		model->num_series = num_series;
		g_free (model->names);
		model->names = (num_series)? g_new0 (char const*, num_series): NULL;
	}
	/* always update series axis because a series name might have changed */
	gog_axis_bound_changed (model->base.axis[(model->vertical)? 0: 1],
							GOG_OBJECT (model));
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static GOData *
gog_box_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			      GogPlotBoundInfo *bounds)
{
	GogBoxPlot *model = GOG_BOX_PLOT (plot);

	if ((axis == GOG_AXIS_X && model->vertical) ||
			(axis == GOG_AXIS_Y && !model->vertical)) {
		GOData *s;
		GogSeries *series;
		GSList *ptr;
		unsigned n = 0;
		gboolean has_names = FALSE;
		if (model->names)
			for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
				series = GOG_SERIES (ptr->data);
				if (!gog_series_is_valid (GOG_SERIES (series)) ||
					!go_data_get_vector_size (series->values[0].data))
					continue;
				s = gog_series_get_name (series);
				if (s && n < model->num_series) {
					model->names[n] = go_data_get_scalar_string (s);
					has_names = TRUE;
				}
				n++;
			}
		bounds->val.minima = .5;
		bounds->val.maxima = model->num_series + .5;
		bounds->is_discrete = TRUE;
		bounds->center_on_ticks = FALSE;
		return has_names? GO_DATA (go_data_vector_str_new (model->names, n, g_free)): NULL;
	} else {
		bounds->val.minima = model->min;
		bounds->val.maxima = model->max;
		bounds->is_discrete = FALSE;
	}

	return NULL;
}

static void
gog_box_plot_child_name_changed (GogObject const *obj, GogObject const *child)
{
	if (GOG_IS_SERIES (child)) {
		GogBoxPlot *model = GOG_BOX_PLOT (obj);
		GogAxis *axis = model->base.axis[(model->vertical)? 0: 1];
		gog_axis_bound_changed (axis, GOG_OBJECT (obj));
		gog_object_emit_changed (GOG_OBJECT (axis), TRUE);
	}
}

static void
gog_box_plot_finalize (GObject *obj)
{
	GogBoxPlot *plot = GOG_BOX_PLOT (obj);
	if (plot && plot->names)
		g_free (plot->names);
	G_OBJECT_CLASS (gog_box_plot_parent_klass)->finalize (obj);
}

static void
gog_box_plot_class_init (GogPlotClass *gog_plot_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) gog_plot_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) gog_plot_klass;
	GogPlotClass   *plot_klass = (GogPlotClass *) gog_plot_klass;

	gog_box_plot_parent_klass = g_type_class_peek_parent (gog_plot_klass);

	gobject_klass->set_property = gog_box_plot_set_property;
	gobject_klass->get_property = gog_box_plot_get_property;
	gobject_klass->finalize = gog_box_plot_finalize;
	g_object_class_install_property (gobject_klass, BOX_PLOT_PROP_GAP_PERCENTAGE,
		g_param_spec_int ("gap-percentage",
			_("Gap percentage"),
			_("The padding around each group as a percentage of their width"),
			0, 500, 150,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX_PLOT_PROP_VERTICAL,
		g_param_spec_boolean ("vertical",
			_("Vertical"),
			_("Whether the box-plot should be vertical instead of horizontal"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX_PLOT_PROP_OUTLIERS,
		g_param_spec_boolean ("outliers",
			_("Outliers"),
			_("Whether outliers should be taken into account and displayed"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX_PLOT_PROP_RADIUS_RATIO,
		g_param_spec_double ("radius-ratio",
			_("Radius ratio"),
			_("The ratio between the radius of the circles representing outliers and the rectangle width"),
			0., 0.5, 0.125,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX_PLOT_PROP_BEFORE_GRID,
		g_param_spec_boolean ("before-grid",
			_("Displayed under the grids"),
			_("Should the plot be displayed before the grids"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_klass->type_name	= gog_box_plot_type_name;
	gog_object_klass->view_type	= gog_box_plot_view_get_type ();
	gog_object_klass->update	= gog_box_plot_update;
	gog_object_klass->child_name_changed	= gog_box_plot_child_name_changed;
#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor = gog_box_plot_populate_editor;
#endif

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Values"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
	}
	plot_klass->desc.num_series_max = G_MAXINT;
	plot_klass->series_type = gog_box_plot_series_get_type ();
	plot_klass->axis_set = GOG_AXIS_SET_XY;
	plot_klass->desc.series.style_fields	= GO_STYLE_LINE | GO_STYLE_OUTLINE | GO_STYLE_FILL;
	plot_klass->axis_get_bounds   		= gog_box_plot_axis_get_bounds;
}

static void
gog_box_plot_init (GogBoxPlot *model)
{
	model->gap_percentage = 150;
	model->radius_ratio = 0.125;
}

GSF_DYNAMIC_CLASS (GogBoxPlot, gog_box_plot,
	gog_box_plot_class_init, gog_box_plot_init,
	GOG_TYPE_PLOT)

/*****************************************************************************/
typedef GogPlotView		GogBoxPlotView;
typedef GogPlotViewClass	GogBoxPlotViewClass;

static void
gog_box_plot_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogBoxPlot const *model = GOG_BOX_PLOT (view->model);
	GogChart *chart = GOG_CHART (view->model->parent);
	GogChartMap *chart_map;
	GogViewAllocation const *area;
	GogAxisMap *map, *ser_map;
	GogBoxPlotSeries const *series;
	double hrect, hser, y, hbar;
	double min, qu1, med, qu3, max;
	GOPath *path;
	GSList *ptr;
	GOStyle *style;
	int num_ser = 1;
	gboolean show_low_whisker, show_high_whisker;

	area = gog_chart_view_get_plot_area (view->parent);
	chart_map = gog_chart_map_new (chart, area,
				       GOG_PLOT (model)->axis[GOG_AXIS_X],
				       GOG_PLOT (model)->axis[GOG_AXIS_Y],
				       NULL, FALSE);
	if (!gog_chart_map_is_valid (chart_map)) {
		gog_chart_map_free (chart_map);
		return;
	}

	if (model->vertical) {
		map = gog_chart_map_get_axis_map (chart_map, 1);
		ser_map = gog_chart_map_get_axis_map (chart_map, 0);
	} else {
		map = gog_chart_map_get_axis_map (chart_map, 0);
		ser_map = gog_chart_map_get_axis_map (chart_map, 1);
	}

	if (model->vertical) {
		hser = view->allocation.w / model->num_series;
		hrect = hser / (1. + model->gap_percentage / 100.);
	} else {
		hser = view->allocation.h / model->num_series;
		hrect = hser / (1. + model->gap_percentage / 100.);
	}
	hrect /= 2.;
	hbar = hrect / 2.;

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)) ||
			!go_data_get_vector_size (series->base.values[0].data))
			continue;
		style = go_style_dup (GOG_STYLED_OBJECT (series)->style);
		y = gog_axis_map_to_view (ser_map, num_ser);
		gog_renderer_push_style (view->renderer, style);
		if (model->outliers) {
			double l1, l2, m1, m2, d, r = 2. * hrect * model->radius_ratio;
			int i = 0;
			d = series->vals[3] - series->vals[1];
			l1 = series->vals[1] - d * 1.5;
			l2 = series->vals[1] - d * 3.;
			m1 = series->vals[3] + d * 1.5;
			m2 = series->vals[3] + d * 3.;
			while (i < series->nb_valid && series->svals[i] < l1) {
				/* display the outlier as a mark */
				d = gog_axis_map_to_view (map, series->svals[i]);
				if (model->vertical) {
					if (series->svals[i] < l2)
						gog_renderer_stroke_circle (view->renderer, y, d, r);
					else
						gog_renderer_draw_circle (view->renderer, y, d, r);
				} else {
					if (series->svals[i] < l2)
						gog_renderer_stroke_circle (view->renderer, d, y, r);
					else
						gog_renderer_draw_circle (view->renderer, d, y, r);
				}
				i++;
			}
			min = series->svals[i];
			i = series->nb_valid - 1;
			while (series->svals[i] > m1) {
				/* display the outlier as a mark */
				d = gog_axis_map_to_view (map, series->svals[i]);
				if (model->vertical) {
					if (series->svals[i] > m2)
						gog_renderer_stroke_circle (view->renderer, y, d, r);
					else
						gog_renderer_draw_circle (view->renderer, y, d, r);
				} else {
					if (series->svals[i] > m2)
						gog_renderer_stroke_circle (view->renderer, d, y, r);
					else
						gog_renderer_draw_circle (view->renderer, d, y, r);
				}
				i--;
			}
			max = series->svals[i];
		} else {
			min = series->vals[0];
			max = series->vals[4];
		}
		show_low_whisker = min < series->vals[1];
		show_high_whisker = max > series->vals[3];
		min = gog_axis_map_to_view (map, min);
		qu1 = gog_axis_map_to_view (map, series->vals[1]);
		med = gog_axis_map_to_view (map, series->vals[2]);
		qu3 = gog_axis_map_to_view (map, series->vals[3]);
		max = gog_axis_map_to_view (map, max);
		if (model->vertical) {
			go_path_move_to (path, y - hrect, qu1);
			go_path_line_to (path, y - hrect, qu3);
			go_path_line_to (path, y + hrect, qu3);
			go_path_line_to (path, y + hrect, qu1);
			go_path_close (path);
			gog_renderer_draw_shape (view->renderer, path);
			go_path_clear (path);
			/* whiskers are displayed only if they are not inside the quartile box
			 * which might be the case if outliers are taken into account, see #345 */
			if (show_low_whisker) {
				go_path_move_to (path, y + hbar, min);
				go_path_line_to (path, y - hbar, min);
				go_path_move_to (path, y, min);
				go_path_line_to (path, y, qu1);
			}
			if (show_high_whisker) {
				go_path_move_to (path, y + hbar, max);
				go_path_line_to (path, y - hbar, max);
				go_path_move_to (path, y, max);
				go_path_line_to (path, y, qu3);
			}
			go_path_move_to (path, y - hrect, med);
			go_path_line_to (path, y + hrect, med);
			gog_renderer_stroke_shape (view->renderer, path);
			go_path_clear (path);
		} else {
			go_path_move_to (path, qu1, y - hrect);
			go_path_line_to (path, qu3, y - hrect);
			go_path_line_to (path, qu3, y + hrect);
			go_path_line_to (path, qu1, y + hrect);
			go_path_close (path);
			gog_renderer_draw_shape (view->renderer, path);
			go_path_clear (path);
			if (show_low_whisker) {
				go_path_move_to (path, min, y + hbar);
				go_path_line_to (path, min, y - hbar);
				go_path_move_to (path, min, y);
				go_path_line_to (path, qu1, y);
			}
			if (show_high_whisker) {
				go_path_move_to (path, max, y + hbar);
				go_path_line_to (path, max, y - hbar);
				go_path_move_to (path, max, y);
				go_path_line_to (path, qu3, y);
			}
			go_path_move_to (path, med, y - hrect);
			go_path_line_to (path, med, y + hrect);
			gog_renderer_stroke_shape (view->renderer, path);
			go_path_clear (path);
		}
		gog_renderer_pop_style (view->renderer);
		g_object_unref (style);
		num_ser++;
	}
	go_path_free (path);
	gog_chart_map_free (chart_map);
}

static void
gog_box_plot_view_class_init (GogViewClass *view_klass)
{
	view_klass->render	  = gog_box_plot_view_render;
	view_klass->clip	  = TRUE;
}

GSF_DYNAMIC_CLASS (GogBoxPlotView, gog_box_plot_view,
	gog_box_plot_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)

/*****************************************************************************/

static GogObjectClass *gog_box_plot_series_parent_klass;

static void
gog_box_plot_series_update (GogObject *obj)
{
	double *vals = NULL;
	int len = 0;
	GogBoxPlotSeries *series = GOG_BOX_PLOT_SERIES (obj);
	unsigned old_num = series->base.num_elements;

	g_free (series->svals);
	series->svals = NULL;

	if (series->base.values[0].data != NULL) {
		vals = go_data_get_values (series->base.values[0].data);
		len = go_data_get_vector_size (series->base.values[0].data);
	}
	series->base.num_elements = len;
	if (len > 0) {
		double x;
		int n, max = 0;
		series->svals = g_new (double, len);
		for (n = 0; n < len; n++)
			if (go_finite (vals[n]))
				series->svals[max++] = vals[n];
		go_range_fractile_inter_nonconst (series->svals, max, &series->vals[0], 0);
		for (x = 0.25,n = 1; n < 5; n++, x+= 0.25)
			go_range_fractile_inter_sorted (series->svals, max, &series->vals[n], x);
		series->nb_valid = max;
	}
	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));
	if (old_num != series->base.num_elements)
		gog_plot_request_cardinality_update (series->base.plot);

	if (gog_box_plot_series_parent_klass->update)
		gog_box_plot_series_parent_klass->update (obj);
}

static void
gog_box_plot_series_finalize (GObject *obj)
{
	g_free (GOG_BOX_PLOT_SERIES (obj)->svals);
	((GObjectClass *) gog_box_plot_series_parent_klass)->finalize (obj);
}

static void
gog_box_plot_series_class_init (GogObjectClass *obj_klass)
{
	GObjectClass *object_class = (GObjectClass *) obj_klass;

	gog_box_plot_series_parent_klass = g_type_class_peek_parent (obj_klass);
	object_class->finalize = gog_box_plot_series_finalize;
	obj_klass->update = gog_box_plot_series_update;
}

GSF_DYNAMIC_CLASS (GogBoxPlotSeries, gog_box_plot_series,
	gog_box_plot_series_class_init, NULL,
	GOG_TYPE_SERIES)
