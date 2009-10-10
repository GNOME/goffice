/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-utils.h :
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_UTILS_H
#define GOC_UTILS_H

#include <goffice/goffice.h>

typedef struct {
	/*< private >*/
	unsigned n;
	unsigned refs;
	/*< public >*/
	GocPoint *points;
} GocPoints;

GType goc_points_get_type (void);
#define GOC_TYPE_POINTS goc_points_get_type ()

GocPoints       *goc_points_new (unsigned n);
GocPoints	*goc_points_ref (GocPoints *points);
void		 goc_points_unref (GocPoints *points);

#endif  /* GOC_UTILS_H */
