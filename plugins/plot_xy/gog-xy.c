/*
 * go-xy.c
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
#include "gog-xy.h"
#include "gog-xy-dropbar.h"
#include "gog-xy-minmax.h"
#include <goffice/app/module-plugin-defs.h>

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#include <gtk/gtk.h>
#endif

#include "embedded-stuff.c"

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <math.h>
#include <string.h>

typedef struct {
	GogPlotClass	base;

	void (*adjust_bounds) (Gog2DPlot *model, double *x_min, double *x_max, double *y_min, double *y_max);
} Gog2DPlotClass;

typedef Gog2DPlotClass GogXYPlotClass;

typedef Gog2DPlotClass GogBubblePlotClass;

typedef Gog2DPlotClass GogXYColorPlotClass;

GOFFICE_PLUGIN_MODULE_HEADER;

static GogObjectClass *plot2d_parent_klass;
static void gog_2d_plot_adjust_bounds (Gog2DPlot *model, double *x_min, double *x_max, double *y_min, double *y_max);

#define GOG_2D_PLOT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_2D_PLOT_TYPE, Gog2DPlotClass))

static void
gog_2d_plot_clear_formats (Gog2DPlot *plot2d)
{
	go_format_unref (plot2d->x.fmt);
	plot2d->x.fmt = NULL;

	go_format_unref (plot2d->y.fmt);
	plot2d->y.fmt = NULL;
}

static void
gog_2d_plot_update (GogObject *obj)
{
	Gog2DPlot *model = GOG_2D_PLOT (obj);
	GogXYSeries const *series = NULL;
	double x_min, x_max, y_min, y_max, tmp_min, tmp_max;
	GSList *ptr;
	GogAxis *x_axis = gog_plot_get_axis (GOG_PLOT (model), GOG_AXIS_X);
	GogAxis *y_axis = gog_plot_get_axis (GOG_PLOT (model), GOG_AXIS_Y);

	x_min = y_min =  DBL_MAX;
	x_max = y_max = -DBL_MAX;
	gog_2d_plot_clear_formats (model);
	for (ptr = model->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;

		gog_axis_data_get_bounds (y_axis, series->base.values[1].data, &tmp_min, &tmp_max);
		if (y_min > tmp_min) y_min = tmp_min;
		if (y_max < tmp_max) y_max = tmp_max;
		if (model->y.fmt == NULL)
			model->y.fmt = go_data_preferred_fmt (series->base.values[1].data);
		model->y.date_conv = go_data_date_conv (series->base.values[1].data);

		if (series->base.values[0].data != NULL) {
			gog_axis_data_get_bounds (x_axis, series->base.values[0].data, &tmp_min, &tmp_max);

			if (!go_finite (tmp_min) || !go_finite (tmp_max) ||
			    tmp_min > tmp_max) {
				tmp_min = 0;
				tmp_max = go_data_get_vector_size (series->base.values[1].data);

			} else if (model->x.fmt == NULL)
				model->x.fmt = go_data_preferred_fmt (series->base.values[0].data);

			model->x.date_conv = go_data_date_conv (series->base.values[0].data);
		} else {
			tmp_min = 0;
			tmp_max = go_data_get_vector_size (series->base.values[1].data);
		}

		if (x_min > tmp_min) x_min = tmp_min;
		if (x_max < tmp_max) x_max = tmp_max;
		/* add room for error bars */
		if (gog_error_bar_is_visible (series->x_errors)) {
			gog_error_bar_get_minmax (series->x_errors, &tmp_min, &tmp_max);
			if (x_min > tmp_min)
				x_min = tmp_min;
			if (x_max < tmp_max)
				x_max = tmp_max;
		}
		if (gog_error_bar_is_visible (series->y_errors)) {
			gog_error_bar_get_minmax (series->y_errors, &tmp_min, &tmp_max);
			if (y_min > tmp_min)
				y_min = tmp_min;
			if (y_max < tmp_max)
				y_max = tmp_max;
		}
	}

	/*adjust bounds to allow large markers or bubbles*/
	gog_2d_plot_adjust_bounds (model, &x_min, &x_max, &y_min, &y_max);

	if (model->x.minima != x_min || model->x.maxima != x_max) {
		model->x.minima = x_min;
		model->x.maxima = x_max;
		gog_axis_bound_changed (model->base.axis[0], GOG_OBJECT (model));
	}
	if (model->y.minima != y_min || model->y.maxima != y_max) {
		model->y.minima = y_min;
		model->y.maxima = y_max;
		gog_axis_bound_changed (model->base.axis[1], GOG_OBJECT (model));
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
	if (plot2d_parent_klass->update)
		plot2d_parent_klass->update (obj);
}

static void
gog_2d_plot_real_adjust_bounds (Gog2DPlot *model, double *x_min, double *x_max, double *y_min, double *y_max)
{
}

static void
gog_2d_plot_adjust_bounds (Gog2DPlot *model, double *x_min, double *x_max, double *y_min, double *y_max)
{
	Gog2DPlotClass *klass = GOG_2D_PLOT_GET_CLASS (model);
	klass->adjust_bounds (model, x_min, x_max, y_min, y_max);
}

