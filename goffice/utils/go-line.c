/*
 * go-line.c :
 *
 * Copyright (C) 2004-2006 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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

#include <string.h>
#include <glib/gi18n-lib.h>

/**
 * GOLineDashType:
 * @GO_LINE_NONE: No line displayed.
 * @GO_LINE_SOLID: Solid line.
 * @GO_LINE_S_DOT: Line with dot pattern.
 * @GO_LINE_S_DASH_DOT: Line with dash-dot pattern.
 * @GO_LINE_S_DASH_DOT_DOT: Line with dash-dot-dot pattern.
 * @GO_LINE_DASH_DOT_DOT_DOT: Line with dash-dot-dot-dot pattern.
 * @GO_LINE_DOT: Dotted line.
 * @GO_LINE_S_DASH: Line with short dashes.
 * @GO_LINE_DASH: Line with dash pattern
 * @GO_LINE_LONG_DASH: Line with long dashes.
 * @GO_LINE_DASH_DOT: Line with dash-dot pattern.
 * @GO_LINE_DASH_DOT_DOT: Line with dash-dot-dot pattern.
 * @GO_LINE_MAX: Number of line dash types
 **/

/**
 * GOLineInterpolation:
 * @GO_LINE_INTERPOLATION_LINEAR: Linear interpolation.
 * @GO_LINE_INTERPOLATION_SPLINE: Bezier cubic spline interpolation.
 * @GO_LINE_INTERPOLATION_CLOSED_SPLINE: Closed Bezier cubic spline interpolation.
 * @GO_LINE_INTERPOLATION_ODF_SPLINE: ODF compatible Bezier cubic spline interpolation, cyclic if first and last points are identical.
 * @GO_LINE_INTERPOLATION_CUBIC_SPLINE: Cubic spline interpolation with natural limits.
 * @GO_LINE_INTERPOLATION_PARABOLIC_CUBIC_SPLINE: Cubic spline interpolation with parabolic limits.
 * @GO_LINE_INTERPOLATION_CUBIC_CUBIC_SPLINE: Cubic spline interpolation with cubic limits.
 * @GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE: Cubic spline interpolation with fixed derivatives at both ends.
 * @GO_LINE_INTERPOLATION_STEP_START: Steps using first y value.
 * @GO_LINE_INTERPOLATION_STEP_END: Steps using last y value.
 * @GO_LINE_INTERPOLATION_STEP_CENTER_X: Steps centered around each point.
 * @GO_LINE_INTERPOLATION_STEP_CENTER_Y: Steps using mean y value.
 * @GO_LINE_INTERPOLATION_MAX: First invalid value.
 **/

/**
 * GOLineDashSequence:
 * @offset: offset from start.
 * @n_dash: number of values in dash fields
 * @dash: lengths of the dashes segments. See cairo_set_dash() for details.
 **/

/**
 * GOArrowType:
 * @GO_ARROW_NONE: no arrow head.
 * @GO_ARROW_KITE: kite head.
 * @GO_ARROW_OVAL: oval head.
 **/

/**
 * GOArrow:
 * @typ: #GOArrowType.
 * @a: first arrow head size parameter.
 * @b: second arrow head size parameter.
 * @c: third arrow head size parameter.
 **/

static GOLineDashSequence *
go_line_dash_sequence_ref (GOLineDashSequence * sequence)
{
	sequence->ref_count++;
	return sequence;
}

GType
go_line_dash_sequence_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOLineDashSequence",
			 (GBoxedCopyFunc)go_line_dash_sequence_ref,
			 (GBoxedFreeFunc)go_line_dash_sequence_free);
	}
	return t;
}

typedef struct {
	int 		 n_dash;
	double		 length;
	double 		 dash[8];
} GOLineDashDesc;

static GOLineDashDesc const line_short_dot_desc = 		{2, 10,	{ 0, 2 } };
static GOLineDashDesc const line_dot_desc = 			{2, 12,	{ 3, 3 } };
static GOLineDashDesc const line_short_dash_desc =		{2, 9,	{ 6, 3 } };
static GOLineDashDesc const line_short_dash_dot_desc =		{4, 12,	{ 6, 3, 0, 3 } };
static GOLineDashDesc const line_short_dash_dot_dot_desc =    	{6, 15,	{ 6, 3, 0, 3, 0, 3 } };
static GOLineDashDesc const line_dash_dot_dot_dot_desc =    	{8, 21,	{ 9, 3, 0, 3, 0, 3, 0, 3 } };
static GOLineDashDesc const line_dash_dot_desc =		{4, 24,	{ 9, 6, 3, 6 } };
static GOLineDashDesc const line_dash_dot_dot_desc =    	{6, 24,	{ 9, 3, 3, 3, 3, 3 } };
static GOLineDashDesc const line_dash_desc =			{2, 16,	{ 12, 4 } };
static GOLineDashDesc const line_long_dash_desc =		{2, 22,	{ 18, 4 } };

