/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-xyz-surface.c
 *
 * Copyright (C) 2004-2005 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-xyz-surface.h"

#include <goffice/data/go-data.h>
#include <goffice/graph/gog-chart-map-3d.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-format.h>
#include <goffice/utils/go-path.h>
#include <goffice/utils/go-persist.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

/*****************************************************************************/

enum {
	XYZ_SURFACE_PROP_0,
	XYZ_SURFACE_PROP_ROWS,
	XYZ_SURFACE_PROP_COLUMNS
};

static GogObjectClass *plot_xyz_contour_parent_klass;
static GogObjectClass *plot_xyz_surface_parent_klass;

#define EPSILON 1e-13

static double *
gog_xyz_surface_plot_build_matrix (GogXYZPlot const *plot, gboolean *cardinality_changed)
{
	unsigned i, j, k, index;
	GogSeries *series = GOG_SERIES (plot->base.series->data);
	const double *x_vals, *y_vals, *z_vals = NULL;
	double *data;
	unsigned *grid;
	unsigned n = plot->rows * plot->columns;
	unsigned kmax = gog_series_get_xyz_data (GOG_SERIES (series), 
						 &x_vals, &y_vals, &z_vals);
	unsigned imax = plot->rows;
	unsigned jmax = plot->columns;
	double xmin = plot->x.minima;
	double ymin = plot->y.minima;
	double xinc = (plot->x.maxima - xmin) / (jmax - 1);
	double yinc = (plot->y.maxima - ymin) / (imax - 1);

	data = g_new0 (double, n);
	grid = g_new0 (unsigned, n);
	
	for (k = 0; k < kmax; ++k) {
		j = (int) floor ((x_vals[k] - xmin) / xinc + 0.5);
		i = (int) floor ((y_vals[k] - ymin) / yinc + 0.5);
		index = i * jmax + j;
		/* Watch out not to fall beyond array limits; however,
		 * its unlikely and shouldn't happen normally */
		if (G_LIKELY (index < n)) {
			data[index] += z_vals[k];
			grid[index]++;
		}
	}

	for (k = 0; k < n; ++k)
		if (grid[k] != 0)
			data[k] /= grid[k];

	if (GOG_IS_PLOT_CONTOUR (plot)) {
		GogAxisMap *map;
		GogAxisTick *zticks;
		GogAxis *axis = plot->base.axis[GOG_AXIS_PSEUDO_3D];
		unsigned nticks;
		double *x, val, minimum, maximum, slope, offset = 0.;
		unsigned max;

	if (!gog_axis_get_bounds (axis, &minimum, &maximum)) {
			g_free (grid);
			g_free (data);
			return NULL;
		}
		nticks = gog_axis_get_ticks (axis, &zticks);
		map = gog_axis_map_new (axis, 0, 1);
		x = g_new (double, nticks);
		for (i = j = 0; i < nticks; i++)
			if (zticks[i].type == GOG_AXIS_TICK_MAJOR) {
				x[j++] = gog_axis_map_to_view (map, zticks[i].position);
			}
		max = --j;
		if (x[1] > x[0]) {
			if (x[0] > EPSILON) {
				offset = 1.;
				max++;
			}
			if (x[j] < 1. - EPSILON)
				max++;
			slope = 1 / (x[1] - x[0]);
		} else {
			offset = j;
			if (x[0] < 1. - EPSILON)
				max++;
			if (x[j] > EPSILON) {
				max++;
				offset += 1.;
			}
			slope = 1 / (x[0] - x[1]);
		}
		for (k = 0; k < n; ++k) {
			val = gog_axis_map_to_view (map, data[k]);
			if (fabs (val) == DBL_MAX)
				val = go_nan;
			else {
				val = offset + slope * (val - x[0]);
				if (val < 0)
					val = (val < -EPSILON)? go_nan: 0.;
			}
			data[k] = val;
		}
		if (series->num_elements != max) {
			series->num_elements = max;
			*cardinality_changed = TRUE;
		}
		gog_axis_map_free (map);
		g_free (x);
		if (max < 2) { /* this might happen with bad 3d axis configuration */
			g_free (data);
			data = NULL;
		}
	} else
		*cardinality_changed = FALSE;
	g_free (grid);
	return data;
}

