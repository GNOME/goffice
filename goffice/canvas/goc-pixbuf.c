/*
 * goc-pixbuf.c:
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
 * SECTION:goc-pixbuf
 * @short_description: Pixbuf.
 *
 * #GocPixbuf implements image drawing in the canvas.
**/

enum {
	PIXBUF_PROP_0,
	PIXBUF_PROP_X,
	PIXBUF_PROP_Y,
	PIXBUF_PROP_W,
	PIXBUF_PROP_H,
	PIXBUF_PROP_ROTATION,
	PIXBUF_PROP_PIXBUF
};

static GocItemClass *parent_class;

static void
goc_pixbuf_set_property (GObject *gobject, guint param_id,
				    GValue const *value, GParamSpec *pspec)
{
	GocPixbuf *pixbuf = GOC_PIXBUF (gobject);

	switch (param_id) {
	case PIXBUF_PROP_X:
		pixbuf->x = g_value_get_double (value);
		break;

	case PIXBUF_PROP_Y:
		pixbuf->y = g_value_get_double (value);
		break;

	case PIXBUF_PROP_W:
		pixbuf->width = g_value_get_double (value);
		break;

	case PIXBUF_PROP_H:
		pixbuf->height = g_value_get_double (value);
		break;

	case PIXBUF_PROP_ROTATION:
		pixbuf->rotation = g_value_get_double (value);
		break;

	case PIXBUF_PROP_PIXBUF:
		if (pixbuf->pixbuf)
			g_object_unref (pixbuf);
		pixbuf->pixbuf = GDK_PIXBUF (g_object_ref (g_value_get_object (value)));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));

}

static void
goc_pixbuf_get_property (GObject *gobject, guint param_id,
				    GValue *value, GParamSpec *pspec)
{
	GocPixbuf *pixbuf = GOC_PIXBUF (gobject);

	switch (param_id) {
	case PIXBUF_PROP_X:
		g_value_set_double (value, pixbuf->x);
		break;

	case PIXBUF_PROP_Y:
		g_value_set_double (value, pixbuf->y);
		break;

	case PIXBUF_PROP_W:
		g_value_set_double (value, pixbuf->width);
		break;

	case PIXBUF_PROP_H:
		g_value_set_double (value, pixbuf->height);
		break;

	case PIXBUF_PROP_ROTATION:
		g_value_set_double (value, pixbuf->rotation);
		break;

	case PIXBUF_PROP_PIXBUF:
		if (pixbuf->pixbuf)
			g_value_set_object (value, G_OBJECT (pixbuf->pixbuf));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_pixbuf_finalize (GObject *gobject)
{
	GocPixbuf *pixbuf = GOC_PIXBUF (gobject);

	if (pixbuf->pixbuf)
		g_object_unref (pixbuf->pixbuf);
	((GObjectClass *) parent_class)->finalize (gobject);
}

static void
goc_pixbuf_update_bounds (GocItem *item)
{
	GocPixbuf *pixbuf = GOC_PIXBUF (item);
	cairo_surface_t *surface;
	cairo_t *cr;

	if (!pixbuf->pixbuf)
		return;
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);
	cr = cairo_create (surface);

	cairo_save (cr);
	_goc_item_transform (item, cr, FALSE);
	cairo_rectangle (cr, pixbuf->x, pixbuf->y,
	                 ((pixbuf->width > 0.)? pixbuf->width: gdk_pixbuf_get_width (pixbuf->pixbuf)),
	                 ((pixbuf->height > 0.)? pixbuf->height: gdk_pixbuf_get_height (pixbuf->pixbuf)));
	cairo_restore (cr);
	cairo_fill_extents (cr, &item->x0, &item->y0, &item->x1, &item->y1);

	cairo_destroy (cr);
	cairo_surface_destroy (surface);
}

static double
goc_pixbuf_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocPixbuf *pixbuf = GOC_PIXBUF (item);
	/* FIXME: take rotation into account */
	double dx, dy, w, h;
	if (pixbuf->pixbuf == NULL)
		return G_MAXDOUBLE;
	w = (pixbuf->width >= 0.)? pixbuf->width: gdk_pixbuf_get_width (pixbuf->pixbuf);
	h = (pixbuf->height >= 0.)? pixbuf->height: gdk_pixbuf_get_height (pixbuf->pixbuf);
	if (x < pixbuf->x) {
		dx = pixbuf->x - x;
	} else if (x < pixbuf->x + w) {
		dx = 0;
	} else {
		dx = x - pixbuf->x - w;
	}
	if (y < pixbuf->y) {
		dy = pixbuf->y - y;
	} else if (y < pixbuf->y + h) {
		dy = 0;
	} else {
		dy = y - pixbuf->y - h;
	}
	*near_item = item;
	return hypot (dx, dy);
}

