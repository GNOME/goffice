/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-pattern.c :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#include <goffice/goffice-priv.h>
#include <goffice/utils/go-libxml-extras.h>
#include <goffice/utils/go-cairo.h>
#include <goffice/utils/go-pattern.h>
#include <goffice/utils/go-color.h>

#include <glib/gi18n-lib.h>
#include <string.h>

#define CC2XML(s) ((const xmlChar *)(s))

typedef struct {
	char const *name;
	char const *str;
	guint8 pattern[8];
} GOPatternSpec;

static GOPatternSpec const go_patterns [] = {
  { N_("Solid"),                     "solid",           { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } },
/* xgettext:no-c-format */
  { N_("75% Grey"),                  "grey75",          { 0xbb, 0xee, 0xbb, 0xee, 0xbb, 0xee, 0xbb, 0xee } },
/* xgettext:no-c-format */
  { N_("50% Grey"),                  "grey50",          { 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55 } },
/* xgettext:no-c-format */
  { N_("25% Grey"),                  "grey25",          { 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88 } },
/* xgettext:no-c-format */
  { N_("12.5% Grey"),                "grey12.5",        { 0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00 } },
/* xgettext:no-c-format */
  { N_("6.25% Grey"),                "grey6.25",        { 0x20, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02, 0x00 } },
  { N_("Horizontal Stripe"),         "horiz",           { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff } },
  { N_("Vertical Stripe"),           "vert",            { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 } },
  { N_("Reverse Diagonal Stripe"),   "rev-diag",        { 0x33, 0x66, 0xcc, 0x99, 0x33, 0x66, 0xcc, 0x99 } },
  { N_("Diagonal Stripe"),           "diag",            { 0xcc, 0x66, 0x33, 0x99, 0xcc, 0x66, 0x33, 0x99 } },
  { N_("Diagonal Crosshatch"),       "diag-cross",      { 0x99, 0x66, 0x66, 0x99, 0x99, 0x66, 0x66, 0x99 } },
  { N_("Thick Diagonal Crosshatch"), "thick-diag-cross",{ 0xff, 0x66, 0xff, 0x99, 0xff, 0x66, 0xff, 0x99 } },
  { N_("Thin Horizontal Stripe"),    "thin-horiz",      { 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0xff, 0x00 } },
  { N_("Thin Vertical Stripe"),      "thin-vert",       { 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22 } },
  { N_("Thin Reverse Diagonal Stripe"),"thin-rev-diag", { 0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88 } },
  { N_("Thin Diagonal Stripe"),      "thin-diag",       { 0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11 } },
  { N_("Thin Horizontal Crosshatch"),"thin-horiz-cross",{ 0x22, 0x22, 0xff, 0x22, 0x22, 0x22, 0xff, 0x22 } },
  { N_("Thin Diagonal Crosshatch"),  "thin-diag-cross", { 0x88, 0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55 } },
  { N_("Foreground Solid"),          "foreground-solid",{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
  { N_("Small Circles")/* Applix */, "small-circles",   { 0x99, 0x55, 0x33, 0xff, 0x99, 0x55, 0x33, 0xff } },
  { N_("Semi Circles") /* Applix */, "semi-circles",    { 0x10, 0x10, 0x28, 0xc7, 0x01, 0x01, 0x82, 0x7c } },
  { N_("Thatch") /* Applix small thatch */, "thatch",   { 0x22, 0x74, 0xf8, 0x71, 0x22, 0x17, 0x8f, 0x47 } },
  { N_("Large Circles")/*Applix round thatch*/,
				     "large-circles",   { 0xc1, 0x80, 0x1c, 0x3e, 0x3e, 0x3e, 0x1c, 0x80 } },
  { N_("Bricks") /* Applix Brick */, "bricks",          { 0x20, 0x20, 0x20, 0xff, 0x02, 0x02, 0x02, 0xff } }
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
	return (pattern < 0 || pattern >= GO_PATTERN_MAX) ?  "none"
		: go_patterns[pattern].str;
}

/**
 * go_pattern_is_solid :
 * @pat : #GOPattern
 * @color : #GOColor
 *
 * Returns: if @pat is solid, and stores the color in @color.
 * 	If @pat is not solid @color is not touched.
 **/
gboolean
go_pattern_is_solid (GOPattern const *pat, GOColor *color)
{
	g_return_val_if_fail (pat != NULL, FALSE);

	if (pat->pattern == GO_PATTERN_SOLID || pat->fore == pat->back) {
		*color = pat->back;
		return TRUE;
	}
	if (pat->pattern == GO_PATTERN_FOREGROUND_SOLID) {
		*color = pat->fore;
		return TRUE;
	}
	return FALSE;
}

/**
 * go_pattern_set_solid :
 * @pat  : a #GOPattern
 * @fore : a #GOColor
 *
 * Makes @pat a solid pattern with colour @fore.
 *
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
	return go_patterns [pat->pattern].pattern;
}

/**
 * go_pattern_get_svg_path:
 * @pattern: a #GOPattern
 * @width:  pattern width
 * @height:  pattern height
 *
 * Retrieves an SVG path as string, which represents pattern shape.
 * Caller is responsible for freeing the resulting string.
 *
 * If width != NULL, returns pattern width.
 * If height != NULL, returns pattern height.
 *
 * Returns: a #xmlChar buffer (free using xmlFree).
 **/
xmlChar *
go_pattern_get_svg_path (GOPattern const *pattern, double *width, double *height)
{
	char *path;
	char *d = NULL;
	xmlChar	  *name, *svg_path = NULL;
	xmlDocPtr  doc;
	xmlNodePtr ptr;

	g_return_val_if_fail (pattern->pattern < GO_PATTERN_MAX, NULL);

	path = g_build_filename (go_sys_data_dir(), "patterns", "svg-patterns.xml", NULL);
	doc = go_xml_parse_file (path);
	g_free (path);

	g_return_val_if_fail (doc != NULL, NULL);

	for (ptr = doc->xmlRootNode->xmlChildrenNode;
	     ptr != NULL && d == NULL ;
	     ptr = ptr->next)
	{
		if (!xmlIsBlankNode (ptr) &&
		    ptr->name &&
		    !strcmp ((char *)ptr->name, "pattern"))
		{
			double value;
			name = xmlGetProp (ptr, CC2XML ("name"));
			if (name != NULL) {
				if (strcmp ((char *)name, go_patterns [pattern->pattern].str) == 0) {
					if (width != NULL &&
						go_xml_node_get_double (ptr, "width", &value))
					    *width = value;
					if (height != NULL &&
						go_xml_node_get_double (ptr, "height", &value))
					    *height = value;
					svg_path = xmlGetProp (ptr, CC2XML ("d"));
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

#define SVG_PATTERN_SCALE 2.0

/**
 * go_pattern_create_cairo_pattern:
 * @pattern: a #GOPattern
 * @cr: a cairo context
 *
 * Returns: a cairo pattern object corresponding to @pattern parameters. The returned
 * 	surface must be freed after use, using cairo_pattern_destroy.
 **/
cairo_pattern_t *
go_pattern_create_cairo_pattern (GOPattern const *pattern, cairo_t *cr)
{
	cairo_surface_t *cr_surface;
	cairo_pattern_t *cr_pattern;
	GOColor color;

	if (go_pattern_is_solid (pattern, &color)) {
		cr_pattern = cairo_pattern_create_rgba (GO_COLOR_TO_CAIRO (color));
#if 0
		/* This code is disabled for now. Cairo export of vector pattern
		 * to PDF or PS looks terrible, and even SVG export is not properly rendered
		 * with Inkscape. */

	} else if (go_cairo_surface_is_vector (cairo_get_target (cr))) {
		cairo_t *cr_tmp;
		xmlChar *svg_path;
		double width, height;

		svg_path = go_pattern_get_svg_path (pattern, &width, &height);

		cr_surface = cairo_surface_create_similar (cairo_get_target (cr),
							   CAIRO_CONTENT_COLOR_ALPHA,
							   width * SVG_PATTERN_SCALE,
							   height * SVG_PATTERN_SCALE);

		cr_tmp = cairo_create (cr_surface);

		cairo_set_source_rgba (cr_tmp, GO_COLOR_TO_CAIRO (pattern->back));
		cairo_paint (cr_tmp);

		cairo_set_source_rgba (cr_tmp, GO_COLOR_TO_CAIRO (pattern->fore));
		cairo_scale (cr_tmp, SVG_PATTERN_SCALE, SVG_PATTERN_SCALE);
		go_cairo_emit_svg_path (cr_tmp, svg_path);
		cairo_set_line_width (cr_tmp, 0.2);
		cairo_fill (cr_tmp);
		xmlFree (svg_path);

		cairo_destroy (cr_tmp);

		cr_pattern = cairo_pattern_create_for_surface (cr_surface);
		cairo_pattern_set_extend (cr_pattern, CAIRO_EXTEND_REPEAT);
		cairo_surface_destroy (cr_surface);
#endif
	} else {
		unsigned int stride, i, j, t;
		unsigned char *iter;
		guint8 const *pattern_data;

		pattern_data = go_pattern_get_pattern (pattern);

		cr_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 8, 8);
		cairo_surface_flush (cr_surface); /* documentation says this one must be called */
		stride = cairo_image_surface_get_stride (cr_surface);
		iter = cairo_image_surface_get_data (cr_surface);

#define MULT(d,c,a,t) G_STMT_START { t = c * a + 0x7f; d = ((t >> 8) + t) >> 8; } G_STMT_END

		if (iter != NULL) {
			for (i = 0; i < 8; i++) {
				for (j = 0; j < 8; j++) {
					color = pattern_data[i] & (1 << j) ? pattern->fore : pattern->back;
					iter[3] = GO_COLOR_UINT_A (color);
					MULT (iter[0], GO_COLOR_UINT_B (color), iter[3], t);
					MULT (iter[1], GO_COLOR_UINT_G (color), iter[3], t);
					MULT (iter[2], GO_COLOR_UINT_R (color), iter[3], t);
					iter += 4;
				}
				iter += stride - 32;
			}
		}
		cairo_surface_mark_dirty (cr_surface);

		cr_pattern = cairo_pattern_create_for_surface (cr_surface);
		cairo_pattern_set_extend (cr_pattern, CAIRO_EXTEND_REPEAT);
		cairo_surface_destroy (cr_surface);
	}

	return cr_pattern;
}
