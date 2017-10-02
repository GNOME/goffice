/*
 * gog-xyz-surface.c
 *
 * Copyright (C) 2004-2013 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-xyz-surface.h"

#include <goffice/goffice.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

/*****************************************************************************/

enum {
	XYZ_SURFACE_MISSING_AS_NAN,
	XYZ_SURFACE_MISSING_AS_ZERO,
	/* we might add interpolation methods there */
	XYZ_SURFACE_MISSING_MAX
};

static struct {unsigned n; char const *name;} missing_as_strings[] =
{
	{XYZ_SURFACE_MISSING_AS_NAN, "invalid"},
	{XYZ_SURFACE_MISSING_AS_ZERO, "0"}
};

char const *
missing_as_string (unsigned n)
{
	unsigned i;
	for (i = 0 ; i < G_N_ELEMENTS (missing_as_strings); i++)
		if (missing_as_strings[i].n == n)
			return missing_as_strings[i].name;
	return "invalid";	/* default property value */
}

unsigned
missing_as_value (char const *name)
{
	unsigned i;
	for (i = 0 ; i < G_N_ELEMENTS (missing_as_strings); i++)
		if (!strcmp (missing_as_strings[i].name, name))
			return missing_as_strings[i].n;
	return 0;	/* default property value */
}

/*****************************************************************************/

