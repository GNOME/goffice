/*
 * goc-image.c:
 *
 * Copyright (C) 2011 Jean Brefort (jean.brefort@normalesup.org)
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
 * SECTION:goc-image
 * @short_description: Image.
 *
 * #GocImage implements image drawing in the canvas.
**/

enum {
	IMAGE_PROP_0,
	IMAGE_PROP_X,
	IMAGE_PROP_Y,
	IMAGE_PROP_W,
	IMAGE_PROP_H,
	IMAGE_PROP_ROTATION,
	IMAGE_PROP_IMAGE,
	IMAGE_PROP_CROP_BOTTOM,
	IMAGE_PROP_CROP_LEFT,
	IMAGE_PROP_CROP_RIGHT,
	IMAGE_PROP_CROP_TOP
};

static GocItemClass *parent_class;

static void
goc_image_set_property (GObject *gobject, guint param_id,
				    GValue const *value, GParamSpec *pspec)
{
	GocImage *image = GOC_IMAGE (gobject);

	switch (param_id) {
	case IMAGE_PROP_X:
		image->x = g_value_get_double (value);
		break;

	case IMAGE_PROP_Y:
		image->y = g_value_get_double (value);
		break;

	case IMAGE_PROP_W:
		image->width = g_value_get_double (value);
		break;

	case IMAGE_PROP_H:
		image->height = g_value_get_double (value);
		break;

	case IMAGE_PROP_ROTATION:
		image->rotation = g_value_get_double (value);
		break;

	case IMAGE_PROP_IMAGE:
		if (image->image)
			g_object_unref (image);
		image->image = GO_IMAGE (g_object_ref (g_value_get_object (value)));
		break;

	case IMAGE_PROP_CROP_BOTTOM:
		image->crop_bottom = g_value_get_double (value);
		break;
	case IMAGE_PROP_CROP_LEFT:
		image->crop_left = g_value_get_double (value);
		break;
	case IMAGE_PROP_CROP_RIGHT:
		image->crop_right = g_value_get_double (value);
		break;
	case IMAGE_PROP_CROP_TOP:
		image->crop_top = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	goc_item_bounds_changed (GOC_ITEM (gobject));

}

static void
goc_image_get_property (GObject *gobject, guint param_id,
				    GValue *value, GParamSpec *pspec)
{
	GocImage *image = GOC_IMAGE (gobject);

	switch (param_id) {
	case IMAGE_PROP_X:
		g_value_set_double (value, image->x);
		break;

	case IMAGE_PROP_Y:
		g_value_set_double (value, image->y);
		break;

	case IMAGE_PROP_W:
		g_value_set_double (value, image->width);
		break;

	case IMAGE_PROP_H:
		g_value_set_double (value, image->height);
		break;

	case IMAGE_PROP_ROTATION:
		g_value_set_double (value, image->rotation);
		break;

	case IMAGE_PROP_IMAGE:
		if (image->image)
			g_value_set_object (value, G_OBJECT (image->image));
		break;

	case IMAGE_PROP_CROP_BOTTOM:
		g_value_set_double (value, image->crop_bottom);
		break;

	case IMAGE_PROP_CROP_LEFT:
		g_value_set_double (value, image->crop_left);
		break;

	case IMAGE_PROP_CROP_RIGHT:
		g_value_set_double (value, image->crop_right);
		break;

	case IMAGE_PROP_CROP_TOP:
		g_value_set_double (value, image->crop_top);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_image_finalize (GObject *gobject)
{
	GocImage *image = GOC_IMAGE (gobject);

	if (image->image)
		g_object_unref (image->image);
	((GObjectClass *) parent_class)->finalize (gobject);
}

static void
goc_image_update_bounds (GocItem *item)
{
	GocImage *image = GOC_IMAGE (item);
	double w, h;
	if (!image->image)
		return;
	/* FIXME: take rotation into account */
	w = go_image_get_width (image->image) - image->crop_left - image->crop_right;
	h = go_image_get_height (image->image) - image->crop_top - image->crop_bottom;
	if (w <= 0 || h <= 0) {
		/* nothing visible, put it at origin */
		item->x0 = item->x1 = image->x;
		item->y0 = item->y1 = image->y;
	}
	item->x0 = floor (image->x);
	item->y0 = floor (image->y);
	item->x1 = ceil (image->x + ((image->width > 0.)? image->width: w));
	item->y1 = ceil (image->y + ((image->height > 0.)? image->height: h));
}

static double
goc_image_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocImage *image = GOC_IMAGE (item);
	/* FIXME: take rotation into account */
	double dx, dy;
	if (image->image == NULL)
		return G_MAXDOUBLE;
	if (x < item->x0) {
		dx = item->x0 - x;
	} else if (x < item->x1) {
		dx = 0;
	} else {
		dx = x - item->x1;
	}
	if (y < item->y0) {
		dy = item->y0 - y;
	} else if (y < item->y1) {
		dy = 0;
	} else {
		dy = y - item->y1;
	}
	*near_item = item;
	return hypot (dx, dy);
}

static void
goc_image_draw (GocItem const *item, cairo_t *cr)
{
	GocImage *image = GOC_IMAGE (item);
	double height, width, iw, ih;
	double scalex = 1., scaley = 1.;
	int x;

	if (image->image == NULL || image->width == 0. || image->height == 0.)
		return;

	iw = go_image_get_width (image->image);
	ih = go_image_get_height (image->image);
	if (iw == 0. || ih == 0.)
	    return;

	if (image->width < 0.)
		width = iw * (1 - image->crop_left -image->crop_right);
	else {
		width = image->width;
		scalex = width / iw / (1 - image->crop_left -image->crop_right);
	}
	if (image->height < 0.)
		height = ih * (1 - image->crop_top -image->crop_bottom);
	else {
		height = image->height;
		scaley = height / ih / (1 - image->crop_top -image->crop_bottom);
	}

	cairo_save (cr);
	_goc_item_transform (item, cr, TRUE);
	x = (item->canvas && goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL)?
		image->x + image->width: image->x;
	goc_group_cairo_transform (item->parent, cr, x, (int) image->y);
	cairo_rotate (cr, image->rotation);
	cairo_rectangle (cr, 0, 0, image->width, image->height);
	cairo_clip (cr);
	if (scalex != 1. || scaley != 1.)
		cairo_scale (cr, scalex, scaley);
	cairo_translate (cr, -go_image_get_width (image->image) * image->crop_left,
	                 -go_image_get_height (image->image) * image->crop_top);
	cairo_move_to (cr, 0, 0);
	go_image_draw (image->image, cr);
	cairo_restore (cr);
}

static void
goc_image_copy (GocItem *dest, GocItem *source)
{
	GocImage *src = GOC_IMAGE (source), *dst = GOC_IMAGE (dest);

	dst->x = src->x;
	dst->y = src->y;
	dst->width = src->width;
	dst->height = src->height;
	dst->rotation = src->rotation;
	dst->crop_left = src->crop_left;
	dst->crop_right = src->crop_right;
	dst->crop_top = src->crop_top;
	dst->crop_bottom = src->crop_bottom;
	/* just add a reference to the GOImage, it should never be modified */
	dst->image = g_object_ref (src->image);
}

static void
goc_image_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;
	parent_class = g_type_class_peek_parent (item_klass);

	obj_klass->finalize = goc_image_finalize;
	obj_klass->get_property = goc_image_get_property;
	obj_klass->set_property = goc_image_set_property;
	g_object_class_install_property (obj_klass, IMAGE_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The image left position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, IMAGE_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The image top position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, IMAGE_PROP_W,
		g_param_spec_double ("width",
			_("Width"),
			_("The image width or -1 to use the image width"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, IMAGE_PROP_H,
		g_param_spec_double ("height",
			_("Height"),
			_("The image height or -1 to use the image height"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
/*	g_object_class_install_property (obj_klass, IMAGE_PROP_ROTATION,
		g_param_spec_double ("rotation",
			_("Rotation"),
			_("The rotation around top left position"),
			0., 2 * M_PI, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));*/
	g_object_class_install_property (obj_klass, IMAGE_PROP_IMAGE,
	        g_param_spec_object ("image", _("Image"),
	                _("The GOImage to display"),
	                GO_TYPE_IMAGE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, IMAGE_PROP_CROP_BOTTOM,
	        g_param_spec_double ("crop-bottom", _("Cropped bottom"),
	                _("The cropped area at the image bottom as a fraction of the image height"),
	                0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, IMAGE_PROP_CROP_LEFT,
	        g_param_spec_double ("crop-left", _("Cropped left"),
	                _("The cropped area at the image left of the image width"),
	                0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, IMAGE_PROP_CROP_RIGHT,
	        g_param_spec_double ("crop-right", _("Cropped right"),
	                _("The cropped area at the image right of the image width"),
	                0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, IMAGE_PROP_CROP_TOP,
	        g_param_spec_double ("crop-top", _("Cropped top"),
	                _("The cropped area at the image top as a fraction of the image height"),
	                0., G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->update_bounds = goc_image_update_bounds;
	item_klass->distance = goc_image_distance;
	item_klass->draw = goc_image_draw;
	item_klass->copy = goc_image_copy;
}

GSF_CLASS (GocImage, goc_image,
	   goc_image_class_init, NULL,
	   GOC_TYPE_ITEM)
