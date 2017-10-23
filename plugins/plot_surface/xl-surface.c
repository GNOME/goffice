/*
 * xl-surface.c
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
#include "xl-surface.h"

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#include <goffice/data/go-data-simple.h>
#include <goffice/graph/gog-axis.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-format.h>

typedef GogSeries XLXYZSeries;
typedef GogSeriesClass XLXYZSeriesClass;

#define XL_XYZ_SERIES_TYPE	(xl_xyz_series_get_type ())
#define XL_XYZ_SERIES(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), XL_XYZ_SERIES_TYPE, XLXYZSeries))
#define XL_IS_XYZ_SERIES(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), XL_XYZ_SERIES_TYPE))

static GType xl_xyz_series_get_type (void);

static GogStyledObjectClass *series_parent_klass;

static void
xl_xyz_series_update (GogObject *obj)
{
	XLXYZSeries *series = XL_XYZ_SERIES (obj);
	int x_len = 0, z_len = 0;

	if (series->values[1].data != NULL)
		z_len = go_data_get_vector_size (series->values[1].data);
	if (series->values[0].data != NULL)
		x_len = go_data_get_vector_size (series->values[0].data);
	else
		x_len = z_len;
	series->num_elements = MIN (x_len, z_len);

	/* queue plot for redraw */
	gog_object_request_update (GOG_OBJECT (series->plot));

	if (series_parent_klass->base.update)
		series_parent_klass->base.update (obj);
}

static void
xl_xyz_series_init_style (GogStyledObject *gso, GOStyle *style)
{
	series_parent_klass->init_style (gso, style);
	style->interesting_fields = 0;  /* a quick hack to hide the style in the guru */
}

static void
xl_xyz_series_class_init (GogStyledObjectClass *gso_klass)
{
	GogObjectClass * obj_klass = (GogObjectClass *) gso_klass;

	series_parent_klass = g_type_class_peek_parent (gso_klass);
	gso_klass->init_style = xl_xyz_series_init_style;
	obj_klass->update = xl_xyz_series_update;
}

static void
xl_xyz_series_init (GogSeries *series)
{
	series->has_legend = FALSE;
}

GSF_DYNAMIC_CLASS (XLXYZSeries, xl_xyz_series,
	xl_xyz_series_class_init, xl_xyz_series_init,
	GOG_TYPE_SERIES)

/*****************************************************************************/

static void
xl_xyz_plot_update (GogObject *obj)
{
	GogXYZPlot * model = GOG_XYZ_PLOT(obj);
	XLXYZSeries * series;
	double zmin =  DBL_MAX, zmax = -DBL_MAX, tmp_min, tmp_max;
	GSList *ptr;
	model->rows = 0;
	model->columns = 0;

	ptr = model->base.series;
	series = ptr->data;
	while (!gog_series_is_valid (GOG_SERIES (series))) {
		ptr = ptr->next;
		if (!ptr)
			return;
		series = ptr->data;
	}
	/* for first series, num_elements is used for zaxis, so we
	can't use it to evaluate model->columns */
	if (series->values[1].data != NULL) {
		model->columns = go_data_get_vector_size (series->values[1].data);
		if (series->values[0].data != NULL)
			model->rows = go_data_get_vector_size (series->values[0].data);
		if (model->rows < model->columns)
			model->columns = model->rows;
	} else if (series->values[0].data != NULL)
		model->columns = go_data_get_vector_size (series->values[0].data);
	model->rows = 1;

	for (ptr = ptr->next; ptr != NULL; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;
		if (series->num_elements > model->columns)
			model->columns = series->num_elements;
		model->rows++;
		go_data_get_bounds (series->values[1].data, &tmp_min, &tmp_max);
		if (zmin > tmp_min) zmin = tmp_min;
		if (zmax < tmp_max) zmax = tmp_max;
	}
	g_free (model->plotted_data);
	model->plotted_data = NULL;
	if ((zmin != model->z.minima)
			|| (zmax != model->z.maxima)) {
		model->z.minima = zmin;
		model->z.maxima = zmax;
		gog_axis_bound_changed (model->base.axis[GOG_XYZ_PLOT_GET_CLASS (model)->third_axis], obj);
	} else
		gog_plot_update_3d (GOG_PLOT (model));

	gog_axis_bound_changed (model->base.axis[GOG_AXIS_X], obj);
	gog_axis_bound_changed (model->base.axis[GOG_AXIS_Y], obj);
}

