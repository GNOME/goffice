/*
 * gog-contour.c
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
#include "gog-contour.h"
#include <goffice/data/go-data.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-color.h>
#include <goffice/utils/go-locale.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

#include <string.h>

#define EPSILON 1e-13

static GType gog_contour_view_get_type (void);

/*-----------------------------------------------------------------------------
 *
 *  GogContourPlot
 *
 *-----------------------------------------------------------------------------
 */

static double *
gog_contour_plot_build_matrix (GogXYZPlot *plot, gboolean *cardinality_changed)
{
	unsigned i, j;
	GogAxisMap *map;
	GogAxisTick *zticks;
	GogAxis *axis = plot->base.axis[GOG_AXIS_PSEUDO_3D];
	unsigned nticks;
	double *x, val;
	GogSeries *series = GOG_SERIES (plot->base.series->data);
	GOData *mat = series->values[2].data;
	unsigned n = plot->rows * plot->columns;
	double *data, minimum, maximum, slope, offset = 0.;
	unsigned max;
	gboolean has_scale = gog_axis_get_color_scale (axis) != NULL;

	if (!gog_axis_get_bounds (axis, &minimum, &maximum)) {
		series->num_elements = has_scale? 1: 2;
		*cardinality_changed = TRUE;
		return NULL;
	}
	data = g_new (double, n);
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

	for (i = 0; i < plot->rows; i++)
		for (j = 0; j < plot->columns; j++) {
			val = gog_axis_map_to_view (map,
					go_data_get_matrix_value (mat, i, j));
			if (fabs (val) == DBL_MAX)
				val = go_nan;
			else {
				val = offset + slope * (val - x[0]);
				if (val < 0)
					val = (val < -EPSILON)? go_nan: 0.;
			}
			if (plot->transposed)
				data[j * plot->rows + i] = val;
			else
				data[i * plot->columns + j] = val;
		}
	if (gog_series_has_legend (series))
		max++;
	if ((has_scale && series->num_elements != 1) || series->num_elements != max) {
		series->num_elements = has_scale? 1: max; /* we need to count 1 more
		 												for the series style */
		*cardinality_changed = TRUE;
	}
	GOG_CONTOUR_PLOT (plot)->max_colors = max;
	gog_axis_map_free (map);
	g_free (x);
	if (max < 2) { /* this might happen with bad 3d axis configuration */
		g_free (data);
		return NULL;
	}

	return data;
}

static char const *
gog_contour_plot_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name contour plot objects
	*/
	return N_("PlotContour");
}

static void
gog_contour_plot_foreach_elem  (GogPlot *plot, gboolean only_visible,
				    GogEnumFunc func, gpointer data)
{
	unsigned i, j, nticks;
	char *label;
	GOStyle *style;
	GogAxis *axis = plot->axis[GOG_AXIS_PSEUDO_3D];
	GogAxisColorMap const *map = gog_axis_get_color_map (axis);
	GogAxisTick *zticks;
	GogSeries *series = GOG_SERIES (plot->series->data);
	double *limits;
	double minimum, maximum, epsilon, scale;
	char const *separator = go_locale_get_decimal ()->str;

	/* First get the series name and style */
	style = go_style_dup (go_styled_object_get_style (plot->series->data));
	i = 0;
	if (gog_series_has_legend (series)) {
		func (0, style,
			  gog_object_get_name (plot->series->data), NULL, data);
		i = 1;
	}

	if (gog_axis_get_color_scale (axis) || !plot->vary_style_by_element) {
		g_object_unref (style);
		return;
	}
	gog_axis_get_bounds (axis, &minimum, &maximum);

	nticks = gog_axis_get_ticks (axis, &zticks);
	i = j = 0;
	epsilon = (maximum - minimum) / nticks * 1e-10; /* should avoid rounding errors */
	while (zticks[i].type != GOG_AXIS_TICK_MAJOR)
		i++;
	if (zticks[i].position - minimum > epsilon) {
		limits = g_new (double, nticks + 2);
		limits[j++] = minimum;
	} else
		limits = g_new (double, nticks + 1);
	for (; i < nticks; i++)
		if (zticks[i].type == GOG_AXIS_TICK_MAJOR)
			limits[j++] = zticks[i].position;
	if (j == 0 || maximum - limits[j - 1] > epsilon)
		limits[j] = maximum;
	else
		j--;
	scale = (j > gog_axis_color_map_get_max (map) && j > 1)? (double) gog_axis_color_map_get_max (map) / (j - 1): 1.;

	style->interesting_fields = GO_STYLE_FILL | GO_STYLE_OUTLINE;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	style->fill.pattern.pattern = GO_PATTERN_SOLID;

	if (gog_axis_is_inverted (axis)) {
		for (i = 0; i < j; i++) {
			style->fill.pattern.back = (j < 2)? GO_COLOR_WHITE: gog_axis_color_map_get_color (map, i * scale);
			label = g_strdup_printf ("[%g%s %g%c", limits[j - i - 1], separator,
						limits[j - i], (limits[j - i] - minimum > epsilon)? '[':']');
			(func) (i, style, label, NULL, data);
			g_free (label);
		}
		if (limits[i - j] - minimum > epsilon) {
			style->fill.pattern.back = (j < 2)? GO_COLOR_WHITE: gog_axis_color_map_get_color (map, i * scale);
			label = g_strdup_printf ("[%g%s %g]", minimum, separator,
						limits[i - j]);
			(func) (i + 1, style, label, NULL, data);
			g_free (label);
		}
	} else {
		if (epsilon < limits[0] - minimum) {
			style->fill.pattern.back = (j < 2)? GO_COLOR_WHITE: gog_axis_color_map_get_color (map, 0);
			label = g_strdup_printf ("[%g%s %g]", minimum, separator,
						limits[0]);
			(func) (1, style, label, NULL, data);
			g_free (label);
			i = 1;
			j++;
		} else
			i = 0;
		for (; i < j; i++) {
			style->fill.pattern.back = (j < 2)? GO_COLOR_WHITE: gog_axis_color_map_get_color (map, i * scale);
			label = g_strdup_printf ("[%g%s %g%c", limits[i], separator,
						limits[i + 1], (i == j - 1)? ']':'[');
			(func) (i + 1, style, label, NULL, data);
			g_free (label);
		}
	}
	g_free (limits);
	g_object_unref (style);
}