static GOData *
gog_2d_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			     GogPlotBoundInfo *bounds)
{
	Gog2DPlot *model = GOG_2D_PLOT (plot);

	if (axis == GOG_AXIS_X) {
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

	if (axis == GOG_AXIS_Y) {
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
gog_2d_finalize (GObject *obj)
{
	gog_2d_plot_clear_formats (GOG_2D_PLOT (obj));
	G_OBJECT_CLASS (plot2d_parent_klass)->finalize (obj);
}

static GType gog_xy_view_get_type (void);
static GType gog_xy_series_get_type (void);

static void
gog_2d_plot_class_init (GogPlotClass *plot_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) plot_klass;
	GogObjectClass *gog_klass = (GogObjectClass *) plot_klass;
	Gog2DPlotClass *gog_2d_plot_klass = (Gog2DPlotClass*) plot_klass;

	gog_2d_plot_klass->adjust_bounds = gog_2d_plot_real_adjust_bounds;

	plot2d_parent_klass = g_type_class_peek_parent (plot_klass);

	gobject_klass->finalize     = gog_2d_finalize;

	gog_klass->update	= gog_2d_plot_update;
	gog_klass->view_type	= gog_xy_view_get_type ();

	plot_klass->desc.num_series_max = G_MAXINT;
	plot_klass->series_type  	= gog_xy_series_get_type ();
	plot_klass->axis_set	      	= GOG_AXIS_SET_XY;
	plot_klass->axis_get_bounds   	= gog_2d_plot_axis_get_bounds;
}

static void
gog_2d_plot_init (Gog2DPlot *plot2d)
{
	plot2d->base.vary_style_by_element = FALSE;
	plot2d->x.fmt = plot2d->y.fmt = NULL;
}

GSF_DYNAMIC_CLASS (Gog2DPlot, gog_2d_plot,
	gog_2d_plot_class_init, gog_2d_plot_init,
	GOG_TYPE_PLOT)

enum {
	GOG_XY_PROP_0,
	GOG_XY_PROP_DEFAULT_STYLE_HAS_MARKERS,
	GOG_XY_PROP_DEFAULT_STYLE_HAS_LINES,
	GOG_XY_PROP_DEFAULT_STYLE_HAS_FILL,
	GOG_XY_PROP_USE_SPLINES,
	GOG_XY_PROP_DISLAY_BEFORE_GRID
};

static GogObjectClass *xy_parent_klass;

#define GOG_XY_PLOT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_XY_PLOT, GogXYPlotClass))

static char const *
gog_xy_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name scatter plot objects
	 * eg The 2nd plot in a chart will be called
	 * 	PlotXY2 */
	return N_("PlotXY");
}

static void
gog_xy_set_property (GObject *obj, guint param_id,
		     GValue const *value, GParamSpec *pspec)
{
	GogXYPlot *xy = GOG_XY_PLOT (obj);
	switch (param_id) {
	case GOG_XY_PROP_DEFAULT_STYLE_HAS_MARKERS:
		xy->default_style_has_markers = g_value_get_boolean (value);
		break;
	case GOG_XY_PROP_DEFAULT_STYLE_HAS_LINES:
		xy->default_style_has_lines = g_value_get_boolean (value);
		break;
	case GOG_XY_PROP_DEFAULT_STYLE_HAS_FILL:
		xy->default_style_has_fill = g_value_get_boolean (value);
		break;
	case GOG_XY_PROP_USE_SPLINES:
		xy->use_splines = g_value_get_boolean (value);
		break;
	case GOG_XY_PROP_DISLAY_BEFORE_GRID:
		GOG_PLOT (obj)->rendering_order = (g_value_get_boolean (value))?
						GOG_PLOT_RENDERING_BEFORE_GRID:
						GOG_PLOT_RENDERING_LAST;
		gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}
static void
gog_xy_get_property (GObject *obj, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	GogXYPlot const *xy = GOG_XY_PLOT (obj);
	GogSeries *series;
	GSList *iter;
	gboolean use_splines;

	switch (param_id) {
	case GOG_XY_PROP_DEFAULT_STYLE_HAS_MARKERS:
		g_value_set_boolean (value, xy->default_style_has_markers);
		break;
	case GOG_XY_PROP_DEFAULT_STYLE_HAS_LINES:
		g_value_set_boolean (value, xy->default_style_has_lines);
		break;
	case GOG_XY_PROP_DEFAULT_STYLE_HAS_FILL:
		g_value_set_boolean (value, xy->default_style_has_fill);
		break;
	case GOG_XY_PROP_USE_SPLINES:
		use_splines = xy->use_splines;
		/* Drop use_splines as soon as one of the series has interpolation different
		 * from GO_LINE_INTERPOLATION_SPLINE */
		for (iter = GOG_PLOT (xy)->series; iter != NULL; iter = iter->next) {
			series = iter->data;
			use_splines = use_splines && (series->interpolation == GO_LINE_INTERPOLATION_SPLINE);
		}
		g_value_set_boolean (value, use_splines);
		break;
	case GOG_XY_PROP_DISLAY_BEFORE_GRID:
		g_value_set_boolean (value, GOG_PLOT (obj)->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
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
gog_xy_plot_populate_editor (GogObject *obj,
			     GOEditor *editor,
                             GogDataAllocator *dalloc,
                             GOCmdContext *cc)
{
#ifdef GOFFICE_WITH_GTK
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_xy/gog-xy-prefs.ui",
				    GETTEXT_PACKAGE, cc);

	if (gui != NULL) {
		GtkWidget *w = go_gtk_builder_get_widget (gui, "before-grid");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				(GOG_PLOT (obj))->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (display_before_grid_cb), obj);
		w = go_gtk_builder_get_widget (gui, "gog-xy-prefs");
		go_editor_add_page (editor, w, _("Properties"));
		g_object_unref (gui);
	}

#endif
	(GOG_OBJECT_CLASS (xy_parent_klass)->populate_editor) (obj, editor, dalloc, cc);
};

static void
gog_xy_plot_class_init (GogPlotClass *plot_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) plot_klass;
	GogObjectClass *gog_klass = (GogObjectClass *) plot_klass;

	xy_parent_klass = g_type_class_peek_parent (plot_klass);

	gobject_klass->set_property = gog_xy_set_property;
	gobject_klass->get_property = gog_xy_get_property;

	g_object_class_install_property (gobject_klass, GOG_XY_PROP_DEFAULT_STYLE_HAS_MARKERS,
		g_param_spec_boolean ("default-style-has-markers",
			_("Has markers by default"),
			_("Should the default style of a series include markers"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GOG_XY_PROP_DEFAULT_STYLE_HAS_LINES,
		g_param_spec_boolean ("default-style-has-lines",
			_("Has lines by default"),
			_("Should the default style of a series include lines"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GOG_XY_PROP_DEFAULT_STYLE_HAS_FILL,
		g_param_spec_boolean ("default-style-has-fill",
			_("Has fill by default"),
			_("Should the default style of a series include fill"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GOG_XY_PROP_USE_SPLINES,
		g_param_spec_boolean ("use-splines",
			_("Use splines"),
			_("Should the plot use splines instead of linear interpolation"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GOG_XY_PROP_DISLAY_BEFORE_GRID,
		g_param_spec_boolean ("before-grid",
			_("Displayed under the grids"),
			_("Should the plot be displayed before the grids"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	gog_klass->type_name	= gog_xy_plot_type_name;
	gog_klass->populate_editor = gog_xy_plot_populate_editor;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("X"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("Y"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
/* Names of the error data are not translated since they are not used */
			{ "Y+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus1 },
			{ "Y-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus1 },
			{ "X+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus2 },
			{ "X-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus2 }
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		plot_klass->desc.series.style_fields = GO_STYLE_LINE
			| GO_STYLE_FILL
			| GO_STYLE_MARKER
			| GO_STYLE_INTERPOLATION;
	}
}

static void
gog_xy_plot_init (GogXYPlot *xy)
{
	xy->default_style_has_markers = TRUE;
	xy->default_style_has_lines = TRUE;
	xy->default_style_has_fill = FALSE;
}

GSF_DYNAMIC_CLASS (GogXYPlot, gog_xy_plot,
	gog_xy_plot_class_init, gog_xy_plot_init,
	GOG_2D_PLOT_TYPE)

/*****************************************************************************/

static GogObjectClass *bubble_parent_klass;

#define GOG_BUBBLE_PLOT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_BUBBLE_PLOT, GogBubblePlotClass))

static void gog_bubble_plot_adjust_bounds (Gog2DPlot *model, double *x_min, double *x_max, double *y_min, double *y_max);

static char const *
gog_bubble_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	return N_("PlotBubble");
}

#ifdef GOFFICE_WITH_GTK
extern gpointer gog_bubble_plot_pref (GogBubblePlot *bubble, GOCmdContext *cc);
static void
gog_bubble_plot_populate_editor (GogObject *obj,
				 GOEditor *editor,
				 G_GNUC_UNUSED GogDataAllocator *dalloc,
			GOCmdContext *cc)
{
	GtkWidget *w = gog_bubble_plot_pref (GOG_BUBBLE_PLOT (obj), cc);
	go_editor_add_page (editor, w, _("Properties"));
	g_object_unref (w);

	(GOG_OBJECT_CLASS(bubble_parent_klass)->populate_editor) (obj, editor, dalloc, cc);
}
#endif

enum {
	GOG_BUBBLE_PROP_0,
	GOG_BUBBLE_PROP_AS_AREA,
	GOG_BUBBLE_PROP_SHOW_NEGATIVES,
	GOG_BUBBLE_PROP_IN_3D,
	GOG_BUBBLE_PROP_SCALE
};

static void
gog_bubble_plot_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GogBubblePlot *bubble = GOG_BUBBLE_PLOT (obj);

	switch (param_id) {
	case GOG_BUBBLE_PROP_AS_AREA :
		bubble->size_as_area = g_value_get_boolean (value);
		break;
	case GOG_BUBBLE_PROP_SHOW_NEGATIVES :
		bubble->show_negatives = g_value_get_boolean (value);
		break;
	case GOG_BUBBLE_PROP_IN_3D :
		bubble->in_3d = g_value_get_boolean (value);
		break;
	case GOG_BUBBLE_PROP_SCALE :
		bubble->bubble_scale = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	/* none of the attributes triggers a size change yet.
	 * When we add data labels we'll need it */
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_bubble_plot_get_property (GObject *obj, guint param_id,
			  GValue *value, GParamSpec *pspec)
{
	GogBubblePlot *bubble = GOG_BUBBLE_PLOT (obj);

	switch (param_id) {
	case GOG_BUBBLE_PROP_AS_AREA :
		g_value_set_boolean (value, bubble->size_as_area);
		break;
	case GOG_BUBBLE_PROP_SHOW_NEGATIVES :
		g_value_set_boolean (value, bubble->show_negatives);
		break;
	case GOG_BUBBLE_PROP_IN_3D :
		g_value_set_boolean (value, bubble->in_3d);
		break;
	case GOG_BUBBLE_PROP_SCALE :
		g_value_set_double (value, bubble->bubble_scale);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_bubble_plot_class_init (GogPlotClass *plot_klass)
{
	GogObjectClass *gog_klass = (GogObjectClass *) plot_klass;
	GObjectClass *gobject_klass = (GObjectClass *) plot_klass;
	Gog2DPlotClass *gog_2d_plot_klass = (Gog2DPlotClass*) plot_klass;

	bubble_parent_klass = g_type_class_peek_parent (plot_klass);

	gobject_klass->set_property = gog_bubble_plot_set_property;
	gobject_klass->get_property = gog_bubble_plot_get_property;

	gog_klass->type_name	= gog_bubble_plot_type_name;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor	= gog_bubble_plot_populate_editor;
#endif
	gog_2d_plot_klass->adjust_bounds = gog_bubble_plot_adjust_bounds;

	g_object_class_install_property (gobject_klass, GOG_BUBBLE_PROP_AS_AREA,
		g_param_spec_boolean ("size-as-area",
			_("Size as area"),
			_("Display size as area instead of diameter"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GOG_BUBBLE_PROP_SHOW_NEGATIVES,
		g_param_spec_boolean ("show-negatives",
			_("Show negatives"),
			_("Draw bubbles for negative values"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GOG_BUBBLE_PROP_IN_3D,
		g_param_spec_boolean ("in-3d",
			_("In 3D"),
			_("Draw 3D bubbles"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GOG_BUBBLE_PROP_SCALE,
		g_param_spec_double ("bubble-scale",
			_("Bubble scale"),
			_("Fraction of default radius used for display"),
			0., 3., 1.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("X"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("Y"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
			{ N_("Bubble"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_BUBBLES },
/* Names of the error data are not translated since they are not used */
			{ "Y+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus1 },
			{ "Y-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus1 },
			{ "X+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus2 },
			{ "X-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus2 }
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		plot_klass->desc.series.style_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	}
}

#define BUBBLE_MAX_RADIUS_RATIO 12.	/* 4 * MAX(bubble-scale) */
static void
gog_bubble_plot_adjust_bounds (Gog2DPlot *model, double *x_min, double *x_max, double *y_min, double *y_max)
{
	/* Add room for bubbles*/
	double tmp;
	double factor = BUBBLE_MAX_RADIUS_RATIO / GOG_BUBBLE_PLOT (model)->bubble_scale - 2.;
	tmp = (*x_max - *x_min) / factor;
	*x_min -= tmp;
	*x_max += tmp;
	tmp = (*y_max - *y_min) / factor;
	*y_min -= tmp;
	*y_max += tmp;
}

static void
gog_bubble_plot_init (GogBubblePlot *bubble)
{
	bubble->size_as_area = TRUE;
	bubble->in_3d = FALSE;
	bubble->show_negatives = FALSE;
	bubble->bubble_scale = 1.0;
}

GSF_DYNAMIC_CLASS (GogBubblePlot, gog_bubble_plot,
	gog_bubble_plot_class_init, gog_bubble_plot_init,
	GOG_2D_PLOT_TYPE)

/*****************************************************************************/

enum {
	GOG_XY_COLOR_PROP_0,
	GOG_XY_COLOR_PROP_DEFAULT_STYLE_HAS_LINES,
	GOG_XY_COLOR_PROP_DEFAULT_STYLE_HAS_FILL,
	GOG_XY_COLOR_PROP_INTERPOLATION,
	GOG_XY_COLOR_PROP_HIDE_OUTLIERS,
};

static GogObjectClass *map_parent_klass;

#define GOG_XY_COLOR_PLOT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_XY_COLOR_PLOT, GogXYColorPlotClass))

static void
gog_xy_color_plot_clear_formats (GogXYColorPlot *map)
{
	go_format_unref (map->z.fmt);
	map->z.fmt = NULL;
}

static void
gog_xy_color_plot_update (GogObject *obj)
{
	GogXYColorPlot *model = GOG_XY_COLOR_PLOT (obj);
	GogXYSeries const *series = NULL;
	double z_min, z_max, tmp_min, tmp_max;
	GSList *ptr;

	z_min = DBL_MAX;
	z_max = -DBL_MAX;
	gog_xy_color_plot_clear_formats (model);
	for (ptr = model->base.base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;

		go_data_get_bounds (series->base.values[2].data, &tmp_min, &tmp_max);
		if (z_min > tmp_min) z_min = tmp_min;
		if (z_max < tmp_max) z_max = tmp_max;
		if (model->z.fmt == NULL)
			model->z.fmt = go_data_preferred_fmt (series->base.values[2].data);
		model->z.date_conv = go_data_date_conv (series->base.values[2].data);
	}
	if (model->z.minima != z_min || model->z.maxima != z_max) {
		model->z.minima = z_min;
		model->z.maxima = z_max;
		gog_axis_bound_changed (model->base.base.axis[GOG_AXIS_COLOR], GOG_OBJECT (model));
	}
	map_parent_klass->update (obj);
}

static GOData *
gog_xy_color_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			     GogPlotBoundInfo *bounds)
{
	if (axis == GOG_AXIS_COLOR) {
		GogXYColorPlot *model = GOG_XY_COLOR_PLOT (plot);

		bounds->val.minima = model->z.minima;
		bounds->val.maxima = model->z.maxima;
		bounds->is_discrete = model->z.minima > model->z.maxima ||
			!go_finite (model->z.minima) ||
			!go_finite (model->z.maxima);
		if (bounds->fmt == NULL && model->z.fmt != NULL)
			bounds->fmt = go_format_ref (model->z.fmt);
		if (model->z.date_conv)
			bounds->date_conv = model->z.date_conv;
		return NULL;
	}
	return GOG_PLOT_CLASS (map_parent_klass)->axis_get_bounds (plot, axis, bounds);
}

static char const *
gog_xy_color_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name map like plot objects
	 * eg The 2nd plot in a chart will be called
	 * 	Map2 */
	return N_("XYColor");
}

#ifdef GOFFICE_WITH_GTK
static void
hide_outliers_toggled_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "hide-outliers", gtk_toggle_button_get_active (btn), NULL);
}
#endif

static void
gog_xy_color_plot_populate_editor (GogObject *obj,
				   GOEditor *editor,
				   GogDataAllocator *dalloc,
				   GOCmdContext *cc)
{
#ifdef GOFFICE_WITH_GTK
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_xy/gog-xy-color-prefs.ui",
				    GETTEXT_PACKAGE, cc);

	if (gui != NULL) {
		GtkWidget *w = go_gtk_builder_get_widget (gui, "hide-outliers");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				(GOG_XY_COLOR_PLOT (obj))->hide_outliers);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (hide_outliers_toggled_cb), obj);
		w = go_gtk_builder_get_widget (gui, "gog-xy-color-prefs");
		go_editor_add_page (editor, w, _("Properties"));
		g_object_unref (gui);
	}

#endif
	(GOG_OBJECT_CLASS(map_parent_klass)->populate_editor) (obj, editor, dalloc, cc);
}

static void
gog_xy_color_plot_set_property (GObject *obj, guint param_id,
		     GValue const *value, GParamSpec *pspec)
{
	GogXYColorPlot *map = GOG_XY_COLOR_PLOT (obj);
	switch (param_id) {
	case GOG_XY_COLOR_PROP_DEFAULT_STYLE_HAS_LINES:
		map->default_style_has_lines = g_value_get_boolean (value);
		break;
	case GOG_XY_COLOR_PROP_DEFAULT_STYLE_HAS_FILL:
		map->default_style_has_fill = g_value_get_boolean (value);
		break;
	case GOG_XY_COLOR_PROP_HIDE_OUTLIERS:
		map->hide_outliers = g_value_get_boolean (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}

	/* none of the attributes triggers a size change yet.
	 * When we add data labels we'll need it */
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_xy_color_plot_get_property (GObject *obj, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	GogXYColorPlot const *map = GOG_XY_COLOR_PLOT (obj);
	switch (param_id) {
	case GOG_XY_COLOR_PROP_DEFAULT_STYLE_HAS_LINES:
		g_value_set_boolean (value, map->default_style_has_lines);
		break;
	case GOG_XY_COLOR_PROP_DEFAULT_STYLE_HAS_FILL:
		g_value_set_boolean (value, map->default_style_has_fill);
		break;
	case GOG_XY_COLOR_PROP_HIDE_OUTLIERS:
		g_value_set_boolean (value, map->hide_outliers);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_xy_color_plot_finalize (GObject *obj)
{
	gog_xy_color_plot_clear_formats (GOG_XY_COLOR_PLOT (obj));
	G_OBJECT_CLASS (map_parent_klass)->finalize (obj);
}

static void
gog_xy_color_plot_class_init (GogPlotClass *plot_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) plot_klass;
	GogObjectClass *gog_klass = (GogObjectClass *) plot_klass;

	map_parent_klass = g_type_class_peek_parent (plot_klass);

	gobject_klass->set_property = gog_xy_color_plot_set_property;
	gobject_klass->get_property = gog_xy_color_plot_get_property;
	gobject_klass->finalize     = gog_xy_color_plot_finalize;

	g_object_class_install_property (gobject_klass, GOG_XY_COLOR_PROP_DEFAULT_STYLE_HAS_LINES,
		g_param_spec_boolean ("default-style-has-lines",
			_("Has lines by default"),
			_("Should the default style of a series include lines"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GOG_XY_COLOR_PROP_DEFAULT_STYLE_HAS_FILL,
		g_param_spec_boolean ("default-style-has-fill",
			_("Has fill by default"),
			_("Should the default style of a series include fill"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, GOG_XY_COLOR_PROP_HIDE_OUTLIERS,
		g_param_spec_boolean  ("hide-outliers",
			_("hide-outliers"),
			_("Hide data outside of the color axis bounds"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	gog_klass->type_name		= gog_xy_color_plot_type_name;
	gog_klass->update		= gog_xy_color_plot_update;
	gog_klass->populate_editor	= gog_xy_color_plot_populate_editor;
	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("X"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("Y"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
			{ N_("Z"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_EXTRA1 },
/* Names of the error data are not translated since they are not used */
			{ "Y+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus1 },
			{ "Y-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus1 },
			{ "X+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus2 },
			{ "X-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus2 }
		};
		plot_klass->desc.series.dim = dimensions;
		plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		plot_klass->desc.series.style_fields = GO_STYLE_LINE | GO_STYLE_MARKER
			| GO_STYLE_INTERPOLATION |GO_STYLE_MARKER_NO_COLOR;
	}
	plot_klass->axis_set	      	= GOG_AXIS_SET_XY_COLOR;
	plot_klass->axis_get_bounds   	= gog_xy_color_plot_axis_get_bounds;
}

static void
gog_xy_color_plot_init (GogXYColorPlot *map)
{
	map->default_style_has_lines = FALSE;
	map->default_style_has_fill = FALSE;
	map->hide_outliers = TRUE;
}

GSF_DYNAMIC_CLASS (GogXYColorPlot, gog_xy_color_plot,
	gog_xy_color_plot_class_init, gog_xy_color_plot_init,
	GOG_2D_PLOT_TYPE)


/*****************************************************************************/
typedef GogPlotView		GogXYView;
typedef GogPlotViewClass	GogXYViewClass;

/*
static GOColor
get_map_color (double z, gboolean hide_outliers)
{
	if (hide_outliers && (z < 0. || z > 6.))
		return 0;
	if (z <= 0.)
		return GO_COLOR_BLUE;
	if (z <= 1.)
		return GO_COLOR_BLUE + ((int) (z * 255.) << 16);
	if (z <= 2.)
		return GO_COLOR_GREEN + ((int) ((2. - z) * 255) << 8);
	if (z <= 4.)
		return GO_COLOR_GREEN + ((int) ((z / 2. - 1.) * 255) << 24);
	if (z <= 6.)
		return GO_COLOR_RED + ((int) ((3. - z / 2.) * 255) << 16);
	return GO_COLOR_RED;
}
*/

typedef struct {
	double x, y;
	GOColor color;
	unsigned index;
} MarkerData;

static int
gog_xy_view_get_data_at_point (GogPlotView *view, double x, double y, GogSeries **series)
{
	Gog2DPlot const *model = GOG_2D_PLOT (view->base.model);
	GogChart *chart = GOG_CHART (view->base.model->parent);
	GogChartMap *chart_map;
	GogAxisMap *x_map, *y_map;
	GogXYSeries const *pseries = NULL;
	int i = -1, dist, n, max_dist = 0, line_dist = 0;
	GOStyle *style;
	GogViewAllocation const *area;
	double const *y_vals, *x_vals, *z_vals;
	double zmax, rmax = 0., xc, yc, zc;
	gboolean show_negatives, size_as_area = TRUE, is_bubble = GOG_IS_BUBBLE_PLOT (model);
	GSList *ptr, *ser;
	GogSeriesElement *gse;
	GList *overrides; /* not const because we call g_list_* which have no const equivalent */

	if (g_slist_length (model->base.series) < 1)
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

	/* because series and overrides are GSLists, we have to copy the lists and to
	 reverse them to get the right point (in case of overlap) */
	ser = g_slist_reverse (g_slist_copy (model->base.series));
	for (ptr = ser ; ptr != NULL ; ptr = ptr->next) {
		pseries = ptr->data;

		if (!gog_series_is_valid (GOG_SERIES (pseries)))
			continue;

		if (is_bubble)
			n = gog_series_get_xyz_data (GOG_SERIES (pseries), &x_vals, &y_vals, &z_vals);
		else
			n = gog_series_get_xy_data (GOG_SERIES (pseries), &x_vals, &y_vals);
		if (n < 1)
			continue;

		if (!is_bubble) {
			style = go_styled_object_get_style (GO_STYLED_OBJECT (pseries));
			if (go_style_is_line_visible (style))
				line_dist = ceil (style->line.width / 2);
			if (go_style_is_marker_visible (style))
				max_dist = (go_marker_get_size (style->marker.mark) + 1) / 2;
			else if (go_style_is_line_visible (style))
				max_dist = line_dist;
			else
				max_dist = 0;
		} else {
			double zmin;
			go_data_get_bounds (pseries->base.values[2].data, &zmin, &zmax);
			show_negatives = GOG_BUBBLE_PLOT (view->base.model)->show_negatives;
			if ((! go_finite (zmax)) || (!show_negatives && (zmax <= 0)))
				continue;
			rmax = MIN (view->base.residual.w, view->base.residual.h) / BUBBLE_MAX_RADIUS_RATIO
						* GOG_BUBBLE_PLOT (view->base.model)->bubble_scale;
			size_as_area = GOG_BUBBLE_PLOT (view->base.model)->size_as_area;
			if (show_negatives) {
				zmin = fabs (zmin);
				if (zmin > zmax)
					zmax = zmin;
			}
		}
		overrides = g_list_last ((GList *) gog_series_get_overrides (GOG_SERIES (pseries)));

		for (i = n - 1; i >= 0; i--) {
			yc = y_vals[i];
			if (!gog_axis_map_finite (y_map, yc))
				continue;
			xc= x_vals ? x_vals[i] : i + 1;
			if (!gog_axis_map_finite (x_map, xc))
				continue;
			xc = fabs (gog_axis_map_to_view (x_map, xc) - x);
			yc = fabs (gog_axis_map_to_view (y_map, yc) - y);
			if (is_bubble) {
				zc = z_vals[i];
				if (zc < 0) {
					if (GOG_BUBBLE_PLOT(model)->show_negatives)
						dist = ((size_as_area)? sqrt (-zc / zmax): -zc / zmax) * rmax;
					else
						continue;
				} else
					dist = ((size_as_area)? sqrt (zc / zmax): zc / zmax) * rmax;
				/* should we add half bubble outline width to dist? */
				if (hypot (xc, yc) <= dist) {
					*series = GOG_SERIES (pseries);
					goto clean_exit;
				}
			} else {
				dist = MAX (xc, yc);
				gse = NULL;
				while (overrides &&
				       GOG_SERIES_ELEMENT (overrides->data)->index > (unsigned) i)
					overrides = g_list_previous (overrides);
				if (overrides &&
				    GOG_SERIES_ELEMENT (overrides->data)->index == (unsigned) i) {
					gse = GOG_SERIES_ELEMENT (overrides->data);
					overrides = g_list_previous (overrides);
					style = go_styled_object_get_style (GO_STYLED_OBJECT (gse));
					if (go_style_is_marker_visible (style)) {
						if (dist <= (go_marker_get_size (style->marker.mark) + 1) / 2) {
							*series = GOG_SERIES (pseries);
							goto clean_exit;
						}
					} else if (dist <= line_dist) {
						*series = GOG_SERIES (pseries);
						goto clean_exit;
					}
				}
				if (gse == NULL && dist <= max_dist) {
					*series = GOG_SERIES (pseries);
					goto clean_exit;
				}
			}
		}
	}

clean_exit:
	g_slist_free (ser);
	gog_chart_map_free (chart_map);
	return i;
}

static void
gog_xy_view_render (GogView *view, GogViewAllocation const *bbox)
{
	Gog2DPlot const *model = GOG_2D_PLOT (view->model);
	GogTheme *theme = gog_object_get_theme (GOG_OBJECT (model));
	GogChart *chart = GOG_CHART (view->model->parent);
	gboolean is_map = GOG_IS_XY_COLOR_PLOT (model);
	double const scale = gog_renderer_get_scale (view->renderer);
	unsigned num_series;
	GogChartMap *chart_map;
	GogAxisMap *x_map, *y_map, *z_map = NULL;
	unsigned i, j, k, n;
	GOStyle *neg_style = NULL;
	GOStyle *style = NULL;
	GogViewAllocation const *area;
	GOPath *path = NULL, *next_path = NULL;
	GSList *ptr;
	double const *y_vals, *x_vals, *z_vals =  NULL;
	double x = 0., y = 0., z = 0., x_canvas = 0., y_canvas = 0.;
	double zmax, rmax = 0., x_left, y_bottom, x_right, y_top;
	gboolean size_as_area = TRUE, hide_outliers = TRUE;
	GogObjectRole const *lbl_role = NULL;
	GogAxisColorMap const *color_map = NULL;
	double max = 0.;
	GogSeriesElement *gse;
	GList const *overrides;

	for (num_series = 0, ptr = model->base.series ; ptr != NULL ; ptr = ptr->next, num_series++);
	if (num_series < 1)
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

	if (is_map) {
		hide_outliers = GOG_XY_COLOR_PLOT (model)->hide_outliers;
		color_map = gog_axis_get_color_map (model->base.axis[GOG_AXIS_COLOR]);
		max = gog_axis_color_map_get_max (color_map);
		z_map = gog_axis_map_new (model->base.axis[GOG_AXIS_COLOR], 0, max);
	}

	/* What we really want is to draw drop lines from point to
	 * a selected axis. But for this purpose, we need a GogAxisBase selector in
	 * GogSeriesLines (we might actually need GogDropLines. For now just let
	 * the drop lines go the axis. */

	gog_axis_map_get_extents (x_map, &x_left, &x_right);
	x_left = gog_axis_map_to_view (x_map, x_left);
	x_right = gog_axis_map_to_view (x_map, x_right);
	gog_axis_map_get_extents (y_map, &y_bottom, &y_top);
	y_bottom = gog_axis_map_to_view (y_map, y_bottom);
	y_top = gog_axis_map_to_view (y_map, y_top);

	gog_renderer_push_clip_rectangle (view->renderer, view->allocation.x, view->allocation.y,
					  view->allocation.w, view->allocation.h);

	MarkerData **markers = g_alloca (num_series * sizeof (MarkerData *));
	unsigned *num_markers = g_alloca (num_series * sizeof (unsigned));

	for (j = 0, ptr = model->base.series ; ptr != NULL ; ptr = ptr->next, j++) {
		GogXYSeries const *series = ptr->data;
		markers[j] = NULL;

		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;

		if (is_map || GOG_IS_BUBBLE_PLOT (model))
			n = gog_series_get_xyz_data (GOG_SERIES (series), &x_vals, &y_vals, &z_vals);
		else
			n = gog_series_get_xy_data (GOG_SERIES (series), &x_vals, &y_vals);
		if (n < 1 || y_vals == NULL || ((is_map || GOG_IS_BUBBLE_PLOT(model)) && z_vals == NULL))
			continue;

		style = go_styled_object_get_style (GO_STYLED_OBJECT (series));

		gboolean show_marks = go_style_is_marker_visible (style);
		gboolean show_lines = go_style_is_line_visible (style);
		gboolean show_fill = go_style_is_fill_visible (style);

		if (model->base.vary_style_by_element)
			style = go_style_dup (style);
		gog_renderer_push_style (view->renderer, style);

		if ((show_lines || show_fill) && !GOG_IS_BUBBLE_PLOT (model)) {
			if (next_path != NULL)
				path = next_path;
			else
				path = gog_chart_map_make_path (chart_map, x_vals, y_vals,
								n, series->base.interpolation,
								series->base.interpolation_skip_invalid,
								(gpointer)&series->clamped_derivs);

			next_path = NULL;

			if (series->base.interpolation == GO_LINE_INTERPOLATION_CLOSED_SPLINE)
				gog_renderer_fill_serie	(view->renderer, path, NULL);
			else if (series->base.fill_type != GOG_SERIES_FILL_TYPE_NEXT) {
				GOPath *close_path;

				close_path = gog_chart_map_make_close_path (chart_map, x_vals, y_vals, n,
									    series->base.fill_type);
				gog_renderer_fill_serie	(view->renderer, path, close_path);
				if (close_path != NULL)
					go_path_free (close_path);
			} else {
				if (ptr->next != NULL) {
					GogXYSeries *next_series;

					next_series = ptr->next->data;
					if (gog_series_is_valid (GOG_SERIES (next_series))) {
						const double *next_x_vals, *next_y_vals;
						unsigned int next_n_points;

						next_n_points = gog_series_get_xy_data
							(GOG_SERIES (next_series),
							 &next_x_vals, &next_y_vals);

						next_path = gog_chart_map_make_path
							(chart_map, next_x_vals, next_y_vals,
							 next_n_points, next_series->base.interpolation,
							 series->base.interpolation_skip_invalid,
							 (gpointer)&series->clamped_derivs);

					}
				}
				gog_renderer_fill_serie (view->renderer, path, next_path);
			}
		} else if (next_path) {
			go_path_free (next_path);
			next_path = NULL;
		}

		if (series->hdroplines) {
			GOPath *drop_path;
			double y_drop, x_target;
			GogAxis *axis = GOG_PLOT (model)->axis[GOG_AXIS_Y];
			GogAxisPosition pos = gog_axis_base_get_clamped_position (GOG_AXIS_BASE (axis));
			switch (pos) {
			case GOG_AXIS_AT_LOW:
				x_target = gog_axis_map_is_inverted (x_map)? x_right: x_left;
				break;
			case GOG_AXIS_CROSS: {
				GogChartMap *c_map;
				GogAxisMap *a_map;
				c_map = gog_chart_map_new (chart, area, axis,
						gog_axis_base_get_crossed_axis (GOG_AXIS_BASE (axis)),
						NULL, FALSE);
				a_map = gog_chart_map_get_axis_map (c_map, 1);
				x_target = gog_axis_map_to_view (a_map, gog_axis_base_get_cross_location (GOG_AXIS_BASE (axis)));
				gog_chart_map_free (c_map);
				break;
			}
			case GOG_AXIS_AT_HIGH:
				x_target = gog_axis_map_is_inverted (x_map)? x_left: x_right;
				break;
			default:
				/* this should not occur */
				x_target = gog_axis_map_to_view (x_map, 0);
				break;
			}
			gog_renderer_push_style (view->renderer,
				go_styled_object_get_style (GO_STYLED_OBJECT (series->hdroplines)));
			drop_path = go_path_new ();
			go_path_set_options (drop_path, GO_PATH_OPTIONS_SHARP);
			for (i = 0; i < n; i++) {
				if (!gog_axis_map_finite (y_map, y_vals[i]))
					continue;
				x = x_vals ? x_vals[i] : i + 1;
				if (!gog_axis_map_finite (x_map, x))
					continue;
				y_drop = gog_axis_map_to_view (y_map, y_vals[i]);
				go_path_move_to (drop_path, x_target, y_drop);
				go_path_line_to (drop_path, gog_axis_map_to_view (x_map, x), y_drop);
			}
			gog_renderer_stroke_serie (view->renderer, drop_path);
			go_path_free (drop_path);
			gog_renderer_pop_style (view->renderer);
		}

		if (series->vdroplines) {
			GOPath *drop_path;
			double x_drop, y_target;
			GogAxis *axis = GOG_PLOT (model)->axis[GOG_AXIS_X];
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
			gog_renderer_push_style (view->renderer,
				go_styled_object_get_style (GO_STYLED_OBJECT (series->vdroplines)));
			drop_path = go_path_new ();
			go_path_set_options (drop_path, GO_PATH_OPTIONS_SHARP);
			for (i = 0; i < n; i++) {
				if (!gog_axis_map_finite (y_map, y_vals[i]))
					continue;
				x = x_vals ? x_vals[i] : i + 1;
				if (!gog_axis_map_finite (x_map, x))
					continue;
				x_drop = gog_axis_map_to_view (x_map, x);
				go_path_move_to (drop_path, x_drop, y_target);
				go_path_line_to (drop_path, x_drop, gog_axis_map_to_view (y_map, y_vals[i]));
			}
			gog_renderer_stroke_serie (view->renderer, drop_path);
			go_path_free (drop_path);
			gog_renderer_pop_style (view->renderer);
		}

		if ((show_lines || show_fill) && !GOG_IS_BUBBLE_PLOT (model)) {
			gog_renderer_stroke_serie (view->renderer, path);
			go_path_free (path);
		}

		if (GOG_IS_BUBBLE_PLOT (model)) {
			double zmin;
			gboolean show_negatives = GOG_BUBBLE_PLOT (view->model)->show_negatives;
			go_data_get_bounds (series->base.values[2].data, &zmin, &zmax);
			if ((! go_finite (zmax)) || (!show_negatives && (zmax <= 0))) continue;
			rmax = MIN (view->residual.w, view->residual.h) / BUBBLE_MAX_RADIUS_RATIO
						* GOG_BUBBLE_PLOT (view->model)->bubble_scale;
			size_as_area = GOG_BUBBLE_PLOT (view->model)->size_as_area;
			// in_3d = GOG_BUBBLE_PLOT (view->model)->in_3d;
			if (show_negatives) {
				zmin = fabs (zmin);
				if (zmin > zmax) zmax = zmin;
				neg_style = go_style_dup (GOG_STYLED_OBJECT (series)->style);
				neg_style->fill.type = GO_STYLE_FILL_PATTERN;
				neg_style->fill.pattern.pattern = GO_PATTERN_SOLID;
				neg_style->fill.pattern.back = GO_COLOR_WHITE;
			}
		}

		if (show_marks && !GOG_IS_BUBBLE_PLOT (model))
			markers[j] = g_new0 (MarkerData, n);

		double margin = gog_renderer_line_size (view->renderer, 1);
		double x_margin_min = view->allocation.x - margin;
		double x_margin_max = view->allocation.x + view->allocation.w + margin;
		double y_margin_min = view->allocation.y - margin;
		double y_margin_max = view->allocation.y + view->allocation.h + margin;

		overrides = gog_series_get_overrides (GOG_SERIES (series));
		k = 0;
		for (i = 1 ; i <= n ; i++) {
			x = x_vals ? *x_vals++ : i;
			y = *y_vals++;
			if (is_map || GOG_IS_BUBBLE_PLOT(model)) {
				z = *z_vals++;
				if (isnan (z) || !go_finite (z))
					continue;
			}
			if (isnan (y) || isnan (x))
				continue;
			if (!gog_axis_map_finite (y_map, y))
				y = (series->invalid_as_zero)? 0: go_nan; /* excel is just sooooo consistent */
			if (!gog_axis_map_finite (x_map, x))
				x =  (series->invalid_as_zero)? 0: go_nan;
			x_canvas = gog_axis_map_to_view (x_map, x);
			y_canvas = gog_axis_map_to_view (y_map, y);
			if (GOG_IS_BUBBLE_PLOT(model)) {
				if (z < 0) {
					if (GOG_BUBBLE_PLOT(model)->show_negatives) {
						gog_renderer_push_style (view->renderer, neg_style);
						gog_renderer_draw_circle (view->renderer, x_canvas, y_canvas,
								    ((size_as_area) ?
								     sqrt (- z / zmax) :
								    - z / zmax) * rmax);
						gog_renderer_pop_style (view->renderer);
					} else continue;
				} else {
					gse = NULL;
					while (overrides && GOG_SERIES_ELEMENT (overrides->data)->index < i - 1)
						overrides = overrides->next;
					if ((overrides != NULL) &&
					   	(GOG_SERIES_ELEMENT (overrides->data)->index == i - 1)) {
							gse = GOG_SERIES_ELEMENT (overrides->data);
							overrides = overrides->next;
							gog_renderer_push_style (view->renderer,
								go_styled_object_get_style (
									GO_STYLED_OBJECT (gse)));
					} else if (model->base.vary_style_by_element)
						gog_theme_fillin_style (theme, style, GOG_OBJECT (series),
									model->base.index_num + i - 1, style->interesting_fields);
					gog_renderer_draw_circle (view->renderer, x_canvas, y_canvas,
							    ((size_as_area) ?
							    sqrt (z / zmax) :
							    z / zmax) * rmax);
					if (gse)
						gog_renderer_pop_style (view->renderer);
				}
			}

			/* draw error bars after line */
			if (gog_error_bar_is_visible (series->x_errors)) {
				GogErrorBar const *bar = series->x_errors;
				double xerrmin, xerrmax;
				 if (gog_error_bar_get_bounds (bar, i - 1, &xerrmin, &xerrmax)) {
					 gog_error_bar_render (bar, view->renderer,
								   chart_map,
								   x, y,
								   xerrmin, xerrmax,
								   GOG_ERROR_BAR_DIRECTION_HORIZONTAL);
				 }
			}

			if (gog_error_bar_is_visible (series->y_errors)) {
				GogErrorBar const *bar = series->y_errors;
				double yerrmin, yerrmax;
				 if (gog_error_bar_get_bounds (bar, i - 1, &yerrmin, &yerrmax)) {
					 gog_error_bar_render (bar, view->renderer,
								   chart_map, x, y,
								   yerrmin, yerrmax,
								   GOG_ERROR_BAR_DIRECTION_VERTICAL);
				 }
			}

			/* draw marker after line */
			if (show_marks &&
			    x_margin_min <= x_canvas && x_canvas <= x_margin_max &&
			    y_margin_min <= y_canvas && y_canvas <= y_margin_max) {
				markers[j][k].x = x_canvas;
				markers[j][k].y = y_canvas;
				markers[j][k].index = i - 1;
				if (is_map) {
					if (gog_axis_map_finite (z_map, z)) {
						double zc = gog_axis_map_to_view (z_map, z);
						if (hide_outliers && (zc < 0 || zc > max))
							markers[j][k].color = 0;
						else
							markers[j][k].color = gog_axis_color_map_get_color (color_map, CLAMP (zc, 0, max));
					} else
						markers[j][k].color = 0;
				}
				k++;
			}
		}

		gog_renderer_pop_style (view->renderer);
		num_markers[j] = k;
	}

	if (next_path != NULL)
		go_path_free (next_path);

	if (GOG_IS_BUBBLE_PLOT (model)) {
		if (model->base.vary_style_by_element)
			g_object_unref (style);
		if (((GogBubblePlot*)model)->show_negatives)
			g_object_unref (neg_style);
	}
	gog_renderer_pop_clip (view->renderer);

	if (!GOG_IS_BUBBLE_PLOT (model)) {
		int j;

		for (j = 0, ptr = model->base.series ; ptr != NULL ; ptr = ptr->next, j++) {
			GogXYSeries const *series = ptr->data;
			overrides = gog_series_get_overrides (GOG_SERIES (series));
			if (markers[j] != NULL) {
				style = GOG_STYLED_OBJECT (series)->style;
				gog_renderer_push_style (view->renderer, style);
				for (k = 0; k < num_markers[j]; k++) {
					gse = NULL;
					while (overrides &&
					       GOG_SERIES_ELEMENT (overrides->data)->index < markers[j][k].index)
						overrides = overrides->next;
					if (overrides &&
					    GOG_SERIES_ELEMENT (overrides->data)->index == markers[j][k].index) {
						gse = GOG_SERIES_ELEMENT (overrides->data);
						overrides = overrides->next;
						style = go_styled_object_get_style (GO_STYLED_OBJECT (gse));
						gog_renderer_push_style (view->renderer, style);
					}
					if (is_map) {
						go_marker_set_outline_color
							(style->marker.mark,markers[j][k].color);
						go_marker_set_fill_color
							(style->marker.mark,markers[j][k].color);
						gog_renderer_push_style (view->renderer, style);
					}
					gog_renderer_draw_marker (view->renderer,
								  markers[j][k].x,
								  markers[j][k].y);
					if (is_map)
						gog_renderer_pop_style (view->renderer);
					if (gse) {
						gog_renderer_pop_style (view->renderer);
						style = GOG_STYLED_OBJECT (series)->style;
					}
				}
				gog_renderer_pop_style (view->renderer);
				g_free (markers[j]);
			} else if (overrides != NULL) {
				n = gog_series_get_xy_data (GOG_SERIES (series), &x_vals, &y_vals);
				while (overrides != NULL) {
					gse = GOG_SERIES_ELEMENT (overrides->data);
					if (gse->index >= n) {
						g_warning ("Invalid series element index");
						overrides = overrides->next;
						continue;
					}
					x = x_vals ? x_vals[gse->index] : gse->index + 1;
					y = y_vals[gse->index];
					if (gog_axis_map_finite (y_map, y) && gog_axis_map_finite (x_map, x)) {
						style = go_styled_object_get_style (GO_STYLED_OBJECT (gse));
						gog_renderer_push_style (view->renderer, style);
						x_canvas = gog_axis_map_to_view (x_map, x);
						y_canvas = gog_axis_map_to_view (y_map, y);
						gog_renderer_draw_marker (view->renderer,
									  x_canvas,
									  y_canvas);
						gog_renderer_pop_style (view->renderer);
					}
					overrides = overrides->next;
				}
			}
		}
	}

	/* render series labels */
	// first clip again to avoid labels outside of allocation (#47)
	gog_renderer_push_clip_rectangle (view->renderer, view->allocation.x, view->allocation.y,
			  view->allocation.w, view->allocation.h);
	for (j = 0, ptr = model->base.series ; ptr != NULL ; ptr = ptr->next, j++) {
		GogXYSeries const *series = ptr->data;
		GSList *labels, *cur;
		double const *cur_x, *cur_y, *cur_z;
		GogViewAllocation alloc;
		GOAnchorType anchor;
		double msize;

		markers[j] = NULL;
		z_vals = NULL;
		style = go_styled_object_get_style (GO_STYLED_OBJECT (series));
		if (style->interesting_fields & GO_STYLE_MARKER)
			msize = (double) go_marker_get_size (style->marker.mark) / 2.;
		else
			msize = style->line.width / .7;

		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;

		if (!lbl_role)
			lbl_role = gog_object_find_role_by_name (GOG_OBJECT (series), "Data labels");
		labels = gog_object_get_children (GOG_OBJECT (series), lbl_role);
		if (g_slist_length (labels) == 0) {
			g_slist_free (labels);
			continue;
		}
		if (GOG_IS_BUBBLE_PLOT (model)) {
			double zmin;
			go_data_get_bounds (series->base.values[2].data, &zmin, &zmax);
			gboolean show_negatives = GOG_BUBBLE_PLOT (view->model)->show_negatives;
			if ((! go_finite (zmax)) || (!show_negatives && (zmax <= 0))) continue;
			rmax = MIN (view->residual.w, view->residual.h) / BUBBLE_MAX_RADIUS_RATIO
						* GOG_BUBBLE_PLOT (view->model)->bubble_scale;
			size_as_area = GOG_BUBBLE_PLOT (view->model)->size_as_area;
			if (show_negatives) {
				zmin = fabs (zmin);
				if (zmin > zmax) zmax = zmin;
			}
			n = gog_series_get_xyz_data (GOG_SERIES (series), &x_vals, &y_vals, &z_vals);
		} else if (GOG_IS_XY_COLOR_PLOT (model))
			n = gog_series_get_xyz_data (GOG_SERIES (series), &x_vals, &y_vals, &z_vals);
		else
			n = gog_series_get_xy_data (GOG_SERIES (series), &x_vals, &y_vals);
		if (n < 1) {
			g_slist_free (labels);
			continue;
		}
		for (cur = labels; cur != NULL; cur = cur->next) {
			GogSeriesLabels *lbls = GOG_SERIES_LABELS (cur->data);
			GogSeriesLabelElt const *Elt;
			unsigned offset, cur_offset;
			GogSeriesLabelsPos position, cur_position;
			g_object_get (lbls, "offset", &offset, NULL);
			position = gog_series_labels_get_position (lbls);
			cur_x = x_vals;
			cur_y = y_vals;
			cur_z = z_vals;
			if (cur_x == NULL || cur_y == NULL)
				break;
			gog_renderer_push_style (view->renderer, go_styled_object_get_style (GO_STYLED_OBJECT (lbls)));
			overrides = gog_series_get_overrides (GOG_SERIES (series));
			for (i = 1 ; i <= n ; i++) {
				x = x_vals ? *cur_x++ : i;
				y = *cur_y++;
				if (GOG_IS_BUBBLE_PLOT(model)) {
					z = *cur_z++;
					if (isnan (z) || !go_finite (z))
						continue;
				}
				if (isnan (y) || isnan (x))
					continue;
				if (!gog_axis_map_finite (y_map, y))
					y = (series->invalid_as_zero)? 0: go_nan; /* excel is just sooooo consistent */
				if (!gog_axis_map_finite (x_map, x))
					x =  (series->invalid_as_zero)? 0: go_nan;
				alloc.x = gog_axis_map_to_view (x_map, x);
				alloc.y = gog_axis_map_to_view (y_map, y);
				Elt = gog_series_labels_vector_get_element (lbls, i - 1);
				if (Elt && Elt->point) {
					g_object_get (Elt->point, "offset", &cur_offset, NULL);
					cur_position = gog_data_label_get_position (GOG_DATA_LABEL (Elt->point));
				} else {
					cur_offset = offset;
					cur_position = position;
				}
				if (GOG_IS_BUBBLE_PLOT (model)) {
					cur_offset += msize +
						(size_as_area
						 ? sqrt (z / zmax)
						 : z / zmax) * rmax;
				} else if (overrides &&
					   GOG_SERIES_ELEMENT (overrides->data)->index == i) {
					GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (overrides->data));
					cur_offset += (style->interesting_fields & GO_STYLE_MARKER)
						? go_marker_get_size (style->marker.mark) / 2.
						: msize;
				} else
					cur_offset += msize;
				switch (cur_position) {
				default:
				case GOG_SERIES_LABELS_CENTERED:
					anchor = GO_ANCHOR_CENTER;
					break;
				case GOG_SERIES_LABELS_TOP:
					alloc.y -= cur_offset * scale;
					anchor = GO_ANCHOR_SOUTH;
					break;
				case GOG_SERIES_LABELS_BOTTOM:
					alloc.y += cur_offset * scale;
					anchor = GO_ANCHOR_NORTH;
					break;
				case GOG_SERIES_LABELS_LEFT:
					alloc.x -= cur_offset * scale;
					anchor = GO_ANCHOR_EAST;
					break;
				case GOG_SERIES_LABELS_RIGHT:
					alloc.x += cur_offset * scale;
					anchor = GO_ANCHOR_WEST;
					break;
				}
				gog_renderer_draw_data_label (view->renderer,
				                              Elt,
				                              &alloc,
				                              anchor,
				                              style);
			}
			gog_renderer_pop_style (view->renderer);
		}
		g_slist_free (labels);
	}

	gog_renderer_pop_clip (view->renderer);

	/* Now render children, may be should come before markers? */
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);

	gog_chart_map_free (chart_map);
	if (is_map)
		gog_axis_map_free (z_map);
}

static GogViewClass *xy_view_parent_klass;

static void
gog_xy_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;
	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
	(xy_view_parent_klass->size_allocate) (view, allocation);
}

static void
gog_xy_view_class_init (GogViewClass *view_klass)
{
	GogPlotViewClass *pv_klass = (GogPlotViewClass *) view_klass;
	xy_view_parent_klass = (GogViewClass*) g_type_class_peek_parent (view_klass);
	view_klass->render	  = gog_xy_view_render;
	view_klass->size_allocate = gog_xy_view_size_allocate;
	view_klass->clip	  = FALSE;
	pv_klass->get_data_at_point = gog_xy_view_get_data_at_point;
}

GSF_DYNAMIC_CLASS (GogXYView, gog_xy_view,
	gog_xy_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)

/*****************************************************************************/

typedef GogView		GogXYSeriesView;
typedef GogViewClass	GogXYSeriesViewClass;

#define GOG_TYPE_XY_SERIES_VIEW	(gog_xy_series_view_get_type ())
#define GOG_XY_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XY_SERIES_VIEW, GogXYSeriesView))
#define GOG_IS_XY_SERIES_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XY_SERIES_VIEW))

static void
gog_xy_series_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr;
	for (ptr = view->children ; ptr != NULL ; ptr = ptr->next)
		gog_view_render	(ptr->data, bbox);
}

static void
gog_xy_series_view_size_allocate (GogView *view, GogViewAllocation const *allocation)
{
	GSList *ptr;

	for (ptr = view->children; ptr != NULL; ptr = ptr->next)
		gog_view_size_allocate (GOG_VIEW (ptr->data), allocation);
}

static void
gog_xy_series_view_class_init (GogXYSeriesViewClass *gview_klass)
{
	GogViewClass *view_klass = GOG_VIEW_CLASS (gview_klass);
	view_klass->render = gog_xy_series_view_render;
	view_klass->size_allocate = gog_xy_series_view_size_allocate;
	view_klass->build_toolkit = NULL;
}

GSF_DYNAMIC_CLASS (GogXYSeriesView, gog_xy_series_view,
	gog_xy_series_view_class_init, NULL,
	GOG_TYPE_VIEW)

/*****************************************************************************/

static gboolean
horiz_drop_lines_can_add (GogObject const *parent)
{
	GogXYSeries *series = GOG_XY_SERIES (parent);
	return (series->hdroplines == NULL);
}

static void
horiz_drop_lines_post_add (GogObject *parent, GogObject *child)
{
	GogXYSeries *series = GOG_XY_SERIES (parent);
	series->hdroplines = child;
	gog_object_request_update (child);
}

static void
horiz_drop_lines_pre_remove (GogObject *parent, GogObject *child)
{
	GogXYSeries *series = GOG_XY_SERIES (parent);
	series->hdroplines = NULL;
}

/*****************************************************************************/

static gboolean
vert_drop_lines_can_add (GogObject const *parent)
{
	GogXYSeries *series = GOG_XY_SERIES (parent);
	return (series->vdroplines == NULL);
}

static void
vert_drop_lines_post_add (GogObject *parent, GogObject *child)
{
	GogXYSeries *series = GOG_XY_SERIES (parent);
	series->vdroplines = child;
	gog_object_request_update (child);
}

static void
vert_drop_lines_pre_remove (GogObject *parent, GogObject *child)
{
	GogXYSeries *series = GOG_XY_SERIES (parent);
	series->vdroplines = NULL;
}

/****************************************************************************/

typedef GogSeriesElement GogXYSeriesElement;
typedef GogSeriesElementClass GogXYSeriesElementClass;
#define GOG_TYPE_XY_SERIES_ELEMENT	(gog_xy_series_element_get_type ())
#define GOG_XY_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XY_SERIES_ELEMENT, GogXYSeriesElement))
#define GOG_IS_XY_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XY_SERIES_ELEMENT))
GType gog_xy_series_element_get_type (void);

static void
gog_xy_series_element_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogSeries const *series = GOG_SERIES (GOG_OBJECT (gso)->parent);
	GOStyle *parent_style;

	g_return_if_fail (series != NULL);

	parent_style = go_styled_object_get_style (GO_STYLED_OBJECT (series));
	if (parent_style->interesting_fields & GO_STYLE_MARKER)
		style->interesting_fields = parent_style->interesting_fields & (GO_STYLE_MARKER | GO_STYLE_MARKER_NO_COLOR);
	else
		style->interesting_fields = parent_style->interesting_fields;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), GOG_SERIES_ELEMENT (gso)->index, style->interesting_fields);
}

static void
gog_xy_series_element_class_init (GogXYSeriesElementClass *klass)
{
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) klass;
	style_klass->init_style	    	= gog_xy_series_element_init_style;
}

GSF_DYNAMIC_CLASS (GogXYSeriesElement, gog_xy_series_element,
	gog_xy_series_element_class_init, NULL,
	GOG_TYPE_SERIES_ELEMENT)

/****************************************************************************/

typedef struct {
	GogObject base;
	GogXYSeries *series;
	GogDatasetElement *derivs;
} GogXYInterpolationClamps;

typedef GogObjectClass GogXYInterpolationClampsClass;

#define GOG_TYPE_XY_INTERPOLATION_CLAMPS	(gog_xy_interpolation_clamps_get_type ())
#define GOG_XY_INTERPOLATION_CLAMPS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_XY_INTERPOLATION_CLAMPS, GogXYInterpolationClamps))
#define GOG_IS_XY_INTERPOLATION_CLAMPS(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_XY_INTERPOLATION_CLAMPS))
GType gog_xy_interpolation_clamps_get_type (void);

static GObjectClass *interp_parent_klass;

static void
gog_xy_interpolation_clamps_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = 0;
	*last = 1;
}

static GogDatasetElement *
gog_xy_interpolation_clamps_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogXYInterpolationClamps *clamps = GOG_XY_INTERPOLATION_CLAMPS (set);
	g_return_val_if_fail (2 > dim_i, NULL);
	g_return_val_if_fail (dim_i >= 0, NULL);
	return clamps->derivs + dim_i;
}

static void
gog_xy_interpolation_clamps_dataset_dim_changed (GogDataset *set, int dim_i)
{
	GogXYInterpolationClamps *clamps = GOG_XY_INTERPOLATION_CLAMPS (set);
	clamps->series->clamped_derivs[dim_i] = (GO_IS_DATA ((clamps->derivs + dim_i)->data))?
		go_data_get_scalar_value ((clamps->derivs + dim_i)->data): 0.;
	gog_object_request_update (GOG_OBJECT (clamps->series));
}

static void
gog_xy_interpolation_clamps_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_xy_interpolation_clamps_dataset_get_elem;
	iface->dims	   = gog_xy_interpolation_clamps_dataset_dims;
	iface->dim_changed = gog_xy_interpolation_clamps_dataset_dim_changed;
}

static void
gog_xy_interpolation_clamps_finalize (GObject *obj)
{
	GogXYInterpolationClamps *clamps = GOG_XY_INTERPOLATION_CLAMPS (obj);
	if (clamps->derivs != NULL) {
		gog_dataset_finalize (GOG_DATASET (obj));
		g_free (clamps->derivs);
		clamps->derivs = NULL;
	}
	(*interp_parent_klass->finalize) (obj);
}

static void
gog_xy_interpolation_clamps_class_init (GObjectClass *klass)
{
	interp_parent_klass = g_type_class_peek_parent (klass);
	klass->finalize	    = gog_xy_interpolation_clamps_finalize;
}

static void
gog_xy_interpolation_clamps_init (GogXYInterpolationClamps *clamps)
{
	clamps->derivs = g_new0 (GogDatasetElement, 2);
}

GSF_CLASS_FULL (GogXYInterpolationClamps, gog_xy_interpolation_clamps,
		NULL, NULL, gog_xy_interpolation_clamps_class_init, NULL,
		gog_xy_interpolation_clamps_init, GOG_TYPE_OBJECT, 0,
		GSF_INTERFACE (gog_xy_interpolation_clamps_dataset_init, GOG_TYPE_DATASET))

/****************************************************************************/

typedef GogSeriesClass GogXYSeriesClass;

enum {
	SERIES_PROP_0,
	SERIES_PROP_XERRORS,
	SERIES_PROP_YERRORS,
	SERIES_PROP_INVALID_AS_ZERO,
	SERIES_PROP_CLAMP0,
	SERIES_PROP_CLAMP1
};

static GogStyledObjectClass *series_parent_klass;

static GogDataset *
gog_xy_series_get_interpolation_params (GogSeries const *series)
{
	GogXYSeries *xy = GOG_XY_SERIES (series);
	g_return_val_if_fail (xy, NULL);
	return xy->interpolation_props;
}

static void
gog_xy_series_update (GogObject *obj)
{
	const double *x_vals, *y_vals, *z_vals = NULL;
	GogXYSeries *series = GOG_XY_SERIES (obj);
	unsigned old_num = series->base.num_elements;
	GSList *ptr;

	if (GOG_IS_BUBBLE_PLOT (series->base.plot) || GOG_IS_XY_COLOR_PLOT (series->base.plot))
		series->base.num_elements = gog_series_get_xyz_data (GOG_SERIES (series),
								     &x_vals, &y_vals, &z_vals);
	else
		series->base.num_elements = gog_series_get_xy_data (GOG_SERIES (series),
								    &x_vals, &y_vals);

	/* update children */
	for (ptr = obj->children; ptr != NULL; ptr = ptr->next)
		if (!GOG_IS_SERIES_LINES (ptr->data))
			gog_object_request_update (GOG_OBJECT (ptr->data));

	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));
	if (old_num != series->base.num_elements)
		gog_plot_request_cardinality_update (series->base.plot);

	if (series_parent_klass->base.update)
		series_parent_klass->base.update (obj);
}

static void
gog_xy_series_init (GObject *obj)
{
	GogXYSeries *series = GOG_XY_SERIES (obj);

	GOG_SERIES (series)->fill_type = GOG_SERIES_FILL_TYPE_Y_ORIGIN;
	(GOG_SERIES (series))->acceptable_children = GOG_SERIES_ACCEPT_TREND_LINE;
	series->x_errors = series->y_errors = NULL;
	series->hdroplines = series->vdroplines = NULL;
	series->interpolation_props = g_object_new (GOG_TYPE_XY_INTERPOLATION_CLAMPS, NULL);
	GOG_XY_INTERPOLATION_CLAMPS (series->interpolation_props)->series = series;
	gog_dataset_set_dim (series->interpolation_props, 0, go_data_scalar_val_new (0.), NULL);
	gog_dataset_set_dim (series->interpolation_props, 1, go_data_scalar_val_new (0.), NULL);
	GOG_SERIES (series)->allowed_pos = GOG_SERIES_LABELS_CENTERED | GOG_SERIES_LABELS_LEFT
		| GOG_SERIES_LABELS_RIGHT | GOG_SERIES_LABELS_TOP | GOG_SERIES_LABELS_BOTTOM;
	GOG_SERIES (series)->default_pos = GOG_SERIES_LABELS_TOP;
}

static void
gog_xy_series_finalize (GObject *obj)
{
	GogXYSeries *series = GOG_XY_SERIES (obj);

	if (series->x_errors != NULL) {
		g_object_unref (series->x_errors);
		series->x_errors = NULL;
	}

	if (series->y_errors != NULL) {
		g_object_unref (series->y_errors);
		series->y_errors = NULL;
	}

	if (series->interpolation_props != NULL) {
		g_object_unref (series->interpolation_props);
		series->interpolation_props = NULL;
	}

	G_OBJECT_CLASS (series_parent_klass)->finalize (obj);
}

static void
gog_xy_series_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogSeries *series = GOG_SERIES (gso);

	series_parent_klass->init_style (gso, style);
	if (series->plot == NULL ||
	    GOG_IS_BUBBLE_PLOT (series->plot))
		return;

	if (GOG_IS_XY_PLOT (series->plot)) {
		GogXYPlot const *plot = GOG_XY_PLOT (series->plot);

		if (!plot->default_style_has_markers && style->marker.auto_shape)
			go_marker_set_shape (style->marker.mark, GO_MARKER_NONE);

		if (!plot->default_style_has_lines && style->line.auto_dash)
			style->line.dash_type = GO_LINE_NONE;

		if (!plot->default_style_has_fill && style->fill.auto_type)
			style->fill.type = GO_STYLE_FILL_NONE;

		if (plot->use_splines)
			series->interpolation = GO_LINE_INTERPOLATION_SPLINE;
	} else {
		GogXYColorPlot const *plot = GOG_XY_COLOR_PLOT (series->plot);
		if (!plot->default_style_has_lines && style->line.auto_dash)
			style->line.dash_type = GO_LINE_NONE;

		if (!plot->default_style_has_fill && style->fill.auto_type)
			style->fill.type = GO_STYLE_FILL_NONE;
	}
}

static void
gog_xy_series_set_property (GObject *obj, guint param_id,
			    GValue const *value, GParamSpec *pspec)
{
	GogXYSeries *series=  GOG_XY_SERIES (obj);
	GogErrorBar* bar;

	switch (param_id) {
	case SERIES_PROP_XERRORS :
		bar = g_value_get_object (value);
		if (series->x_errors == bar)
			return;
		if (bar) {
			bar = gog_error_bar_dup (bar);
			bar->series = GOG_SERIES (series);
			bar->dim_i = 0;
			bar->error_i = series->base.plot->desc.series.num_dim - 2;
		}
		if (!series->base.needs_recalc) {
			series->base.needs_recalc = TRUE;
			gog_object_emit_changed (GOG_OBJECT (series), FALSE);
		}
		if (series->x_errors != NULL)
			g_object_unref (series->x_errors);
		series->x_errors = bar;
		break;
	case SERIES_PROP_YERRORS :
		bar = g_value_get_object (value);
		if (series->y_errors == bar)
			return;
		if (bar) {
			bar = gog_error_bar_dup (bar);
			bar->series = GOG_SERIES (series);
			bar->dim_i = 1;
			bar->error_i = series->base.plot->desc.series.num_dim - 4;
		}
		if (!series->base.needs_recalc) {
			series->base.needs_recalc = TRUE;
			gog_object_emit_changed (GOG_OBJECT (series), FALSE);
		}
		if (series->y_errors != NULL)
			g_object_unref (series->y_errors);
		series->y_errors = bar;
		break;
	case SERIES_PROP_INVALID_AS_ZERO:
		series->invalid_as_zero = g_value_get_boolean (value);
		gog_object_request_update (GOG_OBJECT (series));
		break;
	case SERIES_PROP_CLAMP0:
		gog_dataset_set_dim (series->interpolation_props, 0,
				     go_data_scalar_val_new (g_value_get_double (value)), NULL);
		break;
	case SERIES_PROP_CLAMP1:
		gog_dataset_set_dim (series->interpolation_props, 1,
				     go_data_scalar_val_new (g_value_get_double (value)), NULL);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_xy_series_get_property (GObject *obj, guint param_id,
			  GValue *value, GParamSpec *pspec)
{
	GogXYSeries *series=  GOG_XY_SERIES (obj);

	switch (param_id) {
	case SERIES_PROP_XERRORS :
		g_value_set_object (value, series->x_errors);
		break;
	case SERIES_PROP_YERRORS :
		g_value_set_object (value, series->y_errors);
		break;
	case SERIES_PROP_INVALID_AS_ZERO:
		g_value_set_boolean (value, series->invalid_as_zero);
		break;
	case SERIES_PROP_CLAMP0:
		g_value_set_double (value, series->clamped_derivs[0]);
		break;
	case SERIES_PROP_CLAMP1:
		g_value_set_double (value, series->clamped_derivs[1]);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
static void
invalid_toggled_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "invalid-as-zero", gtk_toggle_button_get_active (btn), NULL);
}

static void
gog_xy_series_populate_editor (GogObject *obj,
			       GOEditor *editor,
			       GogDataAllocator *dalloc,
			       GOCmdContext *cc)
{
	GtkWidget *w;
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_xy/gog-xy-series-prefs.ui",
				    GETTEXT_PACKAGE, cc);

	(GOG_OBJECT_CLASS(series_parent_klass)->populate_editor) (obj, editor, dalloc, cc);

	if (gui != NULL) {
		w = go_gtk_builder_get_widget (gui, "invalid_as_zero");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				(GOG_XY_SERIES (obj))->invalid_as_zero);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (invalid_toggled_cb), obj);

		w = go_gtk_builder_get_widget (gui, "gog-xy-series-prefs");

		go_editor_add_page (editor, w, _("Details"));
		g_object_unref (gui);
	}

	w = gog_error_bar_prefs (GOG_SERIES (obj), "x-errors", TRUE, dalloc, cc);
	go_editor_add_page (editor, w, _("X error bars"));
	g_object_unref (w);
	w = gog_error_bar_prefs (GOG_SERIES (obj), "y-errors", FALSE, dalloc, cc);
	go_editor_add_page (editor, w, _("Y error bars"));
	g_object_unref (w);
}
#endif

static void
gog_xy_series_class_init (GogStyledObjectClass *gso_klass)
{
	static GogObjectRole const roles[] = {
		{ N_("Horizontal drop lines"), "GogSeriesLines", 2,
			GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
			horiz_drop_lines_can_add,
			NULL,
			NULL,
			horiz_drop_lines_post_add,
			horiz_drop_lines_pre_remove,
			NULL },
		{ N_("Vertical drop lines"), "GogSeriesLines",	3,
			GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
			vert_drop_lines_can_add,
			NULL,
			NULL,
			vert_drop_lines_post_add,
			vert_drop_lines_pre_remove,
			NULL },
	};
	static GogSeriesFillType const valid_fill_type_list[] = {
		GOG_SERIES_FILL_TYPE_BOTTOM,
		GOG_SERIES_FILL_TYPE_TOP,
		GOG_SERIES_FILL_TYPE_LEFT,
		GOG_SERIES_FILL_TYPE_RIGHT,
		GOG_SERIES_FILL_TYPE_Y_ORIGIN,
		GOG_SERIES_FILL_TYPE_X_ORIGIN,
		GOG_SERIES_FILL_TYPE_SELF,
		GOG_SERIES_FILL_TYPE_NEXT,
		GOG_SERIES_FILL_TYPE_X_AXIS_MIN,
		GOG_SERIES_FILL_TYPE_X_AXIS_MAX,
		GOG_SERIES_FILL_TYPE_Y_AXIS_MIN,
		GOG_SERIES_FILL_TYPE_Y_AXIS_MAX,
		GOG_SERIES_FILL_TYPE_INVALID
	};
	GogObjectClass *gog_klass = (GogObjectClass *)gso_klass;
	GObjectClass *gobject_klass = (GObjectClass *) gso_klass;
	GogSeriesClass *series_klass = (GogSeriesClass *) gso_klass;

	series_parent_klass = g_type_class_peek_parent (gso_klass);
	gog_klass->update	= gog_xy_series_update;
	gog_klass->view_type	= gog_xy_series_view_get_type ();
	gso_klass->init_style	= gog_xy_series_init_style;

	gobject_klass->finalize		= gog_xy_series_finalize;
	gobject_klass->set_property = gog_xy_series_set_property;
	gobject_klass->get_property = gog_xy_series_get_property;
	gog_klass->update		= gog_xy_series_update;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor	= gog_xy_series_populate_editor;
#endif
	gso_klass->init_style		= gog_xy_series_init_style;

	series_klass->has_interpolation = TRUE;
	series_klass->has_fill_type	= TRUE;
	series_klass->series_element_type = GOG_TYPE_XY_SERIES_ELEMENT;
	series_klass->get_interpolation_params = gog_xy_series_get_interpolation_params;

	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));

	g_object_class_install_property (gobject_klass, SERIES_PROP_XERRORS,
		g_param_spec_object ("x-errors",
			_("X error bars"),
			_("GogErrorBar *"),
			GOG_TYPE_ERROR_BAR,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, SERIES_PROP_YERRORS,
		g_param_spec_object ("y-errors",
			_("Y error bars"),
			_("GogErrorBar *"),
			GOG_TYPE_ERROR_BAR,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, SERIES_PROP_INVALID_AS_ZERO,
		g_param_spec_boolean ("invalid-as-zero",
			_("Invalid as zero"),
			_("Replace invalid values by 0 when drawing markers or bubbles"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, SERIES_PROP_CLAMP0,
		g_param_spec_double ("clamp0",
			_("Clamp at start"),
			_("Slope at start of the interpolated curve when using clamped spline interpolation"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, SERIES_PROP_CLAMP1,
		g_param_spec_double ("clamp1",
			_("Clamp at end"),
			_("Slope at end of the interpolated curve when using clamped spline interpolation"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	series_klass->valid_fill_type_list = valid_fill_type_list;
}

GSF_DYNAMIC_CLASS (GogXYSeries, gog_xy_series,
	gog_xy_series_class_init, gog_xy_series_init,
	GOG_TYPE_SERIES)

G_MODULE_EXPORT void
go_plugin_init (GOPlugin *plugin, GOCmdContext *cc)
{
	GTypeModule *module = go_plugin_get_type_module (plugin);
	gog_2d_plot_register_type (module);
	gog_xy_plot_register_type (module);
	gog_bubble_plot_register_type (module);
	gog_xy_color_plot_register_type (module);
	gog_xy_view_register_type (module);
	gog_xy_series_view_register_type (module);
	gog_xy_series_register_type (module);
	gog_xy_series_element_register_type (module);
	gog_xy_dropbar_plot_register_type (module);
	gog_xy_dropbar_view_register_type (module);
	gog_xy_dropbar_series_register_type (module);
	gog_xy_minmax_plot_register_type (module);
	gog_xy_minmax_view_register_type (module);
	gog_xy_minmax_series_register_type (module);

	register_embedded_stuff ();
}

G_MODULE_EXPORT void
go_plugin_shutdown (GOPlugin *plugin, GOCmdContext *cc)
{
	unregister_embedded_stuff ();
}
