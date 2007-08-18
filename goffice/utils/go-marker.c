/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-marker.c :
 *
 * Copyright (C) 2003-2007 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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
#include "go-marker.h"
#include "go-color.h"
#include <goffice/utils/go-math.h>
#include <goffice/utils/go-cairo.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#define MARKER_DEFAULT_SIZE 5
#define MARKER_OUTLINE_WIDTH 0.1

struct _GOMarker {
	GObject 	base;

	int		size;
	double		scale;
	GOMarkerShape	shape;
	GOColor		outline_color;
	GOColor		fill_color;
};

typedef struct {
	GObjectClass	base;
} GOMarkerClass;

#define GO_MARKER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS((o),  GO_MARKER_TYPE, GOMarkerClass))

const char square_path[] = 		"M-1,-1 L-1,1 1,1 1,-1 z";
const char diamond_path[] =		"M0,-1 L1,0 0,1 -1,0 z";
const char triangle_down_path[] =	"M-1,-1 L1,-1 0,1 z";
const char triangle_up_path[] =		"M0,-1 L1,1 -1,1 z";
const char triangle_right_path[] =	"M-1,-1 L1,0 -1,1 z";
const char triangle_left_path[] =	"M1,-1 L-1,0 1,1 z";
const char circle_path[] =		"M1,0 C1,0.56 0.56,1 0,1 C-0.56,1 -1,0.56 -1,0 "
					"C-1,-0.56 -0.56,-1 0,-1 C0.56,-1 1,-0.56 1,0 L1,0 z";
const char x_path[] =			"M1,1 L-1,-1 M1,-1 L-1,1";
const char cross_path[] =		"M1,0 L-1,0 M0,1 L0,-1";
const char asterisk_path[] =		"M0.7,0.7 L-0.7,-0.7 M0.7,-0.7 L-0.7,0.7 M1,0 L-1,0 M0,1 L0,-1";
const char bar_path[] =			"M-1 -0.2 L 1 -0.2 L 1 0.2 L -1 0.2 z";
const char half_bar_path[] = 		"M0,-0.2 L1,-0.2 1,0.2 0,0.2 z";
const char butterfly_path[] =		"M-1,-1 L-1,1 0,0 1,1 1,-1 0,0 z";
const char hourglass_path[] =		"M-1,-1 L1,-1 0,0 1,1 -1,1 0,0 z";

typedef struct
{
	char const *name;
	char const *str;
	char const *outline_path;
	char const *fill_path;
} MarkerShape;

#define MAKE_MARKER_SHAPE(name, str, path)	{name, str, path, path}
#define MAKE_MARKER_SQUARED(name, str, path)	{name, str, path, square_path}

static MarkerShape const marker_shapes[GO_MARKER_MAX] = {
    MAKE_MARKER_SHAPE   ( N_("none"),		"none",           NULL),
    MAKE_MARKER_SHAPE   ( N_("square"),		"square",         square_path),
    MAKE_MARKER_SHAPE   ( N_("diamond"),	"diamond",        diamond_path),
    MAKE_MARKER_SHAPE   ( N_("triangle down"),	"triangle-down",  triangle_down_path),
    MAKE_MARKER_SHAPE   ( N_("triangle up"),	"triangle-up",    triangle_up_path),
    MAKE_MARKER_SHAPE   ( N_("triangle right"),	"triangle-right", triangle_right_path),
    MAKE_MARKER_SHAPE   ( N_("triangle left"),	"triangle-left",  triangle_left_path),
    MAKE_MARKER_SHAPE   ( N_("circle"),		"circle",         circle_path),
    MAKE_MARKER_SQUARED ( N_("x"),		"x",              x_path),
    MAKE_MARKER_SQUARED ( N_("cross"),		"cross",          cross_path),
    MAKE_MARKER_SQUARED ( N_("asterisk"),	"asterisk",       asterisk_path),
    MAKE_MARKER_SHAPE   ( N_("bar"), 		"bar",            bar_path),
    MAKE_MARKER_SHAPE   ( N_("half bar"),	"half-bar",       half_bar_path),
    MAKE_MARKER_SHAPE   ( N_("butterfly"),	"butterfly",      butterfly_path),
    MAKE_MARKER_SHAPE   ( N_("hourglass"),	"hourglass",      hourglass_path)
};

static GObjectClass *marker_parent_klass;

static void
go_marker_init (GOMarker * marker)
{
	marker->shape		= GO_MARKER_NONE;
	marker->outline_color	= RGBA_BLACK;
	marker->fill_color	= RGBA_WHITE;
	marker->size		= MARKER_DEFAULT_SIZE;
}

static void
go_marker_class_init (GObjectClass *gobject_klass)
{
	marker_parent_klass = g_type_class_peek_parent (gobject_klass);
}

GOMarkerShape
go_marker_shape_from_str (char const *str)
{
	unsigned i;
	for (i = 0; i < GO_MARKER_MAX; i++)
		if (g_ascii_strcasecmp (marker_shapes[i].str, str) == 0)
			return (GOMarkerShape)i;
	return GO_MARKER_NONE;
}

char const *
go_marker_shape_as_str (GOMarkerShape shape)
{
	return (shape < 0 || shape >= GO_MARKER_MAX) ? "pattern"
		: marker_shapes[shape].str;
}

static void
go_marker_get_paths (GOMarker const *marker,
		     char const **outline_path,
		     char const **fill_path)
{
	*outline_path = marker_shapes[marker->shape].outline_path;
	*fill_path = marker_shapes[marker->shape].fill_path;
}

