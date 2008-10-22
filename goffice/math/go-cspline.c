/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-cspline.c
 *
 * The code in this file has been adapted from public domain code by 
 * Sylvain BARTHELEMY (http://www.sylbarth.com/interp.php).
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

#include <goffice/goffice-config.h>
#include "go-rangefunc.h"
#include "go-cspline.h"
#include <glib/gmem.h>
#include <glib/gmessages.h>

#ifndef DOUBLE

#define DOUBLE double
#define SUFFIX(_n) _n

#ifdef GOFFICE_WITH_LONG_DOUBLE
#include "go-cspline.c"
#undef DOUBLE
#undef SUFFIX

#ifdef HAVE_SUNMATH_H
#include <sunmath.h>
#endif
#define DOUBLE long double
#define SUFFIX(_n) _n ## l
#endif

#endif

/*
Creates a spline structure, and computes the coefficients associated with the
polynoms.
Arguments are:
x: the given abscissas, theymust be given in increasing order
y: the given ordinates
n: the number of valid (x, y) pairs
limits: how the limits must be treated, four values are allowed:
	GO_CSPLINE_NATURAL: first and least second derivatives are 0.
	GO_CSPLINE_PARABOLIC: the curve will be a parabole arc outside of the limits.
	GO_CSPLINE_CUBIC: the curve will be cubic outside of the limits.
	GO_CSPLINE_CLAMPED: the first and last derivatives are imposed.
c0, cn: the first and last derivatives when using clamped splines, not used in the
other limit types.
*/
struct SUFFIX(GOCSpline) *
SUFFIX(go_cspline_init) (DOUBLE const *x, DOUBLE const *y, int n,
			 unsigned limits, DOUBLE c0, DOUBLE cn)
{
	DOUBLE *d1, *d2, *d3, *d4, *s, *h;
	struct SUFFIX(GOCSpline) *sp;
	double dx1 = 0., dy1 = 0., dx2 = 0., dy2 = 0., dxn1 = 0., dxn2 = 0.;
	int nm1, nm2, i, j, first, last;

	/* What is the minimum number of knots? Taking 3 at the moment */
	if (limits >= GO_CSPLINE_MAX || !SUFFIX(go_range_increasing) (x, n) || n < 3)
		return NULL;
	nm1 = n - 1;
	sp = (struct SUFFIX(GOCSpline)*) g_new0 (struct SUFFIX(GOCSpline), 1);
	sp->n = n;
	sp->x = x;
	sp->y = y;
	sp->a = (DOUBLE*) g_new0 (DOUBLE, nm1);
	sp->b = (DOUBLE*) g_new (DOUBLE, nm1);
	sp->c = (DOUBLE*) g_new (DOUBLE, nm1);
	i = n + 1;
	d1 = (DOUBLE*) g_new0 (DOUBLE, i);
	d2 = (DOUBLE*) g_new0 (DOUBLE, i);
	d3 = (DOUBLE*) g_new0 (DOUBLE, i);
	d4 = (DOUBLE*) g_new0 (DOUBLE, i);
	s = (DOUBLE*) g_new0 (DOUBLE, i);
	h = (DOUBLE*) g_new0 (DOUBLE, i);

  /* --- COMPUTE FOR N-2 ROWS --- */
	nm2 = n - 2; 
	dx1 = x[1] - x[0]; 
	dy1 = (y[1] - y[0]) / dx1 * 6.0;
	for ( i = 1; i <= nm2; ++i ) { 
		dx2 = x[i + 1] - x[i];
		dy2 = (y[i + 1] - y[i]) / dx2 * 6.0;
		d1[i] = dx1;
		d2[i] = 2.0 * (dx1 + dx2);
		d3[i] = dx2;
		d4[i] = dy2 - dy1;
		dx1 = dx2; 
		dy1 = dy2;
	}     /* end of for i loop */
	first = 2; 
	last  = nm2;

	/* --- ADJUST FIRST AND LAST ROWS ACCORDING TO IEND CONDITIONS --- */
	switch (limits) {
	case GO_CSPLINE_NATURAL : /* natural end conditions */       ; /* no change needed */
		break;
	case GO_CSPLINE_PARABOLIC : /* parabolic end conditions: s(1)=s(2) and s(n)=s(n-1) */
		d2[1] = d2[1] + x[1] - x[0];
		d2[nm2] = d2[nm2] + x[nm1] - x[nm2];
		break;
	case GO_CSPLINE_CUBIC : /* cubic ends--s[1], s[n] are extrapolated */
		dx1 = x[1] - x[0]; 
		dx2 = x[2] - x[1];
		d2[1] = (dx1 + dx2) * (dx1 + 2.0 * dx2) / dx2;
		d3[1] = (dx2 * dx2 - dx1 * dx1) / dx2;
		dxn2 = x[nm2] - x[nm2 - 1]; 
		dxn1 = x[nm1] - x[nm2];
		d1[nm2] = (dxn2 * dxn2 - dxn1 * dxn1) / dxn2;
		d2[nm2] = (dxn1 + dxn2) * (dxn1 + 2.0 * dxn2) / dxn2;
		break;
	case GO_CSPLINE_CLAMPED : /* Derivative end conditions */
		dx1 = x[1] - x[0]; 
		dy1 = (y[1] - y[0]) / dx1;
		s[0] = c0; 
		s[nm1] = cn;
		d1[0] = 2.0 * dx1;
		d2[0] = dx1; 
		d3[0] = 0.0;
		d4[0] = (dy1 - s[1]) * 6;
		dx1 = x[nm1] - x[nm2]; 
		dy1 = (y[nm1] - y[nm2]) / dx1;
		d1[nm1] = dx1; 
		d2[nm1] = 2.0 * dx1;
		d3[nm1] = 0.0;
		d4[nm1] = (s[n] - dy1) * 6.0;
		first = 1; last = n-1;  
		break;
}

	/* --- NOW WE SOLVE THE TRIDIAGONAL SYSTEM --- */
	for ( i = first; i <= last; ++i ) {
		d1[i] = d1[i] / d2[i - 1];
		d2[i] = d2[i] - d1[i] * d3[i - 1];
		d4[i] = d4[i] - d1[i] * d4[i - 1];
	}

	/* --- NOW WE BACK SUBSTITUTE --- */
	d4[last] = d4[last] / d2[last];
	for (j = last - 1; j >= first - 1; --j )
		d4[j] = (d4[j] - d3[j] * d4[j + 1]) / d2[j];

	/* --- NOW PUT THE VALUES INTO THE S VECTOR --- */
	for ( i = first - 1; i <= last; ++i )
		s[i] = d4[i];

	/* --- TAKE CARE OF SPECIAL END CONDITIONS  FOR S VECTOR --- */
	switch (limits) {
	case GO_CSPLINE_NATURAL :  
		s[0] = 0.0; 
		s[nm1] = 0.0;
		break;
	case GO_CSPLINE_PARABOLIC :  
		s[0] = s[1]; 
		s[nm1] = s[nm2];
		break;
	case GO_CSPLINE_CUBIC :  
		s[0] = ((dx1 + dx2) * s[1] - dx1 * s[2]) / dx2;
		s[nm1] = ((dx2 + dxn1) * s[nm2] - dxn1 * s[nm2 - 1]) / dx2;
		break;
	case GO_CSPLINE_CLAMPED :  /* already taken care of */;
		break;
	}

	/* --- GENERATE THE COEFFICIENTS OF THE POLYNOMIALS --- */
	for ( i = 0; i < nm1; ++i ) {
		h[i] = x[i+1] - x[i];
		sp->a[i] = (s[i + 1] - s[i]) / (6.0 * h[i]);
		sp->b[i] = s[i] / 2.0;
		sp->c[i] = ((y[i + 1] - y[i]) / h[i]) -
					((2.0* h[i] * s[i] + h[i] * s[i + 1]) / 6.0);
	}

	g_free (d1);
	g_free (d2);
	g_free (d3);
	g_free (d4);
	g_free (s);
	g_free (h);
	return sp;
}

/*
	Frees the spline structure when done.
*/
void SUFFIX(go_cspline_destroy) (struct SUFFIX(GOCSpline) *sp)
{
	g_return_if_fail (sp);
	g_free (sp->a);
	g_free (sp->b);
	g_free (sp->c);
	g_free (sp);
}

/*
Returns the interpolated value for x.
sp must be a spine structure as returned by SUFFIX(go_cspline_init)
*/
DOUBLE SUFFIX(go_cspline_get_value) (struct SUFFIX(GOCSpline) *sp, DOUBLE x)
{
	DOUBLE dx;
	int n, j, k;

	g_return_val_if_fail (sp != NULL, 0.);

	n = sp->n - 2;
	if (x >= sp->x[n]) {
		dx = x - sp->x[n];
		return sp->y[n] + dx * (sp->c[n] + dx * (sp->b[n] + dx * sp->a[n]));
	}
	if (x <= sp->x[1]) {
		dx = x - sp->x[0];
		return sp->y[0] + dx * (sp->c[0] + dx * (sp->b[0] + dx * sp->a[0]));
	}
	j = 1;
	/* search the relevant interval by dichotomy */
	while (n > j + 1) {
		k = (n + j) / 2;
		if (x > sp->x[k])
			j = k;
		else
			n = k;
	}
	dx = x - sp->x[j];
	return sp->y[j] + dx * (sp->c[j] + dx * (sp->b[j] + dx * sp->a[j]));
}

/*
Returns the interpolated derivative at x.
sp must be a spine structure as returned by SUFFIX(go_cspline_init)
*/
DOUBLE SUFFIX(go_cspline_get_deriv) (struct SUFFIX(GOCSpline) *sp, DOUBLE x)
{
	DOUBLE dx;
	int n, j, k;

	g_return_val_if_fail (sp != NULL, 0.);

	n = sp->n - 2;
	if (x >= sp->x[n]) {
		dx = x - sp->x[n];
		return sp->c[n] + dx * (2 * sp->b[n] + dx * 3 * sp->a[n]);
	}
	if (x <= sp->x[1]) {
		dx = x - sp->x[0];
		return sp->c[0] + dx * (2 * sp->b[0] + dx * 3 * sp->a[0]);
	}
	j = 1;
	/* search the relevant interval by dichotomy */
	while (n > j + 1) {
		k = (n + j) / 2;
		if (x > sp->x[k])
			j = k;
		else
			n = k;
	}
	dx = x - sp->x[j];
	return sp->c[j] + dx * (2 * sp->b[j] + dx * 3 * sp->a[j]);
}

/*
Returns an array of the n interpolated values at the n values stored in x.
The x values must be in increasing order.
sp must be a spine structure as returned by SUFFIX(go_cspline_init)
*/
DOUBLE *SUFFIX(go_cspline_get_values) (struct SUFFIX(GOCSpline) *sp, DOUBLE const *x, int n)
{
	DOUBLE *res, dx;
	int i, j, k, jmax;
	g_return_val_if_fail (sp != NULL, NULL);
	if (!x || n <= 0 || !SUFFIX(go_range_increasing) (x, n))
		return NULL;
	res = g_new (DOUBLE, n);
	j = 1;
	jmax = sp->n - 1;
	for (i = 0; i < n; i++) {
		dx = x[i];
		while (dx > sp->x[j] && j < jmax)
			j++;
		k = j - 1;
		dx -= sp->x[k];
		res[i] = sp->y[k] + dx * (sp->c[k] + dx * (sp->b[k] + dx * sp->a[k]));
	}
	return res;
}

/*
Returns an array of the n interpolated derivatives at the n values stored in x.
The x values must be in increasing order.
sp must be a spine structure as returned by SUFFIX(go_cspline_init)
*/
DOUBLE *SUFFIX(go_cspline_get_derivs) (struct SUFFIX(GOCSpline) *sp, DOUBLE const *x, int n)
{
	DOUBLE *res, dx;
	int i, j, k, jmax;
	g_return_val_if_fail (sp != NULL, NULL);
	if (!x || n <= 0 || !SUFFIX(go_range_increasing) (x, n))
		return NULL;
	res = g_new (DOUBLE, n);
	j = 1;
	jmax = sp->n - 1;
	for (i = 0; i < n; i++) {
		dx = x[i];
		while (dx > sp->x[j] && j < jmax)
			j++;
		k = j - 1;
		dx -= sp->x[k];
		res[i] = sp->c[k] + dx * (2 * sp->b[k] + dx * 3 * sp->a[k]);
	}
	return res;
}

/*
Returns an array of the n-1 integrals on the intervals between two consecutive
values stored in x.
The x values must be in increasing order.
sp must be a spine structure as returned by SUFFIX(go_cspline_init)
*/
DOUBLE *SUFFIX(go_cspline_get_integrals) (struct SUFFIX(GOCSpline) *sp, DOUBLE const *x, int n)
{
	DOUBLE *res, start, end, sum;
	int i, j, k, jmax;
	g_return_val_if_fail (sp != NULL, NULL);
	if (!x || n <= 1 || !SUFFIX(go_range_increasing) (x, n))
		return NULL;
	res = g_new (DOUBLE, n - 1);
	j = 1;
	jmax = sp->n - 1;
	start = x[0];
	for (i = 1; i < n; i++) {
		sum = 0.;
		end = x[i];
		while (start >= sp->x[j])
			j++;
		k = (j > 1)? j - 1: 0;
		start -= sp->x[k];
		sum = -start * (sp->y[k] + start * (sp->c[k] / 2. + start * (sp->b[k] / 3. + start * sp->a[k] / 4.)));
		while (j < jmax && end > sp->x[j]) {
			start = sp->x[j] - sp->x[k];
			sum += start * (sp->y[k] + start * (sp->c[k] / 2. + start * (sp->b[k] / 3. + start * sp->a[k] / 4.)));
			start = sp->x[j];
			j++;
			k = j - 1;
		}
		start = end - sp->x[k];
		sum += start * (sp->y[k] + start * (sp->c[k] / 2. + start * (sp->b[k] / 3. + start * sp->a[k] / 4.)));
		res[i - 1] = sum;
		start = end;
	}
	return res;
}
