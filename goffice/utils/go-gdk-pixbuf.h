/*
 * go-gdk-pixbuf.h - Misc GdkPixbuf utilities
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
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

#ifndef GO_GDK_PIXBUF_H
#define GO_GDK_PIXBUF_H

#include <goffice/goffice.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

GdkPixbuf 	*go_gdk_pixbuf_load_from_file	(char const *filename);
GdkPixbuf 	*go_gdk_pixbuf_get_from_cache 	(char const *filename);
#ifndef GOFFICE_DISABLE_DEPRECATED
GOFFICE_DEPRECATED_FOR(go_image_get_scaled_pixbuf)
GdkPixbuf 	*go_gdk_pixbuf_intelligent_scale (GdkPixbuf *buf,
						  guint width, guint height);
GOFFICE_DEPRECATED_FOR(go_gdk_pixbuf_load_from_file)
GdkPixbuf 	*go_gdk_pixbuf_new_from_file	(char const *filename);
GOFFICE_DEPRECATED_FOR(go_image_get_pixbuf)
GdkPixbuf	*go_gdk_pixbuf_tile		(GdkPixbuf const *src,
						 guint w, guint h);
#endif

G_END_DECLS

#endif
