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
#ifdef HAVE_LONG_DOUBLE
#ifdef HAVE_SUNMATH_H
#include <sunmath.h>
#endif
#endif

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
#ifdef HAVE_LONG_DOUBLE
	go_nanl = go_nan;
	go_pinfl = go_pinf;
	go_ninfl = go_ninf;
#endif

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
go_fake_round (double x)
{
	double y = go_fake_floor (fabs (x) + 0.5);
	return (x < 0) ? -y : y;
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

long double go_nanl;
long double go_pinfl;
long double go_ninfl;

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

/*
 * In preparation for truncation, make the value a tiny bit larger (seen
 * absolutely).  This makes ROUND (etc.) behave a little closer to what
 * people want, even if it is a bit bogus.
 */
long double
go_add_epsilonl (long double x)
{
	if (!go_finitel (x) || x == 0)
		return x;
	else {
		int exp;
		long double mant = frexpl (fabsl (x), &exp);
		long double absres = ldexpl (mant + LDBL_EPSILON, exp);
		return (x < 0) ? -absres : absres;
	}
}

long double
go_sub_epsilonl (long double x)
{
	if (!go_finitel (x) || x == 0)
		return x;
	else {
		int exp;
		long double mant = frexpl (fabsl (x), &exp);
		long double absres = ldexpl (mant - LDBL_EPSILON, exp);
		return (x < 0) ? -absres : absres;
	}
}

long double
go_fake_floorl (long double x)
{
	return (x >= 0)
		? floorl (go_add_epsilonl (x))
		: floorl (go_sub_epsilonl (x));
}

long double
go_fake_ceill (long double x)
{
	return (x >= 0)
		? ceill (go_sub_epsilonl (x))
		: ceill (go_add_epsilonl (x));
}

long double
go_fake_roundl (long double x)
{
	long double y = go_fake_floorl (fabsl (x) + 0.5L);
	return (x < 0) ? -y : y;
}

long double
go_fake_truncl (long double x)
{
	return (x >= 0)
		? floorl (go_add_epsilonl (x))
		: -floorl (go_add_epsilonl (-x));
}

#ifndef HAVE_LDEXPL
long double
ldexpl (long double x, int exp)
{
	if (!finitel (x) || x == 0)
		return x;
	else {
		long double res = x * go_pow2l (exp);
		if (finitel (res))
			return res;
		else {
			errno = ERANGE;
			return (x > 0) ? go_pinfl : go_ninfl;
		}
	}
}
#endif

#ifndef HAVE_FREXPL
long double
frexpl (long double x, int *exp)
{
	long double l2x;

	if (!finitel (x) || x == 0) {
		*exp = 0;
		return x;
	}

	l2x = logl (fabsl (x)) / logl (2);
	*exp = (int)floorl (l2x);

	/*
	 * Now correct the result and adjust things that might have gotten
	 * off-by-one due to rounding.
	 */
	x /= go_pow2l (*exp);
	if (fabsl (x) >= 1.0)
		x /= 2, (*exp)++;
	else if (fabsl (x) < 0.5)
		x *= 2, (*exp)--;

	return x;
}
#endif

#ifndef HAVE_STRTOLD
long double
strtold (char const *str, char **end)
{
#if defined(HAVE_STRING_TO_DECIMAL) && defined(HAVE_DECIMAL_TO_QUADRUPLE)
	long double res;
	decimal_record dr;
	enum decimal_string_form form;
	decimal_mode dm;
	fp_exception_field_type excp;
	char *echar;

	string_to_decimal ((char **)&str, INT_MAX, 0,
			   &dr, &form, &echar);
	if (end) *end = (char *)str;

	if (form == invalid_form) {
		errno = EINVAL;
		return 0.0L;
	}

	dm.rd = fp_nearest;
	decimal_to_quadruple (&res, &dm, &dr, &excp);
        if (excp & ((1 << fp_overflow) | (1 << fp_underflow)))
                errno = ERANGE;
	return res;
#else
	char *myend;
	long double res;
	int count;

	if (end == 0) end = &myend;
	(void) strtod (str, end);
	if (str == *end)
		return 0.0L;

	errno = 0;
	count = sscanf (str, "%Lf", &res);
	if (count == 1)
		return res;

	/* Now what?  */
	*end = (char *)str;
	errno = ERANGE;
	return 0.0;
#endif
}
#endif

#ifndef HAVE_EXPM1L
long double
expm1l (long double x)
{
	long double y, a = fabsl (x);

	if (a > 1e-8) {
		y = expl (x) - 1;
		if (a > 0.697)
			return y;  /* negligible cancellation */
	} else {
		if (a < LDBL_EPSILON)
			return x;
		/* Taylor expansion, more accurate in this range */
		y = (x / 2 + 1) * x;
	}

	/* Newton step for solving   log(1 + y) = x   for y : */
	y -= (1 + y) * (log1pl (y) - x);
	return y;
}
#endif

#ifndef HAVE_ASINHL
long double
asinhl (long double x)
{
  long double y = fabsl (x);
  long double r = log1pl (y * y / (hypotl (y, 1.0) + 1.0) + y);
  return (x >= 0) ? r : -r;
}
#endif

#ifndef HAVE_ACOSHL
long double
acoshl (long double x)
{
  long double xm1 = x - 1;
  return log1pl (xm1 + sqrtl (xm1) * sqrtl (x + 1.0));
}
#endif

#ifndef HAVE_ATANHL
long double
atanhl (long double x)
{
  long double y = fabsl (x);
  long double r = -0.5 * log1pl (-(y + y) / (1.0 + y));
  return (x >= 0) ? r : -r;
}
#endif

#ifndef HAVE_MODFL
long double
modfl (long double x, long double *iptr)
{
	if (isnanl (x))
		return *iptr = x;
	else if (finitel (x)) {
		if (x >= 0)
			return x - (*iptr = floorl (x));
		else
			return x - (*iptr = -floorl (-x));			
	} else {
		*iptr = x;
		return 0;
	}
}
#endif

#endif

/* ------------------------------------------------------------------------- */

void
gnm_continued_fraction (double val, int max_denom, int *res_num, int *res_denom)
{
	int n1, n2, d1, d2;
	double x, y;

	if (val < 0) {
		gnm_continued_fraction (-val, max_denom, res_num, res_denom);
		*res_num = -*res_num;
		return;
	}

	n1 = 0; d1 = 1;
	n2 = 1; d2 = 0;

	x = val;
	y = 1;

	do {
		int a = (int) (x / y);
		double newy = x - a * y;
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
	if (bd > max_denom || fabs (val * ad - an) < fabs (val * bd - bn)) {
		*res_num = an;
		*res_denom = ad;
	} else {
		*res_num = bn;
		*res_denom = bd;
	}
}
