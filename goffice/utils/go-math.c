/*
 * go-math.c:  Mathematical functions.
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
 */

#include <goffice/goffice-config.h>
#include "go-math.h"
#include <glib/gmessages.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>

#if defined (HAVE_IEEEFP_H) || defined (HAVE_IEEE754_H)
/* Make sure we have this symbol defined, since the existance of either
   header file implies it.  */
#ifndef IEEE_754
#define IEEE_754
#endif
#endif

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifdef HAVE_IEEE754_H
#include <ieee754.h>
#endif

#define ML_UNDERFLOW (GO_EPSILON * GO_EPSILON)

double go_nan;
double go_pinf;
double go_ninf;

void
go_math_init (void)
{
	const char *bug_url = "http://bugzilla.gnome.org/enter_bug.cgi?product=gnumeric";
	char *old_locale;
	double d;
#ifdef SIGFPE
	void (*signal_handler)(int) = (void (*)(int))signal (SIGFPE, SIG_IGN);
#endif

	go_pinf = HUGE_VAL;
	if (go_pinf > 0 && !go_finite (go_pinf))
		goto have_pinf;

#if defined(INFINITY) && defined(__STDC_IEC_559__)
	go_pinf = INFINITY;
	if (go_pinf > 0 && !go_finite (go_pinf))
		goto have_pinf;
#endif

	/* Try sscanf with fixed strings.  */
	old_locale = setlocale (LC_ALL, "C");
	if (sscanf ("Inf", "%lf", &d) != 1 &&
	    sscanf ("+Inf", "%lf", &d) != 1)
		d = 0;
	setlocale (LC_ALL, old_locale);
	go_pinf = d;
	if (go_pinf > 0 && !go_finite (go_pinf))
		goto have_pinf;

	/* Try overflow.  */
	go_pinf = (HUGE_VAL * HUGE_VAL);
	if (go_pinf > 0 && !go_finite (go_pinf))
		goto have_pinf;

	g_error ("Failed to generate +Inf.  Please report at %s",
		 bug_url);
	abort ();

 have_pinf:
	/* ---------------------------------------- */

	go_ninf = -go_pinf;
	if (go_ninf < 0 && !go_finite (go_ninf))
		goto have_ninf;

	g_error ("Failed to generate -Inf.  Please report at %s",
		 bug_url);
	abort ();

 have_ninf:
	/* ---------------------------------------- */

	go_nan = go_pinf * 0.0;
	if (isnan (go_nan))
		goto have_nan;

	/* Try sscanf with fixed strings.  */
	old_locale = setlocale (LC_ALL, "C");
	if (sscanf ("NaN", "%lf", &d) != 1 &&
	    sscanf ("NAN", "%lf", &d) != 1 &&
	    sscanf ("+NaN", "%lf", &d) != 1 &&
	    sscanf ("+NAN", "%lf", &d) != 1)
		d = 0;
	setlocale (LC_ALL, old_locale);
	go_nan = d;
	if (isnan (go_nan))
		goto have_nan;

	go_nan = go_pinf / go_pinf;
	if (isnan (go_nan))
		goto have_nan;

	g_error ("Failed to generate NaN.  Please report at %s",
		 bug_url);
	abort ();

 have_nan:
#ifdef SIGFPE
	signal (SIGFPE, signal_handler);
#endif
	return;
}

/*
 * In preparation for truncation, make the value a tiny bit larger (seen
 * absolutely).  This makes ROUND (etc.) behave a little closer to what
 * people want, even if it is a bit bogus.
 */
double
go_add_epsilon (double x)
{
	if (!go_finite (x) || x == 0)
		return x;
	else {
		int exp;
		double mant = frexp (fabs (x), &exp);
		double absres = ldexp (mant + DBL_EPSILON, exp);
		return (x < 0) ? -absres : absres;
	}
}

double
go_sub_epsilon (double x)
{
	if (!go_finite (x) || x == 0)
		return x;
	else {
		int exp;
		double mant = frexp (fabs (x), &exp);
		double absres = ldexp (mant - DBL_EPSILON, exp);
		return (x < 0) ? -absres : absres;
	}
}

double
go_fake_floor (double x)
{
	return (x >= 0)
		? floor (go_add_epsilon (x))
		: floor (go_sub_epsilon (x));
}

double
go_fake_ceil (double x)
{
	return (x >= 0)
		? ceil (go_sub_epsilon (x))
		: ceil (go_add_epsilon (x));
}

double
go_fake_trunc (double x)
{
	return (x >= 0)
		? floor (go_add_epsilon (x))
		: -floor (go_add_epsilon (-x));
}

int
go_finite (double x)
{
	/* What a circus!  */
#ifdef HAVE_FINITE
	return finite (x);
#elif defined(HAVE_ISFINITE)
	return isfinite (x);
#elif defined(FINITE)
	return FINITE (x);
#else
	x = fabs (x);
	return x < HUGE_VAL;
#endif
}

double
go_pow2 (int n)
{
	g_assert (FLT_RADIX == 2);
	return ldexp (1.0, n);
}

double
go_pow10 (int n)
{
	static const double fast[] = {
		1e-20, 1e-19, 1e-18, 1e-17, 1e-16, 1e-15, 1e-14, 1e-13, 1e-12, 1e-11,
		1e-10, 1e-09, 1e-08, 1e-07, 1e-06, 1e-05, 1e-04, 1e-03, 1e-02, 1e-01,
		1e+0,
		1e+01, 1e+02, 1e+03, 1e+04, 1e+05, 1e+06, 1e+07, 1e+08, 1e+09, 1e+10,
		1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19, 1e+20
	};

	if (n >= -20 && n <= 20)
		return (fast + 20)[n];

	return pow (10.0, n);
}

