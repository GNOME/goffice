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

	case LINE_PROP_START_ARROW:
		line->start_arrow = *((GOArrow *)g_value_peek_pointer (value));
		break;

	case LINE_PROP_END_ARROW:
		line->end_arrow = *((GOArrow *)g_value_peek_pointer (value));
		break;

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

static void
handle_arrow_bounds (GOArrow const *arrow, GocItem *item)
{
	/*
	 * Do not calculate things precisely, just add enough room
	 * in all directions.
	 */

	switch (arrow->typ) {
	case GO_ARROW_NONE:
		break;
	case GO_ARROW_KITE: {
		double d = hypot (arrow->b, arrow->c);
		item->x0 -= d;
		item->x1 += d;
		item->y0 -= d;
		item->y1 += d;
		break;
	}
	case GO_ARROW_OVAL: {
		double d = MAX (arrow->a, arrow->b);
		item->x0 -= d;
		item->x1 += d;
		item->y0 -= d;
		item->y1 += d;
		break;
	}
	default:
		g_assert_not_reached ();
	}
}

static void
goc_line_update_bounds (GocItem *item)
{
	GocLine *line = GOC_LINE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double extra_width = style->line.width /2.;
	/* FIXME: take transform into account */
	if (extra_width <= 0.)
		extra_width = .5;
	if (style->line.cap == CAIRO_LINE_CAP_SQUARE)
		extra_width *= 1.5; /* 1.4142 should be enough */
	if (line->startx < line->endx) {
		item->x0 = line->startx - extra_width;
		item->x1 = line->endx + extra_width;
	} else {
		item->x0 = line->endx - extra_width;
		item->x1 = line->startx + extra_width;
	}
	if (line->starty < line->endy) {
		item->y0 = line->starty - extra_width;
		item->y1 = line->endy + extra_width;
	} else {
		item->y0 = line->endy - extra_width;
		item->y1 = line->starty + extra_width;
	}

	handle_arrow_bounds (&line->start_arrow, item);
	handle_arrow_bounds (&line->end_arrow, item);
}

static double
goc_line_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocLine *line = GOC_LINE (item);
	double dx, dy, l, t;
	GOStyle *style;
	dx = line->endx - line->startx;
	dy = line->endy - line->starty;
	l = hypot (dx, dy);
	x -= line->startx;
	y -= line->starty;
	t = (x * dx + y * dy) / l;
	y = (-x * dy + y * dx) / l;
	*near_item = item;
	if (t < 0.)
		return hypot (t, y); /* that might be not fully exact,
	 but we don't need a large precision */
	if (t > l)
		return hypot (t - l, y);
	style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	t = fabs (y) - ((style->line.width > 5)?  style->line.width/ 2.: 2.5);
	/* FIXME: do we need to take the arrow end into account? */
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
	double endx = (line->endx - line->startx) * sign, endy = line->endy - line->starty;
	double hoffs, voffs = ceil (style->line.width);
	double startx = 0, starty = 0;
	double phi;

	if (line->startx == line->endx && line->starty == line->endy)
		return;

	if (voffs <= 0.)
		voffs = 1.;

	hoffs = ((int) voffs & 1)? .5: 0.;
	voffs = (line->starty == line->endy)? hoffs: 0.;
	if (line->startx != line->endx)
	                hoffs = 0.;

	cairo_save (cr);
	_goc_item_transform (item, cr, TRUE);
	goc_group_cairo_transform (item->parent, cr,
				   hoffs + floor (line->startx),
				   voffs + floor (line->starty));

	endx = (endx > 0.)? ceil (endx): floor (endx);
	endy = (endy > 0.)? ceil (endy): floor (endy);

	phi = atan2 (endy, endx);
	draw_arrow (&line->start_arrow, cr, style,
		    &startx, &starty, phi + M_PI);
	draw_arrow (&line->end_arrow, cr, style,
		    &endx, &endy, phi);

        if ((endx != 0. || endy!= 0.) &&
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

G_END_DECLS
