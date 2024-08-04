/*
 * gog-axis.c :
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
#include <goffice/goffice.h>
#include <goffice/goffice-debug.h>
#include "gog-axis-line-impl.h"

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

#include <string.h>

/* It's not important that this is accurate.  */
#define DAYS_IN_YEAR 365.25

/* this should be per model */
#define PAD_HACK	4	/* pts */

/**
 * SECTION: gog-axis
 * @short_description: An axis.
 *
 * An axis of a #GogPlot. The axis handles things like the bounds, ticks, and
 * tick value formats.
 * When used in plots with X/Y/Z axes, it can optionally have one
 * #GogLabel objects in the role "Label".
 */

/**
 * GogAxisPolarUnit:
 * @GOG_AXIS_POLAR_UNIT_DEGREES: units as degrees.
 * @GOG_AXIS_POLAR_UNIT_RADIANS: units as radians.
 * @GOG_AXIS_POLAR_UNIT_GRADS: units as grads.
 * @GOG_AXIS_POLAR_UNIT_MAX: maximum values, should not occur.
 **/

/**
 * GogAxisTick:
 * @position: position on the axis.
 * @type: #GogAxisTickTypes
 * @str: label, might be rich text.
 **/

/**
 * GogAxisElemType:
 * @GOG_AXIS_ELEM_MIN: minimum value.
 * @GOG_AXIS_ELEM_MAX: maximum value.
 * @GOG_AXIS_ELEM_MAJOR_TICK: distance between two major ticks.
 * @GOG_AXIS_ELEM_MINOR_TICK: distance between two minor ticks.
 * @GOG_AXIS_ELEM_CROSS_POINT: position of the other axis crossing.
 * @GOG_AXIS_ELEM_MAX_ENTRY: maximum value, should not occur.
 *
 * The indices of the #GOData associated to the axis.
 **/

/**
 * GogAxisMetrics:
 * @GOG_AXIS_METRICS_INVALID: invalid.
 * @GOG_AXIS_METRICS_DEFAULT: default.
 * @GOG_AXIS_METRICS_ABSOLUTE: absolute distance between major ticks.
 * @GOG_AXIS_METRICS_RELATIVE: relative to another axis.
 * @GOG_AXIS_METRICS_RELATIVE_TICKS: relative distance between ticks related to
 * another axis.
 * @GOG_AXIS_METRICS_MAX: first unused value.
 **/


static struct {
	GogAxisPolarUnit unit;
	const char 	*name;
	double		 perimeter;
	const char	*xl_format;
	double		 auto_minimum;
	double		 auto_maximum;
	double		 auto_major;
	double		 auto_minor;
} polar_units[GOG_AXIS_POLAR_UNIT_MAX] = {
	{ GOG_AXIS_POLAR_UNIT_DEGREES, N_("Degrees"), 360.0,	"0\"°\"",   0.0, 360.0,     30.0,      10.0},
	{ GOG_AXIS_POLAR_UNIT_RADIANS, N_("Radians"), 2 * M_PI,	"?pi/??", -M_PI,  M_PI, M_PI/4.0, M_PI/16.0},
	{ GOG_AXIS_POLAR_UNIT_GRADS,   N_("Grads"),   400.0,	"General",  0.0, 400.0,     50.0,      10.0}
};

#define GOG_AXIS_CIRCULAR_ROTATION_MIN (-180.0)
#define GOG_AXIS_CIRCULAR_ROTATION_MAX  180.0

typedef struct _GogAxisMapDesc GogAxisMapDesc;

static struct {
	GogAxisMetrics metrics;
	const char 	*name;
} metrics_desc[GOG_AXIS_METRICS_MAX] = {
	{ GOG_AXIS_METRICS_DEFAULT, "default"},
	{ GOG_AXIS_METRICS_ABSOLUTE, "absolute"},
	{ GOG_AXIS_METRICS_RELATIVE, "relative"},
	{ GOG_AXIS_METRICS_RELATIVE_TICKS, "relative-ticks-distance"}
};

static GogAxisMetrics
gog_axis_metrics_from_str (char const *name)
{
	unsigned i;
	GogAxisMetrics ret = GOG_AXIS_METRICS_DEFAULT;

	for (i = 0; i < GOG_AXIS_METRICS_MAX; i++) {
		if (strcmp (metrics_desc[i].name, name) == 0) {
			ret = metrics_desc[i].metrics;
			break;
		}
	}
	return ret;
}

static char const *
gog_axis_metrics_as_str (GogAxisMetrics metrics)
{
	unsigned i;
	char const *ret = "default";

	for (i = 0; i < GO_LINE_MAX; i++) {
		if (metrics_desc[i].metrics == metrics) {
			ret = metrics_desc[i].name;
			break;
		}
	}
	return ret;
}

struct _GogAxis {
	GogAxisBase	 base;

	GogAxisType	 type;
	GSList		*contributors;

	GogDatasetElement source[GOG_AXIS_ELEM_CROSS_POINT];
	double		  auto_bound[GOG_AXIS_ELEM_CROSS_POINT];
	gboolean inverted; /* apply to all map type */

	double		min_val, max_val;
	double		logical_min_val, logical_max_val;
	GogObject	*min_contrib, *max_contrib; /* NULL means use the manual sources */
	gboolean	is_discrete;
	gboolean	center_on_ticks;
	GOData         *labels;
	GogPlot	       *plot_that_supplied_labels;
	GOFormat       *format, *assigned_format;

	GogAxisMapDesc const 	*map_desc;
	GogAxisMapDesc const 	*actual_map_desc;

	const GODateConventions *date_conv;

	GogAxisPolarUnit	 polar_unit;
	double			 circular_rotation;

	GogAxisTick	*ticks;
	unsigned	 tick_nbr;
	double span_start, span_end;    /* percent of used area */
	GogAxisColorMap const *color_map;		/* color map for color and pseudo-3d axis */
	gboolean auto_color_map;
	GogColorScale *color_scale;
	GogAxisMetrics metrics;
	GogAxis *ref_axis;
	GSList *refering_axes;
	double metrics_ratio;
	GoUnitId unit;
	double display_factor;
};

/*****************************************************************************/

#define TICK_LABEL_PAD_VERT	0
#define TICK_LABEL_PAD_HORIZ	1

#define GOG_AXIS_MAX_TICK_NBR				500
#define GOG_AXIS_LOG_AUTO_MAX_MAJOR_TICK_NBR 		8
#define GOG_AXIS_DISCRETE_AUTO_MAX_MAJOR_TICK_NBR 	20

static void gog_axis_set_ticks (GogAxis *axis,int tick_nbr, GogAxisTick *ticks);

static PangoLayout *
gog_get_layout (void)
{
	PangoContext *context = pango_context_new ();
	PangoLayout *layout = pango_layout_new (context);
	g_object_unref (context);
	return layout;
}

static void
gog_axis_ticks_set_text (GogAxisTick *ticks, char const *str)
{
	go_string_unref (ticks->str);
	ticks->str = go_string_new (str);
}

static void
gog_axis_ticks_set_markup (GogAxisTick *ticks, char const *str, PangoAttrList *l)
{
	go_string_unref (ticks->str);
	ticks->str = go_string_new_rich (str, -1, l, NULL);
}

static GogAxisTick *
create_invalid_axis_ticks (double min, double max)
{
	GogAxisTick *ticks;

	ticks = g_new (GogAxisTick, 2);
	ticks[0].position = min;
	ticks[1].position = max;
	ticks[0].type = ticks[1].type = GOG_AXIS_TICK_MAJOR;
	ticks[0].str = ticks[1].str = NULL;
	gog_axis_ticks_set_text (&ticks[0], "##");
	gog_axis_ticks_set_text (&ticks[1], "##");

	return ticks;
}

const GODateConventions *
gog_axis_get_date_conv (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), NULL);

	return axis->date_conv;
}

/**
 * gog_axis_get_effective_format:
 * @axis: #GogAxis
 *
 * Returns: (transfer none): the #GOFormat used for the axis labels. Differs
 * from gog_axis_get_format in that it never returns a general format
 * (see #go_format_is_general).
 **/
GOFormat *
gog_axis_get_effective_format (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), NULL);

	if (axis->assigned_format &&
	    !go_format_is_general (axis->assigned_format))
		return axis->assigned_format;
	return axis->format;
}

static void
axis_format_value (GogAxis *axis, double val, GOString **str,
		   gboolean do_scale)
{
	GOFormat *fmt = gog_axis_get_effective_format (axis);
	const GODateConventions *date_conv = axis->date_conv;
	GOFormatNumberError err;
	PangoLayout *layout = gog_get_layout ();
	int chars;

	g_return_if_fail (layout != NULL);

	go_string_unref (*str);

	if (do_scale)
		val /= axis->display_factor;
	else {
		/* Caller handled scaling, except for sign.  */
		if (axis->display_factor < 0)
			val = 0 - val;
	}

	// If no format is explicitly set, avoid using excess precision
	chars = !fmt || go_format_is_general (fmt)
		? 10 + (val < 0)
		: -1;

	err = go_format_value_gstring
		(layout, NULL,
		 go_format_measure_strlen,
		 go_font_metrics_unit,
		 fmt,
		 val, 'F', NULL, NULL,
		 chars, date_conv, TRUE);
	if (err)
		*str = go_string_new ("#####");
	else {
		*str = go_string_new_rich
			(pango_layout_get_text (layout), -1,
			 pango_attr_list_ref
			 (pango_layout_get_attributes (layout)),
			 NULL);
		*str = go_string_trim (*str, TRUE);
	}

	g_object_unref (layout);
}

static gboolean
split_date (GogAxis *axis, double val, GDate *date)
{
	if (fabs (val) >= G_MAXINT) {
		g_date_clear (date, 1);
		return TRUE;
	}

	go_date_serial_to_g (date, (int)val, axis->date_conv);
	return !g_date_valid (date);
}

/*****************************************************************************/

struct _GogAxisMap {
	GogAxis		*axis;
	GogAxisMapDesc	const *desc;
	gpointer	 data;
	gboolean	 is_valid;	/* Default to FALSE if desc::init == NULL */
	unsigned     ref_count;
};

struct _GogAxisMapDesc {
	double 		(*map) 		 (GogAxisMap *map, double value);
	double 		(*map_to_view)   (GogAxisMap *map, double value);
	double 		(*map_derivative_to_view)   (GogAxisMap *map, double value);
	double 		(*map_from_view) (GogAxisMap *map, double value);
	gboolean	(*map_finite)    (double value);
	double		(*map_baseline)  (GogAxisMap *map);
	void		(*map_bounds)	 (GogAxisMap *map, double *minimum, double *maximum);
	gboolean 	(*init) 	 (GogAxisMap *map, double offset, double length);
	void		(*destroy) 	 (GogAxisMap *map);

	/*
	 * Refine the description, for example by picking a new auto_bound
	 * method based on format.
	 */
	const GogAxisMapDesc* (*subclass) (GogAxis *axis, const GogAxisMapDesc *desc);

	/*
	 * Calculate graph bounds and tick sizes based on data minimum and
	 * maximum.
	 */
	void		(*auto_bound) 	 (GogAxis *axis,
					  double minimum, double maximum,
					  double *bound);

	void		(*calc_ticks) 	 (GogAxis *axis);

	GOFormat *      (*get_dim_format)(GogAxis *axis, unsigned dim);

	char const	*name;
	char const	*description;
};

/*
 * Discrete mapping
 */

typedef struct
{
	double min;
	double max;
	double scale;
	double a;
	double b;
} MapData;

static gboolean
map_discrete_init (GogAxisMap *map, double offset, double length)
{
	MapData *data = map->data = g_new (MapData, 1);

	if (gog_axis_get_bounds (map->axis, &data->min, &data->max)) {
		data->scale = 1.0 / (data->max - data->min);
		data->a = data->scale * length;
		data->b = offset - data->a * data->min;
		return TRUE;
	}
	data->min = 0.0;
	data->max = 1.0;
	data->scale = 1.0;
	data->a = length;
	data->b = offset;
	return FALSE;
}

static double
map_discrete (GogAxisMap *map, double value)
{
	const MapData *data = map->data;

	return (value - data->min) * data->scale;
}

static double
map_discrete_to_view (GogAxisMap *map, double value)
{
	const MapData *data = map->data;

	return map->axis->inverted ?
		(data->min + data->max - value) * data->a + data->b :
		value * data->a + data->b;
}

static double
map_discrete_derivative_to_view (GogAxisMap *map, double value)
{
	/* WARNING: does this make sense? */
	const MapData *data = map->data;

	return map->axis->inverted ? -data->a: data->a;

}

static double
map_discrete_from_view (GogAxisMap *map, double value)
{
	const MapData *data = map->data;

	return map->axis->inverted ?
		go_rint (data->min + data->max - (value - data->b) / data->a) :
		go_rint ((value - data->b) / data->a);
}

static void
map_discrete_auto_bound (GogAxis *axis,
			 double minimum,
			 double maximum,
			 double *bound)
{
	if ((maximum - minimum) > GOG_AXIS_DISCRETE_AUTO_MAX_MAJOR_TICK_NBR)
		bound[GOG_AXIS_ELEM_MAJOR_TICK] =
		bound[GOG_AXIS_ELEM_MINOR_TICK] =
			go_fake_ceil ((maximum - minimum + 1.0) /
			      (double) GOG_AXIS_DISCRETE_AUTO_MAX_MAJOR_TICK_NBR);
	else
		bound[GOG_AXIS_ELEM_MAJOR_TICK] =
		bound[GOG_AXIS_ELEM_MINOR_TICK] = 1.;

	bound[GOG_AXIS_ELEM_MIN] = minimum;
	bound[GOG_AXIS_ELEM_MAX] = maximum;
}

static void
map_discrete_calc_ticks (GogAxis *axis)
{
	GogAxisTick *ticks;
	gboolean valid;
	double maximum, minimum;
	double tick_start, label_start;
	int tick_nbr, label_nbr;
	int i, j, index;
	int major_tick, major_label;

	major_tick = go_rint (gog_axis_get_entry (axis, GOG_AXIS_ELEM_MAJOR_TICK, NULL));
	major_label = go_rint (gog_axis_get_entry (axis, GOG_AXIS_ELEM_MINOR_TICK, NULL));
	if (major_tick < 1)
		major_tick = 1;
	if (major_label < 1)
		major_label = 1;

	valid = gog_axis_get_bounds (axis, &minimum, &maximum);
	if (!valid) {
		gog_axis_set_ticks (axis, 2, create_invalid_axis_ticks (0.0, 1.0));
		return;
	}

	tick_start = axis->center_on_ticks ?
		go_fake_ceil (minimum / (double) major_tick) * major_tick :
		go_fake_ceil ((minimum - 0.5) / (double) major_tick) * major_tick + 0.5;
	label_start = go_fake_ceil (minimum / (double) major_label) * major_label;
	tick_nbr = go_fake_floor ((maximum - tick_start) / major_tick + 1.0);
	label_nbr = go_fake_floor ((maximum - label_start) / major_label + 1.0);
	tick_nbr = CLAMP (tick_nbr, 0, GOG_AXIS_MAX_TICK_NBR);
	label_nbr = CLAMP (label_nbr, 0, GOG_AXIS_MAX_TICK_NBR);
	if (tick_nbr < 1  && label_nbr < 1) {
		gog_axis_set_ticks (axis, 2, create_invalid_axis_ticks (0.0, 1.0));
		return;
	}
	ticks = g_new (GogAxisTick, tick_nbr + label_nbr);

	for (i = 0; i < tick_nbr; i++) {
		ticks[i].position = tick_start + (double) (i) * major_tick;
		ticks[i].type = GOG_AXIS_TICK_MAJOR;
		ticks[i].str = NULL;
	}
	for (i = 0, j = tick_nbr; i < label_nbr; i++, j++) {
		ticks[j].position = go_rint (label_start + (double) (i) * major_label);
		index = ticks[j].position - 1;
		ticks[j].type = GOG_AXIS_TICK_NONE;
		ticks[j].str = NULL;
		if (axis->labels != NULL) {
			if (index < (int) go_data_get_vector_size (axis->labels) && index >= 0) {
				PangoAttrList *l = go_data_get_vector_markup (axis->labels, index);
				if (l != NULL) {
					char *label = go_data_get_vector_string (axis->labels, index);
					gog_axis_ticks_set_markup (&ticks[j], label, l);
					g_free (label);
				} else {
					double val = go_data_get_vector_value (axis->labels, index);
					if (go_finite (val))
						axis_format_value (axis, val, &ticks[j].str, TRUE);
					else {
						char *label = go_data_get_vector_string (axis->labels, index);
						gog_axis_ticks_set_text (&ticks[j], label);
						g_free (label);
					}
				}
			}
		} else {
			char *label = g_strdup_printf ("%d", index + 1);
			gog_axis_ticks_set_text (&ticks[j], label);
			g_free (label);
		}
	}

	gog_axis_set_ticks (axis, tick_nbr + label_nbr, ticks);
}

/*****************************************************************************/

/*
 *  Linear mapping
 *
 * We parameterize the linear map by f(x) = a(x - xmin) + b.
 * (When inverted: f(x) = a(xmax - x) + b.)
 */

static gboolean
map_linear_init (GogAxisMap *map, double offset, double length)
{
	MapData *data = map->data = g_new (MapData, 1);

	if (gog_axis_get_bounds (map->axis, &data->min, &data->max)) {
		data->scale = 1 / (data->max - data->min);
		data->a = data->scale * length;
		data->b = offset;
		return TRUE;
	}
	data->min = 0.0;
	data->max = 1.0;
	data->scale = 1.0;
	data->a = length;
	data->b = offset;
	return FALSE;
}

static double
map_linear (GogAxisMap *map, double value)
{
	const MapData *data = map->data;

	return (value - data->min) * data->scale;
}

static double
map_linear_to_view (GogAxisMap *map, double value)
{
	const MapData *data = map->data;

	return map->axis->inverted
		? (data->max - value) * data->a + data->b
		: (value - data->min) * data->a + data->b;
}

static double
map_linear_derivative_to_view (GogAxisMap *map, double value)
{
	const MapData *data = map->data;

	return map->axis->inverted ? -data->a : data->a;
}

