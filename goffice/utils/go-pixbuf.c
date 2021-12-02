/*
 * go-pixbuf.c
 *
 * Copyright (C) 2011 Jean Brefort (jean.brefort@normalesup.org)
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
#include "go-pixbuf.h"

#include <goffice/goffice-priv.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/* GOPixbuf implementation */

struct _GOPixbuf {
	GOImage parent;
	unsigned rowstride;
	GdkPixbuf *pixbuf;
	cairo_surface_t *surface;
	guint8 *cairo_pixels;
	gsize length;
	char *type;
};

typedef GOImageClass GOPixbufClass;

static GObjectClass *parent_klass;

enum {
	PIXBUF_PROP_0,
	PIXBUF_PROP_PIXBUF,
	PIXBUF_PROP_TYPE
};

static void
pixbuf_to_cairo (GOPixbuf *pixbuf)
{
	unsigned char *src, *dst;
	GOImage *image = GO_IMAGE (pixbuf);

	g_return_if_fail (GO_IS_PIXBUF (pixbuf) && pixbuf->cairo_pixels && pixbuf->pixbuf);

	src = gdk_pixbuf_get_pixels (pixbuf->pixbuf);
	dst = pixbuf->cairo_pixels;

	g_return_if_fail (gdk_pixbuf_get_rowstride (pixbuf->pixbuf) == (int) pixbuf->rowstride);

	go_cairo_convert_data_from_pixbuf (dst, src, image->width, image->height, pixbuf->rowstride);
}

static void
cairo_to_pixbuf (GOPixbuf *pixbuf)
{
	unsigned char *src, *dst;
	GOImage *image = GO_IMAGE (pixbuf);

	g_return_if_fail (GO_IS_PIXBUF (pixbuf) && pixbuf->cairo_pixels && pixbuf->pixbuf);

	dst = gdk_pixbuf_get_pixels (pixbuf->pixbuf);
	src = pixbuf->cairo_pixels;

	g_return_if_fail (gdk_pixbuf_get_rowstride (pixbuf->pixbuf) == (int) pixbuf->rowstride);

	go_cairo_convert_data_to_pixbuf (dst, src, image->width, image->height, pixbuf->rowstride);
}

static void
go_pixbuf_save (GOImage *image, GsfXMLOut *output)
{
	GOPixbuf *pixbuf;
	g_return_if_fail (GO_IS_PIXBUF (image) && image->data != NULL);
	pixbuf = GO_PIXBUF (image);
	if (pixbuf->type == NULL) {
		pixbuf->type = g_strdup ("png");
		g_free (image->data); /* just in case, but the pointer should be NULL */
		gdk_pixbuf_save_to_buffer (pixbuf->pixbuf, (gchar **) &image->data,
		                           &image->data_length, pixbuf->type, NULL,
		                           "compression", 9, NULL);
	}
	gsf_xml_out_add_cstr_unchecked (output, "image-type", pixbuf->type);
	gsf_xml_out_add_base64 (output, NULL,
	                        image->data, image->data_length);
}

static void
go_pixbuf_load_attr (GOImage *image, xmlChar const *attr_name, xmlChar const *attr_value)
{
	GOPixbuf *pixbuf = GO_PIXBUF (image);
	g_return_if_fail (pixbuf);
	if (!strcmp (attr_name, "rowstride")) {
		long l = strtol (attr_value, NULL, 10);
		g_return_if_fail (l > 0 && l < G_MAXINT);
		pixbuf->rowstride = l;
	} else if (!strcmp (attr_name, "image-type")) {
		g_free (pixbuf->type);
		pixbuf->type = g_strdup (attr_value);
	}
}

