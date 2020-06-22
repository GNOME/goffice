/*
 * goc-rectangle.c:
 *
 * Copyright (C) 2008-2009 Jean Brefort (jean.brefort@normalesup.org)
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
 * SECTION:goc-rectangle
 * @short_description: Rectangle.
 *
 * #GocPolygon implements rectangle drawing in the canvas.
**/

static GocItemClass *parent_klass;

enum {
	RECT_PROP_0,
	RECT_PROP_X,
	RECT_PROP_Y,
	RECT_PROP_W,
	RECT_PROP_H,
	RECT_PROP_ROTATION,
	RECT_PROP_RX,
	RECT_PROP_RY,
	RECT_PROP_TYPE,
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

	case RECT_PROP_RX:
		rect->rx = g_value_get_double (value);
		break;

	case RECT_PROP_RY:
		rect->ry = g_value_get_double (value);
		break;

	case RECT_PROP_TYPE:
		rect->type = g_value_get_int (value);
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

	case RECT_PROP_RX:
		g_value_set_double (value, rect->rx);
		break;

	case RECT_PROP_RY:
		g_value_set_double (value, rect->ry);
		break;

	case RECT_PROP_TYPE:
		g_value_set_int (value, rect->type);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static gboolean
goc_rectangle_prepare_draw (GocItem const *item, cairo_t *cr, gboolean flag)
{
	GocRectangle *rect = GOC_RECTANGLE (item);
	double sign = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1.: 1.;

	if (0 == rect->width && 0 == rect->height)
		return FALSE;

	_goc_item_transform (item, cr, flag);
	if (1 == flag) {
		goc_group_cairo_transform (item->parent, cr, rect->x, rect->y);
		cairo_scale (cr, sign, 1.);
	} else {
		cairo_translate (cr, rect->x, rect->y);
	}
	cairo_rotate (cr, rect->rotation * sign);
	if (0 == rect->type || 0 == rect->rx || 0 == rect->ry) {
		cairo_rectangle (cr, 0., 0., (int) rect->width, (int) rect->height);
	} else {

		if (rect->type & 1) {
			cairo_move_to (cr, rect->rx, 0.);
			cairo_save (cr);
			cairo_translate (cr, rect->rx, rect->ry);
			cairo_scale (cr, rect->rx, rect->ry);
			cairo_arc_negative (cr, 0. , 0. ,1. , -M_PI/2. , M_PI);
			cairo_restore (cr);
		} else {
			cairo_move_to (cr, 0., 0.);
		}

		if (rect->type & 8) {
			cairo_line_to (cr, 0., rect->height - rect->ry);
			cairo_save (cr);
			cairo_translate (cr, rect->rx, rect->height - rect->ry);
			cairo_scale (cr, rect->rx, rect->ry);
			cairo_arc_negative (cr, 0., 0. ,1. , M_PI, M_PI/2.);
			cairo_restore (cr);
		} else {
			cairo_line_to (cr, 0., rect->height);
		}

		if (rect->type & 4) {
			cairo_line_to (cr, rect->width - rect->rx, rect->height);
			cairo_save (cr);
			cairo_translate (cr, rect->width - rect->rx, rect->height - rect->ry);
			cairo_scale (cr, rect->rx, rect->ry);
			cairo_arc_negative (cr, 0., 0. ,1. , M_PI/2., 0.);
			cairo_restore (cr);
		} else {
			cairo_line_to (cr, rect->width, rect->height);
		}

		if (rect->type & 2) {
			cairo_line_to (cr, rect->width, rect->ry);
			cairo_save (cr);
			cairo_translate (cr, rect->width - rect->rx, rect->ry);
			cairo_scale (cr, rect->rx, rect->ry);
			cairo_arc_negative (cr, 0., 0. ,1. , 0., -M_PI/2.);
			cairo_restore (cr);
		} else {
			cairo_line_to (cr, rect->width, 0.);
		}
		cairo_close_path (cr);
	}
	return TRUE;
}

static void
goc_rectangle_update_bounds (GocItem *item)
{
	cairo_surface_t *surface;
	cairo_t *cr;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1 , 1);
	cr = cairo_create (surface);

