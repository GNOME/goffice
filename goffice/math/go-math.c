/*
 * go-math.c:  Mathematical functions.
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
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
#include "go-math.h"
#include <goffice/utils/go-locale.h>
#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

double go_nan;
double go_pinf;
double go_ninf;

#ifdef GOFFICE_WITH_LONG_DOUBLE
static gboolean
running_under_buggy_valgrind (void)
{
	long double one = atof ("1");
	if (one * LDBL_MIN > (long double)0)
		return FALSE;

	/*
	 * We get here when long double fails to satisfy a requirement of
	 * C99, namely that LDBL_MIN is positive.  That is probably
	 * valgrind mapping long doubles to doubles.
	 *
	 * Chances are that go_pinfl/go_ninf/go_nanl are fine, but that
	 * finitel fails.  Perform alternate tests.
	 */

	if (!(go_pinfl > DBL_MAX && !isnanl (go_pinfl) && isnanl (go_pinfl - go_pinfl)))
		return FALSE;

	if (!(-go_ninfl > DBL_MAX && !isnanl (go_ninfl) && isnanl (go_ninfl - go_ninfl)))
		return FALSE;

	if (!isnanl (go_nanl) && !(go_nanl >= 0) && !(go_nanl <= 0))
		return FALSE;

	/* finitel must be hosed.  Blame valgrind.  */
	return TRUE;
}
#endif

void
_go_math_init (void)
{
	const char *bug_url = "https://gitlab.gnome.org/GNOME/goffice/issues";
	char *old_locale;
	double d;
#ifdef SIGFPE
	void (*signal_handler)(int) = (void (*)(int))signal (SIGFPE, SIG_IGN);
#endif

	old_locale = g_strdup (setlocale (LC_ALL, NULL));

	go_pinf = HUGE_VAL;
	if (go_pinf > 0 && !go_finite (go_pinf))
		goto have_pinf;

#if defined(INFINITY) && defined(__STDC_IEC_559__)
	go_pinf = INFINITY;
	if (go_pinf > 0 && !go_finite (go_pinf))
		goto have_pinf;
#endif

	/* Try sscanf with fixed strings.  */
	setlocale (LC_ALL, "C");
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

#if defined (__GNUC__) && defined(__FAST_MATH__)
	/*
	 * It seems that -ffast-math implies -ffinite-math-only and that
	 * is not going to work in goffice.
	 */
#error "Please do not compile this code with -ffast-math.  It will not work."
#endif

	go_nan = fabs (go_pinf * 0.0);
	if (isnan (go_nan))
		goto have_nan;

	/* Try sscanf with fixed strings.  */
	setlocale (LC_ALL, "C");
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

#ifdef GOFFICE_WITH_LONG_DOUBLE
	go_nanl = go_nan;
	go_pinfl = go_pinf;
	go_ninfl = go_ninf;
	if (!(isnanl (go_nanl) &&
	      go_pinfl > 0 && !go_finitel (go_pinfl) &&
	      go_ninfl < 0 && !go_finitel (go_ninfl))) {
		if (running_under_buggy_valgrind ()) {
			g_warning ("Running under buggy valgrind, see http://bugs.kde.org/show_bug.cgi?id=164298");
		} else {
			g_error ("Failed to generate long double NaN/+Inf/-Inf.\n"
				 "    go_nanl=%.20Lg\n"
				 "    go_pinfl=%.20Lg\n"
				 "    go_ninfl=%.20Lg\n"
				 "Please report at %s",
				 go_nanl, go_pinfl, go_ninfl,
				 bug_url);
			abort ();
		}
	}
#endif

#ifdef GOFFICE_WITH_DECIMAL64
	go_nanD = go_nan;
	go_pinfD = go_pinf;
	go_ninfD = go_ninf;
	if (!(isnanD (go_nanD) &&
	      go_pinfD > 0 && !finiteD (go_pinfD) &&
	      go_ninfD < 0 && !finiteD (go_ninfD))) {
		g_error ("Failed to generate _Decimal64 NaN/+Inf/-Inf.");
	}

	{
		// There's a chance that some fool used FLT_RADIX here,
		// but that would be really unhelpful.  The whole point
		// is to have a fast, loss-free scaling function
		_Decimal64 y = scalbnD (1, 1);
		if (y != 10) {
			// Don't try to print _Decimal64 at this stage
			g_error ("We expected scalbnD(1,1) to be 10, but got %g",
				 (double)y);
		}
	}
#endif

	{
		double x = g_ascii_strtod ("24985672148.49707", NULL);
		double sx = sin (x);
		double ref = -0.55266506738426566413304;
		if (fabs (sx - ref) > 0.01) {
			g_warning ("Running with buggy math library, see https://bugzilla.gnome.org/show_bug.cgi?id=726250");
		}
	}

	g_free (old_locale);
#ifdef SIGFPE
	signal (SIGFPE, signal_handler);
#endif
	return;
}

