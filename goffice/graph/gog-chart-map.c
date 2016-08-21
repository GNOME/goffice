
/*
 * gog-chart-map.h :
 *
 * Copyright (C) 2006-2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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
#include <goffice/goffice.h>

/**
 * GogChartMapPolarData:
 * @cx: center horizontal position.
 * @cy: center vertical position.
 * @rx: available horizontal space from the center.
 * @ry: available vertical space from the center.
 * @th0: start angle.
 * @th1: end angle.
 **/

struct _GogChartMap {
	GogChart 		*chart;
	GogViewAllocation	 area;
	gpointer	 	 data;
	GogAxisMap		*axis_map[3];
	gboolean		 is_valid;
	unsigned		ref_count;

	void 	 (*map_2D_to_view) 	(GogChartMap *map, double x, double y, double *u, double *v);
	void 	 (*map_view_to_2D) 	(GogChartMap *map, double x, double y, double *u, double *v);
	double 	 (*map_2D_derivative_to_view) (GogChartMap *map, double deriv, double x, double y);
	GOPath  *(*make_path)	   	(GogChartMap *map, double const *x, double const *y, int n_points,
					 GOLineInterpolation interpolation, gboolean skip_invalid, gpointer data);
	GOPath  *(*make_close_path)	(GogChartMap *map, double const *x, double const *y, int n_points,
					 GogSeriesFillType fill_type);
};

static gboolean
get_extremes_2D (double const *x, double const *y, double n_data,
		 double *x_start, double *y_start,
		 double *x_end, double *y_end)
{
	gboolean start_found = FALSE, end_found = FALSE;
	unsigned int i, j;
	double xx, yy;

	for (i = 0, j = n_data - 1; i < n_data; i++, j--) {
		xx = x != NULL ? x[i] : i + 1;
		yy = y != NULL ? y[i] : i + 1;
		if (!start_found && go_finite (xx) && go_finite (yy)) {
			if (x_start != NULL)
				*x_start = xx;
			if (y_start != NULL)
				*y_start = yy;
			if (end_found)
				return TRUE;
			start_found = TRUE;
		}
		xx = x != NULL ? x[j] : j + 1;
		yy = y != NULL ? y[j] : j + 1;
		if (!end_found && go_finite (xx) && go_finite (yy)) {
			if (x_end != NULL)
				*x_end = xx;
			if (y_end != NULL)
				*y_end = yy;
			if (start_found)
				return TRUE;
			end_found = TRUE;
		}
	}

	return FALSE;
}

static void
calc_polygon_parameters (GogViewAllocation const *area, GogChartMapPolarData *data, gboolean fill_area)
{
	double edges = data->th1 - data->th0 + 1.0;
	double width = 2.0 * sin (2.0 * M_PI * go_rint (edges / 4.0) / edges);
	double height = 1.0 - cos (2.0 * M_PI * go_rint (edges / 2.0) / edges);

	data->rx = area->w / width;
	data->ry = area->h / height;

	if (!fill_area) {
		data->rx = MIN (data->rx, data->ry);
		data->ry = data->rx;
	}

	data->cx = area->x + area->w / 2.0;
	data->cy = area->y + data->ry + (area->h - data->ry * height) / 2.0;
}

static void
calc_circle_parameters (GogViewAllocation const *area, GogChartMapPolarData *data, gboolean fill_area)
{
	double x_min, x_max, y_min, y_max;

	if (data->th0 >= data->th1) {
		x_min = y_min = -1.0;
		x_max = y_max = 1.0;
	} else {
		double x;

		if (data->th0 < -2.0 * M_PI) {
			x = data->th0 - fmod (data->th0, 2.0 * M_PI);
			data->th0 -= x;
			data->th1 -= x;
		} else if (data->th1 > 2.0 * M_PI) {
			x = data->th1 - fmod (data->th1, 2.0 * M_PI);
			data->th0 -= x;
			data->th1 -= x;
		}
		if (data->th1 - data->th0 > go_add_epsilon (2 * M_PI))
			data->th1 = data->th0 + 2 * M_PI;

		x_min = x_max = y_min = y_max = 0;
		x = cos (-data->th0);
		x_min = MIN (x_min, x); x_max = MAX (x_max, x);
		x = sin (-data->th0);
		y_min = MIN (y_min, x); y_max = MAX (y_max, x);
		x = cos (-data->th1);
		x_min = MIN (x_min, x); x_max = MAX (x_max, x);
		x = sin (-data->th1);
		y_min = MIN (y_min, x); y_max = MAX (y_max, x);

		if (0 > data->th0 && 0 < data->th1)
			x_max = 1.0;
		if ((       M_PI / 2.0 > data->th0 &&        M_PI / 2.0 < data->th1) ||
		    (-3.0 * M_PI / 2.0 > data->th0 && -3.0 * M_PI / 2.0 < data->th1))
			y_min = -1.0;
		if (( M_PI > data->th0 &&  M_PI < data->th1) ||
		    (-M_PI > data->th0 && -M_PI < data->th1))
			x_min = -1.0;
		if (( 3.0 * M_PI / 2.0 > data->th0 &&  3.0 * M_PI / 2.0 < data->th1) ||
		    (-      M_PI / 2.0 > data->th0 && -      M_PI / 2.0 < data->th1))
			y_max = 1.0;
	}
	data->rx = area->w / (x_max - x_min);
	data->ry = area->h / (y_max - y_min);
	if (!fill_area) {
		data->rx = MIN (data->rx, data->ry);
		data->ry = data->rx;
	}
	data->cx = -x_min * data->rx + area->x + (area->w - data->rx * (x_max - x_min)) / 2.0;
	data->cy = -y_min * data->ry + area->y + (area->h - data->ry * (y_max - y_min)) / 2.0;
}

