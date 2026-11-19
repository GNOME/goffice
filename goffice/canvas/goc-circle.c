/*
 * goc-circle.c :
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
 * SECTION:goc-circle
 * @short_description: Circle.
 *
 * #GocCircle implements circle drawing in the canvas.
**/

static GocItemClass *parent_class;

enum {
	CIRCLE_PROP_0,
	CIRCLE_PROP_X,
	CIRCLE_PROP_Y,
	CIRCLE_PROP_R
};

static void
goc_circle_set_property (GObject *gobject, guint param_id,
			 GValue const *value, GParamSpec *pspec)
{
	GocCircle *circle = GOC_CIRCLE (gobject);

	switch (param_id) {
	case CIRCLE_PROP_X:
		circle->x = g_value_get_double (value);
		break;

	case CIRCLE_PROP_Y:
		circle->y = g_value_get_double (value);
		break;

	case CIRCLE_PROP_R:
		circle->radius = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));
}

static void
goc_circle_get_property (GObject *gobject, guint param_id,
			 GValue *value, GParamSpec *pspec)
{
	GocCircle *circle = GOC_CIRCLE (gobject);

	switch (param_id) {
	case CIRCLE_PROP_X:
		g_value_set_double (value, circle->x);
		break;

	case CIRCLE_PROP_Y:
		g_value_set_double (value, circle->y);
		break;

	case CIRCLE_PROP_R:
		g_value_set_double (value, circle->radius);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static double
goc_circle_outline_extra_radius (GocItem *item)
{
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double ppu = item->canvas? goc_canvas_get_pixels_per_unit (item->canvas): 1.;

	if (style->line.dash_type == GO_LINE_NONE)
		return 0;

	/* The circle is stroked after the item transform is restored, so the
	 * outline does not scale with the transform.  Hairlines are one
	 * device pixel wide.  */
	if (style->line.width > 0.)
		return style->line.width / 2.;
	return .5 / ppu;
}

static double
goc_circle_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocCircle *circle = GOC_CIRCLE (item);
	double k = MAX (_goc_item_transform_scale (item), 1e-6);
	double d, extra_dist = goc_circle_outline_extra_radius (item) / k;
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

	x -= circle->x;
	y -= circle->y;
	d = hypot (x, y);
	return MAX (d - circle->radius - extra_dist, 0);
}

static void
goc_circle_draw (GocItem const *item, cairo_t *cr)
{
	GocCircle *circle = GOC_CIRCLE (item);
	double scale = (circle->radius > 0.)? circle->radius: 1.e-10;

	cairo_save (cr);
	_goc_item_transform (item, cr, TRUE);
	goc_group_cairo_transform (item->parent, cr, circle->x, circle->y);
	cairo_scale (cr, scale, scale);
	cairo_arc (cr, 0., 0., 1., 0., 2 * M_PI);
	cairo_restore (cr);
	/* Fill the shape */
	go_styled_object_fill (GO_STYLED_OBJECT (item), cr, TRUE);
	/* Draw the line */
	if (goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr))
		cairo_stroke (cr);
	else
		cairo_new_path (cr);
}

static void
goc_circle_update_bounds (GocItem *item)
{
	GocCircle *circle = GOC_CIRCLE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	cairo_surface_t *surface;
	cairo_t *cr;
	cairo_matrix_t m;
	double cx, cy, hx, hy, extra;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);
	_goc_item_transform (item, cr, FALSE);
	cairo_get_matrix (cr, &m);
	cairo_destroy (cr);
	cairo_surface_destroy (surface);

	/* The transformed circle is an ellipse; take its axis aligned
	 * extents.  */
	cx = circle->x;
	cy = circle->y;
	cairo_matrix_transform_point (&m, &cx, &cy);
	hx = circle->radius * hypot (m.xx, m.xy);
	hy = circle->radius * hypot (m.yx, m.yy);

	if (style->line.dash_type != GO_LINE_NONE) {
		extra = goc_circle_outline_extra_radius (item);
		item->x0 = cx - hx - extra;
		item->y0 = cy - hy - extra;
		item->x1 = cx + hx + extra;
		item->y1 = cy + hy + extra;
	} else if (go_style_is_fill_visible (style)) {
		item->x0 = cx - hx;
		item->y0 = cy - hy;
		item->x1 = cx + hx;
		item->y1 = cy + hy;
	} else {
		/* nothing to draw, don't keep stale bounds */
		item->x0 = item->y0 = G_MAXDOUBLE;
		item->x1 = item->y1 = -G_MAXDOUBLE;
	}
}

static void
goc_circle_copy (GocItem *dest, GocItem *source)
{
	GocCircle *src = GOC_CIRCLE (source), *dst = GOC_CIRCLE (dest);

	dst->x = src->x;
	dst->y = src->y;
	dst->radius = src->radius;
	parent_class->copy (dest, source);
}

static void
goc_circle_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
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
goc_circle_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;
	parent_class = g_type_class_peek_parent (item_klass);

	obj_klass->get_property = goc_circle_get_property;
	obj_klass->set_property = goc_circle_set_property;
	g_object_class_install_property (obj_klass, CIRCLE_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The circle center horizontal position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, CIRCLE_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The circle center vertical position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, CIRCLE_PROP_R,
		g_param_spec_double ("radius",
			_("Radius"),
			_("The circle radius"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	gsi_klass->init_style = goc_circle_init_style;

	item_klass->distance = goc_circle_distance;
	item_klass->draw = goc_circle_draw;
	item_klass->copy = goc_circle_copy;
	item_klass->update_bounds = goc_circle_update_bounds;
}

GSF_CLASS (GocCircle, goc_circle,
	   goc_circle_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
