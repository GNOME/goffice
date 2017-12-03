/*
 * gog-radar.c
 *
 * Copyright (C) 2004 Michael Devine <mdevine@cs.stanford.edu>
 * Copyright (C) 2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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
#include "gog-radar.h"
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/graph/gog-error-bar.h>
#include <goffice/graph/gog-grid-line.h>
#include <goffice/graph/gog-view.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/data/go-data.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-marker.h>
#include <goffice/utils/go-line.h>
#include <goffice/utils/go-persist.h>
#include <goffice/utils/go-style.h>
#include <goffice/utils/go-styled-object.h>
#include <goffice/app/module-plugin-defs.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#include <string.h>

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#endif

#include "embedded-stuff.c"

typedef struct {
	GogPlotClass	base;
} GogRTPlotClass;

enum {
	PLOT_PROP_0,
	PLOT_PROP_DEFAULT_STYLE_HAS_MARKERS,
	PLOT_PROP_DEFAULT_STYLE_HAS_FILL
};

GOFFICE_PLUGIN_MODULE_HEADER;

typedef struct {
	GogSeries 	   base;
	GogObject 	  *radial_drop_lines;
	GogErrorBar 	  *r_errors;
} GogRTSeries;

typedef struct {
	GogRTSeries        base;
	GogErrorBar 	  *a_errors;
} GogPolarSeries;
typedef GogPolarSeries GogColorPolarSeries;

typedef GogSeriesClass GogRTSeriesClass;
typedef GogRTSeriesClass GogPolarSeriesClass;
typedef GogRTSeriesClass GogColorPolarSeriesClass;

enum {
	SERIES_PROP_0,
	SERIES_PROP_FILL_TYPE
};

#define GOG_TYPE_RT_SERIES	(gog_rt_series_get_type ())
#define GOG_RT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_RT_SERIES, GogRTSeries))
#define GOG_IS_RT_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_RT_SERIES))

static GType gog_rt_series_get_type (void);

#define GOG_TYPE_POLAR_SERIES	(gog_polar_series_get_type ())
#define GOG_POLAR_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_POLAR_SERIES, GogPolarSeries))
#define GOG_IS_POLAR_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_POLAR_SERIES))

static GType gog_polar_series_get_type (void);
static GType gog_color_polar_series_get_type (void);
static GType gog_rt_view_get_type (void);

/*-----------------------------------------------------------------------------
 *
 *  GogRTPlot
 *
 *-----------------------------------------------------------------------------
 */

/*
 *  Accessor for setting GOGRTPlot member variables.
 *
 *  \param obj The rt plot as a GObject.  Must not be NULL.
 */
