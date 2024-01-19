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

// We need multiple versions of this code.  We're going to include ourself
// with different settings of various macros.  gdb will hate us.
#include <goffice/goffice-multipass.h>
#ifdef SKIP_THIS_PASS

// Stub for the benefit of gtk-doc
GType INFIX(go_cspline,_get_type) (void);
GType INFIX(go_cspline,_get_type) (void) { return 0; }

#else

/**
 * GOCSplineType:
 * @GO_CSPLINE_NATURAL: natural.
 * @GO_CSPLINE_PARABOLIC: parabolic.
 * @GO_CSPLINE_CUBIC: cubic.
 * @GO_CSPLINE_CLAMPED: clamped.
 **/

/**
 * go_cspline_init: (skip)
 * @x: the x values
 * @y: the y values
 * @n: the number of x and y values
 * @limits: how the limits must be treated, four values are allowed:
 *	GO_CSPLINE_NATURAL: first and least second derivatives are 0.
 *	GO_CSPLINE_PARABOLIC: the curve will be a parabolic arc outside of the limits.
 *	GO_CSPLINE_CUBIC: the curve will be cubic outside of the limits.
 *	GO_CSPLINE_CLAMPED: the first and last derivatives are imposed.
 * @c0: the first derivative when using clamped splines, not used in the
 *      other limit types.
 * @cn: the first derivative when using clamped splines, not used in the
 *      other limit types.
 *
 * Creates a spline structure, and computes the coefficients associated with the
 * polynomials. The ith polynomial (between x[i-1] and x[i] is:
 * y(x) = y[i-1] + (c[i-1] + (b[i-1] + a[i] * (x - x[i-1])) * (x - x[i-1])) * (x - x[i-1])
 * where a[i-1], b[i-1], c[i-1], x[i-1] and y[i-1] are the corresponding
 * members of the new structure.
 *
 * Returns: a newly created GOCSpline instance which should be
 * destroyed by a call to go_cspline_destroy.
 */
SUFFIX(GOCSpline) *
SUFFIX(go_cspline_init) (DOUBLE const *x, DOUBLE const *y, int n,
			 unsigned limits, DOUBLE c0, DOUBLE cn)
{
	DOUBLE *d1, *d2, *d3, *d4, h;
	SUFFIX(GOCSpline) *sp;
	DOUBLE dx1 = 0., dy1 = 0., dx2 = 0., dy2 = 0., dxn1 = 0., dxn2 = 0.;
	int nm1, nm2, i, j, first, last;

	/* What is the minimum number of knots? Taking 3 at the moment */
	if (limits >= GO_CSPLINE_MAX || !SUFFIX(go_range_increasing) (x, n) || n < 3)
		return NULL;
	nm1 = n - 1;
	sp = g_new0 (SUFFIX(GOCSpline), 1);
	sp->n = n;
	sp->x = x;
	sp->y = y;
	sp->a = g_new0 (DOUBLE, nm1);
	sp->b = g_new (DOUBLE, nm1);
	sp->c = g_new (DOUBLE, nm1);
	sp->ref_count = 1;
	d1 =  g_new0 (DOUBLE, n);
	d2 = g_new0 (DOUBLE, n);
	d3 = g_new0 (DOUBLE, n);
	d4 = g_new0 (DOUBLE, n);

  /* --- COMPUTE FOR N-2 ROWS --- */
	nm2 = n - 2;
	dx1 = x[1] - x[0];
	dy1 = (y[1] - y[0]) / dx1 * 3;
	for ( i = 1; i <= nm2; ++i ) {
		dx2 = x[i + 1] - x[i];
		dy2 = (y[i + 1] - y[i]) / dx2 * 3;
		d1[i] = dx1;
		d2[i] = 2 * (dx1 + dx2);
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
		d2[1] = (dx1 + dx2) * (dx1 + 2 * dx2) / dx2;
		d3[1] = (dx2 * dx2 - dx1 * dx1) / dx2;
		dxn2 = x[nm2] - x[nm2 - 1];
		dxn1 = x[nm1] - x[nm2];
		d1[nm2] = (dxn2 * dxn2 - dxn1 * dxn1) / dxn2;
		d2[nm2] = (dxn1 + dxn2) * (dxn1 + 2 * dxn2) / dxn2;
		break;
	case GO_CSPLINE_CLAMPED : /* Derivative end conditions */
		dx1 = x[1] - x[0];
		dy1 = (y[1] - y[0]) / dx1;
		d1[0] = 0.0;
		d2[0] = 2 * dx1;
		d3[0] = dx1;
		d4[0] = (dy1 - c0) * 3;
		dx1 = x[nm1] - x[nm2];
		dy1 = (y[nm1] - y[nm2]) / dx1;
		d1[nm1] = dx1;
		d2[nm1] = 2 * dx1;
		d3[nm1] = 0.0;
		d4[nm1] = (cn - dy1) * 3;
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

	/* --- TAKE CARE OF SPECIAL END CONDITIONS  FOR S VECTOR --- */
	switch (limits) {
	case GO_CSPLINE_NATURAL :
		d4[0] = 0.0;
		d4[nm1] = 0.0;
		break;
	case GO_CSPLINE_PARABOLIC :
		d4[0] = d4[1];
		d4[nm1] = d4[nm2];
		break;
	case GO_CSPLINE_CUBIC :
		d4[0] = ((dx1 + dx2) * d4[1] - dx1 * d4[2]) / dx2;
		d4[nm1] = ((dxn2 + dxn1) * d4[nm2] - dxn1 * d4[nm2 - 1]) / dxn2;
		break;
	case GO_CSPLINE_CLAMPED :  /* already taken care of */;
		break;
	}

	/* --- GENERATE THE COEFFICIENTS OF THE POLYNOMIALS --- */
	for ( i = 0; i < nm1; ++i ) {
		h = x[i+1] - x[i];
		sp->a[i] = (d4[i + 1] - d4[i]) / (3 * h);
		sp->b[i] = d4[i];
		sp->c[i] = ((y[i + 1] - y[i]) / h) -
					((2 * d4[i] + d4[i + 1]) * h / 3);
	}

	g_free (d1);
	g_free (d2);
	g_free (d3);
	g_free (d4);
	return sp;
}

/**
 * go_cspline_destroy:
 * @sp: a spline structure returned by go_cspline_init.
 *
 * Frees the spline structure when done.
 */
void SUFFIX(go_cspline_destroy) (SUFFIX(GOCSpline) *sp)
{
	g_return_if_fail (sp);
	if (sp->ref_count-- > 1)
		return;
	g_free (sp->a);
	g_free (sp->b);
	g_free (sp->c);
	g_free (sp);
}

static GOCSpline *
SUFFIX(go_cspline_ref) (GOCSpline *sp)
{
	g_return_val_if_fail (sp, NULL);
	sp->ref_count++;
	return sp;
}

GType
INFIX(go_cspline,_get_type) (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static (
		     "GOCSpline" SUFFIX_STR,
		     (GBoxedCopyFunc)SUFFIX(go_cspline_ref),
		     (GBoxedFreeFunc)SUFFIX(go_cspline_destroy));
	}
	return t;
}

