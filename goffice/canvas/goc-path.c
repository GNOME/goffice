/*
 * goc-path.c :
 *
 * Copyright (C) 2010 Valek FIlippov (frob@gnome.org)
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
#include <math.h>

/**
 * SECTION:goc-path
 * @short_description: Path item.
 *
 * #GocPath implements path drawing in the canvas.
**/

static GObjectClass *parent_class;

enum {
	PATH_PROP_0,
	PATH_PROP_X,
	PATH_PROP_Y,
	PATH_PROP_ROTATION,
	PATH_PROP_CLOSED,
	PATH_PROP_FILL_RULE,
	PATH_PROP_PATH,
};

static void
goc_path_set_property (GObject *gobject, guint param_id,
				    GValue const *value, GParamSpec *pspec)
{
	GocPath *path = GOC_PATH (gobject);

	switch (param_id) {
	case PATH_PROP_X:
		path->x = g_value_get_double (value);
		break;

	case PATH_PROP_Y:
		path->y = g_value_get_double (value);
		break;

	case PATH_PROP_ROTATION:
		path->rotation = g_value_get_double (value);
		break;

	case PATH_PROP_CLOSED:
		path->closed = g_value_get_boolean (value);
		break;

	case PATH_PROP_FILL_RULE:
		path->fill_rule = g_value_get_boolean (value);
		break;

	case PATH_PROP_PATH:
		if (path->path)
			go_path_free (path->path);
		path->path = go_path_ref (g_value_get_boxed (value));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));
}

static void
goc_path_get_property (GObject *gobject, guint param_id,
				    GValue *value, GParamSpec *pspec)
{
	GocPath *path = GOC_PATH (gobject);

	switch (param_id) {
	case PATH_PROP_X:
		g_value_set_double (value, path->x);
		break;

	case PATH_PROP_Y:
		g_value_set_double (value, path->y);
		break;

	case PATH_PROP_ROTATION:
		g_value_set_double (value, path->rotation);
		break;

	case PATH_PROP_CLOSED:
		g_value_set_boolean (value, path->closed);
		break;

	case PATH_PROP_FILL_RULE:
		g_value_set_boolean (value, path->fill_rule);
		break;

	case PATH_PROP_PATH:
		g_value_set_boxed (value, &path->path);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static gboolean
goc_path_prepare_draw (GocItem const *item, cairo_t *cr, gboolean flag)
{
	GocPath *path = GOC_PATH (item);
	double sign = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)? -1: 1;

	_goc_item_transform (item, cr, flag);
	if (1 == flag) {
		goc_group_cairo_transform (item->parent, cr, path->x , path->y);
	} else {
		cairo_translate (cr, path->x, path->y);
		sign = 1;
	}
	cairo_scale (cr, sign, 1.);
	cairo_rotate (cr, path->rotation * sign);
	go_path_to_cairo (path->path, GO_PATH_DIRECTION_FORWARD, cr);

	return TRUE;
}

static void
goc_path_update_bounds (GocItem *item)
{
	cairo_surface_t *surface;
	cairo_t *cr;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	cairo_save (cr);
	if (goc_path_prepare_draw (item, cr, 0)) {
		goc_styled_item_set_cairo_line (GOC_STYLED_ITEM (item), cr);
		cairo_restore (cr);
		cairo_stroke_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);
	}

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
}