static void
null_map_2D (GogChartMap *map, double x, double y, double *u, double *v)
{
	g_warning ("[GogChartMap::map_2D] not implemented");
	*u = *v = 0;
}

typedef struct {
	double a, b;
} XMapData;

static void
x_map_2D_to_view (GogChartMap *map, double x, double y, double *u, double *v)
{
	XMapData *data = map->data;

	*u = gog_axis_map_to_view (map->axis_map[0], x);
	*v = data->a * y + data->b;
}

static void
x_map_view_to_2D (GogChartMap *map, double x, double y, double *u, double *v)
{
	XMapData *data = map->data;

	*u = gog_axis_map_from_view (map->axis_map[0], x);
	*v =  (y - data->b) / data->a;
}

typedef struct {
	double		a[2][2];
	double		b[2];
} XYMapData;

static void
xy_map_2D_to_view (GogChartMap *map, double x, double y, double *u, double *v)
{
	*u = gog_axis_map_to_view (map->axis_map[0], x);
	*v = gog_axis_map_to_view (map->axis_map[1], y);
}

static double
xy_map_2D_derivative_to_view (GogChartMap *map, double deriv, double x, double y)
{
	double d;
	d = gog_axis_map_derivative_to_view (map->axis_map[0], x);
	if (isnan (d))
		return go_nan;
	deriv /= d;
	d = gog_axis_map_derivative_to_view (map->axis_map[1], y);
	return (isnan (d))? go_nan: deriv * d;
}

static void
xy_map_view_to_2D (GogChartMap *map, double x, double y, double *u, double *v)
{
	*u = gog_axis_map_from_view (map->axis_map[0], x);
	*v = gog_axis_map_from_view (map->axis_map[1], y);
}

static GOPath *
make_path_linear (GogChartMap *map,
		  double const *x, double const *y,
		  int n_points, gboolean is_polar, gboolean skip_invalid)
{
	GOPath *path;
	int i, n_valid_points = 0;
	double xx, yy;
	double yy_min, yy_max;
	gboolean is_inverted;

	path = go_path_new ();
	if (n_points < 1)
		return path;

	gog_axis_map_get_bounds (map->axis_map[1], &yy_min, &yy_max);
	is_inverted = gog_axis_map_is_inverted (map->axis_map[1]);

	n_valid_points = 0;
	for (i = 0; i < n_points; i++) {
		yy = y != NULL ? y[i] : i + 1;
		if (is_polar) {
			if (is_inverted && yy > yy_max)
				yy = yy_max;
			else if (!is_inverted && yy < yy_min)
				yy = yy_min;
		}
		gog_chart_map_2D_to_view (map,
					  x != NULL ? x[i] : i + 1, yy,
					  &xx, &yy);
		if (go_finite (xx)
		    && go_finite (yy)
		    && fabs (xx) != DBL_MAX
		    && fabs (yy) != DBL_MAX) {
			n_valid_points++;

			if (n_valid_points == 1)
				go_path_move_to (path, xx, yy);
			else
				go_path_line_to (path, xx, yy);
		} else if (!skip_invalid)
			n_valid_points = 0;
	}

	return path;
}

static GOPath *
make_path_spline (GogChartMap *map,
		  double const *x, double const *y, int n_points,
		  gboolean is_polar, gboolean closed, gboolean skip_invalid)
{
	GOPath *path;
	int i, n_valid_points = 0;
	double *uu, *vv, *tt;
	double u, v;
	double yy, yy_min, yy_max;
	gboolean is_inverted;
	GOBezierSpline *spline;

	path = go_path_new ();
	if (n_points < 1)
		return path;

	gog_axis_map_get_bounds (map->axis_map[1], &yy_min, &yy_max);
	is_inverted = gog_axis_map_is_inverted (map->axis_map[1]);

	uu = g_new (double, n_points);
	vv = g_new (double, n_points);
	tt = g_new (double, n_points);
	n_valid_points = 0;
	for (i = 0; i < n_points; i++) {
		yy = y != NULL ? y[i] : i + 1;
		if (is_polar) {
			if (is_inverted && yy > yy_max)
				yy = yy_max;
			else if (!is_inverted && yy < yy_min)
				yy = yy_min;
		}
		gog_chart_map_2D_to_view (map,
					  x != NULL ? x[i] : i + 1, yy,
					  &u, &v);
		if (go_finite (u)
		    && go_finite (v)
		    && fabs (u) != DBL_MAX
		    && fabs (v) != DBL_MAX) {
			uu[n_valid_points] = u;
			vv[n_valid_points] = v;
			tt[n_valid_points] = n_valid_points;
			n_valid_points++;
		} else {
			if (closed || skip_invalid)
				continue;
			if (n_valid_points == 2) {
				go_path_move_to (path, uu[0], vv[0]);
				go_path_line_to (path, uu[1], vv[1]);
			} else if (n_valid_points > 2) {
				/* evaluate the spline */
				spline = go_bezier_spline_init (uu, vv, n_valid_points, closed);
				path = go_bezier_spline_to_path (spline);
				go_bezier_spline_destroy (spline);
			}
			n_valid_points = 0;
		}
	}
	if (n_valid_points == 2) {
		go_path_move_to (path, uu[0], vv[0]);
		go_path_line_to (path, uu[1], vv[1]);
	} else if (n_valid_points > 2) {
		/* evaluate the spline */
		spline = go_bezier_spline_init (uu, vv, n_valid_points, closed);
		path = go_bezier_spline_to_path (spline);
		go_bezier_spline_destroy (spline);
	}

	g_free (uu);
	g_free (vv);
	g_free (tt);
	return path;
}

