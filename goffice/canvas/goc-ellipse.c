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

/**
 * SECTION:goc-ellipse
 * @short_description: Ellipse.
 *
 * #GocEllipse implements ellipse drawing in the canvas.
**/

static GocItemClass *parent_class;

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

static gboolean
goc_ellipse_prepare_draw (GocItem const *item, cairo_t *cr, gboolean flag)
{
	GocEllipse *ellipse = GOC_ELLIPSE (item);
	double sign = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1.: 1.;

	if (0 == ellipse->width && 0 == ellipse->height)
		return FALSE;

	_goc_item_transform (item, cr, flag);
	if (1 == flag) {
		goc_group_cairo_transform (item->parent, cr, ellipse->x + ellipse->width/2., ellipse->y + ellipse->height/2.);
	} else {
		cairo_translate (cr, ellipse->x + ellipse->width/2., ellipse->y + ellipse->height/2.);
	}
	cairo_save (cr);
	cairo_rotate (cr, ellipse->rotation * sign);
	cairo_scale (cr, ellipse->width/2. * sign, ellipse->height/2.);
	cairo_arc (cr, 0., 0., 1., 0., 2 * M_PI);
	cairo_restore (cr);
	return TRUE;
}

static void
goc_ellipse_update_bounds (GocItem *item)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	gboolean set_line,
		scale_line_width = goc_styled_item_get_scale_line_width (GOC_STYLED_ITEM (item));

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1 , 1);
	cr = cairo_create (surface);

	cairo_save (cr);
	if (goc_ellipse_prepare_draw (item, cr, 0)) {
		if (!scale_line_width)
			cairo_restore (cr);
		set_line = go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr);
		if (scale_line_width)
			cairo_restore (cr);
		if (set_line)
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
goc_ellipse_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocEllipse *ellipse = GOC_ELLIPSE (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double tmp_width = 0;
	double res = 20;
	double ppu = goc_canvas_get_pixels_per_unit (item->canvas);
	cairo_surface_t *surface;
	cairo_t *cr;
	gboolean set_line,
		scale_line_width = goc_styled_item_get_scale_line_width (GOC_STYLED_ITEM (item));

	if (0 == ellipse->width && 0 == ellipse->height)
		return res;

	*near_item = item;
	tmp_width = style->line.width;
	if (style->line.width * ppu < 5)
		style->line.width = 5. / (ppu * ppu);
	else
		style->line.width /= ppu;
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	cairo_save (cr);
	if (goc_ellipse_prepare_draw (item, cr, 0)) {
		/* Filled OR both fill and stroke are none */
		if (!scale_line_width)
			cairo_restore (cr);
		set_line = go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr);
		if (scale_line_width)
			cairo_restore (cr);
		if (style->fill.type != GO_STYLE_FILL_NONE ||
			(style->fill.type == GO_STYLE_FILL_NONE && !set_line)) {
			if (cairo_in_fill (cr, x, y))
				res = 0;
		}
		if (set_line && cairo_in_stroke (cr, x, y)) {
			res = 0;
		}
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	style->line.width = tmp_width;
	return res;
}

static void
goc_ellipse_draw (GocItem const *item, cairo_t *cr)
{
	gboolean scale_line_width = goc_styled_item_get_scale_line_width (GOC_STYLED_ITEM (item));
	gboolean needs_restore;

	cairo_save(cr);
	needs_restore = TRUE;
	if (goc_ellipse_prepare_draw (item, cr, 1)) {
		go_styled_object_fill (GO_STYLED_OBJECT (item), cr, TRUE);
		if (!scale_line_width) {
			cairo_restore (cr);
			needs_restore = FALSE;
		}
		if (go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr)) {
			cairo_stroke (cr);
		} else {
			cairo_new_path (cr);
		}
	}
	if (needs_restore)
		cairo_restore(cr);
}

static void
goc_ellipse_copy (GocItem *dest, GocItem *source)
{
	GocEllipse *src = GOC_ELLIPSE (source), *dst = GOC_ELLIPSE (dest);

	dst->rotation = src->rotation;
	dst->x = src->x;
	dst->y = src->y;
	dst->width = src->width;
	dst->height = src->height;
	parent_class->copy (dest, source);
}

static void
goc_ellipse_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
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
goc_ellipse_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;
	parent_class = g_type_class_peek_parent (item_klass);

	obj_klass->get_property = goc_ellipse_get_property;
	obj_klass->set_property = goc_ellipse_set_property;
	g_object_class_install_property (obj_klass, ELLIPSE_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The ellipse left position (or right position in RTL mode)"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ELLIPSE_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The ellipse top position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ELLIPSE_PROP_W,
		g_param_spec_double ("width",
			_("Width"),
			_("The ellipse width"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ELLIPSE_PROP_H,
		g_param_spec_double ("height",
			_("Height"),
			_("The ellipse height"),
			0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, ELLIPSE_PROP_ROTATION,
		g_param_spec_double ("rotation",
			_("Rotation"),
			_("The rotation around top left position"),
			0., 2 * M_PI, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	gsi_klass->init_style = goc_ellipse_init_style;

	item_klass->update_bounds = goc_ellipse_update_bounds;
	item_klass->distance = goc_ellipse_distance;
	item_klass->draw = goc_ellipse_draw;
	item_klass->copy = goc_ellipse_copy;
}

GSF_CLASS (GocEllipse, goc_ellipse,
	   goc_ellipse_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
