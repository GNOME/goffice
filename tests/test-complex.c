/*
 * test-complex.c:  Tests for the complex number library (double version).
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
#include <goffice/math/go-complex.h>

#include <float.h>
#include <math.h>
#include <string.h>

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

/*
 * Compare a single component.  The error is measured relative to @scale,
 * which makes it possible to demand "zero" in one component while the
 * other one is large.
 */
static gboolean
component_ok (double got, double want, double scale, double tol)
{
	if (isnan (want))
		return isnan (got);
	if (isinf (want))
		return got == want;
	return fabs (got - want) <= tol * scale;
}

static void
check_c (const char *what, const GOComplex *got,
	 double re, double im, double tol)
{
	double scale = MAX (fabs (re), fabs (im));
	if (!(scale > 0) || isinf (scale))
		scale = 1;

	if (component_ok (got->re, re, scale, tol) &&
	    component_ok (got->im, im, scale, tol))
		return;

	g_printerr ("FAIL: %s\n    got  (%.17g, %.17g)\n    want (%.17g, %.17g)\n",
		    what, got->re, got->im, re, im);
	failures++;
}

static void
check_str (const char *what, char *got, const char *want)
{
	if (g_strcmp0 (got, want) != 0) {
		g_printerr ("FAIL: %s\n    got  \"%s\"\n    want \"%s\"\n",
			    what, got ? got : "(null)", want);
		failures++;
	}
	g_free (got);
}

/* ------------------------------------------------------------------------- */

static void
test_basics (void)
{
	GOComplex z, w;

	go_complex_init (&z, 3, -4);
	CHECK (z.re == 3 && z.im == -4);

	go_complex_real (&z, 7);
	CHECK (z.re == 7 && z.im == 0);
	CHECK (go_complex_real_p (&z));

	go_complex_init (&z, 7, 1e-300);
	CHECK (!go_complex_real_p (&z));

	go_complex_init (&z, 0, 0);
	CHECK (go_complex_zero_p (&z));
	go_complex_init (&z, 0, 1e-300);
	CHECK (!go_complex_zero_p (&z));
	go_complex_init (&z, 1e-300, 0);
	CHECK (!go_complex_zero_p (&z));

	/* Invalid means "not finite", i.e., NaN or infinite.  */
	go_complex_init (&z, 1, 2);
	CHECK (!go_complex_invalid_p (&z));
	go_complex_invalid (&z);
	CHECK (go_complex_invalid_p (&z));
	CHECK (isnan (z.re) && isnan (z.im));
	go_complex_init (&z, go_nan, 0);
	CHECK (go_complex_invalid_p (&z));
	go_complex_init (&z, 0, go_nan);
	CHECK (go_complex_invalid_p (&z));
	go_complex_init (&z, go_pinf, 0);
	CHECK (go_complex_invalid_p (&z));
	go_complex_init (&z, 0, go_ninf);
	CHECK (go_complex_invalid_p (&z));

	go_complex_init (&z, 1, 2);
	go_complex_conj (&w, &z);
	check_c ("conj", &w, 1, -2, 0);
	go_complex_conj (&z, &z);
	check_c ("conj in place", &z, 1, -2, 0);

	go_complex_init (&z, 1.5, -2);
	go_complex_scale_real (&z, 4);
	check_c ("scale_real", &z, 6, -8, 0);

	CHECK (go_complex_mod (&z) == 10);
	go_complex_init (&z, -1, 0);
	CHECK (go_complex_angle (&z) == M_PI);
	CHECK (go_complex_angle_pi (&z) == 1);
	go_complex_init (&z, 0, 1);
	CHECK (go_complex_angle_pi (&z) == 0.5);
	go_complex_init (&z, 1, 0);
	CHECK (go_complex_angle_pi (&z) == 0);
}

static void
test_boxed (void)
{
	GType t = go_complex_get_type ();
	GOComplex z, *c;

	CHECK (G_TYPE_IS_BOXED (t));
	CHECK (t == go_complex_get_type ());

	go_complex_init (&z, 1, 2);
	c = g_boxed_copy (t, &z);
	CHECK (c != NULL && c != &z);
	if (c) {
		CHECK (c->re == 1 && c->im == 2);
		g_boxed_free (t, c);
	}
}

