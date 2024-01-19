/*
 * go-pattern.c:
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
#include <goffice/goffice-priv.h>
#include <goffice/utils/go-libxml-extras.h>
#include <goffice/utils/go-cairo.h>
#include <goffice/utils/go-pattern.h>
#include <goffice/utils/go-color.h>

#include <glib/gi18n-lib.h>
#include <string.h>

#define CC2XML(s) ((const xmlChar *)(s))
#define CXML2C(s) ((const char *)(s))

/**
 * GOPatternType:
 * @GO_PATTERN_SOLID: solid using background color.
 * @GO_PATTERN_GREY75: 75% background color.
 * @GO_PATTERN_GREY50: 50% background color.
 * @GO_PATTERN_GREY25: 25% background color.
 * @GO_PATTERN_GREY125: 12.5% background color.
 * @GO_PATTERN_GREY625: 6.25% background color.
 * @GO_PATTERN_HORIZ: horizontal stripe.
 * @GO_PATTERN_VERT: vertical stripe.
 * @GO_PATTERN_REV_DIAG: reverse diagonal stripe.
 * @GO_PATTERN_DIAG: diagonal stripe.
 * @GO_PATTERN_DIAG_CROSS: diagonal crosshatch.
 * @GO_PATTERN_THICK_DIAG_CROSS: thick diagonal crosshatch.
 * @GO_PATTERN_THIN_HORIZ: thin horizontal stripe.
 * @GO_PATTERN_THIN_VERT: thin vertical stripe.
 * @GO_PATTERN_THIN_REV_DIAG: thin reverse diagonal stripe.
 * @GO_PATTERN_THIN_DIAG: thin diagonal stripe.
 * @GO_PATTERN_THIN_HORIZ_CROSS: thin horizontal crosshatch.
 * @GO_PATTERN_THIN_DIAG_CROSS: thin diagonal crosshatch.
 * @GO_PATTERN_FOREGROUND_SOLID: solid using foreground color.
 * @GO_PATTERN_SMALL_CIRCLES: small circles.
 * @GO_PATTERN_SEMI_CIRCLES: semi circles.
 * @GO_PATTERN_THATCH: thatch.
 * @GO_PATTERN_LARGE_CIRCLES: large circles.
 * @GO_PATTERN_BRICKS: bricks.
 * @GO_PATTERN_MAX: maximum value, should not occur.
 **/

/**
 * GOPattern:
 * @fore: foreground color.
 * @back: background color.
 * @pattern: A #GOPatternType specifying what kind of pattern to use.
 **/

typedef struct {
	char const *name;
	char const *str;
	int size;
	enum { PT_NA, PT_THIN, PT_NORMAL, PT_THICK } line_width;
	guint8 pattern[8];
} GOPatternSpec;

