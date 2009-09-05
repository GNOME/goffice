/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-polygon.c :
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
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
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

enum {
	POLYGON_PROP_0,
	POLYGON_PROP_POINTS,
	POLYGON_PROP_SPLINE
};

static void
goc_polygon_set_property (GObject *gobject, guint param_id,
				    GValue const *value, GParamSpec *pspec)
{
	GocPolygon *polygon = GOC_POLYGON (gobject);

	switch (param_id) {
	case POLYGON_PROP_POINTS: {
		unsigned i;
		GocPoints *points = (GocPoints *) g_value_get_boxed (value);
		polygon->nb_points = points->n;
		g_free (polygon->points);
		polygon->points = g_new (GocPoint, points->n);
		for (i = 0; i < points->n; i++)
			polygon->points[i] = points->points[i];
		break;
	}
	case POLYGON_PROP_SPLINE:
		polygon->use_spline = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));

}

static void
goc_polygon_get_property (GObject *gobject, guint param_id,
				    GValue *value, GParamSpec *pspec)
{
	GocPolygon *polygon = GOC_POLYGON (gobject);

	switch (param_id) {
	case POLYGON_PROP_POINTS: {
		unsigned i;
		GocPoints *points = goc_points_new (polygon->nb_points);
		for (i = 0; i < points->n; i++)
			points->points[i] = polygon->points[i];
		g_value_set_boxed (value, points);
		goc_points_unref (points);
		break;
	}
	case POLYGON_PROP_SPLINE:
		g_value_set_boolean (value, polygon->use_spline);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_polygon_update_bounds (GocItem *item)
{
	GocPolygon *polygon = GOC_POLYGON (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double extra_width = style->line.width;
	unsigned i;
	if (extra_width <= 0.)
		extra_width = 1.;
	if (polygon->nb_points == 0)
		return;
	/* FIXME: implement the use_spline case */
	item->x0 = item->x1 = polygon->points[0].x;
	item->y0 = item->y1 = polygon->points[0].y;
	for (i = 1; i < polygon->nb_points; i++) {
		if (polygon->points[i].x < item->x0)
			item->x0 = polygon->points[i].x;
		else if (polygon->points[i].x > item->x1)
			item->x1 = polygon->points[i].x;
		if (polygon->points[i].y < item->y0)
			item->y0 = polygon->points[i].y;
		else if (polygon->points[i].y > item->y1)
			item->y1 = polygon->points[i].y;
	}
	item->x0 -= extra_width;
	item->y0 -= extra_width;
	item->x1 -= extra_width;
	item->y1 -= extra_width;
}

static double
goc_polygon_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocPolygon *polygon = GOC_POLYGON (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	/* FIXME: implement the use_spline case */
	double extra_width = (style->line.width)? style->line.width /2.: .5;
	double dx, dy, l, startx, starty, x_, y_, t, res = 0.;
	int i, n;

	*near_item = item;
	/* FIXME: not tested!!! */
	/* first test if the point is inside the polygon */
	startx = atan2 (polygon->points[0].y - y, polygon->points[0].x - x);
	for (i = polygon->nb_points - 1; i >= 0; i--) {
		starty = atan2 (polygon->points[i].y - y, polygon->points[i].x - x);
		startx -= starty;
		if (startx > M_PI)
			startx -= 2 * M_PI;
		else if (startx < -M_PI)
			startx += 2 * M_PI;
		else if (startx == M_PI)
			return 0.; /* the point is on the edge */
		res += startx;
		startx = starty;
	}
	n = res / 2. / M_PI;
	if (n % 1)
		return 0.; /* only odd-even fillinig supported for now */
	startx = polygon->points[0].x;
	starty = polygon->points[0].y;
	res = G_MAXDOUBLE;
	for (i = polygon->nb_points - 1; i >= 0; i--) {
		dx = polygon->points[i].x - startx;
		dy = polygon->points[i].y - starty;
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
		startx = polygon->points[i].x;
		starty = polygon->points[i].y;
	}
	res -= extra_width; /* no need to be more precise */
	return (res > 0.)? res: 0.;
}

static void
goc_polygon_draw (GocItem const *item, cairo_t *cr)
{
	GocPolygon *polygon = GOC_POLYGON (item);
	unsigned i;
	double sign = (goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1: 1;
	if (polygon->nb_points == 0)
		return;
	cairo_save (cr);
	goc_group_cairo_transform (item->parent, cr, polygon->points[0].x, polygon->points[0].y);
        cairo_move_to (cr, 0., 0.);
	/* FIXME: implement the use_spline case */

	for (i = 1; i < polygon->nb_points; i++)
		cairo_line_to (cr, (polygon->points[i].x - polygon->points[0].x) * sign,
		               polygon->points[i].y - polygon->points[0].y);
	cairo_close_path (cr);

	if (go_styled_object_set_cairo_fill (GO_STYLED_OBJECT (item), cr))
		cairo_fill_preserve (cr);

	if (go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr))
		cairo_stroke (cr);
	else
		cairo_new_path (cr);
	cairo_restore (cr);
}

static void
goc_polygon_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = GO_COLOR_BLACK;
	if (style->fill.auto_type)
		style->fill.type  = GO_STYLE_FILL_PATTERN;
	if (style->fill.auto_fore)
		go_pattern_set_solid (&style->fill.pattern, GO_COLOR_WHITE);
}

static void
goc_polygon_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;

	gsi_klass->init_style = goc_polygon_init_style;

	obj_klass->get_property = goc_polygon_get_property;
	obj_klass->set_property = goc_polygon_set_property;
        g_object_class_install_property (obj_klass, POLYGON_PROP_POINTS,
                 g_param_spec_boxed ("points", _("points"), _("The polygon vertices"),
				     GOC_TYPE_POINTS,
				     GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, POLYGON_PROP_SPLINE,
		g_param_spec_boolean ("use-spline",
				      _("Use spline"),
				      _("Use a Bezier closed cubic spline as contour"),
				      FALSE,
				      GSF_PARAM_STATIC | G_PARAM_READABLE));

	item_klass->update_bounds = goc_polygon_update_bounds;
	item_klass->distance = goc_polygon_distance;
	item_klass->draw = goc_polygon_draw;
}

GSF_CLASS (GocPolygon, goc_polygon,
	   goc_polygon_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