static GOData *
get_y_vector (GogPlot *plot)
{
	GSList *ptr;
	int i;
	char const ***y_labels = (GOG_IS_CONTOUR_PLOT (plot))?
				&(XL_CONTOUR_PLOT (plot)->y_labels):
				&(XL_SURFACE_PLOT (plot)->y_labels);

	g_free (*y_labels);
	*y_labels = g_new0 (char const *, GOG_XYZ_PLOT (plot)->rows);

	for (ptr = plot->series, i = 0 ; ptr != NULL ; ptr = ptr->next) {
		XLXYZSeries *series = ptr->data;

		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;
		(*y_labels) [i] = (series->values[-1].data)?
				go_data_get_scalar_string (series->values[-1].data):
				g_strdup_printf("S%d", i + 1); /* excel like labels */
		i++;
	}

	return GO_DATA (go_data_vector_str_new (*y_labels, i, g_free));
}

static GOData *
xl_xyz_plot_axis_get_bounds (GogPlot *plot, GogAxisType axis,
			     GogPlotBoundInfo * bounds)
{
	GogXYZPlot *xyz = GOG_XYZ_PLOT (plot);
	GOData *vec = NULL;
	GOFormat const *fmt;

	if (axis == GOG_AXIS_X) {
		XLXYZSeries *series = XL_XYZ_SERIES (plot->series->data);
		vec = series->values[0].data;
		fmt = xyz->x.fmt;
	} else if (axis == GOG_AXIS_Y) {
		if (!xyz->rows)
			return NULL;
		vec = get_y_vector (plot);
		fmt = xyz->y.fmt;
	} else {
		if (bounds->fmt == NULL && xyz->z.fmt != NULL)
			bounds->fmt = go_format_ref (xyz->z.fmt);
		bounds->val.minima = xyz->z.minima;
		bounds->val.maxima = xyz->z.maxima;
		return NULL;
	}
	if (bounds->fmt == NULL && fmt != NULL)
		bounds->fmt = go_format_ref (fmt);
	bounds->val.minima = 1.;
	bounds->logical.minima = 1.;
	bounds->logical.maxima = go_nan;
	bounds->is_discrete    = TRUE;
	bounds->center_on_ticks = TRUE;
	bounds->val.maxima = (axis == GOG_AXIS_X)?
		xyz->columns:
		xyz->rows;
	return (GOData*) vec;
}
/*****************************************************************************/

static GogObjectClass *xl_contour_parent_klass;

typedef GogContourPlotClass XLContourPlotClass;

static double *
xl_contour_plot_build_matrix (GogXYZPlot *plot, gboolean *cardinality_changed)
{
	unsigned i, j, length;
	GogAxisMap *map;
	GogAxisTick *zticks;
	GogAxis *axis = plot->base.axis[GOG_AXIS_PSEUDO_3D];
	unsigned nticks;
	double x[2], val;
	GogSeries *series = NULL;
	GOData *vec;
	unsigned n = plot->rows * plot->columns;
	double *data, minimum, maximum;
	unsigned max;
	GSList *ptr;
	gboolean has_scale = gog_axis_get_color_scale (axis) != NULL;

	if (!gog_axis_get_bounds (axis, &minimum, &maximum))
		return NULL;
	data = g_new (double, n);
	nticks = gog_axis_get_ticks (axis, &zticks);
	map = gog_axis_map_new (axis, 0, 1);
	for (i = j = 0; i < nticks; i++)
		if (zticks[i].type == GOG_AXIS_TICK_MAJOR) {
			x[j++] = gog_axis_map_to_view (map, zticks[i].position);
			if (j > 1)
				break;
		}
	x[1] -= x[0];

	for (i = 0, ptr = plot->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;
		vec = series->values[1].data;
		length = go_data_get_vector_size (vec);
		for (j = 0; j < plot->columns; j++) {
			/* The vector might be too short, excel is so ugly ;-) */
			val = (j < length)? gog_axis_map_to_view (map,
					go_data_get_vector_value (vec, j)): 0.;
			/* This is an excel compatible plot, so let's be compatible */
			if (val == go_nan || !go_finite (val))
				val = 0.;
			if (fabs (val) == DBL_MAX)
				val = go_nan;
			else {
				val = val/ x[1] - x[0];
				if (val < 0) {
					val = go_nan;
				}
			}
			data[i * plot->columns + j] = val;
		}
		i++;
	}
	g_return_val_if_fail (series != NULL, NULL);
	max = (unsigned) ceil (1 / x[1]);
	series = plot->base.series->data;
	if ((has_scale && series->num_elements != 1) || series->num_elements != max) {
		series->num_elements = has_scale? 1: max;
		*cardinality_changed = TRUE;
	}
	GOG_CONTOUR_PLOT (plot)->max_colors = max;
	gog_axis_map_free (map);
	return data;
}

static void
xl_contour_plot_finalize (GObject *obj)
{
	XLContourPlot *plot = XL_CONTOUR_PLOT (obj);
	g_free (plot->y_labels);
	G_OBJECT_CLASS (xl_contour_parent_klass)->finalize (obj);
}

