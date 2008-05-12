/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-xyz.c
 *
 * Copyright (C) 2004-2007 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-contour.h"
#include <goffice/data/go-data.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-color.h>

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
gog_contour_plot_build_matrix (GogXYZPlot const *plot, gboolean *cardinality_changed)
{
	unsigned i, j;
	GogAxisMap *map;
	GogAxisTick *zticks;
	GogAxis *axis = plot->base.axis[GOG_AXIS_PSEUDO_3D];
	unsigned nticks;
	double *x, val;
	GogSeries *series = GOG_SERIES (plot->base.series->data);
	GODataMatrix *mat = GO_DATA_MATRIX (series->values[2].data);
	unsigned n = plot->rows * plot->columns;
	double *data, minimum, maximum, slope, offset = 0.;
	unsigned max;

	if (!gog_axis_get_bounds (axis, &minimum, &maximum))
		return NULL;
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
					go_data_matrix_get_value (mat, i, j));
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
	if (series->num_elements != max) {
		series->num_elements = max;
		*cardinality_changed = TRUE;
	}
	gog_axis_map_free (map);
	g_free (x);
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
	static char separator = 0;
	GogStyle *style = gog_style_new ();
	GogTheme *theme = gog_object_get_theme (GOG_OBJECT (plot));
	GogAxis *axis = plot->axis[GOG_AXIS_PSEUDO_3D];
	GOColor *color;
	GogAxisTick *zticks;
	double *limits;
	double minimum, maximum;

	gog_axis_get_bounds (axis, &minimum, &maximum);

	if (separator == 0) {
		struct lconv *lc = localeconv ();
		separator = (strcmp (lc->decimal_point, ","))? ',': ';';
	}
	nticks = gog_axis_get_ticks (axis, &zticks);
	limits = g_new (double, nticks + 1);
	for (i = j = 0; i < nticks; i++)
		if (zticks[i].type == GOG_AXIS_TICK_MAJOR)
			limits[j++] = zticks[i].position;
	j--;
	if (maximum > limits[j])
		limits[++j] = maximum;
	/* build the colors table */
	color = g_new0 (GOColor, (j > 0)? j: 1);
	if (j < 2)
		color[0] = RGBA_WHITE;
	else for (i = 0; i < j; i++) {
		gog_theme_fillin_style (theme, style, GOG_OBJECT (plot->series->data), i, FALSE);
		color[i] = style->fill.pattern.back;
	}
	g_object_unref (style);

	style = gog_style_new ();
	style->interesting_fields = GOG_STYLE_FILL;
	style->disable_theming = GOG_STYLE_ALL;
	style->fill.type = GOG_FILL_STYLE_PATTERN;
	style->fill.pattern.pattern = GO_PATTERN_SOLID;

	if (gog_axis_is_inverted (axis)) {
		for (i = 0; i < j; i++) {
			style->fill.pattern.back = color[i];
			label = g_strdup_printf ("[%g%c %g%c", limits[j - i - 1], separator,
						limits[j - i], (limits[i - j] > minimum)? '[':']');
			(func) (i, style, label, data);
			g_free (label);
		}
		if (limits[i - j] > minimum) {
			gog_theme_fillin_style (theme, style, GOG_OBJECT (plot->series->data), i, FALSE);
			label = g_strdup_printf ("[%g%c %g]", minimum, separator,
						limits[i - j]);
			(func) (i, style, label, data);
			g_free (label);
		}
	} else {
		if (minimum < limits[0]) {
			style->fill.pattern.back = color[0];
			label = g_strdup_printf ("[%g%c %g]", minimum, separator,
						limits[0]);
			(func) (0, style, label, data);
			g_free (label);
			i = 1;
			j++;
		} else
			i = 0;
		for (; i < j; i++) {
			style->fill.pattern.back = color[i];
			label = g_strdup_printf ("[%g%c %g%c", limits[i], separator,
						limits[i + 1], (i == j - 1)? ']':'[');
			(func) (i, style, label, data);
			g_free (label);
		}
	}
	g_free (limits);
	g_object_unref (style);
	g_free (color);
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

	plot->render_before_axes = TRUE;
}

GSF_DYNAMIC_CLASS (GogContourPlot, gog_contour_plot,
	gog_contour_plot_class_init, gog_contour_plot_init,
	GOG_XYZ_PLOT_TYPE)

/*****************************************************************************/

typedef GogPlotView		GogContourView;
typedef GogPlotViewClass	GogContourViewClass;
#define CONTOUR_EPSILON 1e-10

