/*
 * gog-renderer.c :
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
#include <goffice/goffice-debug.h>
#include <goffice/graph/gog-renderer.h>

#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-output-stdio.h>

#include <glib/gi18n-lib.h>

#include <pango/pangocairo.h>

#include <cairo.h>
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif

#include <math.h>

/**
 * SECTION: gog-renderer
 * @short_description: Rendering
 *
 * Note that #GogGraph has a functions for export/rendering, so you do not always
 * need to use a #GogRenderer directly.
*/

enum {
	RENDERER_PROP_0,
	RENDERER_PROP_MODEL,
	RENDERER_PROP_VIEW,
	RENDERER_PROP_ZOOM
};
enum {
	RENDERER_SIGNAL_REQUEST_UPDATE,
	RENDERER_SIGNAL_LAST
};
static gulong renderer_signals [RENDERER_SIGNAL_LAST] = { 0, };

static GObjectClass *parent_klass;

struct _GogRenderer {
	GObject	 base;

	GogGraph *model;
	GogView	 *view;
	double	  scale, scale_x, scale_y;

	GClosure *font_watcher;
	gboolean  needs_update;

	GOStyle const *cur_style;
	GSList   *style_stack;

	GOLineDashSequence 	*line_dash;

	GOStyle 	*grip_style;
	GOStyle 	*selection_style;

	int 		 w, h;

	gboolean	 is_vector;

	cairo_t		*cairo;
	cairo_surface_t *cairo_surface;

	GdkPixbuf 	*pixbuf;

	gboolean	 marker_as_surface;
	cairo_surface_t *marker_surface;
	double		 marker_offset;
	GOMarker	*marker;
};

typedef struct {
	GObjectClass base;

	/* Signals */
	void (*request_update) (GogRenderer *renderer);
} GogRendererClass;

#define GOG_RENDERER_CLASS(k)	 (G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_RENDERER, GogRendererClass))
#define GOG_IS_RENDERER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_RENDERER))

#define GRC_LINEAR_SCALE(w,scale) (go_sub_epsilon (w) <= 0.0 ? GOG_RENDERER_HAIRLINE_WIDTH_PTS : w) * scale

static double
_grc_line_size (GogRenderer const *rend, double width, gboolean sharp)
{
	if (rend->is_vector)
		return GRC_LINEAR_SCALE (width, rend->scale);

	if (go_sub_epsilon (width) <= 0.) /* cheesy version of hairline */
		return 1.;

	width *= rend->scale;
	if (!sharp || width <= 1.)
		return width;

	return go_fake_round (width);
}

static double
_line_size (GogRenderer const *rend, double width, gboolean sharp)
{
	double size = _grc_line_size (rend, width, sharp);

	if (!rend->is_vector && sharp && size < 1.0)
		return ceil (size);

	return size;
}

static void
_update_dash (GogRenderer *rend)
{
	double size;

	go_line_dash_sequence_free (rend->line_dash);
	rend->line_dash = NULL;

	if (rend->cur_style == NULL)
		return;

	size = _line_size (rend, rend->cur_style->line.width, FALSE);
	rend->line_dash = go_line_dash_get_sequence (rend->cur_style->line.dash_type, size);
}

double
gog_renderer_line_size (GogRenderer const *rend, double width)
{
	return _line_size (rend, width, TRUE);
}

double
gog_renderer_pt2r_x (GogRenderer const *rend, double d)
{
	return d * rend->scale_x;
}

double
gog_renderer_pt2r_y (GogRenderer const *rend, double d)
{
	return d * rend->scale_y;
}

double
gog_renderer_pt2r (GogRenderer const *rend, double d)
{
	return d * rend->scale;
}

/*****************************************************************************/

static void
emit_line (GogRenderer *rend, gboolean preserve, GOPathOptions options)
{
	GOStyle const *style = rend->cur_style;
	cairo_t *cr = rend->cairo;
	double width;

	if (!go_style_is_line_visible (style)) {
		if (!preserve)
			cairo_new_path (cr);
		return;
	}

	width = _grc_line_size (rend, style->line.width, options & GO_PATH_OPTIONS_SNAP_WIDTH);
	cairo_set_line_width (cr, width);
	if (rend->line_dash != NULL)
		cairo_set_dash (cr,
				rend->line_dash->dash,
				rend->line_dash->n_dash,
				rend->line_dash->offset);
	else
		cairo_set_dash (cr, NULL, 0, 0);

	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->line.color));
	cairo_set_line_cap (cr, (width <= 2.0 && !rend->is_vector) ?
			    CAIRO_LINE_CAP_SQUARE : CAIRO_LINE_CAP_ROUND);

	if (preserve)
		cairo_stroke_preserve (cr);
	else
		cairo_stroke (cr);
}

static void
emit_fill (GogRenderer *rend, gboolean preserve)
{
	go_style_fill (rend->cur_style, rend->cairo, preserve);
}

static void
path_raw_move_to (void *closure, GOPathPoint const *point)
{
	cairo_move_to ((cairo_t *) closure, point->x, point->y);
}

static void
path_raw_line_to (void *closure, GOPathPoint const *point)
{
	cairo_line_to ((cairo_t *) closure, point->x, point->y);
}

static void
path_snap_even_move_to (void *closure, GOPathPoint const *point)
{
	cairo_move_to ((cairo_t *) closure,
		       floor (point->x + .5),
		       floor (point->y + .5));
}

static void
path_snap_even_line_to (void *closure, GOPathPoint const *point)
{
	cairo_line_to ((cairo_t *) closure,
		       floor (point->x + .5),
		       floor (point->y + .5));
}

static void
path_snap_odd_move_to (void *closure, GOPathPoint const *point)
{
	cairo_move_to ((cairo_t *) closure,
		       floor (point->x) + .5,
		       floor (point->y) + .5);
}

static void
path_snap_odd_line_to (void *closure, GOPathPoint const *point)
{
	cairo_line_to ((cairo_t *) closure,
		       floor (point->x) + .5,
		       floor (point->y) + .5);
}

static void
path_curve_to (void *closure,
	       GOPathPoint const *point0,
	       GOPathPoint const *point1,
	       GOPathPoint const *point2)
{
	cairo_curve_to ((cairo_t *) closure,
			point0->x, point0->y,
			point1->x, point1->y,
			point2->x, point2->y);
}

static void
path_close_path (void *closure)
{
	cairo_close_path ((cairo_t *) closure);
}

static void
path_interpret (GogRenderer 	 *rend,
		GOPath const 	 *path,
		double 		  line_width)
{
	if (rend->is_vector
	    || (go_path_get_options (path) & GO_PATH_OPTIONS_SNAP_COORDINATES) == 0) {
		go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
				   path_raw_move_to,
				   path_raw_line_to,
				   path_curve_to,
				   path_close_path,
				   rend->cairo);
		return;
	}

	if (((int) (go_fake_ceil (line_width)) % 2 == 0) && (line_width > 1.0 || line_width <= 0.0))
		go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
				   path_snap_even_move_to,
				   path_snap_even_line_to,
				   path_curve_to,
				   path_close_path,
				   rend->cairo);
	else
		go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
				   path_snap_odd_move_to,
				   path_snap_odd_line_to,
				   path_curve_to,
				   path_close_path,
				   rend->cairo);
}

typedef struct {
	cairo_t	*cairo;
	gboolean first_point;
} FillPathData;

static void
fill_path_line_to (void *closure, GOPathPoint const *point)
{
	FillPathData *data = closure;

	if (data->first_point) {
		cairo_move_to (data->cairo, point->x, point->y);
		data->first_point = FALSE;
	}
	else
		cairo_line_to (data->cairo, point->x, point->y);
}

