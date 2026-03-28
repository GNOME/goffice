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

// We need multiple versions of this code.  We're going to include ourself
// with different settings of various macros.  gdb will hate us.
#include <goffice/goffice-multipass.h>
#ifndef SKIP_THIS_PASS

/* ------------------------------------------------------------------------- */

DOUBLE SUFFIX(go_nan);
DOUBLE SUFFIX(go_pinf);
DOUBLE SUFFIX(go_ninf);

#ifdef DEFINE_COMMON
static gboolean
running_under_buggy_valgrind (void)
{
#ifdef HAVE_LONG_DOUBLE
	volatile long double one = atof ("1");
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

#ifdef GOFFICE_WITH_LONG_DOUBLE
	if (!(go_pinfl > DBL_MAX && !isnanl (go_pinfl) && isnanl (go_pinfl - go_pinfl)))
		return FALSE;

	if (!(-go_ninfl > DBL_MAX && !isnanl (go_ninfl) && isnanl (go_ninfl - go_ninfl)))
		return FALSE;

	if (!isnanl (go_nanl) && !(go_nanl >= 0) && !(go_nanl <= 0))
		return FALSE;
#endif

	/* finitel must be hosed.  Blame valgrind.  */
	return TRUE;
#else
	// No long double, so no issue
	return FALSE;
#endif
}

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
#endif

/* ------------------------------------------------------------------------- */

/**
 * go_add_epsilon:
 * @x: a number
 *
 * Returns: the next-larger representable value, except that zero and
 * infinites are returned unchanged.
 */
DOUBLE
SUFFIX(go_add_epsilon) (DOUBLE x)
{
	if (!SUFFIX(go_finite) (x) || x == 0)
		return x;
#ifdef HAVE_NEXTAFTER
	return SUFFIX(nextafter) (x, SUFFIX(go_pinf));
#else
	if (x < 0)
		return 0 - SUFFIX(go_sub_epsilon)(-x);
	else {
		int e;
		DOUBLE mant = SUFFIX(frexp) (SUFFIX(fabs) (x), &e);
		// mant is in range [0.5; 1)
		return SUFFIX(ldexp) (mant + DOUBLE_EPSILON / 2, e);
	}
#endif
}

/**
 * go_sub_epsilon:
 * @x: a number
 *
 * Returns: the next-smaller representable value, except that zero and
 * infinites are returned unchanged.
 */
DOUBLE
SUFFIX(go_sub_epsilon) (DOUBLE x)
{
	if (!SUFFIX(go_finite) (x) || x == 0)
		return x;
#ifdef HAVE_NEXTAFTER
	return SUFFIX(nextafter) (x, SUFFIX(go_ninf));
#else
	if (x < 0)
		return 0 - SUFFIX(go_add_epsilon)(-x);
	else {
		int e;
		DOUBLE mant = SUFFIX(frexp) (SUFFIX(fabs) (x), &e);
		// mant is in range [0.5; 1)
		// FIXME: what about base 10?
		return SUFFIX(ldexp) (mant - (mant == 0.5 ? DOUBLE_EPSILON / 4 : DOUBLE_EPSILON / 2), e);
	}
#endif
}