/**
 * go_cspline_get_value:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: The value
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 *
 * Returns: the interpolated value for x, or 0 if an error occurred.
 */
DOUBLE SUFFIX(go_cspline_get_value) (SUFFIX(GOCSpline) const *sp, DOUBLE x)
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

/**
 * go_cspline_get_deriv:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: the value
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 *
 * Returns: the interpolated derivative at x, or 0 if an error occurred.
 */
DOUBLE SUFFIX(go_cspline_get_deriv) (SUFFIX(GOCSpline) const *sp, DOUBLE x)
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

/**
 * go_cspline_get_values:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: a vector a values at which interpolation is requested.
 * @n: the number of interpolation requested.
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 * The x values must be sorted in increasing order.
 *
 * Returns: a newly allocated array of interpolated values which should
 * be destroyed by a call to g_free when not anymore needed, or %NULL if
 * an error occurred.
 */
DOUBLE *SUFFIX(go_cspline_get_values) (SUFFIX(GOCSpline) const *sp, DOUBLE const *x, int n)
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

/**
 * go_cspline_get_derivs:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: a vector a values at which interpolation is requested.
 * @n: the number of interpolation requested.
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 * The x values must be sorted in increasing order.
 *
 * Returns: a newly allocated array of the n interpolated derivatives which
 * should be destroyed by a call to g_free when not anymore needed, or %NULL if
 * an error occurred.
 */
DOUBLE *SUFFIX(go_cspline_get_derivs) (SUFFIX(GOCSpline) const *sp, DOUBLE const *x, int n)
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

/**
 * go_cspline_get_integrals:
 * @sp: a spline structure returned by go_cspline_init.
 * @x: a vector a values at which interpolation is requested.
 * @n: the number of interpolation requested.
 *
 * sp must be a valid spline structure as returned by go_cspline_init.
 * The x values must be sorted in increasing order.
 *
 * Returns: a newly allocated array of the n-1 integrals on the intervals
 * between two consecutive values stored in x. which should be destroyed by
 * a call to g_free when not anymore needed, or %NULL if  an error occurred.
 */
DOUBLE *SUFFIX(go_cspline_get_integrals) (SUFFIX(GOCSpline) const *sp, DOUBLE const *x, int n)
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
		sum = -start * (sp->y[k] + start * (sp->c[k] / 2 + start * (sp->b[k] / 3 + start * sp->a[k] / 4)));
		while (j < jmax && end > sp->x[j]) {
			start = sp->x[j] - sp->x[k];
			sum += start * (sp->y[k] + start * (sp->c[k] / 2 + start * (sp->b[k] / 3 + start * sp->a[k] / 4)));
			start = sp->x[j];
			j++;
			k = j - 1;
		}
		start = end - sp->x[k];
		sum += start * (sp->y[k] + start * (sp->c[k] / 2 + start * (sp->b[k] / 3 + start * sp->a[k] / 4)));
		res[i - 1] = sum;
		start = end;
	}
	return res;
}

/* ------------------------------------------------------------------------- */

// See comments at top
#endif // SKIP_THIS_PASS
#if INCLUDE_PASS < INCLUDE_PASS_LAST
#include __FILE__
#endif