static double
map_linear_from_view (GogAxisMap *map, double value)
{
	const MapData *data = map->data;

	return map->axis->inverted ?
		data->max - (value - data->b) / data->a :
		(value - data->b) / data->a + data->min;
}

static double
map_baseline (GogAxisMap *map)
{
	const MapData *data = map->data;

	if (0 < data->min)
		return map_linear_to_view (map, data->min);
	else if (0 > data->max)
		return map_linear_to_view (map, data->max);

	return map_linear_to_view (map, 0.);
}

static void
map_bounds (GogAxisMap *map, double *minimum, double *maximum)
{
	const MapData *data = map->data;

	if (minimum != NULL) *minimum = data->min;
	if (maximum != NULL) *maximum = data->max;
}

static void
map_polar_auto_bound (GogAxis *axis, double minimum, double maximum, double *bound)
{
	bound[GOG_AXIS_ELEM_MIN] = polar_units[axis->polar_unit].auto_minimum;
	bound[GOG_AXIS_ELEM_MAX] = polar_units[axis->polar_unit].auto_maximum;
	bound[GOG_AXIS_ELEM_MAJOR_TICK] = polar_units[axis->polar_unit].auto_major;
	bound[GOG_AXIS_ELEM_MINOR_TICK] = polar_units[axis->polar_unit].auto_minor;
}

static void
map_time_auto_bound (GogAxis *axis, double minimum, double maximum, double *bound)
{
	enum { U_Second, U_Minute, U_Hour, U_Day } u;
	static const double invunits[4] = { 24* 60 * 60, 24 * 60, 24, 1 };
	GOFormat *fmt = gog_axis_get_effective_format (axis);
	int is_time = go_format_is_time (fmt);
	double range = fabs (maximum - minimum);
	double iu, step;

	/* handle singletons */
	if (go_sub_epsilon (range) <= 0.) {
		minimum = floor (minimum) - 1;
		maximum = minimum + 1;
		range = 1;
	}

	if (go_format_has_hour (fmt))
		u = U_Hour;
	else if (go_format_has_minute (fmt))
		u = U_Minute;
	else
		u = U_Second;

retry:
	iu = invunits[u];
	step = pow (10, go_fake_floor (log10 (range * iu)));

	if (is_time == 1) {
		switch (u) {
		case U_Hour:
			if (step == 10) {
				/* Step=10 is unusually bad... */
				if (range <= 24)
					step = 4;
				else
					step = 24;
				break;
			}
			/* Fall through */
		case U_Minute:
		case U_Second:
			if (step >= 100) {
				u++;
				goto retry;
			} else if (step < 0.10001 && u != U_Second) {
				u--;
				goto retry;
			}
			break;
		case U_Day:
		default:
			break;
		}
	} else {
		/* This is the elapsed-time case.  */
	}
	if (range * iu / step < 3)
		step /= 2;

	step /= iu;

	bound[GOG_AXIS_ELEM_MIN] = step * go_fake_floor (minimum / step);
	bound[GOG_AXIS_ELEM_MAX] = step * go_fake_ceil (maximum / step);
	bound[GOG_AXIS_ELEM_MAJOR_TICK] = step;
	bound[GOG_AXIS_ELEM_MINOR_TICK] = step / 2;

#if 0
	g_printerr ("min=%g (%g)  max=%g (%g) step=%g (%g)\n",
		    minimum, bound[GOG_AXIS_ELEM_MIN],
		    maximum, bound[GOG_AXIS_ELEM_MAX],
		    step, bound[GOG_AXIS_ELEM_MAJOR_TICK]);
#endif
}

static void
map_date_auto_bound (GogAxis *axis, double minimum, double maximum, double *bound)
{
	double range, step, minor_step;
	GDate min_date, max_date;
	int years;

	minimum = go_fake_floor (minimum);
	maximum = go_fake_ceil (maximum);

	while (split_date (axis, minimum, &min_date)) {
		minimum = 1;
	}

	while (minimum > maximum ||
	       split_date (axis, maximum, &max_date)) {
		maximum = minimum;
	}

	if (minimum == maximum) {
		if (minimum > 1)
			while (split_date (axis, --maximum, &max_date))
				/* Nothing */;
		else
			while (split_date (axis, ++maximum, &max_date))
				/* Nothing */;
	}

	range = maximum - minimum;
	years = g_date_get_year (&max_date) - g_date_get_year (&min_date);

	if (range <= 60) {
		step = 1;
		minor_step = 1;
	} else if (years < 2) {
		g_date_set_day (&min_date, 1);
		minimum = go_date_g_to_serial (&min_date, axis->date_conv);

		if (g_date_get_year (&max_date) < 9999 ||
		    g_date_get_month (&max_date) < 12) {
			g_date_set_day (&max_date, 1);
			g_date_add_months (&max_date, 1);
		}
		maximum = go_date_g_to_serial (&max_date, axis->date_conv);

		step = 30;
		minor_step = (range <= 180 ? 1 : step);
	} else {
		int N = 1, y, maxy;

		while (years > 20 * N) {
			N *= 10;
		}

		g_date_set_day (&min_date, 1);
		g_date_set_month (&min_date, 1);
		y = g_date_get_year (&min_date) / N * N;
		if (g_date_valid_dmy (1, 1, y))
			g_date_set_year (&min_date, y);

		maxy = g_date_get_year (&max_date);
		y = (maxy + N - 1) / N * N;
		/* Make sure we are not going backwards. */
		if (y == maxy &&
		    (g_date_get_day (&max_date) != 1 ||
		     g_date_get_month (&max_date) != 1))
			y += N;

		if (g_date_valid_dmy (1, 1, y))
			g_date_set_dmy (&max_date, 1, 1, y);

		minimum = go_date_g_to_serial (&min_date, axis->date_conv);
		maximum = go_date_g_to_serial (&max_date, axis->date_conv);
		range = maximum - minimum;
		step = DAYS_IN_YEAR * N;
		minor_step = step / (N == 1 ? 12 : 10);
	}

	bound[GOG_AXIS_ELEM_MIN] = minimum;
	bound[GOG_AXIS_ELEM_MAX] = maximum;
	bound[GOG_AXIS_ELEM_MAJOR_TICK] = step;
	bound[GOG_AXIS_ELEM_MINOR_TICK] = minor_step;

#if 0
	g_printerr ("min=%g (%g)  max=%g (%g) step=%g (%g)\n",
		    minimum, bound[GOG_AXIS_ELEM_MIN],
		    maximum, bound[GOG_AXIS_ELEM_MAX],
		    step, bound[GOG_AXIS_ELEM_MAJOR_TICK]);
#endif
}

static void
map_linear_auto_bound (GogAxis *axis, double minimum, double maximum, double *bound)
{
	double step, range, mant;
	int expon;

	range = fabs (maximum - minimum);

	/* handle singletons */
	if (range > 0. && range / (fabs (minimum) + fabs (maximum)) <= DBL_EPSILON)
		range = 0.;
	if (go_sub_epsilon (range) <= 0.) {
		if (maximum > 0)
			minimum = 0.;
		else if (minimum < 0.)
			maximum = 0.;
		else {
			maximum = 1;
			minimum = 0;
		}

		range = fabs (maximum - minimum);
	}

	step = pow (10, go_fake_floor (log10 (range)));
	if (range / step < 1.6)
		step /= 5.;	/* .2 .4 .6 */
	else if (range / step < 3)
		step /= 2.;	/* 0 5 10 */
	else if (range / step > 8)
		step *= 2.;	/* 2 4 6 */

	/* we want the bounds to be loose so jump up a step if we get too close */
	mant = frexp (minimum / step, &expon);
	bound[GOG_AXIS_ELEM_MIN] = step * floor (ldexp (mant - DBL_EPSILON, expon));
	mant = frexp (maximum / step, &expon);
	bound[GOG_AXIS_ELEM_MAX] = step * ceil (ldexp (mant + DBL_EPSILON, expon));
	bound[GOG_AXIS_ELEM_MAJOR_TICK] = step;
	bound[GOG_AXIS_ELEM_MINOR_TICK] = step / 5.;

	/* pull to zero if its nearby (do not pull both directions to 0) */
	if (bound[GOG_AXIS_ELEM_MIN] > 0 &&
	    (bound[GOG_AXIS_ELEM_MIN] - 9.99 * step) < 0)
		bound[GOG_AXIS_ELEM_MIN] = 0;
	else if (bound[GOG_AXIS_ELEM_MAX] < 0 &&
	    (bound[GOG_AXIS_ELEM_MAX] + 9.99 * step) > 0)
		bound[GOG_AXIS_ELEM_MAX] = 0;

	/* The epsilon shift can pull us away from a zero we want to
	 * keep (eg percentage bars with no negative elements) */
	if (bound[GOG_AXIS_ELEM_MIN] < 0 && minimum >= 0.)
		bound[GOG_AXIS_ELEM_MIN] = 0;
	else if (bound[GOG_AXIS_ELEM_MAX] > 0 && maximum <= 0.)
		bound[GOG_AXIS_ELEM_MAX] = 0;
}

static double
scale_step (double step, double *scale)
{
	double f, r;
	int l10;

	if (step <= 0 || !go_finite (step)) {
		*scale = 1;
		return step;
	}

	r = go_fake_floor (step);
	f = step - r;
	if (f <= step * 1e-10) {
		*scale = 1;
		return r;
	}

	l10 = (int)floor (log10 (f));
	*scale = go_pow10 (-l10 + 1);
	step *=* scale;  /* "*= *" isn't cute enough.  */

	r = go_fake_floor (step);
	if (fabs (r - step) < 1e-10)
		step = r;

	return step;
}

static void
map_linear_calc_ticks (GogAxis *axis)
{
	GogAxisTick *ticks;
	double maximum, minimum, start_i, range;
	double maj_step, min_step, step_scale;
	int t, N;
	int maj_i, maj_N; /* Ticks for -1,....,maj_N     */
	int min_i, min_N; /* Ticks for 1,....,(min_N-1) */
	double display_factor = fabs (axis->display_factor);

	if (!gog_axis_get_bounds (axis, &minimum, &maximum)) {
		gog_axis_set_ticks (axis, 2, create_invalid_axis_ticks (0.0, 1.0));
		return;
	}
	maximum /= display_factor;
	minimum /= display_factor;
	range = maximum - minimum;

	maj_step = gog_axis_get_entry (axis, GOG_AXIS_ELEM_MAJOR_TICK, NULL);
	if (maj_step <= 0.)
		maj_step = range;
	else
		maj_step /= display_factor;
	while (1) {
		double ratio = go_fake_floor (range / maj_step);
		double Nd = (ratio + 1);  /* Correct if no minor */
		if (Nd >= 10 * GOG_AXIS_MAX_TICK_NBR)
			maj_step *= 10;
		else if (Nd > GOG_AXIS_MAX_TICK_NBR)
			maj_step *= 2;
		else {
			maj_N = (int)ratio;
			break;
		}
	}

	min_step = gog_axis_get_entry (axis, GOG_AXIS_ELEM_MINOR_TICK, NULL);
	if (min_step <= 0.)
		min_step = maj_step;
	else
		min_step /= display_factor;
	while (1) {
		/* add 0.9 there because the ratio might not be an integer
		 * or there might be rounding errors, more than 0.9 would
		 * put a minor tick quite near the next major tick, see #657670) */
		double ratio = go_fake_floor (maj_step / min_step + 0.9);
		double Nd = (maj_N + 2) * ratio;
		if (Nd >= 10 * GOG_AXIS_MAX_TICK_NBR)
			min_step *= 10;
		else if (Nd > GOG_AXIS_MAX_TICK_NBR)
			min_step *= 2;
		else {
			min_N = MAX (1, (int)ratio);
			break;
		}
	}

	/*
	 * We now have the steps we want.  It is time to align the steps
	 * so they hit round numbers (where round is relative to the step
	 * size).  We do this by rounding up the minimum.
	 *
	 * Due to this rounding we start the major tick counting at
	 * index -1 although we do not put a major tick there. This way,
	 * we have room for minor ticks before the first major tick.
	 *
	 * The last major tick goes at index N.  There may be minor ticks
	 * after that.
	 *
	 * Also due to the rounding, we may have room for one less major
	 * tick.  Recomputing maj_N seems to be the most numerically
	 * stable way of figuring that out.
	 */
	start_i = go_fake_ceil (minimum / maj_step);
	maj_N = (int)(go_fake_floor (maximum / maj_step) - start_i);

	N = (maj_N + 2) * min_N;
	ticks = g_new0 (GogAxisTick, N);

	/*
	 * maj_step might be something like 1.1 which is inexact.
	 * Turn this into 110/100 so calculations can use integers
	 * as long as possible.
	 */
	maj_step = scale_step (maj_step, &step_scale);

	t = 0;
	for (maj_i = -1; maj_i <= maj_N; maj_i++) {
		/*
		 * Always calculate based on start to avoid a build-up of
		 * rounding errors.  Note, that the left factor is an
		 * integer, so we will get precisely zero when we need
		 * to.
		 */
		double maj_pos = (start_i + maj_i) * maj_step / step_scale;

		if (maj_i >= 0) {
			g_assert (t < N);
			ticks[t].position = maj_pos * display_factor;
			ticks[t].type = GOG_AXIS_TICK_MAJOR;
			axis_format_value (axis, maj_pos, &ticks[t].str, FALSE);
			t++;
		}

		for (min_i = 1; min_i < min_N; min_i++) {
			double min_pos = maj_pos + min_i * min_step;
			if (min_pos < minimum)
				continue;
			if (min_pos > maximum)
				break;

			g_assert (t < N);
			ticks[t].position = min_pos * display_factor;
			ticks[t].type = GOG_AXIS_TICK_MINOR;
			ticks[t].str = NULL;
			t++;
		}
	}

	if (t > N)
		g_critical ("[GogAxisMap::linear_calc_ticks] wrong allocation size");
	gog_axis_set_ticks (axis, t, ticks);
}

static const int j_horizon = 23936166;  /* 31-Dec-65535 */

typedef struct {
	enum { DS_MONTHS, DS_DAYS } unit;
	double n;
} DateStep;

/*
 * Moral equivalent of "res = start + i * step".
 *
 * Note the date system: (GDate's) Julian day + fractional day.
 */
static gboolean
dt_add (double *res, double start, double i, const DateStep *step)
{
	if (start <= 0 || start > j_horizon)
		return FALSE;

	switch (step->unit) {
	case DS_DAYS:
		*res = start + i * step->n;
		break;
	case DS_MONTHS: {
		GDate d;
		int istart = (int)start;
		int m;
		double n = i * step->n;

		if (n < 0 || n != floor (n))
			return FALSE;

		g_date_clear (&d, 1);
		g_date_set_julian (&d, istart);
		m = (65535 - g_date_get_year (&d)) * 12 +
			(12 - g_date_get_month (&d));
		if (n > m)
			return FALSE;
		g_date_add_months (&d, (int)n);

		*res = g_date_get_julian (&d) + (start - istart);
		break;
	}
	default:
		return FALSE;
	}

	return *res > 0 && *res <= j_horizon;
}

/*
 * Calculate a lower bound to the number of days in the step.  Accuracy is
 * not terribly important.
 */
static double
dt_inflen (const DateStep *step)
{
	switch (step->unit) {
	case DS_DAYS:
		return step->n;
	case DS_MONTHS:
		return step->n * 28;
	default:
		return go_pinf;
	}
}


static double
dt_serial (double dt, const GODateConventions *date_conv)
{
	double dt0 = go_fake_floor (dt);

	if (dt0 > 0 && dt < j_horizon) {
		GDate d;
		int s;

		g_date_clear (&d, 1);
		g_date_set_julian (&d, (int)dt0);

		s = go_date_g_to_serial (&d, date_conv);
		return s + (dt - dt0);
	} else
		return go_pinf;
}

static void
map_date_calc_ticks (GogAxis *axis)
{
	GogAxisTick *ticks;
	double major_tick, minor_tick;
	double minimum, maximum, range, maj_months, min_months;
	GDate min_date, max_date;
	DateStep major_step, minor_step;
	double start;
	double maj_extra;
	int N, t, maj_i;

	if (!gog_axis_get_bounds (axis, &minimum, &maximum) ||
	    split_date (axis, minimum, &min_date) ||
	    split_date (axis, maximum, &max_date)) {
		gog_axis_set_ticks (axis, 2, create_invalid_axis_ticks (0.0, 1.0));
		return;
	}
	range = maximum - minimum;

	major_tick = gog_axis_get_entry (axis, GOG_AXIS_ELEM_MAJOR_TICK, NULL);
	minor_tick = gog_axis_get_entry (axis, GOG_AXIS_ELEM_MINOR_TICK, NULL);

	major_tick = MAX (major_tick, 0.0);
	minor_tick = CLAMP (minor_tick, 0.0, major_tick);

	maj_months = go_fake_round (major_tick / (DAYS_IN_YEAR / 12));
	maj_months = CLAMP (maj_months, 0, G_MAXINT / 12);
	if (maj_months == 0) {
		major_step.unit = DS_DAYS;
		major_step.n = major_tick;
	} else {
		major_step.unit = DS_MONTHS;
		major_step.n = maj_months;
	}

	min_months = go_fake_round (minor_tick / (DAYS_IN_YEAR / 12));
	min_months = CLAMP (min_months, 0, maj_months);
	if (min_months == 0) {
		minor_step.unit = DS_DAYS;
		minor_step.n = minor_tick;
	} else {
		minor_step.unit = DS_MONTHS;
		minor_step.n = min_months;
	}

	N = 0;
	while (1) {
		double maj_N = range / dt_inflen (&major_step);
		double min_N = range / dt_inflen (&minor_step);
		double Nd = maj_N + min_N + 1;

		if (Nd <= GOG_AXIS_MAX_TICK_NBR) {
			N = (int)Nd;
			break;
		}

		/* Too many.  Now what?  */
		if (min_N >= 2) {
			/*  Drop minor ticks.  */
			minor_step.n = go_pinf;
		} else {
			switch (major_step.unit) {
			case DS_MONTHS:
				if (major_step.n < 12)
					major_step.n = 12;
				else
					major_step.n *= 10;
				break;
			case DS_DAYS:
				if (major_step.n < 1)
					major_step.n = 1;
				else {
					major_step.unit = DS_MONTHS;
					major_step.n = 1;
				}
				break;
			default:
				g_assert_not_reached ();
			}

			major_step.n = go_pinf;
		}
	}

	start = g_date_get_julian (&min_date) +
		(minimum - go_date_g_to_serial (&min_date, axis->date_conv));

	switch (major_step.unit) {
	case DS_MONTHS:
		maj_extra = 1;
		break;
	case DS_DAYS:
		maj_extra = 1 - 0.5 / N;
		break;
	default:
		g_assert_not_reached ();
	}

	ticks = g_new0 (GogAxisTick, N);

	t = 0;
	for (maj_i = 0; t < N; maj_i++) {
		double maj_d, maj_dp1;
		double maj_pos, maj_posp1, min_limit;
		int min_i;

		if (!dt_add (&maj_d, start, maj_i, &major_step))
			break;
		maj_pos = dt_serial (maj_d, axis->date_conv);
		if (maj_pos > maximum)
			break;

		ticks[t].position = maj_pos;
		ticks[t].type = GOG_AXIS_TICK_MAJOR;
		axis_format_value (axis, maj_pos, &ticks[t].str, TRUE);
		t++;

		/* Calculate next major so we know when to stop minors.  */
		if (!dt_add (&maj_dp1, start, maj_i + maj_extra, &major_step))
			break;
		maj_posp1 = dt_serial (maj_dp1, axis->date_conv);
		min_limit = MIN (maximum, maj_posp1);

		for (min_i = 1; t < N; min_i++) {
			double min_d;
			double min_pos;

			if (!dt_add (&min_d, maj_d, min_i, &minor_step))
				break;

			min_pos = dt_serial (min_d, axis->date_conv);
			if (min_pos >= min_limit)
				break;

			ticks[t].position = min_pos;
			ticks[t].type = GOG_AXIS_TICK_MINOR;
			ticks[t].str = NULL;
			t++;
		}
	}

	gog_axis_set_ticks (axis, t, ticks);
}