/* ------------------------------------------------------------------------- */

#ifdef HAVE_LONG_DOUBLE

long double
go_pow2l (int n)
{
#ifdef HAVE_LDEXPL
	g_assert (FLT_RADIX == 2);
	return ldexpl (1.0L, n);
#else
	return powl (2.0L, n);
#endif
}

long double
go_pow10l (int n)
{
	static const long double fast[] = {
		1e-20L, 1e-19L, 1e-18L, 1e-17L, 1e-16L, 1e-15L, 1e-14L, 1e-13L, 1e-12L, 1e-11L,
		1e-10L, 1e-09L, 1e-08L, 1e-07L, 1e-06L, 1e-05L, 1e-04L, 1e-03L, 1e-02L, 1e-01L,
		1e+0L,
		1e+01L, 1e+02L, 1e+03L, 1e+04L, 1e+05L, 1e+06L, 1e+07L, 1e+08L, 1e+09L, 1e+10L,
		1e+11L, 1e+12L, 1e+13L, 1e+14L, 1e+15L, 1e+16L, 1e+17L, 1e+18L, 1e+19L, 1e+20L
	};

	if (n >= -20 && n <= 20)
		return (fast + 20)[n];

	return powl (10.0L, n);
}

#endif

/* ------------------------------------------------------------------------- */


/****************************************************************/

/*
 * In preparation for truncation, make the value a tiny bit larger (seen
 * absolutely).  This makes ROUND (etc.) behave a little closer to what
 * people want, even if it is a bit bogus.
 */
gnm_float
gnm_add_epsilon (gnm_float x)
{
	if (!gnm_finite (x) || x == 0)
		return x;
	else {
		int exp;
		gnm_float mant = gnm_frexp (gnm_abs (x), &exp);
		gnm_float absres = gnm_ldexp (mant + GNM_EPSILON, exp);
		return (x < 0) ? -absres : absres;
	}
}

gnm_float
gnm_sub_epsilon (gnm_float x)
{
	if (!gnm_finite (x) || x == 0)
		return x;
	else {
		int exp;
		gnm_float mant = gnm_frexp (gnm_abs (x), &exp);
		gnm_float absres = gnm_ldexp (mant - GNM_EPSILON, exp);
		return (x < 0) ? -absres : absres;
	}
}

/*
 *  2.000...0000001   -> 2
 *  2.000...0000000   -> 2
 *  1.999...9999999   -> 2
 * -0.999...9999999   -> -1
 * -1.000...0000000   -> -1
 * -1.000...0000001   -> -1
 */
gnm_float
gnm_fake_floor (gnm_float x)
{
	static const gnm_float cutoff = 0.5 / GNM_EPSILON;

	if (gnm_abs (x) < cutoff)
		x = (x >= 0)
			? gnm_add_epsilon (x)
			: gnm_sub_epsilon (x);

	return gnm_floor (x);
}

/*
 *  2.000...0000001   -> 2
 *  2.000...0000000   -> 2
 *  1.999...9999999   -> 2
 * -0.999...9999999   -> -1
 * -1.000...0000000   -> -1
 * -1.000...0000001   -> -1
 */
gnm_float
gnm_fake_ceil (gnm_float x)
{
	static const gnm_float cutoff = 0.5 / GNM_EPSILON;

	if (gnm_abs (x) < cutoff)
		x = (x >= 0)
			? gnm_sub_epsilon (x)
			: gnm_add_epsilon (x);

	return gnm_ceil (x);
}

gnm_float
gnm_fake_round (gnm_float x)
{
	gnm_float y = gnm_fake_floor (gnm_abs (x) + 0.5);
	return (x < 0) ? -y : y;
}

gnm_float
gnm_fake_trunc (gnm_float x)
{
	gnm_float y = gnm_fake_floor (gnm_abs (x));
	return (x < 0) ? -y : y;
}

void
gnm_continued_fraction (gnm_float val, int max_denom, int *res_num, int *res_denom)
{
	int n1, n2, d1, d2;
	gnm_float x, y;

	if (val < 0) {
		gnm_continued_fraction (gnm_abs (val), max_denom, res_num, res_denom);
		*res_num = -*res_num;
		return;
	}

	n1 = 0; d1 = 1;
	n2 = 1; d2 = 0;

	x = val;
	y = 1;

	do {
		int a = (int) (x / y);
		gnm_float newy = x - a * y;
		int n3, d3;

		if ((n2 && a > (INT_MAX - n1) / n2) ||
		    (d2 && a > (INT_MAX - d1) / d2) ||
		    a * d2 + d1 > max_denom) {
			*res_num = n2;
			*res_denom = d2;
			return;
		}

		n3 = a * n2 + n1;
		d3 = a * d2 + d1;

		x = y;
		y = newy;

		n1 = n2; n2 = n3;
		d1 = d2; d2 = d3;
	} while (y > 1e-10);

	*res_num = n2;
	*res_denom = d2;
}


void
go_stern_brocot (double val, int max_denom, int *res_num, int *res_denom)
{
	int an = 0, ad = 1;
	int bn = 1, bd = 1;
	int n, d;
	double sp, delta;

	while ((d = ad + bd) <= max_denom) {
		sp = 1e-5 * d;/* Quick and dirty,  do adaptive later */
		n = an + bn;
		delta = val * d - n;
		if (delta > sp) {
			an = n;
			ad = d;
		} else if (delta < -sp) {
			bn = n;
			bd = d;
		} else {
			*res_num = n;
			*res_denom = d;
			return;
		}
	}
	if (bd > max_denom || gnm_abs (val * ad - an) < gnm_abs (val * bd - bn)) {
		*res_num = an;
		*res_denom = ad;
	} else {
		*res_num = bn;
		*res_denom = bd;
	}
}
