/*
 * go-marker.c :
 *
 * Copyright (C) 2003-2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

/**
 * GOMarkerShape:
 * @GO_MARKER_NONE: no mark.
 * @GO_MARKER_SQUARE: square.
 * @GO_MARKER_DIAMOND: diamond.
 * @GO_MARKER_TRIANGLE_DOWN: triangle down.
 * @GO_MARKER_TRIANGLE_UP: triangle up.
 * @GO_MARKER_TRIANGLE_RIGHT: triangle right.
 * @GO_MARKER_TRIANGLE_LEFT: triangle left.
 * @GO_MARKER_CIRCLE: circle.
 * @GO_MARKER_X: X.
 * @GO_MARKER_CROSS: cross.
 * @GO_MARKER_ASTERISK: asterisk.
 * @GO_MARKER_BAR: horizontal bar.
 * @GO_MARKER_HALF_BAR: right half bar.
 * @GO_MARKER_BUTTERFLY: butterfly.
 * @GO_MARKER_HOURGLASS: hourglass.
 * @GO_MARKER_LEFT_HALF_BAR: left half bar.
 * @GO_MARKER_MAX: maximum value, should not occur.
 **/

#define MARKER_DEFAULT_SIZE 5
#define MARKER_OUTLINE_WIDTH 0.1

struct _GOMarker {
	GObject 	base;

	int		size;
	GOMarkerShape	shape;
	GOColor		outline_color;
	GOColor		fill_color;
};

typedef struct {
	GObjectClass	base;
} GOMarkerClass;

#define GO_MARKER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS((o),  GO_TYPE_MARKER, GOMarkerClass))

static char const square_path[] = 		"M-1,-1 L-1,1 1,1 1,-1 z";
static char const diamond_path[] =		"M0,-1 L1,0 0,1 -1,0 z";
static char const triangle_down_path[] =	"M-1,-1 L1,-1 0,1 z";
static char const triangle_up_path[] =		"M0,-1 L1,1 -1,1 z";
static char const triangle_right_path[] =	"M-1,-1 L1,0 -1,1 z";
static char const triangle_left_path[] =	"M1,-1 L-1,0 1,1 z";
static char const circle_path[] =		"M1,0 C1,0.56 0.56,1 0,1 C-0.56,1 -1,0.56 -1,0 "
						"C-1,-0.56 -0.56,-1 0,-1 C0.56,-1 1,-0.56 1,0 L1,0 z";
static char const x_path[] =			"M1,1 L-1,-1 M1,-1 L-1,1";
static char const cross_path[] =		"M1,0 L-1,0 M0,1 L0,-1";
static char const asterisk_path[] =		"M0.7,0.7 L-0.7,-0.7 M0.7,-0.7 L-0.7,0.7 M1,0 L-1,0 M0,1 L0,-1";
static char const bar_path[] =			"M-1 -0.2 L 1 -0.2 L 1 0.2 L -1 0.2 z";
static char const half_bar_path[] = 		"M0,-0.2 L1,-0.2 1,0.2 0,0.2 z";
static char const butterfly_path[] =		"M-1,-1 L-1,1 0,0 1,1 1,-1 0,0 z";
static char const hourglass_path[] =		"M-1,-1 L1,-1 0,0 1,1 -1,1 0,0 z";
static char const left_half_bar_path[] =	"M0,-0.2 L-1,-0.2 -1,0.2 0,0.2 z";

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
    MAKE_MARKER_SHAPE   ( N_("hourglass"),	"hourglass",      hourglass_path),
    MAKE_MARKER_SHAPE   ( N_("left half bar"),	"lefthalf-bar",	  left_half_bar_path)
};

static GObjectClass *marker_parent_klass;

static void
go_marker_init (GOMarker * marker)
{
	marker->shape		= GO_MARKER_NONE;
	marker->outline_color	= GO_COLOR_BLACK;
	marker->fill_color	= GO_COLOR_WHITE;
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
	return (unsigned)shape >= GO_MARKER_MAX
		? "pattern"
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
	g_return_if_fail (GO_IS_MARKER (marker));

	if (marker->shape == shape)
		return;
	marker->shape = shape;
}

