/*
 * goc-component.c :
 *
 * Copyright (C) 2010 Jean Brefort (jean.brefort@normalesup.org)
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
 * SECTION:goc-component
 * @short_description: Components.
 *
 * #GocComponent implements #GOComponent embedding in the canvas.
**/

struct _GocComponent {
	GocItem base;

	double x, y, w, h;
	double rotation;
	GOComponent *component;
};

typedef GocItemClass GocComponentClass;

static GObjectClass *parent_klass;

enum {
	COMPONENT_PROP_0,
	COMPONENT_PROP_X,
	COMPONENT_PROP_Y,
	COMPONENT_PROP_H,
	COMPONENT_PROP_W,
	COMPONENT_PROP_ROTATION,
	COMPONENT_PROP_OBJECT
};

static void
goc_component_set_property (GObject *obj, guint param_id,
			GValue const *value, GParamSpec *pspec)
{
	GocComponent *component = GOC_COMPONENT (obj);

	switch (param_id) {
	case COMPONENT_PROP_X: {
		double x = g_value_get_double (value);
		if (x == component->x)
			return;
		component->x = x;
		return;
	}
	case COMPONENT_PROP_Y: {
		double y = g_value_get_double (value);
		if (y == component->y)
			return;
		component->y = y;
		break;
	}
	/* NOTE: it is allowed to set width and height even if the component is not resizable
	 * but this should be only used to convert the size in inches to pixels.
	 * The default is 1 pixel == 1 point.
	 */
	case COMPONENT_PROP_H: {
		double h = g_value_get_double (value);
		if (!component->component || h == component->h)
			return;
		component->h = h;
		break;
	}
	case COMPONENT_PROP_W: {
		double w = g_value_get_double (value);
		if (!component->component || w == component->w)
			return;
		component->w = w;
		break;
	}
	case COMPONENT_PROP_ROTATION: {
		double r = g_value_get_double (value) / 180. * M_PI;
		if (!component->component || r == component->rotation)
			return;
		component->rotation = r;
		break;
	}

	case COMPONENT_PROP_OBJECT:
		if (component->component != NULL)
			g_object_unref (component->component);
		component->component = GO_COMPONENT (g_value_get_object (value));
		if (component->component != NULL) {
			g_object_ref (component->component);
			/* set default or fixed size */
			if (component->w == 0 || component->h == 0 || !go_component_is_resizable (component->component)) {
				go_component_get_size (component->component, &component->w, &component->h);
				component->w = GO_IN_TO_PT (component->w);
				component->h = GO_IN_TO_PT (component->h);
			}
		}
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}

	goc_item_bounds_changed (GOC_ITEM (component));
}

static void
goc_component_get_property (GObject *obj, guint param_id,
			GValue *value, GParamSpec *pspec)
{
	GocComponent *component = GOC_COMPONENT (obj);

	switch (param_id) {
	case COMPONENT_PROP_X:
		g_value_set_double (value, component->x);
		break;
	case COMPONENT_PROP_Y:
		g_value_set_double (value, component->y);
		break;
	case COMPONENT_PROP_H:
		g_value_set_double (value, component->h);
		break;
	case COMPONENT_PROP_W:
		g_value_set_double (value, component->w);
		break;
	case COMPONENT_PROP_ROTATION:
		g_value_set_double (value, component->rotation * 180. / M_PI);
		break;
	case COMPONENT_PROP_OBJECT:
		g_value_set_object (value, component->component);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		break;
	}
}

static void
goc_component_finalize (GObject *obj)
{
	GocComponent *component = GOC_COMPONENT (obj);

	if (component->component != NULL) {
		g_object_unref (component->component);
		component->component = NULL;
	}
	(*parent_klass->finalize) (obj);
}

static double
goc_component_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocComponent *component = GOC_COMPONENT (item);
	double dx, dy;
	x -= component->x;
	y -= component->y;
	/* adjust for rotation */
	if (component->rotation != 0.) {
		double sinr = sin (-component->rotation), cosr = cos (component->rotation), t;
		x -= component->w / 2.;
		y -= component->h / 2.;
		t = x * cosr + y * sinr;
		y = y * cosr - x * sinr;
		x = t + component->w / 2.;
		y += component->h / 2.;
	}
	if (x < 0) {
		dx = -x;
	} else if (x < component->w) {
		dx = 0;
	} else {
		dx = x - component->w;
	}
	if (y < 0) {
		dy = -y;
	} else if (y < component->h) {
		dy = 0;
	} else {
		dy = y - component->h;
	}
	*near_item = item;
	return hypot (dx, dy);
}

