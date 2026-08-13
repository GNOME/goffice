/*
 * test-classify.c:  Tests for the floating-point classification helpers.
 *
 * go_finite() and friends must agree with the C99/POSIX type-generic macros
 * for the binary types, and must not go through them for _Decimal64.
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
#include <goffice/math/go-math.h>
#ifdef GOFFICE_WITH_DECIMAL64
#include <goffice/math/go-decimal.h>
#endif

#include <float.h>
#include <math.h>

static int failures;

static void
check (gboolean ok, const char *what)
{
	if (!ok) {
		g_printerr ("FAIL: %s\n", what);
		failures++;
	}
}

#define CHECK(cond) check ((cond) ? TRUE : FALSE, #cond)

/* ------------------------------------------------------------------------- */

static void
test_double (void)
{
	/* go_finite() wraps the type-generic isfinite().  */
	CHECK (go_finite (0.0));
	CHECK (go_finite (-0.0));
	CHECK (go_finite (1.0));
	CHECK (go_finite (-1.0));
	CHECK (go_finite (DBL_MAX));
	CHECK (go_finite (-DBL_MAX));
	CHECK (go_finite (DBL_MIN));

	CHECK (!go_finite (go_pinf));
	CHECK (!go_finite (go_ninf));
	CHECK (!go_finite (go_nan));

	CHECK (isnan (go_nan));
	CHECK (!isnan (go_pinf));
	CHECK (!isnan (go_ninf));
	CHECK (!isnan (0.0));
	CHECK (!isnan (1.0));

	/* The infinities must have the right signs.  */
	CHECK (go_pinf > 0);
	CHECK (go_ninf < 0);
	CHECK (go_pinf > DBL_MAX);
	CHECK (go_ninf < -DBL_MAX);

	/* NaN is unordered with respect to everything, including itself.  */
	CHECK (!(go_nan == go_nan));
	CHECK (!(go_nan < 0));
	CHECK (!(go_nan > 0));

	/* What a -ffast-math build would break: the compiler folds the
	 * isfinite() away and these come out true.  configure rejects such
	 * a build, but the check costs nothing.  */
	CHECK (go_finite (go_pinf) == 0);
	CHECK (go_finite (go_nan) == 0);

	/* Non-finite values pass through the epsilon helpers unchanged.  */
	CHECK (go_add_epsilon (go_pinf) == go_pinf);
	CHECK (go_sub_epsilon (go_ninf) == go_ninf);
	CHECK (go_add_epsilon (0.0) == 0.0);
	CHECK (go_sub_epsilon (0.0) == 0.0);
}

/* ------------------------------------------------------------------------- */

#ifdef GOFFICE_WITH_LONG_DOUBLE
static void
test_long_double (void)
{
	CHECK (go_finitel (0.0L));
	CHECK (go_finitel (1.0L));
	CHECK (go_finitel (-1.0L));
	CHECK (go_finitel (LDBL_MAX));
	CHECK (go_finitel (-LDBL_MAX));
	CHECK (go_finitel (LDBL_MIN));

	CHECK (!go_finitel (go_pinfl));  // Fails under valgrind
	CHECK (!go_finitel (go_ninfl));  // Fails under valgrind
	CHECK (!go_finitel (go_nanl));

	CHECK (isnan (go_nanl));
	CHECK (!isnan (go_pinfl));
	CHECK (!isnan (go_ninfl));
	CHECK (!isnan (1.0L));

	CHECK (isfinite (1.0L));
	CHECK (!isfinite (go_pinfl));  // Fails under valgrind
	CHECK (!isfinite (go_nanl));

	CHECK (go_pinfl > 0);
	CHECK (go_ninfl < 0);
	CHECK (!(go_nanl == go_nanl));

	/* go_finitel() must agree with the macro it is built on.  */
	CHECK (!go_finitel (go_pinfl) == !isfinite (go_pinfl));
	CHECK (!go_finitel (1.0L) == !isfinite (1.0L));

	CHECK (go_add_epsilonl (go_pinfl) == go_pinfl);
	CHECK (go_sub_epsilonl (go_ninfl) == go_ninfl);
}
#endif

/* ------------------------------------------------------------------------- */

#ifdef GOFFICE_WITH_DECIMAL64
static void
test_decimal (void)
{
	CHECK (go_finiteD (0.dd));
	CHECK (go_finiteD (1.dd));
	CHECK (go_finiteD (-1.dd));

	CHECK (!go_finiteD (go_pinfD));
	CHECK (!go_finiteD (go_ninfD));
	CHECK (!go_finiteD (go_nanD));

	CHECK (isnanD (go_nanD));
	CHECK (!isnanD (go_pinfD));
	CHECK (!isnanD (go_ninfD));
	CHECK (!isnanD (1.dd));

	CHECK (isfiniteD (1.dd));
	CHECK (!isfiniteD (go_pinfD));
	CHECK (!isfiniteD (go_nanD));

	CHECK (signbitD (-1.dd));
	CHECK (!signbitD (1.dd));

	CHECK (go_pinfD > 0);
	CHECK (go_ninfD < 0);
	CHECK (!(go_nanD == go_nanD));

	/* go_finiteD() must be exactly isfiniteD().  */
	CHECK (!go_finiteD (go_pinfD) == !isfiniteD (go_pinfD));
	CHECK (!go_finiteD (go_nanD) == !isfiniteD (go_nanD));
	CHECK (!go_finiteD (1.dd) == !isfiniteD (1.dd));

	/*
	 * The important one.  DECIMAL64_MAX is about 1e385, far beyond a
	 * 64-bit long double.  Should the decimal branch of go_finite()
	 * ever be "simplified" to the type-generic isfinite(), the implicit
	 * conversion overflows and reports a finite number as infinite --
	 * on ARM32, MSVC and wherever else long double is not extended.
	 */
	CHECK (go_finiteD (DECIMAL64_MAX));
	CHECK (go_finiteD (-DECIMAL64_MAX));
	CHECK (isfiniteD (DECIMAL64_MAX));
	CHECK (go_finiteD (DECIMAL64_MIN));

	/* Likewise, tiny decimals must not be flushed to zero.  */
	CHECK (DECIMAL64_MIN > 0);
	CHECK (DECIMAL64_MAX > 0);
}
#endif

/* ------------------------------------------------------------------------- */

int
main (int argc, char **argv)
{
	GString *passes;

	(void)argc;
	(void)argv;

	libgoffice_init ();

	passes = g_string_new ("double");
	test_double ();

#ifdef GOFFICE_WITH_LONG_DOUBLE
	g_string_append (passes, ", long double");
	test_long_double ();
#endif

#ifdef GOFFICE_WITH_DECIMAL64
	g_string_append (passes, ", _Decimal64");
	test_decimal ();
#endif

	/* Say what was covered: the optional sections vanish at
	 * preprocessing time, so otherwise every run looks alike.  */
	g_print ("test-classify: exercised %s\n", passes->str);
	g_string_free (passes, TRUE);

	libgoffice_shutdown ();

	if (failures)
		g_printerr ("%d classification test(s) failed.\n", failures);

	return failures ? 1 : 0;
}
