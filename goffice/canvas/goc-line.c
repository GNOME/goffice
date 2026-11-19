/*
 * goc-line.c :
 *
 * Copyright (C) 2009 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/**
 * SECTION:goc-line
 * @short_description: Simple line.
 *
 * #GocLine implements simple line drawing in the canvas. The line can have
 * arrows at the start and/or at the end.
**/

static GocItemClass *parent_class;

enum {
	LINE_PROP_0,
	LINE_PROP_X0,
	LINE_PROP_Y0,
	LINE_PROP_X1,
	LINE_PROP_Y1,
	LINE_PROP_START_ARROW,
	LINE_PROP_END_ARROW
};

static void
goc_line_set_property (GObject *gobject, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GocLine *line = GOC_LINE (gobject);

	switch (param_id) {
	case LINE_PROP_X0:
		line->startx = g_value_get_double (value);
		break;

	case LINE_PROP_Y0:
		line->starty = g_value_get_double (value);
		break;

	case LINE_PROP_X1:
		line->endx = g_value_get_double (value);
		break;

	case LINE_PROP_Y1:
		line->endy = g_value_get_double (value);
		break;

	case LINE_PROP_START_ARROW: {
		GOArrow const *arrow = g_value_get_boxed (value);
		if (arrow != NULL)
			line->start_arrow = *arrow;
		else
			go_arrow_clear (&line->start_arrow);
		break;
	}

	case LINE_PROP_END_ARROW: {
		GOArrow const *arrow = g_value_get_boxed (value);
		if (arrow != NULL)
			line->end_arrow = *arrow;
		else
			go_arrow_clear (&line->end_arrow);
		break;
	}

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));
}