static GOPath *
make_path_cspline (GogChartMap *map,
		  double const *x, double const *y, int n_points,
		  gboolean is_polar, GOCSplineType type, gboolean skip_invalid,
		  gpointer data)
{
	GOPath *path;
	int i, n_valid_points = 0;
	double *uu, *vv, u, v;
	double p0 = 0., p1 = 0.; /* clamped derivatives */
	GOCSpline *spline;

	path = go_path_new ();
	if (n_points < 1 || (x && !go_range_vary_uniformly (x, n_points)))
		return path;

	uu = g_new (double, n_points);
	vv = g_new (double, n_points);
	n_valid_points = 0;

	for (i = 0; i < n_points; i++) {
		gog_chart_map_2D_to_view (map,
					  x != NULL ? x[i] : i + 1, y != NULL ? y[i] : i + 1,
					  &u, &v);
		if (go_finite (u)
		    && go_finite (v)
		    && fabs (u) != DBL_MAX
		    && fabs (v) != DBL_MAX) {
			uu[n_valid_points] = u;
			vv[n_valid_points] = v;
			n_valid_points++;
		} else {
			if (skip_invalid || type == GO_CSPLINE_CLAMPED)
				continue;
			if (n_valid_points == 2) {
				go_path_move_to (path, uu[0], vv[0]);
				go_path_line_to (path, uu[1], vv[1]);
			} else if (n_valid_points > 2) {
				int j;
				/* evaluate the spline */
				spline = go_cspline_init (uu, vv, n_valid_points, type, p0, p1);
				if (spline) {
					double x0, x1, x2, x3, y0, y1, y2, y3;
					x0 = uu[0];
					y0 = vv[0];
					go_path_move_to (path, x0, y0);
					for (j = 1; j < spline->n; j++) {
						x3 = uu[j];
						y3 = vv[j];
						u = x3 - x0;
						x1 = (2. * x0 + x3) / 3.;
						x2 = (x0 + 2. * x3) / 3.;
						y1 = y0 + spline->c[j-1] / 3. * u;
						y2 = y0 + (2. * spline->c[j-1] + spline->b[j-1] * u) / 3. * u;
						go_path_curve_to (path, x1, y1, x2, y2, x3, y3);
						x0 = x3;
						y0 = y3;
					}
					go_cspline_destroy (spline);
				}
			}
			n_valid_points = 0;
		}
	}
	if (n_valid_points == 2) {
		go_path_move_to (path, uu[0], vv[0]);
		go_path_line_to (path, uu[1], vv[1]);
	} else if (n_valid_points > 2) {
		if (type == GO_CSPLINE_CLAMPED && data != NULL) {
			p0 = gog_chart_map_2D_derivative_to_view (map, ((double*) data)[0], uu[0], vv[0]);
			p1 = gog_chart_map_2D_derivative_to_view (map, ((double*) data)[1],
								  uu[n_valid_points - 1], vv[n_valid_points - 1]);
		}
		spline = go_cspline_init (uu, vv, n_valid_points, type, p0, p1);
		if (spline) {
			double x0, x1, x2, x3, y0, y1, y2, y3;
			x0 = uu[0];
			y0 = vv[0];
			go_path_move_to (path, x0, y0);
			for (i = 1; i < spline->n; i++) {
				x3 = uu[i];
				y3 = vv[i];
				u = x3 - x0;
				x1 = (2. * x0 + x3) / 3.;
				x2 = (x0 + 2. * x3) / 3.;
				y1 = y0 + spline->c[i-1] / 3. * u;
				y2 = y0 + (2. * spline->c[i-1] + spline->b[i-1] * u) / 3. * u;
				go_path_curve_to (path, x1, y1, x2, y2, x3, y3);
				x0 = x3;
				y0 = y3;
			}
			go_cspline_destroy (spline);
		}
	}

	g_free (uu);
	g_free (vv);

	return path;
}

