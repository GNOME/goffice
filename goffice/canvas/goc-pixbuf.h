/*
 * goc-pixbuf.h:
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

#ifndef GOC_PIXBUF_H
#define GOC_PIXBUF_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GocPixbuf {
	GocItem base;

	double x, y, width, height, rotation;
	GdkPixbuf *pixbuf;
	gpointer priv;
};
typedef GocItemClass GocPixbufClass;

#define GOC_TYPE_PIXBUF	(goc_pixbuf_get_type ())
#define GOC_PIXBUF(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_PIXBUF, GocPixbuf))
#define GOC_IS_PIXBUF(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_PIXBUF))

GType goc_pixbuf_get_type (void);

G_END_DECLS

#endif  /* GOC_PIXBUF_H */
