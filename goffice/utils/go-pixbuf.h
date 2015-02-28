/*
 * go-pixbuf.h
 *
 * Copyright (C) 2011-2012 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_PIXBUF_H
#define GO_PIXBUF_H

#include <goffice/goffice.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define GO_TYPE_PIXBUF	(go_pixbuf_get_type ())
#define GO_PIXBUF(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PIXBUF, GOPixbuf))
#define GO_IS_PIXBUF(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PIXBUF))

GType go_pixbuf_get_type (void);

GOImage		*go_pixbuf_new_from_pixbuf  (GdkPixbuf *pixbuf);
GOImage		*go_pixbuf_new_from_data	(char const *type, guint8 const *data,
			                             gsize length, GError **error);
int 		 go_pixbuf_get_rowstride 	(GOPixbuf *pixbuf);

G_END_DECLS

#endif