static void
go_pixbuf_create_pixbuf (GOPixbuf *pixbuf, GError **error)
{
	GOImage *image = GO_IMAGE (pixbuf);
	GdkPixbufLoader *loader = gdk_pixbuf_loader_new_with_type (pixbuf->type, NULL);
	if (loader) {
		if (gdk_pixbuf_loader_write (loader, image->data, image->data_length, error))
			g_object_set (pixbuf,
			              "pixbuf", gdk_pixbuf_loader_get_pixbuf (loader),
			              NULL);
		gdk_pixbuf_loader_close (loader, error && !*error ? error : NULL);
		g_object_unref (loader);
	}
}

static void
go_pixbuf_restore_data (GOPixbuf *pixbuf)
{
	GOImage *image = GO_IMAGE (pixbuf);
	if (pixbuf->pixbuf == NULL) {
		if (image->width == 0 || image->height == 0 || pixbuf->cairo_pixels == NULL)
			return;
		pixbuf->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
		                                 image->width, image->height);
		cairo_to_pixbuf (pixbuf);
	}
	if (image->data == NULL && pixbuf->pixbuf && pixbuf->type)
		gdk_pixbuf_save_to_buffer (pixbuf->pixbuf, (gchar **) &image->data,
				                   &image->data_length, pixbuf->type, NULL,
				                   NULL);
}

static void
go_pixbuf_load_data (GOImage *image, GsfXMLIn *xin)
{
	size_t length, expected;
	int stride;
	GOPixbuf *pixbuf = GO_PIXBUF (image);

	stride = go_pixbuf_get_rowstride (GO_PIXBUF (image));
	length = gsf_base64_decode_simple (xin->content->str, strlen(xin->content->str));
	if (stride > 0) {
		/* load raw pixels */
		expected = image->height * (size_t)stride;
		if (expected != length)
			g_critical ("Invalid image size, expected %" G_GSIZE_FORMAT " bytes, got %" G_GSIZE_FORMAT,
					expected, length);
		pixbuf->cairo_pixels = g_try_malloc (expected);
		pixbuf->length = expected;
		if (pixbuf->cairo_pixels == NULL) {
			g_critical ("go_pixbuf_load_data: assertion `pixbuf->cairo_pixels != NULL' failed");
			return;
		}
		memcpy (pixbuf->cairo_pixels, xin->content->str, MIN (length, expected));
		if (length < expected) /* fill with 0 */
			memset (pixbuf->cairo_pixels + length, 0, expected - length);
		go_pixbuf_restore_data (pixbuf);
	} else {
		image->data = g_try_malloc (length);
		image->data_length = length;
		if (image->data == NULL) {
			g_critical ("go_pixbuf_load_data: assertion `image->data != NULL' failed");
			return;
		}
		memcpy (image->data, xin->content->str, length);
		/* create the pixbuf */
		go_pixbuf_create_pixbuf (pixbuf, NULL);
	}
}

static void
go_pixbuf_draw (GOImage *image, cairo_t *cr)
{
	GOPixbuf *pixbuf = GO_PIXBUF (image);
	g_return_if_fail (pixbuf);
	if (pixbuf->surface == NULL) {
		if (pixbuf->cairo_pixels == NULL) {
			/* image built from a pixbuf */
			pixbuf->cairo_pixels = g_try_new0 (guint8, image->height * pixbuf->rowstride);
			if (pixbuf->cairo_pixels == NULL) {
				g_critical ("go_pixbuf_load_data: assertion `image->data != NULL' failed");
				return;
			}
			pixbuf_to_cairo (pixbuf);
		}
		pixbuf->surface = cairo_image_surface_create_for_data (pixbuf->cairo_pixels,
			                                               CAIRO_FORMAT_ARGB32,
			                                               image->width,
			                                               image->height,
			                                               pixbuf->rowstride);
	}
	cairo_save (cr);
	cairo_set_source_surface (cr, pixbuf->surface, 0., 0.);
	cairo_rectangle (cr, 0., 0., image->width, image->height);
	cairo_fill (cr);
	cairo_restore (cr);
}

