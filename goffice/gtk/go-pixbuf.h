/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-pixbuf.h - Misc GdkPixbuf utilities
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License, version 2, as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#ifndef GO_PIXBUF_H
#define GO_PIXBUF_H

#include <gdk/gdk.h>

G_BEGIN_DECLS

GdkPixbuf 	*go_pixbuf_intelligent_scale	(GdkPixbuf *buf,
						 guint width, guint height);
GdkPixbuf 	*go_pixbuf_new_from_file	(char const *filename);
GdkPixbuf 	*go_pixbuf_get_from_cache 	(char const *filename);
GdkPixbuf	*go_pixbuf_tile			(GdkPixbuf const *src,
						 guint w, guint h);

G_END_DECLS

#endif