static void
goc_line_get_property (GObject *gobject, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GocLine *line = GOC_LINE (gobject);

	switch (param_id) {
	case LINE_PROP_X0:
		g_value_set_double (value, line->startx);
		break;

	case LINE_PROP_Y0:
		g_value_set_double (value, line->starty);
		break;

	case LINE_PROP_X1:
		g_value_set_double (value, line->endx);
		break;

	case LINE_PROP_Y1:
		g_value_set_double (value, line->endy);
		break;

	case LINE_PROP_START_ARROW:
		g_value_set_boxed (value, &line->start_arrow);
		break;

	case LINE_PROP_END_ARROW:
		g_value_set_boxed (value, &line->end_arrow);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

/* Half of the drawn stroke width in item coordinates.  The pen is circular
 * in user space, so an explicit style width is used as is; hairlines are
 * one device pixel wide.  */
static double
goc_line_half_width (GocItem const *item, GOStyle *style)
{
	double ppu = item->canvas? goc_canvas_get_pixels_per_unit (item->canvas): 1.;

	if (style->line.width > 0.)
		return style->line.width / 2.;
	return .5 / (ppu * MAX (_goc_item_transform_scale (item), 1e-6));
}

/* How far an arrow may stick out of the line box, in item coordinates.
 * Deliberately generous: the tip extent a is covered as well.  */
static double
goc_line_arrow_extra (GOArrow const *arrow)
{
	switch (arrow->typ) {
	case GO_ARROW_NONE:
		return 0.;
	case GO_ARROW_KITE:
		return MAX (arrow->a, hypot (arrow->b, arrow->c));
	case GO_ARROW_OVAL:
		return MAX (arrow->a, arrow->b);
	default:
		g_assert_not_reached ();
	}
	return 0.;
}

/* The arrow silhouette, in the local frame used by go_arrow_draw().  */
static void
goc_line_arrow_path (GOArrow const *arrow, cairo_t *cr)
{
	switch (arrow->typ) {
	case GO_ARROW_NONE:
		break;
	case GO_ARROW_KITE:
		cairo_move_to (cr, 0., 0.);
		cairo_line_to (cr, -arrow->c, -arrow->b);
		cairo_line_to (cr, 0., -arrow->a);
		cairo_line_to (cr, arrow->c, -arrow->b);
		cairo_close_path (cr);
		break;
	case GO_ARROW_OVAL:
		if (arrow->a > 0 && arrow->b > 0) {
			cairo_save (cr);
			cairo_scale (cr, arrow->a, arrow->b);
			cairo_arc (cr, 0., 0., 1., 0., 2. * M_PI);
			cairo_restore (cr);
		}
		break;
	default:
		g_assert_not_reached ();
	}
}

/* The pixel snapping of goc_line_draw: returns the snapped start point and
 * the snapped end offset.  dx is the horizontal delta possibly mirrored by
 * the RTL direction.  */
static void
goc_line_snap (GocLine const *line, GOStyle *style, double dx, double dy,
	       double *startx, double *starty, double *endx, double *endy)
{
	double hoffs, voffs = ceil (style->line.width);

	if (voffs <= 0.)
		voffs = 1.;
	hoffs = ((int) voffs & 1)? .5: 0.;
	voffs = (line->starty == line->endy)? hoffs: 0.;
	if (line->startx != line->endx)
		hoffs = 0.;

	*startx = hoffs + floor (line->startx);
	*starty = voffs + floor (line->starty);
	*endx = (dx > 0.)? ceil (dx): floor (dx);
	*endy = (dy > 0.)? ceil (dy): floor (dy);
}

static void
goc_line_update_bounds (GocItem *item)
{
	GocLine *line = GOC_LINE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_matrix_t m;
	double startx, starty, dx, dy, extra;
	double bx0, bx1, by0, by1;
	double px[4], py[4];
	int i;

	if (line->startx == line->endx && line->starty == line->endy) {
		/* a zero length line paints nothing; keep a small point */
		startx = line->startx;
		starty = line->starty;
		dx = dy = 0.;
	} else {
		/* the same pixel snapping as goc_line_draw, so that the
		 * drawn ink never leaves the bounds */
		goc_line_snap (line, style, line->endx - line->startx,
			       line->endy - line->starty,
			       &startx, &starty, &dx, &dy);
	}

	/* room for the stroke and for the arrows */
	extra = goc_line_half_width (item, style);
	extra += MAX (goc_line_arrow_extra (&line->start_arrow),
		      goc_line_arrow_extra (&line->end_arrow));

	bx0 = MIN (startx, startx + dx) - extra;
	bx1 = MAX (startx, startx + dx) + extra;
	by0 = MIN (starty, starty + dy) - extra;
	by1 = MAX (starty, starty + dy) + extra;
	px[0] = bx0; py[0] = by0;
	px[1] = bx1; py[1] = by0;
	px[2] = bx1; py[2] = by1;
	px[3] = bx0; py[3] = by1;

	/* the item transform turns the box into the item bounds */
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);
	_goc_item_transform (item, cr, FALSE);
	cairo_get_matrix (cr, &m);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	item->x0 = item->y0 = G_MAXDOUBLE;
	item->x1 = item->y1 = -G_MAXDOUBLE;
	for (i = 0; i < 4; i++) {
		double qx = px[i], qy = py[i];
		cairo_matrix_transform_point (&m, &qx, &qy);
		item->x0 = MIN (item->x0, qx);
		item->y0 = MIN (item->y0, qy);
		item->x1 = MAX (item->x1, qx);
		item->y1 = MAX (item->y1, qy);
	}
}

static double
goc_line_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocLine *line = GOC_LINE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double ppu = item->canvas? goc_canvas_get_pixels_per_unit (item->canvas): 1.;
	double k = MAX (_goc_item_transform_scale (item), 1e-6);
	double tol, dx, dy, l, t;
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_matrix_t m;

	*near_item = item;

	/* Query points come in the parent space: test the geometry in item
	 * coordinates.  */
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);
	_goc_item_transform (item, cr, FALSE);
	cairo_get_matrix (cr, &m);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	if (cairo_matrix_invert (&m) != CAIRO_STATUS_SUCCESS)
		return 20;
	cairo_matrix_transform_point (&m, &x, &y);

	dx = line->endx - line->startx;
	dy = line->endy - line->starty;
	l = hypot (dx, dy);
	if (l == 0.) {
		/* degenerate line: just a point, avoid dividing by zero */
		return hypot (x - line->startx, y - line->starty);
	}

	/* the hit tolerance is at least 5 device pixels wide */
	tol = MAX (goc_line_half_width (item, style), 2.5 / (ppu * k));

	/* the arrow heads are filled shapes: hit test them exactly */
	if (line->start_arrow.typ != GO_ARROW_NONE ||
	    line->end_arrow.typ != GO_ARROW_NONE) {
		double phi = atan2 (dy, dx);

		surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
		cr = cairo_create (surface);
		cairo_set_line_width (cr, 2. * tol);
		if (line->start_arrow.typ != GO_ARROW_NONE) {
			cairo_save (cr);
			cairo_translate (cr, line->startx, line->starty);
			cairo_rotate (cr, phi + M_PI / 2.);
			goc_line_arrow_path (&line->start_arrow, cr);
			cairo_restore (cr);
		}
		if (line->end_arrow.typ != GO_ARROW_NONE) {
			cairo_save (cr);
			cairo_translate (cr, line->endx, line->endy);
			cairo_rotate (cr, phi - M_PI / 2.);
			goc_line_arrow_path (&line->end_arrow, cr);
			cairo_restore (cr);
		}
		if (cairo_in_fill (cr, x, y) || cairo_in_stroke (cr, x, y)) {
			cairo_destroy (cr);
			cairo_surface_destroy (surface);
			return 0.;
		}
		cairo_destroy (cr);
		cairo_surface_destroy (surface);
	}

	x -= line->startx;
	y -= line->starty;
	t = (x * dx + y * dy) / l;
	y = (-x * dy + y * dx) / l;
	if (t < 0.)
		return hypot (t, y); /* that might be not fully exact,
	 but we don't need a large precision */
	if (t > l)
		return hypot (t - l, y);
	t = fabs (y) - tol;
	return (t > 0.)? t: 0.;
}

