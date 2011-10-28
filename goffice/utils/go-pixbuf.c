/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-pixbuf.c
 *
 * Copyright (C) 2000-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2011 Jean Brefort (jean.brefort@normalesup.org)
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
#include "go-pixbuf.h"

#include <goffice/goffice-priv.h>
#include <string.h>
#include <gsf/gsf-utils.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/**
 * go_gdk_pixbuf_intelligent_scale:
 * @buf: a #GdkPixbuf
 * @width: new width
 * @height: new height
 *
 * Intelligent pixbuf scaling.
 *
 * returns: a new GdkPixbuf reference.
 **/

GdkPixbuf *
go_gdk_pixbuf_intelligent_scale (GdkPixbuf *buf, guint width, guint height)
{
	GdkPixbuf *scaled;
	int w, h;
	unsigned long int ow = gdk_pixbuf_get_width (buf);
	unsigned long int oh = gdk_pixbuf_get_height (buf);

	if (ow > width || oh > height) {
		if (ow * height > oh * width) {
			w = width;
			h = width * (((double)oh)/(double)ow);
		} else {
			h = height;
			w = height * (((double)ow)/(double)oh);
		}

		scaled = gdk_pixbuf_scale_simple (buf, w, h, GDK_INTERP_BILINEAR);
	} else
		scaled = g_object_ref (buf);

	return scaled;
}

/**
 * go_gdk_pixbuf_new_from_file:
 * @filename : pixbuf filename
 *
 * Utility routine to create pixbufs from file @name in the goffice_icon_dir.
 *
 * Returns: a GdkPixbuf that the caller is responsible for.
 **/

GdkPixbuf *
go_gdk_pixbuf_new_from_file (char const *filename)
{
	char *path;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (filename != NULL, NULL);

	if (g_path_is_absolute (filename))
		path = g_strdup (filename);
	else
		path = g_build_filename (go_sys_icon_dir (), filename, NULL);

	pixbuf = gdk_pixbuf_new_from_file (path, NULL);

	g_free (path);

	return pixbuf;
}

/**
 * go_gdk_pixbuf_get_from_cache:
 * @filename: pixbuf filename
 *
 * Retrieves a pixbuf from the image cache, loading it from the file
 * @filename located in goffice_icon_dir if not cached yet.
 *
 * returns: a GdkPixbuf, NULL on error.
 **/

GdkPixbuf *
go_gdk_pixbuf_get_from_cache (char const *filename)
{
	static GHashTable *cache = NULL;
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (filename != NULL, NULL);

	if (cache != NULL) {
		pixbuf = g_hash_table_lookup (cache, filename);
		if (pixbuf != NULL)
			return pixbuf;
	} else
		cache = g_hash_table_new_full (g_str_hash, g_str_equal,
					       NULL, g_object_unref);

	pixbuf = go_gdk_pixbuf_new_from_file (filename);
	if (pixbuf != NULL)
		g_hash_table_insert (cache, (gpointer) filename, pixbuf);

	return pixbuf;
}


/**
 * go_gdk_pixbuf_tile:
 * @src: source pixbuf
 * @w: desired width
 * @h: desired height
 *
 * returns: a new pixbuf consisting of the source pixbuf tiled
 * enough times to fill out the space needed.
 *
 * The fractional tiles are spead evenly left-right and top-bottom.
 */
GdkPixbuf *
go_gdk_pixbuf_tile (GdkPixbuf const *src, guint w, guint h)
{
	int src_w, src_h;
	int tile_x, tile_y;
	int left_x, left_y;
	int dst_y = 0;
	int stripe_y;
	GdkPixbuf *dst;

	g_return_val_if_fail (GDK_IS_PIXBUF (src), NULL);
	g_return_val_if_fail (w < G_MAXINT, NULL);
	g_return_val_if_fail (h < G_MAXINT, NULL);

	src_w = gdk_pixbuf_get_width (src);
	src_h = gdk_pixbuf_get_height (src);

	tile_x = w / src_w;  /* Number of full tiles  */
	tile_y = h / src_h;

	left_x = w - tile_x * src_w;
	left_y = h - tile_y * src_h;

	dst_y = 0;
	dst = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (src),
			      gdk_pixbuf_get_has_alpha (src),
			      gdk_pixbuf_get_bits_per_sample (src),
			      MAX (w, 1),
			      MAX (h, 1));

	for (stripe_y = -1; stripe_y <= tile_y; stripe_y++) {
		int dst_x = 0;
		int stripe_x;
		int this_h, start_y = 0;

		if (stripe_y == -1) {
			this_h = (left_y + 1) / 2;
			start_y = src_h - this_h;
		} else if (stripe_y == tile_y)
			this_h = left_y / 2;
		else
			this_h = src_h;

		if (this_h == 0)
			continue;

		for (stripe_x = -1; stripe_x <= tile_x; stripe_x++) {
			int this_w, start_x = 0;

			if (stripe_x == -1) {
				this_w = (left_x + 1) / 2;
				start_x = src_w - this_w;
			} else if (stripe_x == tile_x)
				this_w = left_x / 2;
			else
				this_w = src_w;

			if (this_w == 0)
				continue;

			gdk_pixbuf_copy_area (src, start_x, start_y,
					      this_w, this_h,
					      dst,
					      dst_x, dst_y);

			dst_x += this_w;
		}

		dst_y += this_h;
	}

	return dst;
}