static GOFormat *
map_time_get_dim_format (GogAxis *axis, unsigned dim)
{
	switch (dim) {
	case GOG_AXIS_ELEM_MIN:
	case GOG_AXIS_ELEM_MAX:
	case GOG_AXIS_ELEM_MAJOR_TICK:
	case GOG_AXIS_ELEM_MINOR_TICK:
		return go_format_new_from_XL ("[hh]:mm:ss");

	default:
		return NULL;
	}
}

static GOFormat *
map_date_get_dim_format (GogAxis *axis, unsigned dim)
{
	switch (dim) {
	case GOG_AXIS_ELEM_MIN:

	case GOG_AXIS_ELEM_MAX: {
		GOFormat *fmt = gog_axis_get_effective_format (axis);
		/*
		 * This is not a particular pretty solution to the problem
		 * of bug #629121.  Improvements are welcome.
		 */
		if (go_format_is_date (fmt) == 2 ||
		    go_format_is_time (fmt) >= 1)
			return go_format_new_from_XL ("d-mmm-yy hh:mm:ss");
		return go_format_new_magic (GO_FORMAT_MAGIC_MEDIUM_DATE);
	}

	case GOG_AXIS_ELEM_MAJOR_TICK:
	case GOG_AXIS_ELEM_MINOR_TICK:
		/* In units of days, so default is fine.  */
	default:
		return NULL;
	}
}

static const GogAxisMapDesc *
map_linear_subclass (GogAxis *axis, const GogAxisMapDesc *desc)
{
	GOFormat *fmt;

	if (gog_axis_get_atype (axis) == GOG_AXIS_CIRCULAR) {
		static GogAxisMapDesc map_desc_polar;

		if (!map_desc_polar.auto_bound) {
			map_desc_polar = *desc;
			map_desc_polar.auto_bound = map_polar_auto_bound;
			map_desc_polar.subclass = NULL;
		}

		return &map_desc_polar;
	}

	fmt = gog_axis_get_effective_format (axis);
	if (fmt && go_format_is_time (fmt) > 0) {
		static GogAxisMapDesc map_desc_time;

		if (!map_desc_time.auto_bound) {
			map_desc_time = *desc;
			map_desc_time.auto_bound = map_time_auto_bound;
			map_desc_time.get_dim_format = map_time_get_dim_format;
			map_desc_time.subclass = NULL;
		}

		return &map_desc_time;
	}

	if (fmt && go_format_is_date (fmt) > 0) {
		static GogAxisMapDesc map_desc_date;

		if (!map_desc_date.auto_bound) {
			map_desc_date = *desc;
			map_desc_date.auto_bound = map_date_auto_bound;
			map_desc_date.calc_ticks = map_date_calc_ticks;
			map_desc_date.get_dim_format = map_date_get_dim_format;
			map_desc_date.subclass = NULL;
		}

		return &map_desc_date;
	}

	return NULL;
}

/*****************************************************************************/

/*
 * Logarithmic mapping
 */

typedef struct
{
	double min;
	double max;
	double scale;
	double a;
	double b;
	double a_inv;
	double b_inv;
} MapLogData;

static gboolean
map_log_init (GogAxisMap *map, double offset, double length)
{
	MapLogData *data = map->data = g_new (MapLogData, 1);

	if (gog_axis_get_bounds (map->axis, &data->min, &data->max))
		if (data->min > 0.0)  {
			data->min = log (data->min);
			data->max = log (data->max);
			data->scale = 1/ (data->max - data->min);
			data->a = data->scale * length;
			data->b = offset - data->a * data->min;
			data->a_inv = -data->scale * length;
			data->b_inv = offset + length - data->a_inv * data->min;
			return TRUE;
		}

	data->min = 0.0;
	data->max = log (10.0);
	data->scale = 1 / log(10.0);
	data->a = data->scale * length;
	data->b = offset;
	data->a_inv = -data->scale * length;
	data->b_inv = offset + length;

	return FALSE;
}

static double
map_log (GogAxisMap *map, double value)
{
	const MapLogData *data = map->data;

	return (log (value) - data->min) * data->scale;
}

static double
map_log_to_view (GogAxisMap *map, double value)
{
	const MapLogData *data = map->data;
	double result;

	if (value <= 0.)
		/* Make libart happy, do we still need it? */
		result = map->axis->inverted ? -DBL_MAX : DBL_MAX;
	else
		result = map->axis->inverted ?
			log (value) * data->a_inv + data->b_inv :
			log (value) * data->a + data->b;

	return result;
}

static double
map_log_derivative_to_view (GogAxisMap *map, double value)
{
	const MapLogData *data = map->data;

	if (value <= 0.)
		return go_nan;
	return map->axis->inverted ? data->a_inv / value:
		data->a / value;

}

static double
map_log_from_view (GogAxisMap *map, double value)
{
	const MapLogData *data = map->data;

	return  map->axis->inverted ?
		exp ((value - data->b_inv) / data->a_inv) :
		exp ((value - data->b) / data->a);
}

static gboolean
map_log_finite (double value)
{
	return go_finite (value) && value > 0.;
}

static double
map_log_baseline (GogAxisMap *map)
{
	const MapLogData *data = map->data;

	return map->axis->inverted ?
		data->min * data->a_inv + data->b_inv :
		data->min * data->a + data->b;
}

static void
map_log_bounds (GogAxisMap *map, double *minimum, double *maximum)
{
	const MapLogData *data = map->data;

	if (minimum != NULL) *minimum = exp (data->min);
	if (maximum != NULL) *maximum = exp (data->max);
}

static void
map_log_auto_bound (GogAxis *axis, double minimum, double maximum, double *bound)
{
	double step;

	if (maximum <= 0.0)
		maximum = 1.0;
	if (minimum <= 0.0)
		minimum = maximum / 100.0;
	if (maximum < minimum)
		maximum = minimum * 100.0;

	maximum = go_fake_ceil (log10 (maximum));
	minimum = go_fake_floor (log10 (minimum));

	step = go_fake_ceil ((maximum - minimum + 1.0) /
		     (double) GOG_AXIS_LOG_AUTO_MAX_MAJOR_TICK_NBR);

	bound[GOG_AXIS_ELEM_MIN] = pow (10.0, minimum);
	bound[GOG_AXIS_ELEM_MAX] = pow (10.0, maximum);
	bound[GOG_AXIS_ELEM_MAJOR_TICK] = step;
	bound[GOG_AXIS_ELEM_MINOR_TICK] = 9 * step - 1;
}

static void
map_log_calc_ticks (GogAxis *axis)
{
	GogAxisTick *ticks;
	double maximum, minimum, lminimum, lmaximum, start_i, lrange;
	double maj_step, min_step;
	gboolean base10;
	int t, N;
	int maj_i, maj_N; /* Ticks for -1,....,maj_N     */
	int min_i, min_N; /* Ticks for 1,....,(min_N-1) */
	int base10_unit = 0;

	if (!gog_axis_get_bounds (axis, &minimum, &maximum) ||
	    minimum <= 0.0) {
		gog_axis_set_ticks (axis, 2, create_invalid_axis_ticks (1.0, 10.0));
		return;
	}

	lminimum = log10 (minimum);
	lmaximum = log10 (maximum);
	lrange = lmaximum - lminimum;

	/* This is log10(factor) between major ticks.  */
	maj_step = gog_axis_get_entry (axis, GOG_AXIS_ELEM_MAJOR_TICK, NULL);
	if (!(maj_step > 0))
		maj_step = 1;
	while (1) {
		double ratio = go_fake_floor (lrange / maj_step);
		double Nd = (ratio + 1);  /* Correct if no minor */
		if (Nd > GOG_AXIS_MAX_TICK_NBR)
			maj_step *= 2;
		else {
			maj_N = (int)ratio;
			break;
		}
	}
	base10 = (maj_step >= 1 && maj_step == floor (maj_step));

	/*
	 * This is the number of subticks we want strictly between
	 * each major tick.
	 */
	min_step = go_fake_floor (gog_axis_get_entry (axis, GOG_AXIS_ELEM_MINOR_TICK, NULL));
	if (min_step < 0) {
		min_N = 1;
	} else if (base10) {
		int ims = (int)maj_step;
		/*
		 * We basically have three choices:
		 *   9N-1 : minor ticks at {1,2,...,9}*10^n and at 10^n
		 *          without major ticks.
		 *   N-1  : minor ticks at 10^n without major tick
		 *
		 *   1    : no minor ticks.
		 */
		do {
			if (min_step >= 9 * ims - 1)
				min_step = 9 * ims - 1;
			else if (min_step >= ims - 1)
				min_step = ims - 1;
			else
				min_step = 0;

			if (maj_N * (min_step + 1) > GOG_AXIS_MAX_TICK_NBR) {
				min_step -= 1;
				continue;
			}
		} while (0);

		min_N = (int)min_step + 1;
		base10_unit = (min_N == ims ? 1 : 9);
		min_step = go_nan;
	} else {
		while ((min_step + 1) * maj_N > GOG_AXIS_MAX_TICK_NBR)
			min_step = floor (min_step / 2);
		min_N = 1 + (int)min_step;
		min_step = maj_step / min_N;
	}

	start_i = go_fake_ceil (lminimum / maj_step);
	maj_N = (int)(go_fake_floor (lmaximum / maj_step) - start_i);

#if 0
	g_printerr ("lmin=%g  lmax=%g  maj_N=%d  maj_step=%g  start_i=%g\n",
		    lminimum, lmaximum, maj_N, maj_step, start_i);
#endif

	N = (maj_N + 2) * min_N;
	ticks = g_new0 (GogAxisTick, N);

	t = 0;
	for (maj_i = -1; maj_i <= maj_N; maj_i++) {
		/*
		 * Always calculate based on start to avoid a build-up of
		 * rounding errors.  Note, that the left factor is an
		 * integer, so we will get precisely zero when we need
		 * to.
		 */
		double maj_lpos = (start_i + maj_i) * maj_step;
		double maj_pos = pow (10, maj_lpos);

		if (maj_i >= 0) {
			g_assert (t < N);
			ticks[t].position = maj_pos;
			ticks[t].type = GOG_AXIS_TICK_MAJOR;
		        axis_format_value (axis, maj_pos, &ticks[t].str, TRUE);
			t++;
		}

		for (min_i = 1; min_i < min_N; min_i++) {
			double min_pos;

			if (base10) {
				min_pos = maj_pos *
					go_pow10 (min_i / base10_unit) *
					(1 + min_i % base10_unit);
			} else {
				double min_lpos = maj_lpos + min_i * min_step;
				min_pos = pow (10, min_lpos);
			}

			if (min_pos < minimum)
				continue;
			if (min_pos > maximum)
				break;

			g_assert (t < N);
			ticks[t].position = min_pos;
			ticks[t].type = GOG_AXIS_TICK_MINOR;
			ticks[t].str = NULL;
			t++;
		}
	}

	if (t > N)
		g_critical ("[GogAxisMap::log_calc_ticks] wrong allocation size");
	gog_axis_set_ticks (axis, t, ticks);
}

/*****************************************************************************/

static const GogAxisMapDesc map_desc_discrete =
{
	map_discrete,			map_discrete_to_view,   map_discrete_derivative_to_view,
	map_discrete_from_view,		go_finite,
	map_baseline,			map_bounds,
	map_discrete_init,		NULL,
	NULL,
	map_discrete_auto_bound,	map_discrete_calc_ticks,
	NULL,
	N_("Discrete"),			N_("Discrete mapping")
};

static const GogAxisMapDesc map_desc_linear =
{
	map_linear,		map_linear_to_view,     map_linear_derivative_to_view,
	map_linear_from_view,   go_finite,
	map_baseline,		map_bounds,
	map_linear_init, 	NULL,
	map_linear_subclass,
	map_linear_auto_bound, 	map_linear_calc_ticks,
	NULL,
	N_("Linear"),		N_("Linear mapping")
};

static const GogAxisMapDesc map_desc_log =
{
	map_log,		map_log_to_view,	map_log_derivative_to_view,
	map_log_from_view,	map_log_finite,
	map_log_baseline,	map_log_bounds,
	map_log_init,		NULL,
	NULL,
	map_log_auto_bound, 	map_log_calc_ticks,
	NULL,
	N_("Log"),		N_("Logarithm mapping")
};

static const GogAxisMapDesc *map_descs[] = {
	&map_desc_linear,
	&map_desc_log
};

#ifdef GOFFICE_WITH_GTK
static void
gog_axis_map_set_by_num (GogAxis *axis, gint num)
{
	g_return_if_fail (GOG_IS_AXIS (axis));

	if (num >= 0 && num < (gint)G_N_ELEMENTS (map_descs))
		g_object_set (G_OBJECT (axis), "map-name", map_descs[num]->name, NULL);
	else
		g_object_set (G_OBJECT (axis), "map-name", "", NULL);
}

static void
gog_axis_map_populate_combo (GogAxis *axis, GtkComboBoxText *combo)
{
	unsigned i;

	g_return_if_fail (GOG_IS_AXIS (axis));

	for (i = 0; i < G_N_ELEMENTS (map_descs); i++) {
		const char *thisname = map_descs[i]->name;
		gtk_combo_box_text_append_text (combo, _(thisname));
		if (!g_ascii_strcasecmp (thisname, axis->map_desc->name))
			gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
	}
}
#endif

static void
gog_axis_figure_subclass (GogAxis *axis)
{
	if (axis->is_discrete) {
		axis->actual_map_desc = &map_desc_discrete;
		return;
	}

	axis->actual_map_desc = axis->map_desc;
	while (axis->actual_map_desc->subclass) {
		const GogAxisMapDesc *c =
			axis->actual_map_desc->subclass (axis, axis->actual_map_desc);
		if (!c)
			break;
		axis->actual_map_desc = c;
	}
}

static void
gog_axis_map_set (GogAxis *axis, char const *name)
{
	unsigned i, map = 0;

	g_return_if_fail (GOG_IS_AXIS (axis));

	if (name != NULL)
		for (i = 0; i < G_N_ELEMENTS(map_descs); i++) {
			const char *thisname = map_descs[i]->name;
			if (!g_ascii_strcasecmp (name, thisname)) {
				map = i;
				break;
			}
		}
	axis->map_desc = map_descs[map];
	gog_axis_figure_subclass (axis);
}

/**
 * gog_axis_map_is_valid:
 * @map: a #GogAxisMap
 *
 * Tests if @map was correctly initialized, i.e. if bounds are
 * valid.
 *
 * Returns: %TRUE if map is valid
 **/
gboolean
gog_axis_map_is_valid (GogAxisMap *map)
{
	g_return_val_if_fail (map != NULL, FALSE);

	return map->is_valid;
}

/**
 * gog_axis_map_new:
 * @axis: a #GogAxis
 * @offset: start of plot area.
 * @length: length of plot area.
 *
 * Creates a #GogAxisMap for data mapping to plot window.
 * offset and length are optional parameters to be used with
 * gog_axis_map_to_view in order to translates data coordinates
 * into canvas space.
 *
 * Returns: (transfer full): a newly allocated #GogAxisMap.
 **/
GogAxisMap *
gog_axis_map_new (GogAxis *axis, double offset, double length)
{
	GogAxisMap *map;

	g_return_val_if_fail (GOG_IS_AXIS (axis), NULL);

	map = g_new0 (GogAxisMap, 1);

	g_object_ref (axis);
	map->desc = axis->actual_map_desc;
	map->axis = axis;
	map->data = NULL;
	map->is_valid = FALSE;
	map->ref_count = 1;

	if (axis->type != GOG_AXIS_CIRCULAR) {
		offset += axis->span_start * length;
		length *= axis->span_end - axis->span_start;
	}

	if (map->desc->init != NULL)
		map->is_valid = map->desc->init (map, offset, length);

	return map;
}

/**
 * gog_axis_map:
 * @map: a #GogAxisMap
 * @value: value to map to plot space.
 *
 * Converts @value to plot coordinates. A value in [0,1.0] range means a data
 * within axis bounds.
 *
 * Returns: mapped value.
 **/
