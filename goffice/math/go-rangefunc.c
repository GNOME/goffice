/*
 * rangefunc.c: Functions on ranges (data sets).
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
 *   Andreas J. Guelzow  <aguelzow@taliesin.ca>
 */

#include <goffice/goffice-config.h>
#include "go-rangefunc.h"

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

static LDOUBLE
SUFFIX(sum_helper) (DOUBLE const *xs, int n)
{
	LDOUBLE sum = 0;
	int i;

	for (i = 0; i < n; i++)
		sum += xs[i];

	return sum;
}

/* ------------------------------------------------------------------------- */

/* Arithmetic sum.  */
int
SUFFIX(go_range_sum) (DOUBLE const *xs, int n, DOUBLE *res)
{
	*res = SUFFIX(sum_helper) (xs, n);
	return 0;
}

/* Arithmetic sum of squares.  */
int
SUFFIX(go_range_sumsq) (DOUBLE const *xs, int n, DOUBLE *res)
{
	/* http://bugzilla.gnome.org/show_bug.cgi?id=131588 */
#ifdef HAVE_LONG_DOUBLE
	long double x, sum = 0;
#else
	DOUBLE x, sum = 0;
#endif
	int i;

	for (i = 0; i < n; i++) {
		x = xs[i];
		sum += x * x;
	}

	*res = sum;
	return 0;
}

/* Arithmetic average.  */
int
SUFFIX(go_range_average) (DOUBLE const *xs, int n, DOUBLE *res)
{
	if (n <= 0)
		return 1;

	*res = SUFFIX(sum_helper) (xs, n) / n;
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

		for (i = 1; i < n; i++)
			if (SUFFIX(fabs) (xs[i]) > max)
				max = SUFFIX(fabs) (xs[i]);
		*res = max;
		return 0;
	} else
		return 1;
}

/* Sum of square deviations from mean.  */
int
SUFFIX(go_range_devsq) (DOUBLE const *xs, int n, DOUBLE *res)
{
	LDOUBLE q = 0;

	if (n > 0) {
		int i;
		LDOUBLE m = SUFFIX(sum_helper) (xs, n) / n;

		for (i = 0; i < n; i++) {
			LDOUBLE dx = xs[i] - m;
			q += dx * dx;
		}
	}
	*res = q;
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
SUFFIX(go_range_increasing) (DOUBLE const *xs, int n)
{
	int i;
	g_return_val_if_fail (n == 0 || xs != NULL, 0);
	for (i = 1; i < n; i++)
		if (xs[i - 1] >= xs[i])
			return 0;
	return 1;
}

int
SUFFIX(go_range_decreasing) (DOUBLE const *xs, int n)
{
	int i;
	g_return_val_if_fail (n == 0 || xs != NULL, 0);
	for (i = 1; i < n; i++)
		if (xs[i - 1] <= xs[i])
			return 0;
	return 1;
}

int
SUFFIX(go_range_vary_uniformly) (DOUBLE const *xs, int n)
{
	return SUFFIX(go_range_increasing) (xs, n) || SUFFIX(go_range_decreasing) (xs, n);
}