GOMarkerShape
go_marker_get_shape (GOMarker const *marker)
{
	return marker->shape;
}

void
go_marker_set_shape (GOMarker *marker, GOMarkerShape shape)
{
	g_return_if_fail (IS_GO_MARKER (marker));

	if (marker->shape == shape)
		return;
	marker->shape = shape;
}

GOColor
go_marker_get_outline_color (GOMarker const *marker)
{
	return marker->outline_color;
}

void
go_marker_set_outline_color (GOMarker *marker, GOColor color)
{
	g_return_if_fail (IS_GO_MARKER (marker));
	if (marker->outline_color == color)
		return;
	marker->outline_color = color;
}

GOColor
go_marker_get_fill_color (GOMarker const *marker)
{
	return marker->fill_color;
}

void
go_marker_set_fill_color (GOMarker *marker, GOColor color)
{
	g_return_if_fail (IS_GO_MARKER (marker));
	if (marker->fill_color == color)
		return;
	marker->fill_color = color;
}

int
go_marker_get_size (GOMarker const *marker)
{
	return marker->size;
}

double
go_marker_get_outline_width (GOMarker const *marker)
{
	return (double)marker->size * MARKER_OUTLINE_WIDTH;
}

void
go_marker_set_size (GOMarker *marker, int size)
{
	g_return_if_fail (IS_GO_MARKER (marker));
	g_return_if_fail (size >= 0);
	if (marker->size == size)
		return;
	marker->size = size;
}

void
go_marker_assign (GOMarker *dst, GOMarker const *src)
{
	if (src == dst)
		return;

	g_return_if_fail (GO_MARKER (src) != NULL);
	g_return_if_fail (GO_MARKER (dst) != NULL);

	dst->size		= src->size;
	dst->shape		= src->shape;
	dst->outline_color	= src->outline_color;
	dst->fill_color		= src->fill_color;

}

GOMarker *
go_marker_dup (GOMarker const *src)
{
	GOMarker *dst = go_marker_new ();
	go_marker_assign (dst, src);
	return dst;
}

GOMarker *
go_marker_new (void)
{
	return g_object_new (GO_MARKER_TYPE, NULL);
}

/**
 * go_marker_render:
 * @marker: a #GOMarker
 * @cr: a cairo context
 * @x: x position
 * @y: y position
 * @scale: current scale
 *
 * Renders @marker onto the @cairo target, using @x and @y for the position.
 **/

void
go_marker_render (GOMarker const *marker, cairo_t *cr, double x, double y, double scale)
{
	char const *outline_path_raw, *fill_path_raw;
	double half_size;

	go_marker_get_paths (marker, &outline_path_raw, &fill_path_raw);

	if ((outline_path_raw == NULL) ||
	    (fill_path_raw == NULL))
		return;

	cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);

	half_size = 0.5 *  scale * go_marker_get_size (marker);

	cairo_translate (cr, x, y);
	cairo_scale (cr, half_size, half_size);

	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (go_marker_get_fill_color (marker)));
	go_cairo_emit_svg_path (cr, fill_path_raw);
	cairo_fill (cr);

	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (go_marker_get_outline_color (marker)));
	cairo_set_line_width (cr, 2.0 * scale * MARKER_OUTLINE_WIDTH);
	go_cairo_emit_svg_path (cr, outline_path_raw);
	cairo_stroke (cr);
}

/**
 * go_marker_create_cairo_surface:
 * @marker: a #GOMarker
 * @cr: a cairo context
 * @scale: current context scale
 * @center: a placeholder for the center position of the surface
 *
 * Creates a new cairo surface similar to the current target of @cr, and render
 * @marker on it. @center will contain the coordinate of the center of the surface.
 *
 * Returns the newly created surface. This surface should be destroyed using cairo_surface_destroy
 * after use.
 **/

cairo_surface_t *
go_marker_create_cairo_surface (GOMarker const *marker, cairo_t *cr, double scale, double *center)
{
	cairo_t *cairo;
	cairo_surface_t *surface;
	cairo_surface_t *current_surface;
	cairo_surface_type_t surface_type;
	double half_size, offset;

	g_return_val_if_fail (IS_GO_MARKER (marker), NULL);
	g_return_val_if_fail (cr != NULL, NULL);

	current_surface = cairo_get_target (cr);
	surface_type = cairo_surface_get_type (current_surface);

	if (surface_type == CAIRO_SURFACE_TYPE_SVG ||
	    surface_type == CAIRO_SURFACE_TYPE_PS ||
	    surface_type == CAIRO_SURFACE_TYPE_PDF) {
		half_size = scale * go_marker_get_size (marker) / 2.0;
		offset = half_size + scale * go_marker_get_outline_width (marker);
	} else {
		half_size = rint (scale * go_marker_get_size (marker)) / 2.0;
		offset = ceil (scale * go_marker_get_outline_width (marker) / 2.0) +
			half_size + .5;
	}

	surface = cairo_surface_create_similar (cairo_get_target (cr),
						CAIRO_CONTENT_COLOR_ALPHA,
						2.0 * offset,
						2.0 * offset);
	cairo = cairo_create (surface);

	go_marker_render (marker, cairo, offset, offset, scale);

	cairo_destroy (cairo);

	if (center)
		*center = offset;

	return surface;
}

GSF_CLASS (GOMarker, go_marker,
	   go_marker_class_init, go_marker_init,
	   G_TYPE_OBJECT)