static GOPath *
xy_make_path_step (GogChartMap *map, double const *x, double const *y, int n_points,
		   GOLineInterpolation interpolation, gboolean skip_invalid)
{
	GOPath *path;
	int i, n_valid_points = 0;
	double xx, yy, last_xx = 0.0, last_yy = 0.0;

	path = go_path_new ();
	if (n_points < 1)
		return path;

	n_valid_points = 0;
	for (i = 0; i < n_points; i++) {
		gog_chart_map_2D_to_view (map,
					  x != NULL ? x[i] : i + 1,
					  y != NULL ? y[i] : i + 1, &xx, &yy);
		if (go_finite (xx)
		    && go_finite (yy)
		    && fabs (xx) != DBL_MAX
		    && fabs (yy) != DBL_MAX) {
			n_valid_points++;

			if (n_valid_points == 1)
				go_path_move_to (path, xx, yy);
			else {
				switch (interpolation) {
					case GO_LINE_INTERPOLATION_STEP_START:
						go_path_line_to (path, xx, last_yy);
						break;
					case GO_LINE_INTERPOLATION_STEP_END:
						go_path_line_to (path, last_xx, yy);
						break;
					case GO_LINE_INTERPOLATION_STEP_CENTER_X:
						go_path_line_to (path, (last_xx + xx) / 2.0, last_yy);
						go_path_line_to (path, (last_xx + xx) / 2.0, yy);
						break;
					case GO_LINE_INTERPOLATION_STEP_CENTER_Y:
						go_path_line_to (path, last_xx, (last_yy + yy) / 2.0);
						go_path_line_to (path, xx, (last_yy + yy) / 2.0);
						break;
					default:
						g_assert_not_reached ();
						break;
				}
				go_path_line_to (path, xx, yy);
			}
			last_xx = xx;
			last_yy = yy;
		} else if (!skip_invalid)
			n_valid_points = 0;
	}

	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);

	return path;
}

static GOPath *
xy_make_path (GogChartMap *map, double const *x, double const *y,
	      int n_points, GOLineInterpolation interpolation, gboolean skip_invalid, gpointer data)
{
	GOPath *path = NULL;

	switch (interpolation) {
		case GO_LINE_INTERPOLATION_LINEAR:
			path = make_path_linear (map, x, y, n_points, FALSE, skip_invalid);
			break;
		case GO_LINE_INTERPOLATION_SPLINE:
			path = make_path_spline (map, x, y, n_points, FALSE, FALSE, skip_invalid);
			break;
		case GO_LINE_INTERPOLATION_CLOSED_SPLINE:
			path = make_path_spline (map, x, y, n_points, TRUE, TRUE, skip_invalid);
			break;
		case GO_LINE_INTERPOLATION_CUBIC_SPLINE:
			path = make_path_cspline (map, x, y, n_points, TRUE, GO_CSPLINE_NATURAL, skip_invalid, data);
			break;
		case GO_LINE_INTERPOLATION_PARABOLIC_CUBIC_SPLINE:
			path = make_path_cspline (map, x, y, n_points, TRUE, GO_CSPLINE_PARABOLIC, skip_invalid, data);
			break;
		case GO_LINE_INTERPOLATION_CUBIC_CUBIC_SPLINE:
			path = make_path_cspline (map, x, y, n_points, TRUE, GO_CSPLINE_CUBIC, skip_invalid, data);
			break;
		case GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE:
			path = make_path_cspline (map, x, y, n_points, TRUE, GO_CSPLINE_CLAMPED, skip_invalid, data);
			break;
		case GO_LINE_INTERPOLATION_STEP_START:
		case GO_LINE_INTERPOLATION_STEP_END:
		case GO_LINE_INTERPOLATION_STEP_CENTER_X:
		case GO_LINE_INTERPOLATION_STEP_CENTER_Y:
			path = xy_make_path_step (map, x, y, n_points, interpolation, skip_invalid);
			break;
		case GO_LINE_INTERPOLATION_ODF_SPLINE:
			if (x != NULL && y != NULL && x[0] == x[n_points - 1] && y[0] == y[n_points - 1])
				path = make_path_spline (map, x, y, n_points - 1, TRUE, TRUE, skip_invalid);
			else
				path = make_path_spline (map, x, y, n_points, FALSE, FALSE, skip_invalid);
			break;
		default:
			g_assert_not_reached ();
	}

	return path;
}