enum {
	XYZ_SURFACE_PROP_0,
	XYZ_SURFACE_PROP_ROWS,
	XYZ_SURFACE_PROP_COLUMNS,
	XYZ_SURFACE_PROP_AUTO_ROWS,
	XYZ_SURFACE_PROP_AUTO_COLUMNS,
	XYZ_SURFACE_PROP_EXTRA1
};

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
gog_xyz_matrix_plot_build_matrix (GogXYZPlot *plot, gboolean *cardinality_changed)
{
	unsigned i, j, k, l, index;
	GogSeries *series = GOG_SERIES (plot->base.series->data);
	const double *x_vals, *y_vals, *z_vals = NULL;
	double *x_limits, *y_limits, zmin = DBL_MAX, zmax = -DBL_MAX;
	double *data;
	unsigned *grid, n, kmax, imax, jmax;
	xyz_data raw_data;
	unsigned *sort;
	gboolean is_3d = GOG_PLOT (plot)->desc.series.num_dim == 3;

	if (GOG_IS_XYZ_MATRIX_PLOT (plot)) {
		GogXYZMatrixPlot *xyz = GOG_XYZ_MATRIX_PLOT (plot);
		if (!plot->auto_x && xyz->grid[0].data) {
			if (xyz->base.x_vals)
				g_object_unref (xyz->base.x_vals);
			xyz->base.x_vals = g_object_ref (xyz->grid[0].data);
			xyz->base.columns = go_data_get_vector_size (plot->x_vals) - 1;
		}
		if (!plot->auto_y && xyz->grid[1].data) {
			if (xyz->base.y_vals)
				g_object_unref (xyz->base.y_vals);
			xyz->base.y_vals = g_object_ref (xyz->grid[1].data);
			xyz->base.rows = go_data_get_vector_size (plot->y_vals) - 1;
		}
	} else {
		GogXYMatrixPlot *xyz = GOG_XY_MATRIX_PLOT (plot);
		if (!plot->auto_x && xyz->grid[0].data) {
			if (xyz->base.x_vals)
				g_object_unref (xyz->base.x_vals);
			xyz->base.x_vals = g_object_ref (xyz->grid[0].data);
			xyz->base.columns = go_data_get_vector_size (plot->x_vals) - 1;
		}
		if (!plot->auto_y && xyz->grid[1].data) {
			if (xyz->base.y_vals)
				g_object_unref (xyz->base.y_vals);
			xyz->base.y_vals = g_object_ref (xyz->grid[1].data);
			xyz->base.rows = go_data_get_vector_size (plot->y_vals) - 1;
		}
	}
	n = plot->rows * plot->columns;
	if (plot->rows < 2 || plot->columns < 2) {
		/* we store the default value to avoid warnings about invalid values */
		if (plot->rows < 2)
			((GogXYZPlot *) plot)->rows = 10;
		if (plot->columns < 2)
			((GogXYZPlot *) plot)->columns = 10;
		return NULL;
	}
	x_limits = go_range_sort (go_data_get_values (gog_xyz_plot_get_x_vals ((GogXYZPlot *) plot)), plot->columns + 1);
	y_limits = go_range_sort (go_data_get_values (gog_xyz_plot_get_y_vals ((GogXYZPlot *) plot)), plot->rows + 1);
	if (GOG_IS_XY_MATRIX_PLOT (plot)) {
		kmax = gog_series_get_xy_data (GOG_SERIES (series), &x_vals, &y_vals);
	} else
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
	imax = plot->rows + 1;
	jmax = plot->columns + 1;

	data = g_new0 (double, n);
	grid = g_new0 (unsigned, n);

	l = index = 0;
	while (l < kmax && y_vals && y_vals[sort[l]] < y_limits[0])
		l++;
	k = l;
	for (i = 1; i < imax; i++) {
		while (l < kmax && y_vals && y_vals[sort[l]] <= y_limits[i])
			l++;
		g_qsort_with_data (sort + k, l - k, sizeof (unsigned), (GCompareDataFunc) data_compare, &raw_data);
		while (k < l && x_vals && x_vals[sort[k]] < x_limits[0])
			k++;
		for (j = 1; j < jmax && k < l; j++) {
			index = (i - 1) * plot->columns + j - 1;
			while (k < l && x_vals && x_vals[sort[k]] <= x_limits[j]) {
				if (G_LIKELY (index < n)) {
					data[index] += (z_vals)? z_vals[sort[k]]: 1;
					grid[index]++;
				}
				k++;
			}
		}
		k = l;
	}
	if (is_3d) {
		for (k = 0; k < n; ++k) {
			if (grid[k] != 0)
				data[k] /= grid[k];
			else if (GOG_XYZ_MATRIX_PLOT (plot)->missing_as == XYZ_SURFACE_MISSING_AS_NAN)
				data[k] = go_nan;
		}
	} else if (GOG_XY_MATRIX_PLOT (plot)->as_density) {
		double width, height[jmax];
		for (j = 1; j < jmax; j++)
			height[j] = y_limits[j] - y_limits[j - 1];
		for (i = 1; i < imax; i++) {
			width = x_limits[i] - x_limits[i - 1];
			for (j = 1; j < jmax; j++)
				data [(i - 1) * plot->columns + j - 1] /= width * height[j];
		}
	}
	if (cardinality_changed != NULL)
		*cardinality_changed = FALSE;
	g_free (x_limits);
	g_free (y_limits);
	g_free (sort);
	g_free (grid);

	for (k = 0; k < n; ++k)
		if (go_finite (data[k])) {
			if (zmin > data[k])
				zmin = data[k];
			if (zmax < data[k])
				zmax = data[k];
		}
	((GogXYZPlot *) plot)->z.minima = zmin;
	((GogXYZPlot *) plot)->z.maxima = zmax;
	return data;
}

