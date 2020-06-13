/*
 * gog-xyz.c
 *
 * Copyright (C) 2004-2007 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-xyz.h"
#include "gog-contour.h"
#include "gog-matrix.h"
#include "gog-surface.h"
#include "gog-xyz-surface.h"
#include "xl-surface.h"
#include <goffice/app/module-plugin-defs.h>
#include <goffice/data/go-data.h>
#include <goffice/data/go-data-simple.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-format.h>
#include <goffice/utils/go-persist.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#include "embedded-stuff.c"

GOFFICE_PLUGIN_MODULE_HEADER;
/*-----------------------------------------------------------------------------
 *
 *  GogContourPlot
 *
 *-----------------------------------------------------------------------------
 */

enum {
	XYZ_PROP_0,
	XYZ_PROP_TRANSPOSED
};

static GogObjectClass *plot_xyz_parent_klass;

/**
 * gog_xyz_plot_build_matrix :
 * @plot:
 *
 * builds a table of normalized values: first slice = 0-1 second = 1-2,... if any.
 **/

double *
gog_xyz_plot_build_matrix (GogXYZPlot *plot, gboolean *cardinality_changed)
{
	GogXYZPlotClass *klass = GOG_XYZ_PLOT_GET_CLASS (plot);
	return klass->build_matrix (plot, cardinality_changed);
}

static void
gog_xyz_plot_update_3d (GogPlot *plot)
{
	GogXYZPlot *xyz = GOG_XYZ_PLOT (plot);
	gboolean cardinality_changed = FALSE;

	if (plot->series == NULL)
		return;

	g_free (xyz->plotted_data);
	xyz->plotted_data = gog_xyz_plot_build_matrix (xyz, &cardinality_changed);
	if (cardinality_changed) {
		/*	gog_plot_request_cardinality_update can't be called from here
		 *  since the plot might be updating.
		 */
		GogChart *chart = GOG_CHART (GOG_OBJECT (plot)->parent);
		plot->cardinality_valid = FALSE;
		if (chart != NULL)
			gog_chart_request_cardinality_update (chart);
	}
}