double
gog_axis_map (GogAxisMap *map, double value)
{
	g_return_val_if_fail (map != NULL, -1);

	return (map->axis->inverted
		? 1.0 - map->desc->map (map, value)
		: map->desc->map (map, value));
}

/**
 * gog_axis_map_from_view:
 * @map: a #GogAxisMap
 * @value: value to unmap from canvas space.
 *
 * Converts value from canvas space to data space.
 *
 * return value: value in data coordinates
 **/

double
gog_axis_map_from_view (GogAxisMap *map, double value)
{
	g_return_val_if_fail (map != NULL, 0);

	return map->desc->map_from_view (map, value);
}

/**
 * gog_axis_map_to_view:
 * @map: a #GogAxisMap
 * @value: value to map to canvas space
 *
 * Converts value from data space to canvas space, using
 * offset and length parameters given to gog_axis_map_new.
 *
 * return value: a value in canvas coordinates
 **/

double
gog_axis_map_to_view (GogAxisMap *map, double value)
{
	g_return_val_if_fail (map != NULL, 0);

	return map->desc->map_to_view (map, value);
}

/**
 * gog_axis_map_derivative_to_view:
 * @map: a #GogAxisMap
 * @value: value to map to canvas space
 *
 * Returns: the derivative of the mapping expression at value.
 **/

double
gog_axis_map_derivative_to_view (GogAxisMap *map, double value)
{
	g_return_val_if_fail (map != NULL, 0);

	return map->desc->map_derivative_to_view (map, value);
}

/**
 * gog_axis_map_finite:
 * @map: a #GogAxisMap
 * @value: value to test
 *
 * Tests whether @value is valid for the given @map.
 *
 * return value: %TRUE if value means something
 **/

gboolean
gog_axis_map_finite (GogAxisMap *map, double value)
{
	g_return_val_if_fail (map != NULL, FALSE);

	return map->desc->map_finite (value);
}

/**
 * gog_axis_map_get_baseline:
 * @map: a #GogAxisMap
 *
 * Returns: the baseline for @map, in view coordinates,
 * 	clipped to offset and offset+length, where offset and length
 * 	are the parameters of gog_axis_map_new.
 **/
double
gog_axis_map_get_baseline (GogAxisMap *map)
{
	g_return_val_if_fail (map != NULL, 0);

	return map->desc->map_baseline (map);
}

/**
 * gog_axis_map_get_extents:
 * @map: a #GogAxisMap
 * @start: (out) (optional): location to store start for this axis
 * @stop: (out) (optional): location to store stop for this axis
 *
 * Gets start and stop for the whole chart relative to the given axis map
 * in data coordinates. If axis is not inverted, start = minimum and
 * stop = maximum.  If axis is invalid, it'll return arbitrary bounds.
 **/

void
gog_axis_map_get_extents (GogAxisMap *map, double *start, double *stop)
{
	double x0, x1;
	g_return_if_fail (map != NULL);

	if (gog_axis_is_inverted (map->axis))
		map->desc->map_bounds (map, &x1, &x0);
	else
		map->desc->map_bounds (map, &x0, &x1);
	if (map->axis->type != GOG_AXIS_CIRCULAR) {
		if (gog_axis_is_discrete (map->axis)) {
			double buf = (x1 - x0) / (map->axis->span_end - map->axis->span_start);
			x0 -= map->axis->span_start * buf;
			x1 = x0 + buf;
		} else {
			double t, buf;
			t = gog_axis_map_to_view (map, x0);
			buf = (gog_axis_map_to_view (map, x1) - t) / (map->axis->span_end - map->axis->span_start);
			t -= map->axis->span_start * buf;
			x0 = gog_axis_map_from_view (map, t);
			x1 = gog_axis_map_from_view (map, t + buf);
		}
	}
	if (start)
		*start = x0;
	if (stop)
		*stop = x1;
}

/**
 * gog_axis_map_get_real_extents:
 * @map: a #GogAxisMap
 * @start: (out) (optional): location to store start for this axis
 * @stop: (out) (optional): location to store stop for this axis
 *
 * Gets start and stop for the given axis map in data coordinates. If
 * axis is not inverted, start = minimum and stop = maximum.  If axis is
 * invalid, it'll return arbitrary bounds.
 **/

void
gog_axis_map_get_real_extents (GogAxisMap *map, double *start, double *stop)
{
	double x0, x1;
	g_return_if_fail (map != NULL);

	if (gog_axis_is_inverted (map->axis))
		map->desc->map_bounds (map, &x1, &x0);
	else
		map->desc->map_bounds (map, &x0, &x1);
	if (start)
		*start = x0;
	if (stop)
		*stop = x1;
}

/**
 * gog_axis_map_get_bounds:
 * @map: a #GogAxisMap
 * @minimum: (out) (optional): location to store minimum for this axis
 * @maximum: (out) (optional): location to store maximum for this axis
 *
 * Gets bounds for the whole chart relative to the given axis map in data
 * coordinates. If axis is invalid, it'll return arbitrary bounds.
 **/
void
gog_axis_map_get_bounds (GogAxisMap *map, double *minimum, double *maximum)
{
	double x0, x1;
	g_return_if_fail (map != NULL);

	if (gog_axis_is_inverted (map->axis))
		map->desc->map_bounds (map, &x1, &x0);
	else
		map->desc->map_bounds (map, &x0, &x1);
	if (map->axis->type != GOG_AXIS_CIRCULAR) {
		if (gog_axis_is_discrete (map->axis)) {
			double buf = (x1 - x0) / (map->axis->span_end - map->axis->span_start);
			x0 -= map->axis->span_start * buf;
			x1 = x0 + buf;
		} else {
			double t, buf;
			t = gog_axis_map_to_view (map, x0);
			buf = (gog_axis_map_to_view (map, x1) - t) / (map->axis->span_end - map->axis->span_start);
			t -= map->axis->span_start * buf;
			x0 = gog_axis_map_from_view (map, t);
			x1 = gog_axis_map_from_view (map, t + buf);
		}
	}
	if (minimum)
		*minimum = (map->axis->inverted)? x1: x0;
	if (maximum)
		*maximum = (map->axis->inverted)? x0: x1;
}

/**
 * gog_axis_map_get_real_bounds:
 * @map: a #GogAxisMap
 * @minimum: (out) (optional): location to store minimum for this axis
 * @maximum: (out) (optional): location to store maximum for this axis
 *
 * Gets bounds for the given axis map in data coordinates. If axis is invalid,
 * it'll return arbitrary bounds.
 **/
void
gog_axis_map_get_real_bounds (GogAxisMap *map, double *minimum, double *maximum)
{
	double x0, x1;
	g_return_if_fail (map != NULL);

	if (gog_axis_is_inverted (map->axis))
		map->desc->map_bounds (map, &x1, &x0);
	else
		map->desc->map_bounds (map, &x0, &x1);
	if (minimum)
		*minimum = (map->axis->inverted)? x1: x0;
	if (maximum)
		*maximum = (map->axis->inverted)? x0: x1;
}

/**
 * gog_axis_map_is_inverted:
 * @map: a #GogAxisMap
 *
 * Returns: %TRUE is the axis is inverted;
 **/
gboolean
gog_axis_map_is_inverted (GogAxisMap *map)
{
	g_return_val_if_fail (map != NULL, FALSE);

	return map->axis->inverted;
}

/**
 * gog_axis_map_is_discrete:
 * @map: a #GogAxisMap
 *
 * Returns: %TRUE is the axis is discrete;
 **/
gboolean
gog_axis_map_is_discrete (GogAxisMap *map)
{
	g_return_val_if_fail (map != NULL, FALSE);

	return map->axis->is_discrete;
}

/**
 * gog_axis_map_free:
 * @map: (transfer full): a #GogAxisMap
 *
 * Frees #GogAxisMap object.
 **/
void
gog_axis_map_free (GogAxisMap *map)
{
	g_return_if_fail (map != NULL);

	if (map->ref_count-- > 1)
		return;
	if (map->desc->destroy != NULL)
		map->desc->destroy (map);

	g_object_unref (map->axis);
	g_free (map->data);
	g_free (map);
}

static GogAxisMap *
gog_axis_map_ref (GogAxisMap *map)
{
	map->ref_count++;
	return map;
}

GType
gog_axis_map_get_type (void)
{
	static GType t = 0;

	if (t == 0)
		t = g_boxed_type_register_static ("GogAxisMap",
			 (GBoxedCopyFunc) gog_axis_map_ref,
			 (GBoxedFreeFunc) gog_axis_map_free);
	return t;
}

static void
gog_axis_auto_bound (GogAxis *axis)
{
	double maximum, minimum;
	double tmp;
	gboolean user_defined;

	g_return_if_fail (GOG_IS_AXIS (axis));

	minimum = axis->min_val;
	maximum = axis->max_val;

	/*
	 * We handle user-set minimum and maximum by pretending to have
	 * data in that range.  This is a bit dubious.
	 */
	tmp = gog_axis_get_entry (axis, GOG_AXIS_ELEM_MIN, &user_defined);
	if (user_defined) minimum = tmp;

	tmp = gog_axis_get_entry (axis, GOG_AXIS_ELEM_MAX, &user_defined);
	if (user_defined) maximum = tmp;

	if (axis->actual_map_desc->auto_bound)
		axis->actual_map_desc->auto_bound (axis,
						   minimum, maximum,
						   axis->auto_bound);
}

static void
gog_axis_set_ticks (GogAxis *axis, int tick_nbr, GogAxisTick *ticks)
{
	unsigned i;

	g_return_if_fail (GOG_IS_AXIS (axis));

	if (axis->ticks != NULL) {
		for (i = 0; i < axis->tick_nbr; i++)
			go_string_unref (axis->ticks[i].str);

		g_free (axis->ticks);
	}

	axis->tick_nbr = tick_nbr;
	axis->ticks = ticks;
}

static void
gog_axis_calc_ticks (GogAxis *axis)
{
	g_return_if_fail (GOG_IS_AXIS (axis));

	if (axis->actual_map_desc->calc_ticks)
		axis->actual_map_desc->calc_ticks (axis);

	if (axis->type == GOG_AXIS_PSEUDO_3D || axis->type == GOG_AXIS_Z) {
		GSList *l;
		for (l = axis->contributors; l; l = l->next) {
			gog_plot_update_3d (GOG_PLOT (l->data));
		}
	}
}

#ifdef GOFFICE_WITH_GTK
static GOFormat *
gog_axis_get_dim_format (GogAxis *axis, unsigned dim)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), NULL);

	if (!axis->actual_map_desc->get_dim_format)
		return NULL;

	return axis->actual_map_desc->get_dim_format (axis, dim);
}
#endif

/************************************************************************/

typedef GogAxisBaseClass GogAxisClass;

static GType gog_axis_view_get_type (void);

static GObjectClass *parent_klass;

enum {
	AXIS_PROP_0,
	AXIS_PROP_TYPE,
	AXIS_PROP_INVERT,
	AXIS_PROP_MAP,
	AXIS_PROP_ASSIGNED_FORMAT_STR_XL,
	AXIS_PROP_CIRCULAR_ROTATION,
	AXIS_PROP_POLAR_UNIT,
	AXIS_PROP_SPAN_START,
	AXIS_PROP_SPAN_END,
	AXIS_PROP_COLOR_MAP,
	AXIS_PROP_METRICS,
	AXIS_PROP_REF_AXIS,
	AXIS_PROP_METRICS_RATIO,
	AXIS_PROP_METRICS_UNIT,
	AXIS_PROP_DISPLAY_FACTOR
};

/*****************************************************************************/

static gboolean
role_axis_line_can_add (GogObject const *parent)
{
	GogChart *chart = GOG_AXIS_BASE (parent)->chart;
	GogAxisSet axis_set = gog_chart_get_axis_set (chart);

	if (axis_set == GOG_AXIS_SET_XY ||
	    (axis_set == GOG_AXIS_SET_RADAR &&
	     gog_axis_get_atype (GOG_AXIS (parent)) == GOG_AXIS_RADIAL))
		return TRUE;

	return FALSE;
}

static void
role_axis_line_post_add (GogObject *parent, GogObject *child)
{
	gog_axis_base_set_position (GOG_AXIS_BASE (child), GOG_AXIS_AUTO);
}

/**
 * gog_axis_set_format:
 * @axis: #GogAxis
 * @fmt: (transfer full) (nullable): #GOFormat
 *
 * Returns: %TRUE if things changed
 **/
gboolean
gog_axis_set_format (GogAxis *axis, GOFormat *fmt)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), FALSE);

	if (go_format_eq (fmt, axis->assigned_format)) {
		go_format_unref (fmt);
		return FALSE;
	}

	go_format_unref (axis->assigned_format);
	axis->assigned_format = fmt;

	gog_axis_figure_subclass (axis);

	gog_object_request_update (GOG_OBJECT (axis));
	return TRUE;
}

/**
 * gog_axis_get_format:
 * @axis: #GogAxis
 *
 * Returns: (transfer none): the format assigned to @axis.
 **/
GOFormat *
gog_axis_get_format (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), NULL);
	return axis->assigned_format;
}

static gboolean
gog_axis_set_atype (GogAxis *axis, GogAxisType new_type)
{
	if (axis->type == new_type)
		return FALSE;

	axis->type = new_type;
	gog_axis_figure_subclass (axis);
	if (new_type == GOG_AXIS_PSEUDO_3D)
		g_object_set (G_OBJECT (axis),
			      "major-tick-labeled", FALSE,
			      "major-tick-in", FALSE,
			      "major-tick-out", FALSE,
			      "minor-tick-in", FALSE,
			      "minor-tick-out", FALSE,
			      NULL);
	return TRUE;
}

GogAxisType
gog_axis_get_atype (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), GOG_AXIS_UNKNOWN);
	return axis->type;
}

static void
gog_axis_prep_sax (GOPersist *gp, GsfXMLIn *xin, xmlChar const **attrs)
{
	/* Nothing to do */
}

static void
gog_axis_sax_save (GOPersist const *gp, GsfXMLOut *output)
{
	GogAxis const *axis;
	GoResourceType type;

	g_return_if_fail (GOG_IS_AXIS (gp));
	axis = (GogAxis const*) gp;
	if (axis->auto_color_map)
		return;
	type = gog_axis_color_map_get_resource_type (axis->color_map);
	if (type == GO_RESOURCE_CHILD || type == GO_RESOURCE_NATIVE)
		return;
	go_doc_save_resource (gog_graph_get_document (gog_object_get_graph (GOG_OBJECT (gp))),
	                      GO_PERSIST (axis->color_map));
}

struct axis_ref_struct {
	GogAxis *axis;
	char *ref;
};

static gboolean
axis_ref_load_cb (struct axis_ref_struct *s)
{
	GogObject *parent = gog_object_get_parent ((GogObject *) s->axis);
	GogAxis *axis;
	GSList *ptr, *l;
	l = gog_object_get_children (parent, NULL);
	s->axis->ref_axis = NULL;
	if (s->axis->refering_axes != NULL) {
		s->axis->metrics = GOG_AXIS_METRICS_DEFAULT;
		s->axis->metrics_ratio = 1.;
		s->axis->unit = GO_UNIT_CENTIMETER;
		goto clean;
	}
	for (ptr = l; (ptr != NULL) && (s->axis->ref_axis == NULL); ptr = ptr->next) {
		if (!GOG_IS_AXIS (ptr->data))
			continue;
		axis = ptr->data;
		if (axis->metrics > GOG_AXIS_METRICS_ABSOLUTE || axis == s->axis)
			continue;
		if (!strcmp (gog_object_get_name (ptr->data), s->ref)) {
			s->axis->ref_axis = axis;
			axis->refering_axes = g_slist_prepend (axis->refering_axes, s->axis);
		}
	}
	g_slist_free (l);
clean:
	gog_object_request_update ((GogObject *) s->axis);
	g_free (s->ref);
	g_free (s);
	return FALSE;
}

static void
gog_axis_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogAxis *axis = GOG_AXIS (obj);
	gboolean resized = FALSE;
	gboolean calc_ticks = FALSE;
	gboolean request_update = FALSE;

	switch (param_id) {
	case AXIS_PROP_TYPE:
		resized = gog_axis_set_atype (axis, g_value_get_int (value));
		break;

	case AXIS_PROP_INVERT: {
		gboolean new_inv = g_value_get_boolean (value);
		if (axis->inverted != new_inv) {
			axis->inverted = new_inv;
			resized = calc_ticks = TRUE;
		}
		break;
	}
	case AXIS_PROP_MAP: {
		const char *s = g_value_get_string (value);
		if (go_str_compare (s, axis->map_desc->name)) {
			gog_axis_map_set (axis, s);
			request_update = TRUE;
		}
		break;
	}
	case AXIS_PROP_ASSIGNED_FORMAT_STR_XL : {
		char const *str = g_value_get_string (value);
		GOFormat *newfmt = str ? go_format_new_from_XL (str) : NULL;
		resized = calc_ticks = gog_axis_set_format (axis, newfmt);
		break;
	}
	case AXIS_PROP_CIRCULAR_ROTATION:
		axis->circular_rotation = CLAMP (g_value_get_double (value),
						 GOG_AXIS_CIRCULAR_ROTATION_MIN,
						 GOG_AXIS_CIRCULAR_ROTATION_MAX);
		break;
	case AXIS_PROP_POLAR_UNIT: {
		char const *str = g_value_get_string (value);
		unsigned int i;
		for (i = 0; i < G_N_ELEMENTS (polar_units); i++) {
			if (g_ascii_strcasecmp (str, polar_units[i].name) == 0) {
				axis->polar_unit = i;
				resized = calc_ticks = TRUE;
				break;
			}
		}
		break;
	}
	case AXIS_PROP_SPAN_START:
		if (axis->type != GOG_AXIS_CIRCULAR) {
			double new_value = g_value_get_double (value);
			g_return_if_fail (new_value < axis->span_end);
			axis->span_start = new_value;
		}
		break;
	case AXIS_PROP_SPAN_END:
		if (axis->type != GOG_AXIS_CIRCULAR) {
			double new_value = g_value_get_double (value);
			g_return_if_fail (new_value > axis->span_start);
			axis->span_end = new_value;
		}
		break;
	case AXIS_PROP_COLOR_MAP: {
		char const *str = g_value_get_string (value);
		GogAxisColorMap const *map = NULL;
		if (strcmp (str, "default")) {
			GogGraph *graph = gog_object_get_graph (GOG_OBJECT (axis));
			map = gog_theme_get_color_map (gog_graph_get_theme (graph), FALSE);
			if (strcmp (gog_axis_color_map_get_id (map), str))
				map = gog_axis_color_map_get_from_id (str);
		}
		if (map) {
			axis->color_map = map;
			axis->auto_color_map = FALSE;
		}
		break;
	}
	case AXIS_PROP_METRICS:
		axis->metrics = gog_axis_metrics_from_str (g_value_get_string (value));
		break;
	case AXIS_PROP_REF_AXIS: {
		char const *str = g_value_get_string (value);
		if ((axis->ref_axis == NULL && !strcmp (str, "none")) ||
		    (axis->ref_axis && !strcmp (gog_object_get_name (GOG_OBJECT (axis->ref_axis)), str)))
			return;
		if (strcmp (str, "none")) {
			struct axis_ref_struct *s = g_new (struct axis_ref_struct, 1);
			s->ref = g_strdup (str);
			s->axis = axis;
			g_idle_add ((GSourceFunc) axis_ref_load_cb, s);
			return;
		}
		axis->ref_axis = NULL;
		break;
	}
	case AXIS_PROP_METRICS_RATIO:
		axis->metrics_ratio = g_value_get_double (value);
		break;
	case AXIS_PROP_METRICS_UNIT:
		axis->unit = (strcmp (g_value_get_string (value), "in"))? GO_UNIT_CENTIMETER: GO_UNIT_INCH;
		break;
	case AXIS_PROP_DISPLAY_FACTOR:
		axis->display_factor = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	if (request_update)
		gog_object_request_update (GOG_OBJECT (axis));
	else {
		if (calc_ticks)
			gog_axis_calc_ticks (axis);
		gog_object_emit_changed (GOG_OBJECT (obj), resized);
	}
}