static void
goc_pixbuf_draw (GocItem const *item, cairo_t *cr)
{
	GocPixbuf *pixbuf = GOC_PIXBUF (item);
	GOImage * image;
	double height, width;
	double scalex = 1., scaley = 1.;
	int x;

	if (pixbuf->pixbuf == NULL || pixbuf->width == 0. || pixbuf->height == 0.)
		return;

	image = go_pixbuf_new_from_pixbuf (pixbuf->pixbuf);
	if (pixbuf->width < 0.)
		width = gdk_pixbuf_get_width (pixbuf->pixbuf);
	else {
		width = pixbuf->width;
		scalex = width / gdk_pixbuf_get_width (pixbuf->pixbuf);
	}
	if (pixbuf->height < 0.)
		height = gdk_pixbuf_get_height (pixbuf->pixbuf);
	else {
		height = pixbuf->height;
		scaley = height / gdk_pixbuf_get_height (pixbuf->pixbuf);
	}
	cairo_save (cr);
	_goc_item_transform (item, cr, TRUE);
	x = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)?
		pixbuf->x + pixbuf->width: pixbuf->x;
	goc_group_cairo_transform (item->parent, cr, x, (int) pixbuf->y);
	cairo_rotate (cr, pixbuf->rotation);
	if (scalex != 1. || scaley != 1.)
		cairo_scale (cr, scalex, scaley);
	cairo_move_to (cr, 0, 0);
	go_image_draw (image, cr);
	cairo_restore (cr);
	g_object_unref (image);
}

static void
goc_pixbuf_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	parent_class = g_type_class_peek_parent (item_klass);

	obj_klass->finalize = goc_pixbuf_finalize;
	obj_klass->get_property = goc_pixbuf_get_property;
	obj_klass->set_property = goc_pixbuf_set_property;
	g_object_class_install_property (obj_klass, PIXBUF_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The image left position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, PIXBUF_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The image top position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, PIXBUF_PROP_W,
		g_param_spec_double ("width",
			_("Width"),
			_("The image width or -1 to use the image width"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, PIXBUF_PROP_H,
		g_param_spec_double ("height",
			_("Height"),
			_("The image height or -1 to use the image height"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
/*	g_object_class_install_property (obj_klass, PIXBUF_PROP_ROTATION,
		g_param_spec_double ("rotation",
			_("Rotation"),
			_("The rotation around top left position"),
			0., 2 * M_PI, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));*/
	g_object_class_install_property (obj_klass, PIXBUF_PROP_PIXBUF,
	        g_param_spec_object ("pixbuf", _("Pixbuf"),
	                 _("The GdkPixbuf to display"),
	                 GDK_TYPE_PIXBUF,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->update_bounds = goc_pixbuf_update_bounds;
	item_klass->distance = goc_pixbuf_distance;
	item_klass->draw = goc_pixbuf_draw;
}

GSF_CLASS (GocPixbuf, goc_pixbuf,
	   goc_pixbuf_class_init, NULL,
	   GOC_TYPE_ITEM)