static double
goc_path_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocPath *path = GOC_PATH (item);
	GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
	double tmp_width = 0;
	double res = 20;
	double ppu = goc_canvas_get_pixels_per_unit (item->canvas);
	cairo_surface_t *surface;
	cairo_t *cr;
	gboolean set_line,
		scale_line_width = goc_styled_item_get_scale_line_width (GOC_STYLED_ITEM (item));

	*near_item = item;
	tmp_width = style->line.width;
	if (style->line.width * ppu < 5)
		style->line.width = 5. / (ppu * ppu);
	else
		style->line.width /= ppu;
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	cairo_save (cr);
	if (goc_path_prepare_draw (item, cr, 0)) {
		/* Filled OR both fill and stroke are none */
		if (!scale_line_width)
			cairo_restore (cr);
		set_line = go_styled_object_set_cairo_line (GO_STYLED_OBJECT (item), cr);
		if (scale_line_width)
			cairo_restore (cr);
		if ((path->closed && style->fill.type != GO_STYLE_FILL_NONE) ||
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
goc_path_draw (GocItem const *item, cairo_t *cr)
{
	GocPath *path = GOC_PATH (item);
	gboolean scale_line_width = goc_styled_item_get_scale_line_width (GOC_STYLED_ITEM (item));
	gboolean needs_restore;

	cairo_save(cr);
	needs_restore = TRUE;
	cairo_set_fill_rule (cr, path->fill_rule? CAIRO_FILL_RULE_EVEN_ODD: CAIRO_FILL_RULE_WINDING);
	if (goc_path_prepare_draw (item, cr, 1)) {
		if (path->closed)
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
goc_path_copy (GocItem *dest, GocItem *source)
{
	GocPath *src = GOC_PATH (source), *dst = GOC_PATH (dest);

	dst->rotation = src->rotation;
	dst->x = src->x;
	dst->y = src->y;
	dst->closed = src->closed;
	dst->fill_rule = src->fill_rule;
	dst->path = go_path_copy (src->path);
	((GocItemClass *) parent_class)->copy (dest, source);
}

static void
goc_path_init_style (G_GNUC_UNUSED GocStyledItem *item, GOStyle *style)
{
	GocPath *path = GOC_PATH (item);

	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL;
	if (style->line.auto_dash)
		style->line.dash_type = GO_LINE_SOLID;
	if (style->line.auto_color)
		style->line.color = GO_COLOR_BLACK;
	if (style->line.auto_fore)
		style->line.fore  = 0;
	if (path->closed) {
		if (style->fill.auto_type)
			style->fill.type  = GO_STYLE_FILL_PATTERN;
		if (style->fill.auto_fore)
			style->fill.pattern.fore = GO_COLOR_BLACK;
		if (style->fill.auto_back)
			style->fill.pattern.back = GO_COLOR_WHITE;
	}
}

static void
goc_path_finalize (GObject *obj)
{
	GocPath *path = GOC_PATH (obj);
	if (path->path)
		go_path_free (path->path);
	parent_class->finalize (obj);
}

static void
goc_path_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	GocStyledItemClass *gsi_klass = (GocStyledItemClass *) item_klass;
	parent_class = g_type_class_peek_parent (item_klass);

	gsi_klass->init_style = goc_path_init_style;

	obj_klass->finalize = goc_path_finalize;
	obj_klass->get_property = goc_path_get_property;
	obj_klass->set_property = goc_path_set_property;
	g_object_class_install_property (obj_klass, PATH_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The path first point x coordinate"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, PATH_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The path first point y coordinate"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, PATH_PROP_ROTATION,
		g_param_spec_double ("rotation",
			_("Rotation"),
			_("The rotation around first point position"),
			0., 2 * M_PI, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, PATH_PROP_CLOSED,
		g_param_spec_boolean ("closed",
			_("Closed"),
			_("The flag for closed path"),
			FALSE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, PATH_PROP_FILL_RULE,
		g_param_spec_boolean ("fill-rule",
				      _("Fill rule"),
				      _("Set fill rule to winding or even/odd"),
				      FALSE,
				      GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, PATH_PROP_PATH,
		g_param_spec_boxed ("path",
			_("Path"),
			_("The path points"),
			GO_TYPE_PATH,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	item_klass->update_bounds = goc_path_update_bounds;
	item_klass->distance = goc_path_distance;
	item_klass->draw = goc_path_draw;
	item_klass->copy = goc_path_copy;
}

GSF_CLASS (GocPath, goc_path,
	   goc_path_class_init, NULL,
	   GOC_TYPE_STYLED_ITEM)