static void
fill_path_curve_to (void *closure,
		    GOPathPoint const *point0,
		    GOPathPoint const *point1,
		    GOPathPoint const *point2)
{
	FillPathData *data = closure;

	cairo_curve_to (data->cairo,
			point0->x, point0->y,
			point1->x, point1->y,
			point2->x, point2->y);
}

static void
fill_path_close_path (void *closure)
{
}

static void
fill_path_interpret (GogRenderer  *rend,
		     GOPath const *path_0,
		     GOPath const *path_1)
{
	FillPathData data;

	data.first_point = TRUE;
	data.cairo = rend->cairo;

	go_path_interpret (path_0,
			   GO_PATH_DIRECTION_FORWARD,
			   fill_path_line_to,
			   fill_path_line_to,
			   fill_path_curve_to,
			   fill_path_close_path,
			   &data);
	if (path_1 != NULL)
		go_path_interpret (path_1,
				   GO_PATH_DIRECTION_BACKWARD,
				   fill_path_line_to,
				   fill_path_line_to,
				   fill_path_curve_to,
				   fill_path_close_path,
				   &data);
	cairo_close_path (rend->cairo);
}

void
gog_renderer_stroke_serie (GogRenderer *renderer,
			   GOPath const *path)
{
	GOStyle const *style;
	GOPathOptions line_options;
	double width;

	g_return_if_fail (GOG_IS_RENDERER (renderer));
	g_return_if_fail (renderer->cur_style != NULL);
	g_return_if_fail (GO_IS_PATH (path));

        style = renderer->cur_style;
	line_options = go_path_get_options (path);
	width = _grc_line_size (renderer,
				style->line.width,
				line_options & GO_PATH_OPTIONS_SNAP_WIDTH);

	if (go_style_is_line_visible (style)) {
		path_interpret (renderer, path, width);
		emit_line (renderer, FALSE, go_path_get_options (path));
	}
}

void
gog_renderer_fill_serie (GogRenderer *renderer,
			 GOPath const *path,
			 GOPath const *close_path)
{
	GOStyle const *style;

	g_return_if_fail (GOG_IS_RENDERER (renderer));
	g_return_if_fail (renderer->cur_style != NULL);
	g_return_if_fail (GO_IS_PATH (path));

	style = renderer->cur_style;

	if (go_style_is_fill_visible (style)) {
		fill_path_interpret (renderer, path, close_path);
		emit_fill (renderer, FALSE);
	}
}

static void
_draw_shape (GogRenderer *renderer, GOPath const *path, gboolean fill, gboolean stroke)
{
	GOStyle const *style;
	GOPathOptions line_options;
	double width;

	g_return_if_fail (GOG_IS_RENDERER (renderer));
	g_return_if_fail (renderer->cur_style != NULL);
	g_return_if_fail (GO_IS_PATH (path));

	style = renderer->cur_style;

	line_options = go_path_get_options (path);
	width = stroke ? _grc_line_size (renderer, style->line.width,
					 line_options & GO_PATH_OPTIONS_SNAP_WIDTH) : 0;

	path_interpret (renderer, path, width);

	if (fill)
		emit_fill (renderer, stroke);

	if (stroke)
		emit_line (renderer, FALSE, go_path_get_options (path));
}

void
gog_renderer_draw_shape (GogRenderer *renderer,
			 GOPath const *path)
{
	_draw_shape (renderer, path, TRUE, TRUE);
}

void
gog_renderer_stroke_shape (GogRenderer *renderer, GOPath const *path)
{
	_draw_shape (renderer, path, FALSE, TRUE);
}

void
gog_renderer_fill_shape (GogRenderer *renderer, GOPath const *path)
{
	_draw_shape (renderer, path, TRUE, FALSE);
}

static void
_draw_rotated_shape (GogRenderer *renderer, GOPath const *path, gboolean fill, gboolean stroke, gboolean rotate_bg)
{
	GOStyle const *style;
	GOPathOptions line_options;
	double width;

	g_return_if_fail (GOG_IS_RENDERER (renderer));
	g_return_if_fail (renderer->cur_style != NULL);
	g_return_if_fail (GO_IS_PATH (path));

        style = renderer->cur_style;

	line_options = go_path_get_options (path);
	width = stroke ? _grc_line_size (renderer, style->line.width,
					 line_options & GO_PATH_OPTIONS_SNAP_WIDTH) : 0;

	cairo_save (renderer->cairo);
	cairo_rotate (renderer->cairo, -renderer->cur_style->text_layout.angle *M_PI / 180.);
	path_interpret (renderer, path, width);

	if (fill) {
		if (rotate_bg) {
			emit_fill (renderer, stroke);
			cairo_restore (renderer->cairo);
		} else {
			cairo_restore (renderer->cairo);
			emit_fill (renderer, stroke);
		}
	} else
		cairo_restore (renderer->cairo);

	if (stroke)
		emit_line (renderer, FALSE, go_path_get_options (path));
}

/*****************************************************************************/

/**
 * gog_renderer_push_clip:
 * @rend: #GogRenderer
 * @path: a #GOPath
 *
 * Defines the current clipping region.
 **/

void
gog_renderer_push_clip (GogRenderer *rend, GOPath const *path)
{
	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (GO_IS_PATH (path));

	cairo_save (rend->cairo);

	path_interpret (rend, path, 0);

	cairo_clip (rend->cairo);
}

/**
 * gog_renderer_push_clip_rectangle:
 * @rend: #GogRenderer
 * @x: left coordinate
 * @y: top coordinate
 * @w: width of clipping rectangle
 * @h: height of clipping rectangle
 *
 * Defines a rectangular clipping region. For efficient screen rendering,
 * this function takes care to round the coordinates.
 **/

void
gog_renderer_push_clip_rectangle (GogRenderer *rend, double x, double y, double w, double h)
{
	GOPath *path;

	path = go_path_new ();
	if (rend->is_vector)
		go_path_rectangle (path, x, y, w, h);
	else {
		double xx = go_fake_floor (x);
		double yy = go_fake_floor (y);
		go_path_rectangle (path, xx, yy,
				   go_fake_ceil (x + w) - xx,
				   go_fake_ceil (y + h) - yy);
	}
	gog_renderer_push_clip (rend, path);
	go_path_free (path);
}

/**
 * gog_renderer_pop_clip:
 * @rend: #GogRenderer
 *
 * End the current clipping.
 **/

void
gog_renderer_pop_clip (GogRenderer *rend)
{
	g_return_if_fail (GOG_IS_RENDERER (rend));

	cairo_restore (rend->cairo);
}

static void
_draw_circle (GogRenderer *rend, double x, double y, double r, gboolean fill, gboolean stroke)
{
	GOStyle const *style;
	GOPath *path;
	gboolean narrow = r < 1.5;
	double o, o_2;

	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (GO_IS_STYLE (rend->cur_style));

	style = rend->cur_style;
	narrow |= !go_style_is_outline_visible (style);

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);

	if (!narrow) {
		o = gog_renderer_line_size (rend, style->line.width);
		o_2 = o / 2.;
	} else
		o = o_2 = 0.;

	go_path_arc (path, x, y, r, r, 0, 2 * M_PI);

	_draw_shape (rend, path, fill, stroke && !narrow);

	go_path_free (path);
}

void
gog_renderer_draw_circle (GogRenderer *rend, double x, double y, double r)
{
	_draw_circle (rend, x, y, r, TRUE, TRUE);
}