static GOPatternSpec const go_patterns [] = {
	{
		N_("Solid"),
		"solid",
		1, PT_NA,
		{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
	},
	{
		/* xgettext:no-c-format */
		N_("75% Grey"),
		"grey75",
	1,  PT_NA,
		{ 0xbb, 0xee, 0xbb, 0xee, 0xbb, 0xee, 0xbb, 0xee }
	},
	{
		/* xgettext:no-c-format */
		N_("50% Grey"),
		"grey50",
		1, PT_NA,
		{ 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 }
	},
	{
		/* xgettext:no-c-format */
		N_("25% Grey"),
		"grey25",
		1, PT_NA,
		{ 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88 }
	},
	{
		/* xgettext:no-c-format */
		N_("12.5% Grey"),
		"grey12.5",
		1, PT_NA,
		{ 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 }
	},
	{
		/* xgettext:no-c-format */
		N_("6.25% Grey"),
		"grey6.25",
		1, PT_NA,
		{ 0x20, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02, 0x00 }
	},
	{
		N_("Horizontal Stripe"),
		"horiz",
		8, PT_NORMAL,
		{ 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff }
	},
	{
		N_("Vertical Stripe"),
		"vert",
		8, PT_NORMAL,
		{ 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 }
	},
	{
		N_("Reverse Diagonal Stripe"),
		"rev-diag",
		12, PT_NORMAL,
		{ 0x33, 0x66, 0xcc, 0x99, 0x33, 0x66, 0xcc, 0x99 }
	},
	{
		N_("Diagonal Stripe"),
		"diag",
		12, PT_NORMAL,
		{ 0xcc, 0x66, 0x33, 0x99, 0xcc, 0x66, 0x33, 0x99 }
	},
	{
		N_("Diagonal Crosshatch"),
		"diag-cross",
		16, PT_NORMAL,
		{ 0x99, 0x66, 0x66, 0x99, 0x99, 0x66, 0x66, 0x99 }
	},
	{
		N_("Thick Diagonal Crosshatch"),
		"thick-diag-cross",
		16, PT_THICK,
		{ 0xff, 0x66, 0xff, 0x99, 0xff, 0x66, 0xff, 0x99 }
	},
	{
		N_("Thin Horizontal Stripe"),
		"thin-horiz",
		6, PT_THIN,
		{ 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00 }
	},
	{
		N_("Thin Vertical Stripe"),
		"thin-vert",
		6, PT_THIN,
		{ 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22 }
	},
	{
		N_("Thin Reverse Diagonal Stripe"),
		"thin-rev-diag",
		8, PT_THIN,
		{ 0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88 }
	},
	{
		N_("Thin Diagonal Stripe"),
		"thin-diag",
		8, PT_THIN,
		{ 0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11 }
	},
	{
		N_("Thin Horizontal Crosshatch"),
		"thin-horiz-cross",
		6, PT_THIN,
		{ 0x22, 0x22, 0xff, 0x22, 0x22, 0x22, 0xff, 0x22 }
	},
	{
		N_("Thin Diagonal Crosshatch"),
		"thin-diag-cross",
		9, PT_THIN,
		{ 0x88, 0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55 }
	},
	{
		N_("Foreground Solid"),
		"foreground-solid",
		1, PT_NA,
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
	},
	{
		N_("Small Circles") /* Applix */,
		"small-circles",
		8, PT_THIN,
		{ 0x99, 0x55, 0x33, 0xff, 0x99, 0x55, 0x33, 0xff } },
	{
		N_("Semi Circles") /* Applix */,
		"semi-circles",
		12, PT_THIN,
		{ 0x10, 0x10, 0x28, 0xc7, 0x01, 0x01, 0x82, 0x7c }
	},
	{
		N_("Thatch") /* Applix small thatch */,
		"thatch",
		14, PT_NA,
		{ 0x22, 0x74, 0xf8, 0x71, 0x22, 0x17, 0x8f, 0x47 }
	},
	{
		N_("Large Circles") /*Applix round thatch */,
		"large-circles",
		16, PT_NA,
		{ 0xc1, 0x80, 0x1c, 0x3e, 0x3e, 0x3e, 0x1c, 0x80 }
	},
	{
		N_("Bricks") /* Applix Brick */,
		"bricks",
		12, PT_THIN,
		{ 0x20, 0x20, 0x20, 0xff, 0x02, 0x02, 0x02, 0xff }
	}
};


GOPatternType
go_pattern_from_str (char const *str)
{
	unsigned i;

	for (i = 0; i < GO_PATTERN_MAX; i++)
		if (strcmp (go_patterns[i].str, str) == 0)
			return i;

	return GO_PATTERN_SOLID;
}

char const *
go_pattern_as_str (GOPatternType pattern)
{
	return (unsigned)pattern >= GO_PATTERN_MAX
		? "none"
		: go_patterns[pattern].str;
}

/**
 * go_pattern_is_solid:
 * @pat: #GOPattern
 * @color: #GOColor
 *
 * Returns: if @pat is solid, and stores the color in @color.
 * 	If @pat is not solid @color is not touched.
 **/
gboolean
go_pattern_is_solid (GOPattern const *pat, GOColor *color)
{
	GOPatternType p;

	g_return_val_if_fail (pat != NULL, FALSE);

	p = pat->pattern;
	if (pat->fore == pat->back)
		p = GO_PATTERN_SOLID;

	switch (p) {
	case GO_PATTERN_GREY75:
	case GO_PATTERN_GREY50:
	case GO_PATTERN_GREY25:
	case GO_PATTERN_GREY125:
	case GO_PATTERN_GREY625: {
		static double const grey[] = { -1, 0.75, 0.50, 0.25, 0.125, 0.0625 };
		double g = grey[p];
		*color = GO_COLOR_INTERPOLATE (pat->back, pat->fore, g);
		return TRUE;
	}
	case GO_PATTERN_SOLID:
		*color = pat->back;
		return TRUE;
	case GO_PATTERN_FOREGROUND_SOLID:
		*color = pat->fore;
		return TRUE;
	default:
		return FALSE;
	}
}

/**
 * go_pattern_set_solid:
 * @pat: a #GOPattern
 * @fore: a #GOColor
 *
 * Makes @pat a solid pattern with colour @fore.
 **/
void
go_pattern_set_solid (GOPattern *pat, GOColor fore)
{
	g_return_if_fail (pat != NULL);
	pat->pattern = GO_PATTERN_SOLID;
	pat->fore = GO_COLOR_BLACK;
	pat->back = fore;
}

guint8 const *
go_pattern_get_pattern (GOPattern const *pat)
{
	g_return_val_if_fail (pat != NULL && pat->pattern < GO_PATTERN_MAX, NULL);
	return go_patterns [pat->pattern].pattern;
}

/**
 * go_pattern_get_svg_path: (skip)
 * @pattern: a #GOPattern
 * @width: pattern width
 * @height: pattern height
 *
 * Retrieves an SVG path as string, which represents pattern shape.
 * Caller is responsible for freeing the resulting string.
 *
 * If width != NULL, returns pattern width.
 * If height != NULL, returns pattern height.
 *
 * Returns: (transfer full): a #xmlChar buffer (free using xmlFree).
 **/
xmlChar *
go_pattern_get_svg_path (GOPattern const *pattern, double *width, double *height)
{
	xmlChar	  *svg_path = NULL;
	xmlDocPtr  doc;
	xmlNodePtr ptr;
	const char *data;
	size_t length;

	g_return_val_if_fail (pattern != NULL, NULL);
	g_return_val_if_fail (pattern->pattern < GO_PATTERN_MAX, NULL);

	data = go_rsm_lookup ("go:utils/svg-patterns.xml", &length);
	doc = data ? xmlParseMemory (data, length) : NULL;
	g_return_val_if_fail (doc != NULL, NULL);

	for (ptr = doc->xmlRootNode->xmlChildrenNode;
	     ptr != NULL;
	     ptr = ptr->next)
	{
		if (!xmlIsBlankNode (ptr) &&
		    ptr->name &&
		    !strcmp (CXML2C(ptr->name), "pattern"))
		{
			double value;
			xmlChar *name = xmlGetProp (ptr, CC2XML ("name"));
			if (name != NULL) {
				if (strcmp (CXML2C(name), go_patterns [pattern->pattern].str) == 0) {
					if (width != NULL &&
						go_xml_node_get_double (ptr, "width", &value))
					    *width = value;
					if (height != NULL &&
						go_xml_node_get_double (ptr, "height", &value))
					    *height = value;
					svg_path = xmlGetProp (ptr, CC2XML ("d"));
					xmlFree (name);
					break;
				}
				xmlFree (name);
			}
		}
	}
	xmlFreeDoc (doc);

	g_return_val_if_fail (svg_path != NULL, NULL);

	return svg_path;
}


static cairo_pattern_t *
create_direct_pattern (GOPattern const *pattern, cairo_t *cr)
{
	int target_size;
	cairo_surface_t *cr_surface;
	cairo_pattern_t *pat;
	cairo_t *cr_tmp;
	int lw;
	static const int line_widths[4] = { 0, 2, 4, 6 };
	double lwoff;

	target_size = go_patterns[pattern->pattern].size;
	lw = line_widths[go_patterns[pattern->pattern].line_width];
	lwoff = (lw & 1) ? 0.5 : 0;

	cr_surface = cairo_surface_create_similar (cairo_get_target (cr),
						   CAIRO_CONTENT_COLOR_ALPHA,
						   target_size, target_size);

	cr_tmp = cairo_create (cr_surface);
	cairo_set_source_rgba (cr_tmp, GO_COLOR_TO_CAIRO (pattern->back));
	cairo_paint (cr_tmp);
	cairo_set_source_rgba (cr_tmp, GO_COLOR_TO_CAIRO (pattern->fore));
	cairo_set_line_width (cr_tmp, lw);

	/*
	 * Note: axis-parallel lines are created at integer coordinates
	 * (using integer division) plus lwoff in case the width is odd.
	 * That ought to ensure we fill pixels right.  (But note, that
	 * lwoff is bound to be zero right now.)
	 */

	switch (pattern->pattern) {
	default:
		g_assert_not_reached ();

	case GO_PATTERN_HORIZ:
	case GO_PATTERN_THIN_HORIZ:
		cairo_move_to (cr_tmp, 0, target_size / 2 + lwoff);
		cairo_line_to (cr_tmp, target_size, target_size / 2 + lwoff);
		cairo_stroke (cr_tmp);
		break;

	case GO_PATTERN_VERT:
	case GO_PATTERN_THIN_VERT:
		cairo_move_to (cr_tmp, target_size / 2 + lwoff, 0);
		cairo_line_to (cr_tmp, target_size / 2 + lwoff, target_size);
		cairo_stroke (cr_tmp);
		break;

	case GO_PATTERN_THIN_HORIZ_CROSS:
		cairo_move_to (cr_tmp, 0, target_size / 2 + lwoff);
		cairo_line_to (cr_tmp, target_size, target_size / 2 + lwoff);
		cairo_move_to (cr_tmp, target_size / 2 + lwoff, 0);
		cairo_line_to (cr_tmp, target_size / 2 + lwoff, target_size);
		cairo_stroke (cr_tmp);
		break;

	case GO_PATTERN_DIAG:
	case GO_PATTERN_THIN_DIAG:
		cairo_move_to (cr_tmp, 0, target_size);
		cairo_line_to (cr_tmp, target_size, 0);
		cairo_move_to (cr_tmp, 0, 2 * target_size);
		cairo_line_to (cr_tmp, 2 * target_size, 0);
		cairo_move_to (cr_tmp, -target_size, target_size);
		cairo_line_to (cr_tmp, target_size, -target_size);
		cairo_stroke (cr_tmp);
		break;

	case GO_PATTERN_REV_DIAG:
	case GO_PATTERN_THIN_REV_DIAG:
		cairo_move_to (cr_tmp, 0, 0);
		cairo_line_to (cr_tmp, target_size, target_size);
		cairo_move_to (cr_tmp, -target_size, 0);
		cairo_line_to (cr_tmp, target_size, 2 * target_size);
		cairo_move_to (cr_tmp, 0, -target_size);
		cairo_line_to (cr_tmp, 2 * target_size, target_size);
		cairo_stroke (cr_tmp);
		break;

	case GO_PATTERN_DIAG_CROSS:
	case GO_PATTERN_THICK_DIAG_CROSS:
	case GO_PATTERN_THIN_DIAG_CROSS:
		cairo_move_to (cr_tmp, 0, 0);
		cairo_line_to (cr_tmp, target_size, target_size);
		cairo_move_to (cr_tmp, 0, target_size);
		cairo_line_to (cr_tmp, target_size, 0);
		cairo_stroke (cr_tmp);
		break;

	case GO_PATTERN_BRICKS:
		cairo_move_to (cr_tmp, 0, 0);
		cairo_line_to (cr_tmp, target_size, 0);
		cairo_move_to (cr_tmp, 0, target_size / 2 + lwoff);
		cairo_line_to (cr_tmp, target_size, target_size / 2 + lwoff);
		cairo_move_to (cr_tmp, 0, target_size);
		cairo_line_to (cr_tmp, target_size, target_size);
		cairo_move_to (cr_tmp, target_size / 4 + lwoff, 0);
		cairo_line_to (cr_tmp, target_size / 4 + lwoff, target_size / 2 + lwoff);
		cairo_move_to (cr_tmp, target_size * 3 / 4 + lwoff, target_size / 2 + lwoff);
		cairo_line_to (cr_tmp, target_size * 3 / 4 + lwoff, target_size);
		cairo_stroke (cr_tmp);
		break;

	case GO_PATTERN_SMALL_CIRCLES:
		cairo_arc (cr_tmp, target_size / 2.0, target_size / 2.0,
			   target_size / 2.0 - lw * 0.45,  /* 2 * 5% overlap */
			   0, 2 * M_PI);
		cairo_stroke (cr_tmp);
		break;

	case GO_PATTERN_LARGE_CIRCLES: {
		double dlw = target_size * (sqrt (5) - 1) / 4.0;
		/* Inverse colours */
		cairo_paint (cr_tmp);
		cairo_set_source_rgba (cr_tmp, GO_COLOR_TO_CAIRO (pattern->back));
		cairo_set_line_width (cr_tmp, dlw);
		cairo_arc (cr_tmp, target_size / 2.0, target_size / 2.0,
			   dlw / 2 + target_size / 4.0,
			   0, 2 * M_PI);
		cairo_stroke (cr_tmp);
		break;
	}

	case GO_PATTERN_SEMI_CIRCLES:
		cairo_set_line_cap (cr_tmp, CAIRO_LINE_CAP_ROUND);
		cairo_arc (cr_tmp, target_size / 2.0, target_size / 2.0,
			   target_size / 2.0, 0, M_PI);
		cairo_new_sub_path (cr_tmp);
		cairo_arc (cr_tmp, 0, 0,
			   target_size / 2.0, 0, M_PI);
		cairo_new_sub_path (cr_tmp);
		cairo_arc (cr_tmp, target_size, 0,
			   target_size / 2.0, 0, M_PI);
		cairo_new_sub_path (cr_tmp);
		cairo_arc (cr_tmp, target_size / 2.0, -target_size / 2.0,
			   target_size / 2.0, 0, M_PI);
		cairo_stroke (cr_tmp);
		break;

	case GO_PATTERN_THATCH: {
		double U = target_size / 4.0;

		cairo_move_to (cr_tmp, 1 * U, 0 * U);
		cairo_line_to (cr_tmp, 0 * U, 1 * U);
                cairo_line_to (cr_tmp, 0 * U, 3 * U);
                cairo_line_to (cr_tmp, 0.9 * U, 2.1 * U);
                cairo_line_to (cr_tmp, 1.9 * U, 3.1 * U);
                cairo_line_to (cr_tmp, 1 * U, 4 * U);
                cairo_line_to (cr_tmp, 3 * U, 4 * U);
                cairo_line_to (cr_tmp, 4 * U, 3 * U);
                cairo_line_to (cr_tmp, 4 * U, 1 * U);
                cairo_line_to (cr_tmp, 3.1 * U, 1.9 * U);
                cairo_line_to (cr_tmp, 2.1 * U, 0.9 * U);
                cairo_line_to (cr_tmp, 3 * U, 0 * U);
		cairo_close_path (cr_tmp);
		cairo_move_to (cr_tmp, 1.1 * U, 0.1 * U);
                cairo_line_to (cr_tmp, 3.9 * U, 2.9 * U);
                cairo_line_to (cr_tmp, 2.9 * U, 3.9 * U);
                cairo_line_to (cr_tmp, 0.1 * U, 1.1 * U);
		cairo_close_path (cr_tmp);
		cairo_fill (cr_tmp);
		break;
	}
	}
	cairo_destroy (cr_tmp);
	pat = cairo_pattern_create_for_surface (cr_surface);
	cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
	cairo_surface_destroy (cr_surface);

	return pat;
}

/**
 * go_pattern_create_cairo_pattern:
 * @pattern: a #GOPattern
 * @cr: a cairo context
 *
 * Returns: (transfer full): a cairo pattern object corresponding to @pattern
 * parameters.
 **/
cairo_pattern_t *
go_pattern_create_cairo_pattern (GOPattern const *pattern, cairo_t *cr)
{
	GOColor color;

	g_return_val_if_fail (pattern != NULL && pattern->pattern < GO_PATTERN_MAX, NULL);

	if (go_pattern_is_solid (pattern, &color))
		return cairo_pattern_create_rgba (GO_COLOR_TO_CAIRO (color));

	return create_direct_pattern (pattern, cr);
}
