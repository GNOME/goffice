/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-boxplot.c
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
#include "gog-boxplot.h"
#include <goffice/graph/gog-series-impl.h>
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-style.h>
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/data/go-data-simple.h>
#include <goffice/math/go-rangefunc.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-marker.h>
#include <goffice/app/module-plugin-defs.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <string.h>

GOFFICE_PLUGIN_MODULE_HEADER;

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
#include <glade/glade-xml.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtkspinbutton.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkenums.h>

static void
cb_gap_changed (GtkAdjustment *adj, GObject *boxplot)
{
	g_object_set (boxplot, "gap-percentage", (int)adj->value, NULL);
}

static void
cb_layout_changed (GtkComboBox *box, GObject *boxplot)
{
	g_object_set (boxplot, "vertical", gtk_combo_box_get_active (box), NULL);
}

static void
cb_outliers_changed (GtkToggleButton *btn, GObject *boxplot)
{
	GladeXML *gui = GLADE_XML (g_object_get_data (G_OBJECT (btn), "state"));
	GtkWidget *w;
	gboolean outliers = gtk_toggle_button_get_active (btn);
	if (outliers) {
		w = glade_xml_get_widget (gui, "diameter-label");
		gtk_widget_show (w);
		w = glade_xml_get_widget (gui, "diameter");
		gtk_widget_show (w);
		w = glade_xml_get_widget (gui, "diam-pc-label");
		gtk_widget_show (w);
	} else {
		w = glade_xml_get_widget (gui, "diameter-label");
		gtk_widget_hide (w);
		w = glade_xml_get_widget (gui, "diameter");
		gtk_widget_hide (w);
		w = glade_xml_get_widget (gui, "diam-pc-label");
		gtk_widget_hide (w);
	}
	g_object_set (boxplot, "outliers", outliers, NULL);
}

static void
cb_ratio_changed (GtkAdjustment *adj, GObject *boxplot)
{
	g_object_set (boxplot, "radius-ratio", adj->value / 200., NULL);
}

static gpointer
gog_box_plot_pref (GogObject *obj,
		   GogDataAllocator *dalloc, GOCmdContext *cc)
{
	GtkWidget  *w;
	GogBoxPlot *boxplot = GOG_BOX_PLOT (obj);
	char const *dir = go_plugin_get_dir_name (
		go_plugins_get_plugin_by_id ("GOffice_plot_boxes"));
	char	 *path = g_build_filename (dir, "gog-boxplot-prefs.glade", NULL);
	GladeXML *gui = go_libglade_new (path, "gog_box_plot_prefs", GETTEXT_PACKAGE, cc);

	g_free (path);
        if (gui == NULL)
                return NULL;

	w = glade_xml_get_widget (gui, "gap_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), boxplot->gap_percentage);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_gap_changed), boxplot);

	w = glade_xml_get_widget (gui, "layout");
	gtk_combo_box_set_active (GTK_COMBO_BOX (w), boxplot->vertical);
	g_signal_connect (w, "changed", G_CALLBACK (cb_layout_changed), boxplot);

	w = glade_xml_get_widget (gui, "show-outliers");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), boxplot->outliers);
	g_object_set_data (G_OBJECT (w), "state", gui);
	g_signal_connect (w, "toggled", G_CALLBACK (cb_outliers_changed), boxplot);

	w = glade_xml_get_widget (gui, "diameter");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), boxplot->radius_ratio * 200.);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_ratio_changed), boxplot);

	if (!boxplot->outliers) {
		gtk_widget_hide (w);
		w = glade_xml_get_widget (gui, "diameter-label");
		gtk_widget_hide (w);
		w = glade_xml_get_widget (gui, "diam-pc-label");
		gtk_widget_hide (w);
	}

	w = glade_xml_get_widget (gui, "gog_box_plot_prefs");
	g_object_set_data_full (G_OBJECT (w),
		"state", gui, (GDestroyNotify)g_object_unref);

	return w;
}