/* ------------------------------------------------------------------------- */

static void
to_string_case (double re, double im, const char *reformat,
		const char *imformat, char imunit, const char *want)
{
	GOComplex z;
	char *what = g_strdup_printf ("to_string (%g, %g, \"%s\", \"%s\", '%c')",
				      re, im, reformat, imformat, imunit);
	go_complex_init (&z, re, im);
	check_str (what,
		   go_complex_to_string (&z, reformat, imformat, imunit),
		   want);
	g_free (what);
}

static void
test_to_string (void)
{
	/* Plain cases */
	to_string_case (0, 0, "%g", "%g", 'i', "0");
	to_string_case (3, 0, "%g", "%g", 'i', "3");
	to_string_case (-3, 0, "%g", "%g", 'i', "-3");
	to_string_case (1, 2, "%g", "%g", 'i', "1+2i");
	to_string_case (1, -2, "%g", "%g", 'i', "1-2i");
	to_string_case (-1.5, 2.5, "%g", "%g", 'i', "-1.5+2.5i");
	to_string_case (-1.5, -2.5, "%g", "%g", 'i', "-1.5-2.5i");

	/* Purely imaginary: there is no real part to show.  */
	to_string_case (0, 2, "%g", "%g", 'i', "2i");
	to_string_case (0, -2, "%g", "%g", 'i', "-2i");

	/* A unit imaginary part is written without its coefficient.  */
	to_string_case (0, 1, "%g", "%g", 'i', "i");
	to_string_case (0, -1, "%g", "%g", 'i', "-i");
	to_string_case (1, 1, "%g", "%g", 'i', "1+i");
	to_string_case (1, -1, "%g", "%g", 'i', "1-i");

	/* Imaginary unit character */
	to_string_case (1, 2, "%g", "%g", 'j', "1+2j");
	to_string_case (0, -1, "%g", "%g", 'j', "-j");

	/* Formats are used as given.  An explicit plus must not be doubled.  */
	to_string_case (1, 2, "%g", "%+g", 'i', "1+2i");
	to_string_case (1, -2, "%g", "%+g", 'i', "1-2i");
	to_string_case (1.5, 2.25, "%.2f", "%.3f", 'i', "1.50+2.250i");

	/*
	 * Regression test: a NaN imaginary part used to get a minus sign
	 * because NaN >= 0 is false.  Use a NaN with a clear sign bit as
	 * the sign of "nan" is printed by the C library.
	 */
	to_string_case (1, copysign (NAN, 1.0), "%g", "%g", 'i', "1+nani");
}

/* ------------------------------------------------------------------------- */

static void
from_string_case (const char *src, double re, double im, char imunit)
{
	GOComplex z = { -999, -999 };
	char unit = '?';
	char *what = g_strdup_printf ("from_string (\"%s\")", src);
	int res = go_complex_from_string (&z, src, &unit);

	if (res != 0) {
		g_printerr ("FAIL: %s returned %d\n", what, res);
		failures++;
	} else {
		check_c (what, &z, re, im, 0);
		if (unit != imunit) {
			g_printerr ("FAIL: %s: unit '%c', wanted '%c'\n",
				    what, unit, imunit);
			failures++;
		}
	}
	g_free (what);
}

static void
from_string_fails (const char *src)
{
	GOComplex z;
	char unit;
	char *what = g_strdup_printf ("from_string (\"%s\") must fail", src);
	check (go_complex_from_string (&z, src, &unit) != 0, what);
	g_free (what);
}

