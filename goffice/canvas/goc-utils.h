/*
 * goc-utils.h :
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_UTILS_H
#define GOC_UTILS_H

#include <goffice/goffice.h>

typedef struct {
	/*< private >*/
	unsigned int n;
	unsigned int refs;
	/*< public >*/
	GocPoint *points;
} GocPoints;

GType goc_points_get_type (void);
#define GOC_TYPE_POINTS goc_points_get_type ()

GocPoints       *goc_points_new (unsigned n);
GocPoints	*goc_points_ref (GocPoints *points);
void		 goc_points_unref (GocPoints *points);

typedef struct {
	/*< private >*/
	unsigned int refs;
	/*< public >*/
	unsigned int n;
	int *vals;
} GocIntArray;

GType goc_int_array_get_type (void);
#define GOC_TYPE_INT_ARRAY goc_int_array_get_type ()

GocIntArray     *goc_int_array_new (unsigned n);
GocIntArray     *goc_int_array_ref (GocIntArray *array);
void		 goc_int_array_unref (GocIntArray *array);

#endif  /* GOC_UTILS_H */