static DOUBLE
SUFFIX(go_d2d) (DOUBLE x)
{
	/* Kill excess precision by forcing a memory read.  */
	const volatile DOUBLE *px = &x;
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
DOUBLE
SUFFIX(go_fake_floor) (DOUBLE x)
{
	x = SUFFIX(go_d2d) (x);

	if (x == SUFFIX(floor) (x))
		return x;

	return SUFFIX(floor) (SUFFIX(go_add_epsilon) (x));
}

/**
 * go_fake_ceil:
 * @x: value to ceil
 *
 * Returns: the ceiling of @x, ie., the smallest integer that is not smaller
 * than @x.  However, this variant applies a 1 ulp grace interval for
 * values that are just a hair larger than an integer.
 */
DOUBLE
SUFFIX(go_fake_ceil) (DOUBLE x)
{
	x = SUFFIX(go_d2d) (x);

	if (x == SUFFIX(floor) (x))
		return x;

	return SUFFIX(ceil) (SUFFIX(go_sub_epsilon) (x));
}

/**
 * go_fake_round:
 * @x: value to round
 *
 * Returns: @x rounded of to nearest integer.  However, this variant applies
 * a 1 ulp grace interval for values that are just a hair less than a
 * helf-integer.
 */
DOUBLE
SUFFIX(go_fake_round) (DOUBLE x)
{
	DOUBLE y;

	x = SUFFIX(go_d2d) (x);

	if (x == SUFFIX(floor) (x))
		return x;

	/*
	 * Adding a half here is ok.  The only problematic non-integer
	 * case is nextafter(0.5,-1) for which we want to produce 1 here.
	 */
	y = SUFFIX(go_fake_floor) (SUFFIX(fabs) (x) + CONST(0.5));
	return (x < 0) ? -y : y;
}

/**
 * go_fake_trunc:
 * @x: value to truncate
 *
 * Returns: @x rounded of to nearest integer in the direction of zero.
 * However, this variant applies a 1 ulp grace interval for values that
 * are just a hair less than away from an integer.
 */
DOUBLE
SUFFIX(go_fake_trunc) (DOUBLE x)
{
	x = SUFFIX(go_d2d) (x);

	if (x == SUFFIX(floor) (x))
		return x;

	return (x >= 0)
		? SUFFIX(floor) (SUFFIX(go_add_epsilon) (x))
		: -SUFFIX(floor) (SUFFIX(go_add_epsilon) (-x));
}

DOUBLE
SUFFIX(go_rint) (DOUBLE x)
{
#if INCLUDE_PASS != INCLUDE_PASS_DECIMAL64 && defined(HAVE_RINT)
	return SUFFIX(rint) (x);
#else
	DOUBLE y = SUFFIX(floor) (SUFFIX(fabs) (x) + CONST(0.5));
	return (x < 0) ? -y : y;
#endif
}

/* ------------------------------------------------------------------------- */

int
SUFFIX(go_finite) (DOUBLE x)
{
	/* What a circus!  */
#ifdef HAVE_FINITE
	return SUFFIX(finite) (x);
#elif defined(HAVE_ISFINITE)
	return SUFFIX(isfinite) (x);
#elif defined(FINITE)
	return FINITE (x);
#else
	x = SUFFIX(fabs) (x);
	return x < SUFFIX(go_pinf);
#endif
}

/* ------------------------------------------------------------------------- */

#if INCLUDE_PASS == INCLUDE_PASS_DOUBLE
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

#elif INCLUDE_PASS == INCLUDE_PASS_LONG_DOUBLE

long double
go_pow2l (int n)
{
#ifdef GOFFICE_SUPPLIED_LDEXPL
	return powl (2.0L, n);
#else
	return ldexpl (1.0L, n);
#endif
}

#elif INCLUDE_PASS == INCLUDE_PASS_DECIMAL64

_Decimal64
go_pow2D (int n)
{
	if (n >= -1022 && n <= 1023)
		return (_Decimal64)(go_pow2 (n));
	else if (n > 0)
		return (_Decimal64)(go_pow2 (n - 1023)) * go_pow2D (1023);
	else
		return (_Decimal64)(go_pow2 (n + 1022)) * go_pow2D (-1022);
}

#endif

/* ------------------------------------------------------------------------- */

#define TEN(n) ONE(n ## 0),ONE(n ## 1),ONE(n ## 2),ONE(n ## 3),ONE(n ## 4),ONE(n ## 5),ONE(n ## 6),ONE(n ## 7),ONE(n ## 8),ONE(n ## 9)
#define HUNDRED(n) TEN(n ## 0),TEN(n ## 1),TEN(n ## 2),TEN(n ## 3),TEN(n ## 4),TEN(n ## 5),TEN(n ## 6),TEN(n ## 7),TEN(n ## 8),TEN(n ## 9)

#if INCLUDE_PASS == INCLUDE_PASS_DOUBLE
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

#elif INCLUDE_PASS == INCLUDE_PASS_LONG_DOUBLE

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

#elif INCLUDE_PASS == INCLUDE_PASS_DECIMAL64

_Decimal64
go_pow10D (int n)
{
	return scalbnD (1.dd, n);
}

#endif

#undef TEN
#undef HUNDRED

/* ------------------------------------------------------------------------- */

/**
 * go_pow:
 * @x: base
 * @y: exponent
 *
 * Like pow, but with extra effort for @x==10.
 *
 * Returns: @x^@y.
 */
DOUBLE
SUFFIX(go_pow) (DOUBLE x, DOUBLE y)
{
	if (x == 10 && y > G_MININT && y < G_MAXINT && y == SUFFIX(floor) (y))
		return SUFFIX(go_pow10) ((int)y);
	return SUFFIX(pow) (x, y);
}

/* ------------------------------------------------------------------------- */

#ifdef DEFINE_COMMON
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
#endif


/**
 * go_strtod:
 * @s: string to convert
 * @end: (out) (transfer none) (optional): pointer to end of string.
 *
 * Like strtod, but without hex notation and MS extensions.
 * Unlike strtod, there is no need to reset errno before calling this.
 *
 * Returns: the numeric value of the given string.
 */

/**
 * go_strtold:
 * @s: string to convert
 * @end: (out) (transfer none) (optional): pointer to end of string.
 *
 * Like strtod, but without hex notation and MS extensions.
 * Unlike strtold, there is no need to reset errno before calling this.
 *
 * Returns: the numeric value of the given string.
 */

/**
 * go_strtoDd:
 * @s: string to convert
 * @end: (out) (transfer none) (optional): pointer to end of string.
 *
 * Like strtod, but without hex notation and MS extensions.
 * Unlike strtod, there is no need to reset errno before calling this.
 *
 * Returns: the numeric value of the given string.
 */

DOUBLE
STRTO (const char *s, char **end)
{
	int maxlen = strtod_helper (s);
	int save_errno;
	char *tmp;
	DOUBLE res;

	if (maxlen == INT_MAX) {
		errno = 0;
		// libc strtod
		return PLAINSTRTO (s, end);
	} else if (maxlen < 0) {
		/* Hex */
		errno = 0;
		if (end)
			*end = (char *)s - maxlen;
		return 0;
	}

	tmp = g_strndup (s, maxlen);
	errno = 0;
	res = PLAINSTRTO (tmp, end);
	save_errno = errno;
	if (end)
		*end = (char *)s + (*end - tmp);
	g_free (tmp);
	errno = save_errno;

	return res;
}

#if INCLUDE_PASS == INCLUDE_PASS_DOUBLE
/**
 * go_ascii_strtod:
 * @s: string to convert
 * @end: optional pointer to end of string.
 *
 * Like g_ascii_strtod, but without hex notation and MS extensions.
 * There is no need to reset errno before calling this.
 *
 * Returns: the converted value.
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

#else

/**
 * go_ascii_strtold:
 * @s: string to convert
 * @end: optional pointer to end of string.
 *
 * Like strtold, but without hex notation and MS extensions and also
 * assuming "C" locale.
 * Unlike strtold, there is no need to reset errno before calling this.
 *
 * Returns: the converted value.
 */

/**
 * go_ascii_strtoDd:
 * @s: string to convert
 * @end: optional pointer to end of string.
 *
 * Like strtoDd, but without hex notation and MS extensions and also
 * assuming "C" locale.  There is no need to reset errno before
 * calling this.
 *
 * Returns: the converted value.
 */

DOUBLE
ASCIISTRTO (const char *s, char **end)
{
	GString *tmp;
	const GString *decimal = go_locale_get_decimal ();
	int save_errno;
	char *the_end;

	// Fast-track
	if (FALSE && strcmp (decimal->str, ".") == 0)
		return STRTO (s, end);

	/* Use the "double" version for parsing.  */
	(void)go_ascii_strtod (s, &the_end);

	if (end)
		*end = the_end;
	if (the_end == s)
		return 0;

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
	DOUBLE res = STRTO (tmp->str, NULL);
	save_errno = errno;
	g_string_free (tmp, TRUE);
	errno = save_errno;

	return res;
}

#endif

/* ------------------------------------------------------------------------- */


#ifdef DEFINE_COMMON

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

#ifdef GOFFICE_WITH_LONG_DOUBLE
#ifndef HAVE_STRTOLD
long double
strtold (char const *str, char **end)
{
	char *myend;
	long double res;
	int count;
	int n = 0;

	if (end == 0) end = &myend;
	(void) strtod (str, end);
	if (str == *end)
		return 0.0L;

	errno = 0;
	count = sscanf (str, "%Lf%n", &res, &n);
	*end = (char *)str + n;
	if (count == 1)
		return res;

	errno = ERANGE;
	return 0.0;
}
#endif
#endif

#ifdef GOFFICE_SUPPLIED_MODFL
long double
modfl (long double x, long double *iptr)
{
	*iptr = copysignl (floorl (fabsl (x)), x);
	if (go_finitel (x))
		return copysignl (x - *iptr, x);
	else if (isnanl (x))
		return *iptr;
	else
		return copysignl (0.0L, x);
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

#endif  // DEFINE_COMMON

/* ------------------------------------------------------------------------- */

static DOUBLE
SUFFIX(reduce_half) (DOUBLE x, int *pk)
{
	int k = 0;
	DOUBLE HALF = CONST(0.5);

	if (x < 0) {
		x = -reduce_half (-x, &k);
		k = 4 - k;
		if (x == CONST(-0.25))
			x += HALF, k += 3;
	} else {
		x = fmod (x, 2);
		if (x >= 1)
			x -= 1, k += 2;
		if (x >= HALF)
			x -= HALF, k++;
		if (x > CONST(0.25))
			x -= HALF, k++;
	}

	*pk = (k & 3);
	return x;
}

static DOUBLE
SUFFIX(do_sinpi) (DOUBLE x, int k)
{
	DOUBLE y;

	if (x == 0)
		y = k & 1;
	else if (x == CONST(0.25))
		y = CONST(0.707106781186547524400844362104849039284835937688474036588339);
	else
		y = (k & 1) ? SUFFIX(cos) (DOUBLE_PI * x) : SUFFIX(sin) (DOUBLE_PI * x);

	return (k & 2) ? 0 - y : y;
}


/**
 * go_sinpi:
 * @x: a number
 *
 * Returns: the sine of Pi times @x, but with less error than doing the
 * multiplication outright.
 */
DOUBLE
SUFFIX(go_sinpi) (DOUBLE x)
{
	int k;
	DOUBLE x0 = x;
	x = SUFFIX(reduce_half) (x, &k);

	/*
	 * Per IEEE 754 2008:
	 * sinpi(n) == 0 with sign of n.
	 */
	if (x == 0 && (k & 1) == 0)
		return SUFFIX(copysign) (0, x0);

	return SUFFIX(do_sinpi) (x, k);
}

/**
 * go_cospi:
 * @x: a number
 *
 * Returns: the cosine of Pi times @x, but with less error than doing the
 * multiplication outright.
 */
DOUBLE
SUFFIX(go_cospi) (DOUBLE x)
{
	int k;
	x = SUFFIX(reduce_half) (x, &k);

	/*
	 * Per IEEE 754 2008:
	 * cospi(n+0.5) == +0 for any integer n.
	 */
	if (x == 0 && (k & 1) == 1)
		return +0.0;

	return SUFFIX(do_sinpi) (x, k + 1);
}

/**
 * go_tanpi:
 * @x: a number
 *
 * Returns: the tangent of Pi times @x, but with less error than doing the
 * multiplication outright.
 */
DOUBLE
SUFFIX(go_tanpi) (DOUBLE x)
{
	/*
	 * IEEE 754 2008 doesn't have tanpi and thus doesn't define the
	 * behaviour for -0 argument or result.  crlibm has tanpi, but
	 * doesn't seem to be fully clear on these cases.
	 */

	/* inf -> nan; -n -> -0; +n -> +0 */
	x = SUFFIX(fmod) (x, 1.0);

	if (x == 0)
		return SUFFIX(copysign) (0.0, x);
	if (SUFFIX(fabs) (x) == CONST(0.5))
		return SUFFIX(copysign) (SUFFIX(go_nan), x);
	else
		return SUFFIX(go_sinpi) (x) / SUFFIX(go_cospi) (x);
}

/**
 * go_cotpi:
 * @x: a number
 *
 * Returns: the cotangent of Pi times @x, but with less error than doing the
 * multiplication outright.
 */
DOUBLE
SUFFIX(go_cotpi) (DOUBLE x)
{
	/*
	 * IEEE 754 2008 doesn't have cotpi.  Neither does crlibm.  Mirror
	 * tanpi here.
	 */

	/* inf -> nan; -n -> -0; +n -> +0 */
	x = SUFFIX(fmod) (x, 1.0);

	if (x == 0)
		return SUFFIX(copysign) (SUFFIX(go_nan), x);
	if (SUFFIX(fabs) (x) == CONST(0.5))
		return SUFFIX(copysign) (0.0, x);
	else
		return SUFFIX(go_cospi) (x) / SUFFIX(go_sinpi) (x);
}

/**
 * go_atan2pi:
 * @y: a number
 * @x: a number
 *
 * Returns: the polar angle of the point (@x,@y) in radians divided by Pi.
 * The result is a number between -1 and +1.
 */
DOUBLE
SUFFIX(go_atan2pi) (DOUBLE y, DOUBLE x)
{
	/*
	 * This works perfectly for results that are (n/4).   We currently have
	 * nothing interesting to add.
	 */
	return SUFFIX(atan2) (y, x) / DOUBLE_PI;
}

/**
 * go_atanpi:
 * @x: a number
 *
 * Returns: the arc tangent of @x in radians divided by Pi.  The result is a
 * number between -1 and +1.
 */
DOUBLE
SUFFIX(go_atanpi) (DOUBLE x)
{
	return x < 0 ? -SUFFIX(go_atan2pi) (-x, 1) : SUFFIX(go_atan2pi) (x, 1);
}

/* ------------------------------------------------------------------------- */


#if DOUBLE_RADIX == 2

static DOUBLE
SUFFIX(reduce_pi_simple) (DOUBLE x, int *pk, int kbits)
{
	static const DOUBLE two_over_pi = CONST(0.63661977236758134307553505349);
	static const DOUBLE pi_over_two[] = {
		+0x1.921fb5p+0,
		+0x1.110b46p-26,
		+0x1.1a6263p-54,
		+0x1.8a2e03p-81,
		+0x1.c1cd12p-107
	};
	int i;
	DOUBLE k = SUFFIX(round) (x * SUFFIX(ldexp) (two_over_pi, kbits - 2));
	DOUBLE xx = 0;

	g_assert (k < (1 << 26));
	*pk = (int)k;

	if (k == 0)
		return x;

	x -= k * SUFFIX(ldexp) (pi_over_two[0], 2 - kbits);
	for (i = 1; i < 5; i++) {
		DOUBLE dx = k * SUFFIX(ldexp) (pi_over_two[i], 2 - kbits);
		DOUBLE s = x - dx;
		xx += x - s - dx;
		x = s;
	}

	return x + xx;
}

#ifdef DEFINE_COMMON
/*
 * Add the 64-bit number p at *dst and backwards.  This would be very
 * simple and fast in assembler.  In C it's a bit of a mess.
 */
static inline void
add_at (guint32 *dst, guint64 p)
{
	unsigned h = p >> 63;

	p += *dst;
	*dst-- = p & 0xffffffffu;
	p >>= 32;
	if (p) {
		p += *dst;
		*dst-- = p & 0xffffffffu;
		if (p >> 32)
			while (++*dst == 0)
				dst--;
	} else if (h) {
		/* p overflowed, pass carry on. */
		dst--;
		while (++*dst == 0)
			dst--;
	}
}

// A mildly insane number of bits that ought to be good for reduction of
// values up to 2^16k
static const guint32 one_over_two_pi[] = {
	0x28be60dbu, 0x9391054au, 0x7f09d5f4u, 0x7d4d3770u,
	0x36d8a566u, 0x4f10e410u, 0x7f9458eau, 0xf7aef158u,
	0x6dc91b8eu, 0x909374b8u, 0x01924bbau, 0x82746487u,
	0x3f877ac7u, 0x2c4a69cfu, 0xba208d7du, 0x4baed121u,
	0x3a671c09u, 0xad17df90u, 0x4e64758eu, 0x60d4ce7du,
	0x272117e2u, 0xef7e4a0eu, 0xc7fe25ffu, 0xf7816603u,
	0xfbcbc462u, 0xd6829b47u, 0xdb4d9fb3u, 0xc9f2c26du,
	0xd3d18fd9u, 0xa797fa8bu, 0x5d49eeb1u, 0xfaf97c5eu,
	0xcf41ce7du, 0xe294a4bau, 0x9afed7ecu, 0x47e35742u,
	0x1580cc11u, 0xbf1edaeau, 0xfc33ef08u, 0x26bd0d87u,
	0x6a78e458u, 0x57b986c2u, 0x19666157u, 0xc5281a10u,
	0x237ff620u, 0x135cc9ccu, 0x41818555u, 0xb29cea32u,
	0x58389ef0u, 0x231ad1f1u, 0x0670d9f3u, 0x773a024au,
	0xa0d6711du, 0xa2e58729u, 0xb76bd134u, 0x55c6414fu,
	0xa97fc1c1u, 0x4fdf8cfau, 0x0cb0b793u, 0xe60c9f6eu,
	0xf0cf49bbu, 0xdac797beu, 0x27ce87cdu, 0x72bc9fc7u,
	0x61fc4864u, 0x1f1f091au, 0xbe9bb55du, 0xcb4c10ceu,
	0xc571852du, 0x674670f0u, 0xb12b5053u, 0x4b174003u,
	0x119f618bu, 0x5c78e6b1u, 0xa6c0188cu, 0xdf34ad25u,
	0xe9ed3555u, 0x4dfd8fb5u, 0xc60428ffu, 0x1d934aa7u,
	0x592af5dcu, 0x3e1f18d5u, 0xec1eb9c5u, 0x45d59270u,
	0x36758eceu, 0x2129f2c8u, 0xc91de2b5u, 0x88d516aeu,
	0x47c006c2u, 0xbc77f386u, 0x7fcc67dau, 0x87999855u,
	0xe651feebu, 0x361fdfadu, 0xd948a27au, 0x0c982ff9u,
	0xb3713bc2u, 0x4d9b350fu, 0xd775f785u, 0xb78ed624u,
	0xa6f78a08u, 0xb4ba218au, 0x1356388cu, 0xb2b185b8u,
	0xc232df78u, 0x143005e9u, 0xc77cd6f8u, 0x060d04cbu,
	0x9884a0c0u, 0x5220d6e3u, 0xbd5fec2bu, 0x7cba4790u,
	0xd29234d9u, 0xc436376au, 0x9097ebb3u, 0x985aa90au,
	0x02ad2674u, 0xfca9819fu, 0xddd720f0u, 0xa8e20f18u,
	0x5e1ce296u, 0xa32bef75u, 0xdbd8e98bu, 0x72effd3bu,
	0xe06359f0u, 0x49917295u, 0x4db672b4u, 0xaa0a2358u,
	0x709df244u, 0x85098126u, 0xd184b116u, 0x71113172u,
	0x246c937cu, 0xc5c02b50u, 0xf539524au, 0x44357f7fu,
	0x2f803325u, 0x07bbb39cu, 0x3d4f84e0u, 0x3c7b30f9u,
	0xecca3e31u, 0xe50164cfu, 0x9c706cc2u, 0x4bbcd142u,
	0xe704a21eu, 0xc82ae7edu, 0x4bb0a491u, 0xcbcc9edbu,
	0x55432429u, 0xdc87f9dau, 0xe5b2cc52u, 0x859e789eu,
	0x506277fdu, 0x25e53a21u, 0x39b8a5ccu, 0x665afb62u,
	0x0d97d7c3u, 0xbf6eed26u, 0x921b2919u, 0xd09c9c4cu,
	0x97636e05u, 0x67c2796fu, 0x094c634eu, 0x5d3dc701u,
	0x4c004303u, 0x5a0212d6u, 0x3b8b242au, 0x91c0b9ddu,
	0x0935af69u, 0x9f7ddc92u, 0x1bbbc5a7u, 0xe9a523bdu,
	0xa46d1454u, 0xf47c82b3u, 0xcce6081fu, 0x92fd5a18u,
	0xec97cfb7u, 0x40d7501fu, 0xe2614a54u, 0x9570190du,
	0xc4361b4cu, 0x920c9d53u, 0x16f51c53u, 0x9b951170u,
	0x4242da7du, 0x4ab55985u, 0x2741c9d4u, 0x011776ceu,
	0xed315dbau, 0x85fe61dfu, 0x5ad26e89u, 0xc74a5a65u,
	0xab333195u, 0x052b5ab8u, 0xa4227662u, 0x141c8b2fu,
	0xa9012501u, 0xdddc0c3cu, 0xc9ff002au, 0x1c7a9270u,
	0x998f7819u, 0x20f765e5u, 0xcfe8ff65u, 0x10e32183u,
	0x77904c67u, 0x4e64a31cu, 0x3779edc5u, 0xcef7c20au,
	0xcdc56820u, 0x1724e016u, 0xa4844436u, 0x3a03ebe0u,
	0x1b12fff6u, 0xc3e40e1du, 0x86164569u, 0x58aef2d8u,
	0x6e6271efu, 0x5004013cu, 0xb489dd52u, 0x7dadbaeeu,
	0xc8b6ea85u, 0x028bc9a2u, 0x5da0d90cu, 0xcec246a5u,
	0x03aa8e94u, 0x70a8c76bu, 0xbb6bc489u, 0x9713709bu,
	0x671e8b65u, 0xd5b020cfu, 0xc0fdbc02u, 0x63100ae6u,
	0x4c5b41edu, 0x0e454803u, 0x16f0f631u, 0x24bd52ebu,
	0x71a97293u, 0xb34de9cdu, 0xaa79a524u, 0xaada10b7u,
	0x7798c67bu, 0xe31d94a2u, 0xda0df6ffu, 0x2ae86b8cu,
	0x4577e86bu, 0x8036bec3u, 0x1993592du, 0xc17b4c19u,
	0x4a6fd595u, 0xcebfd1eeu, 0x7e5abcefu, 0x9d77e4cau,
	0x0c202afdu, 0xa3198572u, 0xc10188beu, 0x87793669u,
	0x2ccf63c6u, 0xd5c2734du, 0xba5093a9u, 0x2f84ed48u,
	0xccc6aabcu, 0x2a1953e9u, 0x707483cfu, 0xc2f35e16u,
	0xddbe48c1u, 0x22dedc85u, 0xe254e9b1u, 0xb89b9bc0u,
	0x3afbd612u, 0xa6edf6b1u, 0x2e99aab3u, 0xf3dd8740u,
	0xb44b7c6cu, 0x7066631du, 0xeb70f692u, 0x21a8177du,
	0xfd20318bu, 0xfc2b26bbu, 0x376f170fu, 0xdb77b407u,
	0xf1e42db6u, 0xca8e8968u, 0xe6abc024u, 0xd4eb4115u,
	0xedad0b4au, 0x5fa012e9u, 0xc1f683aau, 0x9da8565eu,
	0xca84858bu, 0x6df73f79u, 0x7ebfb6e2u, 0x7f6fa25bu,
	0x1db93f2au, 0x419c200fu, 0x855ba17fu, 0xe1ff41cfu,
	0x8a0cd9d8u, 0x61860abau, 0xaf536bf9u, 0xecdb9b63u,
	0xce59e556u, 0xefcc5235u, 0xe105b7ccu, 0x10cb71cdu,
	0x5849739cu, 0x326e32ccu, 0x3f5b2fe8u, 0x8029391bu,
	0x01683756u, 0x91dbc874u, 0x8498a117u, 0x2e52585cu,
	0x38159ac0u, 0x54a64dd5u, 0x542df547u, 0xb13c4cd7u,
	0xdb84f90cu, 0x176a4ba1u, 0x70ec874du, 0x8ca8692du,
	0xc2352c7au, 0x887dc5b9u, 0x1a63ddffu, 0xc9e000c3u,
	0x0b502368u, 0x3353e669u, 0x4834e8acu, 0xc2974bd0u,
	0xbe6d32f6u, 0x84742f9fu, 0x7076e6efu, 0x45eae068u,
	0xb2971a82u, 0x05d54b95u, 0x4009fc05u, 0x1fe181f8u,
	0x5902c523u, 0x5065b7afu, 0xa1cabf76u, 0xad895acdu,
	0x225effbcu, 0xc167afeeu, 0x53da9a2au, 0x0a9296b1u,
	0x13ef3e0bu, 0x6616b5e5u, 0x71fd2353u, 0x43698e88u,
	0x17d5e92cu, 0x4fc5254eu, 0x20004833u, 0x21b75c6du,
	0xb7b27d58u, 0x2fc45953u, 0x5ac1c06bu, 0x2c233430u,
	0x2c921554u, 0x43bec7b0u, 0xdca54ec1u, 0xa8cd5030u,
	0x1ef701b3u, 0x11783e8au, 0x53b232b5u, 0x907cfa37u,
	0x991f3619u, 0x26cc6fb6u, 0x70e5e935u, 0x161df178u,
	0xda44f6bcu, 0x0f0eae91u, 0x861197ddu, 0x557d6f74u,
	0xb1a49b97u, 0x4bab3b51u, 0x03908f87u, 0x21f1187au,
	0x7f4a7cf5u, 0xb9f29f08u, 0x8d645bf1u, 0x78022375u,
	0xfff89a9bu, 0xb1bf6c30u, 0x4224dd17u, 0x5f2cab5au,
	0xe75bb35eu, 0xdc8f9a84u, 0x71aa73fdu, 0xf7dcca6eu,
	0xb26d5440u, 0x2dc36cb8u, 0x892e9d18u, 0x1f7962b6u,
	0x1d0b0543u, 0x43062065u, 0x199f858au, 0x405d9ea7u,
	0xefbf7f7bu, 0xd1558d9fu, 0xb644f67bu, 0x2e6ea2ffu,
	0x25f109eau, 0x0c70dbbcu, 0x4db16515u, 0xaa362d6au,
	0x2d03b333u, 0xcb62448du, 0x15dbe255u, 0x8b38f3a6u,
	0x6e4835aau, 0x979ae70au, 0x8fb317c4u, 0x5282ff7eu,
	0xfd385b4eu, 0xe38b21b8u, 0xa1353a6au, 0x6d3f347bu,
	0xbbf24d4bu, 0x984e4bd1u, 0x084e3236u, 0x46c2bf20u,
	0x5a92bef6u, 0x070be12du, 0x14e32653u, 0xb3089537u,
	0x154ab5b1u, 0xb0258642u, 0xee1c0699u, 0x255a5816u,
	0x89bb948fu, 0xc3c45fc4u, 0x6d7d3d72u, 0xff0b6f0du,
	0x3baf0d33u, 0x177a1817u, 0xb766e399u, 0xfbcce4aeu,
	0x05f266d6u, 0x186f15f8u, 0x71a0d444u, 0x0fb6121cu,
	0x7777470bu, 0x68462bd1u, 0x8b0875fcu, 0xd6661eb6u,
	0x701527beu, 0xa193ff01u, 0x95ab9e79u, 0x4d88a248u,
	0xab4e3724u, 0xd9eaba15u, 0x4e09a0a6u, 0xf9f2a903u,
	0x546c4ce6u, 0x43b5ea52u, 0x015a7c2cu, 0x9969e21fu,
	0xe5d3220du, 0xb47e6ce4u, 0x8852a09eu, 0xc873e637u,
	0x27d01551u, 0xf70e9d38u, 0x50bad9f7u, 0xe77f97f5u,
	0x17a919deu, 0xdeab2ea8u, 0xbd9548e2u, 0x0ad56e90u,
	0x421b9661u, 0x8a8860d1u, 0xce79b8e2u, 0x7527b950u,
	0x3ed27a55u, 0xbff283c7u, 0x2296714au, 0xfea53170u,
	0x74f3f143u, 0xeb96b6e1u, 0xb151d890u, 0xe14ee188u,
	0x651e4b21u, 0xd8441ed3u, 0x0a868b20u, 0x04afd0e4u,
	0x09a2224fu, 0x1e39312au, 0x1ef6f970u, 0x8eb13abdu,
	0x09a299fdu, 0xefe4834au, 0xe8d96c64u, 0xcf42df2fu,
	0x77146918u, 0xf749f778u, 0x5a466526u, 0xa54a6a0au,
	0x339a2d3bu, 0x424827d1u, 0x32a61398u, 0xe09c08dfu,
	0x1f8cae43u, 0xe3bd69f9u, 0xd585023cu, 0x484aa76du,
	0x535f9bd4u, 0x46696afeu, 0x6d75b7e0u, 0x98776580u,
	0x8d85a7ceu, 0xb12868a0u, 0xdb7b5c9eu, 0xa34e6a6eu,
	0x20970c9au, 0xd6c9d1bbu, 0x4d001dc0u, 0x34957d3fu,
	0x13564060u, 0x1c78384fu, 0xe26ca57cu, 0xd92a3c6bu,
	0xa9d2ce3fu, 0x133aacaeu, 0xd1c9c2eau, 0xf0e9cd2eu,
	0x9814b74du, 0x3e158ebau, 0xdfa28c6eu, 0xcd96256du,
	0xd1fdea65u, 0x30eab4e4u, 0x89479fcfu, 0xb625d3aeu,
	0xea53f62bu, 0x8079986du, 0x0e4f63a9u, 0x48ea8cc1u,
	0xa3858cedu, 0x4eec6207u, 0x4ea75004u, 0xf43306f9u,
	0x7e18b9ceu, 0xfca3ce6du, 0x6fc2f08du, 0x489d1fa8u,
	0x91f0354bu, 0x47c66b74u, 0xe42537e4u, 0xc4742d0au,
	0xc9525b6cu, 0xb8992c97u, 0xbc4d4ef1u, 0xa90692b4u,
	0x2ab24b99u, 0x3a2195ccu, 0x24660b3eu, 0xcc46c682u,
	0x1ca2ef73u, 0xb8583850u, 0xbad90774u, 0x2ee8f956u,
	0x75165ee3u, 0x0a9120fcu, 0xccaebe12u, 0x19cb2346u,
	0xcbea6c14u, 0x3cf77e7du, 0x5cf6d86du, 0x3f88cf9bu,
	0x1069bba8u, 0xc61dd689u, 0xaf179733u, 0xa9c22537u,
	0x15f88065u, 0xbc7a0e6fu, 0x9214574bu
};
static const guint32 pi_over_four[] = {
	0xc90fdaa2u,
	0x2168c234u,
	0xc4c6628bu,
	0x80dc1cd1u
};


#endif


static DOUBLE
SUFFIX(reduce_pi_full) (DOUBLE x, int *pk, int kbits)
{
	DOUBLE m;
	guint32 w2, w1, w0, wm1, wm2;
	int e, neg;
	unsigned di, i, j;
	guint32 r[6], r4[4];
	DOUBLE rh, rl, l48, h48;

	m = SUFFIX(frexp) (x, &e);
	if (e >= DOUBLE_MANT_DIG) {
		di = ((unsigned)e - DOUBLE_MANT_DIG) / 32u;
		e -= di * 32;
	} else
		di = 0;
	m = SUFFIX(ldexp) (m, e - 64);
	w2  = (guint32)SUFFIX(floor) (m); m = SUFFIX(ldexp) (m - w2, 32);
	w1  = (guint32)SUFFIX(floor) (m); m = SUFFIX(ldexp) (m - w1, 32);
	w0  = (guint32)SUFFIX(floor) (m); m = SUFFIX(ldexp) (m - w0, 32);
	wm1 = (guint32)SUFFIX(floor) (m); m = SUFFIX(ldexp) (m - wm1, 32);
	wm2 = (guint32)SUFFIX(floor) (m);

	/*
	 * r[0] is an integer overflow area to be ignored.  We will not create
	 * any carry into r[-1] because 5/(2pi) < 1.
	 */
	r[0] = 0;

	for (i = 0; i < 5; i++) {
		g_assert (i + 2 + di < G_N_ELEMENTS (one_over_two_pi));
		r[i + 1] = 0;
		if (wm2 && i > 1)
			add_at (&r[i + 1], (guint64)wm2 * one_over_two_pi[i - 2]);
		if (wm1 && i > 0)
			add_at (&r[i + 1], (guint64)wm1 * one_over_two_pi[i - 1]);
		if (w0)
			add_at (&r[i + 1], (guint64)w0  * one_over_two_pi[i     + di]);
		if (w1)
			add_at (&r[i + 1], (guint64)w1  * one_over_two_pi[i + 1 + di]);
		if (w2)
			add_at (&r[i + 1], (guint64)w2  * one_over_two_pi[i + 2 + di]);

#if INCLUDE_PASS == INCLUDE_PASS_DOUBLE
		/*
		 * We're done at i==3 unless the first 31-kbits bits, not counting
		 * those ending up in sign and *pk, are all zeros or ones.
		 */
		if (i == 3 && ((r[1] + 1u) & (0x7fffffffu >> kbits)) > 1)
			break;
#endif
	}

	*pk = kbits ? (r[1] >> (32 - kbits)) : 0;
	if ((neg = ((r[1] >> (31 - kbits)) & 1))) {
		(*pk)++;
		/* Two-complement negation */
		for (j = 1; j <= i; j++) r[j] ^= 0xffffffffu;
		add_at (&r[i], 1);
	}
	r[1] &= (0xffffffffu >> kbits);

	j = 1;
	if (r[j] == 0) j++;
	r4[0] = r4[1] = r4[2] = r4[3] = 0;
	add_at (&r4[1], (guint64)r[j    ] * pi_over_four[0]);
	add_at (&r4[2], (guint64)r[j    ] * pi_over_four[1]);
	add_at (&r4[2], (guint64)r[j + 1] * pi_over_four[0]);
	add_at (&r4[3], (guint64)r[j    ] * pi_over_four[2]);
	add_at (&r4[3], (guint64)r[j + 1] * pi_over_four[1]);
	add_at (&r4[3], (guint64)r[j + 2] * pi_over_four[0]);

	h48 = SUFFIX(ldexp) (((guint64)r4[0] << 16) | (r4[1] >> 16),
			 -32 * j + (kbits + 1) - 16);
	l48 = SUFFIX(ldexp) (((guint64)(r4[1] & 0xffff) << 32) | r4[2],
			 -32 * j + (kbits + 1) - 64);

	rh = h48 + l48;
	rl = h48 - rh + l48;

	if (neg) {
		rh = -rh;
		rl = -rl;
	}

	return SUFFIX(ldexp) (rh + rl, 2 - kbits);
}

#endif

/**
  * go_reduce_pi:
  * @x: number of reduce
  * @e: scale between -1 and 8, inclusive.
  * @k: (out): location to return lower @e+1 bits of reduction count
  *
  * This function performs range reduction for trigonometric functions.
  *
  * Returns: a value, xr, such that x = xr + j * Pi/2^@e for some integer
  * number j and |xr| <=  Pi/2^(@e+1).  The lower @e+1 bits of j will be
  * returned in @k.
  */
DOUBLE
SUFFIX(go_reduce_pi) (DOUBLE x, int e, int *k)
{
	DOUBLE xr;

	g_return_val_if_fail (e >= -1 && e <= 8, x);
	g_return_val_if_fail (k != NULL, x);

	if (!SUFFIX(go_finite) (x)) {
		*k = 0;
		return SUFFIX(go_nan);
	}

#if DOUBLE_RADIX == 2
	DOUBLE ax = SUFFIX(fabs) (x);
	if (ax < (1u << (27 - e)))
		xr = SUFFIX(reduce_pi_simple) (ax, k, e + 1);
	else
		xr = SUFFIX(reduce_pi_full) (ax, k, e + 1);

	if (x < 0) {
		xr = -xr;
		*k = -*k;
	}
	*k &= ((1 << (e + 1)) - 1);
#else
	// Not implemented
	*k = 0;
	xr = SUFFIX(go_nan);
#endif

	return xr;
}


/* ------------------------------------------------------------------------- */


// See comments at top
#endif // SKIP_THIS_PASS
#if INCLUDE_PASS < INCLUDE_PASS_LAST
#include __FILE__
#endif