void
gog_renderer_stroke_circle (GogRenderer *rend, double x, double y, double r)
{
	_draw_circle (rend, x, y, r, FALSE, TRUE);
}

void
gog_renderer_fill_circle (GogRenderer *rend, double x, double y, double r)
{
	_draw_circle (rend, x, y, r, TRUE, FALSE);
}

/**
 * gog_renderer_draw_rectangle:
 * @rend: a #GogRenderer
 * @rect: position and extent of rectangle
 *
 * A utility routine to build a closed rectangle vpath.
 *
 **/

static void
_draw_rectangle (GogRenderer *rend, GogViewAllocation const *rect, gboolean fill, gboolean stroke)
{
	GOStyle const *style;
	GOPath *path;
	gboolean narrow = (rect->w < 3.) || (rect->h < 3.);
	double o, o_2;

	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (GO_IS_STYLE (rend->cur_style));

	style = rend->cur_style;
	narrow |= !go_style_is_outline_visible (style);

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);

	if (!narrow) {
		o = gog_renderer_line_size (rend, style->line.width);
		o_2 = o / 2.;
	} else
		o = o_2 = 0.;

	go_path_rectangle (path, rect->x + o_2, rect->y + o_2, rect->w - o, rect->h - o);

	_draw_shape (rend, path, fill, stroke && !narrow);

	go_path_free (path);
}

void
gog_renderer_draw_rectangle (GogRenderer *rend, GogViewAllocation const *rect)
{
	_draw_rectangle (rend, rect, TRUE, TRUE);
}

void
gog_renderer_stroke_rectangle (GogRenderer *rend, GogViewAllocation const *rect)
{
	_draw_rectangle (rend, rect, FALSE, TRUE);
}

void
gog_renderer_fill_rectangle (GogRenderer *rend, GogViewAllocation const *rect)
{
	_draw_rectangle (rend, rect, TRUE, FALSE);
}

static void
_draw_rotated_rectangle (GogRenderer *rend, GogViewAllocation const *rect, gboolean fill, gboolean stroke, gboolean rotate_bg)
{
	GOStyle const *style;
	GOPath *path;
	gboolean narrow = (rect->w < 3.) || (rect->h < 3.);
	double o, o_2;

	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (GO_IS_STYLE (rend->cur_style));

	style = rend->cur_style;
	narrow |= !go_style_is_outline_visible (style);

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);

	if (!narrow) {
		o = gog_renderer_line_size (rend, style->line.width);
		o_2 = o / 2.;
	} else
		o = o_2 = 0.;

	go_path_rectangle (path, 0., 0., rect->w - o, rect->h - o);

	cairo_save (rend->cairo);
	cairo_translate (rend->cairo, rect->x - o_2, rect->y - o_2);
	_draw_rotated_shape (rend, path, fill, stroke && !narrow, rotate_bg);
	cairo_restore (rend->cairo);

	go_path_free (path);
}

void
gog_renderer_draw_rotated_rectangle (GogRenderer *rend, GogViewAllocation const *rect, gboolean rotate_bg)
{
	_draw_rotated_rectangle (rend, rect, TRUE, TRUE, rotate_bg);
}

/**
 * gog_renderer_draw_grip:
 * @renderer: #GogRenderer
 * @x: x position of grip
 * @y: y position of grip
 *
 * Draw a grip, used for moving/resizing of objects.
 **/

void
gog_renderer_draw_grip (GogRenderer *renderer, double x, double y)
{
	GogViewAllocation rectangle;

	if (renderer->grip_style == NULL) {
		GOStyle *style;

		style = go_style_new ();
		style->line.dash_type = GO_LINE_SOLID;
		style->line.width = 0.0;
		style->line.color =
		style->fill.pattern.back = 0xff000080;
		style->fill.pattern.pattern = GO_PATTERN_SOLID;
		style->fill.type = GO_STYLE_FILL_PATTERN;
		style->interesting_fields = GO_STYLE_FILL | GO_STYLE_OUTLINE;

		renderer->grip_style = style;
	}

	rectangle.x = x - GOG_RENDERER_GRIP_SIZE;
	rectangle.y = y - GOG_RENDERER_GRIP_SIZE;
	rectangle.w = rectangle.h = 2 * GOG_RENDERER_GRIP_SIZE;

	gog_renderer_push_style (renderer, renderer->grip_style);

	gog_renderer_draw_rectangle (renderer, &rectangle);

	gog_renderer_pop_style (renderer);
}

void
gog_renderer_draw_selection_rectangle (GogRenderer *renderer, GogViewAllocation const *rectangle)
{
	if (renderer->selection_style == NULL) {
		GOStyle *style;

		style = go_style_new ();
		style->line.dash_type = GO_LINE_DOT;
		style->line.width = 0.0;
		style->line.color = 0x0000ffB0;
		style->fill.type = GO_STYLE_FILL_NONE;
		style->interesting_fields = GO_STYLE_OUTLINE;

		renderer->selection_style = style;
	}

	gog_renderer_push_style (renderer, renderer->selection_style);

	gog_renderer_draw_rectangle (renderer, rectangle);

	gog_renderer_pop_style (renderer);
}

static cairo_surface_t *
_get_marker_surface (GogRenderer *rend)
{
	GOMarker *marker = rend->marker;
	double width;

	if (rend->marker_surface != NULL)
		return rend->marker_surface;

	rend->marker_surface = go_marker_create_cairo_surface (marker, rend->cairo, rend->scale, &width, NULL);

	rend->marker_offset = width * 0.5;
	return rend->marker_surface;
}

/**
 * gog_renderer_draw_marker:
 * @rend: #GogRenderer
 * @x: X-coordinate
 * @y: Y-coordinate
 **/
void
gog_renderer_draw_marker (GogRenderer *rend, double x, double y)
{
	cairo_surface_t *surface;

	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	if (rend->marker == NULL) {
		if (rend->cur_style->marker.auto_fill_color &&
		    !go_marker_is_closed_shape (rend->cur_style->marker.mark)) {
			rend->marker = go_marker_dup (rend->cur_style->marker.mark);
			go_marker_set_fill_color (rend->marker, 0);
		} else
			rend->marker = g_object_ref (rend->cur_style->marker.mark);
	}

	if (rend->is_vector && !rend->marker_as_surface) {
		go_marker_render (rend->marker, rend->cairo,
				  x, y, rend->scale);
		return;
	}

	surface = _get_marker_surface (rend);

	if (surface == NULL)
		return;

	if (rend->is_vector)
		cairo_set_source_surface (rend->cairo, surface,
					  x - rend->marker_offset,
					  y - rend->marker_offset);
	else
		cairo_set_source_surface (rend->cairo, surface,
					  floor (x - rend->marker_offset),
					  floor (y - rend->marker_offset));

	cairo_paint (rend->cairo);
}

/**
 * GoJustification:
 * @GO_JUSTIFY_LEFT: The text is placed at the left edge of the label.
 * @GO_JUSTIFY_RIGHT: The text is placed at the right edge of the label.
 * @GO_JUSTIFY_CENTER: The text is placed in the center of the label.
 * @GO_JUSTIFY_FILL: The text is placed is distributed across the label.
 *
 * Used for justifying the text inside multiline label.
 */

/**
 * gog_renderer_draw_gostring:
 * @rend: #GogRenderer
 * @str: the #GOString to draw
 * @pos: #GogViewAllocation
 * @anchor: #GOAnchorType how to draw relative to @pos
 * @justification: #GoJustification for multiline string.
 * @width: if positive, the maximum width to get a multiline string if needed.
 *
 * Have @rend draw @layout in the at @pos.{x,y} anchored by the @anchor corner.
 * If @pos.w or @pos.h are >= 0 then clip the results to less than that size.
 **/