static struct {
	GOLineDashType type;
	char const *label;
	char const *name;
	GOLineDashDesc const *dash_desc;
} line_dashes[GO_LINE_MAX] = {
	{ GO_LINE_NONE,			N_("None"),
		"none",			NULL },
	{ GO_LINE_SOLID,		N_("Solid"),
		"solid",		NULL },
	{ GO_LINE_S_DOT,		N_("Dot"),
		"s-dot",		&line_short_dot_desc},
	{ GO_LINE_S_DASH_DOT,		N_("Dash dot"),
		"s-dash-dot",		&line_short_dash_dot_desc },
	{ GO_LINE_S_DASH_DOT_DOT,	N_("Dash dot dot"),
		"s-dash-dot-dot",	&line_short_dash_dot_dot_desc },
	{ GO_LINE_DASH_DOT_DOT_DOT,	N_("Dash dot dot dot"),
		"dash-dot-dot-dot",	&line_dash_dot_dot_dot_desc },
	{ GO_LINE_DOT,			N_("Short dash"),
		"dot",			&line_dot_desc},
	{ GO_LINE_S_DASH,		N_("Dash"),
		"s-dash",		&line_short_dash_desc },
	{ GO_LINE_DASH,			N_("Long dash"),
		"dash",			&line_dash_desc },
	{ GO_LINE_LONG_DASH,		N_("Very long dash"),
		"l-dash",		&line_long_dash_desc },
	{ GO_LINE_DASH_DOT,		N_("Long dash dash"),
		"dash-dot",		&line_dash_dot_desc },
	{ GO_LINE_DASH_DOT_DOT,		N_("Long dash dash dash"),
		"dash-dot-dot",		&line_dash_dot_dot_desc }
};

static struct {
	GOLineInterpolation type;
	char const *label;
	char const *name;
	gboolean supports_radial;
	gboolean auto_skip;
} line_interpolations[GO_LINE_INTERPOLATION_MAX] =
{
	{ GO_LINE_INTERPOLATION_LINEAR,			N_("Linear"),
		"linear",		TRUE,  FALSE },
	{ GO_LINE_INTERPOLATION_SPLINE,			N_("Bezier cubic spline"),
		"spline",		TRUE,  FALSE },
	{ GO_LINE_INTERPOLATION_CLOSED_SPLINE,		N_("Closed Bezier cubic spline"),
		"closed-spline",	TRUE,  TRUE },
	{ GO_LINE_INTERPOLATION_ODF_SPLINE,		N_("ODF compatible Bezier cubic spline"),
		"odf-spline",		TRUE,  FALSE },
	{ GO_LINE_INTERPOLATION_CUBIC_SPLINE,		N_("Natural cubic spline"),
		"cspline",		FALSE, FALSE },
	{ GO_LINE_INTERPOLATION_PARABOLIC_CUBIC_SPLINE,	N_("Cubic spline with parabolic extrapolation"),
		"parabolic-cspline",	FALSE, FALSE },
	{ GO_LINE_INTERPOLATION_CUBIC_CUBIC_SPLINE,	N_("Cubic spline with cubic extrapolation"),
		"cubic-cspline",	FALSE, FALSE },
	{ GO_LINE_INTERPOLATION_CLAMPED_CUBIC_SPLINE,   N_("Clamped cubic spline"),
		"clamped-cspline",	FALSE, TRUE },
	{ GO_LINE_INTERPOLATION_STEP_START,		N_("Step at start"),
		"step-start",		TRUE,  FALSE },
	{ GO_LINE_INTERPOLATION_STEP_END,		N_("Step at end"),
		"step-end",		TRUE,  FALSE },
	{ GO_LINE_INTERPOLATION_STEP_CENTER_X,		N_("Step at center"),
		"step-center-x",	TRUE,  FALSE },
	{ GO_LINE_INTERPOLATION_STEP_CENTER_Y,		N_("Step to mean"),
		"step-center-y",	TRUE,  FALSE }
};

/**
 * go_line_dash_from_str:
 * @name: Name of the dash type
 *
 * Returns: a #GOLineDashType corresponding to name, or %GO_LINE_NONE
 * 	if not found.
 **/
GOLineDashType
go_line_dash_from_str (char const *name)
{
	unsigned i;
	GOLineDashType ret = GO_LINE_NONE;

	for (i = 0; i < GO_LINE_MAX; i++) {
		if (strcmp (line_dashes[i].name, name) == 0) {
			ret = line_dashes[i].type;
			break;
		}
	}
	return ret;
}

/**
 * go_line_dash_as_str:
 * @type: a #GOLineDashType
 *
 * Returns: (transfer none): nickname of the dash type, or "none" if
 * type is invalid.
 **/