	cairo_save (cr);
	if (goc_rectangle_prepare_draw (item, cr, 0)) {
		cairo_restore (cr);
		if (go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr))
			cairo_stroke_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);
		else if (go_style_is_fill_visible (go_styled_object_get_style (GO_STYLED_OBJECT (item))))
			cairo_fill_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);
		else {
			item->x0 = item->y0 = G_MAXDOUBLE;
			item->x1 = item->y1 = -G_MAXDOUBLE;
		}
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
}

static double
goc_rectangle_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocRectangle *rect = GOC_RECTANGLE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double tmp_width = 0;
	double res = 20;
	double ppu = goc_canvas_get_pixels_per_unit (item->canvas);
	cairo_surface_t *surface;
	cairo_t *cr;

	if (0 == rect->width && 0 == rect->height)
		return res;

	*near_item = item;
	tmp_width = style->line.width;
	if (style->line.width * ppu < 5)
		style->line.width = 5. / (ppu * ppu);
	else
		style->line.width /= ppu;
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	if (goc_rectangle_prepare_draw (item, cr, 0)) {
		/* Filled OR both fill and stroke are none */
		if (style->fill.type != GO_STYLE_FILL_NONE ||
			(style->fill.type == GO_STYLE_FILL_NONE && !goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr))) {
			if (cairo_in_fill (cr, x, y))
				res = 0;
		}
		if (goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr) && cairo_in_stroke (cr, x, y)) {
			res = 0;
		}
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	style->line.width = tmp_width;
	return res;
}

static void
goc_rectangle_draw (GocItem const *item, cairo_t *cr)
{
	gboolean scale_line_width = goc_styled_item_get_scale_line_width (GOC_STYLED_ITEM (item));
	cairo_save(cr);
	if (goc_rectangle_prepare_draw (item, cr, 1)) {
		go_styled_object_fill (GO_STYLED_OBJECT (item), cr, TRUE);
		if (!scale_line_width)
			cairo_restore (cr);
		if (go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr)) {
			cairo_stroke (cr);
		} else {
			cairo_new_path (cr);
		}
		if (scale_line_width)
			cairo_restore (cr);
	} else
		cairo_restore (cr);
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
goc_rectangle_copy (GocItem *dest, GocItem *source)
{
	GocRectangle *src = GOC_RECTANGLE (source), *dst = GOC_RECTANGLE (dest);

	dst->rotation = src->rotation;
	dst->x = src->x;
	dst->y = src->y;
	dst->width = src->width;
	dst->height = src->height;
	dst->type = src->type;
	dst->rx = src->rx;
	dst->ry = src->ry;
	parent_klass->copy (dest, source);
}

static void
goc_rectangle_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;
	parent_klass = g_type_class_peek_parent (item_klass);

	obj_klass->get_property = goc_rectangle_get_property;
	obj_klass->set_property = goc_rectangle_set_property;
	g_object_class_install_property (obj_klass, RECT_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The rectangle left position (or right position in RTL mode)"),
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
	g_object_class_install_property (obj_klass, RECT_PROP_ROTATION,
		g_param_spec_double ("rotation",
			_("Rotation"),
			_("The rotation around top left position"),
			0., 2 * M_PI, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, RECT_PROP_TYPE,
		g_param_spec_int ("type",
			_("Type"),
			_("The rectangle type: 0 for no rounded corner, 1 for rounded top left, 2 for top right, 4 for bottom right, 8 for bottom left, or any combination of these."),
			0, 15, 0,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, RECT_PROP_RX,
		g_param_spec_double ("rx",
			_("rx"),
			_("The round rectangle rx"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, RECT_PROP_RY,
		g_param_spec_double ("ry",
			_("ry"),
			_("The round rectangle ry"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	gsi_klass->init_style = goc_rectangle_init_style;

	item_klass->update_bounds = goc_rectangle_update_bounds;
	item_klass->distance = goc_rectangle_distance;
	item_klass->draw = goc_rectangle_draw;
	item_klass->copy = goc_rectangle_copy;
}

GSF_CLASS (GocRectangle, goc_rectangle,
	   goc_rectangle_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