static GdkPixbuf *
go_pixbuf_get_pixbuf (GOImage *image)
{
	GOPixbuf *pixbuf = GO_PIXBUF (image);
	g_return_val_if_fail (pixbuf, NULL);
	if (!pixbuf->pixbuf) {
		if (image->width == 0 || image->height == 0 || pixbuf->cairo_pixels == NULL)
			return NULL;
		pixbuf->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
								image->width, image->height);
		cairo_to_pixbuf (pixbuf);
	}
	return g_object_ref (pixbuf->pixbuf);
}

static GdkPixbuf *
go_pixbuf_get_scaled_pixbuf (GOImage *image, int width, int height)
{
	GOPixbuf *pixbuf = GO_PIXBUF (image);
	g_return_val_if_fail (pixbuf, NULL);
	if (!pixbuf->pixbuf) {
		if (image->width == 0 || image->height == 0 || image->data == NULL)
			return NULL;
		pixbuf->pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
								image->width, image->height);
		cairo_to_pixbuf (pixbuf);
	}
	return gdk_pixbuf_scale_simple (pixbuf->pixbuf, width, height, GDK_INTERP_HYPER);
}

static gboolean
go_pixbuf_differ (GOImage *first, GOImage *second)
{
	void *pixels1, *pixels2;
	int size;
	GOPixbuf *pfirst = GO_PIXBUF (first), *psecond = GO_PIXBUF (second);
	if (!pfirst->pixbuf)
		go_pixbuf_get_pixbuf (first);
	if (!psecond->pixbuf)
		go_pixbuf_get_pixbuf (second);
	if (!pfirst->pixbuf || !psecond->pixbuf)
		return TRUE; /* this should not happen */
	if (gdk_pixbuf_get_n_channels (pfirst->pixbuf) != gdk_pixbuf_get_n_channels (psecond->pixbuf))
		return TRUE;
	if (gdk_pixbuf_get_colorspace (pfirst->pixbuf) != gdk_pixbuf_get_colorspace (psecond->pixbuf))
		return TRUE;
	if (gdk_pixbuf_get_bits_per_sample (pfirst->pixbuf) != gdk_pixbuf_get_bits_per_sample (psecond->pixbuf))
		return TRUE;
	if (gdk_pixbuf_get_has_alpha (pfirst->pixbuf) != gdk_pixbuf_get_has_alpha (psecond->pixbuf))
		return TRUE;
	if (gdk_pixbuf_get_width (pfirst->pixbuf) != gdk_pixbuf_get_width (psecond->pixbuf))
		return TRUE;
	if (gdk_pixbuf_get_height (pfirst->pixbuf) != gdk_pixbuf_get_height (psecond->pixbuf))
		return TRUE;
	if (gdk_pixbuf_get_rowstride (pfirst->pixbuf) != gdk_pixbuf_get_rowstride (psecond->pixbuf))
		return TRUE;
	pixels1 = gdk_pixbuf_get_pixels (pfirst->pixbuf);
	pixels2 = gdk_pixbuf_get_pixels (psecond->pixbuf);
	size = gdk_pixbuf_get_rowstride (pfirst->pixbuf) * gdk_pixbuf_get_height (pfirst->pixbuf);
	return memcmp (pixels1, pixels2, size);
}