static double *
gog_xyz_surface_plot_build_matrix (GogXYZPlot *plot, gboolean *cardinality_changed)
{
	unsigned i, j, k, l, index;
	GogSeries *series = GOG_SERIES (plot->base.series->data);
	const double *x_vals, *y_vals, *z_vals = NULL;
	double *x_limits, *y_limits, xmin, ymin, zmin = DBL_MAX, zmax = -DBL_MAX;
	double *data;
	unsigned *grid, n, kmax, imax, jmax;
	xyz_data raw_data;
	unsigned *sort;
	gboolean is_3d = GOG_PLOT (plot)->desc.series.num_dim == 3;
	if (GOG_IS_CONTOUR_PLOT (plot)) {
		if (is_3d) {
			GogXYZContourPlot *xyz = GOG_XYZ_CONTOUR_PLOT (plot);
			if (!plot->auto_x && xyz->grid[0].data) {
				if (plot->x_vals)
					g_object_unref (plot->x_vals);
				plot->x_vals = g_object_ref (xyz->grid[0].data);
				plot->columns = go_data_get_vector_size (plot->x_vals);
			}
			if (!plot->auto_y && xyz->grid[1].data) {
				if (plot->y_vals)
					g_object_unref (plot->y_vals);
				plot->y_vals = g_object_ref (xyz->grid[1].data);
				plot->rows = go_data_get_vector_size (plot->y_vals);
			}
		} else {
			GogXYContourPlot *xyz = GOG_XY_CONTOUR_PLOT (plot);
			if (!plot->auto_x && xyz->grid[0].data) {
				if (plot->x_vals)
					g_object_unref (plot->x_vals);
				plot->x_vals = g_object_ref (xyz->grid[0].data);
				plot->columns = go_data_get_vector_size (plot->x_vals);
			}
			if (!plot->auto_y && xyz->grid[1].data) {
				if (plot->y_vals)
					g_object_unref (plot->y_vals);
				plot->y_vals = g_object_ref (xyz->grid[1].data);
				plot->rows = go_data_get_vector_size (plot->y_vals);
			}
		}
	} else {
		if (is_3d) {
			GogXYZSurfacePlot *xyz = GOG_XYZ_SURFACE_PLOT (plot);
			if (!plot->auto_x && xyz->grid[0].data) {
				if (xyz->base.x_vals)
					g_object_unref (xyz->base.x_vals);
				xyz->base.x_vals = g_object_ref (xyz->grid[0].data);
				xyz->base.columns = go_data_get_vector_size (plot->x_vals);
			}
			if (!plot->auto_y && xyz->grid[1].data) {
				if (xyz->base.y_vals)
					g_object_unref (xyz->base.y_vals);
				xyz->base.y_vals = g_object_ref (xyz->grid[1].data);
				xyz->base.rows = go_data_get_vector_size (plot->y_vals);
			}
		} else {
			GogXYSurfacePlot *xyz = GOG_XY_SURFACE_PLOT (plot);
			if (!plot->auto_x && xyz->grid[0].data) {
				if (xyz->base.x_vals)
					g_object_unref (xyz->base.x_vals);
				xyz->base.x_vals = g_object_ref (xyz->grid[0].data);
				xyz->base.columns = go_data_get_vector_size (plot->x_vals);
			}
			if (!plot->auto_y && xyz->grid[1].data) {
				if (xyz->base.y_vals)
					g_object_unref (xyz->base.y_vals);
				xyz->base.y_vals = g_object_ref (xyz->grid[1].data);
				xyz->base.rows = go_data_get_vector_size (plot->y_vals);
			}
		}
	}
	n = plot->rows * plot->columns;
	if (plot->rows < 2 || plot->columns < 2) {
		/* we store the default value to avoid warnings about invalid values */
		if (plot->rows < 2)
			((GogXYZPlot *) plot)->rows = 10;
		if (plot->columns < 2)
			((GogXYZPlot *) plot)->columns = 10;
		return NULL;
	}
	x_limits = go_range_sort (go_data_get_values (gog_xyz_plot_get_x_vals ((GogXYZPlot *) plot)), plot->columns);
	/* we now take a symetric interval around first bin */
	xmin = (3. * x_limits[0] - x_limits[1]) / 2.;
	for (i = 0; i < plot->columns - 1; i++)
		x_limits[i] = (x_limits[i] + x_limits[i+1]) / 2.;
	/* we now take a symetric interval around last bin */
	x_limits[i] = 2 * x_limits[i] - x_limits[i - 1];
	y_limits = go_range_sort (go_data_get_values (gog_xyz_plot_get_y_vals ((GogXYZPlot *) plot)), plot->rows);
	/* another symetric interval */
	ymin = (3. * y_limits[0] - y_limits[1]) / 2.;
	for (i = 0; i < plot->rows - 1; i++)
		y_limits[i] = (y_limits[i] + y_limits[i+1]) / 2.;
	/* still another symetric interval */
	y_limits[i] = 2 * y_limits[i] - y_limits[i - 1];
	if (GOG_PLOT (plot)->desc.series.num_dim == 3)
		kmax = gog_series_get_xyz_data (GOG_SERIES (series),
							 &x_vals, &y_vals, &z_vals);
	else
		kmax = gog_series_get_xy_data (GOG_SERIES (series), &x_vals, &y_vals);
	if (x_vals == NULL || y_vals == NULL)
		return NULL;
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

	data = g_new0 (double, n);
	grid = g_new0 (unsigned, n);

	k = 0;
	while (k < kmax && y_vals[sort[k]] < ymin)
		k++;
	l = k;
	for (i = 0; i < imax; i++) {
		while (l < kmax && y_vals[sort[l]] < y_limits[i])
			l++;
		g_qsort_with_data (sort + k, l - k, sizeof (unsigned), (GCompareDataFunc) data_compare, &raw_data);
		while (k < l && x_vals[sort[k]] < xmin)
			k++;
		for (j = 0; j < jmax && k < l; j++) {
			index = i * jmax + j;
			while (k < l && x_vals[sort[k]] < x_limits[j]) {
				if (G_LIKELY (index < n)) {
					data[index] += (z_vals)? z_vals[sort[k]]: 1;
					grid[index]++;
				}
				k++;
			}
		}
		k = l;
	}

	if (is_3d) {
		for (k = 0; k < n; ++k)
			if (grid[k] != 0)
				data[k] /= grid[k];
			else if (GOG_IS_CONTOUR_PLOT (plot)?
				     	GOG_XYZ_CONTOUR_PLOT (plot)->missing_as == XYZ_SURFACE_MISSING_AS_NAN:
					    GOG_XYZ_SURFACE_PLOT (plot)->missing_as == XYZ_SURFACE_MISSING_AS_NAN)
				data[k] = go_nan;
	} else {
		gboolean as_density;
		g_object_get (G_OBJECT (plot), "as-density", &as_density, NULL);
		if (as_density) {
			double width[imax], height[jmax];
			width[0] = x_limits[0] - xmin;
			for (i = 1; i < imax; i++)
				width[i] = x_limits[i] - x_limits[i - 1];
			height[0] = y_limits[0] - ymin;
			for (j = 1; j < jmax; j++)
				height[j] = y_limits[j] - y_limits[j - 1];
			for (i = 1; i < imax; i++) {
				for (j = 1; j < jmax; j++)
					data [(i - 1) * plot->columns + j - 1] /= width[i] * height[j];
			}
		}
	}


	g_free (x_limits);
	g_free (y_limits);
	g_free (sort);
	g_free (grid);

	for (k = 0; k < n; ++k)
		if (go_finite (data[k])) {
			if (zmin > data[k])
				zmin = data[k];
			if (zmax < data[k])
				zmax = data[k];
		}
	((GogXYZPlot *) plot)->z.minima = zmin;
	((GogXYZPlot *) plot)->z.maxima = zmax;

	if (GOG_IS_CONTOUR_PLOT (plot)) {
		GogAxisMap *map;
		GogAxisTick *zticks;
		GogAxis *axis = plot->base.axis[GOG_AXIS_PSEUDO_3D];
		unsigned nticks;
		double *x, val, minimum, maximum, slope, offset = 0.;
		unsigned max;
		gboolean has_scale = gog_axis_get_color_scale (axis) != NULL;

		if (!gog_axis_get_bounds (axis, &minimum, &maximum)) {
			series->num_elements = has_scale? 1: 2; /* series name and one slice */
			if (cardinality_changed)
				*cardinality_changed = TRUE;
			return data;
		}
		nticks = gog_axis_get_ticks (axis, &zticks);
		map = gog_axis_map_new (axis, 0, 1);
		x = g_new (double, nticks);
		if (nticks > 1) {
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
		} else {
			slope = 0.;
			max = 1;
			x[0] = 0.; /* is this needed? */
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
		if ((has_scale && series->num_elements != 1) || series->num_elements != max) {
			series->num_elements = has_scale? 1: max;
			if (cardinality_changed)
				*cardinality_changed = TRUE;
		}
		gog_axis_map_free (map);
		g_free (x);
		if (max < 2) { /* this might happen with bad 3d axis configuration */
			g_free (data);
			data = NULL;
		} else
			GOG_CONTOUR_PLOT (plot)->max_colors = max;
	} else if (cardinality_changed != NULL)
		*cardinality_changed = FALSE;

	return data;
}

static char const *
gog_xyz_contour_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name contour plot objects with (x,y,z) data
	*/
	return N_("PlotXYZContour");
}