static void
gog_contour_plot_class_init (GogContourPlotClass *klass)
{
	GogXYZPlotClass *gog_xyz_plot_klass = (GogXYZPlotClass *) klass;
	GogPlotClass *gog_plot_klass = (GogPlotClass*) klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) klass;

	/* Fill in GogObject superclass values */
	gog_object_klass->type_name	= gog_contour_plot_type_name;
	gog_object_klass->view_type	= gog_contour_view_get_type ();

	gog_plot_klass->axis_set = GOG_AXIS_SET_XY_pseudo_3d;
	gog_plot_klass->foreach_elem = gog_contour_plot_foreach_elem;

	gog_xyz_plot_klass->third_axis = GOG_AXIS_PSEUDO_3D;
	gog_xyz_plot_klass->build_matrix = gog_contour_plot_build_matrix;
}

static void
gog_contour_plot_init (GogContourPlot *contour)
{
	GogPlot *plot = GOG_PLOT (contour);

	plot->rendering_order = GOG_PLOT_RENDERING_BEFORE_GRID;
	plot->vary_style_by_element = TRUE;
}

GSF_DYNAMIC_CLASS (GogContourPlot, gog_contour_plot,
	gog_contour_plot_class_init, gog_contour_plot_init,
	GOG_TYPE_XYZ_PLOT)

/*****************************************************************************/

typedef GogPlotView		GogContourView;
typedef GogPlotViewClass	GogContourViewClass;
#define CONTOUR_EPSILON 1e-10

