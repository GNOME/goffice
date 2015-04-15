/*
 * gog-minmax.c
 *
 * Copyright (C) 2005
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
#include "gog-minmax.h"
#include <goffice/graph/gog-series-lines.h>
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/utils/go-marker.h>
#include <goffice/utils/go-path.h>
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-styled-object.h>
#include <goffice/app/go-plugin.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#endif

enum {
	MINMAX_PROP_0,
	MINMAX_PROP_GAP_PERCENTAGE,
	MINMAX_PROP_HORIZONTAL,
	MINMAX_PROP_DEFAULT_STYLE_HAS_MARKERS
};

static GogObjectClass *gog_minmax_parent_klass;

static GType gog_minmax_view_get_type (void);

typedef GogSeries1_5d		GogMinMaxSeries;
typedef GogSeries1_5dClass	GogMinMaxSeriesClass;

static GogStyledObjectClass *series_parent_klass;

static void
gog_minmax_series_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogSeries *series = GOG_SERIES (gso);
	GogMinMaxPlot const *plot;

	series_parent_klass->init_style (gso, style);
	if (series->plot == NULL)
		return;

	plot = GOG_MINMAX_PLOT (series->plot);
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
gog_minmax_series_class_init (GogStyledObjectClass *gso_klass)
{
	series_parent_klass = g_type_class_peek_parent (gso_klass);
	gso_klass->init_style = gog_minmax_series_init_style;
}

GSF_DYNAMIC_CLASS (GogMinMaxSeries, gog_minmax_series,
	gog_minmax_series_class_init, NULL,
	GOG_SERIES1_5D_TYPE)

/*****************************************************************************/

static void
gog_minmax_plot_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GogMinMaxPlot *minmax = GOG_MINMAX_PLOT (obj);

	switch (param_id) {
	case MINMAX_PROP_GAP_PERCENTAGE:
		minmax->gap_percentage = g_value_get_int (value);
		break;

	case MINMAX_PROP_HORIZONTAL:
		minmax->horizontal = g_value_get_boolean (value);
		break;

	case MINMAX_PROP_DEFAULT_STYLE_HAS_MARKERS:
		minmax->default_style_has_markers = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
}