static char const *
gog_xyz_contour_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name surface plot objects
	*/
	return N_("PlotXYZContour");
}

static char const *
gog_xyz_surface_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name surface plot objects
	*/
	return N_("PlotXYZSurface");
}

#ifdef GOFFICE_WITH_GTK
extern gpointer gog_xyz_surface_plot_pref (GogXYZPlot *plot, GOCmdContext *cc);
static void
gog_xyz_surface_plot_populate_editor (GogObject *item,
				  GogEditor *editor,
				  G_GNUC_UNUSED GogDataAllocator *dalloc,
				  GOCmdContext *cc)
{
	GogObjectClass *klass = (GOG_IS_PLOT_CONTOUR (item))?
				plot_xyz_contour_parent_klass:
				plot_xyz_surface_parent_klass;

	gog_editor_add_page (editor,
			     gog_xyz_surface_plot_pref (GOG_XYZ_PLOT (item), cc),
			     _("Properties"));
	(GOG_OBJECT_CLASS (klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

static void
gog_xyz_surface_plot_update (GogObject *obj)
{
	GogXYZPlot *model = GOG_XYZ_PLOT(obj);
	GogXYZSeries *series;
	double tmp_min, tmp_max;
	GogObjectClass *klass = (GOG_IS_PLOT_CONTOUR (obj))?
				plot_xyz_contour_parent_klass:
				plot_xyz_surface_parent_klass;

	if (model->base.series == NULL)
		return;

	series = GOG_XYZ_SERIES (model->base.series->data);
	if (!gog_series_is_valid (GOG_SERIES (series)))
		return;
	
	go_data_vector_get_minmax (GO_DATA_VECTOR (
		series->base.values[0].data), &tmp_min, &tmp_max);
	if (!go_finite (tmp_min) || !go_finite (tmp_max) ||
	    tmp_min > tmp_max) {
		tmp_min = 0;
		tmp_max = go_data_vector_get_len (GO_DATA_VECTOR (series->base.values[0].data));
	} else if (model->x.fmt == NULL)
		model->x.fmt = go_data_preferred_fmt (series->base.values[0].data);
	model->x.minima = tmp_min;
	model->x.maxima = tmp_max;
	gog_axis_bound_changed (model->base.axis[GOG_AXIS_X], GOG_OBJECT (model));

	go_data_vector_get_minmax (GO_DATA_VECTOR (series->base.values[1].data),
	                           &tmp_min, &tmp_max);
	if (!go_finite (tmp_min) || !go_finite (tmp_max) ||
	    tmp_min > tmp_max) {
		tmp_min = 0;
		tmp_max = go_data_vector_get_len (GO_DATA_VECTOR (series->base.values[1].data));
	} else if (model->y.fmt == NULL)
		model->y.fmt = go_data_preferred_fmt (series->base.values[1].data);
	model->y.minima = tmp_min;
	model->y.maxima = tmp_max;
	gog_axis_bound_changed (model->base.axis[GOG_AXIS_Y], GOG_OBJECT (model));

	go_data_vector_get_minmax (GO_DATA_VECTOR (series->base.values[2].data),
	                           &tmp_min, &tmp_max);
	if (!go_finite (tmp_min) || !go_finite (tmp_max) ||
	    tmp_min > tmp_max) {
		tmp_min = 0;
		tmp_max = go_data_vector_get_len (GO_DATA_VECTOR (series->base.values[2].data));
	} else if (model->z.fmt == NULL)
		model->z.fmt = go_data_preferred_fmt (series->base.values[2].data);
	model->z.minima = tmp_min;
	model->z.maxima = tmp_max;
	gog_axis_bound_changed (model->base.axis[((GOG_IS_PLOT_CONTOUR (model))? GOG_AXIS_PSEUDO_3D: GOG_AXIS_Z)], GOG_OBJECT (model));

	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
	if (klass->update)
		klass->update (obj);
}

static void
gog_xyz_surface_plot_set_property (GObject *obj, guint param_id,
				   GValue const *value, GParamSpec *pspec)
{
	GogXYZPlot *plot = GOG_XYZ_PLOT (obj);

	switch (param_id) {
	case XYZ_SURFACE_PROP_ROWS :
		if (plot->rows != g_value_get_uint (value)) {
			plot->rows = g_value_get_uint (value);
			g_free (plot->plotted_data);
			plot->plotted_data = NULL;
			if (plot->y_vals != NULL) {
				g_object_unref (plot->y_vals);
				plot->y_vals = NULL;
			}
		}
		break;
	case XYZ_SURFACE_PROP_COLUMNS :
		if (plot->columns != g_value_get_uint (value)) {
			plot->columns = g_value_get_uint (value);
			g_free (plot->plotted_data);
			plot->plotted_data = NULL;
			if (plot->x_vals != NULL) {
				g_object_unref (plot->x_vals);
				plot->x_vals = NULL;
			}
		}
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_xyz_surface_plot_get_property (GObject *obj, guint param_id,
				   GValue *value, GParamSpec *pspec)
{
	GogXYZPlot *plot = GOG_XYZ_PLOT (obj);

	switch (param_id) {
	case XYZ_SURFACE_PROP_ROWS :
		g_value_set_uint (value, plot->rows);
		break;
	case XYZ_SURFACE_PROP_COLUMNS :
		g_value_set_uint (value, plot->columns);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
common_init_class (GogXYZPlotClass *klass)
{
	GogPlotClass *gog_plot_klass = (GogPlotClass*) klass;
	GObjectClass *gobject_klass = (GObjectClass *) klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;

	gobject_klass->set_property = gog_xyz_surface_plot_set_property;
	gobject_klass->get_property = gog_xyz_surface_plot_get_property;
	g_object_class_install_property (gobject_klass, XYZ_SURFACE_PROP_ROWS,
		g_param_spec_uint ("rows", 
			_("Rows"),
			_("Number of rows"),
			2, 1000, 10, 
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, XYZ_SURFACE_PROP_COLUMNS,
		g_param_spec_uint ("columns", 
			_("Columns"),
			_("Number of columns"),
			2, 1000, 10, 
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_klass->update	= gog_xyz_surface_plot_update;

#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor	= gog_xyz_surface_plot_populate_editor;
#endif

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("X"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
			{ N_("Y"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
			{ N_("Z"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
		};
		gog_plot_klass->desc.series.dim = dimensions;
		gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
	}

	klass->build_matrix = gog_xyz_surface_plot_build_matrix;
}

static void
gog_xyz_contour_plot_class_init (GogXYZPlotClass *klass)
{
	plot_xyz_contour_parent_klass = g_type_class_peek_parent (klass);
	GOG_OBJECT_CLASS (klass)->type_name = gog_xyz_contour_plot_type_name;
	common_init_class (klass);
}

static void
gog_xyz_surface_plot_class_init (GogXYZPlotClass *klass)
{
	plot_xyz_surface_parent_klass = g_type_class_peek_parent (klass);
	GOG_OBJECT_CLASS (klass)->type_name = gog_xyz_surface_plot_type_name;
	common_init_class (klass);
}

static void
gog_xyz_surface_plot_init (GogXYZPlot *xyz)
{
	xyz->data_xyz = TRUE;
	xyz->rows = 10;
	xyz->columns = 10;
}

GSF_DYNAMIC_CLASS (GogXYZContourPlot, gog_xyz_contour_plot,
	gog_xyz_contour_plot_class_init, gog_xyz_surface_plot_init,
	GOG_CONTOUR_PLOT_TYPE)

GSF_DYNAMIC_CLASS (GogXYZSurfacePlot, gog_xyz_surface_plot,
	gog_xyz_surface_plot_class_init, gog_xyz_surface_plot_init,
	GOG_SURFACE_PLOT_TYPE)