void
gog_renderer_draw_gostring (GogRenderer *rend, GOString *str,
			    GogViewAllocation const *pos, GOAnchorType anchor,
                            GoJustification justification, double width)
{
	PangoLayout *layout;
	PangoContext *context;
	cairo_t *cairo;
	GOGeometryOBR obr;
	GOGeometryAABR aabr;
	GOStyle const *style;
	int iw, ih;
	PangoAttrList *attr;

	g_return_if_fail (str != NULL);
	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	cairo = rend->cairo;
	style = rend->cur_style;

	/* Note: orig layout may not have been created using cairo! */
	layout = pango_cairo_create_layout (cairo);
	context = pango_layout_get_context (layout);
	pango_layout_set_text (layout, str->str, -1);
	if (width > 0)
		pango_layout_set_width (layout, width * PANGO_SCALE / rend->scale);
	switch (justification) {
	case GO_JUSTIFY_CENTER:
		pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
		break;
	case GO_JUSTIFY_LEFT:
		pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
		break;
	case GO_JUSTIFY_RIGHT:
		pango_layout_set_alignment (layout, PANGO_ALIGN_RIGHT);
		break;
	case GO_JUSTIFY_FILL:
		pango_layout_set_justify (layout, TRUE);
		break;
	}
	attr = go_string_get_markup (str);
	if (attr) {
		pango_layout_set_attributes (layout, attr);
		go_pango_translate_layout (layout);
	}
	pango_cairo_context_set_resolution (context, 72.0);
	pango_layout_set_font_description (layout, style->font.font->desc);
	pango_layout_get_size (layout, &iw, &ih);

	obr.w = rend->scale * ((double) iw + (double) PANGO_SCALE / 2.0)
		/ (double) PANGO_SCALE;
	obr.h = rend->scale * ((double) ih + (double) PANGO_SCALE / 2.0)
		/(double) PANGO_SCALE;
	obr.alpha = -style->text_layout.angle * M_PI / 180.0;
	obr.x = pos->x;
	obr.y = pos->y;
	go_geometry_OBR_to_AABR (&obr, &aabr);

	switch (anchor) {
		case GO_ANCHOR_NW: case GO_ANCHOR_W: case GO_ANCHOR_SW:
			obr.x += aabr.w / 2.0;
			break;
		case GO_ANCHOR_NE : case GO_ANCHOR_SE : case GO_ANCHOR_E :
			obr.x -= aabr.w / 2.0;
			break;
		default : break;
	}

	switch (anchor) {
		case GO_ANCHOR_NW: case GO_ANCHOR_N: case GO_ANCHOR_NE:
			obr.y += aabr.h / 2.0;
			break;
		case GO_ANCHOR_SE : case GO_ANCHOR_S : case GO_ANCHOR_SW :
			obr.y -= aabr.h / 2.0;
			break;
		default : break;
	}

	cairo_save (cairo);
	cairo_set_source_rgba (cairo, GO_COLOR_TO_CAIRO (style->font.color));
	cairo_move_to (cairo, obr.x - (obr.w / 2.0) * cos (obr.alpha) +
		       (obr.h / 2.0) * sin (obr.alpha),
		       obr.y - (obr.w / 2.0) * sin (obr.alpha) -
		       (obr.h / 2.0) * cos (obr.alpha));
	cairo_rotate (cairo, obr.alpha);
	cairo_scale (cairo, rend->scale, rend->scale);
	pango_cairo_show_layout (cairo, layout);
	cairo_restore (cairo);
	g_object_unref (layout);
}


/**
 * gog_renderer_draw_text:
 * @rend: #GogRenderer
 * @text: the string to draw
 * @pos: #GogViewAllocation
 * @anchor: #GOAnchorType how to draw relative to @pos
 * @use_markup: wether to use pango markup
 * @justification: #GoJustification for multiline text.
 * @width: if positive, the maximum width to get a multiline text if needed.
 *
 * Have @rend draw @text in the at @pos.{x,y} anchored by the @anchor corner.
 * If @pos.w or @pos.h are >= 0 then clip the results to less than that size.
 **/

void
gog_renderer_draw_text (GogRenderer *rend, char const *text,
			GogViewAllocation const *pos, GOAnchorType anchor,
			gboolean use_markup, GoJustification justification,
                        double width)
{
	GOString *str;
	PangoAttrList *attr_list = NULL;
	char *m_text = NULL;

	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (text != NULL);

	if (*text == '\0')
		return;

	if (use_markup && pango_parse_markup  (text, -1, 0,
					       &attr_list, &m_text,
					       NULL, NULL))
		str = go_string_new_rich_nocopy (m_text, -1, attr_list, NULL);
	else
		str = go_string_new (text);
	gog_renderer_draw_gostring (rend, str, pos, anchor, justification, width);
	go_string_unref (str);
}

/**
 * gog_renderer_get_gostring_OBR:
 * @rend: #GogRenderer
 * @str: the string to draw
 * @obr: #GOGeometryOBR to store the Object Bounding Rectangle of @text.
 * @max_width: maximum width or -1 for unrestricted
 **/
void
gog_renderer_get_gostring_OBR (GogRenderer *rend, GOString *str,
                               GOGeometryOBR *obr, double max_width)
{
	GOStyle const *style;
	PangoLayout *layout;
	PangoContext *context;
	PangoRectangle logical;
	cairo_t *cairo;
	PangoAttrList *attr;

	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);
	g_return_if_fail (obr != NULL);

	cairo = rend->cairo;
	obr->x = obr->y = 0;
	if (str->str == NULL || *(str->str) == '\0') {
		/* Make sure invisible things don't skew size */
		obr->w = obr->h = 0;
		obr->alpha = 0;
		return;
	}

	style = rend->cur_style;
	layout = pango_cairo_create_layout (cairo);
	context = pango_layout_get_context (layout);
	pango_layout_set_text (layout, str->str, -1);
	if (max_width > 0)
		pango_layout_set_width (layout, max_width * PANGO_SCALE / rend->scale);
	attr = go_string_get_markup (str);
	if (attr) {
		pango_layout_set_attributes (layout, attr);
		go_pango_translate_layout (layout);
	}
	pango_cairo_context_set_resolution (context, 72.0);
	pango_layout_set_font_description (layout, style->font.font->desc);
	pango_layout_get_extents (layout, NULL, &logical);
	g_object_unref (layout);

	obr->w = rend->scale * ((double) logical.width + (double) PANGO_SCALE / 2.0)
		/ (double) PANGO_SCALE;
	obr->h = rend->scale * ((double) logical.height + (double) PANGO_SCALE / 2.0)
		/(double) PANGO_SCALE;

	/* Make sure invisible things don't skew size */
	if (obr->w == 0)
		obr->h = 0;
	else if (obr->h == 0)
		obr->w = 0;

	obr->alpha = - style->text_layout.angle * M_PI / 180.0;
}


/**
 * gog_renderer_get_text_OBR:
 * @rend: #GogRenderer
 * @text: the string to draw
 * @use_markup: wether to use pango markup
 * @obr: #GOGeometryOBR to store the Object Bounding Rectangle of @text.
 * @max_width: maximum width or -1 for unrestricted
 **/