static GOPath *
xy_make_close_path (GogChartMap *map, double const *x, double const *y,
		    int n_points, GogSeriesFillType fill_type)
{
	GogAxisMap *x_map, *y_map;
	GOPath *close_path = NULL;
	double baseline, position;
	double x_start = 0.0, x_end = 0.0;
	double y_start = 0.0, y_end = 0.0;

	if (!get_extremes_2D (x, y, n_points, &x_start, &y_start, &x_end, &y_end))
		return NULL;

	x_map = map->axis_map[0];
	y_map = map->axis_map[1];

	switch (fill_type) {
		case GOG_SERIES_FILL_TYPE_Y_ORIGIN:
			close_path = go_path_new ();
			baseline = gog_axis_map_get_baseline (y_map);
			go_path_move_to (close_path, gog_axis_map_to_view (x_map, x_start), baseline);
			go_path_line_to (close_path, gog_axis_map_to_view (x_map, x_end), baseline);
			break;
		case GOG_SERIES_FILL_TYPE_X_ORIGIN:
			close_path = go_path_new ();
			baseline = gog_axis_map_get_baseline (x_map);
			go_path_move_to (close_path, baseline, gog_axis_map_to_view (y_map, y_start));
			go_path_line_to (close_path, baseline, gog_axis_map_to_view (y_map, y_end));
			break;
		case GOG_SERIES_FILL_TYPE_BOTTOM:
			close_path = go_path_new ();
			gog_axis_map_get_extents (y_map, &position, NULL);
			position = gog_axis_map_to_view (y_map, position);
			go_path_move_to (close_path, gog_axis_map_to_view (x_map, x_start), position);
			go_path_line_to (close_path, gog_axis_map_to_view (x_map, x_end), position);
			break;
		case GOG_SERIES_FILL_TYPE_LEFT:
			close_path = go_path_new ();
			gog_axis_map_get_extents (x_map, &position, NULL);
			position = gog_axis_map_to_view (x_map, position);
			go_path_move_to (close_path, position, gog_axis_map_to_view (y_map, y_start));
			go_path_line_to (close_path, position, gog_axis_map_to_view (y_map, y_end));
			break;
		case GOG_SERIES_FILL_TYPE_TOP:
			close_path = go_path_new ();
			gog_axis_map_get_extents (y_map, NULL, &position);
			position = gog_axis_map_to_view (y_map, position);
			go_path_move_to (close_path, gog_axis_map_to_view (x_map, x_start), position);
			go_path_line_to (close_path, gog_axis_map_to_view (x_map, x_end), position);
			break;
		case GOG_SERIES_FILL_TYPE_RIGHT:
			close_path = go_path_new ();
			gog_axis_map_get_extents (x_map, NULL, &position);
			position = gog_axis_map_to_view (x_map, position);
			go_path_move_to (close_path, position, gog_axis_map_to_view (y_map, y_start));
			go_path_line_to (close_path, position, gog_axis_map_to_view (y_map, y_end));
			break;
		case GOG_SERIES_FILL_TYPE_SELF:
			break;
		case GOG_SERIES_FILL_TYPE_X_AXIS_MIN:
			close_path = go_path_new ();
			gog_axis_map_get_real_bounds (x_map, &position, NULL);
			position = gog_axis_map_to_view (x_map, position);
			go_path_move_to (close_path, gog_axis_map_to_view (y_map, y_start), position);
			go_path_line_to (close_path, gog_axis_map_to_view (y_map, y_end), position);
			break;
		case GOG_SERIES_FILL_TYPE_X_AXIS_MAX:
			close_path = go_path_new ();
			gog_axis_map_get_real_bounds (x_map, NULL, &position);
			position = gog_axis_map_to_view (x_map, position);
			go_path_move_to (close_path, gog_axis_map_to_view (y_map, y_start), position);
			go_path_line_to (close_path, gog_axis_map_to_view (y_map, y_end), position);
			break;
		case GOG_SERIES_FILL_TYPE_Y_AXIS_MIN:
			close_path = go_path_new ();
			gog_axis_map_get_real_bounds (y_map, &position, NULL);
			position = gog_axis_map_to_view (y_map, position);
			go_path_move_to (close_path, gog_axis_map_to_view (x_map, x_start), position);
			go_path_line_to (close_path, gog_axis_map_to_view (x_map, x_end), position);
			break;
		case GOG_SERIES_FILL_TYPE_Y_AXIS_MAX:
			close_path = go_path_new ();
			gog_axis_map_get_real_bounds (y_map, NULL, &position);
			position = gog_axis_map_to_view (y_map, position);
			go_path_move_to (close_path, gog_axis_map_to_view (x_map, x_start), position);
			go_path_line_to (close_path, gog_axis_map_to_view (x_map, x_end), position);
			break;
		default:
			break;
	}

	return close_path;
}

static void
polar_map_2D_to_view (GogChartMap *map, double x, double y, double *u, double *v)
{
	GogChartMapPolarData *data = (GogChartMapPolarData *) map->data;
	double r = gog_axis_map_to_view (map->axis_map[1], y);
	double t = gog_axis_map_to_view (map->axis_map[0], x);

	*u = data->cx + r * data->rx * cos (t);
	*v = data->cy + r * data->ry * sin (t);
}

static void
polar_map_view_to_2D (GogChartMap *map, double x, double y, double *u, double *v)
{
	GogChartMapPolarData *data = (GogChartMapPolarData *) map->data;
	double r, t;

	x = (x - data->cx) / data->rx;
	y = (y - data->cy) / data->ry;
	r = hypot (x, y);
	t = atan2 (y, x);
	/* FIXME: following code looks like a kludge */
	if (gog_axis_map_is_discrete (map->axis_map[0])) {
		*u = gog_axis_map_from_view (map->axis_map[0], t);
		if (*u < data->th0)
			*u = data->th1;
		else if (*u > data->th1)
			*u = data->th0;
	} else {
		if (t > 0)
			t -= 2 * M_PI; /* Hmm, why? */
		*u = gog_axis_map_from_view (map->axis_map[0], t);
	}
	*v = gog_axis_map_from_view (map->axis_map[1], r);
}