/* GOPixbuf implementation */


struct _GOPixbuf {
	GOImage parent;
	unsigned rowstride;
	GdkPixbuf *pixbuf;
	cairo_surface_t *surface;
};

typedef GOImageClass GOPixbufClass;

static GObjectClass *parent_klass;

enum {
	PIXBUF_PROP_0,
	PIXBUF_PROP_PIXBUF
};

static void
pixbuf_to_cairo (GOPixbuf *pixbuf)
{
	unsigned char *src, *dst;
	GOImage *image = GO_IMAGE (pixbuf);

	g_return_if_fail (GO_IS_PIXBUF (pixbuf) && image->data && pixbuf->pixbuf);

	src = gdk_pixbuf_get_pixels (pixbuf->pixbuf);
	dst = image->data;

	g_return_if_fail (gdk_pixbuf_get_rowstride (pixbuf->pixbuf) == (int) pixbuf->rowstride);

	go_cairo_convert_data_from_pixbuf (dst, src, image->width, image->height, pixbuf->rowstride);
}

static void
cairo_to_pixbuf (GOPixbuf *pixbuf)
{
	unsigned char *src, *dst;
	GOImage *image = GO_IMAGE (pixbuf);

	g_return_if_fail (GO_IS_PIXBUF (pixbuf) && image->data && pixbuf->pixbuf);

	dst = gdk_pixbuf_get_pixels (pixbuf->pixbuf);
	src = image->data;

	g_return_if_fail (gdk_pixbuf_get_rowstride (pixbuf->pixbuf) == (int) pixbuf->rowstride);

	go_cairo_convert_data_to_pixbuf (dst, src, image->width, image->height, pixbuf->rowstride);
}

static void
go_pixbuf_save (GOImage *image, GsfXMLOut *output)
{
	GOPixbuf *pixbuf;
	g_return_if_fail (GO_IS_PIXBUF (image));
	pixbuf = GO_PIXBUF (image);
	gsf_xml_out_add_int (output, "rowstride", pixbuf->rowstride);
	gsf_xml_out_add_base64 (output, NULL,
			image->data, image->height * pixbuf->rowstride);
}

static void
go_pixbuf_load_attr (GOImage *image, xmlChar const *attr_name, xmlChar const *attr_value)
{
	GOPixbuf *pixbuf = GO_PIXBUF (image);
	g_return_if_fail (pixbuf);
	if (!strcmp (attr_name, "rowstride"))
		pixbuf->rowstride = strtol (attr_value, NULL, 10);
}

static void
go_pixbuf_load_data (GOImage *image, GsfXMLIn *xin)
{
	int length;
	length = gsf_base64_decode_simple (xin->content->str, strlen(xin->content->str));
	image->data = g_memdup (xin->content->str, length);
}

static void
go_pixbuf_draw (GOImage *image, cairo_t *cr)
{
	GOPixbuf *pixbuf = GO_PIXBUF (image);
	g_return_if_fail (pixbuf);
	if (pixbuf->surface == NULL) {
		if (image->data == NULL) {
			/* image built from a pixbuf */
			image->data = g_new0 (guint8, image->height * pixbuf->rowstride);
			pixbuf_to_cairo (pixbuf);
		}
		pixbuf->surface = cairo_image_surface_create_for_data (image->data,
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
		if (image->width == 0 || image->height == 0 || image->data == NULL)
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
		g_free (image->data); /* this should be in GOPixbuf */
		image->data = NULL; /* this should be in GOPixbuf */
		_go_image_changed (image, gdk_pixbuf_get_width (pix), gdk_pixbuf_get_height (pix));
		pixbuf->rowstride = gdk_pixbuf_get_rowstride (pix); /* this should be in GOPixbuf */
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
}

GSF_CLASS (GOPixbuf, go_pixbuf,
	   go_pixbuf_class_init, NULL,
	   GO_TYPE_IMAGE)


GOImage *
go_pixbuf_new_from_pixbuf (GdkPixbuf *pixbuf)
{
	return g_object_new (GO_TYPE_PIXBUF, "pixbuf", pixbuf, NULL);
}

int
go_pixbuf_get_rowstride (GOPixbuf *pixbuf)
{
	g_return_val_if_fail (GO_IS_PIXBUF (pixbuf), 0);
	return pixbuf->rowstride;
}