void
gog_renderer_get_text_OBR (GogRenderer *rend, char const *text,
			   gboolean use_markup, GOGeometryOBR *obr,
                           double max_width)
{
	GOString *str;
	PangoAttrList *attr_list = NULL;
	char *m_text = NULL;

	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (text != NULL);

	if (use_markup && pango_parse_markup  (text, -1, 0,
					       &attr_list, &m_text,
					       NULL, NULL))
		str = go_string_new_rich_nocopy (m_text, -1, attr_list, NULL);
	else
		str = go_string_new (text);
	gog_renderer_get_gostring_OBR (rend, str, obr, max_width);
	go_string_unref (str);

}

/**
 * gog_renderer_get_text_AABR:
 * @rend: #GogRenderer
 * @text: the string to draw
 * @use_markup: whether to use pango markup
 * @aabr: #GOGeometryAABR to store the Axis Aligned Bounding Rectangle of @text.
 * @max_width: maximum width or -1 for unrestricted
 **/
void
gog_renderer_get_text_AABR (GogRenderer *rend, char const *text,
			    gboolean use_markup, GOGeometryAABR *aabr,
                            double max_width)
{
	GOGeometryOBR obr;

	gog_renderer_get_text_OBR (rend, text, use_markup, &obr, max_width);
	go_geometry_OBR_to_AABR (&obr, aabr);
}

/**
 * gog_renderer_get_gostring_AABR:
 * @rend: #GogRenderer
 * @str: the string to draw
 * @aabr: #GOGeometryAABR to store the Axis Aligned Bounding Rectangle of @text.
 * @max_width: maximum width or -1 for unrestricted
 **/
void
gog_renderer_get_gostring_AABR (GogRenderer *rend, GOString *str,
				GOGeometryAABR *aabr, double max_width)
{
	GOGeometryOBR obr;

	gog_renderer_get_gostring_OBR (rend, str, &obr, max_width);
	go_geometry_OBR_to_AABR (&obr, aabr);
}

void
gog_renderer_draw_data_label (GogRenderer *rend, GogSeriesLabelElt const *elt,
                              GogViewAllocation const *pos, GOAnchorType anchor,
                              GOStyle *legend_style)
{
	/* things are a bit different from gog_renderer_draw_gostring, so the
	 * code must be copied */
	PangoLayout *layout;
	PangoContext *context;
	cairo_t *cairo;
	GOGeometryOBR obr;
	GOGeometryAABR aabr;
	GOStyle const *style;
	int iw, ih;
	PangoAttrList *attrs;
	PangoRectangle rect, ir, lr;
	GogViewAllocation rectangle;
	double w;

	g_return_if_fail (elt != NULL && elt->str != NULL);
	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (rend->cur_style != NULL);

	cairo = rend->cairo;
	style = (GO_IS_STYLED_OBJECT (elt->point))?
		go_styled_object_get_style (GO_STYLED_OBJECT (elt->point)):
		rend->cur_style;

	/* Note: orig layout may not have been created using cairo! */
	layout = pango_cairo_create_layout (cairo);
	pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
	context = pango_layout_get_context (layout);
	pango_layout_set_text (layout, elt->str->str, -1);
	attrs = pango_attr_list_copy (go_string_get_markup (elt->str));
	pango_layout_set_font_description (layout, style->font.font->desc);
	if (attrs)
		pango_layout_set_attributes (layout, attrs);
	pango_cairo_context_set_resolution (context, 72.0);
	if (elt->legend_pos >= 0) {
		/* we need to add enough room to draw the legend entry */
		PangoAttribute *attr;
		rect.x = rect.y = 0;
		pango_layout_get_size (layout, &iw, &ih);
		rect.height = 1; /* only the width is important */
		rect.width = (legend_style->interesting_fields & GO_STYLE_LINE)? 2 * ih: ih;
		attr = pango_attr_shape_new (&rect, &rect);
		attr->start_index = elt->legend_pos;
		attr->end_index = elt->legend_pos + 1;
		pango_attr_list_insert (attrs, attr);
		pango_layout_set_attributes (layout, attrs);
	}
	/*now get the real size */
	pango_layout_get_extents (layout, &ir, &lr);
	iw = lr.width;
	ih = lr.height;
	pango_attr_list_unref (attrs);

	obr.w = rend->scale * ((double) iw + (double) PANGO_SCALE / 2.0)
		/ (double) PANGO_SCALE;
	obr.h = rend->scale * ((double) ih + (double) PANGO_SCALE / 2.0)
		/(double) PANGO_SCALE;
	obr.alpha = -style->text_layout.angle * M_PI / 180.0;
	obr.x = pos->x;
	obr.y = pos->y;
	go_geometry_OBR_to_AABR (&obr, &aabr);

	w = ((style->interesting_fields | GO_STYLE_LINE) && (style->line.width > 0.))?
			gog_renderer_line_size (rend, style->line.width): 1.; 
	switch (anchor) {
		case GO_ANCHOR_NW: case GO_ANCHOR_W: case GO_ANCHOR_SW:
			obr.x += aabr.w / 2.0 + w;
			break;
		case GO_ANCHOR_NE : case GO_ANCHOR_SE : case GO_ANCHOR_E :
			obr.x -= aabr.w / 2.0 + w;
			break;
		default : break;
	}

	switch (anchor) {
		case GO_ANCHOR_NW: case GO_ANCHOR_N: case GO_ANCHOR_NE:
			obr.y += aabr.h / 2.0 + w;
			break;
		case GO_ANCHOR_SE : case GO_ANCHOR_S : case GO_ANCHOR_SW :
			obr.y -= aabr.h / 2.0 + w;
			break;
		default : break;
	}

	cairo_save (cairo);
	cairo_set_source_rgba (cairo, GO_COLOR_TO_CAIRO (style->font.color));
	cairo_translate (cairo, obr.x - (obr.w / 2.0) * cos (obr.alpha) +
		       (obr.h / 2.0) * sin (obr.alpha),
		       obr.y - (obr.w / 2.0) * sin (obr.alpha) -
		       (obr.h / 2.0) * cos (obr.alpha));
	cairo_rotate (cairo, obr.alpha);

	/* draw outline and background if needed */
	gog_renderer_push_style (rend, style);
	if (style->interesting_fields & (GO_STYLE_FILL | GO_STYLE_LINE)) {
		GOPath *path = go_path_new ();
		double	dx = ((double) ir.width / PANGO_SCALE + 2.) * rend->scale,
				dy = ((double) ir.height / PANGO_SCALE + 2.) * rend->scale,
				x0 = (double) ir.x / PANGO_SCALE - 1.,
				y0 = (double) ir.y / PANGO_SCALE - 1.;
		go_path_move_to (path, x0 - w / 2., y0 - w / 2.);
		go_path_line_to (path, x0 + dx + w / 2., y0 - w / 2.);
		go_path_line_to (path, x0 + dx + w / 2., y0 + dy + w / 2.);
		go_path_line_to (path, x0 - w / 2., y0 + dy + w / 2.);
		go_path_close (path);
		cairo_set_line_join (cairo, CAIRO_LINE_JOIN_MITER); /* probably not the right place */
		gog_renderer_draw_shape (rend, path);
		go_path_free (path);
	}
	/* now draw the legend entry if needed */
	if (elt->legend_pos >= 0 && legend_style != NULL) {
		GOStyle *style = go_style_dup (legend_style);
		double x, y,w, h;
		pango_layout_index_to_pos (layout, elt->legend_pos, &rect);
		x = (double) rect.x / PANGO_SCALE * rend->scale;
		y = (double) rect.y / PANGO_SCALE * rend->scale;
		w = (double) rect.width / PANGO_SCALE * rend->scale;
		h = (double) rect.height / PANGO_SCALE * rend->scale;
		if (style->interesting_fields & GO_STYLE_LINE) { /* line and marker */
			GOPath *line_path;
			if (style->line.width > h / 3.)
				style->line.width = h / 3.;
			if (go_marker_get_size (style->marker.mark) > h)
				go_marker_set_size (style->marker.mark, h);
			gog_renderer_push_style (rend, style);
			line_path = go_path_new ();
			y += h / 2.;
			go_path_move_to (line_path, x, y);
			go_path_line_to (line_path, x + w, y);
			if (style->interesting_fields & GO_STYLE_FILL) {
				rectangle.x = x;
				rectangle.y = y;
				rectangle.w = w;
				rectangle.h = h / 2.0;
				gog_renderer_fill_rectangle (rend, &rectangle);
			}
			gog_renderer_stroke_serie (rend, line_path);
			go_path_free (line_path);
			gog_renderer_draw_marker (rend, x + w / 2., y);
		} else if (style->interesting_fields & GO_STYLE_FILL) {/* area */
			if (style->line.width > h / 3.)
				style->line.width = h / 3.;

			rectangle.x = x;
			rectangle.y = y;
			rectangle.w = w;
			rectangle.h = h;
			gog_renderer_push_style (rend, style);
			gog_renderer_draw_rectangle (rend, &rectangle);
		} else if (style->interesting_fields & GO_STYLE_MARKER) {					/* markers only */
			if (go_marker_get_size (style->marker.mark) > h)
				go_marker_set_size (style->marker.mark, h);
			gog_renderer_push_style (rend, style);
			gog_renderer_draw_marker (rend, x + w / 2., y + h / 2.);
		} else
			gog_renderer_push_style (rend, style); // just to be sure
		gog_renderer_pop_style (rend);
		g_object_unref (style);
	}
	cairo_scale (cairo, rend->scale, rend->scale);
	cairo_set_source_rgba (cairo, GO_COLOR_TO_CAIRO (style->font.color));
	pango_cairo_show_layout (cairo, layout);
	cairo_restore (cairo);
	gog_renderer_pop_style (rend);
	g_object_unref (layout);
}