static void
gog_contour_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogXYZPlot const *plot = GOG_XYZ_PLOT (view->model);
	GogSeries const *series;
	GOData *x_vec = NULL, *y_vec = NULL;
	GogAxisMap *x_map, *y_map;
	GogAxisColorMap const *color_map = gog_axis_get_color_map (gog_plot_get_axis (GOG_PLOT (view->model), GOG_AXIS_PSEUDO_3D));
	double zval0, zval1, zval2 = 0., zval3, t;
	double x[4], y[4], zval[4];
	int z[4];
	int z0 = 0, z1 = 0, z2 = 0, z3 = 0, zmin, zmax, nans, nan = 0;
	int k, kmax, r = 0, s, h;
	unsigned i, imax, j, jmax;
	GogRenderer *rend = view->renderer;
	GOStyle *style;
	double x0, x1, y0, y1;
	GOPath *path, *lines;
	GOColor *color;
	gboolean cw;
	double *data;
	int max;
	gboolean xdiscrete, ydiscrete;

	if (plot->base.series == NULL)
		return;
	series = GOG_SERIES (plot->base.series->data);
	max = GOG_CONTOUR_PLOT (plot)->max_colors;
	if (max < 1)
		return;
	if (plot->transposed) {
		imax = plot->columns;
		jmax = plot->rows;
	} else {
		imax = plot->rows;
		jmax = plot->columns;
	}
	if (imax == 0 || jmax == 0)
		return;

	if (plot->plotted_data)
		data = plot->plotted_data;
	else
		return;

	x_map = gog_axis_map_new (plot->base.axis[0],
				  view->residual.x , view->residual.w);
	y_map = gog_axis_map_new (plot->base.axis[1],
				  view->residual.y + view->residual.h,
				  -view->residual.h);

	if (!(gog_axis_map_is_valid (x_map) &&
	      gog_axis_map_is_valid (y_map))) {
		gog_axis_map_free (x_map);
		gog_axis_map_free (y_map);
		return;
	}

	/* Set cw to ensure that polygons will allways be drawn clockwise */
	xdiscrete = gog_axis_is_discrete (plot->base.axis[0]) ||
			series->values[(plot->transposed)? 1: 0].data == NULL;
	if (xdiscrete) {
		x0 = gog_axis_map_to_view (x_map, 0.);
		x1 = gog_axis_map_to_view (x_map, 1.);
	} else {
		x_vec = gog_xyz_plot_get_x_vals (GOG_XYZ_PLOT (plot));
		x0 = gog_axis_map_to_view (x_map, go_data_get_vector_value (x_vec, 0));
		x1 = gog_axis_map_to_view (x_map, go_data_get_vector_value (x_vec, 1));
	}
	ydiscrete = gog_axis_is_discrete (plot->base.axis[1]) ||
			series->values[(plot->transposed)? 0: 1].data == NULL;
	if (ydiscrete) {
		y0 = gog_axis_map_to_view (y_map, 0.);
		y1 = gog_axis_map_to_view (y_map, 1.);
	} else {
		y_vec = gog_xyz_plot_get_y_vals (GOG_XYZ_PLOT (plot));
		y0 = gog_axis_map_to_view (y_map, go_data_get_vector_value (y_vec, 0));
		y1 = gog_axis_map_to_view (y_map, go_data_get_vector_value (y_vec, 1));
	}
	cw = (x1 > x0) == (y1 > y0);

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);
	/* build the colors table */
	color = g_new0 (GOColor, max);
	if (max < 2)
		color[0] = GO_COLOR_WHITE;
	else {
		double scale = ((unsigned) max > gog_axis_color_map_get_max (color_map))? (double) gog_axis_color_map_get_max (color_map) / (max - 1): 1.;
		for (i = 0; i < (unsigned) max; i++)
			color[i] = gog_axis_color_map_get_color (color_map, i * scale);
	}

	/* clip to avoid problems with logarithmic axes */
	gog_renderer_push_clip_rectangle (rend, view->residual.x, view->residual.y,
					  view->residual.w, view->residual.h);

	style = go_style_new ();
	style->interesting_fields = GO_STYLE_FILL | GO_STYLE_OUTLINE;
	style->disable_theming = GO_STYLE_ALL;
	style->fill.type = GO_STYLE_FILL_PATTERN;
	style->fill.pattern.pattern = GO_PATTERN_SOLID;

	lines = go_path_new ();

	for (j = 1; j < jmax; j++) {
		if (xdiscrete) {
			x0 = gog_axis_map_to_view (x_map, j);
			x1 = gog_axis_map_to_view (x_map, j + 1);
		} else {
			x0 = gog_axis_map_to_view (x_map, go_data_get_vector_value (x_vec, j - 1));
			x1 = gog_axis_map_to_view (x_map, go_data_get_vector_value (x_vec, j));
		}

		for (i = 1; i < imax; i++) {
			if (ydiscrete) {
				y0 = gog_axis_map_to_view (y_map, i);
				y1 = gog_axis_map_to_view (y_map, i + 1);
			} else {
				y0 = gog_axis_map_to_view (y_map, go_data_get_vector_value (y_vec, i - 1));
				y1 = gog_axis_map_to_view (y_map, go_data_get_vector_value (y_vec, i));
			}
			nans = 0;
			nan = 4;
			zmin = max;
			zmax = 0;
			zval0 = data[(i - 1) * jmax + j - 1];
			if (go_finite (zval0)) {
				z0 = floor (zval0);
				if (z0 > zmax)
					zmax = z0;
				if (z0 < zmin) {
					zmin = z0;
					r = 0;
				}
			} else {
				nans++;
				nan = 0;
			}
			zval1 = data[(i - 1) * jmax + j];
			if (go_finite (zval1)) {
				z1 = floor (zval1);
				if (z1 > zmax)
					zmax = z1;
				if (z1 < zmin) {
					zmin = z1;
					r = 1;
				}
			} else {
				nans++;
				nan = 1;
			}
			zval2 = data[i * jmax + j];
			if (go_finite (zval2)) {
				z2 = floor (zval2);
				if (z2 > zmax)
					zmax = z2;
				if (z2 < zmin) {
					zmin = z2;
					r = 2;
				}
			} else {
				nans++;
				nan = 2;
			}
			zval3 = data[i * jmax + j - 1];
			if (go_finite (zval3)) {
				z3 = floor (zval3);
				if (z3 > zmax)
					zmax = z3;
				if (z3 < zmin) {
					zmin = z3;
					r = 3;
				}
			} else {
				nans++;
				nan = 3;
			}
			if (nans > 1)
				continue;
			/* Build the x, y and z arrays for the tile */
			k = r;
			s = 0;
			do {
				if (k != nan) {
					switch (k) {
					case 0:
						x[s] = x0;
						y[s] = y0;
						z[s] = z0;
						zval[s++] = zval0;
						break;
					case 1:
						x[s] = x1;
						y[s] = y0;
						z[s] = z1;
						zval[s++] = zval1;
						break;
					case 2:
						x[s] = x1;
						y[s] = y1;
						z[s] = z2;
						zval[s++] = zval2;
						break;
					default:
						x[s] = x0;
						y[s] = y1;
						z[s] = z3;
						zval[s++] = zval3;
					}
				}
				if (cw) {
					k++;
					k %= 4;
				} else {
					if (k == 0)
						k = 3;
					else
						k--;
				}
			} while (k != r);
			if (zmin == zmax) {
				/* paint everything with one color*/
				style->line.color = color[zmin];
				style->fill.pattern.back = color[zmin];
				gog_renderer_push_style (rend, style);
				go_path_move_to (path, x[0], y[0]);
				for (k = 1; k < s; k++)
					go_path_line_to (path, x[k], y[k]);
				/* narrow parameter is TRUE below to avoid border effects */
				gog_renderer_fill_shape (rend, path);
				go_path_clear (path);
				gog_renderer_pop_style (rend);
			} else {
				kmax = 3 - nans;
				if (!nans && (((z0 < z1) && (z1 > z2) && (z2 < z3) && (z3 > z0)) ||
					((z0 > z1) && (z1 < z2) && (z2 > z3) && (z3 < z0)))) {
					/* we have a saddle point */
					/* first find the most probable altitude of the saddle point */
					int zn, zx;
					gboolean crossing = FALSE, up = FALSE, odd;
					double xl[8], yl[8], xc = 0., yc = 0.;
					/* crossing is TRUE if the saddle point occurs at a slices border */
					zn = MAX (z[0], z[2]);
					if (zval[1] > zval[3])
						zx = (zval[3] == z[3])? z[3] - 1: z[3];
					else
						zx =  (zval[1] == z[1])? z[1] - 1: z[1];
					odd = (zx - zn) % 2;
					if (odd) {
						if ((zx - zn) == 1) {
							double sum = 0.;
							sum += (z[0] == zn)? zval[0]: zn;
							sum += (z[1] == zx)? zval[1]: zx + 1;
							sum += (z[2] == zn)? zval[2]: zn;
							sum += (z[3] == zx)? zval[3]: zx + 1;
							sum /= 4.;
							if (fabs ((sum - zx)) < DBL_EPSILON)
								crossing = TRUE;
							else
								up = (sum - zx) < 0;
						} else
							crossing = TRUE;
						zn = (zn + zx) / 2;
						zx = zn + 1;
					} else
						zn = zx = (zn + zx) / 2;
					/* low values slices */
					if (z[0] < zn) {
						k = z[0];
						style->line.color = color[k];
						style->fill.pattern.back = color[k];
						k++;
						go_path_move_to (path, x[0], y[0]);
						t = (k - zval[0]) / (zval[3] - zval[0]);
						xl[7] = x[0] + t * (x[3] - x[0]);
						yl[7] = y[0] + t * (y[3] - y[0]);
						go_path_line_to (path, xl[7], yl[7]);
						go_path_move_to (lines, xl[7], yl[7]);
						t = (k - zval[0]) / (zval[1] - zval[0]);
						xl[0] = x[0] + t * (x[1] - x[0]);
						yl[0] = y[0] + t * (y[1] - y[0]);
						go_path_line_to (path, xl[0], yl[0]);
						go_path_line_to (lines, xl[0], yl[0]);
						gog_renderer_push_style (rend, style);
						gog_renderer_fill_shape (rend, path);
						go_path_clear (path);
						gog_renderer_pop_style (rend);
						while (k < zn) {

							style->line.color = color[k];
							style->fill.pattern.back = color[k];
							k++;
							go_path_move_to (path, xl[7], yl[7]);
							xc = xl[0];
							yc = yl[0];
							t = (k - zval[0]) / (zval[3] - zval[0]);
							xl[7] = x[0] + t * (x[3] - x[0]);
							yl[7]  = y[0] + t * (y[3] - y[0]);
							go_path_line_to (path, xl[7], yl[7]);
							go_path_move_to (lines, xl[7], yl[7]);
							t = (k - zval[0]) / (zval[1] - zval[0]);
							xl[0] = x[0] + t * (x[1] - x[0]);
							yl[0] =y[0] + t * (y[1] - y[0]);
							go_path_line_to (path, xl[0], yl[0]);
							go_path_line_to (lines, xl[0], yl[0]);
							go_path_line_to (path, xc, yc);
							gog_renderer_push_style (rend, style);
							gog_renderer_fill_shape (rend, path);
							go_path_clear (path);
							gog_renderer_pop_style (rend);
						}
					} else
						xl[0] = xl[7] = -1.;
					if (z[2] < zn) {
						k = z[2];
						style->line.color = color[k];
						style->fill.pattern.back = color[k];
						k++;
						go_path_move_to (path, x[2], y[2]);
						t = (k - zval[2]) / (zval[1] - zval[2]);
						xl[3] = x[2] + t * (x[1] - x[2]);
						yl[3] = y[2] + t * (y[1] - y[2]);
						go_path_line_to (path, xl[3], yl[3]);
						go_path_move_to (lines, xl[3], yl[3]);
						t = (k - zval[2]) / (zval[3] - zval[2]);
						xl[4] = x[2] + t * (x[3] - x[2]);
						yl[4] = y[2] + t * (y[3] - y[2]);
						go_path_line_to (path, xl[4], yl[4]);
						go_path_line_to (lines, xl[4], yl[4]);
						gog_renderer_push_style (rend, style);
						gog_renderer_fill_shape (rend, path);
						go_path_clear (path);
						gog_renderer_pop_style (rend);
						while (k < zn) {
							style->line.color = color[k];
							style->fill.pattern.back = color[k];
							k++;
							go_path_move_to (path, xl[3], yl[3]);
							xc = xl[4];
							yc = yl[4];
							t = (k - zval[2]) / (zval[1] - zval[2]);
							xl[3] = x[2] + t * (x[1] - x[2]);
							yl[3] = y[2] + t * (y[1] - y[2]);
							go_path_line_to (path, xl[3], yl[3]);
							go_path_move_to (lines, xl[3], yl[3]);
							t = (k - zval[2]) / (zval[3] - zval[2]);
							xl[4] = x[2] + t * (x[3] - x[2]);
							yl[4] = y[2] + t * (y[3] - y[2]);
							go_path_line_to (path, xl[4], yl[4]);
							go_path_line_to (lines, xl[4], yl[4]);
							go_path_line_to (path, xc, yc);
							gog_renderer_push_style (rend, style);
							gog_renderer_fill_shape (rend, path);
							go_path_clear (path);
							gog_renderer_pop_style (rend);
						}
					} else
						xl[3] = xl[4] = -1.;
					/* high values slices */
					k = z[1];
					if (zval[1] == k)
						k--;
					if (k > zx) {
						go_path_move_to (path, x[1], y[1]);
						t = (k - zval[1]) / (zval[0] - zval[1]);
						xl[1] = x[1] + t * (x[0] - x[1]);
						yl[1] = y[1] + t * (y[0] - y[1]);
						go_path_line_to (path, xl[1], yl[1]);
						go_path_move_to (lines, xl[1], yl[1]);
						t = (k - zval[1]) / (zval[2] - zval[1]);
						xl[2] = x[1] + t * (x[2] - x[1]);
						yl[2] = y[1] + t * (y[2] - y[1]);
						go_path_line_to (path, xl[2], yl[2]);
						go_path_line_to (lines, xl[2], yl[2]);
						style->line.color = color[k];
						style->fill.pattern.back = color[k];
						gog_renderer_push_style (rend, style);
						gog_renderer_fill_shape (rend, path);
						go_path_clear (path);
						gog_renderer_pop_style (rend);
						k--;
						while (k > zx) {
							go_path_move_to (path, xl[1], yl[1]);
							xc = xl[2];
							yc = yl[2];
							t = (k - zval[1]) / (zval[0] - zval[1]);
							xl[1] = x[1] + t * (x[0] - x[1]);
							yl[1] = y[1] + t * (y[0] - y[1]);
							go_path_line_to (path, xl[1], yl[1]);
							go_path_move_to (lines, xl[1], yl[1]);
							t = (k - zval[1]) / (zval[2] - zval[1]);
							xl[2] = x[1] + t * (x[2] - x[1]);
							yl[2] = y[1] + t * (y[2] - y[1]);
							go_path_line_to (path, xl[2], yl[2]);
							go_path_line_to (lines, xl[2], yl[2]);
							go_path_line_to (path, xc, yc);
							style->line.color = color[k];
							style->fill.pattern.back = color[k];
							gog_renderer_push_style (rend, style);
							gog_renderer_fill_shape (rend, path);
							go_path_clear (path);
							gog_renderer_pop_style (rend);
							k--;
						}
					} else
						xl[1] = xl[2] = -1.;
					k = z[3];
					if (zval[3] == k)
						k--;
					if (k > zx) {
						go_path_move_to (path, x[3], y[3]);
						t = (k - zval[3]) / (zval[2] - zval[3]);
						xl[5] = x[3] + t * (x[2] - x[3]);
						yl[5] = y[3] + t * (y[2] - y[3]);
						go_path_line_to (path, xl[5], yl[5]);
						go_path_move_to (lines, xl[5], yl[5]);
						t = (k - zval[3]) / (zval[0] - zval[3]);
						xl[6] = x[3] + t * (x[0] - x[3]);
						yl[6] = y[3] + t * (y[0] - y[3]);
						go_path_line_to (path, xl[6], yl[6]);
						go_path_line_to (lines, xl[6], yl[6]);
						style->line.color = color[k];
						style->fill.pattern.back = color[k];
						gog_renderer_push_style (rend, style);
						gog_renderer_fill_shape (rend, path);
						go_path_clear (path);
						gog_renderer_pop_style (rend);
						k--;
						while (k > zx) {
							go_path_move_to (path, xl[5], yl[5]);
							xc = xl[6];
							yc = yl[6];
							t = (k - zval[3]) / (zval[2] - zval[3]);
							xl[5] = x[3] + t * (x[2] - x[3]);
							yl[5] = y[3] + t * (y[2] - y[3]);
							go_path_line_to (path, xl[5], yl[5]);
							go_path_move_to (lines, xl[5], yl[5]);
							t = (k - zval[3]) / (zval[0] - zval[3]);
							xl[6] = x[3] + t * (x[0] - x[3]);
							yl[6] = y[3] + t * (y[0] - y[3]);
							go_path_line_to (path, xl[6], yl[6]);
							go_path_line_to (lines, xl[6], yl[6]);
							go_path_line_to (path, xc, yc);
							style->line.color = color[k];
							style->fill.pattern.back = color[k];
							gog_renderer_push_style (rend, style);
							gog_renderer_fill_shape (rend, path);
							go_path_clear (path);
							gog_renderer_pop_style (rend);
							k--;
						}
					} else
						xl[5] = xl[6] = -1.;
					/* middle values slices */
					if (odd) {
						if (crossing) {
							double xb[4], yb[4];
							for (k = 0; k < 4; k++) {
								s = (k + 1) % 4;
								t =  (zx - zval[s]) / (zval[k] - zval[s]);
								xb[k] = x[s] + t * (x[k] - x[s]);
								yb[k] = y[s] + t * (y[k] - y[s]);
							}
							go_path_move_to (lines, xb[0], yb[0]);
							go_path_line_to (lines, xb[2], yb[2]);
							go_path_move_to (lines, xb[1], yb[1]);
							go_path_line_to (lines, xb[3], yb[3]);
							/* calculate the coordinates xc and yc of crossing point */
							t = ((xb[1] - xb[0]) * (yb[3] - yb[1])
								+ (xb[1] - xb[3]) * (yb[1] - yb[0])) /
								((xb[2] - xb[0]) * (yb[3] - yb[1])
								+ (xb[1] - xb[3]) * (yb[2] - yb[0]));
							xc = xb[0] + t * (xb[2] - xb[0]);
							yc = yb[0] + t * (yb[2] - yb[0]);
							/* fill */
							if (xl[0] < 0.)
								go_path_move_to (path, x[0], y[0]);
							else
								go_path_move_to (path, xl[7], yl[7]);
							go_path_line_to (path, xb[3], yb[3]);
							go_path_line_to (path, xc, yc);
							go_path_line_to (path, xb[0], yb[0]);
							if (xl[0] >= 0.)
								go_path_line_to (path, xl[0], yl[0]);
							style->line.color = color[zn];
							style->fill.pattern.back = color[zn];
							gog_renderer_push_style (rend, style);
							gog_renderer_fill_shape (rend, path);
							go_path_clear (path);
							if (xl[4] < 0.)
								go_path_move_to (path, x[2], y[2]);
							else
								go_path_move_to (path, xl[3], yl[3]);
							go_path_line_to (path, xb[1], yb[1]);
							go_path_line_to (path, xc, yc);
							go_path_line_to (path, xb[2], yb[2]);
							if (xl[4] >= 0.)
								go_path_line_to (path, xl[4], yl[4]);
							gog_renderer_fill_shape (rend, path);
							go_path_clear (path);
							gog_renderer_pop_style (rend);
							if (xl[2] < 0.)
								go_path_move_to (path, x[1], y[1]);
							else
								go_path_move_to (path, xl[1], yl[1]);
							go_path_line_to (path, xb[0], yb[0]);
							go_path_line_to (path, xc, yc);
							go_path_line_to (path, xb[1], yb[1]);
							if (xl[2] >= 0.)
								go_path_line_to (path, xl[2], yl[2]);
							style->line.color = color[zx];
							style->fill.pattern.back = color[zx];
							gog_renderer_push_style (rend, style);
							gog_renderer_fill_shape (rend, path);
							go_path_clear (path);
							if (xl[6] < 0.)
								go_path_move_to (path, x[3], y[3]);
							else
								go_path_move_to (path, xl[5], yl[5]);
							go_path_line_to (path, xb[2], yb[2]);
							go_path_line_to (path, xc, yc);
							go_path_line_to (path, xb[3], yb[3]);
							if (xl[6] >= 0.)
								go_path_line_to (path, xl[6], yl[6]);
							gog_renderer_fill_shape (rend, path);
							go_path_clear (path);
							gog_renderer_pop_style (rend);
						} else {
							if (up) {
								/* saddle point is in the lower slice */
								/* draw the upper slices */
								if (xl[1] < 0.) {
									go_path_move_to (path, x[1], y[1]);
									xc = -1;
								} else {
									go_path_move_to (path, xl[1], yl[1]);
									xc = xl[2];
									yc = yl[2];
								}
								t = (zx - zval[1]) / (zval[0] - zval[1]);
								xl[1] = x[1] + t * (x[0] - x[1]);
								yl[1] = y[1] + t * (y[0] - y[1]);
								go_path_move_to (lines, xl[1], yl[1]);
								go_path_line_to (path, xl[1], yl[1]);
								t = (zx - zval[1]) / (zval[2] - zval[1]);
								xl[2] = x[1] + t * (x[2] - x[1]);
								yl[2] = y[1] + t * (y[2] - y[1]);
								go_path_line_to (lines, xl[2], yl[2]);
								go_path_line_to (path, xl[2], yl[2]);
								if (xc >= 0.)
									go_path_line_to (path, xc, yc);
								style->line.color = color[zx];
								style->fill.pattern.back = color[zx];
								gog_renderer_push_style (rend, style);
								gog_renderer_fill_shape (rend, path);
								go_path_clear (path);
								if (xl[5] < 0.) {
									go_path_move_to (path, x[3], y[3]);
									xc = -1;
								} else {
									go_path_move_to (path, xl[5], yl[5]);
									xc = xl[6];
									yc = yl[6];
								}
								t = (zx - zval[3]) / (zval[2] - zval[3]);
								xl[5] = x[3] + t * (x[2] - x[3]);
								yl[5] = y[3] + t * (y[2] - y[3]);
								go_path_move_to (lines, xl[5], yl[5]);
								go_path_line_to (path, xl[5], yl[5]);
								t = (zx - zval[3]) / (zval[0] - zval[3]);
								xl[6] = x[3] + t * (x[0] - x[3]);
								yl[6] = y[3] + t * (y[0] - y[3]);
								go_path_line_to (lines, xl[6], yl[6]);
								go_path_line_to (path, xl[6], yl[6]);
								if (xc >= 0.)
									go_path_line_to (path, xc, yc);
								gog_renderer_fill_shape (rend, path);
								go_path_clear (path);
								gog_renderer_pop_style (rend);
							} else {
								/* saddle point is in the upper slice */
								if (xl[0] < 0.) {
									go_path_move_to (path, x[0], y[0]);
									xc = -1.;
								} else {
									go_path_move_to (path, xl[7], yl[7]);
									xc = xl[0];
									yc = yl[0];
								}
								t = (k - zval[0]) / (zval[3] - zval[0]);
								xl[7] = x[0] + t * (x[3] - x[0]);
								yl[7] = y[0] + t * (y[3] - y[0]);
								go_path_move_to (lines, xl[7], yl[7]);
								go_path_line_to (path, xl[7], yl[7]);
								t = (k - zval[0]) / (zval[1] - zval[0]);
								xl[0] = x[0] + t * (x[1] - x[0]);
								yl[0] = y[0] + t * (y[1] - y[0]);
								go_path_line_to (lines, xl[0], yl[0]);
								go_path_line_to (path, xl[0], yl[0]);
								if (xc >= 0.)
									go_path_line_to (path, xc, yc);
								style->line.color = color[zn];
								style->fill.pattern.back = color[zn];
								gog_renderer_push_style (rend, style);
								gog_renderer_fill_shape (rend, path);
								go_path_clear (path);
								if (xl[4] < 0.) {
									go_path_move_to (path, x[2], y[2]);
									xc = -1.;
								} else {
									go_path_move_to (path, xl[3], yl[3]);
									xc = xl[4];
									yc = yl[4];
								}
								t = (k - zval[2]) / (zval[1] - zval[2]);
								xl[3] = x[2] + t * (x[1] - x[2]);
								yl[3] = y[2] + t * (y[1] - y[2]);
								go_path_move_to (lines, xl[3], yl[3]);
								go_path_line_to (path, xl[3], yl[3]);
								t = (k - zval[2]) / (zval[3] - zval[2]);
								xl[4] = x[2] + t * (x[3] - x[2]);
								yl[4] = y[2] + t * (y[3] - y[2]);
								go_path_line_to (lines, xl[4], yl[4]);
								go_path_line_to (path, xl[4], yl[4]);
								if (xc >= 0.)
									go_path_line_to (path, xc, yc);
								gog_renderer_fill_shape (rend, path);
								go_path_clear (path);
								gog_renderer_pop_style (rend);
								zn = zx;
							}
							/* draw the saddle containing slice */
							k = 0;
							for (s = 0; s < 8; s++) {
								if (xl[s] < 0.) {
									if (s == 7)
										break;
									else if (s > 0)
										s++;
									r = s / 2;
									go_path_line_to (path, x[r], y[r]);
								} else
									go_path_line_to (path, xl[s], yl[s]);
							}
							style->line.color = color[zn];
							style->fill.pattern.back = color[zn];
							gog_renderer_push_style (rend, style);
							gog_renderer_fill_shape (rend, path);
							go_path_clear (path);
							gog_renderer_pop_style (rend);
						}
					} else {
						k = 0;
						if (xl[0] < 0)
							go_path_move_to (path, x[0], y[0]);
						else
							go_path_move_to (path, xl[0], yl[0]);
						for (s = 1; s < 8; s++) {
							if (xl[s] < 0.) {
								if (s == 7)
									break;
								else if (s > 0)
									s++;
								r = s / 2;
								go_path_line_to (path, x[r], y[r]);
							} else
								go_path_line_to (path, xl[s], yl[s]);
						}
						style->line.color = color[zx];
						style->fill.pattern.back = color[zx];
						gog_renderer_push_style (rend, style);
						gog_renderer_fill_shape (rend, path);
						go_path_clear (path);
						gog_renderer_pop_style (rend);
					}
				} else {
					/* no saddle point visible */
					double lastx, lasty, x0 = 0., y0 = 0., x1 = 0., y1 = 0.;
					go_path_move_to (path, x[0], y[0]);
					lastx = x[0];
					lasty = y[0];
					go_path_move_to (path, lastx, lasty);
					k = 1;
					s = 0;
					r = kmax;
					while (zmin < zmax) {
						style->line.color = color[zmin];
						style->fill.pattern.back = color[zmin];
						gog_renderer_push_style (rend, style);
						while (z[k] <= zmin && k < kmax) {
							if (fabs (lastx - x[k]) > CONTOUR_EPSILON ||
								fabs (lasty - y[k]) > CONTOUR_EPSILON) {
								lastx = x[k];
								lasty = y[k];
								go_path_line_to (path, lastx, lasty);
							} else
								k++;
						}
						while (z[r] <= zmin && r > 0)
							r--;
						zmin++;
						t = (zmin - zval[k - 1]) / (zval[k] - zval[k - 1]);
						x0 = x[k - 1] + t * (x[k] - x[k - 1]);
						y0 = y[k - 1] + t * (y[k] - y[k - 1]);
						go_path_move_to (lines, x0, y0);
						if (fabs (lastx - x0) > CONTOUR_EPSILON ||
							fabs (lasty - y0) > CONTOUR_EPSILON) {
							go_path_line_to (path, x0, y0);
							lastx = x0;
							lasty = y0;
						}
						if (r < kmax) {
							t = (zmin - zval[r]) / (zval[r + 1] - zval[r]);
							x1 = x[r] + t * (x[r + 1] - x[r]);
							y1 = y[r] + t * (y[r + 1] - y[r]);
						} else {
							t = (zmin - zval[r]) / (zval[0] - zval[r]);
							x1 = x[r] + t * (x[0] - x[r]);
							y1 = y[r] + t * (y[0] - y[r]);
						}
						go_path_line_to (lines, x1, y1);
						if (fabs (lastx - x1) > CONTOUR_EPSILON ||
							fabs (lasty - y1) > CONTOUR_EPSILON) {
							go_path_line_to (path, x1, y1);
							lastx = x1;
							lasty = y1;
						}
						if (s == 0) {
							for (h = r + 1; h <= kmax; h++) {
								if (fabs (lastx - x[h]) > CONTOUR_EPSILON ||
									fabs (lasty - y[h]) > CONTOUR_EPSILON) {
									lastx = x[h];
									lasty = y[h];
									go_path_line_to (path, lastx, lasty);
								}
							}
						} else {
							for (h = r + 1; h < s; h++) {
								if (fabs (lastx - x[h]) > CONTOUR_EPSILON ||
									fabs (lasty - y[h]) > CONTOUR_EPSILON) {
									lastx = x[h];
									lasty = y[h];
									go_path_line_to (path, lastx, lasty);
								}
							}
						}
						s = r + 1;
						gog_renderer_fill_shape (rend, path);
						go_path_clear (path);
						gog_renderer_pop_style (rend);
						go_path_move_to (path, x1, y1);
						if (fabs (x1 - x0) > CONTOUR_EPSILON ||
							fabs (y1 - y0) > CONTOUR_EPSILON) {
							go_path_line_to (path, x0, y0);
							lastx = x0;
							lasty = y0;
						} else {
							lastx = x1;
							lasty = y1;
						}
					}
					if (fabs (x0 - x1) < CONTOUR_EPSILON
						&& fabs (y0 - y1) < CONTOUR_EPSILON
						&&fabs (x0 - x[0]) > CONTOUR_EPSILON
						&& fabs (y0 - y[0]) > CONTOUR_EPSILON)
						continue;
					while (k < s) {
						go_path_line_to (path, x[k], y[k]);
						k++;
					}
					style->line.color = color[zmin];
					style->fill.pattern.back = color[zmin];
					gog_renderer_push_style (rend, style);
					gog_renderer_fill_shape (rend, path);
					go_path_clear (path);
					gog_renderer_pop_style (rend);
				}
			}
		}
	}

	gog_renderer_push_style (rend, GOG_STYLED_OBJECT (series)->style);
	gog_renderer_stroke_serie (rend, lines);
	gog_renderer_pop_style (rend);
	gog_renderer_pop_clip (rend);
	go_path_free (lines);
	go_path_free (path);
	g_object_unref (style);
	g_free (color);
	gog_axis_map_free (x_map);
	gog_axis_map_free (y_map);
	if (!plot->plotted_data)
		g_free (data);
}

static void
gog_contour_view_class_init (GogViewClass *view_klass)
{
	view_klass->render = gog_contour_view_render;
}

GSF_DYNAMIC_CLASS (GogContourView, gog_contour_view,
	gog_contour_view_class_init, NULL,
	GOG_TYPE_PLOT_VIEW)