static void
gog_axis_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogAxis const *axis = GOG_AXIS (obj);

	switch (param_id) {
	case AXIS_PROP_TYPE:
		g_value_set_int (value, axis->type);
		break;
	case AXIS_PROP_INVERT:
		g_value_set_boolean (value, axis->inverted);
		break;
	case AXIS_PROP_MAP:
		g_value_set_string (value, axis->map_desc->name);
		break;
	case AXIS_PROP_ASSIGNED_FORMAT_STR_XL :
		if (axis->assigned_format != NULL)
			g_value_set_string (value,
				go_format_as_XL	(axis->assigned_format));
		else
			g_value_set_static_string (value, NULL);
		break;
	case AXIS_PROP_CIRCULAR_ROTATION:
		g_value_set_double (value, axis->circular_rotation);
		break;
	case AXIS_PROP_POLAR_UNIT:
		g_value_set_string (value, polar_units[axis->polar_unit].name);
		break;
	case AXIS_PROP_SPAN_START:
		g_value_set_double (value, axis->span_start);
		break;
	case AXIS_PROP_SPAN_END:
		g_value_set_double (value, axis->span_end);
		break;
	case AXIS_PROP_COLOR_MAP:
		g_value_set_string (value, (axis->auto_color_map)? "default": gog_axis_color_map_get_id (axis->color_map));
		break;
	case AXIS_PROP_METRICS:
		g_value_set_string (value, gog_axis_metrics_as_str (axis->metrics));
		break;
	case AXIS_PROP_REF_AXIS:
		if (axis->ref_axis == NULL || axis->metrics < GOG_AXIS_METRICS_RELATIVE)
			g_value_set_string (value, "none");
		else
			g_value_set_string (value, gog_object_get_name ((GogObject *) axis->ref_axis));
		break;
	case AXIS_PROP_METRICS_RATIO:
		g_value_set_double (value, axis->metrics_ratio);
		break;
	case AXIS_PROP_METRICS_UNIT:
		g_value_set_string (value, (axis->unit == GO_UNIT_INCH)? "in": "cm");
		break;
	case AXIS_PROP_DISPLAY_FACTOR:
		g_value_set_double (value, axis->display_factor);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_axis_finalize (GObject *obj)
{
	GogAxis *axis = GOG_AXIS (obj);

	gog_axis_clear_contributors (axis);

	g_slist_free (axis->contributors);	axis->contributors = NULL;
	if (axis->labels != NULL) {
		g_object_unref (axis->labels);
		axis->labels   = NULL;
		/* this is for information only, no ref */
		axis->plot_that_supplied_labels = NULL;
	}
	go_format_unref (axis->assigned_format);
	go_format_unref (axis->format);

	gog_axis_set_ticks (axis, 0, NULL);

	gog_dataset_finalize (GOG_DATASET (axis));
	(parent_klass->finalize) (obj);
}

/**
 * gog_axis_get_entry:
 * @axis: #GogAxis
 * @i:
 * @user_defined: an optionally %NULL pointr to gboolean
 *
 * Returns: the value of axis element @i and sets @user_defined or
 * 	NaN on error
 **/
double
gog_axis_get_entry (GogAxis const *axis, GogAxisElemType i, gboolean *user_defined)
{
	GOData *dat;

	if (user_defined)
		*user_defined = FALSE;

	g_return_val_if_fail (GOG_IS_AXIS (axis), go_nan);
	g_return_val_if_fail (i >= GOG_AXIS_ELEM_MIN && i < GOG_AXIS_ELEM_MAX_ENTRY, go_nan);

	if (i != GOG_AXIS_ELEM_CROSS_POINT)
		dat = axis->source[i].data;
	else
		dat = GOG_AXIS_BASE (axis)->cross_location.data;

	if (GO_IS_DATA (dat)) {
		double tmp = go_data_get_scalar_value (dat);
		if (go_finite (tmp)) {
			if (user_defined)
				*user_defined = TRUE;
			return tmp;
		}
	}

	if (i != GOG_AXIS_ELEM_CROSS_POINT)
		return axis->auto_bound[i];
	else
		return 0.;
}

static void
gog_axis_update (GogObject *obj)
{
	GSList *ptr;
	GogAxis *axis = GOG_AXIS (obj);
	double old_min = axis->auto_bound[GOG_AXIS_ELEM_MIN];
	double old_max = axis->auto_bound[GOG_AXIS_ELEM_MAX];
	GogPlotBoundInfo bounds;

	gog_debug (0, g_warning ("axis::update"););

	if (axis->labels != NULL) {
		g_object_unref (axis->labels);
		axis->labels   = NULL;
		axis->plot_that_supplied_labels = NULL;
	}
	axis->is_discrete = TRUE;
	axis->min_val = +DBL_MAX;
	axis->max_val = -DBL_MAX;
	axis->min_contrib = axis->max_contrib = NULL;
	go_format_unref (axis->format);
	axis->format = NULL;
	axis->date_conv = NULL;

	/* Everything else is initialized in gog_plot_get_axis_bounds */
	bounds.fmt = NULL;

	for (ptr = axis->contributors ; ptr != NULL ; ptr = ptr->next) {
		GogPlot *plot = GOG_PLOT (ptr->data);
		GogObject *ploto = GOG_OBJECT (plot);
		GOData *labels;

		labels = gog_plot_get_axis_bounds (plot, axis->type, &bounds);
		if (bounds.date_conv)
			axis->date_conv = bounds.date_conv;

		/* value dimensions have more information than index dimensions.
		 * At least thats what I am guessing today*/
		if (!bounds.is_discrete)
			axis->is_discrete = FALSE;
		else if (axis->labels == NULL && labels != NULL) {
			g_object_ref (labels);
			axis->labels = labels;
			axis->plot_that_supplied_labels = plot;
		}
		axis->center_on_ticks = bounds.center_on_ticks;

		if (axis->min_val > bounds.val.minima) {
			axis->min_val = bounds.val.minima;
			axis->logical_min_val = bounds.logical.minima;
			axis->min_contrib = ploto;
		} else if (axis->min_contrib == ploto) {
			axis->min_contrib = NULL;
			axis->min_val = bounds.val.minima;
		}

		if (axis->max_val < bounds.val.maxima) {
			axis->max_val = bounds.val.maxima;
			axis->logical_max_val = bounds.logical.maxima;
			axis->max_contrib = ploto;
		} else if (axis->max_contrib == ploto) {
			axis->max_contrib = NULL;
			axis->max_val = bounds.val.maxima;
		}
	}
	axis->format = bounds.fmt; /* just absorb the ref if it exists */

	gog_axis_figure_subclass (axis);
	
	gog_axis_auto_bound (axis);

	if (go_finite (axis->logical_min_val) &&
	    axis->auto_bound[GOG_AXIS_ELEM_MIN] < axis->logical_min_val)
		axis->auto_bound[GOG_AXIS_ELEM_MIN] = axis->logical_min_val;
	if (go_finite (axis->logical_max_val) &&
	    axis->auto_bound[GOG_AXIS_ELEM_MAX] > axis->logical_max_val)
		axis->auto_bound[GOG_AXIS_ELEM_MAX] = axis->logical_max_val;

	gog_axis_calc_ticks (axis);

	if (old_min != axis->auto_bound[GOG_AXIS_ELEM_MIN] ||
	    old_max != axis->auto_bound[GOG_AXIS_ELEM_MAX])
		gog_object_emit_changed (GOG_OBJECT (obj), TRUE);
}

#ifdef GOFFICE_WITH_GTK

typedef struct {
	GogAxis		*axis;
	GtkWidget 	*format_selector;
	GtkComboBox *color_map_combo;
	GOCmdContext *cc;
	GtkBuilder *gui;
	GogDataEditor *de[GOG_AXIS_ELEM_CROSS_POINT];
} GogAxisPrefState;

static void
gog_axis_pref_state_free (GogAxisPrefState *state)
{
	if (state->axis != NULL)
		g_object_unref (state->axis);
	g_free (state);
}

static void
cb_axis_toggle_changed (GtkToggleButton *toggle_button, GObject *axis)
{
	g_object_set (axis,
		gtk_buildable_get_name (GTK_BUILDABLE (toggle_button)),
		gtk_toggle_button_get_active (toggle_button),
		NULL);
}

typedef struct {
	GtkWidget *editor;
	GtkToggleButton *toggle;
	GogDataset *set;
	unsigned dim;
	gulong update_editor_handler;
	gulong toggle_handler;
} ElemToggleData;

static void
elem_toggle_data_free (ElemToggleData *data)
{
	g_signal_handler_disconnect (G_OBJECT (data->set), data->update_editor_handler);
	g_free (data);
}

static void
set_to_auto_value (ElemToggleData *closure)
{
	GogAxis *axis = GOG_AXIS (closure->set);
	double bound = axis->auto_bound[closure->dim];
	GODateConventions const *date_conv = axis->date_conv;

	gog_data_editor_set_value_double (GOG_DATA_EDITOR (closure->editor),
					  bound, date_conv);
}

static void
cb_enable_dim (GtkToggleButton *toggle_button, ElemToggleData *closure)
{
	gboolean is_auto = gtk_toggle_button_get_active (toggle_button);

	gtk_widget_set_sensitive (closure->editor, !is_auto);

	if (is_auto) {
		gog_dataset_set_dim (closure->set, closure->dim, NULL, NULL);
		set_to_auto_value (closure);
	} else {
		GogAxis *axis = GOG_AXIS (closure->set);
		GOData *data = go_data_scalar_val_new (axis->auto_bound[closure->dim]);
		gog_dataset_set_dim (closure->set, closure->dim, data, NULL);
	}

}

static void
cb_update_dim_editor (GogObject *gobj, ElemToggleData *closure)
{
	gboolean is_auto;

	g_signal_handler_block (closure->toggle, closure->toggle_handler);

	is_auto = (gog_dataset_get_dim (closure->set, closure->dim) == NULL);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (closure->toggle), is_auto);
	gtk_widget_set_sensitive (GTK_WIDGET (closure->editor), !is_auto);

	if (is_auto)
		set_to_auto_value (closure);

	g_signal_handler_unblock (closure->toggle, closure->toggle_handler);
}

static GogDataEditor *
make_dim_editor (GogDataset *set, GtkGrid *grid, unsigned dim,
		 GogDataAllocator *dalloc, char const *dim_name)
{
	ElemToggleData *info;
	GogDataEditor *deditor = gog_data_allocator_editor (dalloc, set, dim, GOG_DATA_SCALAR);
	GtkWidget *editor = GTK_WIDGET (deditor);
	char *txt;
	GtkWidget *toggle;
	GogAxis *axis = GOG_AXIS (set);
	GOFormat const *fmt;

	txt = g_strconcat (dim_name, ":", NULL);
	toggle = gtk_check_button_new_with_mnemonic (txt);
	g_free (txt);

	fmt = gog_axis_get_dim_format (axis, dim);
	gog_data_editor_set_format (deditor, fmt);
	go_format_unref (fmt);

	info = g_new0 (ElemToggleData, 1);
	info->editor = editor;
	info->set = set;
	info->dim = dim;
	info->toggle = GTK_TOGGLE_BUTTON (toggle);

	info->toggle_handler = g_signal_connect (G_OBJECT (toggle),
						 "toggled",
						 G_CALLBACK (cb_enable_dim), info);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toggle),
				      gog_dataset_get_dim (set, dim) == NULL);

	info->update_editor_handler = g_signal_connect (G_OBJECT (set),
							"update-editor",
							G_CALLBACK (cb_update_dim_editor), info);

	g_object_weak_ref (G_OBJECT (toggle), (GWeakNotify) elem_toggle_data_free, info);

	gtk_grid_attach (grid, toggle,
		0, dim + 1, 1, 1);
	g_object_set (G_OBJECT (editor), "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, editor,
		1, dim + 1, 1, 1);
	return GOG_DATA_EDITOR (editor);
}

static void
cb_axis_fmt_changed (G_GNUC_UNUSED GtkWidget *widget,
		     char *fmt,
		     GogAxis *axis)
{
	g_object_set (axis, "assigned-format-string-XL", fmt, NULL);
}

static void
cb_map_combo_changed (GtkComboBox *combo,
		      GogAxis *axis)
{
	gog_axis_map_set_by_num (axis, gtk_combo_box_get_active (combo));
}

static void
gog_axis_populate_polar_unit_combo (GogAxis *axis, GtkComboBoxText *combo)
{
	unsigned i, id = 0;

	g_return_if_fail (GOG_IS_AXIS (axis));

	for (i = 0; i < G_N_ELEMENTS (polar_units); i++) {
		gtk_combo_box_text_append_text (combo, _(polar_units[i].name));
		if (polar_units[i].unit == axis->polar_unit)
			id = i;
	}
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), id);
}

static void
cb_polar_unit_changed (GtkComboBox *combo,
		       GogAxisPrefState *state)
{
	GogAxis *axis = state->axis;
	GOFormat *format;
	int i;

	axis->polar_unit = gtk_combo_box_get_active (combo);
	format = go_format_new_from_XL (polar_units[axis->polar_unit].xl_format);

	if (gog_axis_set_format (axis, format) &&
	    state->format_selector != NULL)
		go_format_sel_set_style_format (GO_FORMAT_SEL (state->format_selector), format);
	gog_axis_auto_bound (axis);
	for (i = GOG_AXIS_ELEM_MIN; i < GOG_AXIS_ELEM_CROSS_POINT; i++) {
		GOData *data = gog_dataset_get_dim (GOG_DATASET (axis), i);
		gog_data_editor_set_value_double (state->de[i],
			                              (data)? go_data_get_scalar_value (data): axis->auto_bound[i],
			                              axis->date_conv);
	}
}

static void
cb_rotation_changed (GtkSpinButton *spin, GogAxis *axis)
{
	axis->circular_rotation = CLAMP (gtk_spin_button_get_value (spin),
					 GOG_AXIS_CIRCULAR_ROTATION_MIN,
					 GOG_AXIS_CIRCULAR_ROTATION_MAX);
	gog_object_emit_changed (GOG_OBJECT (axis), TRUE);
}

static void
cb_start_changed (GtkSpinButton *spin, GogAxis *axis)
{
	double val = gtk_spin_button_get_value (spin);
	GtkAdjustment *adj = GTK_ADJUSTMENT (g_object_get_data (G_OBJECT (spin), "other-adj"));
	gtk_adjustment_set_lower (adj, fmin (val + 1, axis->span_end * 100.));
	g_object_set (axis, "span-start", val /100., NULL);
}

static void
cb_end_changed (GtkSpinButton *spin, GogAxis *axis)
{
	double val = gtk_spin_button_get_value (spin);
	GtkAdjustment *adj = GTK_ADJUSTMENT (g_object_get_data (G_OBJECT (spin), "other-adj"));
	gtk_adjustment_set_upper (adj, fmax (val - 1, axis->span_start * 100.));
	g_object_set (axis, "span-end", val /100., NULL);
}

static void
color_map_new_cb (GogAxisPrefState *state)
{
	GogAxisColorMap *map = gog_axis_color_map_edit (NULL, state->cc);
	if (map) {
		GtkListStore *model = GTK_LIST_STORE (gtk_combo_box_get_model (state->color_map_combo));
		GtkTreeIter iter;
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
		                    0, gog_axis_color_map_get_name (map),
		                    1, gog_axis_color_map_get_snapshot (map, state->axis->type == GOG_AXIS_PSEUDO_3D,
		                                                        TRUE, 200, 16),
		                    2, map,
		                    -1);
		gtk_combo_box_set_active_iter (state->color_map_combo, &iter);
		gog_object_emit_changed (GOG_OBJECT (state->axis), FALSE);
	}
}