static void
_free_marker_data (GogRenderer *rend)
{
	if (rend->marker_surface != NULL) {
		cairo_surface_destroy (rend->marker_surface);
		rend->marker_surface = NULL;
	}
	if (rend->marker) {
		g_object_unref (rend->marker);
		rend->marker = NULL;
	}
}

void
gog_renderer_draw_color_map (GogRenderer *rend, GogAxisColorMap const *map,
                             int discrete, gboolean horizontal,
                             GogViewAllocation const *rect)
{
	cairo_save (rend->cairo);
	cairo_translate (rend->cairo, rect->x, rect->y);
	gog_axis_color_map_to_cairo (map, rend->cairo, discrete, horizontal,
	                             rect->w, rect->h);
	cairo_restore (rend->cairo);
}

void
gog_renderer_push_style (GogRenderer *rend, GOStyle const *style)
{
	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (GO_IS_STYLE (style));

	if (rend->cur_style != NULL)
		rend->style_stack = g_slist_prepend (
			rend->style_stack, (gpointer)rend->cur_style);
	g_object_ref ((gpointer)style);
	rend->cur_style = style;

	_free_marker_data (rend);

	_update_dash (rend);
}

void
gog_renderer_pop_style (GogRenderer *rend)
{
	g_return_if_fail (GOG_IS_RENDERER (rend));
	g_return_if_fail (GO_IS_STYLE (rend->cur_style));

	g_object_unref ((gpointer)rend->cur_style);
	if (rend->style_stack != NULL) {
		rend->cur_style = rend->style_stack->data;
		rend->style_stack = g_slist_remove (rend->style_stack,
			rend->cur_style);
	} else
		rend->cur_style = NULL;

	_free_marker_data (rend);

	_update_dash (rend);
}

void
gog_renderer_request_update (GogRenderer *renderer)
{
	g_return_if_fail (GOG_IS_RENDERER (renderer));

	if (renderer->needs_update)
		return;
	renderer->needs_update = TRUE;
	g_signal_emit (G_OBJECT (renderer),
		renderer_signals [RENDERER_SIGNAL_REQUEST_UPDATE], 0);
}

/**
 * gog_renderer_update:
 * @renderer: a #GogRenderer
 * @w: requested width
 * @h: requested height
 *
 * Requests a renderer update, only useful for pixbuf based renderer.
 *
 * Returns: %TRUE if a redraw is necessary.
 **/
gboolean
gog_renderer_update (GogRenderer *rend, double w, double h)
{
	GogGraph *graph;
	GogView *view;
	GogViewAllocation allocation;
	gboolean redraw = TRUE;
	gboolean size_changed;

	if (w <= 0 || h <= 0)
		return FALSE;
	g_return_val_if_fail (GOG_IS_RENDERER (rend), FALSE);
	g_return_val_if_fail (GOG_IS_VIEW (rend->view), FALSE);

	size_changed = rend->w != w || rend->h != h;
	if (size_changed) {
		if (rend->cairo_surface != NULL) {
			cairo_surface_destroy (rend->cairo_surface);
			rend->cairo_surface = NULL;
		}

		if (w == 0 || h == 0)
			return FALSE;

		rend->w = w;
		rend->h = h;

		rend->cairo_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, rend->w, rend->h);
	}

	view = rend->view;
	graph = GOG_GRAPH (view->model);
	gog_graph_force_update (graph);

	allocation.x = allocation.y = 0.;
	allocation.w = rend->w;
	allocation.h = rend->h;

	rend->cairo = cairo_create (rend->cairo_surface);

	/* we need to update scale even if size did not change since graph size might have changed (#599887) */
	rend->scale_x = (graph->width >= 1.) ? (rend->w / graph->width) : 1.;
	rend->scale_y = (graph->height >= 1.) ? (rend->h / graph->height) : 1.;
	rend->scale = MIN (rend->scale_x, rend->scale_y);

	if (size_changed) {
		/* make sure we dont try to queue an update while updating */
		rend->needs_update = TRUE;

		/* scale just changed need to recalculate sizes */
		gog_renderer_request_update (rend);
		gog_view_size_allocate (view, &allocation);
	} else
		if (rend->w != view->allocation.w || rend->h != view->allocation.h)
			gog_view_size_allocate (view, &allocation);
		else
			redraw = gog_view_update_sizes (view);

	redraw |= rend->needs_update;
	rend->needs_update = FALSE;

	if (redraw) {
		if (rend->pixbuf) {
			g_object_unref (rend->pixbuf);
			rend->pixbuf = NULL;
		}

		if (!size_changed) {
			cairo_set_operator (rend->cairo, CAIRO_OPERATOR_CLEAR);
			cairo_paint (rend->cairo);
		}

		cairo_set_operator (rend->cairo, CAIRO_OPERATOR_OVER);

		cairo_set_line_join (rend->cairo, CAIRO_LINE_JOIN_ROUND);
		cairo_set_line_cap (rend->cairo, CAIRO_LINE_CAP_ROUND);

		rend->is_vector = FALSE;
		gog_view_render	(view, NULL);
	}

	cairo_destroy (rend->cairo);
	rend->cairo = NULL;

	return redraw;
}

/**
 * gog_renderer_get_pixbuf:
 * @renderer: #GogRenderer
 *
 * Returns: (transfer none): current pixbuf buffer from a renderer that can render into a pixbuf.
 * 	or %NULL on error.
 **/
