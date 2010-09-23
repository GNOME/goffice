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
	XYZ_SURFACE_PROP_COLUMNS,
	XYZ_SURFACE_PROP_AUTO_ROWS,
	XYZ_SURFACE_PROP_AUTO_COLUMNS
};

static GogObjectClass *plot_xyz_contour_parent_klass;
static GogObjectClass *plot_xyz_surface_parent_klass;

#define EPSILON 1e-13

typedef struct {
	double const *values[2];
	unsigned cur_series;
} xyz_data;

static int
data_compare (unsigned const *a, unsigned const *b, xyz_data *data)
{
	double  xa = data->values[data->cur_series][*a],
		xb = data->values[data->cur_series][*b];
	if (xa < xb)
		return -1;
	else if (xa == xb)
		return 0;
	else
		return 1;
}

static double *
gog_xyz_surface_plot_build_matrix (GogXYZPlot const *plot, gboolean *cardinality_changed)
{
	unsigned i, j, k, l, index;
	GogSeries *series = GOG_SERIES (plot->base.series->data);
	const double *x_vals, *y_vals, *z_vals = NULL;
	double *x_limits, *y_limits;
	double *data;
	unsigned *grid, n, kmax, imax, jmax;
	double xmin, ymin, xinc, yinc;
	xyz_data raw_data;
	unsigned *sort;
	if (GOG_IS_CONTOUR_PLOT (plot)) {
		GogXYZContourPlot *xyz = GOG_XYZ_CONTOUR_PLOT (plot);
		if (xyz->grid[0].data) {
			xyz->base.x_vals = g_object_ref (xyz->grid[0].data);
			xyz->base.columns = go_data_get_vector_size (plot->x_vals);
		}
		if (xyz->grid[1].data) {
			xyz->base.y_vals = g_object_ref (xyz->grid[1].data);
			xyz->base.rows = go_data_get_vector_size (plot->y_vals);
		}
	} else {
		GogXYZSurfacePlot *xyz = GOG_XYZ_SURFACE_PLOT (plot);
		if (xyz->grid[0].data) {
			xyz->base.x_vals = g_object_ref (xyz->grid[0].data);
			xyz->base.columns = go_data_get_vector_size (plot->x_vals);
		}
		if (xyz->grid[1].data) {
			xyz->base.y_vals = g_object_ref (xyz->grid[1].data);
			xyz->base.rows = go_data_get_vector_size (plot->y_vals);
		}
	}
	n = plot->rows * plot->columns;
	if (n == 0)
		return NULL;
	x_limits = go_range_sort (go_data_get_values (gog_xyz_plot_get_x_vals ((GogXYZPlot *) plot)), plot->columns);
	for (i = 0; i < plot->columns - 1; i++)
		x_limits[i] = (x_limits[i] + x_limits[i+1]) / 2.;
	x_limits[i] = G_MAXDOUBLE;
	y_limits = go_range_sort (go_data_get_values (gog_xyz_plot_get_y_vals ((GogXYZPlot *) plot)), plot->rows);
	for (i = 0; i < plot->rows - 1; i++)
		y_limits[i] = (y_limits[i] + y_limits[i+1]) / 2.;
	y_limits[i] = G_MAXDOUBLE;
	kmax = gog_series_get_xyz_data (GOG_SERIES (series),
						 &x_vals, &y_vals, &z_vals);
	/* sort the data by column and row */
	raw_data.values[0] = x_vals;
	raw_data.values[1] = y_vals;
	raw_data.cur_series = 1;
	sort = g_new0 (unsigned, kmax);
	for (i = 0; i < kmax; i++)
		sort[i] = i;
	g_qsort_with_data (sort, kmax, sizeof (unsigned), (GCompareDataFunc) data_compare, &raw_data);
	raw_data.cur_series = 0;
	imax = plot->rows;
	jmax = plot->columns;
	xmin = plot->x.minima;
	ymin = plot->y.minima;
	xinc = (plot->x.maxima - xmin) / (jmax - 1);
	yinc = (plot->y.maxima - ymin) / (imax - 1);

	data = g_new0 (double, n);
	grid = g_new0 (unsigned, n);

	k = l = index = 0;
	for (i = 0; i < imax; i++) {
		while (l < kmax && y_vals[sort[l]] < y_limits[i])
			l++;
		g_qsort_with_data (sort + k, l - k, sizeof (unsigned), (GCompareDataFunc) data_compare, &raw_data);
		for (j = 0; j < jmax && k < l; j++) {
			index = i * jmax + j;
			while (k < l && x_vals[sort[k]] < x_limits[j]) {
				if (G_LIKELY (index < n)) {
					data[index] += z_vals[sort[k]];
					grid[index]++;
				}
				k++;
			}
		}
		k = l;
	}

	for (k = 0; k < n; ++k)
		if (grid[k] != 0)
			data[k] /= grid[k];

	if (GOG_IS_CONTOUR_PLOT (plot)) {
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
	g_free (x_limits);
	g_free (y_limits);
	g_free (sort);
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
extern gpointer gog_xyz_surface_plot_pref (GogXYZPlot *plot, GogDataAllocator *dalloc, GOCmdContext *cc);
static void
gog_xyz_surface_plot_populate_editor (GogObject *item,
				  GOEditor *editor,
				  GogDataAllocator *dalloc,
				  GOCmdContext *cc)
{
	GogObjectClass *klass = (GOG_IS_CONTOUR_PLOT (item))?
				plot_xyz_contour_parent_klass:
				plot_xyz_surface_parent_klass;
	GtkWidget *w = gog_xyz_surface_plot_pref (GOG_XYZ_PLOT (item), dalloc, cc);
	go_editor_add_page (editor, w, _("Properties"));
	g_object_unref (G_OBJECT (w));

	(GOG_OBJECT_CLASS (klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

static void
gog_xyz_surface_plot_update (GogObject *obj)
{
	GogXYZPlot *model = GOG_XYZ_PLOT(obj);
	GogXYZSeries *series;
	double tmp_min, tmp_max;
	GogObjectClass *klass = (GOG_IS_CONTOUR_PLOT (obj))?
				plot_xyz_contour_parent_klass:
				plot_xyz_surface_parent_klass;
	if (model->base.series == NULL)
		return;

	series = GOG_XYZ_SERIES (model->base.series->data);
	if (!gog_series_is_valid (GOG_SERIES (series)))
		return;

	go_data_get_bounds (series->base.values[0].data, &tmp_min, &tmp_max);
	if (!go_finite (tmp_min) || !go_finite (tmp_max) ||
	    tmp_min > tmp_max) {
		tmp_min = 0;
		tmp_max = go_data_get_vector_size (series->base.values[0].data);
	} else if (model->x.fmt == NULL)
		model->x.fmt = go_data_preferred_fmt (series->base.values[0].data);
	model->x.date_conv = go_data_date_conv (series->base.values[0].data);
	model->x.minima = tmp_min;
	model->x.maxima = tmp_max;
	gog_axis_bound_changed (model->base.axis[GOG_AXIS_X], GOG_OBJECT (model));
	if (model->x_vals != NULL) {
		g_object_unref (model->x_vals);
		model->x_vals = NULL;
	}

	go_data_get_bounds (series->base.values[1].data, &tmp_min, &tmp_max);
	if (!go_finite (tmp_min) || !go_finite (tmp_max) ||
	    tmp_min > tmp_max) {
		tmp_min = 0;
		tmp_max = go_data_get_vector_size (series->base.values[1].data);
	} else if (model->y.fmt == NULL)
		model->y.fmt = go_data_preferred_fmt (series->base.values[1].data);
	model->y.date_conv = go_data_date_conv (series->base.values[1].data);
	model->y.minima = tmp_min;
	model->y.maxima = tmp_max;
	gog_axis_bound_changed (model->base.axis[GOG_AXIS_Y], GOG_OBJECT (model));
	if (model->y_vals != NULL) {
		g_object_unref (model->y_vals);
		model->y_vals = NULL;
	}

	go_data_get_bounds (series->base.values[2].data, &tmp_min, &tmp_max);
	if (!go_finite (tmp_min) || !go_finite (tmp_max) ||
	    tmp_min > tmp_max) {
		tmp_min = 0;
		tmp_max = go_data_get_vector_size (series->base.values[2].data);
	} else if (model->z.fmt == NULL)
		model->z.fmt = go_data_preferred_fmt (series->base.values[2].data);
	model->z.date_conv = go_data_date_conv (series->base.values[2].data);
	model->z.minima = tmp_min;
	model->z.maxima = tmp_max;
	gog_axis_bound_changed (model->base.axis[((GOG_IS_CONTOUR_PLOT (model))? GOG_AXIS_PSEUDO_3D: GOG_AXIS_Z)], GOG_OBJECT (model));

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
		if (plot->rows == g_value_get_uint (value))
			return;
		plot->rows = g_value_get_uint (value);
		g_free (plot->plotted_data);
		plot->plotted_data = NULL;
		if (plot->y_vals != NULL) {
			g_object_unref (plot->y_vals);
			plot->y_vals = NULL;
		}
		break;
	case XYZ_SURFACE_PROP_COLUMNS :
		if (plot->columns == g_value_get_uint (value))
			return;
		plot->columns = g_value_get_uint (value);
		g_free (plot->plotted_data);
		plot->plotted_data = NULL;
		if (plot->x_vals != NULL) {
			g_object_unref (plot->x_vals);
			plot->x_vals = NULL;
		}
		break;
	case XYZ_SURFACE_PROP_AUTO_ROWS :
		if (plot->auto_y == g_value_get_boolean (value))
			return;
		plot->auto_y = g_value_get_boolean (value);
		g_free (plot->plotted_data);
		plot->plotted_data = NULL;
		if (plot->y_vals != NULL) {
			g_object_unref (plot->y_vals);
			plot->y_vals = NULL;
		}
		break;
	case XYZ_SURFACE_PROP_AUTO_COLUMNS :
		if (plot->auto_x == g_value_get_boolean (value))
			return;
		plot->auto_x = g_value_get_boolean (value);
		g_free (plot->plotted_data);
		plot->plotted_data = NULL;
		if (plot->x_vals != NULL) {
			g_object_unref (plot->x_vals);
			plot->x_vals = NULL;
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
	case XYZ_SURFACE_PROP_AUTO_ROWS :
		g_value_set_boolean (value, plot->auto_y);
		break;
	case XYZ_SURFACE_PROP_AUTO_COLUMNS :
		g_value_set_boolean (value, plot->auto_x);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_xyz_surface_finalize (GObject *obj)
{
	GObjectClass *klass = g_type_class_peek_parent (G_OBJECT_GET_CLASS (obj));
	gog_dataset_finalize (GOG_DATASET (obj));
	klass->finalize (obj);
}

static void
common_init_class (GogXYZPlotClass *klass)
{
	GogPlotClass *gog_plot_klass = (GogPlotClass*) klass;
	GObjectClass *gobject_klass = (GObjectClass *) klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;

	gobject_klass->set_property = gog_xyz_surface_plot_set_property;
	gobject_klass->get_property = gog_xyz_surface_plot_get_property;
	gobject_klass->finalize = gog_xyz_surface_finalize;
	g_object_class_install_property (gobject_klass, XYZ_SURFACE_PROP_ROWS,
		g_param_spec_uint ("rows",
			_("Rows"),
			_("Number of rows"),
			2, 1000, 10,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, XYZ_SURFACE_PROP_AUTO_ROWS,
		g_param_spec_boolean ("auto-rows",
			_("Auto Rows"),
			_("Whether the rows limts should be evaluated"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, XYZ_SURFACE_PROP_COLUMNS,
		g_param_spec_uint ("columns",
			_("Columns"),
			_("Number of columns"),
			2, 1000, 10,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, XYZ_SURFACE_PROP_AUTO_COLUMNS,
		g_param_spec_boolean ("auto-columns",
			_("Auto Columns"),
			_("Whether the columns limts should be evaluated"),
			TRUE,
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
	xyz->auto_x = xyz->auto_y = TRUE;
}

static void
gog_xyz_surface_plot_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = 0;
	*last = 1;
}

static GogDatasetElement *
gog_xyz_contour_plot_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogXYZContourPlot *plot = GOG_XYZ_CONTOUR_PLOT (set);
	g_return_val_if_fail (2 > dim_i, NULL);
	g_return_val_if_fail (dim_i >= 0, NULL);
	return plot->grid + dim_i;
}

static GogDatasetElement *
gog_xyz_surface_plot_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogXYZSurfacePlot *plot = GOG_XYZ_SURFACE_PLOT (set);
	g_return_val_if_fail (2 > dim_i, NULL);
	g_return_val_if_fail (dim_i >= 0, NULL);
	return plot->grid + dim_i;
}

static void
gog_xyz_surface_plot_dataset_dim_changed (GogDataset *set, int dim_i)
{
	gog_object_request_update (GOG_OBJECT (set));
}

static void
gog_xyz_contour_plot_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_xyz_contour_plot_dataset_get_elem;
	iface->dims	   = gog_xyz_surface_plot_dataset_dims;
	iface->dim_changed = gog_xyz_surface_plot_dataset_dim_changed;
}

static void
gog_xyz_surface_plot_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_xyz_surface_plot_dataset_get_elem;
	iface->dims	   = gog_xyz_surface_plot_dataset_dims;
	iface->dim_changed = gog_xyz_surface_plot_dataset_dim_changed;
}

GSF_DYNAMIC_CLASS_FULL (GogXYZContourPlot, gog_xyz_contour_plot,
	NULL, NULL, gog_xyz_contour_plot_class_init, NULL,
        gog_xyz_surface_plot_init, GOG_TYPE_CONTOUR_PLOT, 0,
        GSF_INTERFACE (gog_xyz_contour_plot_dataset_init, GOG_TYPE_DATASET))

GSF_DYNAMIC_CLASS_FULL (GogXYZSurfacePlot, gog_xyz_surface_plot,
	NULL, NULL, gog_xyz_surface_plot_class_init, NULL,
        gog_xyz_surface_plot_init, GOG_TYPE_SURFACE_PLOT, 0,
        GSF_INTERFACE (gog_xyz_surface_plot_dataset_init, GOG_TYPE_DATASET))
