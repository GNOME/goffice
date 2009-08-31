/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-ellipse.c :  
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

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

enum {
	ELLIPSE_PROP_0,
	ELLIPSE_PROP_X,
	ELLIPSE_PROP_Y,
	ELLIPSE_PROP_W,
	ELLIPSE_PROP_H,
	ELLIPSE_PROP_ROTATION
};

static void
goc_ellipse_set_property (GObject *gobject, guint param_id,
				    GValue const *value, GParamSpec *pspec)
{
	GocEllipse *ellipse = GOC_ELLIPSE (gobject);

	switch (param_id) {
	case ELLIPSE_PROP_X:
		ellipse->x = g_value_get_double (value);
		break;

	case ELLIPSE_PROP_Y:
		ellipse->y = g_value_get_double (value);
		break;

	case ELLIPSE_PROP_W:
		ellipse->width = g_value_get_double (value);
		break;

	case ELLIPSE_PROP_H:
		ellipse->height = g_value_get_double (value);
		break;

	case ELLIPSE_PROP_ROTATION:
		ellipse->rotation = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (ellipse));
}

static void
goc_ellipse_get_property (GObject *gobject, guint param_id,
				    GValue *value, GParamSpec *pspec)
{
	GocEllipse *ellipse = GOC_ELLIPSE (gobject);

	switch (param_id) {
	case ELLIPSE_PROP_X:
		g_value_set_double (value, ellipse->x);
		break;

	case ELLIPSE_PROP_Y:
		g_value_set_double (value, ellipse->y);
		break;

	case ELLIPSE_PROP_W:
		g_value_set_double (value, ellipse->width);
		break;

	case ELLIPSE_PROP_H:
		g_value_set_double (value, ellipse->height);
		break;

	case ELLIPSE_PROP_ROTATION:
		g_value_set_double (value, ellipse->rotation);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_ellipse_update_bounds (GocItem *item)
{
	GocEllipse *ellipse = GOC_ELLIPSE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	/* FIXME: take rotation into account */
	double extra_width = style->line.width /2.;
	if (extra_width <= 0.)
		extra_width = .5;
	item->x0 = ellipse->x - extra_width;
	item->y0 = ellipse->y - extra_width;
	item->x1 = ellipse->x + ellipse->width + extra_width;
	item->y1 = ellipse->y + ellipse->height + extra_width;
}

static double
goc_ellipse_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocEllipse *ellipse = GOC_ELLIPSE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	/* FIXME: take rotation into account */
	/* FIXME: we just consider that a point inside the ellipse is at distance 0
	 even if the ellipse is not filled */
	double extra_width = (style->line.width)? style->line.width /2.: .5;
	double last = G_MAXDOUBLE, df, d2f, t, cs, sn,
		a = ellipse->width / 2, b = ellipse->height / 2,
		c = a * a - b * b;
	int i;
	*near_item = item;
	x = fabs (x - ellipse->x - a);
	y = fabs (y - ellipse->y - b);
	if (y < DBL_EPSILON) {
		x -= a + extra_width;
		return (x > 0.)? x: 0.;
	}
	if (x < DBL_EPSILON) {
		y -= b + extra_width;
		return (y > 0.)? y: 0.;
	}
	if (hypot (x / a, y / b) < 1.)
		return 0.;

	/* initial value: */
	t = atan2 (y, x);
	/* iterate using the Newton method */
	/* iterate no more than 10 times which should be largely enough
	 just a security to avoid an infinite loop if something goes wrong */
	for (i = 0; i < 10; i++) {
		cs = cos (t);
		sn = sin (t);
		df = a * x * sn - b * y * cs - c * cs * sn;
		d2f = a * x * cs + b * y * sn - c * (cs * cs - sn * sn);
		t -= df / d2f;
		if ( last == df || fabs (df) < DBL_EPSILON || fabs (df) >= fabs (last))
			break;
		last = df;
	}
	/* evaluate the distance and store in df */
	df = hypot (x - a * cos (t), y - b * sin (t)) - extra_width;
	return (df > 0.)? df: 0.;
}

static void
goc_ellipse_draw (GocItem const *item, cairo_t *cr)
{      
	GocEllipse *ellipse = GOC_ELLIPSE (item);
	double  scalex = (ellipse->width > 0.)? ellipse->width / 2.: 1.e-10,
		scaley = (ellipse->height > 0.)? ellipse->height / 2.: 1.e-10;
	double sign = (goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1.: 1.;
	cairo_save (cr);
	goc_group_cairo_transform (item->parent, cr, ellipse->x, ellipse->y);
	cairo_translate (cr, scalex * sign, scaley);
	cairo_scale (cr, scalex, scaley);
	cairo_rotate (cr, ellipse->rotation);
	cairo_arc (cr, 0., 0., 1., 0., 2 * M_PI);
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
goc_ellipse_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = RGBA_BLACK;
	if (style->fill.auto_type)
		style->fill.type  = GO_STYLE_FILL_PATTERN;
	if (style->fill.auto_fore)
		go_pattern_set_solid (&style->fill.pattern, RGBA_WHITE);
}

static void
goc_ellipse_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;

	obj_klass->get_property = goc_ellipse_get_property;
	obj_klass->set_property = goc_ellipse_set_property;
	g_object_class_install_property (obj_klass, ELLIPSE_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The rectangle left position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ELLIPSE_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The rectangle top position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ELLIPSE_PROP_W,
		g_param_spec_double ("width", 
			_("Width"),
			_("The rectangle width"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ELLIPSE_PROP_H,
		g_param_spec_double ("height", 
			_("Height"),
			_("The rectangle height"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
/*	g_object_class_install_property (obj_klass, ELLIPSE_PROP_ROTATION,
		g_param_spec_double ("rotation", 
			_("Rotation"),
			_("The rotation around top left position"),
			0., 2 * M_PI, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));*/

	gsi_klass->init_style = goc_ellipse_init_style;

	item_klass->update_bounds = goc_ellipse_update_bounds;
	item_klass->distance = goc_ellipse_distance;
	item_klass->draw = goc_ellipse_draw;
}

GSF_CLASS (GocEllipse, goc_ellipse,
	   goc_ellipse_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