static GOPath *
polar_make_path_step (GogChartMap *map, double const *x, double const *y, int n_points,
		      GOLineInterpolation interpolation, gboolean skip_invalid)
{
	GogChartMapPolarData *polar_parms;
	GOPath *path;
	int i, n_valid_points = 0;
	double xx, yy;
	double cx, cy, rx, ry;
	double rho, rho_min, rho_max, theta, last_rho = 0.0, last_theta = 0.0;
	gboolean is_inverted;

	path = go_path_new ();
	if (n_points < 1)
		return path;

	gog_axis_map_get_bounds (map->axis_map[1], &rho_min, &rho_max);
	is_inverted = gog_axis_map_is_inverted (map->axis_map[1]);

	polar_parms = gog_chart_map_get_polar_parms (map);
	cx = polar_parms->cx;
	cy = polar_parms->cy;
	rx = polar_parms->rx;
	ry = polar_parms->ry;

	n_valid_points = 0;
	for (i = 0; i < n_points; i++) {
		rho = y != NULL ? y[i] : i + 1;
		if (is_inverted && rho > rho_max)
			rho = rho_max;
		else if (!is_inverted && rho < rho_min)
			rho = rho_min;
		theta = gog_axis_map_to_view (map->axis_map[0], x != NULL ? x[i] : i + 1);
		rho = gog_axis_map_to_view (map->axis_map[1], rho);
		xx = cx + rho * rx * cos (theta);
		yy = cy + rho * ry * sin (theta);
		if (go_finite (xx)
		    && go_finite (yy)
		    && fabs (xx) != DBL_MAX
		    && fabs (yy) != DBL_MAX) {
			n_valid_points++;

			if (n_valid_points == 1)
				go_path_move_to (path, xx, yy);
			else
				switch (interpolation) {
					case GO_LINE_INTERPOLATION_STEP_START:
						go_path_arc_to (path, cx, cy,
								last_rho * rx, last_rho * ry,
								last_theta, theta);
						break;
					case GO_LINE_INTERPOLATION_STEP_END:
						go_path_arc_to (path, cx, cy,
								rho * rx, rho * ry,
								last_theta, theta);
						break;
					case GO_LINE_INTERPOLATION_STEP_CENTER_X:
						go_path_arc_to (path, cx, cy,
								last_rho * rx, last_rho * ry,
								last_theta, (last_theta + theta) / 2.0);
						go_path_arc_to (path, cx, cy,
								rho * rx, rho * ry,
								(last_theta + theta) / 2.0, theta);
						break;
					case GO_LINE_INTERPOLATION_STEP_CENTER_Y:
						go_path_arc_to (path, cx, cy,
								(last_rho + rho) * rx / 2.0,
								(last_rho + rho) * ry / 2.0,
								last_theta, theta);
						go_path_line_to (path, xx, yy);
						break;
					default:
						g_assert_not_reached ();
						break;
				}
			last_theta = theta;
			last_rho = rho;
		} else if (!skip_invalid)
			n_valid_points = 0;
	}

	return path;
}

static GOPath *
polar_make_path (GogChartMap *map, double const *x, double const *y,
		 int n_points, GOLineInterpolation interpolation, gboolean skip_invalid, gpointer data)
{
	GOPath *path = NULL;

	switch (interpolation) {
		case GO_LINE_INTERPOLATION_LINEAR:
			path = make_path_linear (map, x, y, n_points, TRUE, skip_invalid);
			break;
		case GO_LINE_INTERPOLATION_SPLINE:
			path = make_path_spline (map, x, y, n_points, TRUE, FALSE, skip_invalid);
			break;
		case GO_LINE_INTERPOLATION_CLOSED_SPLINE:
			path = make_path_spline (map, x, y, n_points, TRUE, TRUE, skip_invalid);
			break;
		case GO_LINE_INTERPOLATION_CUBIC_SPLINE:
		case GO_LINE_INTERPOLATION_PARABOLIC_CUBIC_SPLINE:
		case GO_LINE_INTERPOLATION_CUBIC_CUBIC_SPLINE:
		case GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE:
			/* Not supported in polar plots */
			break;
		case GO_LINE_INTERPOLATION_STEP_START:
		case GO_LINE_INTERPOLATION_STEP_END:
		case GO_LINE_INTERPOLATION_STEP_CENTER_X:
		case GO_LINE_INTERPOLATION_STEP_CENTER_Y:
			path = polar_make_path_step (map, x, y, n_points, interpolation, skip_invalid);
			break;
		default:
			g_assert_not_reached ();
	}

	return path;
}

static GOPath *
polar_make_close_path (GogChartMap *map, double const *x, double const *y,
		 int n_points, GogSeriesFillType fill_type)
{
	GogChartMapPolarData *parms = map->data;
	GogAxisMap *t_map, *r_map;
	GOPath *close_path = NULL;
	double rho, theta_start = .0, theta_end = .0;

	if (!get_extremes_2D (x, y, n_points, &theta_start, NULL, &theta_end, NULL))
		return NULL;

	t_map = map->axis_map[0];
	r_map = map->axis_map[1];

	switch (fill_type) {
		case GOG_SERIES_FILL_TYPE_CENTER:
			close_path = go_path_new ();
			go_path_move_to (close_path, parms->cx, parms->cy);
			break;
		case GOG_SERIES_FILL_TYPE_ORIGIN:
			close_path = go_path_new ();
			rho = gog_axis_map_get_baseline (r_map);
			go_path_arc_to (close_path, parms->cx, parms->cy,
					rho * parms->rx, rho * parms->ry,
					gog_axis_map_to_view (t_map, theta_start),
					gog_axis_map_to_view (t_map, theta_end));
			break;
		case GOG_SERIES_FILL_TYPE_EDGE:
			close_path = go_path_new ();
			go_path_arc_to (close_path, parms->cx, parms->cy,
					parms->rx, parms->ry,
					gog_axis_map_to_view (t_map, theta_start),
					gog_axis_map_to_view (t_map, theta_end));
			break;
		case GOG_SERIES_FILL_TYPE_SELF:
			break;
		default:
			break;
	}

	return close_path;
}

