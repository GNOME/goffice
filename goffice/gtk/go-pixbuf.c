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
	char *path = g_build_filename (go_sys_icon_dir (), filename, NULL);
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (path, NULL);

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