#ifdef GOFFICE_WITH_GTK
extern gpointer gog_xyz_plot_pref (GogXYZPlot *plot, GOCmdContext *cc);
static void
gog_xyz_plot_populate_editor (GogObject *item,
				  GOEditor *editor,
				  G_GNUC_UNUSED GogDataAllocator *dalloc,
				  GOCmdContext *cc)
{
	if (!GOG_XYZ_PLOT (item)->data_xyz) {
		GtkWidget *w = gog_xyz_plot_pref (GOG_XYZ_PLOT (item), cc);
		go_editor_add_page (editor, w, _("Properties"));
		g_object_unref (w);
	}

	(GOG_OBJECT_CLASS (plot_xyz_parent_klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

GOData *
gog_xyz_plot_get_x_vals (GogXYZPlot *plot)
{
	double inc;
	double *vals;
	unsigned i, imax;
	if (plot->data_xyz) {
		if (plot->x_vals == NULL) {
			imax = plot->columns;
			if (GOG_IS_MATRIX_PLOT (plot))
				imax++;
			inc = (plot->x.maxima - plot->x.minima) / (imax - 1);
			vals = g_new (double, imax);
			for (i = 0; i < imax; ++i)
				vals[i] = plot->x.minima + i * inc;
			plot->x_vals = GO_DATA (go_data_vector_val_new (vals, imax, g_free));
		}
		return plot->x_vals;
	} else {
		GogSeries *series = GOG_SERIES (plot->base.series->data);
		return series->values[(plot->transposed)?  1: 0].data;
	}
}

GOData *
gog_xyz_plot_get_y_vals (GogXYZPlot *plot)
{
	double inc;
	double *vals;
	unsigned i, imax;
	if (plot->data_xyz) {
		if (plot->y_vals == NULL) {
			imax = plot->rows;
			if (GOG_IS_MATRIX_PLOT (plot))
				imax++;
			inc = (plot->y.maxima - plot->y.minima) / (imax - 1);
			vals = g_new (double, imax);
			for (i = 0; i < imax; ++i)
				vals[i] = plot->y.minima + i * inc;
			plot->y_vals = GO_DATA (go_data_vector_val_new (vals, imax, g_free));
		}
		return plot->y_vals;
	} else {
		GogSeries *series = GOG_SERIES (plot->base.series->data);
		return series->values[(plot->transposed)?  0: 1].data;
	}
}

static void
gog_xyz_plot_clear_formats (GogXYZPlot *plot)
{
	go_format_unref (plot->x.fmt);
	plot->x.fmt = NULL;

	go_format_unref (plot->y.fmt);
	plot->y.fmt = NULL;

	go_format_unref (plot->z.fmt);
	plot->z.fmt = NULL;
}

static void
gog_xyz_plot_update (GogObject *obj)
{
	GogXYZPlot * model = GOG_XYZ_PLOT(obj);
	GogXYZSeries * series;
	GOData *vec;
	GOData *mat;
	double tmp_min, tmp_max;

	if (model->base.series == NULL)
		return;

	if (model->data_xyz) {
		if (plot_xyz_parent_klass->update)
			plot_xyz_parent_klass->update (obj);
		return;
	}

	series = GOG_XYZ_SERIES (model->base.series->data);
	if (!gog_series_is_valid (GOG_SERIES (series)))
		return;

	if ((vec = series->base.values[0].data) != NULL) {
		if (model->x.fmt == NULL)
			model->x.fmt = go_data_preferred_fmt (series->base.values[0].data);
		model->x.date_conv = go_data_date_conv (series->base.values[0].data);
		if (go_data_is_varying_uniformly (vec))
			go_data_get_bounds (vec, &tmp_min, &tmp_max);
		else
			tmp_min = tmp_max = go_nan;
	} else {
		tmp_min = 0;
		tmp_max = series->columns - 1;
	}

	if ((model->columns != series->columns)
			|| (tmp_min != model->x.minima)
			|| (tmp_max != model->x.maxima)) {
		model->columns = series->columns;
		model->x.minima = tmp_min;
		model->x.maxima = tmp_max;
		gog_axis_bound_changed (model->base.axis[(model->transposed)? GOG_AXIS_Y: GOG_AXIS_X],
				GOG_OBJECT (model));
	}

	if ((vec = series->base.values[1].data) != NULL) {
		if (model->y.fmt == NULL)
			model->y.fmt = go_data_preferred_fmt (series->base.values[1].data);
		model->y.date_conv = go_data_date_conv (series->base.values[1].data);
		if (go_data_is_varying_uniformly (vec))
			go_data_get_bounds (vec, &tmp_min, &tmp_max);
		else
			tmp_min = tmp_max = go_nan;
	} else {
		tmp_min = 0;
		tmp_max = series->rows - 1;
	}

	if ((model->rows != series->rows)
			|| (tmp_min != model->y.minima)
			|| (tmp_max != model->y.maxima)) {
		model->rows = series->rows;
		model->y.minima = tmp_min;
		model->y.maxima = tmp_max;
		gog_axis_bound_changed (model->base.axis[(model->transposed)? GOG_AXIS_X: GOG_AXIS_Y],
				GOG_OBJECT (model));
	}

	g_free (model->plotted_data);
	model->plotted_data = NULL;
	mat = series->base.values[2].data;
	go_data_get_bounds (mat, &tmp_min, &tmp_max);
	if ((tmp_min != model->z.minima)
			|| (tmp_max != model->z.maxima)) {
		model->z.minima = tmp_min;
		model->z.maxima = tmp_max;
		gog_axis_bound_changed (
			model->base.axis[GOG_XYZ_PLOT_GET_CLASS (model)->third_axis],
			GOG_OBJECT (model));
	} else
		gog_xyz_plot_update_3d (GOG_PLOT (model));

	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
	if (plot_xyz_parent_klass->update)
		plot_xyz_parent_klass->update (obj);
}

static GOData *
gog_xyz_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
				  GogPlotBoundInfo * bounds)
{
	GogXYZSeries *series;
	GogXYZPlot *xyz = GOG_XYZ_PLOT (plot);
	GOData *vec = NULL;
	double min, max;
	GOFormat const *fmt;
	if (!plot->series)
		return NULL;
	series = GOG_XYZ_SERIES (plot->series->data);
	if ((axis == GOG_AXIS_Y && xyz->transposed) ||
		(axis == GOG_AXIS_X && !xyz->transposed)) {
		vec = series->base.values[0].data;
		fmt = xyz->x.fmt;
		min = xyz->x.minima;
		max = xyz->x.maxima;
		if (xyz->x.date_conv)
			bounds->date_conv = xyz->x.date_conv;
	} else if (axis == GOG_AXIS_X || axis == GOG_AXIS_Y) {
		vec = series->base.values[1].data;
		fmt = xyz->y.fmt;
		min = xyz->y.minima;
		max = xyz->y.maxima;
		if (xyz->y.date_conv)
			bounds->date_conv = xyz->y.date_conv;
	} else {
		if (bounds->fmt == NULL && xyz->z.fmt != NULL)
			bounds->fmt = go_format_ref (xyz->z.fmt);
		if (xyz->z.date_conv)
			bounds->date_conv = xyz->z.date_conv;
		bounds->val.minima = xyz->z.minima;
		bounds->val.maxima = xyz->z.maxima;
		return NULL;
	}
	if (bounds->fmt == NULL && fmt != NULL)
		bounds->fmt = go_format_ref (fmt);
	if (go_finite (min) && vec) {
		bounds->logical.minima = bounds->val.minima = min;
		bounds->logical.maxima = bounds->val.maxima = max;
		bounds->is_discrete = FALSE;
	} else {
		bounds->val.minima = 1.;
		bounds->logical.minima = 1.;
		bounds->logical.maxima = go_nan;
		bounds->is_discrete    = TRUE;
		bounds->center_on_ticks = TRUE;
		bounds->val.maxima = ((axis == GOG_AXIS_Y && xyz->transposed) ||
		(axis == GOG_AXIS_X && !xyz->transposed)) ?
			series->columns:
			series->rows;
		if (GOG_IS_MATRIX_PLOT (plot))
			bounds->val.maxima++;
	}
	return (GOData*) vec;
}

static void
gog_xyz_plot_finalize (GObject *obj)
{
	GogXYZPlot *plot = GOG_XYZ_PLOT (obj);
	gog_xyz_plot_clear_formats (plot);
	g_free (plot->plotted_data);
	if (plot->x_vals != NULL)
		g_object_unref (plot->x_vals);
	if (plot->y_vals != NULL)
		g_object_unref (plot->y_vals);
	G_OBJECT_CLASS (plot_xyz_parent_klass)->finalize (obj);
}

static void
gog_xyz_plot_set_property (GObject *obj, guint param_id,
			     GValue const *value, GParamSpec *pspec)
{
	GogXYZPlot *plot = GOG_XYZ_PLOT (obj);

	switch (param_id) {
	case XYZ_PROP_TRANSPOSED :
		/* Transposed property have no meaning when data set is XYZ */
		if (plot->data_xyz)
			return;
		if (!plot->transposed != !g_value_get_boolean (value)) {
			plot->transposed = g_value_get_boolean (value);
			if (NULL != plot->base.axis[GOG_AXIS_X])
				gog_axis_bound_changed (plot->base.axis[GOG_AXIS_X], GOG_OBJECT (plot));
			if (NULL != plot->base.axis[GOG_AXIS_Y])
				gog_axis_bound_changed (plot->base.axis[GOG_AXIS_Y], GOG_OBJECT (plot));
			g_free (plot->plotted_data);
			plot->plotted_data = NULL;
		}
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_xyz_plot_get_property (GObject *obj, guint param_id,
			     GValue *value, GParamSpec *pspec)
{
	GogXYZPlot *plot = GOG_XYZ_PLOT (obj);

	switch (param_id) {
	case XYZ_PROP_TRANSPOSED :
		g_value_set_boolean (value, plot->transposed);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_xyz_plot_class_init (GogXYZPlotClass *klass)
{
	GogPlotClass *gog_plot_klass = (GogPlotClass*) klass;
	GObjectClass   *gobject_klass = (GObjectClass *) klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;

	plot_xyz_parent_klass = g_type_class_peek_parent (klass);

	klass->get_x_vals = gog_xyz_plot_get_x_vals;
	klass->get_y_vals = gog_xyz_plot_get_y_vals;

	gobject_klass->finalize     = gog_xyz_plot_finalize;
	gobject_klass->set_property = gog_xyz_plot_set_property;
	gobject_klass->get_property = gog_xyz_plot_get_property;
	g_object_class_install_property (gobject_klass, XYZ_PROP_TRANSPOSED,
		g_param_spec_boolean ("transposed",
			_("Transposed"),
			_("Transpose the plot"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE|GO_PARAM_PERSISTENT));

	/* Fill in GOGObject superclass values */
	gog_object_klass->update	= gog_xyz_plot_update;
#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor	= gog_xyz_plot_populate_editor;
#endif

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("X"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_LABEL, GOG_MS_DIM_CATEGORIES },
			{ N_("Y"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_LABEL, GOG_MS_DIM_CATEGORIES },
			{ N_("Z"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_MATRIX, GOG_MS_DIM_VALUES },
		};
		gog_plot_klass->desc.series.dim = dimensions;
		gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		gog_plot_klass->desc.series.style_fields = GO_STYLE_LINE;
	}

	/* Fill in GogPlotClass methods */
	gog_plot_klass->desc.num_series_max = 1;
	gog_plot_klass->series_type = gog_xyz_series_get_type();
	gog_plot_klass->axis_get_bounds	= gog_xyz_plot_axis_get_bounds;
	gog_plot_klass->update_3d = gog_xyz_plot_update_3d;
}

static void
gog_xyz_plot_init (GogXYZPlot *xyz)
{
	xyz->rows = xyz->columns = 0;
	xyz->transposed = FALSE;
	xyz->data_xyz = FALSE;
	xyz->x.minima = xyz->x.maxima = xyz->y.minima
		= xyz->y.maxima = xyz->z.minima = xyz->z.maxima = go_nan;
	xyz->x.fmt = xyz->y.fmt = xyz->z.fmt = NULL;
	xyz->plotted_data = NULL;
	xyz->x_vals = NULL;
	xyz->y_vals = NULL;
}

GSF_DYNAMIC_CLASS_ABSTRACT (GogXYZPlot, gog_xyz_plot,
	gog_xyz_plot_class_init, gog_xyz_plot_init,
	GOG_TYPE_PLOT)

/*****************************************************************************/

static GogStyledObjectClass *series_parent_klass;

static void
gog_xyz_series_update (GogObject *obj)
{
	GogXYZSeries *series = GOG_XYZ_SERIES (obj);
	GODataMatrixSize size;
	GOData *mat;
	GOData *vec;
	int length;
	size.rows = 0;
	size.columns = 0;

	if (GOG_XYZ_PLOT (series->base.plot)->data_xyz) {
		const double *x_vals, *y_vals, *z_vals = NULL;
		series->base.num_elements = gog_series_get_xyz_data (GOG_SERIES (series),
								     &x_vals, &y_vals, &z_vals);
	} else {
		if (series->base.values[2].data != NULL) {
			mat = series->base.values[2].data;
			go_data_get_values (mat);
			go_data_get_matrix_size (mat, &size.rows, &size.columns);
		}
		if (series->base.values[0].data != NULL) {
			vec = series->base.values[0].data;
			go_data_get_values (vec);
			length = go_data_get_vector_size (vec);
			if (GOG_IS_MATRIX_PLOT (series->base.plot) && length > 0)
				length--;
			if (length < size.columns)
				size.columns = length;
		}
		if (series->base.values[1].data != NULL) {
			vec = series->base.values[1].data;
			go_data_get_values (vec);
			length = go_data_get_vector_size (vec);
			if (GOG_IS_MATRIX_PLOT (series->base.plot) && length > 0)
				length--;
			if (length < size.rows)
				size.rows = length;
		}
		series->rows = size.rows;
		series->columns = size.columns;
	}

	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->base.plot));

	if (series_parent_klass->base.update)
		series_parent_klass->base.update (obj);
}

static void
gog_xyz_series_init_style (GogStyledObject *gso, GOStyle *style)
{
	series_parent_klass->init_style (gso, style);
	if (GOG_IS_MATRIX_PLOT (GOG_SERIES (gso)->plot) && style->line.auto_dash)
		style->line.dash_type = GO_LINE_NONE;
}

static void
gog_xyz_series_class_init (GogStyledObjectClass *gso_klass)
{
	GogObjectClass * obj_klass = (GogObjectClass *) gso_klass;

	series_parent_klass = g_type_class_peek_parent (gso_klass);
	gso_klass->init_style = gog_xyz_series_init_style;
	obj_klass->update = gog_xyz_series_update;
}


GSF_DYNAMIC_CLASS (GogXYZSeries, gog_xyz_series,
	gog_xyz_series_class_init, NULL,
	GOG_TYPE_SERIES)

/*****************************************************************************/

G_MODULE_EXPORT void
go_plugin_init (GOPlugin *plugin, GOCmdContext *cc)
{
	GTypeModule *module = go_plugin_get_type_module (plugin);
	gog_xyz_plot_register_type (module);
	gog_contour_plot_register_type (module);
	gog_contour_view_register_type (module);
	gog_matrix_plot_register_type (module);
	gog_matrix_view_register_type (module);
	gog_surface_plot_register_type (module);
	gog_surface_view_register_type (module);
	gog_xyz_contour_plot_register_type (module);
	gog_xyz_matrix_plot_register_type (module);
	gog_xyz_surface_plot_register_type (module);
	gog_xyz_series_register_type (module);
	gog_xy_contour_plot_register_type (module);
	gog_xy_matrix_plot_register_type (module);
	gog_xy_surface_plot_register_type (module);
	xl_xyz_series_register_type (module);
	xl_contour_plot_register_type (module);
	xl_surface_plot_register_type (module);

	register_embedded_stuff ();
}

G_MODULE_EXPORT void
go_plugin_shutdown (GOPlugin *plugin, GOCmdContext *cc)
{
	unregister_embedded_stuff ();
}