/**
 * gog_chart_map_get_polar_parms:
 * @map: a #GogChartMap
 *
 * Convenience function for retrieving data related to polar plot layout.
 *
 * returns: (transfer none): a #GogChartMapPolarData struct.
 **/

GogChartMapPolarData *
gog_chart_map_get_polar_parms (GogChartMap *map)
{
	return (GogChartMapPolarData *) map->data;
}

/**
 * gog_chart_map_new:
 * @chart: a #GogChart
 * @area: area allocated to chart
 * @axis0: 1st dimension axis
 * @axis1: 2nd dimension axis
 * @axis2: 3rd dimension axis
 * @fill_area: does chart fill allocated area
 *
 * Creates a new #GogChartMap, used for conversion from data space
 * to canvas space.
 *
 * returns: (transfer full): a new #GogChartMap object.
 **/

GogChartMap *
gog_chart_map_new (GogChart *chart, GogViewAllocation const *area,
		   GogAxis *axis0, GogAxis *axis1, GogAxis *axis2,
		   gboolean fill_area)
{
	GogChartMap *map;
	GogAxisSet axis_set;

	g_return_val_if_fail (GOG_IS_CHART (chart), NULL);
	axis_set = gog_chart_get_axis_set (chart);
	g_return_val_if_fail (axis_set != GOG_AXIS_SET_UNKNOWN &&
			      axis_set != GOG_AXIS_SET_NONE, NULL);

	map = g_new (GogChartMap, 1);

	g_object_ref (chart);
	map->chart = chart;
	map->area = *area;
	map->data = NULL;
	map->is_valid = FALSE;
	map->axis_map[0] = map->axis_map[1] = map->axis_map[2] = NULL;
	map->ref_count = 1;

	switch (axis_set & GOG_AXIS_SET_FUNDAMENTAL) {
		case GOG_AXIS_SET_X:
			{
				XMapData *data = g_new (XMapData, 1);

				map->axis_map[0] = gog_axis_map_new (axis0, map->area.x, map->area.w);

				data->b = area->y + area->h;
				data->a = - area->h;

				map->map_2D_to_view = x_map_2D_to_view;
				map->map_view_to_2D = x_map_view_to_2D;
				map->map_2D_derivative_to_view = NULL;
				map->make_path = NULL;
				map->make_close_path = NULL;
				map->data = data;

				map->is_valid = gog_axis_map_is_valid (map->axis_map [0]);
				break;
			}
		case GOG_AXIS_SET_XY:
		case GOG_AXIS_SET_XY_pseudo_3d:
		case GOG_AXIS_SET_XY_COLOR:
			{
				map->axis_map[0] = gog_axis_map_new (axis0, map->area.x, map->area.w);
				map->axis_map[1] = gog_axis_map_new (axis1, map->area.y + map->area.h,
								     -map->area.h);

				map->data = NULL;
				map->map_2D_to_view = xy_map_2D_to_view;
				map->map_2D_derivative_to_view = xy_map_2D_derivative_to_view;
				map->map_view_to_2D = xy_map_view_to_2D;
				map->make_path = xy_make_path;
				map->make_close_path = xy_make_close_path;

				map->is_valid = gog_axis_map_is_valid (map->axis_map[0]) &&
					gog_axis_map_is_valid (map->axis_map[1]);
				break;
			}
		case GOG_AXIS_SET_RADAR:
			{
				GogChartMapPolarData *data = g_new (GogChartMapPolarData, 1);
				double minimum, maximum;
				double z_rotation = gog_axis_get_circular_rotation (axis0) * M_PI / 180.0;
				double perimeter;

				map->axis_map[0] = gog_axis_map_new (axis0, 0.0, 1.0);
				gog_axis_map_get_bounds (map->axis_map[0], &minimum, &maximum);
				if (gog_axis_is_discrete (axis0)) {
					data->th0 = go_rint (minimum);
					data->th1 = go_rint (maximum);
					calc_polygon_parameters (area, data, fill_area);
					gog_axis_map_free (map->axis_map[0]);
					map->axis_map[0] = gog_axis_map_new (axis0,
						- M_PI / 2.0 + z_rotation,
						2.0 * M_PI * (maximum - minimum) / (maximum - minimum + 1));
				} else {
					perimeter = gog_axis_get_polar_perimeter (axis0);
					minimum = minimum * 2.0 * M_PI / perimeter + z_rotation;
					maximum = maximum * 2.0 * M_PI / perimeter + z_rotation;
					data->th0 = minimum;
					data->th1 = maximum;
					calc_circle_parameters (area, data, fill_area);
					gog_axis_map_free (map->axis_map[0]);
					map->axis_map[0] = gog_axis_map_new (axis0, -minimum,
									     minimum - maximum);
				}
				map->axis_map[1] = gog_axis_map_new (axis1, 0.0, 1.0);

				map->data = data;
				map->map_2D_to_view = polar_map_2D_to_view;
				map->map_2D_derivative_to_view = NULL;
				map->map_view_to_2D = polar_map_view_to_2D;
				map->make_path = polar_make_path;
				map->make_close_path = polar_make_close_path;

				map->is_valid = gog_axis_map_is_valid (map->axis_map[0]) &&
					gog_axis_map_is_valid (map->axis_map[1]);
				break;
			}
		default:
			g_warning ("[GogChartMap::new] unimplemented for axis set %d", axis_set);
			map->map_2D_to_view = null_map_2D;
			map->map_2D_derivative_to_view = NULL;
			map->map_view_to_2D = null_map_2D;
			break;
	}

	return map;
}

