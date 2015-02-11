/*
 * rangefunc.c: Functions on ranges (data sets).
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
 *   Andreas J. Guelzow  <aguelzow@taliesin.ca>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef DOUBLE

#define DOUBLE double
#ifdef HAVE_LONG_DOUBLE
#define LDOUBLE long double
#endif
#define SUFFIX(_n) _n

#ifdef GOFFICE_WITH_LONG_DOUBLE
#include "go-rangefunc.c"
#undef DOUBLE
#undef LDOUBLE
#undef SUFFIX

#ifdef HAVE_SUNMATH_H
#include <sunmath.h>
#endif
#define DOUBLE long double
#define SUFFIX(_n) _n ## l
#endif

#endif

#ifndef LDOUBLE
#define LDOUBLE DOUBLE
#endif

/* ------------------------------------------------------------------------- */

static SUFFIX(GOAccumulator)*
SUFFIX(sum_helper) (DOUBLE const *xs, int n)
{
	SUFFIX(GOAccumulator) *acc = SUFFIX(go_accumulator_new) ();
	while (n > 0) {
		n--;
		SUFFIX(go_accumulator_add) (acc, xs[n]);
	}
	return acc;
}


/* Arithmetic sum.  */
int
SUFFIX(go_range_sum) (DOUBLE const *xs, int n, DOUBLE *res)
{
	void *state = SUFFIX(go_accumulator_start) ();
	SUFFIX(GOAccumulator) *acc = SUFFIX(sum_helper) (xs, n);
	*res = SUFFIX(go_accumulator_value) (acc);
	SUFFIX(go_accumulator_free) (acc);
	SUFFIX(go_accumulator_end) (state);
	return 0;
}

/* Arithmetic sum of squares.  */
int
SUFFIX(go_range_sumsq) (DOUBLE const *xs, int n, DOUBLE *res)
{
	void *state = SUFFIX(go_accumulator_start) ();
	SUFFIX(GOAccumulator) *acc = SUFFIX(go_accumulator_new) ();
	while (n > 0) {
		DOUBLE x = xs[--n];
		SUFFIX(GOQuad) q;
		SUFFIX(go_quad_mul12) (&q, x, x);
		SUFFIX(go_accumulator_add_quad) (acc, &q);
	}
	*res = SUFFIX(go_accumulator_value) (acc);
	SUFFIX(go_accumulator_free) (acc);
	SUFFIX(go_accumulator_end) (state);
	return 0;
}

/* Arithmetic average.  */
int
SUFFIX(go_range_average) (DOUBLE const *xs, int n, DOUBLE *res)
{
	if (n <= 0)
		return 1;

	if (SUFFIX(go_range_constant) (xs, n)) {
		*res = xs[0];
		return 0;
	}

	SUFFIX(go_range_sum) (xs, n, res);
	*res /= n;
	return 0;
}

/* Minimum element.  */
int
SUFFIX(go_range_min) (DOUBLE const *xs, int n, DOUBLE *res)
{
	if (n > 0) {
		DOUBLE min = xs[0];
		int i;

		for (i = 1; i < n; i++)
			if (xs[i] < min)
				min = xs[i];
		*res = min;
		return 0;
	} else
		return 1;
}

/* Maximum element.  */
int
SUFFIX(go_range_max) (DOUBLE const *xs, int n, DOUBLE *res)
{
	if (n > 0) {
		DOUBLE max = xs[0];
		int i;

		for (i = 1; i < n; i++)
			if (xs[i] > max)
				max = xs[i];
		*res = max;
		return 0;
	} else
		return 1;
}

/* Maximum absolute element.  */
int
SUFFIX(go_range_maxabs) (DOUBLE const *xs, int n, DOUBLE *res)
{
	if (n > 0) {
		DOUBLE max = SUFFIX(fabs) (xs[0]);
		int i;

		for (i = 1; i < n; i++) {
			DOUBLE xabs = SUFFIX(fabs) (xs[i]);
			if (xabs > max)
				max = xabs;
		}
		*res = max;
		return 0;
	} else
		return 1;
}

/* Sum of square deviations from mean.  */
int
SUFFIX(go_range_devsq) (DOUBLE const *xs, int n, DOUBLE *res)
{
	if (SUFFIX(go_range_constant) (xs, n))
		*res = 0;
	else {
		void *state;
		SUFFIX(GOAccumulator) *acc;
		DOUBLE sum, sumh, suml;
		SUFFIX(GOQuad) qavg, qtmp, qn;

		state = SUFFIX(go_accumulator_start) ();
		acc = SUFFIX(sum_helper) (xs, n);
		sumh = SUFFIX(go_accumulator_value) (acc);
		SUFFIX(go_accumulator_add) (acc, -sumh);
		suml = SUFFIX(go_accumulator_value) (acc);

		SUFFIX(go_quad_init) (&qavg, sumh);
		SUFFIX(go_quad_init) (&qtmp, suml);
		SUFFIX(go_quad_add) (&qavg, &qavg, &qtmp);
		SUFFIX(go_range_sum) (xs, n, &sum);
		SUFFIX(go_quad_init) (&qn, n);
		SUFFIX(go_quad_div) (&qavg, &qavg, &qn);
		/*
		 * The just-generated qavg isn't exact, but should be
		 * good to near-GOQuad precision.  We hope that's good
		 * enough.
		 */

		SUFFIX(go_accumulator_clear) (acc);
		while (n > 0) {
			SUFFIX(GOQuad) q;
			n--;
			SUFFIX(go_quad_init) (&q, xs[n]);
			SUFFIX(go_quad_sub) (&q, &q, &qavg);
			SUFFIX(go_quad_mul) (&q, &q, &q);
			SUFFIX(go_accumulator_add_quad) (acc, &q);
		}
		*res = SUFFIX(go_accumulator_value) (acc);
		SUFFIX(go_accumulator_free) (acc);
		SUFFIX(go_accumulator_end) (state);
	}
	return 0;
}