char const *
go_line_dash_as_str (GOLineDashType type)
{
	unsigned i;
	char const *ret = "none";

	for (i = 0; i < GO_LINE_MAX; i++) {
		if (line_dashes[i].type == type) {
			ret = line_dashes[i].name;
			break;
		}
	}
	return ret;
}

/**
 * go_line_dash_as_label:
 * @type: a #GOLineDashType
 *
 * Returns: (transfer none): user readable name of the dash type,
 * 	or the name of %GO_LINE_NONE if type is invalid.
 **/
char const *
go_line_dash_as_label (GOLineDashType type)
{
	unsigned i;
	char const *ret = line_dashes[0].label;

	for (i = 0; i < GO_LINE_MAX; i++) {
		if (line_dashes[i].type == type) {
			ret = line_dashes[i].label;
			break;
		}
	}
	return _(ret);
}

/**
 * go_line_dash_get_length:
 * @type: #GOLineDashType
 *
 * Returns: the unscaled length of the dash sequence.
 **/
double
go_line_dash_get_length (GOLineDashType type)
{
	GOLineDashDesc const *dash_desc;

	if ((unsigned)type >= G_N_ELEMENTS (line_dashes))
		return 1.0;

	dash_desc = line_dashes[type].dash_desc;
	return dash_desc != NULL ? dash_desc->length : 1.0;
}

/**
 * go_line_dash_get_sequence:
 * @type: a #GOLineDashType
 * @scale: dash scale
 *
 * Returns: (transfer full) (nullable): a struct containing the dash
 * sequence corresponding to @type, or %NULL if type is invalid or
 * equal to %GO_LINE_NONE.  The lengths are scaled according to @scale.
 **/
GOLineDashSequence *
go_line_dash_get_sequence (GOLineDashType type, double scale)
{
	unsigned int i;
	GOLineDashSequence *sequence = NULL;
	GOLineDashDesc const *dash_desc;

	if ((unsigned)type >= G_N_ELEMENTS (line_dashes))
		return NULL;

	dash_desc = line_dashes[type].dash_desc;
	if (dash_desc != NULL) {
		sequence = g_new (GOLineDashSequence, 1);
		sequence->offset = 0.0;
		sequence->n_dash = dash_desc->n_dash;
		sequence->dash = g_new (double, sequence->n_dash);
		for (i = 0; i < sequence->n_dash; i++)
			sequence->dash[i] = scale * dash_desc->dash[i];
		sequence->ref_count = 1;
	}

	return sequence;
}

/**
 * go_line_dash_sequence_free:
 * @sequence: a #GOLineDashSequence
 *
 * Frees the dash sequence struct.
 **/
void
go_line_dash_sequence_free (GOLineDashSequence *sequence)
{
	if (sequence == NULL || sequence->ref_count-- > 1)
		return;
	g_free (sequence->dash);
	g_free (sequence);
}

/**
 * go_line_interpolation_from_str:
 * @name: an interpolation type nickname
 *
 * Returns: a #GOLineInterpolation corresponding to @name, or
 * 	%GO_LINE_INTERPOLATION_LINEAR if not found.
 **/
GOLineInterpolation
go_line_interpolation_from_str (char const *name)
{
	unsigned i;
	GOLineInterpolation ret = GO_LINE_INTERPOLATION_LINEAR;

	for (i = 0; i < GO_LINE_INTERPOLATION_MAX; i++) {
		if (strcmp (line_interpolations[i].name, name) == 0) {
			ret = line_interpolations[i].type;
			break;
		}
	}
	return ret;
}

/**
 * go_line_interpolation_as_str:
 * @type: an interpolation type
 *
 * Returns: (transfer none): nickname of @type, or "linear" if type
 * is invalid.
 **/
char const *
go_line_interpolation_as_str (GOLineInterpolation type)
{
	unsigned i;
	char const *ret = "linear";

	for (i = 0; i < G_N_ELEMENTS (line_interpolations); i++) {
		if (line_interpolations[i].type == type) {
			ret = line_interpolations[i].name;
			break;
		}
	}
	return ret;
}

/**
 * go_line_interpolation_as_label:
 * @type: an interpolation type
 *
 * Returns: (transfer none): label of @type, or the name of
 * %GO_LINE_INTERPOLATION_LINEAR if type is invalid.
 **/
char const *
go_line_interpolation_as_label (GOLineInterpolation type)
{
	unsigned i;
	char const *ret = _("Linear");

	for (i = 0; i < G_N_ELEMENTS (line_interpolations); i++) {
		if (line_interpolations[i].type == type) {
			ret = _(line_interpolations[i].label);
			break;
		}
	}
	return ret;
}

/**
 * go_line_interpolation_supports_radial:
 * @type: an interpolation type
 *
 * Returns: %TRUE if the line interpolation type can be used with radial
 * axes set, %FALSE if it cannot.
 **/