/**
 * gog_chart_map_2D_to_view:
 * @map: a #GogChartMap
 * @x: data x value
 * @y: data y value
 * @u: placeholder for x converted value
 * @v: placeholder for y converted value
 *
 * Converts a 2D coordinate from data space to canvas space.
 **/

void
gog_chart_map_2D_to_view (GogChartMap *map, double x, double y, double *u, double *v)
{
	(map->map_2D_to_view) (map, x, y, u, v);
}

/**
 * gog_chart_map_2D_derivative_to_view:
 * @map: a #GogChartMap
 * @deriv: the slope in data space
 * @x: data x value
 * @y: data y value
 *
 * Converts a 2D slope from data space to canvas space. It is only implemented
 for xy maps.
 * Returns: the slope in canvas space or go_nan.
 **/

double
gog_chart_map_2D_derivative_to_view (GogChartMap *map, double deriv, double x, double y)
{
	return (map->map_2D_derivative_to_view)?
		map->map_2D_derivative_to_view (map, deriv, x, y): go_nan;
}

/**
 * gog_chart_map_view_to_2D:
 * @map: a #GogChartMap
 * @x: data x value
 * @y: data y value
 * @u: placeholder for x converted value
 * @v: placeholder for y converted value
 *
 * Converts a 2D coordinate from canvas space to data space.
 **/

void
gog_chart_map_view_to_2D (GogChartMap *map, double x, double y, double *u, double *v)
{
	(map->map_view_to_2D) (map, x, y, u, v);
}

/**
 * gog_chart_map_get_axis_map:
 * @map: a #GogChartMap
 * @index: axis index
 *
 * Convenience function which returns one of the associated axis_map.
 *
 * Valid values are in range [0..2].
 *
 * returns: (transfer none): a #GogAxisMap.
 **/

GogAxisMap *
gog_chart_map_get_axis_map (GogChartMap *map, unsigned int i)
{
	g_return_val_if_fail (map != NULL, NULL);
	g_return_val_if_fail (i < 3, NULL);

	return map->axis_map[i];
}

/**
 * gog_chart_map_is_valid:
 * @map: a #GogChartMap
 *
 * Tests if @map was correctly initializied, i.e. if all associated axis_map
 * are valid (see gog_axis_map_is_valid() ).
 *
 * given
 * to gog_chart_map_new.
 * returns: %TRUE if @map is valid.
 **/

gboolean
gog_chart_map_is_valid (GogChartMap *map)
{
	g_return_val_if_fail (map != NULL, FALSE);

	return map->is_valid;
}

/**
 * gog_chart_map_free:
 * @map: a #GogChartMap
 *
 * Frees @map object.
 **/

void
gog_chart_map_free (GogChartMap *map)
{
	int i;

	g_return_if_fail (map != NULL);

	if (map->ref_count-- > 1)
		return;
	for (i = 0; i < 3; i++)
		if (map->axis_map[i] != NULL)
			gog_axis_map_free (map->axis_map[i]);

	g_free (map->data);
	g_object_unref (map->chart);
	g_free (map);
}

static GogChartMap *
gog_chart_map_ref (GogChartMap *map)
{
	map->ref_count++;
	return map;
}

GType
gog_chart_map_get_type (void)
{
	static GType t = 0;

	if (t == 0)
		t = g_boxed_type_register_static ("GogChartMap",
			 (GBoxedCopyFunc) gog_chart_map_ref,
			 (GBoxedFreeFunc) gog_chart_map_free);
	return t;
}

/**
 * gog_chart_map_make_path:
 * @map: a #GogChartMap
 * @x: x data
 * @y: y data
 * @n_points: number of points
 * @interpolation: interpolation type
 * @skip_invalid: whether to ignore invalid data or interrupt the interpolation
 * @data: a user pointer
 *
 * Returns: a new GOPath using @x and @y data, each valid point being connected with respect to @interpolation.
 **/
GOPath *
gog_chart_map_make_path (GogChartMap *map, double const *x, double const *y,
			 int n_points,
			 GOLineInterpolation interpolation, gboolean skip_invalid, gpointer data)
{
	if (map->make_path != NULL)
		return (map->make_path) (map, x, y, n_points, interpolation, skip_invalid, data);

	return NULL;
}

/**
 * gog_chart_map_make_close_path:
 * @map: a #GogChartMap
 * @x: x data
 * @y: y data
 * @n_points: number of points
 * @fill_type: fill type
 *
 * Returns: a new GOPath using @x and @y data, with respect to @fill_type.
 **/
GOPath *
gog_chart_map_make_close_path (GogChartMap *map, double const *x, double const *y,
			       int n_points,
			       GogSeriesFillType fill_type)
{
	if (map->make_close_path != NULL)
		return (map->make_close_path) (map, x, y, n_points, fill_type);

	return NULL;
}
