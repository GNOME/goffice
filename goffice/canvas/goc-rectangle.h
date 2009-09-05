/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-rectangle.h :
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_RECTANGLE_H
#define GOC_RECTANGLE_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GocRectangle {
	GocStyledItem base;

	double rotation; /* rotation around the center in radians */
	double x, y, width, height;
};

typedef GocStyledItemClass GocRectangleClass;

#define GOC_TYPE_RECTANGLE	(goc_rectangle_get_type ())
#define GOC_RECTANGLE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_RECTANGLE, GocRectangle))
#define GOC_IS_RECTANGLE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_RECTANGLE))

GType goc_rectangle_get_type (void);

G_END_DECLS

#endif  /* GOC_RECTANGLE_H */