static void
color_map_dup_cb (GogAxisPrefState *state)
{
	GogAxisColorMap *map = gog_axis_color_map_dup (state->axis->color_map);
	if (gog_axis_color_map_edit (map, state->cc) != NULL) {
		GtkListStore *model = GTK_LIST_STORE (gtk_combo_box_get_model (state->color_map_combo));
		GtkTreeIter iter;
		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
		                    0, gog_axis_color_map_get_name (map),
		                    1, gog_axis_color_map_get_snapshot (map, state->axis->type == GOG_AXIS_PSEUDO_3D,
		                                                        TRUE, 200, 16),
		                    2, map,
		                    -1);
		gtk_combo_box_set_active_iter (state->color_map_combo, &iter);
		gog_object_emit_changed (GOG_OBJECT (state->axis), FALSE);
	} else
		g_object_unref (map);
}

static void
color_map_save_cb (GogAxisPrefState *state)
{
	go_persist_sax_save (GO_PERSIST (state->axis->color_map), NULL);
	gtk_widget_hide (go_gtk_builder_get_widget (state->gui, "save-btn"));
}

static void
color_map_changed_cb (GtkComboBox *combo, GogAxisPrefState *state)
{
	GtkTreeModel *model = gtk_combo_box_get_model (combo);
	GtkTreeIter iter;
	GogAxisColorMap *map;
	GogTheme *theme = gog_graph_get_theme (gog_object_get_graph (GOG_OBJECT (state->axis)));
	gtk_combo_box_get_active_iter (combo, &iter);
	gtk_tree_model_get (model, &iter, 2, &map, -1);
	state->axis->color_map = map;
	state->axis->auto_color_map = map == gog_theme_get_color_map (theme, state->axis->type == GOG_AXIS_PSEUDO_3D);
	gog_object_emit_changed (GOG_OBJECT (state->axis), FALSE);
	gtk_widget_set_visible (go_gtk_builder_get_widget (state->gui, "save-btn"),
	                        gog_axis_color_map_get_resource_type (map) == GO_RESOURCE_EXTERNAL);
}

struct ColorMapState {
	GtkComboBox *combo;
	GtkListStore *model;
	GogAxisColorMap const *target;
	gboolean discrete;
};

static void
add_color_map_cb (GogAxisColorMap const *map, struct ColorMapState *state)
{
	GtkTreeIter iter;
	GdkPixbuf *pixbuf = gog_axis_color_map_get_snapshot (map, state->discrete,
		                                          TRUE, 200, 16);
	gtk_list_store_append (state->model, &iter);
	gtk_list_store_set (state->model, &iter, 0, gog_axis_color_map_get_name (map),
	                    1, pixbuf, 2, map, -1);
	g_object_unref (pixbuf);
	if (map == state->target)
		gtk_combo_box_set_active_iter (state->combo, &iter);
}

struct MetricsState {
	GogAxis *axis;
	GtkWidget *axes, *label, *ratio, *unit;
};

static void
metrics_ratio_changed_cb (GtkSpinButton *btn, struct MetricsState *state)
{
	state->axis->metrics_ratio = gtk_spin_button_get_value (btn);
	gog_object_request_update ((GogObject *) state->axis);
}

static void
metrics_unit_changed_cb (GtkComboBox *box, struct MetricsState *state)
{
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_combo_box_get_model (box);
	gtk_combo_box_get_active_iter (box, &iter);
	gtk_tree_model_get (model, &iter, 1, &state->axis->unit, -1);
	gog_object_request_update ((GogObject *) state->axis);
}

static void
metrics_axis_changed_cb (GtkComboBox *box, struct MetricsState *state)
{
	GogAxis *ref_axis;
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (state->axis->ref_axis != NULL)
		state->axis->ref_axis->refering_axes = g_slist_remove (state->axis->ref_axis->refering_axes, state->axis);
	model = gtk_combo_box_get_model (box);
	gtk_combo_box_get_active_iter (box, &iter);
	gtk_tree_model_get (model, &iter, 1, &ref_axis, -1);
	ref_axis->refering_axes = g_slist_prepend (ref_axis->refering_axes, state->axis);
	state->axis->ref_axis = ref_axis;
	gog_object_request_update ((GogObject *) state->axis);
}

static void
metrics_changed_cb (GtkComboBox *box, struct MetricsState *state)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	model = gtk_combo_box_get_model (box);
	gtk_combo_box_get_active_iter (box, &iter);
	gtk_tree_model_get (model, &iter, 1, &state->axis->metrics, -1);
	switch (state->axis->metrics) {
	case GOG_AXIS_METRICS_DEFAULT:
		gtk_widget_hide (state->axes);
		gtk_widget_hide (state->label);
		gtk_widget_hide (state->ratio);
		gtk_widget_hide (state->unit);
		state->axis->ref_axis = NULL;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->ratio), 1.);
		gtk_combo_box_set_active (GTK_COMBO_BOX (state->unit), 0);
		break;
	case GOG_AXIS_METRICS_ABSOLUTE:
		gtk_widget_hide (state->axes);
		state->axis->ref_axis = NULL;
		gtk_label_set_text (GTK_LABEL (state->label), _("Distance:"));
		gtk_widget_show (state->label);
		gtk_widget_show (state->ratio);
		gtk_widget_show (state->unit);
		break;
	case GOG_AXIS_METRICS_RELATIVE:
		gtk_widget_show (state->axes);
		gtk_label_set_text (GTK_LABEL (state->label), _("Ratio:"));
		gtk_widget_show (state->label);
		gtk_widget_show (state->ratio);
		gtk_widget_hide (state->unit);
		if (gtk_combo_box_get_active ((GtkComboBox *) state->axes) < 0)
			gtk_combo_box_set_active ((GtkComboBox *) state->axes, 0);
		else
			metrics_axis_changed_cb (GTK_COMBO_BOX (state->axes), state);
		gtk_combo_box_set_active (GTK_COMBO_BOX (state->unit), 0);
		break;
	case GOG_AXIS_METRICS_RELATIVE_TICKS:
		gtk_widget_show (state->axes);
		gtk_label_set_text (GTK_LABEL (state->label), _("Ratio:"));
		gtk_widget_show (state->label);
		gtk_widget_show (state->ratio);
		gtk_widget_hide (state->unit);
		if (gtk_combo_box_get_active ((GtkComboBox *) state->axes) < 0)
				gtk_combo_box_set_active ((GtkComboBox *) state->axes, 0);
		else
			metrics_axis_changed_cb ((GtkComboBox *) state->axes, state);
		gtk_combo_box_set_active (GTK_COMBO_BOX (state->unit), 0);
		break;
	default:
		/* will never occur */
		break;
	}
	gog_object_request_update ((GogObject *) state->axis);
}

static void
gog_axis_populate_editor (GogObject *gobj,
			  GOEditor *editor,
			  GogDataAllocator *dalloc,
			  GOCmdContext *cc)
{
	static guint axis_pref_page = 0;
	static char const *toggle_props[] = {
		"invert-axis"
	};
	GtkWidget *w;
	GtkGrid *grid;
	unsigned i = 0;
	GogAxis *axis = GOG_AXIS (gobj);
	GogAxisPrefState *state;
	GogDataset *set = GOG_DATASET (gobj);
	GtkBuilder *gui;
	gui = go_gtk_builder_load_internal ("res:go:graph/gog-axis-prefs.ui", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	state = g_new0 (GogAxisPrefState, 1);
	state->axis = axis;
	state->cc = cc;
	state->gui = gui;
	g_object_ref (axis);

	/* Bounds Page */
	grid = GTK_GRID (gtk_builder_get_object (gui, "bound-grid"));
	if (axis->is_discrete) {
		static char const * const dim_names[] = {
			N_("M_inimum"),
			N_("M_aximum"),
			N_("Categories between _ticks"),
			N_("Categories between _labels")
		};
		for (i = GOG_AXIS_ELEM_MIN; i < GOG_AXIS_ELEM_CROSS_POINT ; i++)
			state->de[i] = make_dim_editor (set, grid, i, dalloc,
			                                _(dim_names[i]));
	} else {
		static char const * const dim_names[] = {
			N_("M_inimum"),
			N_("M_aximum"),
			N_("Ma_jor ticks"),
			N_("Mi_nor ticks")
		};

		for (i = GOG_AXIS_ELEM_MIN; i < GOG_AXIS_ELEM_CROSS_POINT ; i++)
			state->de[i] = make_dim_editor (set, grid, i, dalloc,
			                                _(dim_names[i]));
	}
	gtk_widget_show_all (GTK_WIDGET (grid));

	/* Details page */
	if (!axis->is_discrete && gog_axis_get_atype (axis) != GOG_AXIS_CIRCULAR) {
		GtkComboBoxText *box = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (gui, "map-type-combo"));
		gog_axis_map_populate_combo (axis, box);
		g_signal_connect_object (G_OBJECT (box),
					 "changed",
					 G_CALLBACK (cb_map_combo_changed),
					 axis, 0);
	} else {
		GtkWidget *w = go_gtk_builder_get_widget (gui, "map-label");
		gtk_widget_hide (w);
		w = go_gtk_builder_get_widget (gui, "map-type-combo");
		gtk_widget_hide (w);
	}

	if (gog_axis_get_atype (axis) == GOG_AXIS_CIRCULAR) {
		GtkWidget *w;
		GtkComboBoxText *box = GTK_COMBO_BOX_TEXT (gtk_builder_get_object (gui, "polar-unit-combo"));
		if (!axis->is_discrete) {
			gog_axis_populate_polar_unit_combo (axis, box);
			g_signal_connect (G_OBJECT (box),
					  "changed",
					  G_CALLBACK (cb_polar_unit_changed),
					  state);
		} else {
			gtk_widget_hide (GTK_WIDGET (box));
			gtk_widget_hide (go_gtk_builder_get_widget (gui, "unit-lbl"));
		}

		w = go_gtk_builder_get_widget (gui, "circular-rotation-spinbutton");
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), axis->circular_rotation);
		g_signal_connect_object (G_OBJECT (w), "value-changed",
					 G_CALLBACK (cb_rotation_changed),
					 axis, 0);
	} else
		gtk_widget_hide (go_gtk_builder_get_widget (gui, "circular-grid"));

	for (i = 0; i < G_N_ELEMENTS (toggle_props) ; i++) {
		gboolean cur_val;
		GtkWidget *w = go_gtk_builder_get_widget (gui, toggle_props[i]);

		g_object_get (G_OBJECT (gobj), toggle_props[i], &cur_val, NULL);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), cur_val);
		g_signal_connect_object (G_OBJECT (w),
					 "toggled",
					 G_CALLBACK (cb_axis_toggle_changed), axis, 0);
	}

	go_editor_add_page (editor,
			     go_gtk_builder_get_widget (gui, "axis-pref-grid"),
			     _("Scale"));

	/* effective area */
	/* we actually only support with some axes (X, Y, and radial) in 2D charts */
	if ((axis->type == GOG_AXIS_X || axis->type == GOG_AXIS_Y || axis->type == GOG_AXIS_RADIAL) &&
	    (GOG_IS_CHART (gog_object_get_parent (gobj))? !gog_chart_is_3d (GOG_CHART (gog_object_get_parent (gobj))): TRUE)) {
		GtkAdjustment *adj;
		double start = axis->span_start * 100., end = axis->span_end * 100.;
		w = go_gtk_builder_get_widget (gui, "start-btn");
		adj = GTK_ADJUSTMENT (gtk_builder_get_object (gui, "end-adj"));
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), start);
		gtk_adjustment_set_lower (adj, fmin (start + 1., end));
		g_signal_connect (w, "value_changed", G_CALLBACK (cb_start_changed), axis);
		g_object_set_data (G_OBJECT (w), "other-adj", adj);
		w = go_gtk_builder_get_widget (gui, "end-btn");
		adj = GTK_ADJUSTMENT (gtk_builder_get_object (gui, "start-adj"));
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), end);
		gtk_adjustment_set_upper (adj, fmax (end - 1., start));
		g_signal_connect (w, "value_changed", G_CALLBACK (cb_end_changed), axis);
		g_object_set_data (G_OBJECT (w), "other-adj", adj);
		go_editor_add_page (editor,
		                    go_gtk_builder_get_widget (gui, "area-grid"),
		                    _("Span"));
	}

	/* color map */
	if (axis->type == GOG_AXIS_COLOR || axis->type == GOG_AXIS_PSEUDO_3D) {
		GtkListStore *model = GTK_LIST_STORE (gtk_builder_get_object (gui, "color-map-list"));
		GtkTreeIter iter;
		GdkPixbuf *pixbuf;
		GogAxisColorMap const *map;
		GogTheme *theme = gog_graph_get_theme (gog_object_get_graph (GOG_OBJECT (axis)));
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
		GtkWidget *combo = go_gtk_builder_get_widget (gui, "color-map-combo");
		struct ColorMapState color_state;
		state->color_map_combo = GTK_COMBO_BOX (combo);
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
				                        "text", 0, NULL);
		renderer = gtk_cell_renderer_pixbuf_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
				                        "pixbuf", 1, NULL);

		if (axis->type == GOG_AXIS_PSEUDO_3D) {
			gtk_list_store_append (model, &iter);
			map = gog_theme_get_color_map (theme, TRUE);
			pixbuf = gog_axis_color_map_get_snapshot (map, TRUE,
				                                      TRUE, 200, 16);
			gtk_list_store_set (model, &iter, 0, gog_axis_color_map_get_name (map),
				                1, pixbuf, 2, map, -1);
			g_object_unref (pixbuf);
			if (map == axis->color_map)
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
		}
		gtk_list_store_append (model, &iter);
		map = gog_theme_get_color_map (theme, FALSE);
		pixbuf = gog_axis_color_map_get_snapshot (map, FALSE,
		                                          TRUE, 200, 16);
		gtk_list_store_set (model, &iter, 0, gog_axis_color_map_get_name (map),
		                    1, pixbuf, 2, map, -1);
		g_object_unref (pixbuf);
		if (map == axis->color_map)
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
		/* add all locally defined maps */
		color_state.combo = GTK_COMBO_BOX (combo);
		color_state.model = model;
		color_state.target = axis->color_map;
		color_state.discrete = axis->type == GOG_AXIS_PSEUDO_3D;
		gog_axis_color_map_foreach ((GogAxisColorMapHandler) add_color_map_cb, &color_state);
		g_signal_connect (combo, "changed", G_CALLBACK (color_map_changed_cb), state);
		w = go_gtk_builder_get_widget (gui, "new-btn");
		g_signal_connect_swapped (w, "clicked", G_CALLBACK (color_map_new_cb), state);
		w = go_gtk_builder_get_widget (gui, "duplicate-btn");
		g_signal_connect_swapped (w, "clicked", G_CALLBACK (color_map_dup_cb), state);
		w = go_gtk_builder_get_widget (gui, "save-btn");
		g_signal_connect_swapped (w, "clicked", G_CALLBACK (color_map_save_cb), state);
		go_editor_add_page (editor,
		                    go_gtk_builder_get_widget (gui, "color-map-grid"),
		                    _("Colors"));
	}
	/* Metrics */
	if ((axis->type == GOG_AXIS_X || axis->type == GOG_AXIS_Y ||
	    axis->type == GOG_AXIS_Z || axis->type == GOG_AXIS_RADIAL)
	    && (gog_chart_is_3d (GOG_CHART (gog_object_get_parent ((GogObject *) axis)))
	        && axis->refering_axes == NULL)) {
		/* only 3d for now */
		GogChart *parent = GOG_CHART (gog_object_get_parent ((GogObject *) axis));
		GogAxis *axis_;
		GtkGrid *grid;
		GtkWidget *combo;
		GSList *l, *ptr;
		GtkListStore *store;
		GtkCellRenderer *cell;
		GtkTreeIter iter;
		GtkAdjustment *adj;
		GoUnit const *unit;
		struct MetricsState *state = g_new (struct MetricsState, 1);
		state->axis = axis;
		w = gtk_grid_new ();
		g_object_set (w, "border-width", 12, "column-spacing", 12, "row-spacing", 6, NULL);
		gtk_widget_show (w);
		grid = GTK_GRID (w);
		store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_UINT);
		combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
		cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
						                "text", 0, NULL);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, _("Default"), 1, GOG_AXIS_METRICS_DEFAULT, -1);
		if (axis->metrics == GOG_AXIS_METRICS_DEFAULT)
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
		if (!gog_chart_is_3d ((GogChart *) parent)) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, _("Absolute"), 1, GOG_AXIS_METRICS_ABSOLUTE, -1);
			if (axis->metrics == GOG_AXIS_METRICS_ABSOLUTE)
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
		}
		if (axis->refering_axes == NULL) {
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, _("Relative length"), 1, GOG_AXIS_METRICS_RELATIVE, -1);
			if (axis->metrics == GOG_AXIS_METRICS_RELATIVE)
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, _("Relative ticks distance"), 1, GOG_AXIS_METRICS_RELATIVE_TICKS, -1);
			if (axis->metrics == GOG_AXIS_METRICS_RELATIVE_TICKS)
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
		}
		gtk_grid_attach (grid, combo, 0, 0, 3, 1);
		gtk_widget_show ((GtkWidget *) combo);
		state->axes = gtk_combo_box_new ();
		store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_combo_box_set_model (GTK_COMBO_BOX (state->axes), GTK_TREE_MODEL (store));
		cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (state->axes), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (state->axes), cell,
						                "text", 0, NULL);
		l = gog_object_get_children ((GogObject *) parent, NULL);
		for (ptr = l; ptr != NULL; ptr = ptr->next) {
			if (!GOG_IS_AXIS (ptr->data))
				continue;
			axis_ = ptr->data;
			if (axis_->metrics > GOG_AXIS_METRICS_ABSOLUTE || axis_ == axis)
				continue;
			gtk_list_store_append (store, &iter);
			gtk_list_store_set (store, &iter, 0, gog_object_get_name (ptr->data), 1, axis_, -1);
			if (axis_ == axis->ref_axis)
				gtk_combo_box_set_active_iter (GTK_COMBO_BOX (state->axes), &iter);
		}
		/* now add a spin button for the metrix_ratio */
		gtk_grid_attach (grid, state->axes, 0, 1, 3, 1);
		g_signal_connect (state->axes, "changed", G_CALLBACK (metrics_axis_changed_cb), state);
		if (axis->metrics > GOG_AXIS_METRICS_ABSOLUTE)
			gtk_widget_show (state->axes);
		state->label = gtk_label_new (NULL); /* the text will be set later */
		gtk_grid_attach (grid, state->label, 0, 2, 1, 1);
		g_signal_connect (combo, "changed", G_CALLBACK (metrics_changed_cb), state);
		/* now add a spin button for the metrix_ratio */
		adj = gtk_adjustment_new (1., 0.01, 100., .1, 1., 1.);
		state->ratio = gtk_spin_button_new (adj, .1, 2);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (state->ratio), axis->metrics_ratio);
		g_signal_connect (state->ratio, "value-changed", G_CALLBACK (metrics_ratio_changed_cb), state);
		gtk_grid_attach (grid, state->ratio, 1, 2, 1, 1);
		state->unit = gtk_combo_box_new ();
		store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_combo_box_set_model (GTK_COMBO_BOX (state->unit), GTK_TREE_MODEL (store));
		cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (state->unit), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (state->unit), cell,
						                "text", 0, NULL);
		unit = go_unit_get (GO_UNIT_CENTIMETER);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, go_unit_get_symbol (unit), 1, unit, -1);
		if (axis->unit == GO_UNIT_CENTIMETER)
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (state->unit), &iter);
		unit = go_unit_get (GO_UNIT_INCH);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, go_unit_get_symbol (unit), 1, unit, -1);
		if (axis->unit == GO_UNIT_INCH)
			gtk_combo_box_set_active_iter (GTK_COMBO_BOX (state->unit), &iter);
		gtk_grid_attach (grid, state->unit, 2, 2, 1, 1);
		g_signal_connect (state->unit, "changed", G_CALLBACK (metrics_unit_changed_cb), state);
		if (axis->metrics >= GOG_AXIS_METRICS_ABSOLUTE) {
			gtk_widget_show (state->label);
			gtk_widget_show (state->ratio);
			if (axis->metrics == GOG_AXIS_METRICS_ABSOLUTE)
				gtk_widget_show (state->unit);
		}
		g_object_set_data_full ((GObject *) w, "state", state, g_free);
		go_editor_add_page (editor, w, _("Metrics"));
	}

	if (gog_object_is_visible (axis) && gog_axis_get_atype (axis) < GOG_AXIS_VIRTUAL) {
	    /* Style page */
	    (GOG_OBJECT_CLASS(parent_klass)->populate_editor) (gobj, editor, dalloc, cc);

	    /* Format page */
	    {
		    GOFormat *fmt = gog_axis_get_effective_format (axis);
		    w = go_format_sel_new_full (TRUE);
		    state->format_selector = w;

		    if (fmt)
			    go_format_sel_set_style_format (GO_FORMAT_SEL (w),
							    fmt);
			gtk_widget_show (w);

		    go_editor_add_page (editor, w, _("Format"));

		    g_signal_connect (G_OBJECT (w),
			    "format_changed", G_CALLBACK (cb_axis_fmt_changed), axis);
	    }
	}

	g_signal_connect_swapped (gtk_builder_get_object (gui, "axis-pref-grid"),
	                          "destroy", G_CALLBACK (g_object_unref), gui);
	g_object_set_data_full (gtk_builder_get_object (gui, "axis-pref-grid"),
				"state", state, (GDestroyNotify) gog_axis_pref_state_free);

	go_editor_set_store_page (editor, &axis_pref_page);
}
#endif

