/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-cspline.h
 *
 * Copyright (C) 2007 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_CSPLINE_H
#define GO_CSPLINE_H

struct GOCSpline {
	double const *x, *y;
	double *a, *b, *c;
	int n;
};

enum {
	GO_CSPLINE_NATURAL,
	GO_CSPLINE_PARABOLIC,
	GO_CSPLINE_CUBIC,
	GO_CSPLINE_CLAMPED,
	GO_CSPLINE_MAX,
};

struct GOCSpline *go_cspline_init (double const *x, double const *y, int n,
				   unsigned limits, double c0, double cn);
void go_cspline_destroy (struct GOCSpline *sp);
double go_cspline_get_value (struct GOCSpline *sp, double x);
double go_cspline_get_deriv (struct GOCSpline *sp, double x);
double *go_cspline_get_values (struct GOCSpline *sp, double *x, int n);
double *go_cspline_get_derivs (struct GOCSpline *sp, double *x, int n);
double *go_cspline_get_integrals (struct GOCSpline *sp, double *x, int n);

#ifdef GOFFICE_WITH_LONG_DOUBLE
struct GOCSplinel {
	long double const *x, *y;
	long double *a, *b, *c;
	int n;
};

struct GOCSplinel *go_cspline_initl (long double const *x, long double const *y, int n,
				     unsigned limits, long double a0, long double a1);
void go_cspline_destroyl (struct GOCSplinel *sp);
long double go_cspline_get_valuel (struct GOCSplinel *sp, long double x);
long double go_cspline_get_derivl (struct GOCSplinel *sp, long double x);
long double *go_cspline_get_valuesl (struct GOCSplinel *sp, long double *x, int n);
long double *go_cspline_get_derivsl (struct GOCSplinel *sp, long double *x, int n);
long double *go_cspline_get_integralsl (struct GOCSplinel *sp, long double *x, int n);
#endif

#endif