static void
go_pixbuf_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GOPixbuf *pixbuf = GO_PIXBUF (obj);
	GOImage *image = GO_IMAGE (obj);

	switch (param_id) {
	case PIXBUF_PROP_PIXBUF: {
		GdkPixbuf *pix = GDK_PIXBUF (g_value_get_object (value));
		if (!GDK_IS_PIXBUF (pix))
			break;
		if (!gdk_pixbuf_get_has_alpha (pix))
			pix = gdk_pixbuf_add_alpha (pix, FALSE, 0, 0, 0);
		else
			g_object_ref (pix);
		if (pixbuf->pixbuf)
			g_object_unref (pixbuf->pixbuf);
		pixbuf->pixbuf = pix;
		_go_image_changed (image, gdk_pixbuf_get_width (pix), gdk_pixbuf_get_height (pix));
		pixbuf->rowstride = gdk_pixbuf_get_rowstride (pix); /* this should be in GOPixbuf */
	}
		break;
	case PIXBUF_PROP_TYPE: {
		char const *type = g_value_get_string (value);
		if (pixbuf->type && !strcmp (type, pixbuf->type))
			break;
		g_return_if_fail (pixbuf->type == NULL);
		pixbuf->type = g_strdup (type);
		go_pixbuf_restore_data (pixbuf);
	}
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
go_pixbuf_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GOPixbuf *pixbuf = GO_PIXBUF (obj);

	switch (param_id) {
	case PIXBUF_PROP_PIXBUF:
		g_value_set_object (value, pixbuf->pixbuf);
		break;
	case PIXBUF_PROP_TYPE:
		g_value_set_string (value, pixbuf->type);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
go_pixbuf_finalize (GObject *obj)
{
	GOPixbuf *pixbuf = GO_PIXBUF (obj);
	if (pixbuf->pixbuf)
		g_object_unref (pixbuf->pixbuf);
	if (pixbuf->surface)
		cairo_surface_destroy (pixbuf->surface);
	g_free (pixbuf->cairo_pixels);
	g_free (pixbuf->type);
	(parent_klass->finalize) (obj);
}

static void
go_pixbuf_class_init (GObjectClass *klass)
{
	GOImageClass *image_klass = (GOImageClass *) klass;

	klass->finalize = go_pixbuf_finalize;
	klass->set_property = go_pixbuf_set_property;
	klass->get_property = go_pixbuf_get_property;
	parent_klass = g_type_class_peek_parent (klass);

	image_klass->save = go_pixbuf_save;
	image_klass->load_attr = go_pixbuf_load_attr;
	image_klass->load_data = go_pixbuf_load_data;
	image_klass->get_pixbuf = go_pixbuf_get_pixbuf;
	image_klass->get_scaled_pixbuf = go_pixbuf_get_scaled_pixbuf;
	image_klass->draw = go_pixbuf_draw;
	image_klass->differ = go_pixbuf_differ;

	g_object_class_install_property (klass, PIXBUF_PROP_PIXBUF,
					 g_param_spec_object ("pixbuf", _("Pixbuf"),
							      _("GdkPixbuf object from which the GOPixbuf is built"),
							      GDK_TYPE_PIXBUF, G_PARAM_READWRITE));
	g_object_class_install_property (klass, PIXBUF_PROP_TYPE,
		 g_param_spec_string ("image-type",
				      N_("Image type"),
				      N_("Type of image"),
				      NULL,
				      GSF_PARAM_STATIC | G_PARAM_READWRITE));
}

GSF_CLASS (GOPixbuf, go_pixbuf,
	   go_pixbuf_class_init, NULL,
	   GO_TYPE_IMAGE)


GOImage *
go_pixbuf_new_from_pixbuf (GdkPixbuf *pixbuf)
{
	return g_object_new (GO_TYPE_PIXBUF, "pixbuf", pixbuf, NULL);
}

GOImage *
go_pixbuf_new_from_data (char const *type, guint8 const *data, gsize length, GError **error)
{
	GOImage *image = g_object_new (GO_TYPE_PIXBUF, NULL);
	GError *dummy = NULL;

	if (!error)
		error = &dummy;

	image->data = go_memdup (data, length);
	image->data_length = length;
	g_object_set (image, "image-type", type, NULL);
	go_pixbuf_create_pixbuf (GO_PIXBUF (image), error);
	if (*error != NULL) {
		g_object_unref (image);
		image = NULL;
	}

	if (dummy)
		g_error_free (dummy);

	return image;
}

int
go_pixbuf_get_rowstride (GOPixbuf *pixbuf)
{
	g_return_val_if_fail (GO_IS_PIXBUF (pixbuf), 0);
	return pixbuf->rowstride;
}