static void
gog_axis_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogAxis *axis = GOG_AXIS (gso);
	GogAxisType type = gog_axis_get_atype (axis);
	GogTheme *theme = gog_object_get_theme (GOG_OBJECT (gso));
	if (type != GOG_AXIS_PSEUDO_3D && type != GOG_AXIS_COLOR)
		style->interesting_fields = GO_STYLE_LINE | GO_STYLE_FONT |
			GO_STYLE_TEXT_LAYOUT;
	else {
		style->interesting_fields = 0;
		if (axis->auto_color_map)
			axis->color_map = gog_theme_get_color_map (theme, type == GOG_AXIS_PSEUDO_3D);
	}
	gog_theme_fillin_style (theme, style, GOG_OBJECT (gso), 0, GO_STYLE_LINE |
	                        GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT);
}

static void
gog_axis_class_init (GObjectClass *gobject_klass)
{
	static GogObjectRole const roles[] = {
		{ N_("AxisLine"), "GogAxisLine", 2,
		  GOG_POSITION_PADDING, GOG_POSITION_PADDING, GOG_OBJECT_NAME_BY_ROLE,
		  role_axis_line_can_add, NULL, NULL, role_axis_line_post_add, NULL, NULL, { -1 } },
	};

	GogObjectClass *gog_klass = (GogObjectClass *) gobject_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;

	parent_klass = g_type_class_peek_parent (gobject_klass);
	gobject_klass->set_property = gog_axis_set_property;
	gobject_klass->get_property = gog_axis_get_property;
	gobject_klass->finalize	    = gog_axis_finalize;

	/* no need to persist, the role handles that */
	g_object_class_install_property (gobject_klass, AXIS_PROP_TYPE,
		g_param_spec_int ("type", _("Type"),
			_("Numerical type of this axis"),
			GOG_AXIS_UNKNOWN, GOG_AXIS_TYPES, GOG_AXIS_UNKNOWN,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, AXIS_PROP_INVERT,
		g_param_spec_boolean ("invert-axis", _("Invert axis"),
			_("Scale from high to low rather than low to high"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_MAP,
		g_param_spec_string ("map-name", _("MapName"),
			_("The name of the map for scaling"),
			"Linear",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_ASSIGNED_FORMAT_STR_XL,
		g_param_spec_string ("assigned-format-string-XL",
			_("Assigned XL format"),
			_("The user assigned format to use for non-discrete axis labels (XL format)"),
			"General",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_CIRCULAR_ROTATION,
		g_param_spec_double ("circular-rotation",
			_("Rotation of circular axis"),
			_("Rotation of circular axis"),
			GOG_AXIS_CIRCULAR_ROTATION_MIN,
			GOG_AXIS_CIRCULAR_ROTATION_MAX,
			0.0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_POLAR_UNIT,
		g_param_spec_string ("polar-unit",
			_("Polar axis set unit"),
			_("Polar axis set unit"),
			polar_units[GOG_AXIS_POLAR_UNIT_DEGREES].name,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_SPAN_START,
		g_param_spec_double ("span-start",
			_("Axis start position"),
			_("Position of the plot area at which the axis effective area starts, expressed as a percentage of the available position. Defaults to 0.0"),
			0., 1., 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_SPAN_END,
		g_param_spec_double ("span-end",
			_("Axis end position"),
			_("Position of the plot area at which the axis effective area ends, expressed as a percentage of the available position. Defaults to 1.0"),
			0., 1., 1.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_COLOR_MAP,
		g_param_spec_string ("color-map-name", _("ColorMapName"),
			_("The name of the color map"),
			"default",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_METRICS,
		g_param_spec_string ("metrics", _("Metrics"),
			_("The way the axis ticks distance is evaluated"),
			"default",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_REF_AXIS,
		g_param_spec_string ("axis-ref", _("AxisRef"),
			_("The name of the axis used as reference for ticks distance"),
			"none",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_METRICS_RATIO,
		g_param_spec_double ("metrics-ratio",
			_("Metrics ratio"),
			_("If an axis is used as reference, gives the ratio of the ticks distance, and if the metrix is absolute, the ticks distance. Defaults to 1.0"),
		    0.01, 100., 1.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_REF_AXIS,
		g_param_spec_string ("metrics-unit", _("Metrics Unit"),
			_("The unit symbol for the absolute distance unit between ticks. Might be \"cm\" or \"in\""),
			"cm",
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, AXIS_PROP_DISPLAY_FACTOR,
		g_param_spec_double ("display-factor",
			_("Display factor"),
			_("Real values are the displayed ones multipled by the display factor."),
		    G_MINDOUBLE, G_MAXDOUBLE, 1.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_object_register_roles (gog_klass, roles, G_N_ELEMENTS (roles));

	gog_klass->update		= gog_axis_update;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor	= gog_axis_populate_editor;
#endif
	gog_klass->view_type	= gog_axis_view_get_type ();
	style_klass->init_style = gog_axis_init_style;
}

static void
gog_axis_init (GogAxis *axis)
{
	axis->type	 = GOG_AXIS_UNKNOWN;
	axis->contributors = NULL;
	axis->inverted = FALSE;

	/* yes we want min = MAX */
	axis->min_val =  DBL_MAX;
	axis->max_val = -DBL_MAX;
	axis->min_contrib = axis->max_contrib = NULL;
	axis->is_discrete = FALSE;
	axis->center_on_ticks = TRUE;
	axis->labels = NULL;
	axis->plot_that_supplied_labels = NULL;
	axis->format = axis->assigned_format = NULL;

	gog_axis_map_set (axis, NULL);

	axis->polar_unit = GOG_AXIS_POLAR_UNIT_DEGREES;
	axis->circular_rotation = 0;

	axis->ticks = NULL;
	axis->tick_nbr = 0;
	axis->span_start = 0.;
	axis->span_end = 1.;
	axis->auto_color_map = TRUE;
	axis->metrics_ratio = 1.;
	axis->unit = GO_UNIT_CENTIMETER;
	axis->display_factor = 1.;
}

static void
gog_axis_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = GOG_AXIS_ELEM_MIN;
	*last  = GOG_AXIS_ELEM_CROSS_POINT;
}

static GogDatasetElement *
gog_axis_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogAxis *axis = GOG_AXIS (set);
	if (GOG_AXIS_ELEM_MIN <= dim_i && dim_i < GOG_AXIS_ELEM_CROSS_POINT)
		return &axis->source[dim_i];
	if (dim_i == GOG_AXIS_ELEM_CROSS_POINT) {
		return &(axis->base.cross_location);
	}
	return NULL;
}

static void
gog_axis_dim_changed (GogDataset *set, int dim_i)
{
	gog_axis_update (GOG_OBJECT (set));
	gog_object_emit_changed (GOG_OBJECT (set), TRUE);
}

static void
gog_axis_dataset_init (GogDatasetClass *iface)
{
	iface->dims	   = gog_axis_dataset_dims;
	iface->get_elem	   = gog_axis_dataset_get_elem;
	iface->dim_changed = gog_axis_dim_changed;
}

static void
gog_axis_persist_init (GOPersistClass *iface)
{
	iface->sax_save = gog_axis_sax_save;
	iface->prep_sax = gog_axis_prep_sax;
}

GSF_CLASS_FULL (GogAxis, gog_axis,
		NULL, NULL, gog_axis_class_init, NULL,
		gog_axis_init, GOG_TYPE_AXIS_BASE, 0,
		GSF_INTERFACE (gog_axis_dataset_init, GOG_TYPE_DATASET) \
		GSF_INTERFACE (gog_axis_persist_init, GO_TYPE_PERSIST))


/**
 * gog_axis_is_center_on_ticks:
 * @axis: #GogAxis
 *
 * Returns: %TRUE if labels are centered on ticks when @axis is discrete
 **/
gboolean
gog_axis_is_center_on_ticks (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), FALSE);
	return axis->center_on_ticks;
}

/**
 * gog_axis_is_discrete:
 * @axis: #GogAxis
 *
 * Returns: %TRUE if @axis enumerates a set of discrete items, rather than a
 * 	continuous value
 **/
gboolean
gog_axis_is_discrete (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), FALSE);
	return axis->is_discrete;
}

/**
 * gog_axis_is_inverted:
 * @axis: #GogAxis
 *
 * Returns: %TRUE if @axis is inverted.
 **/
gboolean
gog_axis_is_inverted (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), FALSE);
	return axis->inverted;
}

/**
 * gog_axis_get_bounds:
 * @axis: #GogAxis
 * @minima: (out): storage for result
 * @maxima: (out): storage for result
 *
 * Returns: %TRUE if the bounds stored in @minima and @maxima are sane
 **/
gboolean
gog_axis_get_bounds (GogAxis const *axis, double *minima, double *maxima)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), FALSE);
	g_return_val_if_fail (minima != NULL, FALSE);
	g_return_val_if_fail (maxima != NULL, FALSE);

	*minima = gog_axis_get_entry (axis, GOG_AXIS_ELEM_MIN, NULL);
	*maxima = gog_axis_get_entry (axis, GOG_AXIS_ELEM_MAX, NULL);

	return go_finite (*minima) && go_finite (*maxima) && *minima < *maxima;
}

/**
 * gog_axis_set_bounds:
 * @axis: #GogAxis
 * @minimum: axis low bound
 * @maximum: axis high bound
 *
 * Sets axis bounds. If minimum or maximum are not finite values, corresponding
 * bound remains unchanged.
 **/
void
gog_axis_set_bounds (GogAxis *axis, double minimum, double maximum)
{
	g_return_if_fail (GOG_IS_AXIS (axis));

	/*
	 * ???????
	 * This sets a new dim instead of using the one embedded in the
	 * axis.  Is that really right?
	 * --MW 20090515
	 */

	if (go_finite (minimum)) {
		GOData *data = GO_DATA (go_data_scalar_val_new (minimum));
		gog_dataset_set_dim (GOG_DATASET (axis), GOG_AXIS_ELEM_MIN,
				     data, NULL);
	}
	if (go_finite (maximum)) {
		GOData *data = GO_DATA (go_data_scalar_val_new (maximum));
		gog_dataset_set_dim (GOG_DATASET (axis), GOG_AXIS_ELEM_MAX,
				     data, NULL);
	}
}

void
gog_axis_get_effective_span (GogAxis const *axis, double *start, double *end)
{
	g_return_if_fail (GOG_IS_AXIS (axis));

	*start = axis->span_start;
	*end = axis->span_end;
}

/**
 * gog_axis_set_extents:
 * @axis: #GogAxis
 * @start: axis start bound
 * @stop: axis stop bound
 *
 * Set axis exents. It's a convenience function that sets axis bounds taking
 * into account invert flag.
 **/
void
gog_axis_set_extents (GogAxis *axis, double start, double stop)
{
	g_return_if_fail (GOG_IS_AXIS (axis));

	if (axis->inverted)
		gog_axis_set_bounds (axis, stop, start);
	else
		gog_axis_set_bounds (axis, start, stop);
}


/**
 * gog_axis_get_ticks:
 * @axis: #GogAxis
 * @ticks: an array of #GogAxisTick
 *
 * An accessor to @axis->ticks.
 *
 * return value: number of ticks
 **/
unsigned
gog_axis_get_ticks (GogAxis *axis, GogAxisTick **ticks)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), 0);
	g_return_val_if_fail (ticks != NULL, 0);

	*ticks = axis->ticks;
	return axis->tick_nbr;
}

/**
 * gog_axis_get_labels:
 * @axis: a #GogAxis
 * @plot_that_labeled_axis: a #GogPlot
 *
 * Returns: (transfer none): the possibly NULL #GOData used as a label for this axis
 * along with the plot that it was associated with
 **/
GOData *
gog_axis_get_labels (GogAxis const *axis, GogPlot **plot_that_labeled_axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), NULL);

	if (axis->is_discrete) {
		if (plot_that_labeled_axis != NULL)
			*plot_that_labeled_axis = axis->plot_that_supplied_labels;
		return GO_DATA (axis->labels);
	}
	if (plot_that_labeled_axis != NULL)
		*plot_that_labeled_axis = NULL;
	return NULL;
}

/**
 * gog_axis_add_contributor:
 * @axis: #GogAxis
 * @contrib: #GogObject (can we relax this to use an interface ?)
 *
 * Register @contrib as taking part in the negotiation of @axis's bounds.
 **/
void
gog_axis_add_contributor (GogAxis *axis, GogObject *contrib)
{
	g_return_if_fail (GOG_IS_AXIS (axis));
	g_return_if_fail (g_slist_find (axis->contributors, contrib) == NULL);

	axis->contributors = g_slist_prepend (axis->contributors, contrib);

	gog_object_request_update (GOG_OBJECT (axis));
}

/**
 * gog_axis_del_contributor:
 * @axis: #GogAxis
 * @contrib: #GogObject (can we relax this to use an interface ?)
 *
 * @contrib no longer takes part in the negotiation of @axis's bounds.
 **/
void
gog_axis_del_contributor (GogAxis *axis, GogObject *contrib)
{
	gboolean update = FALSE;

	g_return_if_fail (GOG_IS_AXIS (axis));
	g_return_if_fail (g_slist_find (axis->contributors, contrib) != NULL);

	if (axis->min_contrib == contrib) {
		axis->min_contrib = NULL;
		update = TRUE;
	}
	if (axis->max_contrib == contrib) {
		axis->max_contrib = NULL;
		update = TRUE;
	}
	axis->contributors = g_slist_remove (axis->contributors, contrib);

	if (update)
		gog_object_request_update (GOG_OBJECT (axis));
}

void
gog_axis_clear_contributors (GogAxis *axis)
{
	GSList *ptr, *list;
	GogAxisSet filter;

	g_return_if_fail (GOG_IS_AXIS (axis));

	filter = 1 << axis->type;
	list = g_slist_copy (axis->contributors);
	for (ptr = list; ptr != NULL ; ptr = ptr->next)
		gog_plot_axis_clear (GOG_PLOT (ptr->data), filter);
	g_slist_free (list);
}

/**
 * gog_axis_contributors:
 * @axis: #GogAxis
 *
 * Returns: (element-type GogObject) (transfer none): the list of the axis
 * contributors
 **/
GSList const *
gog_axis_contributors (GogAxis *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), NULL);

	return axis->contributors;
}

/**
 * gog_axis_bound_changed:
 * @axis: #GogAxis
 * @contrib: #GogObject
**/
void
gog_axis_bound_changed (GogAxis *axis, GogObject *contrib)
{
	g_return_if_fail (GOG_IS_AXIS (axis));

	gog_object_request_update (GOG_OBJECT (axis));
}

/*
 * gog_axis_data_get_bounds:
 * @axis: (allow-none): the axis for which the data applies
 * @data: the data to get bounds for
 * @minimum: smallest valid data value.
 * @maximum: largest valid data value.
 */
