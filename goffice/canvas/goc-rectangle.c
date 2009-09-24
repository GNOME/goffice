/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-rectangle.c:
 *
 * Copyright (C) 2008-2009 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/goffice.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

enum {
	RECT_PROP_0,
	RECT_PROP_X,
	RECT_PROP_Y,
	RECT_PROP_W,
	RECT_PROP_H,
	RECT_PROP_ROTATION
};

static void
goc_rectangle_set_property (GObject *gobject, guint param_id,
				    GValue const *value, GParamSpec *pspec)
{
	GocRectangle *rect = GOC_RECTANGLE (gobject);

	switch (param_id) {
	case RECT_PROP_X:
		rect->x = g_value_get_double (value);
		break;

	case RECT_PROP_Y:
		rect->y = g_value_get_double (value);
		break;

	case RECT_PROP_W:
		rect->width = g_value_get_double (value);
		break;

	case RECT_PROP_H:
		rect->height = g_value_get_double (value);
		break;

	case RECT_PROP_ROTATION:
		rect->rotation = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));

}

static void
goc_rectangle_get_property (GObject *gobject, guint param_id,
				    GValue *value, GParamSpec *pspec)
{
	GocRectangle *rect = GOC_RECTANGLE (gobject);

	switch (param_id) {
	case RECT_PROP_X:
		g_value_set_double (value, rect->x);
		break;

	case RECT_PROP_Y:
		g_value_set_double (value, rect->y);
		break;

	case RECT_PROP_W:
		g_value_set_double (value, rect->width);
		break;

	case RECT_PROP_H:
		g_value_set_double (value, rect->height);
		break;

	case RECT_PROP_ROTATION:
		g_value_set_double (value, rect->rotation);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_rectangle_update_bounds (GocItem *item)
{
	GocRectangle *rect = GOC_RECTANGLE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	/* FIXME: take rotation into account */
	double extra_width = style->line.width /2.;
	if (extra_width <= 0.)
		extra_width = .5;
	item->x0 = rect->x - extra_width;
	item->y0 = rect->y - extra_width;
	item->x1 = rect->x + rect->width + extra_width;
	item->y1 = rect->y + rect->height + extra_width;
}

static double
goc_rectangle_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocRectangle *rect = GOC_RECTANGLE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	/* FIXME: take rotation into account */
	double extra_width = (style->line.width)? style->line.width /2.: .5;
	double dx, dy;
	if (x < rect->x - extra_width) {
		dx = rect->x - extra_width - x;
	} else if (x < rect->x + rect->width + extra_width) {
		dx = 0;
	} else {
		dx = x - extra_width - rect->x - rect->width;
	}
	if (y < rect->y - extra_width) {
		dy = rect->y - extra_width - y;
	} else if (y < rect->y + rect->height + extra_width) {
		dy = 0;
	} else {
		dy = y - extra_width - rect->y - rect->height;
	}
	*near_item = item;
	return hypot (dx, dy);
}

static void
goc_rectangle_draw (GocItem const *item, cairo_t *cr)
{
	GocRectangle *rect = GOC_RECTANGLE (item);
	double hoffs, voffs = ceil (go_styled_object_get_style (GO_STYLED_OBJECT (item))->line.width);
	if (voffs <= 0.)
		voffs = 1.;
	hoffs = ((int) voffs & 1)? .5: 0.;
	voffs = hoffs;
	if (goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)
		hoffs += (int) rect->width;
	cairo_save (cr);
	goc_group_cairo_transform (item->parent, cr, hoffs + (int) rect->x, voffs + (int) rect->y);
	cairo_rotate (cr, rect->rotation);
	cairo_rectangle (cr, 0., 0., (int) rect->width, (int)rect->height);
	cairo_restore (cr);
	/* Fill the shape */
	if (go_styled_object_set_cairo_fill (GO_STYLED_OBJECT (item), cr))
		cairo_fill_preserve (cr);
	/* Draw the line */
	if (goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr))
		cairo_stroke (cr);
	else
		cairo_new_path (cr);
}

static void
goc_rectangle_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = GO_COLOR_BLACK;
	if (style->fill.auto_type)
		style->fill.type  = GO_STYLE_FILL_PATTERN;
	if (style->fill.auto_fore)
		style->fill.pattern.fore = GO_COLOR_BLACK;
	if (style->fill.auto_back)
		style->fill.pattern.back = GO_COLOR_WHITE;
}

static void
goc_rectangle_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;

	obj_klass->get_property = goc_rectangle_get_property;
	obj_klass->set_property = goc_rectangle_set_property;
	g_object_class_install_property (obj_klass, RECT_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The rectangle left position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, RECT_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The rectangle top position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, RECT_PROP_W,
		g_param_spec_double ("width",
			_("Width"),
			_("The rectangle width"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, RECT_PROP_H,
		g_param_spec_double ("height",
			_("Height"),
			_("The rectangle height"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
/*	g_object_class_install_property (obj_klass, RECT_PROP_ROTATION,
		g_param_spec_double ("rotation",
			_("Rotation"),
			_("The rotation around top left position"),
			0., 2 * M_PI, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));*/

	gsi_klass->init_style = goc_rectangle_init_style;

	item_klass->update_bounds = goc_rectangle_update_bounds;
	item_klass->distance = goc_rectangle_distance;
	item_klass->draw = goc_rectangle_draw;
}

GSF_CLASS (GocRectangle, goc_rectangle,
	   goc_rectangle_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