static void
gog_minmax_plot_get_property (GObject *obj, guint param_id,
			      GValue *value, GParamSpec *pspec)
{
	GogMinMaxPlot *minmax = GOG_MINMAX_PLOT (obj);

	switch (param_id) {
	case MINMAX_PROP_GAP_PERCENTAGE:
		g_value_set_int (value, minmax->gap_percentage);
		break;
	case MINMAX_PROP_HORIZONTAL:
		g_value_set_boolean (value, minmax->horizontal);
		break;
	case MINMAX_PROP_DEFAULT_STYLE_HAS_MARKERS:
		g_value_set_boolean (value, minmax->default_style_has_markers);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static char const *
gog_minmax_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name min/max line plot objects
	 * eg The 2nd min/max line plot in a chart will be called
	 * 	PlotMinMax2 */
	return N_("PlotMinMax");
}

static GOData *
gog_minmax_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			    GogPlotBoundInfo *bounds)
{
	GogPlot1_5d *model = GOG_PLOT1_5D (plot);
	GogPlot1_5dClass *plot1_5d_klass = GOG_PLOT1_5D_CLASS (gog_minmax_parent_klass);
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

#ifdef GOFFICE_WITH_GTK
static void
cb_gap_changed (GtkAdjustment *adj, GObject *minmax)
{
	g_object_set (minmax, "gap-percentage", (int) gtk_adjustment_get_value (adj), NULL);
}

static void
gog_minmax_plot_populate_editor (GogObject *item,
				 GOEditor *editor,
				 G_GNUC_UNUSED GogDataAllocator *dalloc,
				 GOCmdContext *cc)
{
	GogMinMaxPlot *minmax = GOG_MINMAX_PLOT (item);
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_barcol/gog-minmax-prefs.ui",
				    GETTEXT_PACKAGE, cc);
	GtkWidget  *w;

	if (gui == NULL)
		return;

	w = go_gtk_builder_get_widget (gui, "gap_spinner");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), minmax->gap_percentage);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_gap_changed), minmax);

	w = go_gtk_builder_get_widget (gui, "gog-minmax-prefs");
	go_editor_add_page (editor, w, _("Properties"));
	g_object_unref (gui);

	(GOG_OBJECT_CLASS(gog_minmax_parent_klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

static gboolean
gog_minmax_swap_x_and_y (GogPlot1_5d *model)
{
	return GOG_MINMAX_PLOT (model)->horizontal;
}

static void
gog_minmax_plot_class_init (GogPlot1_5dClass *gog_plot_1_5d_klass)
{
	GObjectClass   *gobject_klass = (GObjectClass *) gog_plot_1_5d_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) gog_plot_1_5d_klass;
	GogPlotClass   *plot_klass = (GogPlotClass *) gog_plot_1_5d_klass;
	gog_minmax_parent_klass = g_type_class_peek_parent (gog_plot_1_5d_klass);

	gobject_klass->set_property = gog_minmax_plot_set_property;
	gobject_klass->get_property = gog_minmax_plot_get_property;

	g_object_class_install_property (gobject_klass, MINMAX_PROP_GAP_PERCENTAGE,
		g_param_spec_int ("gap-percentage",
			_("Gap percentage"),
			_("The padding around each group as a percentage of their width"),
			0, 500, 150,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, MINMAX_PROP_HORIZONTAL,
		g_param_spec_boolean ("horizontal",
			_("Horizontal"),
			_("Horizontal or vertical lines"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, MINMAX_PROP_DEFAULT_STYLE_HAS_MARKERS,
		g_param_spec_boolean ("default-style-has-markers",
			_("Default markers"),
			_("Should the default style of a series include markers"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_klass->type_name	= gog_minmax_plot_type_name;
	gog_object_klass->view_type	= gog_minmax_view_get_type ();
#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor	= gog_minmax_plot_populate_editor;
#endif

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Labels"), GOG_SERIES_SUGGESTED, TRUE,
			  GOG_DIM_LABEL, GOG_MS_DIM_CATEGORIES },
			{ N_("Min"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_LOW },
			{ N_("Max"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_HIGH },
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
	}
	plot_klass->desc.series.style_fields = GO_STYLE_LINE | GO_STYLE_MARKER;
	plot_klass->axis_get_bounds   		= gog_minmax_axis_get_bounds;
	plot_klass->series_type = gog_minmax_series_get_type ();

	gog_plot_1_5d_klass->swap_x_and_y = gog_minmax_swap_x_and_y;
	gog_plot_1_5d_klass->update_stacked_and_percentage = NULL;
}

static void
gog_minmax_plot_init (GogMinMaxPlot *minmax)
{
	minmax->default_style_has_markers = FALSE;
	minmax->gap_percentage = 150;
	GOG_PLOT1_5D (minmax)->support_lines = TRUE;
}

GSF_DYNAMIC_CLASS (GogMinMaxPlot, gog_minmax_plot,
	gog_minmax_plot_class_init, gog_minmax_plot_init,
	GOG_PLOT1_5D_TYPE)

/*****************************************************************************/
typedef GogPlotView		GogMinMaxView;
typedef GogPlotViewClass	GogMinMaxViewClass;


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
gog_minmax_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogMinMaxPlot const *model = GOG_MINMAX_PLOT (view->model);
	GogPlot1_5d const *gog_1_5d_model = GOG_PLOT1_5D (view->model);
	GogSeries1_5d const *series;
	GogSeries const *base_series;
	GogAxisMap *x_map, *y_map;
	gboolean is_vertical = ! (model->horizontal);
	double *max_vals, *min_vals;
	double x, xmapped, minmapped, maxmapped;
	double step, offset;
	unsigned i, j;
	unsigned num_elements = gog_1_5d_model->num_elements;
	unsigned num_series = gog_1_5d_model->num_series;
	GSList *ptr;
	unsigned n, tmp;
	GOPath *path, *Mpath, *mpath;
	GogObjectRole const *role = NULL;
	GogSeriesLines *lines;
	GOStyle * style;
	gboolean prec_valid;

	if (num_elements <= 0 || num_series <= 0)
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

	step = 1. / (num_series + model->gap_percentage / 100.);
	offset = - step * (num_series - 1) / 2.;

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);

	for (ptr = gog_1_5d_model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		base_series = GOG_SERIES (series);
		if (!gog_series_is_valid (base_series))
			continue;
		style = go_styled_object_get_style (GO_STYLED_OBJECT (series));
		x = offset;
		min_vals = go_data_get_values (base_series->values[1].data);
		n = go_data_get_vector_size (base_series->values[1].data);
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
			x++;
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
		if (series->has_lines) {
			if (!role)
				role = gog_object_find_role_by_name (
							GOG_OBJECT (series), "Lines");
			lines = GOG_SERIES_LINES (
					gog_object_get_child_by_role (GOG_OBJECT (series), role));
			gog_renderer_push_style (view->renderer,
				go_styled_object_get_style (GO_STYLED_OBJECT (lines)));
			gog_series_lines_stroke (lines, view->renderer, bbox, mpath, TRUE);
			gog_series_lines_stroke (lines, view->renderer, bbox, Mpath, FALSE);
			gog_renderer_pop_style (view->renderer);
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
		offset += step;
	}

	go_path_free (path);
	gog_axis_map_free (x_map);
	gog_axis_map_free (y_map);
}

static void
gog_minmax_view_class_init (GogViewClass *view_klass)
{
	view_klass->render	  = gog_minmax_view_render;
	view_klass->clip	  = TRUE;
}

GSF_DYNAMIC_CLASS (GogMinMaxView, gog_minmax_view,
	gog_minmax_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)