GdkPixbuf *
gog_renderer_get_pixbuf (GogRenderer *rend)
{
	g_return_val_if_fail (GOG_IS_RENDERER (rend), NULL);

	if (rend->cairo_surface == NULL)
		return NULL;

	if (rend->pixbuf == NULL) {
		int width = cairo_image_surface_get_width (rend->cairo_surface);
		int height = cairo_image_surface_get_height (rend->cairo_surface);

		if (width <= 0 || height <= 0)
			rend->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
		else {
			int rowstride = cairo_image_surface_get_stride (rend->cairo_surface);
			unsigned char *data = cairo_image_surface_get_data (rend->cairo_surface);
			rend->pixbuf = gdk_pixbuf_new_from_data (data, GDK_COLORSPACE_RGB, TRUE, 8,
								 width, height, rowstride, NULL, NULL);
			go_cairo_convert_data_to_pixbuf (data, NULL, width, height, rowstride);
		}
	}

	return rend->pixbuf;
}

cairo_surface_t *
gog_renderer_get_cairo_surface (GogRenderer *rend)
{
	g_return_val_if_fail (GOG_IS_RENDERER (rend), NULL);

	return rend->cairo_surface;
}

static gboolean
_gsf_gdk_pixbuf_save (const gchar *buf,
		      gsize count,
		      GError **error,
		      gpointer data)
{
	GsfOutput *output = GSF_OUTPUT (data);
	gboolean ok = gsf_output_write (output, count, buf);

	if (!ok && error)
		*error = g_error_copy (gsf_output_error (output));

	return ok;
}

