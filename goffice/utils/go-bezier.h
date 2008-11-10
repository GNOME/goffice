/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-bezier.h
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

#ifndef GO_BEZIER_H
#define GO_BEZIER_H

#include <glib.h>
#include <cairo.h>
#include "go-path.h"

G_BEGIN_DECLS

struct GOBezierSpline {
	double *x, *y;
	int n;
	gboolean closed;
};

struct GOBezierSpline *go_bezier_spline_init (double const *x, double const *y, int n,
				   gboolean closed);
void go_bezier_spline_destroy (struct GOBezierSpline *sp);

GOPath *go_bezier_spline_to_path (struct GOBezierSpline *sp);

G_END_DECLS

#endif  /* GO_BEZIER_H */