static int
SUFFIX(float_compare) (DOUBLE const *a, DOUBLE const *b)
{
	if (*a < *b)
		return -1;
	else if (*a == *b)
		return 0;
	else
		return 1;
}

DOUBLE *
SUFFIX(go_range_sort) (DOUBLE const *xs, int n)
{
	if (n <= 0)
		return NULL;
	else {
		DOUBLE *ys = g_new (DOUBLE, n);
		memcpy (ys, xs, n * sizeof (DOUBLE));
		qsort (ys, n, sizeof (ys[0]), (int (*) (const void *, const void *))&SUFFIX(float_compare));
		return ys;
	}
}
/* This requires sorted data.  */
int
SUFFIX(go_range_fractile_inter_sorted) (DOUBLE const *xs, int n, DOUBLE *res, DOUBLE f)
{
	DOUBLE fpos, residual;
	int pos;

	if (n <= 0 || f < 0.0 || f > 1.0)
		return 1;

	fpos = (n - 1) * f;
	pos = (int)fpos;
	residual = fpos - pos;

	if (residual == 0.0 || pos + 1 >= n)
		*res = xs[pos];
	else
		*res = (1 - residual) * xs[pos] + residual * xs[pos + 1];

	return 0;
}

int
SUFFIX(go_range_fractile_inter) (DOUBLE const *xs, int n, DOUBLE *res, DOUBLE f)
{
	DOUBLE *ys = SUFFIX(go_range_sort) (xs, n);
	int error = SUFFIX(go_range_fractile_inter_sorted) (ys, n, res, f);
	g_free (ys);
	return error;
}

int
SUFFIX(go_range_fractile_inter_nonconst) (DOUBLE *xs, int n, DOUBLE *res, DOUBLE f)
{
	qsort (xs, n, sizeof (xs[0]), (int (*) (const void *, const void *))&SUFFIX(float_compare));
	return SUFFIX(go_range_fractile_inter_sorted) (xs, n, res, f);
}

int
SUFFIX(go_range_median_inter) (DOUBLE const *xs, int n, DOUBLE *res)
{
	return SUFFIX(go_range_fractile_inter) (xs, n, res, 0.5);
}

int
SUFFIX(go_range_median_inter_sorted) (DOUBLE const *xs, int n, DOUBLE *res)
{
	return SUFFIX(go_range_fractile_inter_sorted) (xs, n, res, 0.5);
}

int
SUFFIX(go_range_median_inter_nonconst) (DOUBLE *xs, int n, DOUBLE *res)
{
	return SUFFIX(go_range_fractile_inter_nonconst) (xs, n, res, 0.5);
}

int
SUFFIX(go_range_constant) (DOUBLE const *xs, int n)
{
	int i;
	for (i = 1; i < n; i++)
		if (xs[0] != xs[i])
			return 0;
	return 1;
}

int
SUFFIX(go_range_increasing) (DOUBLE const *xs, int n)
{
	int i = 0;
	DOUBLE last;
	g_return_val_if_fail (n == 0 || xs != NULL, 0);
	while ( i < n && isnan (xs[i]))
		i++;
	if (i == n)
		return 0;
	last = xs[i];
	for (i = i + 1; i < n; i++) {
		if (isnan (xs[i]))
		    continue;
		if (last >= xs[i])
			return 0;
		last = xs[i];
	}
	return 1;
}

int
SUFFIX(go_range_decreasing) (DOUBLE const *xs, int n)
{
	int i = 0;
	DOUBLE last;
	g_return_val_if_fail (n == 0 || xs != NULL, 0);
	while ( i < n &&  isnan (xs[i]))
		i++;
	if (i == n)
		return 0;
	last = xs[i];
	for (i = i + 1; i < n; i++) {
		if (isnan (xs[i]))
		    continue;
		if (last <= xs[i])
			return 0;
		last = xs[i];
	}
	return 1;
}

int
SUFFIX(go_range_vary_uniformly) (DOUBLE const *xs, int n)
{
	return SUFFIX(go_range_increasing) (xs, n) || SUFFIX(go_range_decreasing) (xs, n);
}
