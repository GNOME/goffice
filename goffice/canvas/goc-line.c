/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

enum {
	LINE_PROP_0,
	LINE_PROP_X0,
	LINE_PROP_Y0,
	LINE_PROP_X1,
	LINE_PROP_Y1,
	LINE_PROP_ARROWHEAD,
	LINE_PROP_ARROW_SHAPE_A,
	LINE_PROP_ARROW_SHAPE_B,
	LINE_PROP_ARROW_SHAPE_C
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

	case LINE_PROP_ARROWHEAD:
		line->arrowhead = g_value_get_boolean (value);
		break;

	case LINE_PROP_ARROW_SHAPE_A:
		line->headA = g_value_get_double (value);
		break;

	case LINE_PROP_ARROW_SHAPE_B:
		line->headA = g_value_get_double (value);
		break;

	case LINE_PROP_ARROW_SHAPE_C:
		line->headA = g_value_get_double (value);
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

	case LINE_PROP_ARROWHEAD:
		g_value_set_boolean (value, line->arrowhead);
		break;

	case LINE_PROP_ARROW_SHAPE_A:
		g_value_set_double (value, line->headA);
		break;

	case LINE_PROP_ARROW_SHAPE_B:
		g_value_set_double (value, line->headB);
		break;

	case LINE_PROP_ARROW_SHAPE_C:
		g_value_set_double (value, line->headC);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_line_update_bounds (GocItem *item)
{
	GocLine *line = GOC_LINE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	/* FIXME: take rotation into account */
	double extra_width = style->line.width /2.;
	/* fix me, take ends and orientation into account */
	if (extra_width <= 0.)
		extra_width = .5;
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
	t = y - style->line.width / 2.;
	return (t > 0.)? t: 0.;
}

static void goc_line_draw (GocItem const *item, cairo_t *cr)
{
	GocLine *line = GOC_LINE (item);
	double sign = (goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1: 1;
        if (go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr)) {
		/* try to avoid horizontal and vertical lines between two pixels */
		double hoffs, voffs = ceil (go_styled_object_get_style (GO_STYLED_OBJECT (item))->line.width);
		if (voffs <= 0.)
			voffs = 1.;
		hoffs = ((int) voffs & 1)? .5: 0.;
		voffs = (line->starty == line->endy)? hoffs: 0.;
		if (line->startx != line->endx)
		                hoffs = 0.;
		cairo_save (cr);
		goc_group_cairo_transform (item->parent, cr, hoffs + (int) line->startx, voffs + (int) line->starty);
		cairo_move_to (cr, 0., 0.);
		cairo_line_to (cr, (int) (line->endx - line->startx) * sign, (int) (line->endy - line->starty));
		cairo_stroke (cr);
		cairo_restore (cr);
	}
}

static void
goc_line_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = RGBA_BLACK;
	if (style->line.auto_fore)
		style->line.fore  = 0;
}

static void
goc_line_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;

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
	g_object_class_install_property (obj_klass, LINE_PROP_ARROWHEAD,
		g_param_spec_boolean ("arrowhead",
			_("Arrow head"),
			_("Wether to add an arrow head atthe end of the line or not"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, LINE_PROP_ARROW_SHAPE_A,
		g_param_spec_double ("arrow-shape-a",
			_("Arrow head shape A"),
			_("The distance from tip of arrow head to center"),
			0, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, LINE_PROP_ARROW_SHAPE_B,
		g_param_spec_double ("arrow-shape-b",
			_("Arrow head shape B"),
			_("The distance from tip of arrow head to trailing point, measured along shaft"),
			0, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, LINE_PROP_ARROW_SHAPE_C,
		g_param_spec_double ("arrow-shape-c",
			_("Arrow head shape C"),
			_("The distance of trailing points from outside edge of shaft"),
			0, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->update_bounds = goc_line_update_bounds;
	item_klass->distance = goc_line_distance;
	item_klass->draw = goc_line_draw;
}

GSF_CLASS (GocLine, goc_line,
	   goc_line_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)

G_END_DECLS