gboolean
go_marker_is_closed_shape (GOMarker const *m)
{
	g_return_val_if_fail (GO_IS_MARKER (m), FALSE);
	switch (m->shape) {
	case GO_MARKER_X:
	case GO_MARKER_CROSS:
	case GO_MARKER_ASTERISK:
		return FALSE;
	default:
		return TRUE;
	}
}

GOColor
go_marker_get_outline_color (GOMarker const *marker)
{
	return marker->outline_color;
}

void
go_marker_set_outline_color (GOMarker *marker, GOColor color)
{
	g_return_if_fail (GO_IS_MARKER (marker));
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
	g_return_if_fail (GO_IS_MARKER (marker));
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
	g_return_if_fail (GO_IS_MARKER (marker));
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

/**
 * go_marker_dup:
 * @src: the #GOMarker to duplicate
 *
 * Duplicates @src.
 * Returns: (transfer full): the duplicated marker.
 **/
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
	return g_object_new (GO_TYPE_MARKER, NULL);
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

	cairo_save (cr);

	cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
	cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);

	half_size = 0.5 *  scale * go_marker_get_size (marker);

	cairo_translate (cr, x, y);
	cairo_scale (cr, half_size, half_size);

	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (go_marker_get_fill_color (marker)));
	go_cairo_emit_svg_path (cr, fill_path_raw);
	cairo_fill (cr);

	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (go_marker_get_outline_color (marker)));
	cairo_set_line_width (cr, 2.0 * MARKER_OUTLINE_WIDTH);
	cairo_set_dash (cr, NULL, 0, 0.);
	go_cairo_emit_svg_path (cr, outline_path_raw);
	cairo_stroke (cr);

	cairo_restore (cr);
}

/**
 * go_marker_create_cairo_surface:
 * @marker: a #GOMarker
 * @cr: a cairo context
 * @scale: current context scale
 * @width: a placeholder for the surface width
 * @height: a placeholder for the surface height
 *
 * Creates a new cairo surface similar to the current target of @cr, and render
 * @marker on it. @center will contain the coordinate of the center of the surface.
 *
 * Returns:  a newly created #cairo_surface_t. This surface should be destroyed
 * 	using cairo_surface_destroy after use.
 **/
cairo_surface_t *
go_marker_create_cairo_surface (GOMarker const *marker, cairo_t *cr, double scale,
				double *width, double *height)
{
	cairo_t *cr_tmp;
	cairo_surface_t *cr_surface;
	cairo_surface_t *current_cr_surface;
	double half_size, offset;

	g_return_val_if_fail (GO_IS_MARKER (marker), NULL);
	g_return_val_if_fail (cr != NULL, NULL);

	current_cr_surface = cairo_get_target (cr);

	if (go_cairo_surface_is_vector (current_cr_surface)) {
		half_size = scale * go_marker_get_size (marker) * 0.5;
		offset = half_size + scale * go_marker_get_outline_width (marker);
	} else {
		half_size = rint (scale * go_marker_get_size (marker)) * 0.5;
		offset = ceil (scale * go_marker_get_outline_width (marker) * 0.5) +
			half_size + .5;
	}

	cr_surface = cairo_surface_create_similar (current_cr_surface,
						   CAIRO_CONTENT_COLOR_ALPHA,
						   ceil (2.0 * offset),
						   ceil (2.0 * offset));
	cr_tmp = cairo_create (cr_surface);

	go_marker_render (marker, cr_tmp, offset, offset, scale);

	cairo_destroy (cr_tmp);

	if (width != NULL)
		*width = offset * 2.0;
	if (height != NULL)
		*height = offset * 2.0;

	return cr_surface;
}

GSF_CLASS (GOMarker, go_marker,
	   go_marker_class_init, go_marker_init,
	   G_TYPE_OBJECT)