static void
gog_box_plot_populate_editor (GogObject *item,
			      GogEditor *editor,
			      G_GNUC_UNUSED GogDataAllocator *dalloc,
			      GOCmdContext *cc)
{
	gog_editor_add_page (editor, gog_box_plot_pref (item, dalloc, cc), _("Properties"));
	
	(GOG_OBJECT_CLASS(gog_box_plot_parent_klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

enum {
	BOX_PLOT_PROP_0,
	BOX_PLOT_PROP_GAP_PERCENTAGE,
	BOX_PLOT_PROP_VERTICAL,
	BOX_PLOT_PROP_OUTLIERS,
	BOX_PLOT_PROP_RADIUS_RATIO,
};

typedef struct {
	GogSeries base;
	int	 gap_percentage;
	double vals[5];
	double *svals; //sorted data
	int nb_valid;
} GogBoxPlotSeries;
typedef GogSeriesClass GogBoxPlotSeriesClass;

#define GOG_BOX_PLOT_SERIES_TYPE	(gog_box_plot_series_get_type ())
#define GOG_BOX_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_BOX_PLOT_SERIES_TYPE, GogBoxPlotSeries))
#define IS_GOG_BOX_PLOT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_BOX_PLOT_SERIES_TYPE))

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
			!go_data_vector_get_len (GO_DATA_VECTOR (series->base.values[0].data)))
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
		GODataScalar *s;
		GogSeries *series;
		GSList *ptr;
		int n = 0;
		gboolean has_names = FALSE;
		if (model->names)
			for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
				series = GOG_SERIES (ptr->data);
				if (!gog_series_is_valid (GOG_SERIES (series)) ||
					!go_data_vector_get_len (GO_DATA_VECTOR (series->values[0].data)))
					continue;
				s = gog_series_get_name (series);
				if (s) {
					model->names[n] = go_data_scalar_get_str (s);
					has_names = TRUE;
				}
				n++;
			}
		bounds->val.minima = .5;
		bounds->val.maxima = model->num_series + .5;
		bounds->is_discrete = TRUE;
		bounds->center_on_ticks = FALSE;
		return has_names? go_data_vector_str_new (model->names, n, NULL): NULL;
		
	} else {
		bounds->val.minima = model->min;
		bounds->val.maxima = model->max;
		bounds->is_discrete = FALSE;
	}

	return NULL;
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
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX_PLOT_PROP_VERTICAL,
		g_param_spec_boolean ("vertical", 
			_("Vertical"),
			_("Whether the box-plot should be vertical instead of horizontal"),
			FALSE, 
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX_PLOT_PROP_OUTLIERS,
		g_param_spec_boolean ("outliers", 
			_("Outliers"),
			_("Whether outliers should be taken into account and displayed"),
			FALSE, 
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX_PLOT_PROP_RADIUS_RATIO,
		g_param_spec_double ("radius-ratio", 
			_("Radius ratio"),
			_("The ratio between the radius of the circles representing outliers and the rectangle width"),
			0., 0.5, 0.125, 
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));

	gog_object_klass->type_name	= gog_box_plot_type_name;
	gog_object_klass->view_type	= gog_box_plot_view_get_type ();
	gog_object_klass->update	= gog_box_plot_update;
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
	plot_klass->desc.num_series_min = 1;
	plot_klass->desc.num_series_max = G_MAXINT;
	plot_klass->series_type = gog_box_plot_series_get_type ();
	plot_klass->axis_set = GOG_AXIS_SET_XY;
	plot_klass->desc.series.style_fields	= GOG_STYLE_LINE | GOG_STYLE_FILL;
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
	GOG_PLOT_TYPE)

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
	ArtVpath	path[6];
	GSList *ptr;
	GogStyle *style;
	int num_ser = 1;

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
	path[0].code = ART_MOVETO;
	path[1].code = ART_LINETO;
	path[3].code = ART_LINETO;
	path[4].code = ART_LINETO;
	path[5].code = ART_END;
		
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)) ||
			!go_data_vector_get_len (GO_DATA_VECTOR (series->base.values[0].data)))
			continue;
		style = gog_style_dup (GOG_STYLED_OBJECT (series)->style);
		y = gog_axis_map_to_view (ser_map, num_ser);
		gog_renderer_push_style (view->renderer, style);
		if (model->outliers) {
			double l1, l2, m1, m2, d, r = 2. * hrect * model->radius_ratio;
			int i = 0;
		    	style->outline = style->line;
			d = series->vals[3] - series->vals[1];
			l1 = series->vals[1] - d * 1.5;
			l2 = series->vals[1] - d * 3.;
			m1 = series->vals[3] + d * 1.5;
			m2 = series->vals[3] + d * 3.;
			while (series->svals[i] < l1) {
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
		min = gog_axis_map_to_view (map, min);
		qu1 = gog_axis_map_to_view (map, series->vals[1]);
		med = gog_axis_map_to_view (map, series->vals[2]);
		qu3 = gog_axis_map_to_view (map, series->vals[3]);
		max = gog_axis_map_to_view (map, max);
		if (model->vertical) {
			path[2].code = ART_LINETO;
			path[0].y = path[3].y = path[4].y = qu1;
			path[1].y = path[2].y = qu3;
			path[0].x = path[1].x = path[4].x = y - hrect;
			path[2].x = path[3].x = y + hrect;
			gog_renderer_draw_sharp_polygon (view->renderer, path, TRUE);
			path[2].code = ART_END;
			path[0].x = y + hbar;
			path[1].x = y - hbar;
			path[0].y = path[1].y = min;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[0].y = path[1].y = max;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[0].x = path[1].x = y;
			path[0].y = qu3;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[0].y = min;
			path[1].y = qu1;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[0].y = path[1].y = med;
			path[0].x = y + hrect;
			path[1].x = y - hrect;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[2].code = ART_LINETO;
			path[0].y = path[3].y = path[4].y = qu1;
			path[1].y = path[2].y = qu3;
			path[0].x = path[1].x = path[4].x = y - hrect;
			path[2].x = path[3].x = y + hrect;
		} else {
			path[2].code = ART_LINETO;
			path[0].x = path[3].x = path[4].x = qu1;
			path[1].x = path[2].x = qu3;
			path[0].y = path[1].y = path[4].y = y - hrect;
			path[2].y = path[3].y = y + hrect;
			gog_renderer_draw_sharp_polygon (view->renderer, path, TRUE);
			path[2].code = ART_END;
			path[0].y = y + hbar;
			path[1].y = y - hbar;
			path[0].x = path[1].x = min;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[0].x = path[1].x = max;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[0].y = path[1].y = y;
			path[0].x = qu3;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[0].x = min;
			path[1].x = qu1;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[0].x = path[1].x = med;
			path[0].y = y + hrect;
			path[1].y = y - hrect;
			gog_renderer_draw_sharp_path (view->renderer, path);
			path[2].code = ART_LINETO;
			path[0].x = path[3].x = path[4].x = qu1;
			path[1].x = path[2].x = qu3;
			path[0].y = path[1].y = path[4].y = y - hrect;
			path[2].y = path[3].y = y + hrect;
		}
		gog_renderer_draw_sharp_path (view->renderer, path);
		gog_renderer_pop_style (view->renderer);
		g_object_unref (style);
		num_ser++;
	}
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
	GOG_PLOT_VIEW_TYPE)

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
		vals = go_data_vector_get_values (GO_DATA_VECTOR (series->base.values[0].data));
		len = go_data_vector_get_len 
			(GO_DATA_VECTOR (series->base.values[0].data));
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
gog_box_plot_series_init_style (GogStyledObject *gso, GogStyle *style)
{
	((GogStyledObjectClass*) gog_box_plot_series_parent_klass)->init_style (gso, style);

	style->outline.dash_type = GO_LINE_NONE;
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
	GogStyledObjectClass *gso_klass = (GogStyledObjectClass*) obj_klass;

	gog_box_plot_series_parent_klass = g_type_class_peek_parent (obj_klass);
	object_class->finalize = gog_box_plot_series_finalize;
	obj_klass->update = gog_box_plot_series_update;
	gso_klass->init_style = gog_box_plot_series_init_style;
}

GSF_DYNAMIC_CLASS (GogBoxPlotSeries, gog_box_plot_series,
	gog_box_plot_series_class_init, NULL,
	GOG_SERIES_TYPE)

/* Plugin initialization */
void  gog_histogram_plot_register_type (GTypeModule *module);
void  gog_histogram_plot_series_register_type (GTypeModule *module);
void  gog_histogram_plot_view_register_type (GTypeModule *module);
void  gog_histogram_series_view_register_type (GTypeModule *module);

G_MODULE_EXPORT void
go_plugin_init (GOPlugin *plugin, GOCmdContext *cc)
{
	GTypeModule *module = go_plugin_get_type_module (plugin);
	gog_box_plot_register_type (module);
	gog_box_plot_view_register_type (module);
	gog_box_plot_series_register_type (module);
	gog_histogram_plot_register_type (module);
	gog_histogram_plot_view_register_type (module);
	gog_histogram_plot_series_register_type (module);
	gog_histogram_series_view_register_type (module);
}

G_MODULE_EXPORT void
go_plugin_shutdown (GOPlugin *plugin, GOCmdContext *cc)
{
}