/**
 * go_add_epsilon:
 * @x: a number
 *
 * Returns the next-larger representable value, except that zero and
 * infinites are returned unchanged.
 */
double
go_add_epsilon (double x)
{
	if (!go_finite (x) || x == 0)
		return x;
#ifdef HAVE_NEXTAFTER
	return nextafter (x, go_pinf);
#else
	if (x < 0)
		return 0 - go_sub_epsilon(-x);
	else {
		int e;
		double mant = frexp (fabs (x), &e);
		// mant is in range [0.5; 1)
		return ldexp (mant + DBL_EPSILON / 2, e);
	}
#endif
}

/**
 * go_sub_epsilon:
 * @x: a number
 *
 * Returns the next-smaller representable value, except that zero and
 * infinites are returned unchanged.
 */
double
go_sub_epsilon (double x)
{
	if (!go_finite (x) || x == 0)
		return x;
#ifdef HAVE_NEXTAFTER
	return nextafter (x, go_ninf);
#else
	if (x < 0)
		return 0 - go_add_epsilon(-x);
	else {
		int e;
		double mant = frexp (fabs (x), &e);
		// mant is in range [0.5; 1)
		return ldexp (mant - (mant == 0.5 ? DBL_EPSILON / 4 : DBL_EPSILON / 2), e);
	}
#endif
}

static double
go_d2d (double x)
{
	/* Kill excess precision by forcing a memory read.  */
	const volatile double *px = &x;
	return *px;
}

/**
 * go_fake_floor:
 * @x: value to floor
 *
 * Returns: the floor of @x, ie., the largest integer that is not larger
 * than @x.  However, this variant applies a 1 ulp grace interval for
 * values that are just a hair less than an integer.
 */
double
go_fake_floor (double x)
{
	x = go_d2d (x);

	if (x == floor (x))
		return x;

	return floor (go_add_epsilon (x));
}

/**
 * go_fake_ceil:
 * @x: value to ceil
 *
 * Returns: the ceiling of @x, ie., the smallest integer that is not smaller
 * than @x.  However, this variant applies a 1 ulp grace interval for
 * values that are just a hair larger than an integer.
 */
double
go_fake_ceil (double x)
{
	x = go_d2d (x);

	if (x == floor (x))
		return x;

	return ceil (go_sub_epsilon (x));
}

double
go_fake_round (double x)
{
	double y;

	x = go_d2d (x);

	if (x == floor (x))
		return x;

	/*
	 * Adding a half here is ok.  The only problematic non-integer
	 * case is nextafter(0.5,-1) for which we want to produce 1 here.
	 */
	y = go_fake_floor (fabs (x) + 0.5);
	return (x < 0) ? -y : y;
}

double
go_fake_trunc (double x)
{
	x = go_d2d (x);

	if (x == floor (x))
		return x;

	return (x >= 0)
		? floor (go_add_epsilon (x))
		: -floor (go_add_epsilon (-x));
}