static void
test_from_string (void)
{
	/* Real numbers */
	from_string_case ("0", 0, 0, 'i');
	from_string_case ("42", 42, 0, 'i');
	from_string_case ("-42", -42, 0, 'i');
	from_string_case ("+42", 42, 0, 'i');
	from_string_case ("1.5e2", 150, 0, 'i');

	/* Pure imaginary numbers */
	from_string_case ("i", 0, 1, 'i');
	from_string_case ("+i", 0, 1, 'i');
	from_string_case ("-i", 0, -1, 'i');
	from_string_case ("j", 0, 1, 'j');
	from_string_case ("-j", 0, -1, 'j');
	from_string_case ("42i", 0, 42, 'i');
	from_string_case ("-42i", 0, -42, 'i');
	from_string_case ("2.5j", 0, 2.5, 'j');

	/* General numbers */
	from_string_case ("3+4i", 3, 4, 'i');
	from_string_case ("3-4i", 3, -4, 'i');
	from_string_case ("-3+4i", -3, 4, 'i');
	from_string_case ("-3-4i", -3, -4, 'i');
	from_string_case ("3+i", 3, 1, 'i');
	from_string_case ("3-i", 3, -1, 'i');
	from_string_case ("-3-i", -3, -1, 'i');
	from_string_case ("1.5e2+2.5e-1j", 150, 0.25, 'j');
	from_string_case ("1e+2-1e-2i", 100, -0.01, 'i');

	/* Garbage */
	from_string_fails ("");
	from_string_fails ("abc");
	from_string_fails ("k");
	from_string_fails ("3+");
	from_string_fails ("3+4");
	from_string_fails ("3+4k");
	from_string_fails ("3+4ii");
	from_string_fails ("3+ii");
	from_string_fails ("i3");
	from_string_fails ("3i+4");
	from_string_fails ("3+4i garbage");
}