static char const *
gog_xyz_matrix_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name matrix plot objects with (x,y,z) data
	*/
	return N_("PlotXYZMatrix");
}

static char const *
gog_xyz_surface_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name surface plot objects with (x,y,z) data
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
	GObjectClass *klass = g_type_class_peek_parent (G_OBJECT_GET_CLASS (item));
	GtkWidget *w = gog_xyz_surface_plot_pref (GOG_XYZ_PLOT (item), dalloc, cc);
	go_editor_add_page (editor, w, _("Properties"));
	g_object_unref (w);

	(GOG_OBJECT_CLASS (klass)->populate_editor) (item, editor, dalloc, cc);
}
#endif

static void
gog_xyz_surface_plot_update (GogObject *obj)
{
	GogXYZPlot *model = GOG_XYZ_PLOT(obj);
	GogXYZSeries *series;
	double tmp_min, tmp_max;
	GogObjectClass *klass = g_type_class_peek_parent (G_OBJECT_GET_CLASS (obj));
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

	if (GOG_PLOT (model)->desc.series.num_dim > 2 && model->z.fmt == NULL)
			model->z.fmt = go_data_preferred_fmt (series->base.values[2].data);
	g_free (model->plotted_data);
	model->plotted_data = gog_xyz_plot_build_matrix (model, NULL);
	if (model->plotted_data) {
		gog_axis_bound_changed (model->base.axis[GOG_XYZ_PLOT_GET_CLASS (model)->third_axis],
		        	            GOG_OBJECT (model));
	}

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
	case XYZ_SURFACE_PROP_EXTRA1:
		if (GOG_IS_XYZ_CONTOUR_PLOT (plot))
			GOG_XYZ_CONTOUR_PLOT (plot)->missing_as = missing_as_value (g_value_get_string (value));
		else if (GOG_IS_XYZ_MATRIX_PLOT (plot))
			GOG_XYZ_MATRIX_PLOT (plot)->missing_as = missing_as_value (g_value_get_string (value));
		else if (GOG_IS_XYZ_SURFACE_PLOT (plot))
			GOG_XYZ_SURFACE_PLOT (plot)->missing_as = missing_as_value (g_value_get_string (value));
		else if (GOG_IS_XY_CONTOUR_PLOT (plot))
			GOG_XY_CONTOUR_PLOT (plot)->as_density = g_value_get_boolean (value);
		else if (GOG_IS_XY_MATRIX_PLOT (plot))
			GOG_XY_MATRIX_PLOT (plot)->as_density = g_value_get_boolean (value);
		else
			GOG_XY_SURFACE_PLOT (plot)->as_density = g_value_get_boolean (value);
		gog_object_request_update (GOG_OBJECT (plot));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_request_update (GOG_OBJECT (obj));
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
	case XYZ_SURFACE_PROP_EXTRA1 :
		if (GOG_PLOT (plot)->desc.series.num_dim == 2) {
			if (GOG_IS_CONTOUR_PLOT (plot))
				g_value_set_boolean (value, GOG_XY_CONTOUR_PLOT (plot)->as_density);
			else if (GOG_IS_MATRIX_PLOT (plot))
				g_value_set_boolean (value, GOG_XY_MATRIX_PLOT (plot)->as_density);
			else
				g_value_set_boolean (value, GOG_XY_SURFACE_PLOT (plot)->as_density);
		} else {
			if (GOG_IS_CONTOUR_PLOT (plot))
				g_value_set_string (value, missing_as_string (GOG_XYZ_CONTOUR_PLOT (plot)->missing_as));
			else if (GOG_IS_MATRIX_PLOT (plot))
				g_value_set_string (value, missing_as_string (GOG_XYZ_MATRIX_PLOT (plot)->missing_as));
			else
				g_value_set_string (value, missing_as_string (GOG_XYZ_SURFACE_PLOT (plot)->missing_as));
		}
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
common_init_class (GogXYZPlotClass *klass, gboolean is_3d)
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
			_("Whether the rows limits should be evaluated"),
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
			_("Whether the columns limits should be evaluated"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	if (is_3d) {
		g_object_class_install_property (gobject_klass, XYZ_SURFACE_PROP_EXTRA1,
			g_param_spec_string ("missing-as",
				_("Missing as"),
				_("How to deal with missing data"),
				"invalid",
				GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
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
	} else {
		g_object_class_install_property (gobject_klass, XYZ_SURFACE_PROP_EXTRA1,
			g_param_spec_boolean ("as-density",
				_("As density"),
				_("Display the data as density instead or raw data"),
				TRUE,
				GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
		{
			static GogSeriesDimDesc dimensions[] = {
				{ N_("X"), GOG_SERIES_REQUIRED, FALSE,
				  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
				{ N_("Y"), GOG_SERIES_REQUIRED, FALSE,
				  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
			};
			gog_plot_klass->desc.series.dim = dimensions;
			gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		}
	}

	gog_object_klass->update	= gog_xyz_surface_plot_update;

#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor	= gog_xyz_surface_plot_populate_editor;
#endif

}

static void
gog_xyz_contour_plot_class_init (GogXYZPlotClass *klass)
{
	GOG_OBJECT_CLASS (klass)->type_name = gog_xyz_contour_plot_type_name;
	common_init_class (klass, TRUE);
	klass->build_matrix = gog_xyz_surface_plot_build_matrix;
}

static void
gog_xyz_matrix_plot_class_init (GogXYZPlotClass *klass)
{
	GOG_OBJECT_CLASS (klass)->type_name = gog_xyz_matrix_plot_type_name;
	common_init_class (klass, TRUE);
	klass->build_matrix = gog_xyz_matrix_plot_build_matrix;
}

static void
gog_xyz_surface_plot_class_init (GogXYZPlotClass *klass)
{
	GOG_OBJECT_CLASS (klass)->type_name = gog_xyz_surface_plot_type_name;
	common_init_class (klass, TRUE);
	klass->build_matrix = gog_xyz_surface_plot_build_matrix;
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
gog_xyz_matrix_plot_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogXYZMatrixPlot *plot = GOG_XYZ_MATRIX_PLOT (set);
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
	/* we need to update cardinality for contour plots */
	gog_plot_request_cardinality_update (GOG_PLOT (set));
}

static void
gog_xyz_contour_plot_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_xyz_contour_plot_dataset_get_elem;
	iface->dims	   = gog_xyz_surface_plot_dataset_dims;
	iface->dim_changed = gog_xyz_surface_plot_dataset_dim_changed;
}

static void
gog_xyz_matrix_plot_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_xyz_matrix_plot_dataset_get_elem;
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

GSF_DYNAMIC_CLASS_FULL (GogXYZMatrixPlot, gog_xyz_matrix_plot,
	NULL, NULL, gog_xyz_matrix_plot_class_init, NULL,
        gog_xyz_surface_plot_init, GOG_TYPE_MATRIX_PLOT, 0,
        GSF_INTERFACE (gog_xyz_matrix_plot_dataset_init, GOG_TYPE_DATASET))

GSF_DYNAMIC_CLASS_FULL (GogXYZSurfacePlot, gog_xyz_surface_plot,
	NULL, NULL, gog_xyz_surface_plot_class_init, NULL,
        gog_xyz_surface_plot_init, GOG_TYPE_SURFACE_PLOT, 0,
        GSF_INTERFACE (gog_xyz_surface_plot_dataset_init, GOG_TYPE_DATASET))

/*-----------------------------------------------------------------------------
 *
 *  GogXYContourPlot
 *
 *-----------------------------------------------------------------------------
 */

static char const *
gog_xy_contour_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name contour plot objects with (x,y) data
	*/
	return N_("PlotXYMatrix");
}

static void
gog_xy_contour_plot_class_init (GogXYZPlotClass *klass)
{
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;

	common_init_class (klass, FALSE);
	/* Fill in GogObject superclass values */
	gog_object_klass->type_name	= gog_xy_contour_plot_type_name;

	klass->build_matrix = gog_xyz_surface_plot_build_matrix;
}

static void
gog_xy_contour_plot_init (GogXYMatrixPlot *plot)
{
	gog_xyz_surface_plot_init (&plot->base);
	plot->as_density = TRUE;
}

static GogDatasetElement *
gog_xy_contour_plot_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogXYContourPlot *plot = GOG_XY_CONTOUR_PLOT (set);
	g_return_val_if_fail (2 > dim_i, NULL);
	g_return_val_if_fail (dim_i >= 0, NULL);
	return plot->grid + dim_i;
}

static void
gog_xy_contour_plot_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_xy_contour_plot_dataset_get_elem;
	iface->dims	   = gog_xyz_surface_plot_dataset_dims;
	iface->dim_changed = gog_xyz_surface_plot_dataset_dim_changed;
}

GSF_DYNAMIC_CLASS_FULL (GogXYContourPlot, gog_xy_contour_plot,
	NULL, NULL, gog_xy_contour_plot_class_init, NULL,
        gog_xy_contour_plot_init, GOG_TYPE_CONTOUR_PLOT, 0,
        GSF_INTERFACE (gog_xy_contour_plot_dataset_init, GOG_TYPE_DATASET))

/*-----------------------------------------------------------------------------
 *
 *  GogXYMatrixPlot
 *
 *-----------------------------------------------------------------------------
 */

static char const *
gog_xy_matrix_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name matrix plot objects with (x,y) data
	*/
	return N_("PlotXYMatrix");
}

static void
gog_xy_matrix_plot_class_init (GogXYZPlotClass *klass)
{
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;

	common_init_class (klass, FALSE);
	/* Fill in GogObject superclass values */
	gog_object_klass->type_name	= gog_xy_matrix_plot_type_name;

	klass->build_matrix = gog_xyz_matrix_plot_build_matrix;
}

static void
gog_xy_matrix_plot_init (GogXYMatrixPlot *plot)
{
	gog_xyz_surface_plot_init (&plot->base);
	plot->as_density = TRUE;
}

static GogDatasetElement *
gog_xy_matrix_plot_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogXYMatrixPlot *plot = GOG_XY_MATRIX_PLOT (set);
	g_return_val_if_fail (2 > dim_i, NULL);
	g_return_val_if_fail (dim_i >= 0, NULL);
	return plot->grid + dim_i;
}

static void
gog_xy_matrix_plot_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_xy_matrix_plot_dataset_get_elem;
	iface->dims	   = gog_xyz_surface_plot_dataset_dims;
	iface->dim_changed = gog_xyz_surface_plot_dataset_dim_changed;
}

