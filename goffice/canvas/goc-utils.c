/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-utils.c :
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

#include <goffice/goffice-config.h>
#include <goffice/canvas/goc-utils.h>

/**
 * SECTION:goc-utils
 *
 * Only one structure is currently available: #GocPoints.
 **/

/**
 * GocPoints:
 * @points: The embedded points.
 *
 * A boxed type used to hold a list of #GocPoint instances.
 **/

/**
 * goc_points_new:
 * @n: the number of #GocPoint instances.
 *
 * Creates a new #GocPoints instances with @n points with nul initial
 * coordinates. The coordinates can be changed using direct access:
 *
 * <programlisting>
 *      GocPoints points = goc_points_new (1);
 *      points->points[0].x = my_x;
 *      points->points[0].y = my_y;
 * </programlisting>
 *
 * Returns: the newly created #GocPoints with an initial references count of 1.
 **/

GocPoints *
goc_points_new (unsigned n)
{
	GocPoints *points = g_new (GocPoints, 1);
	points->n = n;
	points->refs = 1;
	points->points = g_new0 (GocPoint, n);
	return points;
}

/**
 * goc_points_ref :
 * @points: #GocPoints
 *
 * Increases the references count of @points by 1.
 * Returns: the referenced #GocPoints.
 **/
GocPoints *
goc_points_ref (GocPoints *points)
{
	points->refs++;
	return points;
}

/**
 * goc_points_unref:
 * @points: #GocPoints
 *
 * Decreases the references count of @points by 1, and destroys it if the
 * references count becomes 0.
 **/
void
goc_points_unref (GocPoints *points)
{
	points->refs--;
	if (points->refs == 0) {
		g_free (points->points);
		points->points = NULL;
		g_free (points);
	}
}

GType
goc_points_get_type (void)
{
    static GType type_points = 0;

    if (!type_points)
	type_points = g_boxed_type_register_static
	    ("GocPoints",
	     (GBoxedCopyFunc) goc_points_ref,
	     (GBoxedFreeFunc) goc_points_unref);

    return type_points;
}