static void
goc_component_draw (GocItem const *item, cairo_t *cr)
{
	GocComponent *component = GOC_COMPONENT (item);
	GocCanvas *canvas = item->canvas;
	double x0, y0 = component->y;
	if (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL) {
		x0 = component->x + component->w;
		goc_group_adjust_coords (item->parent, &x0, &y0);
		x0 = canvas->width - (int) (x0 - canvas->scroll_x1);
	} else {
		x0 = component->x;
		goc_group_adjust_coords (item->parent, &x0, &y0);
		x0 = (int) (x0 - canvas->scroll_x1);
	}
	cairo_save (cr);
	_goc_item_transform (item, cr, TRUE);
	if (component->rotation == 0.)
		cairo_translate (cr, x0,
			         (int) (y0 - canvas->scroll_y1));
	else {
		cairo_translate (cr, x0 + component->w / 2,
			         (int) (y0 - canvas->scroll_y1 + component->h / 2));
		cairo_rotate (cr, -component->rotation);
		cairo_translate (cr, -component->w / 2, -component->h / 2);
	}
	cairo_rectangle (cr, 0., 0., component->w,
	                     component->h);
	cairo_clip (cr);
	go_component_render (component->component, cr,
	                     component->w,
	                     component->h);
	cairo_restore (cr);
}

static void
goc_component_update_bounds (GocItem *item)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	GocComponent *component = GOC_COMPONENT (item);
	double x0, y0 = component->y;
	GocCanvas *canvas = item->canvas;

	if (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL) {
		x0 = component->x + component->w;
		goc_group_adjust_coords (item->parent, &x0, &y0);
		x0 = canvas->width - (int) (x0 - canvas->scroll_x1) * canvas->pixels_per_unit;
	} else {
		x0 = component->x;
		goc_group_adjust_coords (item->parent, &x0, &y0);
		x0 = (int) (x0 - canvas->scroll_x1) * canvas->pixels_per_unit;
	}
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);
	_goc_item_transform (item, cr, FALSE);
	if (component->rotation == 0.)
		cairo_translate (cr, x0,
			         (int) (y0 - canvas->scroll_y1));
	else {
		cairo_translate (cr, x0 + component->w / 2,
			         (int) (y0 - canvas->scroll_y1 + component->h / 2));
		cairo_rotate (cr, -component->rotation);
		cairo_translate (cr, -component->w / 2,
			         -component->h / 2);
	}
	cairo_rectangle (cr, 0., 0., component->w,
	                     component->h);
	cairo_fill_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
}

static void
goc_component_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;

	parent_klass = g_type_class_peek_parent (obj_klass);

	obj_klass->set_property = goc_component_set_property;
	obj_klass->get_property = goc_component_get_property;
	obj_klass->finalize	= goc_component_finalize;

	g_object_class_install_property (obj_klass, COMPONENT_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The object left position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, COMPONENT_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The object top position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, COMPONENT_PROP_H,
		 g_param_spec_double ("height",
			_("Height"),
			_("Height"),
			0, G_MAXDOUBLE, 100.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, COMPONENT_PROP_W,
		 g_param_spec_double ("width",
			_("Width"),
			_("Width"),
			0, G_MAXDOUBLE, 100.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, COMPONENT_PROP_ROTATION,
		g_param_spec_double ("rotation",
			_("Rotation"),
			_("The rotation around center"),
			0., 360., 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, COMPONENT_PROP_OBJECT,
		g_param_spec_object ("object",
			_("Object"),
			_("The embedded GOComponent object"),
			GO_TYPE_COMPONENT,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->draw = goc_component_draw;
	item_klass->update_bounds = goc_component_update_bounds;
	item_klass->distance = goc_component_distance;
}

GSF_CLASS (GocComponent, goc_component,
	   goc_component_class_init, NULL,
	   GOC_TYPE_ITEM)

/**
 * goc_component_get_object:
 * @component: #GocComponent
 *
 * Returns: (transfer none): the embedded object.
 */

GOComponent * goc_component_get_object (GocComponent *component)
{
	g_return_val_if_fail (GOC_IS_COMPONENT (component), NULL);
	return component->component;
}
