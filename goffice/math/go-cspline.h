/*
 * go-cspline.h
 *
 * Copyright (C) 2007-2008 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_CSPLINE_H
#define GO_CSPLINE_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GOCSpline GOCSpline;
struct _GOCSpline {
	double const *x, *y;
	double *a, *b, *c;
	int n;
	unsigned int ref_count;
};

typedef enum {
	GO_CSPLINE_NATURAL,
	GO_CSPLINE_PARABOLIC,
	GO_CSPLINE_CUBIC,
	GO_CSPLINE_CLAMPED,
	/* <private> */
	GO_CSPLINE_MAX
} GOCSplineType;

GType go_cspline_get_type (void);
GOCSpline *go_cspline_init (double const *x, double const *y, int n,
			    unsigned limits, double c0, double cn);
void go_cspline_destroy (GOCSpline *sp);
double go_cspline_get_value (GOCSpline const *sp, double x);
double go_cspline_get_deriv (GOCSpline const *sp, double x);
double *go_cspline_get_values (GOCSpline const *sp, double const *x, int n);
double *go_cspline_get_derivs (GOCSpline const *sp, double const *x, int n);
double *go_cspline_get_integrals (GOCSpline const *sp, double const *x, int n);

// -----------------------------------------------------------------------------

#ifdef GOFFICE_WITH_LONG_DOUBLE
typedef struct _GOCSplinel GOCSplinel;
struct _GOCSplinel {
	long double const *x, *y;
	long double *a, *b, *c;
	int n;
	unsigned int ref_count;
};

GType go_csplinel_get_type (void);
GOCSplinel *go_cspline_initl (long double const *x, long double const *y, int n,
			      unsigned limits, long double c0, long double cn);
void go_cspline_destroyl (GOCSplinel *sp);
long double go_cspline_get_valuel (GOCSplinel const *sp, long double x);
long double go_cspline_get_derivl (GOCSplinel const *sp, long double x);
long double *go_cspline_get_valuesl (GOCSplinel const *sp, long double const *x, int n);
long double *go_cspline_get_derivsl (GOCSplinel const *sp, long double const *x, int n);
long double *go_cspline_get_integralsl (GOCSplinel const *sp, long double const *x, int n);
#endif

// -----------------------------------------------------------------------------

#ifdef GOFFICE_WITH_DECIMAL64
typedef struct _GOCSplineD GOCSplineD;
struct _GOCSplineD {
	_Decimal64 const *x, *y;
	_Decimal64 *a, *b, *c;
	int n;
	unsigned int ref_count;
};

GType go_csplineD_get_type (void);
GOCSplineD *go_cspline_initD (_Decimal64 const *x, _Decimal64 const *y, int n,
			      unsigned limits, _Decimal64 c0, _Decimal64 cn);
void go_cspline_destroyD (GOCSplineD *sp);
_Decimal64 go_cspline_get_valueD (GOCSplineD const *sp, _Decimal64 x);
_Decimal64 go_cspline_get_derivD (GOCSplineD const *sp, _Decimal64 x);
_Decimal64 *go_cspline_get_valuesD (GOCSplineD const *sp, _Decimal64 const *x, int n);
_Decimal64 *go_cspline_get_derivsD (GOCSplineD const *sp, _Decimal64 const *x, int n);
_Decimal64 *go_cspline_get_integralsD (GOCSplineD const *sp, _Decimal64 const *x, int n);
#endif

// -----------------------------------------------------------------------------

G_END_DECLS

#endif