gboolean
go_line_interpolation_supports_radial (GOLineInterpolation type)
{
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (line_interpolations); i++) {
		if (line_interpolations[i].type == type) {
			return line_interpolations[i].supports_radial;
		}
	}
	return FALSE;
}

/**
 * go_line_interpolation_auto_skip:
 * @type: an interpolation type
 *
 * Returns: %TRUE if the line interpolation type forces skipping invalid
 * data, %FALSE if it is only optional.
 **/
gboolean
go_line_interpolation_auto_skip	(GOLineInterpolation type)
{
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (line_interpolations); i++) {
		if (line_interpolations[i].type == type) {
			return line_interpolations[i].auto_skip;
		}
	}
	return FALSE;
}

/* ------------------------------------------------------------------------- */

static struct {
	GOArrowType typ;
	char const *name;
} arrow_types[] = {
	{ GO_ARROW_NONE, "none" },
	{ GO_ARROW_KITE, "kite" },
	{ GO_ARROW_OVAL, "oval" }
};

char const *
go_arrow_type_as_str (GOArrowType typ)
{
	unsigned ui;

	for (ui = 0; ui < G_N_ELEMENTS (arrow_types); ui++)
		if (typ == arrow_types[ui].typ)
			return arrow_types[ui].name;

	return NULL;
}

GOArrowType
go_arrow_type_from_str (const char *name)
{
	unsigned ui;
	GOArrowType ret = GO_ARROW_NONE;

	for (ui = 0; ui < G_N_ELEMENTS (arrow_types); ui++) {
		if (strcmp (arrow_types[ui].name, name) == 0) {
			ret = arrow_types[ui].typ;
			break;
		}
	}

	return ret;
}

GType
go_arrow_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOArrow",
			 (GBoxedCopyFunc)go_arrow_dup,
			 (GBoxedFreeFunc)g_free);
	}
	return t;
}

void
go_arrow_init (GOArrow *res, GOArrowType typ,
	       double a, double b, double c)
{
	res->typ = typ;
	res->a = a;
	res->b = b;
	res->c = c;
}

void
go_arrow_clear (GOArrow *dst)
{
	go_arrow_init (dst, GO_ARROW_NONE, 0, 0, 0);
}

void
go_arrow_init_kite (GOArrow *dst, double a, double b, double c)
{
	go_arrow_init (dst, GO_ARROW_KITE, a, b, c);
}

void
go_arrow_init_oval (GOArrow *dst, double ra, double rb)
{
	go_arrow_init (dst, GO_ARROW_OVAL, ra, rb, 0);
}

GOArrow *
go_arrow_dup (GOArrow *src)
{
	return go_memdup (src, sizeof (*src));
}

gboolean
go_arrow_equal (const GOArrow *a, const GOArrow *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	if (a->typ != b->typ)
		return FALSE;

	switch (a->typ) {
	default:
		g_assert_not_reached ();
	case GO_ARROW_NONE:
		return TRUE;

	case GO_ARROW_KITE:
		if (a->c != b->c)
			return FALSE;
		/* fall through */
	case GO_ARROW_OVAL:
		return (a->a == b->a && a->b == b->b);
	}
}


/**
 * go_arrow_draw:
 * @arrow: arrow to draw
 * @cr: cairo surface to draw on
 * @dx: (out): suggested change of line end-point
 * @dy: (out): suggested change of line end-point
 * @phi: angle to draw at
 *
 **/
void
go_arrow_draw (const GOArrow *arrow, cairo_t *cr,
	       double *dx, double *dy, double phi)
{
	if (dx) *dx = 0;
	if (dy) *dy = 0;

	switch (arrow->typ) {
	case GO_ARROW_NONE:
		return;

	case GO_ARROW_KITE:
		cairo_rotate (cr, phi);
		cairo_set_line_width (cr, 1.0);
		cairo_new_path (cr);
		cairo_move_to (cr, 0.0, 0.0);
		cairo_line_to (cr, -arrow->c, -arrow->b);
		cairo_line_to (cr, 0.0, -arrow->a);
		cairo_line_to (cr, arrow->c, -arrow->b);
		cairo_close_path (cr);
		cairo_fill (cr);

		/*
		 * Make the line shorter so that the arrow won't be on top
		 * of a (perhaps quite fat) line.
		 */
		if (dx) *dx = +arrow->a * sin (phi);
	        if (dy) *dy = -arrow->a * cos (phi);
		break;

	case GO_ARROW_OVAL:
		if (arrow->a > 0 && arrow->b > 0) {
			cairo_rotate (cr, phi);
			cairo_scale (cr, arrow->a, arrow->b);
			cairo_arc (cr, 0., 0., 1., 0., 2 * M_PI);
			cairo_fill (cr);
		}
		break;
	}
}