GSF_DYNAMIC_CLASS_FULL (GogXYMatrixPlot, gog_xy_matrix_plot,
	NULL, NULL, gog_xy_matrix_plot_class_init, NULL,
        gog_xy_matrix_plot_init, GOG_TYPE_MATRIX_PLOT, 0,
        GSF_INTERFACE (gog_xy_matrix_plot_dataset_init, GOG_TYPE_DATASET))

/*-----------------------------------------------------------------------------
 *
 *  GogXYSurfacePlot
 *
 *-----------------------------------------------------------------------------
 */

static char const *
gog_xy_surface_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name surface plots objects with (x,y) data
	*/
	return N_("PlotXYSurface");
}

static void
gog_xy_surface_plot_class_init (GogXYZPlotClass *klass)
{
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;

	common_init_class (klass, FALSE);
	/* Fill in GogObject superclass values */
	gog_object_klass->type_name	= gog_xy_surface_plot_type_name;

	klass->build_matrix = gog_xyz_surface_plot_build_matrix;
}

static void
gog_xy_surface_plot_init (GogXYMatrixPlot *plot)
{
	gog_xyz_surface_plot_init (&plot->base);
	plot->as_density = TRUE;
}

static GogDatasetElement *
gog_xy_surface_plot_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogXYSurfacePlot *plot = GOG_XY_SURFACE_PLOT (set);
	g_return_val_if_fail (2 > dim_i, NULL);
	g_return_val_if_fail (dim_i >= 0, NULL);
	return plot->grid + dim_i;
}

static void
gog_xy_surface_plot_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_xy_surface_plot_dataset_get_elem;
	iface->dims	   = gog_xyz_surface_plot_dataset_dims;
	iface->dim_changed = gog_xyz_surface_plot_dataset_dim_changed;
}

GSF_DYNAMIC_CLASS_FULL (GogXYSurfacePlot, gog_xy_surface_plot,
	NULL, NULL, gog_xy_surface_plot_class_init, NULL,
        gog_xy_surface_plot_init, GOG_TYPE_SURFACE_PLOT, 0,
        GSF_INTERFACE (gog_xy_surface_plot_dataset_init, GOG_TYPE_DATASET))