static void
draw_arrow (GOArrow const *arrow, cairo_t *cr, GOStyle *style,
	    double *endx, double *endy, double phi)
{
	double dx, dy;

	if (arrow->typ == GO_ARROW_NONE)
		return;

	cairo_save (cr);
	cairo_translate (cr, *endx, *endy);
	cairo_set_source_rgba (cr, GO_COLOR_TO_CAIRO (style->line.color));
	go_arrow_draw (arrow, cr, &dx, &dy, phi - M_PI / 2);
	*endx += dx;
	*endy += dy;
	cairo_restore (cr);
}

static void
goc_line_draw (GocItem const *item, cairo_t *cr)
{
	GocLine *line = GOC_LINE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double sign = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1: 1;
	double endx, endy, startx, starty;
	double startx_snap, starty_snap;
	double phi;

	if (line->startx == line->endx && line->starty == line->endy)
		return;

	goc_line_snap (line, style, (line->endx - line->startx) * sign,
		       line->endy - line->starty,
		       &startx_snap, &starty_snap, &endx, &endy);

	cairo_save (cr);
	_goc_item_transform (item, cr, TRUE);
	goc_group_cairo_transform (item->parent, cr, startx_snap, starty_snap);

	startx = 0.;
	starty = 0.;
	phi = atan2 (endy, endx);
	draw_arrow (&line->start_arrow, cr, style,
		    &startx, &starty, phi + M_PI);
	draw_arrow (&line->end_arrow, cr, style,
		    &endx, &endy, phi);

	if ((endx != 0. || endy != 0.) &&
	    go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr)) {
		/* try to avoid horizontal and vertical lines between two pixels */
		cairo_move_to (cr, startx, starty);
		cairo_line_to (cr, endx, endy);
		cairo_stroke (cr);
	}
	cairo_restore (cr);
}

static void
goc_line_copy (GocItem *dest, GocItem *source)
{
	GocLine *src = GOC_LINE (source), *dst = GOC_LINE (dest);

	dst->startx = src->startx;
	dst->starty = src->starty;
	dst->endx = src->endx;
	dst->endy = src->endy;
	dst->start_arrow = src->start_arrow;
	dst->end_arrow = src->end_arrow;
	parent_class->copy (dest, source);
}

static void
goc_line_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = GO_COLOR_BLACK;
	if (style->line.auto_fore)
		style->line.fore  = 0;
}

static void
goc_line_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;
	parent_class = g_type_class_peek_parent (item_klass);

	gsi_klass->init_style = goc_line_init_style;

	obj_klass->get_property = goc_line_get_property;
	obj_klass->set_property = goc_line_set_property;
	g_object_class_install_property (obj_klass, LINE_PROP_X0,
		g_param_spec_double ("x0",
			_("x0"),
			_("The line start x coordinate"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, LINE_PROP_Y0,
		g_param_spec_double ("y0",
			_("y0"),
			_("The line start y coordinate"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, LINE_PROP_X1,
		g_param_spec_double ("x1",
			_("x1"),
			_("The line end x coordinate"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, LINE_PROP_Y1,
		g_param_spec_double ("y1",
			_("y1"),
			_("The line end y coordinate"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
        g_object_class_install_property (obj_klass, LINE_PROP_START_ARROW,
                 g_param_spec_boxed ("start-arrow",
				     _("Start Arrow"),
				     _("Arrow for line's start"),
				     GO_ARROW_TYPE,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE));
        g_object_class_install_property (obj_klass, LINE_PROP_END_ARROW,
                 g_param_spec_boxed ("end-arrow",
				     _("End Arrow"),
				     _("Arrow for line's end"),
				     GO_ARROW_TYPE,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->update_bounds = goc_line_update_bounds;
	item_klass->distance = goc_line_distance;
	item_klass->draw = goc_line_draw;
	item_klass->copy = goc_line_copy;
}

GSF_CLASS (GocLine, goc_line,
	   goc_line_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