static cairo_status_t
_cairo_write_func (void *closure,
		   const unsigned char *data,
		   unsigned int length)
{
	gboolean result;
	GsfOutput *output = GSF_OUTPUT (closure);

	result = gsf_output_write (output, length, data);

	return result ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

gboolean
gog_renderer_render_to_cairo (GogRenderer *renderer, cairo_t *cairo, double width, double height)
{
	GogViewAllocation allocation;
	double width_in_pts, height_in_pts;

	g_return_val_if_fail (GOG_IS_RENDERER (renderer), FALSE);
	g_return_val_if_fail (GOG_IS_VIEW (renderer->view), FALSE);
	g_return_val_if_fail (GOG_IS_GRAPH (renderer->model), FALSE);

	gog_graph_force_update (renderer->model);
	gog_graph_get_size (renderer->model, &width_in_pts, &height_in_pts);

	renderer->cairo = cairo;
	renderer->is_vector = go_cairo_surface_is_vector (cairo_get_target (cairo));

	cairo_set_line_join (renderer->cairo, CAIRO_LINE_JOIN_ROUND);
	cairo_set_line_cap (renderer->cairo, CAIRO_LINE_CAP_ROUND);

	renderer->w = width;
	renderer->h = height;

	allocation.x = 0.;
	allocation.y = 0.;
	allocation.w = width;
	allocation.h = height;

	renderer->scale_x = (width_in_pts >= 1.) ? (width / width_in_pts) : 1.;
	renderer->scale_y = (height_in_pts >= 1.) ? (height / height_in_pts) : 1.;
	renderer->scale = MIN (renderer->scale_x, renderer->scale_y);

	gog_view_size_allocate (renderer->view, &allocation);
	gog_view_render	(renderer->view, NULL);

	renderer->cairo = NULL;

	return cairo_status (cairo) == CAIRO_STATUS_SUCCESS;
}

/**
 * gog_renderer_export_image:
 * @renderer: a #GogRenderer
 * @format: image format for export
 * @output: a #GsfOutput stream
 * @x_dpi: x resolution of exported graph
 * @y_dpi: y resolution of exported graph
 *
 * Exports an image of @graph in given @format, writing results in a #GsfOutput stream.
 * If export format type is a bitmap one, it computes image size with x_dpi, y_dpi and
 * @graph size (see gog_graph_get_size()).
 *
 * returns: %TRUE if export succeed.
 **/

gboolean
gog_renderer_export_image (GogRenderer *rend, GOImageFormat format,
			   GsfOutput *output, double x_dpi, double y_dpi)
{
	GOImageFormatInfo const *format_info;
	cairo_t *cairo;
	cairo_surface_t *surface = NULL;
	gboolean status;
	GdkPixbuf *pixbuf;
	GdkPixbuf *output_pixbuf;
	gboolean result;
	double width_in_pts, height_in_pts;

	g_return_val_if_fail (GOG_IS_RENDERER (rend), FALSE);

	if (x_dpi <= 0.)
		x_dpi = 96.;
	if (y_dpi <= 0.)
		y_dpi = 96.;
	gog_graph_force_update (rend->model);

	gog_graph_get_size (rend->model, &width_in_pts, &height_in_pts);

	/* Prevent Cairo from faulting.  */
	width_in_pts = CLAMP (width_in_pts, 1, 32767 * 72.0 / x_dpi);
	height_in_pts = CLAMP (height_in_pts, 1, 32767 * 72.0 / y_dpi);

	switch (format) {
		case GO_IMAGE_FORMAT_EPS:
			rend->marker_as_surface = FALSE;
			surface = cairo_ps_surface_create_for_stream
				(_cairo_write_func,
				 output, width_in_pts, height_in_pts);
			cairo_ps_surface_set_eps (surface, TRUE);
			cairo_surface_set_fallback_resolution (surface, x_dpi, y_dpi);
			goto do_export_vectorial;
		case GO_IMAGE_FORMAT_PDF:
			rend->marker_as_surface = FALSE;
			surface = cairo_pdf_surface_create_for_stream
				(_cairo_write_func,
				 output, width_in_pts, height_in_pts);
			cairo_surface_set_fallback_resolution (surface, x_dpi, y_dpi);
			goto do_export_vectorial;
		case GO_IMAGE_FORMAT_PS:
			rend->marker_as_surface = FALSE;
			surface = cairo_ps_surface_create_for_stream
				(_cairo_write_func,
				 output, width_in_pts, height_in_pts);
			cairo_surface_set_fallback_resolution (surface, x_dpi, y_dpi);
			goto do_export_vectorial;
		case GO_IMAGE_FORMAT_SVG:
#ifdef CAIRO_BUG_63633_FIXED
			rend->marker_as_surface = TRUE;
#else
			rend->marker_as_surface = FALSE;
#endif
			surface = cairo_svg_surface_create_for_stream
				(_cairo_write_func,
				 output, width_in_pts, height_in_pts);
			cairo_surface_set_fallback_resolution (surface, x_dpi, y_dpi);
do_export_vectorial:
			rend->scale = 1.0;
			cairo = cairo_create (surface);
			cairo_surface_destroy (surface);
			status = gog_renderer_render_to_cairo (rend, cairo, width_in_pts, height_in_pts);
			cairo_destroy (cairo);

			return status;

			break;
		default:
			format_info = go_image_get_format_info (format);
			if (!format_info->has_pixbuf_saver) {
				g_warning ("[GogRendererCairo:export_image] unsupported format");
				return FALSE;
			}

			gog_renderer_update (rend, width_in_pts * x_dpi / 72.0, height_in_pts * y_dpi / 72.0);
			pixbuf = gog_renderer_get_pixbuf (rend);
			if (pixbuf == NULL)
				return FALSE;
			format_info = go_image_get_format_info (format);
			if (!format_info->alpha_support)
				output_pixbuf = gdk_pixbuf_composite_color_simple
					(pixbuf,
					 gdk_pixbuf_get_width (pixbuf),
					 gdk_pixbuf_get_height (pixbuf),
					 GDK_INTERP_NEAREST,
					 255, 256, 0xffffffff,
					 0xffffffff);
			else
				output_pixbuf = pixbuf;
			result = gdk_pixbuf_save_to_callback (output_pixbuf,
							      _gsf_gdk_pixbuf_save,
							      output, format_info->name,
							      NULL, NULL);
			if (!format_info->alpha_support)
				g_object_unref (output_pixbuf);
			return result;
	}

	return FALSE;
}

/**
 * gog_renderer_get_hairline_width_pts:
 * @rend: a #GogRenderer
 *
 * Returns: the hairline width in pts.
 **/
double
gog_renderer_get_hairline_width_pts (GogRenderer const *rend)
{
	g_return_val_if_fail (GOG_IS_RENDERER (rend), GOG_RENDERER_HAIRLINE_WIDTH_PTS);

	if (rend->is_vector
	    || go_sub_epsilon (rend->scale) <= 0.0)
	       return GOG_RENDERER_HAIRLINE_WIDTH_PTS;

	return 1.0 / rend->scale;
}

#ifdef GOFFICE_WITH_LASEM

void
gog_renderer_draw_equation (GogRenderer *renderer, LsmDomView *mathml_view, double x, double y)
{
	cairo_t *cairo;
	double w, h, alpha;

	g_return_if_fail (GOG_IS_RENDERER (renderer));
	g_return_if_fail (LSM_IS_DOM_VIEW (mathml_view));
	g_return_if_fail (renderer->cur_style != NULL);

	cairo = renderer->cairo;

	alpha = -renderer->cur_style->text_layout.angle * M_PI / 180.0;
	lsm_dom_view_get_size (LSM_DOM_VIEW (mathml_view), &w, &h, NULL);
	w *= renderer->scale;
	h *= renderer->scale;
	x = x - (w / 2.0) * cos (alpha) + (h / 2.0) * sin (alpha);
	y = y - (w / 2.0) * sin (alpha) - (h / 2.0) * cos (alpha);

	cairo_save (cairo);

	cairo_translate (cairo, x, y);
	cairo_rotate (cairo, alpha);
	cairo_scale (cairo, renderer->scale_x, renderer->scale_y);

	lsm_dom_view_render (LSM_DOM_VIEW (mathml_view), cairo, 0., 0.);

	cairo_restore (cairo);
}

#endif

/**
 * gog_renderer_new:
 * @graph: graph model
 *
 * Returns:  a new #GogRenderer which can render into a pixbuf, and sets @graph
 * 	as its model.
 **/
GogRenderer*
gog_renderer_new (GogGraph *graph)
{
	return g_object_new (GOG_TYPE_RENDERER, "model", graph, NULL);
}

static void
_cb_font_removed (GogRenderer *rend, GOFont const *font)
{
	g_return_if_fail (GOG_IS_RENDERER (rend));

	gog_debug (0, g_warning ("notify a '%s' that %p is invalid",
				 G_OBJECT_TYPE_NAME (rend), font););
}

static void
gog_renderer_init (GogRenderer *rend)
{
	rend->cairo = NULL;
	rend->cairo_surface = NULL;
	rend->marker_surface = NULL;
	rend->w = rend->h = 0;
	rend->is_vector = FALSE;

	rend->line_dash = NULL;

	rend->needs_update = FALSE;
	rend->cur_style    = NULL;
	rend->style_stack  = NULL;
	rend->font_watcher = g_cclosure_new_swap (G_CALLBACK (_cb_font_removed),
		rend, NULL);
	go_font_cache_register (rend->font_watcher);

	rend->grip_style = NULL;
	rend->selection_style = NULL;

	rend->pixbuf = NULL;
}

static void
gog_renderer_finalize (GObject *obj)
{
	GogRenderer *rend = GOG_RENDERER (obj);

	if (rend->pixbuf != NULL) {
		g_object_unref (rend->pixbuf);
		rend->pixbuf = NULL;
	}

	if (rend->cairo_surface != NULL) {
		cairo_surface_destroy (rend->cairo_surface);
		rend->cairo_surface = NULL;
	}

	_free_marker_data (rend);

	go_line_dash_sequence_free (rend->line_dash);
	rend->line_dash = NULL;

	if (rend->grip_style != NULL) {
		g_object_unref (rend->grip_style);
		rend->grip_style = NULL;
	}

	if (rend->selection_style != NULL) {
		g_object_unref (rend->selection_style);
		rend->selection_style = NULL;
	}

	if (rend->cur_style != NULL) {
		g_warning ("Missing calls to gog_renderer_style_pop left dangling style references");
		g_slist_foreach (rend->style_stack,
			(GFunc)g_object_unref, NULL);
		g_slist_free (rend->style_stack);
		rend->style_stack = NULL;
		g_object_unref ((gpointer)rend->cur_style);
		rend->cur_style = NULL;
	}

	if (rend->view != NULL) {
		g_object_unref (rend->view);
		rend->view = NULL;
	}

	if (rend->font_watcher != NULL) {
		go_font_cache_unregister (rend->font_watcher);
		g_closure_unref (rend->font_watcher);
		rend->font_watcher = NULL;
	}

	(*parent_klass->finalize) (obj);
}

static void
gog_renderer_set_property (GObject *obj, guint param_id,
			   GValue const *value, GParamSpec *pspec)
{
	GogRenderer *rend = GOG_RENDERER (obj);

	switch (param_id) {
	case RENDERER_PROP_MODEL:
		rend->model = GOG_GRAPH (g_value_get_object (value));
		if (rend->view != NULL)
			g_object_unref (rend->view);
		rend->view = g_object_new (gog_graph_view_get_type (),
					   "renderer",	rend,
					   "model",	rend->model,
					   NULL);
		gog_renderer_request_update (rend);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_renderer_get_property (GObject *obj, guint param_id,
			   GValue *value, GParamSpec *pspec)
{
	GogRenderer const *rend = GOG_RENDERER (obj);

	switch (param_id) {
	case RENDERER_PROP_MODEL:
		g_value_set_object (value, rend->model);
		break;
	case RENDERER_PROP_VIEW:
		g_value_set_object (value, rend->view);
		break;
	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_renderer_class_init (GogRendererClass *renderer_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *)renderer_klass;

	parent_klass = g_type_class_peek_parent (gobject_klass);
	gobject_klass->finalize	    = gog_renderer_finalize;
	gobject_klass->set_property = gog_renderer_set_property;
	gobject_klass->get_property = gog_renderer_get_property;

	g_object_class_install_property (gobject_klass, RENDERER_PROP_MODEL,
		g_param_spec_object ("model",
			_("Model"),
			_("The GogGraph this renderer displays"),
			GOG_TYPE_GRAPH,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (gobject_klass, RENDERER_PROP_VIEW,
		g_param_spec_object ("view",
			_("View"),
			_("the GogView this renderer is displaying"),
			GOG_TYPE_VIEW,
			GSF_PARAM_STATIC | G_PARAM_READABLE));

	renderer_signals [RENDERER_SIGNAL_REQUEST_UPDATE] = g_signal_new ("request-update",
		G_TYPE_FROM_CLASS (renderer_klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GogRendererClass, request_update),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
}

double
gog_renderer_get_scale (GogRenderer *renderer)
{
	return renderer->scale;
}

GSF_CLASS (GogRenderer, gog_renderer,
	   gog_renderer_class_init, gog_renderer_init,
	   G_TYPE_OBJECT)
