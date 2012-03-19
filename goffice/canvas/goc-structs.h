/*
 * goc-structs.h :
 *
 * Copyright (C) 2008-2009 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_STRUCTS_H
#define GOC_STRUCTS_H

/**
 * SECTION:goc-structs
 *
 * Simple structures used in the canvas code.
 **/

/**
 * GocPoint:
 * @x: horizontal position of the point.
 * @y: vertical position of the point.
 *
 * A simple point.
 **/
typedef struct {
	double x, y;
} GocPoint;


/**
 * GocRect:
 * @x: lowest horizontal bound of the rectangle.
 * @y: lowest vertical bound of the rectangle.
 * @width: rectangle width.
 * @height: rectangle height.
 *
 * A simple rectangle.
 **/
typedef struct {
	double x, y;
	double width, height;
} GocRect;

#endif  /* GOC_STRUCTS_H */