double
go_rint (double x)
{
#ifdef HAVE_RINT
	return rint (x);
#else
	double y = floor (fabs (x) + 0.5);
	return (x < 0) ? -y : y;
#endif
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

/**
 * go_pow2:
 * @n: exponent
 *
 * Computes 2 to the power of @n.  This is fast and accurate.
 */
double
go_pow2 (int n)
{
	return ldexp (1.0, n);
}

#define TEN(n) ONE(n ## 0),ONE(n ## 1),ONE(n ## 2),ONE(n ## 3),ONE(n ## 4),ONE(n ## 5),ONE(n ## 6),ONE(n ## 7),ONE(n ## 8),ONE(n ## 9)
#define HUNDRED(n) TEN(n ## 0),TEN(n ## 1),TEN(n ## 2),TEN(n ## 3),TEN(n ## 4),TEN(n ## 5),TEN(n ## 6),TEN(n ## 7),TEN(n ## 8),TEN(n ## 9)

/**
 * go_pow10:
 * @n: exponent
 *
 * Computes 10 to the power of @n.  This is fast and accurate (under the
 * reasonable assumption that the compiler is accurate).
 */
double
go_pow10 (int n)
{
	// On the assumption that the compiler's handling of double constants
	// is correct, make a large table covering more or less the whole
	// range.  We do not want to depend on the accuracy of pow(10,n)
	// because it is known not to be what it should be.

	static const double pos[] = {
#define ONE(n) 1e+0 ## n
		HUNDRED(0), HUNDRED(1), HUNDRED(2),
		1e+300, 1e+301, 1e+302, 1e+303, 1e+304, 1e+305,
		1e+306, 1e+307, 1e+308
#undef ONE
	};
	static const double neg[] = {
#define ONE(n) 1e-0 ## n
		HUNDRED(0), HUNDRED(1), HUNDRED(2),
		TEN(30), TEN(31),
		1e-320, 1e-321, 1e-322, 1e-323
#undef ONE
	};

	if (n >= 0 && n < (int)G_N_ELEMENTS(pos))
		return pos[n];
	if (n < 0 && (size_t)-n < G_N_ELEMENTS(neg))
		return neg[-n];

	return pow (10.0, n);
}

double
go_pow (double x, double y)
{
	if (x == 10 && y > G_MININT && y < G_MAXINT && y == floor (y))
		return go_pow10 ((int)y);
	return pow (x, y);
}


static int
strtod_helper (const char *s)
{
	const char *p = s;

	while (g_ascii_isspace (*p))
		p++;
	if (*p == '+' || *p == '-')
		p++;
	if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
		/* Disallow C99 hex notation.  */
		return s - (p + 1);

	while (1) {
		if (*p == 'd' || *p == 'D')
			return p - s;

		if (*p == 0 ||
		    g_ascii_isspace (*p) ||
		    g_ascii_isalpha (*p))
			return INT_MAX;

		p++;
	}
}


/**
 * go_strtod:
 * @s: string to convert
 * @end: (out) (transfer none) (optional): pointer to end of string.
 *
 * Returns: the numeric value of the given string.
 * Like strtod, but without hex notation and MS extensions.
 * Unlike strtod, there is no need to reset errno before calling this.
 */
double
go_strtod (const char *s, char **end)
{
	int maxlen = strtod_helper (s);
	int save_errno;
	char *tmp;
	double res;

	if (maxlen == INT_MAX) {
		errno = 0;
		return strtod (s, end);
	} else if (maxlen < 0) {
		/* Hex */
		errno = 0;
		if (end)
			*end = (char *)s - maxlen;
		return 0;
	}

	tmp = g_strndup (s, maxlen);
	errno = 0;
	res = strtod (tmp, end);
	save_errno = errno;
	if (end)
		*end = (char *)s + (*end - tmp);
	g_free (tmp);
	errno = save_errno;

	return res;
}

/**
 * go_ascii_strtod:
 * @s: string to convert
 * @end: optional pointer to end of string.
 *
 * Like g_ascii_strtod, but without hex notation and MS extensions.
 * There is no need to reset errno before calling this.
 */
double
go_ascii_strtod (const char *s, char **end)
{
	int maxlen = strtod_helper (s);
	int save_errno;
	char *tmp;
	double res;

	if (maxlen == INT_MAX)
		return g_ascii_strtod (s, end);
	else if (maxlen < 0) {
		/* Hex */
		errno = 0;
		if (end)
			*end = (char *)s - maxlen;
		return 0;
	}

	tmp = g_strndup (s, maxlen);
	errno = 0;
	res = g_ascii_strtod (tmp, end);
	save_errno = errno;
	if (end)
		*end = (char *)s + (*end - tmp);
	g_free (tmp);
	errno = save_errno;

	return res;
}


#ifdef GOFFICE_SUPPLIED_LOG1P
double
log1p (double x)
{
	double term, sum;
	int i;

	if (fabs (x) > 0.25)
		return log (x + 1);

	i = 0;
	sum = 0;
	term = -1;
	while (fabs (term) > fabs (sum) * DBL_EPSILON) {
		term *= -x;
		i++;
		sum += term / i;
	}

	return sum;
}
#endif

#ifdef GOFFICE_SUPPLIED_EXPM1
double
expm1 (double x)
{
	double y, a = fabs (x);

	if (a > 1e-8) {
		y = exp (x) - 1;
		if (a > 0.697)
			return y;  /* negligible cancellation */
	} else {
		if (a < DBL_EPSILON)
			return x;
		/* Taylor expansion, more accurate in this range */
		y = (x / 2 + 1) * x;
	}

	/* Newton step for solving   log(1 + y) = x   for y : */
	y -= (1 + y) * (log1p (y) - x);
	return y;
}
#endif

#ifdef GOFFICE_SUPPLIED_ASINH
double
asinh (double x)
{
	double y = fabs (x);
	double r = log1p (y * y / (hypot (y, 1.0) + 1.0) + y);
	return (x >= 0) ? r : -r;
}
#endif

#ifdef GOFFICE_SUPPLIED_ACOSH
double
acosh (double x)
{
	double xm1 = x - 1;
	return log1p (xm1 + sqrt (xm1) * sqrt (x + 1.0));
}
#endif

#ifdef GOFFICE_SUPPLIED_ATANH
double
atanh (double x)
{
	double y = fabs (x);
	double r = -0.5 * log1p (-(y + y) / (1.0 + y));
	return (x >= 0) ? r : -r;
}
#endif

/* ------------------------------------------------------------------------- */

#ifdef GOFFICE_WITH_LONG_DOUBLE

long double go_nanl;
long double go_pinfl;
long double go_ninfl;

int
go_finitel (long double x)
{
#ifdef HAVE_FINITEL
	return finitel (x);
#else
	return fabs (x) < go_pinfl;
#endif
}

long double
go_pow2l (int n)
{
#ifdef GOFFICE_SUPPLIED_LDEXPL
	return powl (2.0L, n);
#else
	return ldexpl (1.0L, n);
#endif
}

long double
go_pow10l (int n)
{
	// On the assumption that the compiler's handling of long double
	// constants is correct, make a large table covering more or less
	// the whole range of long double.
	// We do not want to depend on the accuracy of powl(10,n)
	// because it is known not to be what it should be.

	static const long double pos[] = {
#define ONE(n) 1E+0 ## n ## L
		HUNDRED(0), HUNDRED(1), HUNDRED(2), HUNDRED(3), HUNDRED(4),
		HUNDRED(5), HUNDRED(6), HUNDRED(7), HUNDRED(8), HUNDRED(9),
		HUNDRED(10), HUNDRED(11), HUNDRED(12), HUNDRED(13), HUNDRED(14),
		HUNDRED(15), HUNDRED(16), HUNDRED(17), HUNDRED(18), HUNDRED(19),
		HUNDRED(20), HUNDRED(21), HUNDRED(22), HUNDRED(23), HUNDRED(24),
		HUNDRED(25), HUNDRED(26), HUNDRED(27), HUNDRED(28), HUNDRED(29),
		HUNDRED(30), HUNDRED(31), HUNDRED(32), HUNDRED(33), HUNDRED(34),
		HUNDRED(35), HUNDRED(36), HUNDRED(37), HUNDRED(38), HUNDRED(39),
		HUNDRED(40), HUNDRED(41), HUNDRED(42), HUNDRED(43), HUNDRED(44),
		HUNDRED(45), HUNDRED(46), HUNDRED(47), HUNDRED(48),
		TEN(490), TEN(491), TEN(492),
		1E+4930L, 1E+4931L, 1E+4932L
#undef ONE
	};
	static const long double neg[] = {
#define ONE(n) 1E-0 ## n ## L
		HUNDRED(0), HUNDRED(1), HUNDRED(2), HUNDRED(3), HUNDRED(4),
		HUNDRED(5), HUNDRED(6), HUNDRED(7), HUNDRED(8), HUNDRED(9),
		HUNDRED(10), HUNDRED(11), HUNDRED(12), HUNDRED(13), HUNDRED(14),
		HUNDRED(15), HUNDRED(16), HUNDRED(17), HUNDRED(18), HUNDRED(19),
		HUNDRED(20), HUNDRED(21), HUNDRED(22), HUNDRED(23), HUNDRED(24),
		HUNDRED(25), HUNDRED(26), HUNDRED(27), HUNDRED(28), HUNDRED(29),
		HUNDRED(30), HUNDRED(31), HUNDRED(32), HUNDRED(33), HUNDRED(34),
		HUNDRED(35), HUNDRED(36), HUNDRED(37), HUNDRED(38), HUNDRED(39),
		HUNDRED(40), HUNDRED(41), HUNDRED(42), HUNDRED(43), HUNDRED(44),
		HUNDRED(45), HUNDRED(46), HUNDRED(47), HUNDRED(48),
		TEN(490), TEN(491), TEN(492), TEN(493), TEN(494),
		1E-4950L
#undef ONE
	};

	if (n >= 0 && n < (int)G_N_ELEMENTS(pos))
		return pos[n];
	if (n < 0 && (size_t)-n < G_N_ELEMENTS(neg))
		return neg[-n];

	return powl (10.0L, n);
}
#undef TEN
#undef HUNDRED

long double
go_powl (long double x, long double y)
{
	if (x == 10 && y > G_MININT && y < G_MAXINT && y == floor (y))
		return go_pow10l ((int)y);
	return powl (x, y);
}

long double
go_log10l (long double x)
{
	long double l = log10l (x);
	int il;

	// Quick rejection of stuff we cannot improve.  10^28 is too big
	// for long double to be exact.
	if (!(x == floorl (x)) || x < 1 || x > 1e27L)
		return l;

	il = (int)(l + 0.5);
	if (x != go_pow10l (il))
		return l;

	return il;
}


/**
 * go_strtold:
 * @s: string to convert
 * @end: (out) (transfer none) (optional): pointer to end of string.
 *
 * Returns: the numeric value of the given string.
 * Like strtold, but without hex notation and MS extensions.
 * Unlike strtold, there is no need to reset errno before calling this.
 */
long double
go_strtold (const char *s, char **end)
{
	int maxlen = strtod_helper (s);
	int save_errno;
	char *tmp;
	long double res;

	if (maxlen == INT_MAX) {
		errno = 0;
		return strtold (s, end);
	} else if (maxlen < 0) {
		errno = 0;
		if (end)
			*end = (char *)s - maxlen;
		return 0;
	}

	tmp = g_strndup (s, maxlen);
	errno = 0;
	res = strtold (tmp, end);
	save_errno = errno;
	if (end)
		*end = (char *)s + (*end - tmp);
	g_free (tmp);
	errno = save_errno;

	return res;
}

/**
 * go_ascii_strtold:
 * @s: string to convert
 * @end: optional pointer to end of string.
 *
 * Like strtold, but without hex notation and MS extensions and also
 * assuming "C" locale.
 * Unlike strtold, there is no need to reset errno before calling this.
 */
long double
go_ascii_strtold (const char *s, char **end)
{
	GString *tmp;
	const GString *decimal;
	int save_errno;
	char *the_end;
	/* Use the "double" version for parsing.  */
	long double res = go_ascii_strtod (s, &the_end);
	if (end)
		*end = the_end;
	if (the_end == s)
		return res;

	decimal = go_locale_get_decimal ();
	tmp = g_string_sized_new (the_end - s + 10);
	while (s < the_end) {
		if (*s == '.') {
			g_string_append_len (tmp, decimal->str, decimal->len);
			g_string_append (tmp, ++s);
			break;
		}
		g_string_append_c (tmp, *s++);
	}
	errno = 0;
	res = strtold (tmp->str, NULL);
	save_errno = errno;
	g_string_free (tmp, TRUE);
	errno = save_errno;

	return res;
}

long double
go_add_epsilonl (long double x)
{
	if (!go_finitel (x) || x == 0)
		return x;
#ifdef HAVE_NEXTAFTERL
	return nextafterl (x, go_pinfl);
#else
	if (x < 0)
		return 0 - go_sub_epsilonl(-x);
	else {
		int e;
		long double mant = frexpl (fabsl (x), &e);
		// mant is in range [0.5; 1)
		return ldexpl (mant + LDBL_EPSILON / 2, e);
	}
#endif
}

long double
go_sub_epsilonl (long double x)
{
	if (!go_finitel (x) || x == 0)
		return x;
#ifdef HAVE_NEXTAFTERL
	return x == 0 ? x : nextafterl (x, go_ninfl);
#else
	if (x < 0)
		return 0 - go_add_epsilonl(-x);
	else {
		int e;
		long double mant = frexpl (fabsl (x), &e);
		// mant is in range [0.5; 1)
		return ldexpl (mant - (mant == 0.5L ? LDBL_EPSILON / 4 : LDBL_EPSILON / 2), e);
	}
#endif
}

long double
go_fake_floorl (long double x)
{
	if (x == floorl (x))
		return x;

	return floorl (go_add_epsilonl (x));
}

long double
go_fake_ceill (long double x)
{
	if (x == floorl (x))
		return x;

	return ceill (go_sub_epsilonl (x));
}

long double
go_fake_roundl (long double x)
{
	long double y;

	if (x == floorl (x))
		return x;

	/*
	 * Adding a half here is ok.  The only problematic non-integer
	 * case is nextafter(0.5,-1) for which we want to produce 1 here.
	 */
	y = go_fake_floorl (fabsl (x) + 0.5L);
	return (x < 0) ? 0 - y : y;
}

long double
go_fake_truncl (long double x)
{
	if (x == floorl (x))
		return x;

	return (x >= 0)
		? floorl (go_add_epsilonl (x))
		: 0 - floorl (go_add_epsilonl (-x));
}

#ifdef GOFFICE_SUPPLIED_LDEXPL
long double
ldexpl (long double x, int e)
{
	if (!go_finitel (x) || x == 0)
		return x;
	else {
		long double res = x * go_pow2l (e);
		if (go_finitel (res))
			return res;
		else {
			errno = ERANGE;
			return (x > 0) ? go_pinfl : go_ninfl;
		}
	}
}
#endif

#ifdef GOFFICE_SUPPLIED_FREXPL
long double
frexpl (long double x, int *e)
{
	long double l2x;

	if (!go_finitel (x) || x == 0) {
		*e = 0;
		return x;
	}

	l2x = logl (fabsl (x)) / logl (2);
	*e = (int)floorl (l2x);

	/*
	 * Now correct the result and adjust things that might have gotten
	 * off-by-one due to rounding.
	 */
	x /= go_pow2l (*e);
	if (fabsl (x) >= 1.0)
		x /= 2, (*e)++;
	else if (fabsl (x) < 0.5)
		x *= 2, (*e)--;

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

#ifdef GOFFICE_SUPPLIED_MODFL
long double
modfl (long double x, long double *iptr)
{
	if (isnanl (x))
		return *iptr = x;
	else if (go_finitel (x)) {
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

#ifdef GOFFICE_WITH_DECIMAL64

_Decimal64 go_nanD;
_Decimal64 go_pinfD;
_Decimal64 go_ninfD;

int
go_finiteD (_Decimal64 x)
{
	return finiteD (x);
}

_Decimal64
go_pow2D (int n)
{
	if (n >= -1022 && n <= 1023)
		return (_Decimal64)(go_pow2 (n));
	else
		return powD (2.dd, n);
}

_Decimal64
go_pow10D (int n)
{
	return scalbnD (1.dd, n);
}

_Decimal64
go_powD (_Decimal64 x, _Decimal64 y)
{
	return powD (x, y);
}

_Decimal64
go_log10D (_Decimal64 x)
{
	return log10D (x);
}


/**
 * go_strtoDd:
 * @s: string to convert
 * @end: (out) (transfer none) (optional): pointer to end of string.
 *
 * Returns: the numeric value of the given string.
 * Like strtod, but for type _Decimal64 and without hex notation and
 * MS extensions.
 * Unlike strtold, there is no need to reset errno before calling this.
 */
_Decimal64
go_strtoDd (const char *s, char **end)
{
	int maxlen = strtod_helper (s);
	int save_errno;
	char *tmp;
	_Decimal64 res;

	if (maxlen == INT_MAX) {
		errno = 0;
		return strtoDd (s, end);
	} else if (maxlen < 0) {
		errno = 0;
		if (end)
			*end = (char *)s - maxlen;
		return 0;
	}

	tmp = g_strndup (s, maxlen);
	errno = 0;
	res = strtoDd (tmp, end);
	save_errno = errno;
	if (end)
		*end = (char *)s + (*end - tmp);
	g_free (tmp);
	errno = save_errno;

	return res;
}

/**
 * go_ascii_strtoDd:
 * @s: string to convert
 * @end: optional pointer to end of string.
 *
 * Like strtoDd, but applying "C" locale and without hex notation and
 * MS extensions.
 * Unlike strtod, there is no need to reset errno before calling this.
 */
_Decimal64
go_ascii_strtoDd (const char *s, char **end)
{
	GString *tmp;
	const GString *decimal;
	int save_errno;
	char *the_end;
	/* Use the "double" version for parsing.  */
	_Decimal64 res = go_ascii_strtod (s, &the_end);
	if (end)
		*end = the_end;
	if (the_end == s)
		return res;

	decimal = go_locale_get_decimal ();
	tmp = g_string_sized_new (the_end - s + 10);
	while (s < the_end) {
		if (*s == '.') {
			g_string_append_len (tmp, decimal->str, decimal->len);
			g_string_append (tmp, ++s);
			break;
		}
		g_string_append_c (tmp, *s++);
	}
	errno = 0;
	res = strtoDd (tmp->str, NULL);
	save_errno = errno;
	g_string_free (tmp, TRUE);
	errno = save_errno;

	return res;
}

_Decimal64
go_add_epsilonD (_Decimal64 x)
{
	return x == 0 ? x : nextafterD (x, go_pinfD);
}

_Decimal64
go_sub_epsilonD (_Decimal64 x)
{
	return x == 0 ? x : nextafterD (x, go_ninfD);
}

_Decimal64
go_fake_floorD (_Decimal64 x)
{
	if (x == floorD (x))
		return x;

	return floorD (go_add_epsilonD (x));
}

_Decimal64
go_fake_ceilD (_Decimal64 x)
{
	if (x == floorD (x))
		return x;

	return ceilD (go_sub_epsilonD (x));
}

_Decimal64
go_fake_roundD (_Decimal64 x)
{
	_Decimal64 y;

	if (x == floorD (x))
		return x;

	/*
	 * Adding a half here is ok.  The only problematic non-integer
	 * case is nextafter(0.5,-1) for which we want to produce 1 here.
	 */
	y = go_fake_floorD (fabsD (x) + 0.5dd);
	return (x < 0) ? 0 - y : y;
}

_Decimal64
go_fake_truncD (_Decimal64 x)
{
	if (x == floorD (x))
		return x;

	return (x >= 0)
		? floorD (go_add_epsilonD (x))
		: 0 - floorD (go_add_epsilonD (-x));
}

#endif

/* ------------------------------------------------------------------------- */

void
go_continued_fraction (double val, int max_denom, int *res_num, int *res_denom)
{
	int n1, n2, d1, d2;
	double x, y;

	if (val < 0) {
		go_continued_fraction (-val, max_denom, res_num, res_denom);
		*res_num = -*res_num;
		return;
	}

	n1 = 0; d1 = 1;
	n2 = 1; d2 = 0;

	x = val;
	y = 1;

	do {
		double a = floor (x / y);
		double newy = x - a * y;
		int ia, n3, d3;

		if ((n2 && a > (INT_MAX - n1) / n2) ||
		    (d2 && a > (INT_MAX - d1) / d2) ||
		    a * d2 + d1 > max_denom) {
			*res_num = n2;
			*res_denom = d2;
			return;
		}

		ia = (int)a;
		n3 = ia * n2 + n1;
		d3 = ia * d2 + d1;

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

/* ------------------------------------------------------------------------- */

static double
reduce_half (double x, int *pk)
{
	int k = 0;

	if (x < 0) {
		x = -reduce_half (-x, &k);
		k = 4 - k;
		if (x == -0.25)
			x += 0.5, k += 3;
	} else {
		x = fmod (x, 2);
		if (x >= 1)
			x -= 1, k += 2;
		if (x >= 0.5)
			x -= 0.5, k++;
		if (x > 0.25)
			x -= 0.5, k++;
	}

	*pk = (k & 3);
	return x;
}

static double
do_sinpi (double x, int k)
{
	double y;

	if (x == 0)
		y = k & 1;
	else if (x == 0.25)
		y = 0.707106781186547524400844362104849039284835937688474036588339;
	else
		y = (k & 1) ? cos (M_PI * x) : sin (M_PI * x);

	return (k & 2) ? 0 - y : y;
}


/**
 * go_sinpi:
 * @x: a number
 *
 * Returns: the sine of Pi times @x, but with less error than doing the
 * multiplication outright.
 */
double
go_sinpi (double x)
{
	int k;
	double x0 = x;
	x = reduce_half (x, &k);

	/*
	 * Per IEEE 754 2008:
	 * sinpi(n) == 0 with sign of n.
	 */
	if (x == 0 && (k & 1) == 0)
		return copysign (0, x0);

	return do_sinpi (x, k);
}

/**
 * go_cospi:
 * @x: a number
 *
 * Returns: the cosine of Pi times @x, but with less error than doing the
 * multiplication outright.
 */
double
go_cospi (double x)
{
	int k;
	x = reduce_half (x, &k);

	/*
	 * Per IEEE 754 2008:
	 * cospi(n+0.5) == +0 for any integer n.
	 */
	if (x == 0 && (k & 1) == 1)
		return +0.0;

	return do_sinpi (x, k + 1);
}

/**
 * go_tanpi:
 * @x: a number
 *
 * Returns: the tangent of Pi times @x, but with less error than doing the
 * multiplication outright.
 */
double
go_tanpi (double x)
{
	/*
	 * IEEE 754 2008 doesn't have tanpi and thus doesn't define the
	 * behaviour for -0 argument or result.  crlibm has tanpi, but
	 * doesn't seem to be fully clear on these cases.
	 */

	/* inf -> nan; -n -> -0; +n -> +0 */
	x = fmod (x, 1.0);

	if (x == 0)
		return copysign (0.0, x);
	if (fabs (x) == 0.5)
		return copysign (go_nan, x);
	else
		return go_sinpi (x) / go_cospi (x);
}

/**
 * go_cotpi:
 * @x: a number
 *
 * Returns: the cotangent of Pi times @x, but with less error than doing the
 * multiplication outright.
 */
double
go_cotpi (double x)
{
	/*
	 * IEEE 754 2008 doesn't have cotpi.  Neither does crlibm.  Mirror
	 * tanpi here.
	 */

	/* inf -> nan; -n -> -0; +n -> +0 */
	x = fmod (x, 1.0);

	if (x == 0)
		return copysign (go_nan, x);
	if (fabs (x) == 0.5)
		return copysign (0.0, x);
	else
		return go_cospi (x) / go_sinpi (x);
}

/**
 * go_atan2pi:
 * @y: a number
 * @x: a number
 *
 * Returns: the polar angle of the point (@x,@y) in radians divided by Pi.
 * The result is a number between -1 and +1.
 */
double
go_atan2pi (double y, double x)
{
	/*
	 * This works perfectly for results that are (n/4).   We currently have
	 * nothing interesting to add.
	 */
	return atan2 (y, x) / M_PI;
}

/**
 * go_atanpi:
 * @x: a number
 *
 * Returns: the arc tangent of @x in radians divided by Pi.  The result is a
 * number between -1 and +1.
 */
double
go_atanpi (double x)
{
	return x < 0 ? -go_atan2pi (-x, 1) : go_atan2pi (x, 1);
}

#ifdef GOFFICE_WITH_LONG_DOUBLE

#define M_PIgo    3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117L

static long double
reduce_halfl (long double x, int *pk)
{
	int k = 0;

	if (x < 0) {
		x = -reduce_halfl (-x, &k);
		k = 4 - k;
		if (x == -0.25)
			x += 0.5, k += 3;
	} else {
		x = fmod (x, 2);
		if (x >= 1)
			x -= 1, k += 2;
		if (x >= 0.5)
			x -= 0.5, k++;
		if (x > 0.25)
			x -= 0.5, k++;
	}

	*pk = (k & 3);
	return x;
}

static long double
do_sinpil (long double x, int k)
{
	long double y;

	if (x == 0)
		y = k & 1;
	else if (x == 0.25)
		y = 0.707106781186547524400844362104849039284835937688474036588339L;
	else
		y = (k & 1) ? cosl (M_PIgo * x) : sinl (M_PIgo * x);

	return (k & 2) ? 0 - y : y;
}

long double
go_sinpil (long double x)
{
	int k;
	long double x0 = x;
	x = reduce_halfl (x, &k);

	/*
	 * Per IEEE 754 2008:
	 * sinpi(n) == 0 with sign of n.
	 */
	if (x == 0 && (k & 1) == 0)
		return copysign (0, x0);

	return do_sinpil (x, k);
}

long double
go_cospil (long double x)
{
	int k;
	x = reduce_halfl (x, &k);

	/*
	 * Per IEEE 754 2008:
	 * cospi(n+0.5) == +0 for any integer n.
	 */
	if (x == 0 && (k & 1) == 1)
		return +0.0l;

	return do_sinpil (x, k + 1);
}

long double
go_tanpil (long double x)
{
	/*
	 * IEEE 754 2008 doesn't have tanpi and thus doesn't define the
	 * behaviour for -0 argument or result.  crlibm has tanpi, but
	 * doesn't seem to be fully clear on these cases.
	 */

	/* inf -> nan; -n -> -0; +n -> +0 */
	x = fmodl (x, 1.0l);

	if (x == 0)
		return copysignl (0.0l, x);
	if (fabsl (x) == 0.5l)
		return copysignl (go_nanl, x);
	else
		return go_sinpil (x) / go_cospil (x);
}

long double
go_cotpil (long double x)
{
	/*
	 * IEEE 754 2008 doesn't have cotpi.  Neither does crlibm.  Mirror
	 * tanpi here.
	 */

	/* inf -> nan; -n -> -0; +n -> +0 */
	x = fmodl (x, 1.0l);

	if (x == 0)
		return copysignl (go_nanl, x);
	if (fabsl (x) == 0.5)
		return copysignl (0.0l, x);
	else
		return go_cospil (x) / go_sinpil (x);
}

long double
go_atan2pil (long double y, long double x)
{
	/*
	 * This works perfectly for results that are (n/4), at least with x86
	 * long double.   We currently have nothing interesting to add.
	 */
	return atan2l (y, x) / M_PIgo;
}

long double
go_atanpil (long double x)
{
	return x < 0 ? -go_atan2pil (-x, 1) : go_atan2pil (x, 1);
}

#endif

#ifdef GOFFICE_WITH_DECIMAL64

static _Decimal64
reduce_halfD (_Decimal64 x, int *pk)
{
	int k = 0;

	if (x < 0) {
		x = -reduce_halfD (-x, &k);
		k = 4 - k;
		if (x == -0.25dd)
			x += 0.5dd, k += 3;
	} else {
		x = fmod (x, 2);
		if (x >= 1)
			x -= 1, k += 2;
		if (x >= 0.5dd)
			x -= 0.5dd, k++;
		if (x > 0.25dd)
			x -= 0.5dd, k++;
	}

	*pk = (k & 3);
	return x;
}

static _Decimal64
do_sinpiD (_Decimal64 x, int k)
{
	_Decimal64 y;

	if (x == 0)
		y = k & 1;
	else if (x == 0.25dd)
		y = 0.707106781186547524400844362104849039284835937688474036588339dd;
	else
		y = (k & 1) ? cos (M_PID * x) : sin (M_PID * x);

	return (k & 2) ? 0 - y : y;
}

_Decimal64
go_sinpiD (_Decimal64 x)
{
	int k;
	_Decimal64 x0 = x;
	x = reduce_halfD (x, &k);

	/*
	 * Per IEEE 754 2008:
	 * sinpi(n) == 0 with sign of n.
	 */
	if (x == 0 && (k & 1) == 0)
		return copysignD (0, x0);

	return do_sinpiD (x, k);
}

_Decimal64
go_cospiD (_Decimal64 x)
{
	int k;
	x = reduce_halfD (x, &k);

	/*
	 * Per IEEE 754 2008:
	 * cospi(n+0.5) == +0 for any integer n.
	 */
	if (x == 0 && (k & 1) == 1)
		return +0.0;

	return do_sinpiD (x, k + 1);
}

_Decimal64
go_tanpiD (_Decimal64 x)
{
	/*
	 * IEEE 754 2008 doesn't have tanpi and thus doesn't define the
	 * behaviour for -0 argument or result.  crlibm has tanpi, but
	 * doesn't seem to be fully clear on these cases.
	 */

	/* inf -> nan; -n -> -0; +n -> +0 */
	x = fmodD (x, 1.0dd);

	if (x == 0)
		return copysignD (0.0dd, x);
	if (fabsD (x) == 0.5dd)
		return copysignD (go_nanD, x);
	else
		return go_sinpiD (x) / go_cospiD (x);
}

_Decimal64
go_cotpiD (_Decimal64 x)
{
	/*
	 * IEEE 754 2008 doesn't have cotpi.  Neither does crlibm.  Mirror
	 * tanpi here.
	 */

	/* inf -> nan; -n -> -0; +n -> +0 */
	x = fmodD (x, 1.0dd);

	if (x == 0)
		return copysign (go_nanD, x);
	if (fabsD (x) == 0.5dd)
		return copysignD (0.0dd, x);
	else
		return go_cospiD (x) / go_sinpiD (x);
}

_Decimal64
go_atan2piD (_Decimal64 y, _Decimal64 x)
{
	return atan2D (y, x) / M_PID;
}

_Decimal64
go_atanpiD (_Decimal64 x)
{
	return x < 0 ? -go_atan2piD (-x, 1) : go_atan2piD (x, 1);
}

#endif


/* ------------------------------------------------------------------------- */
