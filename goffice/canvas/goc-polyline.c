/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-polyline.c :
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
 * SECTION:goc-polyline
 * @short_description: Multi-segment line.
 *
 * #GocPolyline implements muti-segment line drawing in the canvas.
**/

enum {
	POLYLINE_PROP_0,
	POLYLINE_PROP_POINTS,
	POLYLINE_PROP_SPLINE
};

static void
goc_polyline_set_property (GObject *gobject, guint param_id,
				    GValue const *value, GParamSpec *pspec)
{
	GocPolyline *polyline = GOC_POLYLINE (gobject);

	switch (param_id) {
	case POLYLINE_PROP_POINTS: {
		unsigned i;
		GocPoints *points = (GocPoints *) g_value_get_boxed (value);
		polyline->nb_points = points->n;
		g_free (polyline->points);
		polyline->points = g_new (GocPoint, points->n);
		for (i = 0; i < points->n; i++)
			polyline->points[i] = points->points[i];
		break;
	}
	case POLYLINE_PROP_SPLINE:
		polyline->use_spline = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));

}

static void
goc_polyline_get_property (GObject *gobject, guint param_id,
				    GValue *value, GParamSpec *pspec)
{
	GocPolyline *polyline = GOC_POLYLINE (gobject);

	switch (param_id) {
	case POLYLINE_PROP_POINTS: {
		unsigned i;
		GocPoints *points = goc_points_new (polyline->nb_points);
		for (i = 0; i < points->n; i++)
			points->points[i] = polyline->points[i];
		g_value_set_boxed (value, points);
		goc_points_unref (points);
		break;
	}
	case POLYLINE_PROP_SPLINE:
		g_value_set_boolean (value, polyline->use_spline);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_polyline_update_bounds (GocItem *item)
{
	GocPolyline *polyline = GOC_POLYLINE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double extra_width = style->line.width;
	unsigned i;
	if (extra_width <= 0.)
		extra_width = 1.;
	if (polyline->nb_points == 0)
		return;
	/* FIXME: implement the use_spline case */
	item->x0 = item->x1 = polyline->points[0].x;
	item->y0 = item->y1 = polyline->points[0].y;
	for (i = 1; i < polyline->nb_points; i++) {
		if (polyline->points[i].x < item->x0)
			item->x0 = polyline->points[i].x;
		else if (polyline->points[i].x > item->x1)
			item->x1 = polyline->points[i].x;
		if (polyline->points[i].y < item->y0)
			item->y0 = polyline->points[i].y;
		else if (polyline->points[i].y > item->y1)
			item->y1 = polyline->points[i].y;
	}
	item->x0 -= extra_width;
	item->y0 -= extra_width;
	item->x1 -= extra_width;
	item->y1 -= extra_width;
}

static double
goc_polyline_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocPolyline *polyline = GOC_POLYLINE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	/* FIXME: implement the use_spline case */
	double extra_width = (style->line.width)? style->line.width /2.: .5;
	double dx, dy, l, startx, starty, x_, y_, t, res;
	unsigned i;

	*near_item = item;
	/* FIXME: not tested!!! */
	/* first test if the point is inside the polygon */
	startx = polyline->points[0].x;
	starty = polyline->points[0].y;
	res = hypot (polyline->points[polyline->nb_points - 1].x - x, polyline->points[polyline->nb_points - 1].y - y);
	for (i = 0; i < polyline->nb_points; i++) {
		dx = polyline->points[i].x - startx;
		dy = polyline->points[i].y - starty;
		l = hypot (dx, dy);
		x_ = x - startx;
		y_ = y - starty;
		t = (x_ * dx + y_ * dy) / l;
		y = (-x_ * dy + y_ * dx) / l;
		x_ = t;
		*near_item = item;
		if (x < 0. ) {
			t = hypot (x_, y_);
			if (t < res)
				res = t;
		} else if (t <= l) {
			if (y_ < res)
				res = y_;
		}
		startx = polyline->points[i].x;
		starty = polyline->points[i].y;
	}
	res -= extra_width; /* no need to be more precise */
	return (res > 0.)? res: 0.;
}

static void
goc_polyline_draw (GocItem const *item, cairo_t *cr)
{
	GocPolyline *polyline = GOC_POLYLINE (item);
	unsigned i;
	double sign = (goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1: 1;
	if (polyline->nb_points == 0)
		return;
	cairo_save (cr);
	goc_group_cairo_transform (item->parent, cr, polyline->points[0].x, polyline->points[0].y);
        cairo_move_to (cr, 0., 0.);
	/* FIXME: implement the use_spline case */

	for (i = 1; i < polyline->nb_points; i++)
		cairo_line_to (cr, (polyline->points[i].x - polyline->points[0].x) * sign,
		               polyline->points[i].y - polyline->points[0].y);

	if (go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr))
		cairo_stroke (cr);
	else
		cairo_new_path (cr);
	cairo_restore (cr);
}

static void
goc_polyline_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = GO_COLOR_BLACK;
}

static void
goc_polyline_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;

	gsi_klass->init_style = goc_polyline_init_style;

	obj_klass->get_property = goc_polyline_get_property;
	obj_klass->set_property = goc_polyline_set_property;
        g_object_class_install_property (obj_klass, POLYLINE_PROP_POINTS,
                 g_param_spec_boxed ("points", _("points"), _("The polyline vertices"),
				     GOC_TYPE_POINTS,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE));
/*	g_object_class_install_property (obj_klass, POLYLINE_PROP_SPLINE,
		g_param_spec_boolean ("use-spline",
				      _("Use spline"),
				      _("Use a Bezier cubic spline as line"),
				      FALSE,
				      GSF_PARAM_STATIC | G_PARAM_READABLE));*/

	item_klass->update_bounds = goc_polyline_update_bounds;
	item_klass->distance = goc_polyline_distance;
	item_klass->draw = goc_polyline_draw;
}

GSF_CLASS (GocPolyline, goc_polyline,
	   goc_polyline_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
