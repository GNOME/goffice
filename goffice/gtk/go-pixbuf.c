/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-pixbuf.c
 *
 * Copyright (C) 2000-2004 Jody Goldberg (jody@gnome.org)
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

/**
 * go_pixbuf_intelligent_scale:
 * @buf: a #GdkPixbuf
 * @width: new width
 * @height: new height
 *
 * Intelligent pixbuf scaling.
 *
 * returns: a new GdkPixbuf reference.
 **/

GdkPixbuf *
go_pixbuf_intelligent_scale (GdkPixbuf *buf, guint width, guint height)
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
 * go_pixbuf_new_from_file:
 * @filename : pixbuf filename
 *
 * Utility routine to create pixbufs from file @name in the goffice_icon_dir.
 *
 * Returns: a GdkPixbuf that the caller is responsible for.
 **/

GdkPixbuf *
go_pixbuf_new_from_file (char const *filename)
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
 * go_pixbuf_get_from_cache:
 * @filename: pixbuf filename
 *
 * Retrieves a pixbuf from the image cache, loading it from the file
 * @filename located in goffice_icon_dir if not cached yet.
 *
 * returns: a GdkPixbuf, NULL on error.
 **/

GdkPixbuf *
go_pixbuf_get_from_cache (char const *filename)
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

	pixbuf = go_pixbuf_new_from_file (filename);
	if (pixbuf != NULL)
		g_hash_table_insert (cache, (gpointer) filename, pixbuf);

	return pixbuf;
}


/**
 * go_pixbuf_tile:
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
go_pixbuf_tile (GdkPixbuf const *src, guint w, guint h)
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