static void
xl_contour_plot_class_init (GogContourPlotClass *klass)
{
	GogPlotClass *gog_plot_klass = (GogPlotClass*) klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;
	GObjectClass   *gobject_klass = (GObjectClass *) klass;

	xl_contour_parent_klass = g_type_class_peek_parent (klass);

	gobject_klass->finalize     = xl_contour_plot_finalize;

	/* Fill in GOGObject superclass values */
	gog_object_klass->update	= xl_xyz_plot_update;
	gog_object_klass->populate_editor	= NULL;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("X"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_LABEL, GOG_MS_DIM_CATEGORIES },
			{ N_("Z"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
		};
		gog_plot_klass->desc.series.dim = dimensions;
		gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		gog_plot_klass->desc.series.style_fields = 0;
	}
	/* Fill in GogPlotClass methods */
	gog_plot_klass->desc.num_series_max = G_MAXINT;
	gog_plot_klass->axis_get_bounds	= xl_xyz_plot_axis_get_bounds;
	gog_plot_klass->series_type = xl_xyz_series_get_type();

	klass->build_matrix = xl_contour_plot_build_matrix;
}

static void
xl_contour_plot_init (XLContourPlot *contour)
{
	contour->y_labels = NULL;
}

GSF_DYNAMIC_CLASS (XLContourPlot, xl_contour_plot,
	xl_contour_plot_class_init, xl_contour_plot_init,
	GOG_TYPE_CONTOUR_PLOT)

/*****************************************************************************/

static GogObjectClass *xl_surface_parent_klass;

typedef GogSurfacePlotClass XLSurfacePlotClass;

static double *
xl_surface_plot_build_matrix (GogXYZPlot *plot, gboolean *cardinality_changed)
{
	unsigned i, j, length;
	double val;
	GogSeries *series = NULL;
	GOData *vec;
	unsigned n = plot->rows * plot->columns;
	double *data;
	GSList *ptr;

	data = g_new (double, n);
	for (i = 0, ptr = plot->base.series ; ptr != NULL ; ptr = ptr->next) {
		series = ptr->data;
		if (!gog_series_is_valid (GOG_SERIES (series)))
			continue;
		vec = series->values[1].data;
		length = go_data_get_vector_size (vec);
		for (j = 0; j < plot->columns; j++) {
			/* The vector might be too short, excel is so ugly ;-) */
			val = (j < length)? go_data_get_vector_value (vec, j): 0.;
			/* This is an excel compatible plot, so let's be compatible */
			if (val == go_nan || !go_finite (val))
				val = 0.;
			if (fabs (val) == DBL_MAX)
				val = go_nan;
			data[i * plot->columns + j] = val;
		}
		i++;
	}
	*cardinality_changed = FALSE;
	return data;
}

static void
xl_surface_plot_finalize (GObject *obj)
{
	XLSurfacePlot *plot = XL_SURFACE_PLOT (obj);
	g_free (plot->y_labels);
	G_OBJECT_CLASS (xl_surface_parent_klass)->finalize (obj);
}

static void
xl_surface_plot_class_init (GogSurfacePlotClass *klass)
{
	GogPlotClass *gog_plot_klass = (GogPlotClass*) klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;
	GObjectClass   *gobject_klass = (GObjectClass *) klass;

	xl_surface_parent_klass = g_type_class_peek_parent (klass);

	gobject_klass->finalize     = xl_surface_plot_finalize;

	/* Fill in GOGObject superclass values */
	gog_object_klass->update	= xl_xyz_plot_update;
	gog_object_klass->populate_editor	= NULL;

	{
		static GogSeriesDimDesc dimensions[] = {
			{ N_("X"), GOG_SERIES_SUGGESTED, FALSE,
			  GOG_DIM_LABEL, GOG_MS_DIM_CATEGORIES },
			{ N_("Z"), GOG_SERIES_REQUIRED, FALSE,
			  GOG_DIM_VALUE, GOG_MS_DIM_VALUES },
		};
		gog_plot_klass->desc.series.dim = dimensions;
		gog_plot_klass->desc.series.num_dim = G_N_ELEMENTS (dimensions);
		gog_plot_klass->desc.series.style_fields = GO_STYLE_LINE | GO_STYLE_FILL;
	}
	/* Fill in GogPlotClass methods */
	gog_plot_klass->desc.num_series_max = G_MAXINT;
	gog_plot_klass->axis_get_bounds	= xl_xyz_plot_axis_get_bounds;
	gog_plot_klass->series_type = xl_xyz_series_get_type();

	klass->build_matrix = xl_surface_plot_build_matrix;
}

static void
xl_surface_plot_init (XLSurfacePlot *contour)
{
	contour->y_labels = NULL;
}

GSF_DYNAMIC_CLASS (XLSurfacePlot, xl_surface_plot,
	xl_surface_plot_class_init, xl_surface_plot_init,
	GOG_TYPE_SURFACE_PLOT)

