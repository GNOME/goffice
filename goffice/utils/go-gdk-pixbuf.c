/*
 * go-gdk-pixbuf.c
 *
 * Copyright (C) 2000-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2011-2012 Jean Brefort (jean.brefort@normalesup.org)
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
#include "go-gdk-pixbuf.h"
#include <goffice/goffice-priv.h>
#include <string.h>

/*****************************************************************************
 * The functions below do not pertain to GOPixbuf, they should be moved to
 * elsewhere, may be go-gdk-pixbuf.[c,h]
 *****************************************************************************/
/**
 * go_gdk_pixbuf_intelligent_scale:
 * @buf: a #GdkPixbuf
 * @width: new width
 * @height: new height
 *
 * Intelligent pixbuf scaling.
 *
 * Deprecated: 0.9.6 use go_image_get_scaled_pixbuf() instead
 * Returns: a new GdkPixbuf reference.
 **/
/* FIXME: deprecate, use go_image_get_scaled_pixbuf instead*/
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

static GdkPixbuf *
new_from_data (gconstpointer data, size_t length)
{
	GdkPixbuf *res;
	GdkPixbufLoader *loader = gdk_pixbuf_loader_new ();
	gdk_pixbuf_loader_write (loader, data, length, NULL);
	gdk_pixbuf_loader_close (loader, NULL);
	res = gdk_pixbuf_loader_get_pixbuf (loader);
	if (res)
		g_object_ref (res);
	g_object_unref (loader);
	return res;
}

/**
 * go_gdk_pixbuf_load_from_file:
 * @filename: pixbuf filename
 *
 * Utility routine to create pixbufs from file @name in the goffice_icon_dir.
 * As a special case, @filename may have the form "res:<resource name>" in
 * which case the resource manager is queried instead of loading a file.
 *
 * Since: 0.9.6
 * Returns: (transfer full): a GdkPixbuf that the caller is responsible for.
 **/
GdkPixbuf *
go_gdk_pixbuf_load_from_file (char const *filename)
{
	GdkPixbuf *pixbuf;

	g_return_val_if_fail (filename != NULL, NULL);

	if (strncmp (filename, "res:", 4) == 0) {
		size_t length;
		const char *data = go_rsm_lookup (filename + 4, &length);
		pixbuf = data
			? new_from_data (data, length)
			: NULL;
	} else {
		char *path = g_path_is_absolute (filename)
			? g_strdup (filename)
			: g_build_filename (go_sys_icon_dir (), filename, NULL);
		pixbuf = gdk_pixbuf_new_from_file (path, NULL);
		g_free (path);
	}

	if (!pixbuf && go_debug_flag ("pixbuf"))
		g_printerr ("Failed to load pixbuf from %s\n", filename);

	return pixbuf;
}

/**
 * go_gdk_pixbuf_new_from_file:
 * @filename: pixbuf filename
 *
 * Utility routine to create pixbufs from file @name in the goffice_icon_dir.
 * As a special case, @filename may have the form "res:<resource name>" in
 * which case the resource manager is queried instead of loading a file.
 *
 * Deprecated: 0.9.6 use go_gdk_pixbuf_load_from_file() instead
 * Returns: (transfer full): a GdkPixbuf that the caller is responsible for.
 **/
GdkPixbuf *
go_gdk_pixbuf_new_from_file (char const *filename)
{
	return go_gdk_pixbuf_load_from_file (filename);
}

/**
 * go_gdk_pixbuf_get_from_cache:
 * @filename: pixbuf filename
 *
 * Retrieves a pixbuf from the image cache, loading it from the file
 * @filename located in goffice_icon_dir if not cached yet.
 *
 * Returns: (transfer none) (nullable): a GdkPixbuf, %NULL on error.
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

	pixbuf = go_gdk_pixbuf_load_from_file (filename);
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
 * Deprecated: 0.9.6
 * Returns: (transfer full): a new pixbuf consisting of the source pixbuf tiled
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