static void
test_string_roundtrip (void)
{
	static const double vals[][2] = {
		{ 0, 0 }, { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
		{ 1, 1 }, { 1, -1 }, { -2.5, 0.125 }, { 1.0 / 3, -2.0 / 3 },
		{ 1e-300, 1e300 }, { -1e300, -1e-300 }, { 12345.6789, 0.001 }
	};
	unsigned ui;

	for (ui = 0; ui < G_N_ELEMENTS (vals); ui++) {
		GOComplex z, w;
		char unit;
		char *s, *what;

		go_complex_init (&z, vals[ui][0], vals[ui][1]);
		s = go_complex_to_string (&z, "%.17g", "%.17g", 'i');
		what = g_strdup_printf ("round trip of \"%s\"", s);
		if (go_complex_from_string (&w, s, &unit) != 0 ||
		    w.re != z.re || w.im != z.im || unit != 'i') {
			g_printerr ("FAIL: %s\n", what);
			failures++;
		}
		g_free (what);
		g_free (s);
	}
}

/* ------------------------------------------------------------------------- */

static void
test_polar (void)
{
	GOComplex z;
	double m, a;

	go_complex_init (&z, 3, 4);
	go_complex_to_polar (&m, &a, &z);
	CHECK (m == 5);
	CHECK (fabs (a - atan2 (4, 3)) < 1e-16);

	go_complex_from_polar (&z, 2, M_PI / 2);
	check_c ("from_polar (2, pi/2)", &z, 0, 2, 1e-15);
	go_complex_from_polar (&z, 3, 0);
	check_c ("from_polar (3, 0)", &z, 3, 0, 0);

	/* The _pi version has exact results at the important angles.  */
	go_complex_from_polar_pi (&z, 2, 0.5);
	CHECK (z.re == 0 && z.im == 2);
	go_complex_from_polar_pi (&z, 1, 1);
	CHECK (z.re == -1 && z.im == 0);
	go_complex_from_polar_pi (&z, 1, 0);
	CHECK (z.re == 1 && z.im == 0);
	go_complex_from_polar_pi (&z, 1, -0.5);
	CHECK (z.re == 0 && z.im == -1);
	go_complex_from_polar_pi (&z, 2, 0.25);
	check_c ("from_polar_pi (2, 1/4)", &z, M_SQRT2, M_SQRT2, 1e-15);
}

/* ------------------------------------------------------------------------- */

static void
test_add_sub_mul (void)
{
	GOComplex a, b, r;

	go_complex_init (&a, 1, 2);
	go_complex_init (&b, 3, -5);

	go_complex_add (&r, &a, &b);
	check_c ("add", &r, 4, -3, 0);
	go_complex_sub (&r, &a, &b);
	check_c ("sub", &r, -2, 7, 0);
	/* Order matters: this is where "@b is the second argument" shows. */
	go_complex_sub (&r, &b, &a);
	check_c ("sub reversed", &r, 2, -7, 0);

	go_complex_init (&a, 1, 2);
	go_complex_init (&b, 3, 4);
	go_complex_mul (&r, &a, &b);
	check_c ("mul", &r, -5, 10, 0);

	/* Aliasing */
	go_complex_mul (&a, &a, &a);
	check_c ("mul in place", &a, -3, 4, 0);
	go_complex_add (&b, &b, &b);
	check_c ("add in place", &b, 6, 8, 0);
}

static void
div_case (const char *what, double are, double aim, double bre, double bim,
	  double re, double im, double tol)
{
	GOComplex a, b, r;
	go_complex_init (&a, are, aim);
	go_complex_init (&b, bre, bim);
	go_complex_div (&r, &a, &b);
	check_c (what, &r, re, im, tol);
}

static void
test_div (void)
{
	GOComplex a, b, r;

	div_case ("div general", 1, 2, 3, 4, 0.44, 0.08, 1e-15);
	div_case ("div by real", 3, 6, 3, 0, 1, 2, 0);
	div_case ("div by imaginary", 1, 2, 0, 2, 1, -0.5, 0);
	div_case ("div by one", 5, -7, 1, 0, 5, -7, 0);
	div_case ("div zero numerator", 0, 0, 1, 1, 0, 0, 0);

	/* Where the textbook formula overflows. */
	div_case ("div, huge operands", 1e200, 1e200, 1e200, -1e200,
		  0, 1, 1e-14);
	div_case ("div, huge numerator", 1e300, 1e300, 1e200, 1e200,
		  1e100, 0, 1e-14);

	/* ...and where it underflows. */
	div_case ("div, tiny operands", 1e-200, 1e-200, 1e-200, -1e-200,
		  0, 1, 1e-14);
	div_case ("div, tiny numerator", 1e-300, 1e-300, 1e-200, 2e-200,
		  0.6e-100, -0.2e-100, 1e-14);

	/* Division by zero, infinities and NaN all give "invalid".  */
	go_complex_init (&a, 1, 1);
	go_complex_init (&b, 0, 0);
	go_complex_div (&r, &a, &b);
	CHECK (go_complex_invalid_p (&r));

	go_complex_init (&a, go_pinf, 1);
	go_complex_init (&b, 1, 1);
	go_complex_div (&r, &a, &b);
	CHECK (go_complex_invalid_p (&r));

	go_complex_init (&a, 1, 1);
	go_complex_init (&b, 1, go_pinf);
	go_complex_div (&r, &a, &b);
	CHECK (go_complex_invalid_p (&r));

	go_complex_init (&a, 1, 1);
	go_complex_init (&b, go_nan, 1);
	go_complex_div (&r, &a, &b);
	CHECK (go_complex_invalid_p (&r));

	/* Aliasing */
	go_complex_init (&a, 1, 2);
	go_complex_init (&b, 3, 4);
	go_complex_div (&a, &a, &b);
	check_c ("div in place", &a, 0.44, 0.08, 1e-15);
	go_complex_div (&b, &b, &b);
	check_c ("z / z", &b, 1, 0, 1e-15);
}

/* ------------------------------------------------------------------------- */

static void
sqrt_case (const char *what, double re, double im,
	   double wre, double wim, double tol)
{
	GOComplex z, r;
	go_complex_init (&z, re, im);
	go_complex_sqrt (&r, &z);
	check_c (what, &r, wre, wim, tol);
}

static void
sqrt_square_case (double re, double im)
{
	GOComplex z, r, sq;
	char *what = g_strdup_printf ("sqrt (%g, %g) squared", re, im);

	go_complex_init (&z, re, im);
	go_complex_sqrt (&r, &z);
	/* Halve before squaring (exactly) so that the square of a root of
	 * something near DBL_MAX cannot overflow.  */
	go_complex_scale_real (&r, 0.5);
	go_complex_mul (&sq, &r, &r);
	check_c (what, &sq, re / 4, im / 4, 1e-13);
	go_complex_scale_real (&r, 2);
	/* The principal root has a non-negative real part.  */
	check (r.re >= 0, what);
	g_free (what);
}

static void
test_sqrt (void)
{
	sqrt_case ("sqrt 0", 0, 0, 0, 0, 0);
	sqrt_case ("sqrt 4", 4, 0, 2, 0, 1e-15);
	sqrt_case ("sqrt -4", -4, 0, 0, 2, 1e-15);
	sqrt_case ("sqrt i", 0, 1, M_SQRT1_2, M_SQRT1_2, 1e-15);
	sqrt_case ("sqrt -i", 0, -1, M_SQRT1_2, -M_SQRT1_2, 1e-15);
	sqrt_case ("sqrt 3+4i", 3, 4, 2, 1, 1e-15);
	sqrt_case ("sqrt 3-4i", 3, -4, 2, -1, 1e-15);
	sqrt_case ("sqrt -3+4i", -3, 4, 1, 2, 1e-15);
	sqrt_case ("sqrt -3-4i", -3, -4, 1, -2, 1e-15);

	/* The two branches of the implementation, on both sides of the cut. */
	sqrt_case ("sqrt -1+1e-20i", -1, 1e-20, 5e-21, 1, 1e-15);
	sqrt_case ("sqrt -1-1e-20i", -1, -1e-20, 5e-21, -1, 1e-15);

	/* Extreme magnitudes */
	sqrt_case ("sqrt 1e300", 1e300, 0, 1e150, 0, 1e-15);
	sqrt_case ("sqrt -1e300", -1e300, 0, 0, 1e150, 1e-15);
	sqrt_case ("sqrt 1e-300", 1e-300, 0, 1e-150, 0, 1e-15);

	sqrt_square_case (2, 3);
	sqrt_square_case (-2, 3);
	sqrt_square_case (-2, -3);
	sqrt_square_case (2, -3);
	sqrt_square_case (0.001, -1000);
	sqrt_square_case (-1000, 0.001);
	/* Modulus overflows; the result must still be right.  */
	sqrt_square_case (DBL_MAX, DBL_MAX);
	sqrt_square_case (-DBL_MAX, DBL_MAX / 2);
}

/* ------------------------------------------------------------------------- */

static void
test_exp_ln (void)
{
	GOComplex z, r;

	go_complex_init (&z, 0, 0);
	go_complex_exp (&r, &z);
	check_c ("exp 0", &r, 1, 0, 0);

	go_complex_init (&z, 1, 0);
	go_complex_exp (&r, &z);
	check_c ("exp 1", &r, M_E, 0, 1e-15);

	go_complex_init (&z, 0, M_PI);
	go_complex_exp (&r, &z);
	check_c ("exp i*pi", &r, -1, 0, 1e-15);

	go_complex_init (&z, 0, M_PI / 2);
	go_complex_exp (&r, &z);
	check_c ("exp i*pi/2", &r, 0, 1, 1e-15);

	go_complex_init (&z, M_LN2, M_PI / 3);
	go_complex_exp (&r, &z);
	check_c ("exp ln2 + i*pi/3", &r, 1, sqrt (3.0), 1e-15);

	go_complex_init (&z, -800, 0);
	go_complex_exp (&r, &z);
	check_c ("exp -800", &r, 0, 0, 0);

	/* ln */
	go_complex_init (&z, 1, 0);
	go_complex_ln (&r, &z);
	check_c ("ln 1", &r, 0, 0, 0);

	go_complex_init (&z, M_E, 0);
	go_complex_ln (&r, &z);
	check_c ("ln e", &r, 1, 0, 1e-15);

	go_complex_init (&z, -1, 0);
	go_complex_ln (&r, &z);
	check_c ("ln -1", &r, 0, M_PI, 1e-15);

	go_complex_init (&z, 0, 1);
	go_complex_ln (&r, &z);
	check_c ("ln i", &r, 0, M_PI / 2, 1e-15);

	go_complex_init (&z, 0, -2);
	go_complex_ln (&r, &z);
	check_c ("ln -2i", &r, M_LN2, -M_PI / 2, 1e-15);

	go_complex_init (&z, 3, 4);
	go_complex_ln (&r, &z);
	check_c ("ln 3+4i", &r, log (5.0), atan2 (4, 3), 1e-15);

	go_complex_init (&z, -3, -4);
	go_complex_ln (&r, &z);
	check_c ("ln -3-4i", &r, log (5.0), atan2 (-4, -3), 1e-15);

	/* The modulus must not overflow or underflow.  */
	go_complex_init (&z, 1e300, 1e300);
	go_complex_ln (&r, &z);
	check_c ("ln 1e300(1+i)", &r, log (1e300) + 0.5 * M_LN2, M_PI / 4, 1e-15);

	go_complex_init (&z, 1e-300, -1e-300);
	go_complex_ln (&r, &z);
	check_c ("ln 1e-300(1-i)", &r, log (1e-300) + 0.5 * M_LN2, -M_PI / 4, 1e-15);

	/* On the unit circle the real part is zero. */
	go_complex_init (&z, 0.6, 0.8);
	go_complex_ln (&r, &z);
	check_c ("ln 0.6+0.8i", &r, 0, atan2 (0.8, 0.6), 1e-15);

	/* exp and ln are inverses (away from the branch cut).  */
	go_complex_init (&z, 0.7, -2.1);
	go_complex_ln (&r, &z);
	go_complex_exp (&r, &r);
	check_c ("exp (ln z)", &r, 0.7, -2.1, 1e-14);

	go_complex_init (&z, -1.3, 2.9);
	go_complex_exp (&r, &z);
	go_complex_ln (&r, &r);
	/* Imaginary part 2.9 is inside (-pi,pi], so no 2*pi shift.  */
	check_c ("ln (exp z)", &r, -1.3, 2.9, 1e-14);
}

/* ------------------------------------------------------------------------- */

static void
test_trig (void)
{
	static const double pts[][2] = {
		{ 0.3, 0.7 }, { 1.2, -0.4 }, { -2.0, 0.9 }, { 0.5, 3.0 },
		{ 4.0, -5.0 }, { -0.1, 0.01 }, { 1.0, 1.0 }, { 0.5, 50 },
		{ 1.5, -20 }
	};
	GOComplex z, r;
	unsigned ui;

	go_complex_init (&z, 0, 0);
	go_complex_sin (&r, &z);
	check_c ("sin 0", &r, 0, 0, 0);
	go_complex_cos (&r, &z);
	check_c ("cos 0", &r, 1, 0, 0);
	go_complex_tan (&r, &z);
	check_c ("tan 0", &r, 0, 0, 0);

	go_complex_init (&z, M_PI / 2, 0);
	go_complex_sin (&r, &z);
	check_c ("sin pi/2", &r, 1, 0, 1e-15);
	go_complex_cos (&r, &z);
	check_c ("cos pi/2", &r, 0, 0, 1e-15);

	/* sin(iy) = i sinh y; cos(iy) = cosh y; tan(iy) = i tanh y */
	go_complex_init (&z, 0, 1);
	go_complex_sin (&r, &z);
	check_c ("sin i", &r, 0, sinh (1.0), 1e-15);
	go_complex_cos (&r, &z);
	check_c ("cos i", &r, cosh (1.0), 0, 1e-15);
	go_complex_tan (&r, &z);
	check_c ("tan i", &r, 0, tanh (1.0), 1e-15);

	go_complex_init (&z, 1, 0);
	go_complex_tan (&r, &z);
	check_c ("tan 1", &r, tan (1.0), 0, 1e-15);

	/* Odd and even symmetry. */
	go_complex_init (&z, 0.5, 3.0);
	{
		GOComplex m, rs, rms, rt, rmt, rc, rmc;
		go_complex_init (&m, -0.5, -3.0);
		go_complex_sin (&rs, &z);
		go_complex_sin (&rms, &m);
		go_complex_tan (&rt, &z);
		go_complex_tan (&rmt, &m);
		go_complex_cos (&rc, &z);
		go_complex_cos (&rmc, &m);
		check_c ("sin odd", &rms, -rs.re, -rs.im, 1e-15);
		check_c ("tan odd", &rmt, -rt.re, -rt.im, 1e-15);
		check_c ("cos even", &rmc, rc.re, rc.im, 1e-15);
	}

	for (ui = 0; ui < G_N_ELEMENTS (pts); ui++) {
		GOComplex s, c, s2, c2, sum, q, t;
		char *what;

		go_complex_init (&z, pts[ui][0], pts[ui][1]);
		go_complex_sin (&s, &z);
		go_complex_cos (&c, &z);

		/* sin^2 + cos^2 = 1 */
		go_complex_mul (&s2, &s, &s);
		go_complex_mul (&c2, &c, &c);
		go_complex_add (&sum, &s2, &c2);
		what = g_strdup_printf ("sin^2+cos^2 at (%g,%g)", z.re, z.im);
		/* Cancellation makes the error relative to the squares.  */
		{
			double scale = MAX (go_complex_mod (&s2),
					    go_complex_mod (&c2));
			if (!(fabs (sum.re - 1) <= 1e-15 * scale &&
			      fabs (sum.im) <= 1e-15 * scale)) {
				g_printerr ("FAIL: %s: (%.17g, %.17g)\n",
					    what, sum.re, sum.im);
				failures++;
			}
		}
		g_free (what);

		/* tan = sin / cos */
		go_complex_div (&q, &s, &c);
		go_complex_tan (&t, &z);
		what = g_strdup_printf ("tan = sin/cos at (%g,%g)", z.re, z.im);
		check_c (what, &t, q.re, q.im, 1e-13);
		g_free (what);
	}

	/* For large imaginary part, tan approaches +-i.  */
	go_complex_init (&z, 0.5, 50);
	go_complex_tan (&r, &z);
	CHECK (r.im == 1);
	CHECK (fabs (r.re) < 1e-40);
	go_complex_init (&z, 0.5, -50);
	go_complex_tan (&r, &z);
	CHECK (r.im == -1);
	CHECK (fabs (r.re) < 1e-40);
}

/* ------------------------------------------------------------------------- */

static void
pow_case (const char *what, double are, double aim, double bre, double bim,
	  double re, double im, double tol)
{
	GOComplex a, b, r;
	go_complex_init (&a, are, aim);
	go_complex_init (&b, bre, bim);
	go_complex_pow (&r, &a, &b);
	check_c (what, &r, re, im, tol);
}

static void
test_pow (void)
{
	GOComplex a, b, r, ex;
	double e;

	/* Real arguments, including the fast path.  */
	pow_case ("2^3", 2, 0, 3, 0, 8, 0, 1e-15);
	pow_case ("2^0.5", 2, 0, 0.5, 0, M_SQRT2, 0, 1e-15);
	pow_case ("2^-2", 2, 0, -2, 0, 0.25, 0, 1e-15);
	pow_case ("0^0", 0, 0, 0, 0, 1, 0, 0);
	pow_case ("0^2", 0, 0, 2, 0, 0, 0, 0);
	pow_case ("(-2)^3", -2, 0, 3, 0, -8, 0, 1e-15);

	/* Special exponents.  */
	pow_case ("z^0", 1.5, -2.5, 0, 0, 1, 0, 0);
	pow_case ("z^1", 1.5, -2.5, 1, 0, 1.5, -2.5, 0);
	pow_case ("z^2", 1.5, -2.5, 2, 0, 1.5 * 1.5 - 2.5 * 2.5, -7.5, 1e-15);
	pow_case ("(1+i)^2", 1, 1, 2, 0, 0, 2, 1e-15);

	/* Principal values. */
	pow_case ("(-1)^0.5", -1, 0, 0.5, 0, 0, 1, 1e-15);
	pow_case ("(-8)^(1/3)", -8, 0, 1.0 / 3, 0, 1, sqrt (3.0), 1e-14);
	pow_case ("i^i", 0, 1, 0, 1, exp (-M_PI / 2), 0, 1e-14);
	pow_case ("i^2 via general path", 0, 1, 2.0000000000000004, 0,
		  -1, 0, 1e-14);
	pow_case ("(1+i)^4", 1, 1, 4, 0, -4, 0, 1e-14);
	pow_case ("e^(i*pi)", M_E, 0, 0, M_PI, -1, 0, 1e-14);

	/* z^w = exp (w ln z) */
	go_complex_init (&a, 1.5, 0.5);
	go_complex_init (&b, 0.7, -1.3);
	go_complex_pow (&r, &a, &b);
	go_complex_ln (&ex, &a);
	go_complex_mul (&ex, &ex, &b);
	go_complex_exp (&ex, &ex);
	check_c ("z^w == exp (w ln z)", &r, ex.re, ex.im, 1e-13);

	go_complex_init (&a, -2.5, -1.0);
	go_complex_init (&b, -1.1, 2.2);
	go_complex_pow (&r, &a, &b);
	go_complex_ln (&ex, &a);
	go_complex_mul (&ex, &ex, &b);
	go_complex_exp (&ex, &ex);
	check_c ("z^w == exp (w ln z), 3rd quadrant", &r, ex.re, ex.im, 1e-13);

	/* Aliasing */
	go_complex_init (&a, 1, 1);
	go_complex_init (&b, 4, 0);
	go_complex_pow (&a, &a, &b);
	check_c ("pow in place", &a, -4, 0, 1e-14);

	/* powx: the result is dst * 2^e.  */
	go_complex_init (&a, 2, 0);
	go_complex_init (&b, 3, 0);
	go_complex_powx (&r, &e, &a, &b);
	CHECK (fabs (ldexp (r.re, (int)e) - 8) <= 1e-14 * 8);
	CHECK (fabs (ldexp (r.im, (int)e)) <= 1e-14 * 8);

	go_complex_init (&b, 0, 0);
	go_complex_powx (&r, &e, &a, &b);
	check_c ("powx, exponent 0", &r, 1, 0, 0);
	CHECK (e == 0);

	/*
	 * 10^400 is not representable, but powx can still represent it as
	 * mantissa * 2^e.  Compare logarithms.
	 */
	go_complex_init (&a, 10, 0);
	go_complex_init (&b, 400, 0);
	go_complex_powx (&r, &e, &a, &b);
	CHECK (r.re > 0 && isfinite (r.re));
	CHECK (fabs (log10 (r.re) + e * log10 (2.0) - 400) < 1e-9);
	CHECK (fabs (r.im) <= 1e-14 * r.re);

	/* ...and 10^-400 likewise. */
	go_complex_init (&b, -400, 0);
	go_complex_powx (&r, &e, &a, &b);
	CHECK (r.re > 0 && isfinite (r.re));
	CHECK (fabs (log10 (r.re) + e * log10 (2.0) + 400) < 1e-9);

	/* With e == NULL powx is pow.  */
	go_complex_init (&a, 1.5, 0.5);
	go_complex_init (&b, 0.7, -1.3);
	go_complex_powx (&ex, NULL, &a, &b);
	go_complex_pow (&r, &a, &b);
	CHECK (ex.re == r.re && ex.im == r.im);
}

/* ------------------------------------------------------------------------- */

int
main (int argc, char **argv)
{
	(void)argc;
	(void)argv;

	libgoffice_init ();

	test_basics ();
	test_boxed ();
	test_to_string ();
	test_from_string ();
	test_string_roundtrip ();
	test_polar ();
	test_add_sub_mul ();
	test_div ();
	test_sqrt ();
	test_exp_ln ();
	test_trig ();
	test_pow ();

	libgoffice_shutdown ();

	if (failures)
		g_printerr ("%d complex number test(s) failed.\n", failures);

	return failures ? 1 : 0;
}