void
gog_axis_data_get_bounds (GogAxis *axis, GOData *data,
			  double *minimum, double *maximum)
{
	gboolean (*map_finite) (double value) =
		axis ? axis->actual_map_desc->map_finite : go_finite;

	if (map_finite != go_finite) {
		size_t i, n = go_data_get_n_values (data);
		const double *values = go_data_get_values (data);
		*minimum = go_pinf;
		*maximum = go_ninf;
		for (i = 0; i < n; i++) {
			double x = values[i];
			if (!map_finite (x))
				continue;
			*minimum = MIN (*minimum, x);
			*maximum = MAX (*maximum, x);
		}
	} else
		go_data_get_bounds (data, minimum, maximum);
}

/*
 * gog_axis_is_zero_important:
 * @axis: the axis to check
 *
 * Returns %TRUE if the axis is of a type in which zero is important and
 * preferentially should be included in the range.
 *
 * This generally means a linear axis, i.e., not a log axis and not a
 * date formatted axis.
 */
gboolean
gog_axis_is_zero_important (GogAxis *axis)
{
	GogAxisMapDesc const *desc = axis->actual_map_desc;

	return !axis->is_discrete &&
		desc->map_finite (0.0) &&
		desc->auto_bound != map_date_auto_bound;
}

/**
 * gog_axis_get_grid_line:
 * @axis: #GogAxis
 * @major: whether to retrieve major or minor grid line.
 *
 * Returns: (transfer none): a pointer to GridLine object associated to given axis, NULL
 * if it doesn't exists.
 **/
GogGridLine *
gog_axis_get_grid_line (GogAxis *axis, gboolean major)
{
	GogGridLine *grid_line;
	GSList *children;

	children = gog_object_get_children (GOG_OBJECT (axis),
		gog_object_find_role_by_name (GOG_OBJECT (axis),
			major ? "MajorGrid" : "MinorGrid"));
	if (children != NULL) {
		grid_line = GOG_GRID_LINE (children->data);
		g_slist_free (children);
		return grid_line;
	}
	return NULL;
}

/**
 * gog_axis_set_polar_unit:
 * @axis: a #GogAxis
 * @unit: #GogAxisPolarUnit
 *
 * Sets unit of a circular axis. See #GogAxisPolarUnit for valid
 * values.
 **/

void
gog_axis_set_polar_unit (GogAxis *axis, GogAxisPolarUnit unit)
{
	g_return_if_fail (GOG_IS_AXIS (axis));

	axis->polar_unit = CLAMP (unit, 0, GOG_AXIS_POLAR_UNIT_MAX - 1);
}

/**
 * gog_axis_get_polar_unit:
 * @axis: a #GogAxis
 *
 * Returns: unit of @axis if it's a circular axis of a polar
 * 	axis set, -1 otherwise.
 **/

GogAxisPolarUnit
gog_axis_get_polar_unit (GogAxis *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), 0);

	return axis->polar_unit;
}

/**
 * gog_axis_get_circular_perimeter:
 * @axis: a #GogAxis
 *
 * Returns: perimeter of a circular #GogAxis of a polar axis set.
 * 	radians: 2*pi
 * 	degrees: 360.0
 * 	grads  : 400.0
 **/
double
gog_axis_get_polar_perimeter (GogAxis *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), 0.0);

	return polar_units[axis->polar_unit].perimeter;
}

/**
 * gog_axis_get_circular_rotation:
 * @axis: a #GogAxis
 *
 * Returns: rotation of a circular #GogAxis.
 **/
double
gog_axis_get_circular_rotation (GogAxis *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), 0.0);

	return axis->circular_rotation;
}

/**
 * gog_axis_get_color_map:
 * @axis: a #GogAxis
 *
 * Retrieves the #GogAxisColorMap associated to the axis or %NULL.
 * Returns: (transfer none): the color map used by the axis if any.
 **/
GogAxisColorMap const *
gog_axis_get_color_map (GogAxis *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), NULL);

	return axis->color_map;
}


/**
 * gog_axis_get_color_scale:
 * @axis: a #GogAxis
 *
 * Retrieves the #GogColorScale associated to the axis or %NULL.
 * Returns: (transfer none): the color scale used to display the axis colors
 * for color and pseudo-3d axes.
 **/
GogColorScale *
gog_axis_get_color_scale (GogAxis *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), NULL);

	return axis->color_scale;
}

void
_gog_axis_set_color_scale (GogAxis *axis, GogColorScale *scale)
{
	g_return_if_fail (GOG_IS_AXIS (axis) &&
	                  (axis->type == GOG_AXIS_COLOR || axis->type == GOG_AXIS_PSEUDO_3D) &&
	                  (axis->color_scale == NULL || scale == NULL));
	axis->color_scale = scale;
}

GogAxisMetrics
gog_axis_get_metrics (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), GOG_AXIS_METRICS_INVALID);
	return axis->metrics;
}

/**
 * gog_axis_get_ref_axis:
 * @axis: a #GogAxis
 *
 * Since: 0.10.17
 * Returns: (transfer none): the axis used to evaluate the length of @axis or
 * NULL.
 **/
GogAxis *
gog_axis_get_ref_axis (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis) && axis->metrics > GOG_AXIS_METRICS_ABSOLUTE, NULL);
	return axis->ref_axis;
}

double
gog_axis_get_major_ticks_distance (GogAxis const *axis)
{
	g_return_val_if_fail (GOG_IS_AXIS (axis), go_nan);
	return gog_axis_get_entry (axis, GOG_AXIS_ELEM_MAJOR_TICK, NULL);
}

/****************************************************************************/

typedef struct {
	GogAxisBaseView base;
	double padding_low, padding_high;
} GogAxisView;
typedef GogAxisBaseViewClass	GogAxisViewClass;

#define GOG_TYPE_AXIS_VIEW	(gog_axis_view_get_type ())
#define GOG_AXIS_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_AXIS_VIEW, GogAxisView))
#define GOG_IS_AXIS_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_AXIS_VIEW))

static GogViewClass *aview_parent_klass;

static void
gog_axis_view_padding_request_3d (GogView *view, GogView *child,
                                  GogViewAllocation const *plot_area,
				  GogViewPadding *label_padding)
{
	GogViewAllocation child_bbox;
	GogViewAllocation label_pos;
	GogViewAllocation tmp = *plot_area;
	GogViewRequisition req, available;
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (child->model));
	double angle;

	gog_axis_base_view_label_position_request (view, plot_area, &label_pos);
	if (style->text_layout.auto_angle) {
		angle = atan2 (label_pos.w, label_pos.h) * 180. / M_PI;
		if (angle < 0.)
			angle += 180.;
		style->text_layout.angle = (angle > 45. && angle < 135.)? 90. : 0.;
	}

	available.w = plot_area->w;
	available.h = plot_area->h;

	gog_view_size_request (child, &available, &req);

	if (req.w == 0 || req.h == 0)
		return;

	child_bbox.x = label_pos.x + label_pos.w;
	if (label_pos.w < 0)
		child_bbox.x -= req.w;
	child_bbox.y = label_pos.y + label_pos.h;
	if (label_pos.h < 0)
		child_bbox.y -= req.h;

	child_bbox.w = req.w;
	child_bbox.h = req.h;

	tmp.x -= label_padding->wl;
	tmp.w += label_padding->wl + label_padding->wr;
	tmp.y -= label_padding->hb;
	tmp.h += label_padding->hb + label_padding->ht;

	label_padding->wl += MAX (0, tmp.x - child_bbox.x);
	label_padding->ht += MAX (0, tmp.y - child_bbox.y);
	label_padding->wr += MAX (0, child_bbox.x + child_bbox.w
				  - tmp.x - tmp.w);
	label_padding->hb += MAX (0, child_bbox.y + child_bbox.h
				  - tmp.y - tmp.h);
}

static void
gog_axis_view_padding_request (GogView *view,
			       GogViewAllocation const *bbox,
			       GogViewPadding *padding)
{
	GogView *child;
	GogAxis *axis = GOG_AXIS (view->model);
	GogAxisView *axis_view = GOG_AXIS_VIEW (view);
	GogAxisType type = gog_axis_get_atype (axis);
	GogObjectPosition pos;
	GogAxisPosition axis_pos, base_axis_pos;
	GogViewAllocation tmp = *bbox;
	GogViewRequisition req, available;
	GogViewPadding label_padding, child_padding;
	GogObject *parent = gog_object_get_parent (view->model);
	gboolean is_3d = GOG_IS_CHART (parent) && gog_chart_is_3d (GOG_CHART (parent));
	GSList *ptr, *saved_ptr = NULL;
	double const pad_h = gog_renderer_pt2r_y (view->renderer, PAD_HACK);
	double const pad_w = gog_renderer_pt2r_x (view->renderer, PAD_HACK);

	label_padding.wr = label_padding.wl = label_padding.ht = label_padding.hb = 0;

	base_axis_pos = axis_pos = gog_axis_base_get_clamped_position (GOG_AXIS_BASE (axis));

	ptr = view->children;
	while (ptr) {
		child = ptr->data;
		pos = child->model->position;
		if (GOG_IS_LABEL (child->model) && !(pos & GOG_POSITION_MANUAL)) {
			if (is_3d) {
				gog_axis_view_padding_request_3d (view, child,
					bbox, &label_padding);
			} else {
				available.w = bbox->w;
				available.h = bbox->h;
				gog_view_size_request (child, &available, &req);
				if (type == GOG_AXIS_X)
					switch (axis_pos) {
						case GOG_AXIS_AT_HIGH:
							label_padding.ht += req.h + pad_h;
							break;
						case GOG_AXIS_AT_LOW:
						default:
							label_padding.hb += req.h + pad_h;
							break;
					}
				else
					switch (axis_pos) {
						case GOG_AXIS_AT_HIGH:
							label_padding.wr += req.w + pad_w;
							break;
						case GOG_AXIS_AT_LOW:
						default:
							label_padding.wl += req.w + pad_w;
							break;
					}
			}
		}
		if (GOG_IS_AXIS_LINE (child->model) && saved_ptr == NULL && child->children != NULL) {
			axis_pos = gog_axis_base_get_clamped_position (GOG_AXIS_BASE (child->model));
			saved_ptr = ptr;
			ptr = child->children;
		} else if (saved_ptr != NULL && ptr->next == NULL) {
			axis_pos = base_axis_pos;
			ptr = saved_ptr->next;
			saved_ptr = NULL;
		} else
			ptr = ptr->next;
	}

	if (is_3d) {
		/* For 3d chart we have to calculate how much more padding
		 * is needed for the axis itself */
		tmp.x -= label_padding.wl;
		tmp.w += label_padding.wl + label_padding.wr;
		tmp.y -= label_padding.hb;
		tmp.h += label_padding.hb + label_padding.ht;
	} else {
		tmp.x += label_padding.wl;
		tmp.w -= label_padding.wl + label_padding.wr;
		tmp.y += label_padding.hb;
		tmp.h -= label_padding.hb + label_padding.ht;
	}

	(aview_parent_klass->padding_request) (view, &tmp, padding);

	for (ptr = view->children; ptr != NULL ; ptr = ptr->next) {
		child = ptr->data;
		if (GOG_POSITION_IS_PADDING (child->model->position)) {
			gog_view_padding_request (child, &tmp, &child_padding);
			padding->wr = MAX (padding->wr, child_padding.wr);
			padding->wl = MAX (padding->wl, child_padding.wl);
			padding->hb = MAX (padding->hb, child_padding.hb);
			padding->ht = MAX (padding->ht, child_padding.ht);
		}
	}

	padding->wr += label_padding.wr;
	padding->wl += label_padding.wl;
	padding->ht += label_padding.ht;
	padding->hb += label_padding.hb;
	if (!is_3d) {
		if (type == GOG_AXIS_X) {
			axis_view->padding_high = padding->ht;
			axis_view->padding_low = padding->hb;
		} else {
			axis_view->padding_high = padding->wr;
			axis_view->padding_low = padding->wl;
		}
	}
}

static void
gog_axis_view_size_allocate_3d (GogView *view, GogView *child,
                                GogViewAllocation const *plot_area)
{
	GogViewAllocation child_bbox;
	GogViewAllocation label_pos;
	GogViewRequisition req, available;

	gog_view_size_request (child, &available, &req);
	gog_axis_base_view_label_position_request (view, plot_area, &label_pos);

	child_bbox.x = label_pos.x + label_pos.w;
	if (label_pos.w < 0)
		child_bbox.x -= req.w;
	child_bbox.y = label_pos.y + label_pos.h;
	if (label_pos.h < 0)
		child_bbox.y -= req.h;

	child_bbox.w = req.w;
	child_bbox.h = req.h;

	gog_view_size_allocate (child, &child_bbox);
}

static void
gog_axis_view_size_allocate (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr, *saved_ptr = NULL;
	GogView *child;
	GogAxis *axis = GOG_AXIS (view->model);
	GogAxisView *axis_view = GOG_AXIS_VIEW (view);
	GogAxisType type = gog_axis_get_atype (axis);
	GogViewAllocation tmp = *bbox;
	GogViewAllocation const *plot_area = gog_chart_view_get_plot_area (view->parent);
	GogViewAllocation child_bbox;
	GogViewRequisition req, available;
	GogObjectPosition pos;
	GogAxisPosition axis_pos, base_axis_pos;
	GogChart *chart = GOG_CHART (gog_object_get_parent (view->model));
	double const pad_h = gog_renderer_pt2r_y (view->renderer, PAD_HACK);
	double const pad_w = gog_renderer_pt2r_x (view->renderer, PAD_HACK);
	double start, end;

	if (!gog_chart_is_3d (chart)) {
		double d;
		if (type == GOG_AXIS_X) {
			d = plot_area->y - tmp.y - axis_view->padding_high;
			tmp.y += d;
			tmp.h = plot_area->h + axis_view->padding_low + axis_view->padding_high;
		} else {
			d = plot_area->x - tmp.x - axis_view->padding_low;
			tmp.x += d;
			tmp.w = plot_area->w + axis_view->padding_low + axis_view->padding_high;
		}
	}
	available.w = tmp.w;
	available.h = tmp.h;

	base_axis_pos = axis_pos = gog_axis_base_get_clamped_position (GOG_AXIS_BASE (axis));

	ptr = view->children;
	while (ptr != NULL) {
		child = ptr->data;
		pos = child->model->position;
		if (GOG_IS_LABEL (child->model) && (pos & GOG_POSITION_MANUAL)) {
			gog_view_size_request (child, &available, &req);
			child_bbox = gog_object_get_manual_allocation (gog_view_get_model (child),
								       bbox, &req);
			gog_view_size_allocate (child, &child_bbox);
		} else {
			if (GOG_POSITION_IS_SPECIAL (pos)) {
				if (GOG_IS_LABEL (child->model)) {
					if (gog_chart_is_3d (chart)) {
						gog_axis_view_size_allocate_3d (view,
							child, plot_area);
						ptr = ptr->next;
						continue;
					}
					gog_view_size_request (child, &available, &req);
					gog_axis_get_effective_span (axis, &start, &end);
					if (type == GOG_AXIS_X) {
						child_bbox.x = plot_area->x +
							(plot_area->w * (start + end) - req.w) / 2.0;
						child_bbox.w = req.w;
						child_bbox.h = req.h;
						switch (axis_pos) {
							case GOG_AXIS_AT_HIGH:
								child_bbox.y = tmp.y;
								tmp.y += req.h + pad_h;
								break;
							case GOG_AXIS_AT_LOW:
							default:
								child_bbox.y = tmp.y + tmp.h - req.h;
								break;
						}
						tmp.h -= req.h + pad_h;
					} else {
						child_bbox.y = plot_area->y + plot_area->h -
							(plot_area->h * (start + end) + req.h) / 2.0;
						child_bbox.h = req.h;
						child_bbox.w = req.w;
						switch (axis_pos) {
							case GOG_AXIS_AT_HIGH:
								child_bbox.x = tmp.x + tmp.w - req.w;
								break;
							case GOG_AXIS_AT_LOW:
							default:
								child_bbox.x = tmp.x;
								tmp.x += req.w + pad_w;
								break;
						}
						tmp.w -= req.w + pad_w;
					}
					gog_view_size_allocate (child, &child_bbox);
				} else {
					gog_view_size_allocate (child, plot_area);
				}
			} else if (GOG_IS_AXIS_LINE (child->model))
				child->allocation_valid = TRUE; /* KLUDGE, but needed for axis lines
					otherwise adding a title does not invalidate the chart allocation,
					see #760675, comments 5+ */

		}
		if (GOG_IS_AXIS_LINE (child->model) && saved_ptr == NULL && child->children != NULL) {
			axis_pos = gog_axis_base_get_clamped_position (GOG_AXIS_BASE (child->model));
			saved_ptr = ptr;
			ptr = child->children;
		} else if (saved_ptr != NULL && ptr->next == NULL) {
			axis_pos = base_axis_pos;
			ptr = saved_ptr->next;
			saved_ptr = NULL;
		} else
			ptr = ptr->next;
	}
}

static void
gog_axis_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GSList *ptr, *saved_ptr = NULL;
	GogView *child;

	(aview_parent_klass->render) (view, bbox);

	/* Render every child except grid lines. Those are rendered
	 * before in gog_chart_view since we don't want to render them
	 * over axis. */
	ptr = view->children;
	while (ptr != NULL) {
		child = ptr->data;
		if (!GOG_IS_GRID_LINE (child->model))
			gog_view_render	(ptr->data, bbox);
		if (GOG_IS_AXIS_LINE (child->model) && saved_ptr == NULL && child->children != NULL) {
			saved_ptr = ptr;
			ptr = child->children;
		} else if (saved_ptr != NULL && ptr->next == NULL) {
			ptr = saved_ptr->next;
			saved_ptr = NULL;
		} else
			ptr = ptr->next;
	}
}

static void
gog_axis_view_class_init (GogAxisViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;

	aview_parent_klass = g_type_class_peek_parent (gview_klass);
	view_klass->size_allocate = gog_axis_view_size_allocate;
	view_klass->padding_request = gog_axis_view_padding_request;
	view_klass->render	    = gog_axis_view_render;
}

static GSF_CLASS (GogAxisView, gog_axis_view,
		  gog_axis_view_class_init, NULL,
		  GOG_TYPE_AXIS_BASE_VIEW)