static void
gog_contour_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogXYZPlot const *plot = GOG_XYZ_PLOT (view->model);
	GogSeries const *series;
	GODataVector *x_vec = NULL, *y_vec = NULL;
	GogAxisMap *x_map, *y_map;
	double zval0, zval1, zval2 = 0., zval3, t;
	double x[4], y[4], zval[4];
	int z[4];
	int z0 = 0, z1 = 0, z2 = 0, z3 = 0, zmin, zmax, nans, nan = 0;
	int k, kmax, r = 0, s, h;
	unsigned i, imax, j, jmax, l, lmax, p;
	GogRenderer *rend = view->renderer;
	GogStyle *style;
	GogTheme *theme = gog_object_get_theme (GOG_OBJECT (plot));
	double x0, x1, y0, y1;
	ArtVpath *path, *lines;
	GOColor *color;
	gboolean cw;
	double *data;
	int max;
	gboolean xdiscrete, ydiscrete;

	if (plot->base.series == NULL)
		return;
	series = GOG_SERIES (plot->base.series->data);
	max = series->num_elements;
	if (plot->transposed) {
		imax = plot->columns;
		jmax = plot->rows;
	} else {
		imax = plot->rows;
		jmax = plot->columns;
	}
	if (imax ==0 || jmax == 0)
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

	if (plot->plotted_data)
		data = plot->plotted_data;
	else
		data = gog_xyz_plot_build_matrix (plot, &cw);

	/* Set cw to ensure that polygons will allways be drawn clockwise */
	xdiscrete = gog_axis_is_discrete (plot->base.axis[0]) ||
			series->values[(plot->transposed)? 1: 0].data == NULL;
	if (xdiscrete) {
		x0 = gog_axis_map_to_view (x_map, 0.);
		x1 = gog_axis_map_to_view (x_map, 1.);
	}else {
		x_vec = GO_DATA_VECTOR (series->values[(plot->transposed)? 1: 0].data);
		x0 = gog_axis_map_to_view (x_map, go_data_vector_get_value (x_vec, 0));
		x1 = gog_axis_map_to_view (x_map, go_data_vector_get_value (x_vec, 1));
	}
	ydiscrete = gog_axis_is_discrete (plot->base.axis[1]) ||
			series->values[(plot->transposed)? 0: 1].data == NULL;
	if (ydiscrete) {
		y0 = gog_axis_map_to_view (y_map, 0.);
		y1 = gog_axis_map_to_view (y_map, 1.);
	}else {
		y_vec = GO_DATA_VECTOR (series->values[(plot->transposed)? 0: 1].data);
		y0 = gog_axis_map_to_view (y_map, go_data_vector_get_value (y_vec, 0));
		y1 = gog_axis_map_to_view (y_map, go_data_vector_get_value (y_vec, 1));
	}
	cw = (x1 > x0) == (y1 > y0);

	style = gog_style_new ();
	path = art_new (ArtVpath, 10);
	/* build the colors table */
	color = g_new0 (GOColor, max);
	if (max < 2)
		color[0] = RGBA_WHITE;
	else for (i = 0; i < (unsigned) max; i++) {
		gog_theme_fillin_style (theme, style, GOG_OBJECT (series), i, FALSE);
		color[i] = style->fill.pattern.back;
	}
	g_object_unref (style);

	/* clip to avoid problems with logarithmic axes */
	gog_renderer_push_clip_rectangle (rend, view->residual.x, view->residual.y,
					  view->residual.w, view->residual.h);

	style = gog_style_new ();
	style->interesting_fields = GOG_STYLE_FILL | GOG_STYLE_OUTLINE;
	style->disable_theming = GOG_STYLE_ALL;
	style->fill.type = GOG_FILL_STYLE_PATTERN;
	style->fill.pattern.pattern = GO_PATTERN_SOLID;
	style->outline.dash_type = GO_LINE_SOLID;
	style->outline.auto_dash = FALSE;
	style->outline.auto_color = FALSE;
	style->outline.width = 0.5;
	style->outline.color = RGBA_BLACK;

	lines = art_new (ArtVpath, lmax = 64);
	l = 0;

	for (j = 1; j < jmax; j++) {
		if (xdiscrete) {
			x0 = gog_axis_map_to_view (x_map, j);
			x1 = gog_axis_map_to_view (x_map, j + 1);
		}else {
			x0 = gog_axis_map_to_view (x_map, go_data_vector_get_value (x_vec, j - 1));
			x1 = gog_axis_map_to_view (x_map, go_data_vector_get_value (x_vec, j));
		}
		
		for (i = 1; i < imax; i++) {
			if (ydiscrete) {
				y0 = gog_axis_map_to_view (y_map, i);
				y1 = gog_axis_map_to_view (y_map, i + 1);
			}else {
				y0 = gog_axis_map_to_view (y_map, go_data_vector_get_value (y_vec, i - 1));
				y1 = gog_axis_map_to_view (y_map, go_data_vector_get_value (y_vec, i));
			}
			nans = 0;
			nan = 4;
			zmin = max;
			zmax = 0;
			zval0 = data[(i - 1) * jmax + j - 1];
			if (!isnan (zval0)) {
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
			if (!isnan (zval1)) {
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
			if (!isnan (zval2)) {
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
			if (!isnan (zval3)) {
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
				style->outline.color = color[zmin];
				style->fill.pattern.back = color[zmin];
				gog_renderer_push_style (rend, style);
				path[0].code = ART_MOVETO;
				for (k = 0; k < s; ) {
					path[k].x = x[k];
					path[k].y = y[k];
					path[++k].code = ART_LINETO;
				}
				path[k].x = x[0];
				path[k].y = y[0];
				path[k + 1].code = ART_END;
				/* narrow parameter is TRUE below to avoid border effects */
				gog_renderer_draw_polygon (rend, path, FALSE);
				gog_renderer_pop_style (rend);
			} else {
				kmax = 3 - nans;
				if (!nans && (((z0 < z1) && (z1 > z2) && (z2 < z3) && (z3 > z0)) ||
					((z0 > z1) && (z1 < z2) && (z2 > z3) && (z3 < z0)))) {
					/* we have a saddle point */
					/* first find the most probable altitude of the saddle point */
					int zn, zx;
					gboolean crossing = FALSE, up = FALSE, odd;
					double xl[8], yl[8];
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
						style->outline.color = color[k];
						style->fill.pattern.back = color[k];
						k++;
						path[0].code = ART_MOVETO;
						path[1].code = ART_LINETO;
						path[2].code = ART_LINETO;
						path[3].code = ART_LINETO;
						path[4].code = ART_END;
						path[0].x = path[3].x = x[0];
						path[0].y = path[3].y = y[0];
						if ((l + 3) >= lmax)
							lines = art_renew (lines, ArtVpath, lmax += 64);
						lines[l].code = ART_MOVETO_OPEN;
						t = (k - zval[0]) / (zval[3] - zval[0]);
						xl[7] = lines[l].x = path[1].x = x[0] + t * (x[3] - x[0]);
						yl[7] = lines[l++].y = path[1].y = y[0] + t * (y[3] - y[0]);
						lines[l].code = ART_LINETO;
						t = (k - zval[0]) / (zval[1] - zval[0]);
						xl[0] = lines[l].x = path[2].x = x[0] + t * (x[1] - x[0]);
						yl[0] = lines[l++].y = path[2].y = y[0] + t * (y[1] - y[0]);
						gog_renderer_push_style (rend, style);
						gog_renderer_draw_polygon (rend, path, FALSE);
						gog_renderer_pop_style (rend);
						path[4].code = ART_LINETO;
						path[5].code = ART_END;
						while (k < zn) {
							style->outline.color = color[k];
							style->fill.pattern.back = color[k];
							k++;
							path[0].x = path[4].x = xl[7];
							path[0].y = path[4].y = yl[7];
							path[3].x = xl[0];
							path[3].y = yl[0];
							if ((l + 3) >= lmax)
								lines = art_renew (lines, ArtVpath, lmax += 64);
							lines[l].code = ART_MOVETO_OPEN;
							t = (k - zval[0]) / (zval[3] - zval[0]);
							xl[7] = lines[l].x = path[1].x = x[0] + t * (x[3] - x[0]);
							yl[7] = lines[l++].y = path[1].y = y[0] + t * (y[3] - y[0]);
							lines[l].code = ART_LINETO;
							t = (k - zval[0]) / (zval[1] - zval[0]);
							xl[0] = lines[l].x = path[2].x = x[0] + t * (x[1] - x[0]);
							yl[0] = lines[l++].y = path[2].y = y[0] + t * (y[1] - y[0]);
							gog_renderer_push_style (rend, style);
							gog_renderer_draw_polygon (rend, path, FALSE);
							gog_renderer_pop_style (rend);
						}
					} else
						xl[0] = xl[7] = -1.;
					if (z[2] < zn) {
						k = z[2];
						style->outline.color = color[k];
						style->fill.pattern.back = color[k];
						k++;
						path[0].code = ART_MOVETO;
						path[1].code = ART_LINETO;
						path[2].code = ART_LINETO;
						path[3].code = ART_LINETO;
						path[4].code = ART_END;
						path[0].x = path[3].x = x[2];
						path[0].y = path[3].y = y[2];
						if ((l + 3) >= lmax)
							lines = art_renew (lines, ArtVpath, lmax += 64);
						lines[l].code = ART_MOVETO_OPEN;
						t = (k - zval[2]) / (zval[1] - zval[2]);
						xl[3] = lines[l].x = path[1].x = x[2] + t * (x[1] - x[2]);
						yl[3] = lines[l++].y = path[1].y = y[2] + t * (y[1] - y[2]);
						lines[l].code = ART_LINETO;
						t = (k - zval[2]) / (zval[3] - zval[2]);
						xl[4] = lines[l].x = path[2].x = x[2] + t * (x[3] - x[2]);
						yl[4] = lines[l++].y = path[2].y = y[2] + t * (y[3] - y[2]);
						gog_renderer_push_style (rend, style);
						gog_renderer_draw_polygon (rend, path, FALSE);
						gog_renderer_pop_style (rend);
						path[4].code = ART_LINETO;
						path[5].code = ART_END;
						while (k < zn) {
							style->outline.color = color[k];
							style->fill.pattern.back = color[k];
							k++;
							path[0].x = path[4].x = xl[3];
							path[0].y = path[4].y = yl[3];
							path[3].x = xl[4];
							path[3].y = yl[4];
							if ((l + 3) >= lmax)
								lines = art_renew (lines, ArtVpath, lmax += 64);
							lines[l].code = ART_MOVETO_OPEN;
							t = (k - zval[2]) / (zval[1] - zval[2]);
							xl[3] = lines[l].x = path[1].x = x[2] + t * (x[1] - x[2]);
							yl[3] = lines[l++].y = path[1].y = y[2] + t * (y[1] - y[2]);
							lines[l].code = ART_LINETO;
							t = (k - zval[2]) / (zval[3] - zval[2]);
							xl[4] = lines[l].x = path[2].x = x[2] + t * (x[3] - x[2]);
							yl[4] = lines[l++].y = path[2].y = y[2] + t * (y[3] - y[2]);
							gog_renderer_push_style (rend, style);
							gog_renderer_draw_polygon (rend, path, FALSE);
							gog_renderer_pop_style (rend);
						}
					} else
						xl[3] = xl[4] = -1.;
					/* high values slices */
					k = z[1];
					if (zval[1] == k)
						k--;
					if (k > zx) {
						path[0].code = ART_MOVETO;
						path[1].code = ART_LINETO;
						path[2].code = ART_LINETO;
						path[3].code = ART_LINETO;
						path[4].code = ART_END;
						path[0].x = path[3].x = x[1];
						path[0].y = path[3].y = y[1];
						if ((l + 3) >= lmax)
							lines = art_renew (lines, ArtVpath, lmax += 64);
						lines[l].code = ART_MOVETO_OPEN;
						t = (k - zval[1]) / (zval[0] - zval[1]);
						xl[1] = lines[l].x = path[1].x = x[1] + t * (x[0] - x[1]);
						yl[1] = lines[l++].y = path[1].y = y[1] + t * (y[0] - y[1]);
						lines[l].code = ART_LINETO;
						t = (k - zval[1]) / (zval[2] - zval[1]);
						xl[2] = lines[l].x = path[2].x = x[1] + t * (x[2] - x[1]);
						yl[2] = lines[l++].y = path[2].y = y[1] + t * (y[2] - y[1]);
						style->outline.color = color[k];
						style->fill.pattern.back = color[k];
						gog_renderer_push_style (rend, style);
						gog_renderer_draw_polygon (rend, path, FALSE);
						gog_renderer_pop_style (rend);
						path[4].code = ART_LINETO;
						path[5].code = ART_END;
						k--;
						while (k > zx) {
							path[0].x = path[4].x = xl[1];
							path[0].y = path[4].y = yl[1];
							path[3].x = xl[2];
							path[3].y = yl[2];
							if ((l + 3) >= lmax)
								lines = art_renew (lines, ArtVpath, lmax += 64);
							lines[l].code = ART_MOVETO_OPEN;
							t = (k - zval[1]) / (zval[0] - zval[1]);
							xl[1] = lines[l].x = path[1].x = x[1] + t * (x[0] - x[1]);
							yl[1] = lines[l++].y = path[1].y = y[1] + t * (y[0] - y[1]);
							lines[l].code = ART_LINETO;
							t = (k - zval[1]) / (zval[2] - zval[1]);
							xl[2] = lines[l].x = path[2].x = x[1] + t * (x[2] - x[1]);
							yl[2] = lines[l++].y = path[2].y = y[1] + t * (y[2] - y[1]);
							style->outline.color = color[k];
							style->fill.pattern.back = color[k];
							gog_renderer_push_style (rend, style);
							gog_renderer_draw_polygon (rend, path, FALSE);
							gog_renderer_pop_style (rend);
							k--;
						}
					} else
						xl[1] = xl[2] = -1.;
					k = z[3];
					if (zval[3] == k)
						k--;
					if (k > zx) {
						path[0].code = ART_MOVETO;
						path[1].code = ART_LINETO;
						path[2].code = ART_LINETO;
						path[3].code = ART_LINETO;
						path[4].code = ART_END;
						path[0].x = path[3].x = x[3];
						path[0].y = path[3].y = y[3];
						if ((l + 3) >= lmax)
							lines = art_renew (lines, ArtVpath, lmax += 64);
						lines[l].code = ART_MOVETO_OPEN;
						t = (k - zval[3]) / (zval[2] - zval[3]);
						xl[5] = lines[l].x = path[1].x = x[3] + t * (x[2] - x[3]);
						yl[5] = lines[l++].y = path[1].y = y[3] + t * (y[2] - y[3]);
						lines[l].code = ART_LINETO;
						t = (k - zval[3]) / (zval[0] - zval[3]);
						xl[6] = lines[l].x = path[2].x = x[3] + t * (x[0] - x[3]);
						yl[6] = lines[l++].y = path[2].y = y[3] + t * (y[0] - y[3]);
						style->outline.color = color[k];
						style->fill.pattern.back = color[k];
						gog_renderer_push_style (rend, style);
						gog_renderer_draw_polygon (rend, path, FALSE);
						gog_renderer_pop_style (rend);
						path[4].code = ART_LINETO;
						path[5].code = ART_END;
						k--;
						while (k > zx) {
							path[0].x = path[4].x = xl[5];
							path[0].y = path[4].y = yl[5];
							path[3].x = xl[6];
							path[3].y = yl[6];
							if ((l + 3) >= lmax)
								lines = art_renew (lines, ArtVpath, lmax += 64);
							lines[l].code = ART_MOVETO_OPEN;
							t = (k - zval[3]) / (zval[2] - zval[3]);
							xl[5] = lines[l].x = path[1].x = x[3] + t * (x[2] - x[3]);
							yl[5] = lines[l++].y = path[1].y = y[3] + t * (y[2] - y[3]);
							lines[l].code = ART_LINETO;
							t = (k - zval[3]) / (zval[0] - zval[3]);
							xl[6] = lines[l].x = path[2].x = x[3] + t * (x[0] - x[3]);
							yl[6] = lines[l++].y = path[2].y = y[3] + t * (y[0] - y[3]);
							style->outline.color = color[k];
							style->fill.pattern.back = color[k];
							gog_renderer_push_style (rend, style);
							gog_renderer_draw_polygon (rend, path, FALSE);
							gog_renderer_pop_style (rend);
							k--;
						}
					} else
						xl[5] = xl[6] = -1.;
					/* middle values slices */
					if (odd) {
						if (crossing) {
							double xb[4], yb[4], xc, yc;
							for (k = 0; k < 4; k++) {
								s = (k + 1) % 4;
								t =  (zx - zval[s]) / (zval[k] - zval[s]);
								xb[k] = x[s] + t * (x[k] - x[s]);
								yb[k] = y[s] + t * (y[k] - y[s]);
							}
							if ((l + 5) >= lmax)
								lines = art_renew (lines, ArtVpath, lmax += 64);
							lines[l].code = ART_MOVETO_OPEN;
							lines[l].x = xb[0];
							lines[l++].y = yb[0];
							lines[l].code = ART_LINETO;
							lines[l].x = xb[2];
							lines[l++].y = yb[2];
							lines[l].code = ART_MOVETO_OPEN;
							lines[l].x = xb[1];
							lines[l++].y = yb[1];
							lines[l].code = ART_LINETO;
							lines[l].x = xb[3];
							lines[l++].y = yb[3];
							/* calculate the coordinates xc and yc of crossing point */
							t = ((xb[1] - xb[0]) * (yb[3] - yb[1])
								+ (xb[1] - xb[3]) * (yb[1] - yb[0])) /
								((xb[2] - xb[0]) * (yb[3] - yb[1])
								+ (xb[1] - xb[3]) * (yb[2] - yb[0]));
							xc = xb[0] + t * (xb[2] - xb[0]);
							yc = yb[0] + t * (yb[2] - yb[0]);
							/* fill */
							path[0].code = ART_MOVETO;
							path[1].code = ART_LINETO;
							path[2].code = ART_LINETO;
							path[3].code = ART_LINETO;
							path[4].code = ART_LINETO;
							if (xl[0] < 0.) {
								path[4].x = path[0].x = x[0];
								path[4].y = path[0].y = y[0];
								path[5].code = ART_END;
							} else {
								path[5].x = path[0].x = xl[7];
								path[5].y = path[0].y = yl[7];
								path[4].x = xl[0];
								path[4].y = yl[0];
								path[5].code = ART_LINETO;
								path[6].code = ART_END;
							}
							path[1].x = xb[3];
							path[1].y = yb[3];
							path[2].x = xc;
							path[2].y = yc;
							path[3].x = xb[0];
							path[3].y = yb[0];
							style->outline.color = color[zn];
							style->fill.pattern.back = color[zn];
							gog_renderer_push_style (rend, style);
							gog_renderer_draw_polygon (rend, path, FALSE);
							if (xl[2] < 0.) {
								path[4].x = path[0].x = x[2];
								path[4].y = path[0].y = y[2];
								path[5].code = ART_END;
							} else {
								path[5].x = path[0].x = xl[3];
								path[5].y = path[0].y = yl[3];
								path[4].x = xl[4];
								path[4].y = yl[4];
								path[5].code = ART_LINETO;
								path[6].code = ART_END;
							}
							path[1].x = xb[1];
							path[1].y = yb[1];
							path[3].x = xb[2];
							path[3].y = yb[2];
							gog_renderer_draw_polygon (rend, path, FALSE);
							gog_renderer_pop_style (rend);
							if (xl[2] < 0.) {
								path[4].x = path[0].x = x[1];
								path[4].y = path[0].y = y[1];
								path[5].code = ART_END;
							} else {
								path[5].x = path[0].x = xl[1];
								path[5].y = path[0].y = yl[1];
								path[4].x = xl[2];
								path[4].y = yl[2];
								path[5].code = ART_LINETO;
								path[6].code = ART_END;
							}
							path[1].x = xb[0];
							path[1].y = yb[0];
							path[3].x = xb[1];
							path[3].y = yb[1];
							style->outline.color = color[zx];
							style->fill.pattern.back = color[zx];
							gog_renderer_push_style (rend, style);
							gog_renderer_draw_polygon (rend, path, FALSE);
							if (xl[6] < 0.) {
								path[4].x = path[0].x = x[3];
								path[4].y = path[0].y = y[3];
								path[5].code = ART_END;
							} else {
								path[5].x = path[0].x = xl[5];
								path[5].y = path[0].y = yl[5];
								path[4].x = xl[6];
								path[4].y = yl[6];
								path[5].code = ART_LINETO;
								path[6].code = ART_END;
							}
							path[1].x = xb[2];
							path[1].y = yb[2];
							path[3].x = xb[3];
							path[3].y = yb[3];
							gog_renderer_draw_polygon (rend, path, FALSE);
							gog_renderer_pop_style (rend);
						} else {
							if (up) {
								/* saddle point is in the lower slice */
								/* draw the upper slices */
								path[0].code = ART_MOVETO;
								path[1].code = ART_LINETO;
								path[2].code = ART_LINETO;
								path[3].code = ART_LINETO;
								if (xl[1] < 0.) {
									path[4].code = ART_END;
									path[0].x = path[3].x = x[1];
									path[0].y = path[3].y = y[1];
								} else {
									path[4].code = ART_LINETO;
									path[5].code = ART_END;
									path[0].x = path[4].x = xl[1];
									path[0].y = path[4].y = yl[1];
									path[3].x = xl[2];
									path[3].y = yl[2];
								}
								if ((l + 5) >= lmax)
									lines = art_renew (lines, ArtVpath, lmax += 64);
								lines[l].code = ART_MOVETO_OPEN;
								t = (zx - zval[1]) / (zval[0] - zval[1]);
								xl[1] = lines[l].x = path[1].x = x[1] + t * (x[0] - x[1]);
								yl[1] = lines[l++].y = path[1].y = y[1] + t * (y[0] - y[1]);
								lines[l].code = ART_LINETO;
								t = (zx - zval[1]) / (zval[2] - zval[1]);
								xl[2] = lines[l].x = path[2].x = x[1] + t * (x[2] - x[1]);
								yl[2] = lines[l++].y = path[2].y = y[1] + t * (y[2] - y[1]);
								style->outline.color = color[zx];
								style->fill.pattern.back = color[zx];
								gog_renderer_push_style (rend, style);
								gog_renderer_draw_polygon (rend, path, FALSE);
								if (xl[5] < 0.) {
									path[4].code = ART_END;
									path[0].x = path[3].x = x[3];
									path[0].y = path[3].y = y[3];
								} else {
									path[4].code = ART_LINETO;
									path[5].code = ART_END;
									path[0].x = path[4].x = xl[5];
									path[0].y = path[4].y = yl[5];
									path[3].x = xl[6];
									path[3].y = yl[6];
								}
								if ((l + 5) >= lmax)
									lines = art_renew (lines, ArtVpath, lmax += 64);
								lines[l].code = ART_MOVETO_OPEN;
								t = (zx - zval[3]) / (zval[2] - zval[3]);
								xl[5] = lines[l].x = path[1].x = x[3] + t * (x[2] - x[3]);
								yl[5] = lines[l++].y = path[1].y = y[3] + t * (y[2] - y[3]);
								lines[l].code = ART_LINETO;
								t = (zx - zval[3]) / (zval[0] - zval[3]);
								xl[6] = lines[l].x = path[2].x = x[3] + t * (x[0] - x[3]);
								yl[6] = lines[l++].y = path[2].y = y[3] + t * (y[0] - y[3]);
								gog_renderer_draw_polygon (rend, path, FALSE);
								gog_renderer_pop_style (rend);
							} else {
								/* saddle point is in the upper slice */
								path[0].code = ART_MOVETO;
								path[1].code = ART_LINETO;
								path[2].code = ART_LINETO;
								path[3].code = ART_LINETO;
								if (xl[0] < 0.) {
									path[4].code = ART_END;
									path[0].x = path[3].x = x[0];
									path[0].y = path[3].y = y[0];
								} else {
									path[4].code = ART_LINETO;
									path[5].code = ART_END;
									path[0].x = path[4].x = xl[7];
									path[0].y = path[4].y = yl[7];
									path[3].x = xl[0];
									path[3].y = yl[0];
								}
								if ((l + 5) >= lmax)
									lines = art_renew (lines, ArtVpath, lmax += 64);
								lines[l].code = ART_MOVETO_OPEN;
								t = (k - zval[0]) / (zval[3] - zval[0]);
								xl[7] = lines[l].x = path[1].x = x[0] + t * (x[3] - x[0]);
								yl[7] = lines[l++].y = path[1].y = y[0] + t * (y[3] - y[0]);
								lines[l].code = ART_LINETO;
								t = (k - zval[0]) / (zval[1] - zval[0]);
								xl[0] = lines[l].x = path[2].x = x[0] + t * (x[1] - x[0]);
								yl[0] = lines[l++].y = path[2].y = y[0] + t * (y[1] - y[0]);
								style->outline.color = color[zn];
								style->fill.pattern.back = color[zn];
								gog_renderer_push_style (rend, style);
								gog_renderer_draw_polygon (rend, path, FALSE);
								if (xl[4] < 0.) {
									path[4].code = ART_END;
									path[0].x = path[3].x = x[2];
									path[0].y = path[3].y = y[2];
								} else {
									path[4].code = ART_LINETO;
									path[5].code = ART_END;
									path[0].x = path[4].x = xl[3];
									path[0].y = path[4].y = yl[3];
									path[3].x = xl[4];
									path[3].y = yl[4];
								}
								lines[l].code = ART_MOVETO_OPEN;
								t = (k - zval[2]) / (zval[1] - zval[2]);
								xl[3] = lines[l].x = path[1].x = x[2] + t * (x[1] - x[2]);
								yl[3] = lines[l++].y = path[1].y = y[2] + t * (y[1] - y[2]);
								lines[l].code = ART_LINETO;
								t = (k - zval[2]) / (zval[3] - zval[2]);
								xl[4] = lines[l].x = path[2].x = x[2] + t * (x[3] - x[2]);
								yl[4] = lines[l++].y = path[2].y = y[2] + t * (y[3] - y[2]);
								gog_renderer_draw_polygon (rend, path, FALSE);
								gog_renderer_pop_style (rend);
								zn = zx;
							}
							/* draw the saddle containing slice */
							k = 0;
							for (s = 0; s < 8; s++) {
								path[k].code = (k)? ART_LINETO: ART_MOVETO;
								if (xl[s] < 0.) {
									if (s == 7)
										break;
									else if (s > 0)
										s++;
									r = s / 2;
									path[k].x = x[r];
									path[k++].y = y[r];
								} else {
									path[k].x = xl[s];
									path[k++].y = yl[s];
								}
							}
							path[k].code = ART_LINETO;
							path[k].x = path[0].x;
							path[k++].y = path[0].y;
							path[k].code = ART_END;
							style->outline.color = color[zn];
							style->fill.pattern.back = color[zn];
							gog_renderer_push_style (rend, style);
							gog_renderer_draw_polygon (rend, path, FALSE);
							gog_renderer_pop_style (rend);
						}
					} else {
						k = 0;
						for (s = 0; s < 8; s++) {
							path[k].code = (k)? ART_LINETO: ART_MOVETO;
							if (xl[s] < 0.) {
								if (s == 7)
									break;
								else if (s > 0)
									s++;
								r = s / 2;
								path[k].x = x[r];
								path[k++].y = y[r];
							} else {
								path[k].x = xl[s];
								path[k++].y = yl[s];
							}
						}
						path[k].code = ART_LINETO;
						path[k].x = path[0].x;
						path[k++].y = path[0].y;
						path[k].code = ART_END;
						style->outline.color = color[zx];
						style->fill.pattern.back = color[zx];
						gog_renderer_push_style (rend, style);
						gog_renderer_draw_polygon (rend, path, FALSE);
						gog_renderer_pop_style (rend);
					}
				} else {
					/* no saddle point visible */
					if ((l + (zmax - zmin) * 2 + 1) >= lmax)
						lines = art_renew (lines, ArtVpath, lmax += 64);
					path[0].code = ART_MOVETO;
					path[0].x = x[0];
					path[0].y = y[0];
					p = 1;
					k = 1;
					s = 0;
					r = kmax;
					while (zmin < zmax) {
						style->outline.color = color[zmin];
						style->fill.pattern.back = color[zmin];
						gog_renderer_push_style (rend, style);
						while (z[k] <= zmin && k < kmax) {
							if (fabs (path[p-1].x - x[k]) > CONTOUR_EPSILON ||
								fabs (path[p-1].y - y[k]) > CONTOUR_EPSILON) {
								path[p].code = ART_LINETO;
								path[p].x = x[k];
								path[p++].y = y[k++];
							} else
								k++;	
						}
						while (z[r] <= zmin && r > 0)
							r--;
						zmin++;
						t = (zmin - zval[k - 1]) / (zval[k] - zval[k - 1]);
						path[p].code = ART_LINETO;
						lines[l].code = ART_MOVETO_OPEN;
						lines[l].x = path[p].x = x[k - 1] + t * (x[k] - x[k - 1]);
						lines[l++].y = path[p].y = y[k - 1] + t * (y[k] - y[k - 1]);
						if (fabs (path[p-1].x - path[p].x) > CONTOUR_EPSILON ||
							fabs (path[p-1].y - path[p].y) > CONTOUR_EPSILON)
							p++;
						path[p].code = ART_LINETO;
						lines[l].code = ART_LINETO;
						if (r < kmax) {
							t = (zmin - zval[r]) / (zval[r + 1] - zval[r]);
							lines[l].x = path[p].x = x[r] + t * (x[r + 1] - x[r]);
							lines[l++].y = path[p].y = y[r] + t * (y[r + 1] - y[r]);
						} else {
							t = (zmin - zval[r]) / (zval[0] - zval[r]);
							lines[l].x = path[p].x = x[r] + t * (x[0] - x[r]);
							lines[l++].y = path[p].y = y[r] + t * (y[0] - y[r]);
						}
						if (fabs (path[p-1].x - path[p].x) > CONTOUR_EPSILON ||
							fabs (path[p-1].y - path[p].y) > CONTOUR_EPSILON)
							p++;
						if (s == 0) {
							for (h = r + 1; h <= kmax; h++) {
								if (fabs (path[p-1].x - x[h]) > CONTOUR_EPSILON ||
									fabs (path[p-1].y - y[h]) > CONTOUR_EPSILON) {
									path[p].code = ART_LINETO;
									path[p].x = x[h];
									path[p++].y = y[h];
								}
							}
						} else {
							for (h = r + 1; h < s; h++) {
								if (fabs (path[p-1].x - x[h]) > CONTOUR_EPSILON ||
									fabs (path[p-1].y - y[h]) > CONTOUR_EPSILON) {
									path[p].code = ART_LINETO;
									path[p].x = x[h];
									path[p++].y = y[h];
								}
							}
						}
						s = r + 1;
						if (fabs (path[p-1].x - path[0].x) > CONTOUR_EPSILON ||
							fabs (path[p-1].y -path[0].y) > CONTOUR_EPSILON) {
							path[p].code = ART_LINETO;
							path[p].x = path[0].x;
							path[p++].y = path[0].y;
						} else {
							/* use the exact values so that the polygon is closed */
							path[p-1].x = path[0].x;
							path[p-1].y = path[0].y;
						}
						path[p].code = ART_END;
						gog_renderer_draw_polygon (rend, path, FALSE);
						gog_renderer_pop_style (rend);
						path[0].x = lines[l - 1].x;
						path[0].y = lines[l - 1].y;
						path[1].x = lines[l - 2].x;
						path[1].y = lines[l - 2].y;
						p = (fabs (path[0].x - path[1].x) > CONTOUR_EPSILON ||
							fabs (path[0].y - path[1].y) > CONTOUR_EPSILON)?
							2: 1;
					}
					if (fabs (path[0].x - path[1].x) < CONTOUR_EPSILON
						&& fabs (path[0].y - path[1].y) < CONTOUR_EPSILON)
						continue;
					while (k < s) {
						path[p].code = ART_LINETO;
						path[p].x = x[k];
						path[p++].y = y[k++];
					}
					path[p].code = ART_LINETO;
					path[p].x = path[0].x;
					path[p++].y = path[0].y;
					path[p].code = ART_END;
					style->outline.color = color[zmin];
					style->fill.pattern.back = color[zmin];
					gog_renderer_push_style (rend, style);
					gog_renderer_draw_polygon (rend, path, FALSE);
					gog_renderer_pop_style (rend);
				}
			}
		}
	}
	lines[l].code = ART_END;
	gog_renderer_push_style (rend, GOG_STYLED_OBJECT (series)->style);
	gog_renderer_draw_path  (rend, lines);
	gog_renderer_pop_style (rend);
	gog_renderer_pop_clip (rend);
	art_free (lines);
	art_free (path);
	g_object_unref (style);
	gog_axis_map_free (x_map);
	gog_axis_map_free (y_map);
}

static void
gog_contour_view_class_init (GogViewClass *view_klass)
{
	view_klass->render = gog_contour_view_render;
}

GSF_DYNAMIC_CLASS (GogContourView, gog_contour_view,
	gog_contour_view_class_init, NULL,
	GOG_PLOT_VIEW_TYPE)