static void
gog_rt_plot_set_property (GObject *obj, guint param_id,
			      GValue const *value, GParamSpec *pspec)
{
	GogRTPlot *rt = GOG_RT_PLOT (obj);

	switch (param_id) {
	case PLOT_PROP_DEFAULT_STYLE_HAS_MARKERS:
		rt->default_style_has_markers = g_value_get_boolean (value);
		break;
	case PLOT_PROP_DEFAULT_STYLE_HAS_FILL:
		rt->default_style_has_fill = g_value_get_boolean (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
}

/*
 *  Accessor for getting GOGRTPlot member variables.
 */
static void
gog_rt_plot_get_property (GObject *obj, guint param_id,
			      GValue *value, GParamSpec *pspec)
{
	GogRTPlot *rt = GOG_RT_PLOT (obj);

	switch (param_id) {
	case PLOT_PROP_DEFAULT_STYLE_HAS_MARKERS:
		g_value_set_boolean (value, rt->default_style_has_markers);
		break;
	case PLOT_PROP_DEFAULT_STYLE_HAS_FILL:
		g_value_set_boolean (value, rt->default_style_has_fill);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_rt_plot_update (GogObject *obj)
{
	GogRTPlot * model = GOG_RT_PLOT(obj);
	GogRTSeries const *series;
	unsigned num_elements = 0;
	double val_min, val_max, tmp_min, tmp_max;
	GSList *ptr;
	GogErrorBar *errors;
	GogAxis *raxis = model->base.axis[GOG_AXIS_RADIAL];

	val_min =  DBL_MAX;
	val_max = -DBL_MAX;
	for (ptr = model->base.series; ptr != NULL; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;

		if (num_elements < series->base.num_elements)
			num_elements = series->base.num_elements;
		gog_axis_data_get_bounds (raxis, series->base.values[1].data, &tmp_min, &tmp_max);
		if (val_min > tmp_min) val_min = tmp_min;
		if (val_max < tmp_max) val_max = tmp_max;

		errors = series->r_errors;
		if (gog_error_bar_is_visible (errors)) {
			gog_error_bar_get_minmax (errors, &tmp_min, &tmp_max);
			if (val_min > tmp_min)
				val_min = tmp_min;
			if (val_max < tmp_max)
				val_max = tmp_max;
		}
	}
	model->num_elements = num_elements;

	if (model->r.minima != val_min || model->r.maxima != val_max) {
		model->r.minima = val_min;
		model->r.maxima = val_max;
		gog_axis_bound_changed (raxis, GOG_OBJECT (model));
	}

	model->t.minima = 1;
	model->t.maxima = num_elements;

	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_rt_plot_guru_helper (GogPlot *plot, char const *hint)
{
	if (strcmp (hint, "circular-no-line") == 0) {
		GogAxis *axis = gog_plot_get_axis (plot, GOG_AXIS_CIRCULAR);
		GOStyle *style;

		g_return_if_fail (GOG_AXIS (axis) != NULL);

		style = go_styled_object_get_style (GO_STYLED_OBJECT (axis));
		style->line.dash_type = GO_LINE_NONE;
		style->line.auto_dash = FALSE;
	};
}

static void
gog_rt_plot_class_init (GogPlotClass *gog_plot_klass)
{
	GObjectClass   *gobject_klass = (GObjectClass *) gog_plot_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) gog_plot_klass;

	/* Override methods of GObject */
	gobject_klass->set_property = gog_rt_plot_set_property;
	gobject_klass->get_property = gog_rt_plot_get_property;

	/* Fill in GOGObject superclass values */
	gog_object_klass->update	= gog_rt_plot_update;
	gog_object_klass->view_type	= gog_rt_view_get_type ();

	g_object_class_install_property (gobject_klass,
		PLOT_PROP_DEFAULT_STYLE_HAS_MARKERS,
		g_param_spec_boolean ("default-style-has-markers",
			_("Default markers"),
			_("Should the default style of a series include markers"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass,
		PLOT_PROP_DEFAULT_STYLE_HAS_FILL,
		g_param_spec_boolean ("default-style-has-fill",
			_("Default fill"),
			_("Should the default style of a series include fill"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	/* Fill in GogPlotClass methods */
	gog_plot_klass->desc.num_series_max = G_MAXINT;
	gog_plot_klass->series_type = gog_rt_series_get_type();

	gog_plot_klass->axis_set    = GOG_AXIS_SET_RADAR;
	gog_plot_klass->guru_helper = gog_rt_plot_guru_helper;
}

static void
gog_rt_plot_init (GogRTPlot *rt)
{
	rt->base.vary_style_by_element = FALSE;
	rt->default_style_has_markers = FALSE;
	rt->default_style_has_fill = FALSE;
	rt->num_elements = 0;
}

GSF_DYNAMIC_CLASS (GogRTPlot, gog_rt_plot,
	   gog_rt_plot_class_init, gog_rt_plot_init,
	   GOG_TYPE_PLOT)

/*****************************************************************************/

typedef GogRTPlotClass GogRadarPlotClass;

static char const *
gog_radar_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name rt plot objects
	 * eg The 2nd rt plot in a chart will be called
	 * 	PlotRT2 */
	return N_("PlotRadar");
}

static GOData *
gog_radar_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
				GogPlotBoundInfo * bounds)
{
	GSList *ptr;
	GogRTPlot *rt = GOG_RT_PLOT (plot);

	switch (axis) {
	case GOG_AXIS_CIRCULAR:
		bounds->val.minima = rt->t.minima;
		bounds->val.maxima = rt->t.maxima;
		bounds->logical.minima = 0.;
		bounds->logical.maxima = go_nan;
		bounds->is_discrete    = TRUE;
		bounds->center_on_ticks = TRUE;

		for (ptr = plot->series; ptr != NULL ; ptr = ptr->next)
			if (gog_series_is_valid (GOG_SERIES (ptr->data)))
				return GOG_SERIES (ptr->data)->values[0].data;
		break;
	case GOG_AXIS_RADIAL:
		bounds->val.minima = rt->r.minima;
		bounds->val.maxima = rt->r.maxima;
		bounds->logical.maxima = bounds->logical.minima = go_nan;
		bounds->is_discrete = FALSE;
		break;
	default:
		g_warning("[GogRadarPlot::axis_set_bounds] bad axis (%i)", axis);
		break;
	}

	return NULL;
}

static void
gog_radar_plot_class_init (GogPlotClass *gog_plot_klass)
{
	GogObjectClass *gog_object_klass = (GogObjectClass *) gog_plot_klass;

	/* Fill in GOGObject superclass values */
	gog_object_klass->type_name	= gog_radar_plot_type_name;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Labels"), GOG_SERIES_SUGGESTED, TRUE,
			  GOG_DIM_LABEL, GOG_MS_DIM_CATEGORIES },
			{ N_("Values"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
			{ "Magnitude+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus1 },
			{ "Magnitude-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus1 }
		};
		gog_plot_klass->desc.series.dim = dimensions;
		gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		gog_plot_klass->desc.series.style_fields = GO_STYLE_LINE | GO_STYLE_MARKER;
	}

	gog_plot_klass->axis_get_bounds	= gog_radar_plot_axis_get_bounds;
}

GSF_DYNAMIC_CLASS (GogRadarPlot, gog_radar_plot,
	gog_radar_plot_class_init, NULL,
	GOG_TYPE_RT_PLOT)

/*****************************************************************************/

enum {
	PLOT_PROP_FILL_0,
	PLOT_PROP_FILL_BEFORE_GRID,
};

static void
gog_polar_area_set_property (GObject *obj, guint param_id,
		     GValue const *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);
	switch (param_id) {
	case PLOT_PROP_FILL_BEFORE_GRID:
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
gog_polar_area_get_property (GObject *obj, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	GogPlot *plot = GOG_PLOT (obj);

	switch (param_id) {
	case PLOT_PROP_FILL_BEFORE_GRID:
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
gog_polar_area_populate_editor (GogObject *obj,
			     GOEditor *editor,
                             GogDataAllocator *dalloc,
                             GOCmdContext *cc)
{
#ifdef GOFFICE_WITH_GTK
	GogObjectClass *gog_class = (GogObjectClass *) g_type_class_peek_parent (G_OBJECT_GET_CLASS (obj));
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_radar/gog-polar-prefs.ui",
				    GETTEXT_PACKAGE, cc);

	if (gui != NULL) {
		GtkWidget *w = go_gtk_builder_get_widget (gui, "before-grid");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				(GOG_PLOT (obj))->rendering_order == GOG_PLOT_RENDERING_BEFORE_GRID);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (display_before_grid_cb), obj);
		w = go_gtk_builder_get_widget (gui, "gog-polar-prefs");
		go_editor_add_page (editor, w, _("Properties"));
		g_object_unref (gui);
	}

	gog_class->populate_editor (obj, editor, dalloc, cc);
#endif
};

/*****************************************************************************/

#define GOG_TYPE_RADAR_AREA_PLOT  (gog_radar_area_plot_get_type ())
#define GOG_RADAR_AREA_PLOT(o)	  (G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_RADAR_AREA_PLOT, GogRadarAreaPlot))
#define GOG_IS_PLOT_RADAR_AREA(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_RADAR_AREA_PLOT))

typedef GogRadarPlot		GogRadarAreaPlot;
typedef GogRadarPlotClass	GogRadarAreaPlotClass;

GType gog_radar_area_plot_get_type (void);

static char const *
gog_radar_area_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name bar/col plot objects
	 * eg The 2nd line plot in a chart will be called
	 * 	PlotRadarArea2
	 */
	return N_("PlotRadarArea");
}
static void
gog_radar_area_plot_class_init (GObjectClass *obj_class)
{
	GogObjectClass *gog_klass = (GogObjectClass *) obj_class;
	GogPlotClass *plot_klass = (GogPlotClass *) obj_class;

	obj_class->get_property = gog_polar_area_get_property;
	obj_class->set_property = gog_polar_area_set_property;
	g_object_class_install_property (obj_class, PLOT_PROP_FILL_BEFORE_GRID,
		g_param_spec_boolean ("before-grid",
			_("Displayed under the grids"),
			_("Should the plot be displayed before the grids"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	plot_klass->desc.series.style_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;

	gog_klass->populate_editor = gog_polar_area_populate_editor;
	gog_klass->type_name	= gog_radar_area_plot_type_name;
}

static void
gog_radar_area_plot_init (GogPlot *plot)
{
	GOG_RT_PLOT(plot)->default_style_has_fill = TRUE;
	plot->rendering_order = GOG_PLOT_RENDERING_BEFORE_AXIS;
}

GSF_DYNAMIC_CLASS (GogRadarAreaPlot, gog_radar_area_plot,
	gog_radar_area_plot_class_init, gog_radar_area_plot_init,
	GOG_TYPE_RADAR_PLOT)

/*****************************************************************************/

typedef GogRTPlotClass GogPolarPlotClass;

static char const *
gog_polar_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name rt plot objects
	 * eg The 2nd rt plot in a chart will be called
	 * 	PlotPolar2 */
	return N_("PlotPolar");
}

static GOData *
gog_polar_plot_axis_get_bounds (GogPlot *plot, GogAxisType atype,
				GogPlotBoundInfo * bounds)
{
	GogRTPlot *rt = GOG_RT_PLOT (plot);
	GogAxis *axis = gog_plot_get_axis (plot, atype);

	switch (atype) {
	case GOG_AXIS_CIRCULAR:
		bounds->val.minima = bounds->logical.minima = -G_MAXDOUBLE;
		bounds->val.maxima = bounds->logical.maxima = +G_MAXDOUBLE;
		bounds->is_discrete = FALSE;
		break;
	case GOG_AXIS_RADIAL:
		bounds->val.minima = bounds->logical.minima =
			gog_axis_is_zero_important (axis)
			? 0.0
			: rt->r.minima;
		bounds->val.maxima = rt->r.maxima;
		bounds->logical.maxima = go_nan;
		bounds->is_discrete = FALSE;
		break;
	default:
		g_warning("[GogPolarPlot::axis_set_bounds] bad axis (%i)", atype);
		break;
	}

	return NULL;
}

static void
gog_polar_plot_class_init (GObjectClass *obj_class)
{
	GogObjectClass *gog_object_klass = (GogObjectClass *) obj_class;
	GogPlotClass *gog_plot_klass = (GogPlotClass *) obj_class;

	obj_class->get_property = gog_polar_area_get_property;
	obj_class->set_property = gog_polar_area_set_property;
	g_object_class_install_property (obj_class, PLOT_PROP_FILL_BEFORE_GRID,
		g_param_spec_boolean ("before-grid",
			_("Displayed under the grids"),
			_("Should the plot be displayed before the grids"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	/* Fill in GOGObject superclass values */
	gog_object_klass->type_name	= gog_polar_plot_type_name;
	gog_object_klass->populate_editor = gog_polar_area_populate_editor;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Angle"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("Magnitude"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
/* Names of the error data are not translated since they are not used */
			{ "Angle+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus2 },
			{ "Angle-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus2 },
			{ "Magnitude+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus1 },
			{ "Magnitude-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus1 }
		};
		gog_plot_klass->desc.series.dim = dimensions;
		gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		gog_plot_klass->desc.series.style_fields = GO_STYLE_LINE
			| GO_STYLE_FILL
			| GO_STYLE_MARKER
			| GO_STYLE_INTERPOLATION;
	}

	gog_plot_klass->series_type = gog_polar_series_get_type ();
	gog_plot_klass->axis_get_bounds	= gog_polar_plot_axis_get_bounds;
}

GSF_DYNAMIC_CLASS (GogPolarPlot, gog_polar_plot,
	gog_polar_plot_class_init, NULL,
	GOG_TYPE_RT_PLOT)

/*****************************************************************************/

typedef GogPolarPlotClass GogColorPolarPlotClass;

static GogObjectClass *color_parent_klass;

enum {
	GOG_COLOR_POLAR_PROP_0,
	GOG_COLOR_POLAR_PROP_HIDE_OUTLIERS,
};

#ifdef GOFFICE_WITH_GTK
static void
hide_outliers_toggled_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "hide-outliers", gtk_toggle_button_get_active (btn), NULL);
}
#endif

static void
gog_color_polar_plot_populate_editor (GogObject *obj,
				   GOEditor *editor,
				   GogDataAllocator *dalloc,
				   GOCmdContext *cc)
{
#ifdef GOFFICE_WITH_GTK
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:plot_radar/gog-color-polar-prefs.ui",
				    GETTEXT_PACKAGE, cc);
	if (gui != NULL) {
		GtkWidget *w = go_gtk_builder_get_widget (gui, "hide-outliers");
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
				(GOG_COLOR_POLAR_PLOT (obj))->hide_outliers);
		g_signal_connect (G_OBJECT (w),
			"toggled",
			G_CALLBACK (hide_outliers_toggled_cb), obj);
		w = go_gtk_builder_get_widget (gui, "gog-color-polar-prefs");
		go_editor_add_page (editor, w, _("Properties"));
		g_object_unref (gui);
	}

#endif
	(GOG_OBJECT_CLASS (color_parent_klass)->populate_editor) (obj, editor, dalloc, cc);
}

static void
gog_color_polar_plot_set_property (GObject *obj, guint param_id,
		     GValue const *value, GParamSpec *pspec)
{
	GogColorPolarPlot *plot = GOG_COLOR_POLAR_PLOT (obj);
	switch (param_id) {
	case GOG_COLOR_POLAR_PROP_HIDE_OUTLIERS:
		plot->hide_outliers = g_value_get_boolean (value);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}

	/* none of the attributes triggers a size change yet.
	 * When we add data labels we'll need it */
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_color_polar_plot_get_property (GObject *obj, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	GogColorPolarPlot *plot = GOG_COLOR_POLAR_PLOT (obj);
	switch (param_id) {
	case GOG_COLOR_POLAR_PROP_HIDE_OUTLIERS:
		g_value_set_boolean (value, plot->hide_outliers);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static char const *
gog_color_polar_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name rt plot objects
	 * eg The 2nd rt plot in a chart will be called
	 * 	PlotColoredPolar2 */
	return N_("PlotColorPolar");
}

static void
gog_color_polar_plot_update (GogObject *obj)
{
	GogColorPolarPlot *model = GOG_COLOR_POLAR_PLOT (obj);
	GogSeries const *series = NULL;
	double z_min, z_max, tmp_min, tmp_max;
	GSList *ptr;

	z_min = DBL_MAX;
	z_max = -DBL_MAX;
	for (ptr = model->base.base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;

		go_data_get_bounds (series->values[2].data, &tmp_min, &tmp_max);
		if (z_min > tmp_min) z_min = tmp_min;
		if (z_max < tmp_max) z_max = tmp_max;
	}
	if (model->z.minima != z_min || model->z.maxima != z_max) {
		model->z.minima = z_min;
		model->z.maxima = z_max;
		gog_axis_bound_changed (model->base.base.axis[GOG_AXIS_COLOR], GOG_OBJECT (model));
	}
	color_parent_klass->update (obj);
}

static GOData *
gog_color_polar_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
				GogPlotBoundInfo * bounds)
{
	GogRTPlot *rt = GOG_RT_PLOT (plot);

	switch (axis) {
	case GOG_AXIS_CIRCULAR:
		bounds->val.minima = bounds->logical.minima= -G_MAXDOUBLE;
		bounds->val.maxima = bounds->logical.maxima=  G_MAXDOUBLE;
		bounds->is_discrete    = FALSE;
		break;
	case GOG_AXIS_RADIAL:
		bounds->val.minima = bounds->logical.minima = 0.;
		bounds->val.maxima = rt->r.maxima;
		bounds->logical.maxima = go_nan;
		bounds->is_discrete = FALSE;
		break;
	case GOG_AXIS_COLOR: {
		GogColorPolarPlot *model = GOG_COLOR_POLAR_PLOT (plot);

		bounds->val.minima = model->z.minima;
		bounds->val.maxima = model->z.maxima;
		bounds->is_discrete = model->z.minima > model->z.maxima ||
			!go_finite (model->z.minima) ||
			!go_finite (model->z.maxima);
		return NULL;
	}
	default:
		g_warning("[GogColorPolarPlot::axis_set_bounds] bad axis (%i)", axis);
		break;
	}

	return NULL;
}

static void
gog_color_polar_plot_class_init (GogPlotClass *gog_plot_klass)
{
	GogObjectClass *gog_object_klass = (GogObjectClass *) gog_plot_klass;
	GObjectClass *gobject_klass = (GObjectClass *) gog_plot_klass;

	color_parent_klass = g_type_class_peek_parent (gog_plot_klass);
	gog_object_klass->update = gog_color_polar_plot_update;
	gog_object_klass->populate_editor = gog_color_polar_plot_populate_editor;

	gobject_klass->set_property = gog_color_polar_plot_set_property;
	gobject_klass->get_property = gog_color_polar_plot_get_property;
	g_object_class_install_property (gobject_klass, GOG_COLOR_POLAR_PROP_HIDE_OUTLIERS,
		g_param_spec_boolean  ("hide-outliers",
			_("hide-outliers"),
			_("Hide data outside of the color axis bounds"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	/* Fill in GOGObject superclass values */
	gog_object_klass->type_name	= gog_color_polar_plot_type_name;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("Angle"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_INDEX, GOG_MS_DIM_CATEGORIES },
			{ N_("Magnitude"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
			{ N_("Z"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_EXTRA1 },
			{ "Angle+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus2 },
			{ "Angle-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus2 },
			{ "Magnitude+err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_plus1 },
			{ "Magnitude-err", GOG_SERIES_ERRORS, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_ERR_minus1 }
		};
		gog_plot_klass->desc.series.dim = dimensions;
		gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		gog_plot_klass->desc.series.style_fields = GO_STYLE_LINE
			| GO_STYLE_MARKER
			| GO_STYLE_INTERPOLATION
			| GO_STYLE_MARKER_NO_COLOR;
	}

	gog_plot_klass->series_type     = gog_color_polar_series_get_type();
	gog_plot_klass->axis_get_bounds	= gog_color_polar_plot_axis_get_bounds;
	gog_plot_klass->axis_set	= GOG_AXIS_SET_RADAR | (1 << GOG_AXIS_COLOR);
}

static void
gog_color_polar_plot_init (GogColorPolarPlot *plot)
{
	plot->hide_outliers = TRUE;
}

GSF_DYNAMIC_CLASS (GogColorPolarPlot, gog_color_polar_plot,
	gog_color_polar_plot_class_init, gog_color_polar_plot_init,
	GOG_TYPE_POLAR_PLOT)

/*****************************************************************************/

typedef GogPlotView		GogRTView;
typedef GogPlotViewClass	GogRTViewClass;

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

static void
gog_rt_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogRTPlot const *model = GOG_RT_PLOT (view->model);
	GogAxis *r_axis, *c_axis;
	GogChart *chart = GOG_CHART (view->model->parent);
	GogAxisMap *r_map, *c_map, *z_map = NULL;
	GogChartMap *chart_map;
	GogChartMapPolarData *parms;
	GogViewAllocation const *area;
	GOPath *next_path = NULL;
	GSList   *ptr;
	double theta_min, theta_max, theta = 0;
	double rho_min, rho_max, rho;
	gboolean const is_polar = GOG_IS_PLOT_POLAR (model);
	gboolean is_map = GOG_IS_PLOT_COLOR_POLAR (model), hide_outliers = TRUE;
	double errmin, errmax;

	GogSeriesElement *gse;
	GList const *overrides;

	r_axis = GOG_PLOT (model)->axis[GOG_AXIS_RADIAL];
	c_axis = GOG_PLOT (model)->axis[GOG_AXIS_CIRCULAR];
	g_return_if_fail (r_axis != NULL && c_axis != NULL);

	area = gog_chart_view_get_plot_area (view->parent);
	chart_map = gog_chart_map_new (chart, area, c_axis, r_axis, NULL, FALSE);
	if (!gog_chart_map_is_valid (chart_map)) {
		gog_chart_map_free (chart_map);
		return;
	}
	c_map = gog_chart_map_get_axis_map (chart_map, 0);
	r_map = gog_chart_map_get_axis_map (chart_map, 1);
	parms = gog_chart_map_get_polar_parms (chart_map);

	if (is_map) {
		z_map = gog_axis_map_new (GOG_PLOT (model)->axis[GOG_AXIS_COLOR], 0, 6);
		hide_outliers = GOG_COLOR_POLAR_PLOT (model)->hide_outliers;
	}

	gog_axis_map_get_bounds (c_map, &theta_min, &theta_max);
	gog_axis_map_get_bounds (r_map, &rho_min, &rho_max);
	/* convert theta value to radians */
	theta_min = gog_axis_map_to_view (c_map, theta_min);
	theta_max = gog_axis_map_to_view (c_map, theta_max);
	if (theta_min > theta_max) {
		/* angles may be inverted */
		theta = theta_min;
		theta_min = theta_max;
		theta_max = theta;
	}

	for (ptr = model->base.series; ptr != NULL; ptr = ptr->next) {

		GogRTSeries *series = GOG_RT_SERIES (ptr->data);
		GOStyle *style, *color_style = NULL, *estyle = NULL;
		GOPath *path;
		unsigned count;
		double *r_vals, *c_vals, *z_vals = NULL;
		double x, y, z = 0;

		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;

		style = GOG_STYLED_OBJECT (series)->style;

		gog_renderer_push_style (view->renderer, style);

		r_vals = go_data_get_values (series->base.values[1].data);
		c_vals = is_polar ?  go_data_get_values (series->base.values[0].data) : NULL;
		if (is_map) {
			z_vals = go_data_get_values (series->base.values[2].data);
			color_style = go_style_dup (style);
		}

		if (is_polar) {
			GOPath *clip_path;

			clip_path = go_path_new ();
			go_path_ring_wedge (clip_path, parms->cx, parms->cy, parms->rx, parms->ry,
					    0.0, 0.0, -parms->th0, -parms->th1);
			gog_renderer_push_clip (view->renderer, clip_path);
			go_path_free (clip_path);
		}

		if (next_path != NULL)
			path = next_path;
		else
			path = gog_chart_map_make_path (chart_map, c_vals,r_vals, series->base.num_elements,
							series->base.interpolation,
							series->base.interpolation_skip_invalid, NULL);

		next_path = NULL;

		if (path) {
			if (!is_polar) {
				go_path_close (path);
				gog_renderer_fill_serie (view->renderer, path, NULL);
			} else {
				if (series->base.interpolation == GO_LINE_INTERPOLATION_CLOSED_SPLINE)
					gog_renderer_fill_serie	(view->renderer, path, NULL);
				else if (series->base.fill_type != GOG_SERIES_FILL_TYPE_NEXT) {
					GOPath *close_path;

					close_path = gog_chart_map_make_close_path (chart_map, c_vals, r_vals,
										    series->base.num_elements,
										    series->base.fill_type);
					gog_renderer_fill_serie (view->renderer, path, close_path);
					if (close_path != NULL)
						go_path_free (close_path);
				} else {
					if (ptr->next != NULL) {
						GogRTSeries *next_series;

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
								 series->base.interpolation_skip_invalid, NULL);

						}
					}
					gog_renderer_fill_serie (view->renderer, path, next_path);
				}
			}
		}

		if (series->radial_drop_lines != NULL) {
			GOPath *drop_path;
			double theta, rho;
			unsigned int i;

			gog_renderer_push_style (view->renderer,
				go_styled_object_get_style (GO_STYLED_OBJECT (series->radial_drop_lines)));
			drop_path = go_path_new ();
			for (i = 0; i < series->base.num_elements; i++) {
				gog_chart_map_2D_to_view (chart_map, ((c_vals != NULL) ? c_vals[i] : i+1),
							  r_vals[i], &theta, &rho);
				if (   go_finite (theta)
				    && go_finite (rho)
				    && r_vals[i] > rho_min
				    && r_vals[i] <= rho_max) {
					go_path_move_to (drop_path, parms->cx, parms->cy);
					go_path_line_to (drop_path, theta, rho);
				}
			}
			gog_renderer_stroke_serie (view->renderer, drop_path);
			go_path_free (drop_path);
			gog_renderer_pop_style (view->renderer);
		}

		if (path) {
			gog_renderer_stroke_serie (view->renderer, path);
			go_path_free (path);
		}

		/* draw error bars before markers and after anything else */
		for (count = 0; count < series->base.num_elements; count++)
			if (gog_error_bar_is_visible (series->r_errors)) {
				GogErrorBar const *bar = series->r_errors;
				if (gog_error_bar_get_bounds (bar, count, &errmin, &errmax)) {
					gog_error_bar_render (bar, view->renderer,
							      chart_map,
							      ((c_vals != NULL) ? c_vals[count] : count + 1), r_vals[count],
							      errmin, errmax,
							      GOG_ERROR_BAR_DIRECTION_RADIAL);
					 }
				}
		if (GOG_IS_POLAR_SERIES (series)) {
			GogPolarSeries *polar_series = GOG_POLAR_SERIES (series);
			for (count = 0; count < series->base.num_elements; count++)
				if (gog_error_bar_is_visible (polar_series->a_errors)) {
					GogErrorBar const *bar = polar_series->a_errors;
					if (gog_error_bar_get_bounds (bar, count, &errmin, &errmax)) {
						gog_error_bar_render (bar, view->renderer,
								      chart_map,
								      c_vals[count], r_vals[count],
								      errmin, errmax,
								      GOG_ERROR_BAR_DIRECTION_ANGULAR);
					 }
				}
		}

		if (is_polar)
			gog_renderer_pop_clip (view->renderer);

		overrides = gog_series_get_overrides (GOG_SERIES (series));
		if (go_style_is_marker_visible (style)) {
			for (count = 0; count < series->base.num_elements; count++) {
				rho = (!is_polar || (go_add_epsilon (r_vals[count] - rho_min) >= 0.0)) ?
					r_vals[count] : rho_min;
				if (is_map) {
					z = *z_vals++;
					if (isnan (z) || !go_finite (z))
						continue;
				}
				gog_chart_map_2D_to_view (chart_map,
							  is_polar ? c_vals[count] : count + 1, rho,
							  &x, &y);

				if (is_polar)
					theta = gog_axis_map_to_view (c_map, c_vals[count]);

				if ( !is_polar ||
				     (go_add_epsilon (r_vals[count] - rho_min) >= 0.0 &&
				      go_add_epsilon (rho_max - r_vals[count]) >= 0.0 &&
				      go_add_epsilon ((theta_max - theta_min)
						      - fmod (theta_max - theta, 2 * M_PI)) >= 0.0 &&
				      go_add_epsilon ((theta_max - theta_min)
						      - fmod (theta - theta_min, 2 * M_PI)) >= 0.0)) {
					gse = NULL;
					while (overrides && GOG_SERIES_ELEMENT (overrides->data)->index < count)
						overrides = overrides->next;
					if (overrides && GOG_SERIES_ELEMENT (overrides->data)->index == count) {
						gse = GOG_SERIES_ELEMENT (overrides->data);
						overrides = overrides->next;
						if (!is_map) {
							style = go_styled_object_get_style (GO_STYLED_OBJECT (gse));
							gog_renderer_push_style (view->renderer, style);
						}
					}
					if (is_map) {
						GOColor color = (gog_axis_map_finite (z_map, z))?
							get_map_color (gog_axis_map_to_view (z_map, z), hide_outliers):
							0;
						if (gse) {
							estyle = go_style_dup (go_styled_object_get_style (GO_STYLED_OBJECT (gse)));
							go_marker_set_fill_color (estyle->marker.mark, color);
							gog_renderer_push_style (view->renderer, estyle);							go_marker_set_outline_color (estyle->marker.mark, color);
							gog_renderer_draw_marker (view->renderer, x, y);
						} else {
							go_marker_set_fill_color (color_style->marker.mark, color);
							go_marker_set_outline_color (color_style->marker.mark, color);
							gog_renderer_push_style (view->renderer, color_style);
							gog_renderer_draw_marker (view->renderer, x, y);
							gog_renderer_pop_style (view->renderer);
						}
					} else
						gog_renderer_draw_marker (view->renderer, x, y);
					if (gse)
						gog_renderer_pop_style (view->renderer);
					if (estyle) {
						g_object_unref (estyle);
						estyle = NULL;
					}
				}
			}
		} else if (overrides) {
			while (overrides) {
				count = GOG_SERIES_ELEMENT (overrides->data)->index;
				if (count >= series->base.num_elements) {
					g_warning ("Invalid series element index");
					overrides = overrides->next;
					continue;
				}
				gse = GOG_SERIES_ELEMENT (overrides->data);
				rho = (!is_polar || (go_add_epsilon (r_vals[count] - rho_min) >= 0.0)) ?
					r_vals[count] : rho_min;
				gog_chart_map_2D_to_view (chart_map,
							  is_polar ? c_vals[count] : count + 1, rho,
							  &x, &y);
				if ( !is_polar ||
				     (go_add_epsilon (r_vals[count] - rho_min) >= 0.0 &&
				      go_add_epsilon (rho_max - r_vals[count]) >= 0.0 &&
				      go_add_epsilon ((theta_max - theta_min)
						      - fmod (theta_max - theta, 2 * M_PI)) >= 0.0 &&
				      go_add_epsilon ((theta_max - theta_min)
						      - fmod (theta - theta_min, 2 * M_PI)) >= 0.0)) {
					if (is_map) {
						GOColor color = (gog_axis_map_finite (z_map, z_vals[count]))?
							get_map_color (gog_axis_map_to_view (z_map, z_vals[count]), hide_outliers):
							0;
						estyle = go_style_dup (go_styled_object_get_style (GO_STYLED_OBJECT (gse)));
						go_marker_set_fill_color (estyle->marker.mark, color);
						go_marker_set_outline_color (estyle->marker.mark, color);
						gog_renderer_push_style (view->renderer, estyle);
						gog_renderer_draw_marker (view->renderer, x, y);
						gog_renderer_pop_style (view->renderer);
						g_object_unref (estyle);
					} else {
						style = go_styled_object_get_style (GO_STYLED_OBJECT (gse));
						gog_renderer_push_style (view->renderer, style);
						gog_renderer_draw_marker (view->renderer, x, y);
						gog_renderer_pop_style (view->renderer);
					}
				}
				overrides = overrides->next;
			}
			estyle = NULL;
		}

		gog_renderer_pop_style (view->renderer);
		if (is_map)
			g_object_unref (color_style);
	}
	gog_chart_map_free (chart_map);
	if (is_map)
		gog_axis_map_free (z_map);

	if (next_path != NULL)
		go_path_free (next_path);
}

static void
gog_rt_view_class_init (GogViewClass *view_klass)
{
	view_klass->render	  = gog_rt_view_render;
	view_klass->clip	  = FALSE;
}

GSF_DYNAMIC_CLASS (GogRTView, gog_rt_view,
	gog_rt_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)

/*****************************************************************************/

static gboolean
radial_drop_lines_can_add (GogObject const *parent)
{
	GogRTSeries *series = GOG_RT_SERIES (parent);
	return (   series->radial_drop_lines == NULL
		&& GOG_IS_PLOT_POLAR (gog_series_get_plot (GOG_SERIES (parent))));
}

static void
radial_drop_lines_post_add (GogObject *parent, GogObject *child)
{
	GogRTSeries *series = GOG_RT_SERIES (parent);
	series->radial_drop_lines = child;
	gog_object_request_update (child);
}

static void
radial_drop_lines_pre_remove (GogObject *parent, GogObject *child)
{
	GogRTSeries *series = GOG_RT_SERIES (parent);
	series->radial_drop_lines = NULL;
}
/****************************************************************************/

typedef GogSeriesElement GogRTSeriesElement;
typedef GogSeriesElementClass GogRTSeriesElementClass;
#define GOG_TYPE_RT_SERIES_ELEMENT	(gog_rt_series_element_get_type ())
#define GOG_RT_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPERT_SERIES_ELEMENT, GogRTSeriesElement))
#define GOG_IS_RT_SERIES_ELEMENT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_RT_SERIES_ELEMENT))
GType gog_rt_series_element_get_type (void);

static void
gog_rt_series_element_init_style (GogStyledObject *gso, GOStyle *style)
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
gog_rt_series_element_class_init (GogRTSeriesElementClass *klass)
{
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) klass;
	style_klass->init_style	    	= gog_rt_series_element_init_style;
}

GSF_DYNAMIC_CLASS (GogRTSeriesElement, gog_rt_series_element,
	gog_rt_series_element_class_init, NULL,
	GOG_TYPE_SERIES_ELEMENT)


/*****************************************************************************/

static GogStyledObjectClass *series_parent_klass;

enum {
	RT_SERIES_PROP_0,
	RT_SERIES_PROP_RERRORS,
};

static void
gog_rt_series_update (GogObject *obj)
{
	GogRTSeries *series = GOG_RT_SERIES (obj);
	unsigned old_num = series->base.num_elements;
	unsigned len = 0;

	if (series->base.values[1].data != NULL) {
		//vals = go_data_get_values (series->base.values[1].data);
		len = go_data_get_vector_size (series->base.values[1].data);
	}
	if (GOG_IS_POLAR_SERIES (obj) && series->base.values[0].data != NULL)
	{
		unsigned alen = go_data_get_vector_size (series->base.values[0].data);
		if (alen < len)
			len = alen;
	}
	series->base.num_elements = len;

	/* queue plot and axis for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));
	if (old_num != len)
		gog_object_request_update (GOG_OBJECT (series->base.plot->axis[GOG_AXIS_CIRCULAR]));

	if (old_num != series->base.num_elements)
		gog_plot_request_cardinality_update (series->base.plot);

	if (((GogObjectClass *)series_parent_klass)->update)
		((GogObjectClass *)series_parent_klass)->update(obj);
}

static void
gog_rt_series_init (GObject *obj)
{
	GogSeries *series = GOG_SERIES (obj);
	GogRTSeries *rt_series = GOG_RT_SERIES (obj);

	series->fill_type = GOG_SERIES_FILL_TYPE_ORIGIN;
	rt_series->radial_drop_lines = NULL;
}

static void
gog_rt_series_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogSeries *series = GOG_SERIES (gso);
	GogRTPlot const *plot;

	series_parent_klass->init_style (gso, style);
	if (series->plot == NULL)
		return;

	plot = GOG_RT_PLOT (series->plot);

	if (!plot->default_style_has_markers && style->marker.auto_shape)
		go_marker_set_shape (style->marker.mark, GO_MARKER_NONE);

	if (!plot->default_style_has_fill && style->fill.auto_type)
		style->fill.type = GO_STYLE_FILL_NONE;
}

static void
gog_rt_series_set_property (GObject *obj, guint param_id,
			    GValue const *value, GParamSpec *pspec)
{
	GogRTSeries *series=  GOG_RT_SERIES (obj);
	GogErrorBar* bar;

	switch (param_id) {
	case RT_SERIES_PROP_RERRORS :
		bar = g_value_get_object (value);
		if (series->r_errors == bar)
			return;
		if (bar) {
			bar = gog_error_bar_dup (bar);
			bar->series = GOG_SERIES (series);
			bar->dim_i = 1;
			bar->error_i = series->base.plot->desc.series.num_dim - 2;
		}
		if (!series->base.needs_recalc) {
			series->base.needs_recalc = TRUE;
			gog_object_emit_changed (GOG_OBJECT (series), FALSE);
		}
		if (series->r_errors != NULL)
			g_object_unref (series->r_errors);
		series->r_errors = bar;
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_rt_series_get_property (GObject *obj, guint param_id,
			  GValue *value, GParamSpec *pspec)
{
	GogRTSeries *series=  GOG_RT_SERIES (obj);

	switch (param_id) {
	case RT_SERIES_PROP_RERRORS :
		g_value_set_object (value, series->r_errors);
		break;
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_rt_series_finalize (GObject *obj)
{
	GogRTSeries *series = GOG_RT_SERIES (obj);

	if (series->r_errors != NULL) {
		g_object_unref (series->r_errors);
		series->r_errors = NULL;
	}

	G_OBJECT_CLASS (series_parent_klass)->finalize (obj);
}

#ifdef GOFFICE_WITH_GTK
static void
gog_rt_series_populate_editor (GogObject *obj,
			       GOEditor *editor,
			       GogDataAllocator *dalloc,
			       GOCmdContext *cc)
{
	GtkWidget *w;
	(GOG_OBJECT_CLASS(series_parent_klass)->populate_editor) (obj, editor, dalloc, cc);

	w = gog_error_bar_prefs (GOG_SERIES (obj), "r-errors", GOG_ERROR_BAR_DIRECTION_RADIAL, dalloc, cc);
	go_editor_add_page (editor, w, _("Radial error bars"));
}
#endif

static void
gog_rt_series_class_init (GogStyledObjectClass *gso_klass)
{
	static GogObjectRole const roles[] = {
		{ N_("Radial drop lines"), "GogSeriesLines", 2,
			GOG_POSITION_SPECIAL, GOG_POSITION_SPECIAL, GOG_OBJECT_NAME_BY_ROLE,
			radial_drop_lines_can_add,
			NULL,
			NULL,
			radial_drop_lines_post_add,
			radial_drop_lines_pre_remove,
			NULL }
	};

	GObjectClass *gobject_klass = G_OBJECT_CLASS (gso_klass);
	GogObjectClass *obj_klass = GOG_OBJECT_CLASS (gso_klass);
	GogSeriesClass *series_klass = GOG_SERIES_CLASS (gso_klass);

	series_parent_klass = 	g_type_class_peek_parent (gso_klass);
	gso_klass->init_style =	gog_rt_series_init_style;
	gobject_klass->finalize		= gog_rt_series_finalize;
	gobject_klass->set_property = gog_rt_series_set_property;
	gobject_klass->get_property = gog_rt_series_get_property;
	obj_klass->update = gog_rt_series_update;
#ifdef GOFFICE_WITH_GTK
	obj_klass->populate_editor = gog_rt_series_populate_editor;
#endif
	g_object_class_install_property (gobject_klass, RT_SERIES_PROP_RERRORS,
		g_param_spec_object ("r-errors",
			_("Radial error bars"),
			_("GogErrorBar *"),
			GOG_TYPE_ERROR_BAR,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	series_klass->has_interpolation = TRUE;
	series_klass->series_element_type = GOG_TYPE_RT_SERIES_ELEMENT;

	gog_object_register_roles (obj_klass, roles, G_N_ELEMENTS (roles));
}

GSF_DYNAMIC_CLASS (GogRTSeries, gog_rt_series,
	gog_rt_series_class_init, gog_rt_series_init,
	GOG_TYPE_SERIES)

/*****************************************************************************/

static GObjectClass *polar_series_parent_klass;

enum {
	POLAR_SERIES_PROP_0,
	POLAR_SERIES_PROP_AERRORS,
};

/*****************************************************************************/

static void
gog_polar_series_set_property (GObject *obj, guint param_id,
			    GValue const *value, GParamSpec *pspec)
{
	GogPolarSeries *series=  GOG_POLAR_SERIES (obj);
	GogErrorBar* bar;

	switch (param_id) {
	case POLAR_SERIES_PROP_AERRORS :
		bar = g_value_get_object (value);
		if (series->a_errors == bar)
			return;
		if (bar) {
			bar = gog_error_bar_dup (bar);
			bar->series = GOG_SERIES (series);
			bar->dim_i = 0;
			bar->error_i = series->base.base.plot->desc.series.num_dim - 4;
		}
		if (!series->base.base.needs_recalc) {
			series->base.base.needs_recalc = TRUE;
			gog_object_emit_changed (GOG_OBJECT (series), FALSE);
		}
		if (series->a_errors != NULL)
			g_object_unref (series->a_errors);
		series->a_errors = bar;
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_polar_series_get_property (GObject *obj, guint param_id,
			  GValue *value, GParamSpec *pspec)
{
	GogPolarSeries *series=  GOG_POLAR_SERIES (obj);

	switch (param_id) {
	case POLAR_SERIES_PROP_AERRORS :
		g_value_set_object (value, series->a_errors);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
static void
gog_polar_series_populate_editor (GogObject *obj,
			       GOEditor *editor,
			       GogDataAllocator *dalloc,
			       GOCmdContext *cc)
{
	GtkWidget *w;
	(GOG_OBJECT_CLASS(polar_series_parent_klass)->populate_editor) (obj, editor, dalloc, cc);

	w = gog_error_bar_prefs (GOG_SERIES (obj), "a-errors", GOG_ERROR_BAR_DIRECTION_ANGULAR, dalloc, cc);
	go_editor_add_page (editor, w, _("Angular error bars"));
}
#endif

static void
gog_polar_series_finalize (GObject *obj)
{
	GogPolarSeries *series = GOG_POLAR_SERIES (obj);

	if (series->a_errors != NULL) {
		g_object_unref (series->a_errors);
		series->a_errors = NULL;
	}

	G_OBJECT_CLASS (polar_series_parent_klass)->finalize (obj);
}

static void
gog_polar_series_class_init (GogObjectClass *gog_klass)
{
	static GogSeriesFillType const valid_fill_type_list[] = {
		GOG_SERIES_FILL_TYPE_CENTER,
		GOG_SERIES_FILL_TYPE_EDGE,
		GOG_SERIES_FILL_TYPE_ORIGIN,
		GOG_SERIES_FILL_TYPE_SELF,
		GOG_SERIES_FILL_TYPE_NEXT,
		GOG_SERIES_FILL_TYPE_INVALID
	};

	GObjectClass *gobject_klass = G_OBJECT_CLASS (gog_klass);
	GogSeriesClass *series_klass = GOG_SERIES_CLASS (gog_klass);

	polar_series_parent_klass = 	g_type_class_peek_parent (gog_klass);
	series_klass->has_fill_type =		TRUE;
	series_klass->valid_fill_type_list = 	valid_fill_type_list;
	gobject_klass->finalize		= gog_polar_series_finalize;
	gobject_klass->set_property = gog_polar_series_set_property;
	gobject_klass->get_property = gog_polar_series_get_property;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_polar_series_populate_editor;
#endif
	g_object_class_install_property (gobject_klass, POLAR_SERIES_PROP_AERRORS,
		g_param_spec_object ("a-errors",
			_("Angular error bars"),
			_("GogErrorBar *"),
			GOG_TYPE_ERROR_BAR,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

GSF_DYNAMIC_CLASS (GogPolarSeries, gog_polar_series,
	gog_polar_series_class_init, NULL,
	GOG_TYPE_RT_SERIES)

/*****************************************************************************/

static void
gog_color_polar_series_init_style (GogStyledObject *gso, GOStyle *style)
{
	series_parent_klass->init_style (gso, style);
	style->fill.type = GO_STYLE_FILL_NONE;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_NONE;
}

static void
gog_color_polar_series_update (GogObject *obj)
{
	const double *a_vals, *r_vals, *z_vals = NULL;
	GogRTSeries *series = GOG_RT_SERIES (obj);
	unsigned old_num = series->base.num_elements;

	series->base.num_elements = gog_series_get_xyz_data (GOG_SERIES (series),
								     &a_vals, &r_vals, &z_vals);

	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));
	if (old_num != series->base.num_elements)
		gog_plot_request_cardinality_update (series->base.plot);

	if (series_parent_klass->base.update)
		series_parent_klass->base.update (obj); /* do not call gog_rt_series_update */
}

static void
gog_color_polar_series_class_init (GogObjectClass *gog_klass)
{
	GogStyledObjectClass *gso_klass = (GogStyledObjectClass *) gog_klass;

	gog_klass->update = gog_color_polar_series_update;
	gso_klass->init_style =	gog_color_polar_series_init_style;
}

GSF_DYNAMIC_CLASS (GogColorPolarSeries, gog_color_polar_series,
	gog_color_polar_series_class_init, NULL,
	GOG_TYPE_POLAR_SERIES)

G_MODULE_EXPORT void
go_plugin_init (GOPlugin *plugin, GOCmdContext *cc)
{
	GTypeModule *module = go_plugin_get_type_module (plugin);
	gog_rt_plot_register_type (module);
	gog_radar_plot_register_type (module);
	gog_radar_area_plot_register_type (module);
	gog_polar_plot_register_type (module);
	gog_color_polar_plot_register_type (module);
	gog_rt_view_register_type (module);
	gog_rt_series_register_type (module);
	gog_rt_series_element_register_type (module);
	gog_polar_series_register_type (module);
	gog_color_polar_series_register_type (module);

	register_embedded_stuff ();
}

G_MODULE_EXPORT void
go_plugin_shutdown (GOPlugin *plugin, GOCmdContext *cc)
{
	unregister_embedded_stuff ();
}
