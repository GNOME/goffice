#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <errno.h>

#define REFTEST(a_,f_,r_, txt_)						\
	do {								\
		double a = (a_);					\
		double r = (r_);					\
		double fa = f_(a);					\
		g_printerr ("%s(%g) = %g  [%g]\n",			\
			    txt_, a, fa, r);				\
		g_assert (copysign (1,r) == copysign (1,fa));		\
		if (isnan (r))						\
			g_assert (isnan (fa));				\
		else if (r == floor (r))				\
			g_assert (r == fa);				\
		else							\
			g_assert (fabs (r - fa) / fabs (r) < 1e-14);	\
	} while (0)

#define REFTEST2(a_,b_,f_,r_, txt_)					\
	do {								\
		double a = (a_);					\
		double b = (b_);					\
		double r = (r_);					\
		double fab = f_(a, b);					\
		g_printerr ("%s(%g,%g) = %g  [%g]\n",			\
			    txt_, a, b, fab, r);			\
		g_assert (copysign (1,r) == copysign(1,fab));		\
		if (r * 256 == floor (r * 256))				\
			g_assert (r == fab);				\
		else							\
			g_assert (fabs (r - fab) / fabs (r) < 1e-14);	\
	} while (0)

#ifdef GOFFICE_WITH_LONG_DOUBLE

/* isnan() below is the C99 type-generic macro, which handles long double;
 * isnanl() is a glibc extension.  */

#define REFTESTl(a_,f_,r_, txt_)					\
	do {								\
		long double a = (a_);					\
		long double r = (r_);					\
		long double fa = f_(a);					\
		g_printerr ("%s(%Lg) = %Lg  [%Lg]\n",			\
			    txt_, a, fa, r);				\
		g_assert (copysignl (1,r) == copysignl (1,fa));		\
		if (isnan (r))						\
			g_assert (isnan (fa));				\
		else if (r == floorl (r))				\
			g_assert (r == fa);				\
		else							\
			g_assert (fabsl (r - fa) / fabsl (r) < 1e-14);	\
	} while (0)

#define REFTEST2l(a_,b_,f_,r_, txt_)					\
	do {								\
		long double a = (a_);					\
		long double b = (b_);					\
		long double r = (r_);					\
		long double fab = f_(a, b);				\
		g_printerr ("%s(%Lg,%Lg) = %Lg  [%Lg]\n",		\
			    txt_, a, b, fab, r);			\
		g_assert (copysignl (1,r) == copysignl(1,fab));		\
		if (r * 256 == floorl (r * 256))			\
			g_assert (r == fab);				\
		else							\
			g_assert (fabsl (r - fab) / fabsl (r) < 1e-14);	\
	} while (0)
#else
#define REFTESTl(a_,f_,r_, txt_)
#define REFTEST2l(a_,b_,f_,r_, txt_)
#endif

/* ------------------------------------------------------------------------- */

static double fake_sinpi (double x) { return x == floor (x) ? copysign(0,x) : sin (x * M_PI); }
static double fake_cospi (double x) { return x + 0.5 == floor (x + 0.5) ? +0 : cos (x * M_PI); }
static double fake_tanpi (double x) { x = fmod(x,1); return x + 0.5 == floor (x + 0.5) ? copysign (go_nan,x) : tan (x * M_PI); }
static double fake_cotpi (double x) { x = fmod(x,1); return x == 0 ? copysign (go_nan,x) : fake_cospi (x) / fake_sinpi (x); }
static double fake_atanpi (double x) { return atan (x) / M_PI; }
static double fake_atan2pi (double y, double x) { return atan2 (y,x) / M_PI; }


#define TEST1(a_) do {							\
	REFTEST(a_,go_sinpi,fake_sinpi(a_),"sinpi");			\
	REFTEST(a_,go_cospi,fake_cospi(a_),"cospi");			\
	REFTEST(a_,go_tanpi,fake_tanpi(a_),"tanpi");			\
	REFTEST(a_,go_cotpi,fake_cotpi(a_),"cotpi");			\
	REFTEST(a_,go_atanpi,fake_atanpi(a_),"atanpi");			\
									\
	REFTESTl(a_,go_sinpil,fake_sinpi(a_),"sinpil");			\
	REFTESTl(a_,go_cospil,fake_cospi(a_),"cospil");			\
	REFTESTl(a_,go_tanpil,fake_tanpi(a_),"tanpil");			\
	REFTESTl(a_,go_cotpil,fake_cotpi(a_),"cotpil");			\
	REFTESTl(a_,go_atanpil,fake_atanpi(a_),"atanpil");		\
} while (0)

#define TEST2(a_,b_) do {						\
		REFTEST2(a_,b_,go_atan2pi,fake_atan2pi(a_,b_),"atan2pi"); \
		REFTEST2l(a_,b_,go_atan2pil,fake_atan2pi(a_,b_),"atan2pil"); \
} while (0)

static void
trig_tests (void)
{
	double d;

	for (d = 0; d < 5; d += 0.125) {
		TEST1(d);
		TEST1(-d);
	}

	TEST2 (0, +2);
	TEST2 (0, -2);
	TEST2 (3, 0);
	TEST2 (-3, 0);
	TEST2 (0, 0);
	TEST2 (+1, +1);
	TEST2 (-1, +1);
	TEST2 (+1, -1);
	TEST2 (-1, -1);
	TEST2 (+2.3, +1.2);
	TEST2 (+2.3, -1.2);
	TEST2 (+2.3, +0.2);
	TEST2 (+2.3, -0.2);
	TEST2 (+0.2, +2.3);
	TEST2 (+0.2, -2.3);
	TEST2 (+2.3, +3);
	TEST2 (+2.3, -3);
	TEST2 (-2.3, +3);
	TEST2 (-2.3, -3);
	TEST2 (+2.3, +4);
	TEST2 (+2.3, -4);
	TEST2 (-2.3, +4);
	TEST2 (-2.3, -4);
	TEST2 (2, 100);
	TEST2 (2, -100);
	TEST2 (1.0/256, 1.0/1024);
	TEST2 (exp(1), 1);
	TEST2 (exp(1), -1);
	TEST2 (exp(1), log(2));
	TEST2 (exp(1), -log(2));

	/* Since the results are n/4 these are tested exactly.  */
	REFTEST2(0,1,go_atan2pi,0,"atan2pi");
	REFTEST2(1,1,go_atan2pi,0.25,"atan2pi");
	REFTEST2(1,0,go_atan2pi,0.5,"atan2pi");
	REFTEST2(1,-1,go_atan2pi,0.75,"atan2pi");
	REFTEST2(0,-1,go_atan2pi,1,"atan2pi");
	REFTEST2(-1,1,go_atan2pi,-0.25,"atan2pi");
	REFTEST2(-1,0,go_atan2pi,-0.5,"atan2pi");
	REFTEST2(-1,-1,go_atan2pi,-0.75,"atan2pi");

	/* Since the results are n/4 these are tested exactly.  */
	REFTEST2l(0,1,go_atan2pil,0,"atan2pil");
	REFTEST2l(1,1,go_atan2pil,0.25,"atan2pil");
	REFTEST2l(1,0,go_atan2pil,0.5,"atan2pil");
	REFTEST2l(1,-1,go_atan2pil,0.75,"atan2pil");
	REFTEST2l(0,-1,go_atan2pil,1,"atan2pil");
	REFTEST2l(-1,1,go_atan2pil,-0.25,"atan2pil");
	REFTEST2l(-1,0,go_atan2pil,-0.5,"atan2pil");
	REFTEST2l(-1,-1,go_atan2pil,-0.75,"atan2pil");
}

#undef TEST1
#undef TEST2

/* ------------------------------------------------------------------------- */

static void
test_strto1 (const char *txt, double value, gboolean ascii, int n)
{
	{
		char *end = NULL;
		const char *func = ascii ? "go_ascii_strtod" : "go_strtod";
		double v = ascii
			? go_ascii_strtod (txt, &end)
			: go_strtod (txt, &end);
		int nactual = end ? end - txt : -1;
		g_printerr ("%s(\"%s\") = %g using %d chars\n", func, txt, v, nactual);
		if (nactual != n) {
			g_printerr ("Expected %d characters\n", n);
			abort ();
		}

		gboolean good = isnan (value) == isnan (v) && signbit (value) == signbit (v);
		if (good && !isnan (value) && value != v)
			good = fabs (v - value) / (fabs (value) + fabs (v)) < 1e-10;
		if (!good) {
			g_printerr ("Expected value %g\n", value);
			abort ();
		}
	}

#ifdef GOFFICE_WITH_LONG_DOUBLE
	{
		char *end = NULL;
		const char *func = ascii ? "go_ascii_strtold" : "go_strtold";
		long double v = ascii
			? go_ascii_strtold (txt, &end)
			: go_strtold (txt, &end);
		int nactual = end ? end - txt : -1;
		g_printerr ("%s(\"%s\") = %Lg using %d chars\n", func, txt, v, nactual);
		if (nactual != n) {
			g_printerr ("Expected %d characters\n", n);
			abort ();
		}

		/* Type-generic isnan() copes with the double "value" and
		 * the long double "v" alike.  */
		gboolean good = isnan (value) == isnan (v) && signbit (value) == signbit (v);
		if (good && !isnan (value) && (long double)value != v)
			good = fabsl (v - value) / (fabsl (value) + fabsl (v)) < 1e-10L;
		if (!good) {
			g_printerr ("Expected value %g\n", value);
			abort ();
		}
	}
#endif

#ifdef GOFFICE_WITH_DECIMAL64
	{
		_Decimal64 valueD = (_Decimal64)value;
		char *end = NULL;
		const char *func = ascii ? "go_ascii_strtoDd" : "go_strtoDd";
		_Decimal64 v = ascii
			? go_ascii_strtoDd (txt, &end)
			: go_strtoDd (txt, &end);
		int nactual = end ? end - txt : -1;
		g_printerr ("%s(\"%s\") = %Dg using %d chars\n", func, txt, v, nactual);
		if (nactual != n) {
			g_printerr ("Expected %d characters\n", n);
			abort ();
		}

		gboolean good = isnanD (valueD) == isnanD (v) && signbitD (valueD) == signbitD (v);
		if (good && !isnanD (valueD) && valueD != v)
			good = fabsD (v - valueD) / (fabsD (valueD) + fabsD (v)) < 1e-10dd;
		if (!good) {
			g_printerr ("Expected value %g\n", value);
			abort ();
		}
	}
#endif
}

static void
strto_tests (void)
{
	const GString *decimal = go_locale_get_decimal ();
	gboolean qdot = strcmp (decimal->str, ".") == 0;

	for (int ascii = 0; ascii <= (qdot ? 1 : 0); ascii++) {
		test_strto1 ("123.45", 123.45, ascii, 6);
		test_strto1 ("123.45e", 123.45, ascii, 6);
		test_strto1 ("123.45e+", 123.45, ascii, 6);
		test_strto1 ("123.45e+1", 1234.5, ascii, 9);
		test_strto1 ("1", 1, ascii, 1);
		test_strto1 ("1e", 1, ascii, 1);
		test_strto1 ("1e-", 1, ascii, 1);
		test_strto1 ("1e-0", 1, ascii, 4);
		test_strto1 ("1e-01", 0.1, ascii, 5);
		test_strto1 ("1d1", 1, ascii, 1);
		test_strto1 ("0x1p+0", 0, ascii, 1);
	}
}


/* ------------------------------------------------------------------------- */

static void
strto_errno_tests (void)
{
	char *end;

	/* Successful parses leave errno clear, per the documentation.  */
	errno = ERANGE;
	(void) go_ascii_strtod ("123.45", &end);
	g_assert (errno == 0);

	errno = ERANGE;
	(void) go_strtod ("123.45", &end);
	g_assert (errno == 0);

	errno = ERANGE;
	(void) go_ascii_strtod ("", &end);
	g_assert (errno == 0);

	/* The "d" slow path must clear and restore errno, too.  */
	errno = ERANGE;
	(void) go_ascii_strtod ("1d5", &end);
	g_assert (errno == 0);

#ifdef GOFFICE_WITH_LONG_DOUBLE
	errno = ERANGE;
	(void) go_ascii_strtold ("123.45", &end);
	g_assert (errno == 0);
#endif

	/* Real overflow is still reported.  */
	errno = 0;
	(void) go_ascii_strtod ("1e999", &end);
	g_assert (errno == ERANGE);
	errno = 0;
	(void) go_strtod ("1e999", &end);
	g_assert (errno == ERANGE);
}

/* ------------------------------------------------------------------------- */

static void
pseudo_inverse_case (int m, int n, const double *a, const double *want)
{
	double *A[3], *B[3];
	double Astore[3][3], Bstore[3][3];
	int i, j;

	g_assert (m <= 3 && n <= 3);
	for (i = 0; i < m; i++) {
		A[i] = Astore[i];
		for (j = 0; j < n; j++)
			A[i][j] = a[i * n + j];
	}
	for (i = 0; i < n; i++) {
		B[i] = Bstore[i];
		for (j = 0; j < m; j++)
			B[i][j] = go_nan;
	}

	go_matrix_pseudo_inverse (A, m, n, 0.0, B);

	for (i = 0; i < n; i++)
		for (j = 0; j < m; j++) {
			g_printerr ("pseudo_inverse(%dx%d)[%d][%d] = %g  [%g]\n",
				    m, n, i, j, B[i][j], want[i * m + j]);
			g_assert (fabs (B[i][j] - want[i * m + j]) < 1e-10);
		}
}

static void
pseudo_inverse_tests (void)
{
	/* Rank-deficient matrices with threshold 0: R^T R is singular, which
	 * used to make the inverse return NULL and the process crash.  */
	{
		static const double a[] = { 1, 0, 0, 0 };
		static const double w[] = { 1, 0, 0, 0 };
		pseudo_inverse_case (2, 2, a, w);
	}
	{
		static const double a[] = { 1, 0, 0, 0, 0, 0 };
		static const double w[] = { 1, 0, 0, 0, 0, 0 };
		pseudo_inverse_case (3, 2, a, w);
	}
	{
		static const double a[] = { 1, 0, 0, 0, 0, 0 };
		static const double w[] = { 1, 0, 0, 0, 0, 0 };
		pseudo_inverse_case (2, 3, a, w);
	}
	/* Full rank stays exact.  */
	{
		static const double a[] = { 3, 0, 0, 2 };
		static const double w[] = { 1.0 / 3, 0, 0, 0.5 };
		pseudo_inverse_case (2, 2, a, w);
	}
}

/* ------------------------------------------------------------------------- */

static int fft_criticals;

static void
count_fft_critical (G_GNUC_UNUSED const gchar *domain,
		    G_GNUC_UNUSED GLogLevelFlags level,
		    G_GNUC_UNUSED const gchar *message,
		    G_GNUC_UNUSED gpointer user_data)
{
	fft_criticals++;
}

static void
fft_invalid_n (int n)
{
	GOComplex in[16], *out = NULL;
	GLogFunc old_handler;
	GLogLevelFlags old_fatal;
	int old_criticals = fft_criticals;
	int i;

	for (i = 0; i < 16; i++) {
		in[i].re = i;
		in[i].im = 0;
	}

	old_fatal = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	old_handler = g_log_set_default_handler (count_fft_critical, NULL);
	go_fourier_fft (in, n, 1, &out, FALSE);
	g_log_set_default_handler (old_handler, NULL);
	g_log_set_always_fatal (old_fatal);

	g_assert (fft_criticals - old_criticals == 1);
	g_assert (out == NULL);
}

static void
fft_tests (void)
{
	GOComplex in[16], *out = NULL;
	int k, n, i, j;

	for (i = 0; i < 16; i++) {
		in[i].re = sin (i * 0.7) + 0.1 * i;
		in[i].im = cos (i * 1.3) - 0.05 * i;
	}

	/* The transform is a naive DFT scaled by 1/n.  */
	for (k = 0; k <= 4; k++) {
		n = 1 << k;
		go_fourier_fft (in, n, 1, &out, FALSE);
		g_assert (out != NULL);
		for (j = 0; j < n; j++) {
			double re = 0, im = 0;
			for (i = 0; i < n; i++) {
				double a = -2 * M_PI * j * i / n;
				re += in[i].re * cos (a) - in[i].im * sin (a);
				im += in[i].re * sin (a) + in[i].im * cos (a);
			}
			g_assert (fabs (out[j].re - re / n) < 1e-12);
			g_assert (fabs (out[j].im - im / n) < 1e-12);
		}
		g_free (out);
	}

	/* Likewise for the inverse transform.  */
	n = 8;
	go_fourier_fft (in, n, 1, &out, FALSE);
	{
		GOComplex *inv = NULL;
		go_fourier_fft (out, n, 1, &inv, TRUE);
		for (j = 0; j < n; j++) {
			double re = 0, im = 0;
			for (i = 0; i < n; i++) {
				double a = 2 * M_PI * j * i / n;
				re += out[i].re * cos (a) - out[i].im * sin (a);
				im += out[i].re * sin (a) + out[i].im * cos (a);
			}
			g_assert (fabs (inv[j].re - re / n) < 1e-12);
			g_assert (fabs (inv[j].im - im / n) < 1e-12);
		}
		g_free (inv);
	}
	g_free (out);

	/* n must be a positive power of two.  */
	fft_invalid_n (0);
	fft_invalid_n (-4);
	fft_invalid_n (6);
	fft_invalid_n (7);
}

/* ------------------------------------------------------------------------- */

static double
matrix3x3_error (const GOMatrix3x3 *a, const GOMatrix3x3 *b)
{
	double e = 0;
	e = fmax (e, fabs (a->a11 - b->a11));
	e = fmax (e, fabs (a->a12 - b->a12));
	e = fmax (e, fabs (a->a13 - b->a13));
	e = fmax (e, fabs (a->a21 - b->a21));
	e = fmax (e, fabs (a->a22 - b->a22));
	e = fmax (e, fabs (a->a23 - b->a23));
	e = fmax (e, fabs (a->a31 - b->a31));
	e = fmax (e, fabs (a->a32 - b->a32));
	e = fmax (e, fabs (a->a33 - b->a33));
	return e;
}

static void
euler_roundtrip (double psi, double theta, double phi)
{
	GOMatrix3x3 m, m2;
	double psi2, theta2, phi2, err;

	go_matrix3x3_from_euler (&m, psi, theta, phi);
	go_matrix3x3_to_euler (&m, &psi2, &theta2, &phi2);
	go_matrix3x3_from_euler (&m2, psi2, theta2, phi2);

	err = matrix3x3_error (&m, &m2);
	g_printerr ("euler (%.10g, %.10g, %.10g) -> (%.10g, %.10g, %.10g), err %.3g\n",
		    psi, theta, phi, psi2, theta2, phi2, err);
	g_assert (err < 1e-5);
}

static void
euler_tests (void)
{
	GOMatrix3x3 m;
	double psi, theta, phi;

	euler_roundtrip (0, 0, 0);
	euler_roundtrip (1, 2, 3);
	euler_roundtrip (-1, 0.5, 2);
	euler_roundtrip (M_PI / 2, 0, M_PI / 2);
	euler_roundtrip (0.3, M_PI, 0.7);
	euler_roundtrip (M_PI + 1e-6, 1, 0.2);
	euler_roundtrip (0.3, 1, M_PI + 1e-6);
	euler_roundtrip (3, 0, -2);

	/* At the Theta == pi gimbal the extraction must report pi, not
	 * 2*pi, and the Phi sign must flip with Theta.  */
	go_matrix3x3_from_euler (&m, 0.3, M_PI, 0.7);
	go_matrix3x3_to_euler (&m, &psi, &theta, &phi);
	g_printerr ("gimbal pi: (%.10g, %.10g, %.10g)  [0, pi, 0.4]\n",
		    psi, theta, phi);
	g_assert (psi == 0);
	g_assert (fabs (theta - M_PI) < 1e-12);
	g_assert (fabs (phi - 0.4) < 1e-9);
}

/* ------------------------------------------------------------------------- */

static double
cspline_x2 (double x)
{
	return x * x;
}

static double
cspline_x3 (double x)
{
	return x * x * x;
}

static double
cspline_dx2 (double x)
{
	return 2 * x;
}

static double
cspline_dx3 (double x)
{
	return 3 * x * x;
}

static void
cspline_exact (unsigned limits, int n,
	       double (*f) (double), double (*df) (double))
{
	double x[5], y[5], t;
	GOCSpline *sp;
	int i;

	g_assert (n <= 5);
	for (i = 0; i < n; i++) {
		x[i] = i;
		y[i] = f (i);
	}
	sp = go_cspline_init (x, y, n, limits,
			      df ? df (0) : 0, df ? df (n - 1) : 0);
	g_assert (sp != NULL);
	for (t = -1; t <= n; t += 0.25)
		g_assert (fabs (go_cspline_get_value (sp, t) - f (t)) < 1e-9);
	go_cspline_destroy (sp);
}

static void
cspline_tests (void)
{
	GOCSpline *c, *p;

	/* GO_CSPLINE_CUBIC with three points is treated as
	 * GO_CSPLINE_PARABOLIC: the unique parabola through them.  */
	{
		double x[] = { 0, 1, 2 }, y[] = { 0, 1, 4 };

		c = go_cspline_init (x, y, 3, GO_CSPLINE_CUBIC, 0, 0);
		p = go_cspline_init (x, y, 3, GO_CSPLINE_PARABOLIC, 0, 0);
		g_assert (c != NULL && p != NULL);
		g_assert (c->a[0] == p->a[0] && c->a[1] == p->a[1]);
		g_assert (c->b[0] == p->b[0] && c->b[1] == p->b[1]);
		g_assert (c->c[0] == p->c[0] && c->c[1] == p->c[1]);
		g_assert (fabs (go_cspline_get_value (c, -2) - 4) < 1e-12);
		g_assert (fabs (go_cspline_get_value (c, 3.5) - 12.25) < 1e-12);
		go_cspline_destroy (c);
		go_cspline_destroy (p);
	}

	/* With four or more points the limit types still reproduce
	 * polynomials exactly as expected.  */
	cspline_exact (GO_CSPLINE_PARABOLIC, 5, cspline_x2, NULL);
	cspline_exact (GO_CSPLINE_CUBIC, 4, cspline_x2, NULL);
	cspline_exact (GO_CSPLINE_CUBIC, 4, cspline_x3, NULL);
	cspline_exact (GO_CSPLINE_CUBIC, 5, cspline_x3, NULL);
	cspline_exact (GO_CSPLINE_CLAMPED, 5, cspline_x2, cspline_dx2);
	cspline_exact (GO_CSPLINE_CLAMPED, 4, cspline_x3, cspline_dx3);

	/* Array interfaces and integrals of x^2 on [0,4].  */
	{
		double x[] = { 0, 1, 2, 3, 4 }, y[5], q[5], *vals, *ders, *ints;
		int i;
		GOCSpline *sp;

		for (i = 0; i < 5; i++) {
			y[i] = (double) i * i;
			q[i] = i + 0.5;
		}
		sp = go_cspline_init (x, y, 5, GO_CSPLINE_CUBIC, 0, 0);

		vals = go_cspline_get_values (sp, q, 5);
		ders = go_cspline_get_derivs (sp, q, 5);
		g_assert (vals != NULL && ders != NULL);
		for (i = 0; i < 5; i++) {
			g_assert (fabs (vals[i] - q[i] * q[i]) < 1e-12);
			g_assert (fabs (ders[i] - 2 * q[i]) < 1e-12);
		}
		g_free (vals);
		g_free (ders);

		g_assert (fabs (go_cspline_get_deriv (sp, 1.5) - 3) < 1e-12);

		/* Each integral covers one pair of input x values.  */
		ints = go_cspline_get_integrals (sp, x, 5);
		g_assert (ints != NULL);
		for (i = 0; i < 4; i++) {
			double want;
			double b = i + 1;
			want = (b * b * b - (double) i * i * i) / 3.0;
			g_assert (fabs (ints[i] - want) < 1e-12);
		}
		g_free (ints);

		/* n <= 1 and non-increasing input are rejected.  */
		g_assert (go_cspline_get_integrals (sp, x, 1) == NULL);
		{
			double bad[] = { 1, 0 };
			g_assert (go_cspline_get_integrals (sp, bad, 2) == NULL);
		}

		go_cspline_destroy (sp);
	}
}

/* ------------------------------------------------------------------------- */

static void
range_devsq_tests (void)
{
	double xs[] = { 3, 1, 2 };
	double res;

	/* Compensated mean; the redundant sum call is gone.  */
	g_assert (go_range_devsq (xs, 3, &res) == 0);
	g_printerr ("devsq(3,1,2) = %g  [2]\n", res);
	g_assert (fabs (res - 2) < 1e-15);

	/* Near-cancellation: the mean must still be good enough.  */
	{
		double big[] = { 1e15 + 1, 1e15 + 2, 1e15 + 3 };
		g_assert (go_range_devsq (big, 3, &res) == 0);
		g_printerr ("devsq(1e15+1..3) = %g  [2]\n", res);
		g_assert (fabs (res - 2) < 1e-9);
	}

	/* A single value is constant.  */
	{
		double one[] = { 42 };
		g_assert (go_range_devsq (one, 1, &res) == 0);
		g_assert (res == 0);
	}

	/* An empty input gives 0, not an error.  */
	res = go_nan;
	g_assert (go_range_devsq (xs, 0, &res) == 0);
	g_assert (res == 0);
	res = go_nan;
	g_assert (go_range_devsq (xs, -1, &res) == 0);
	g_assert (res == 0);
}

/* ------------------------------------------------------------------------- */

static void
range_fractile_tests (void)
{
	double xs[] = { 3, 1, 2 };
	double res;

	/* A NaN fraction used to be undefined / out-of-bounds.  */
	g_assert (go_range_fractile_inter (xs, 3, &res, go_nan) != 0);
	g_assert (go_range_fractile_inter_nonconst (xs, 3, &res, go_nan) != 0);
	g_assert (go_range_fractile_inter_sorted (xs, 3, &res, go_nan) != 0);

	/* Negative n used to qsort with a huge size.  */
	g_assert (go_range_fractile_inter_nonconst (xs, -1, &res, 0.5) != 0);
	g_assert (go_range_fractile_inter_nonconst (xs, 0, &res, 0.5) != 0);

	/* Ordinary use is unaffected.  */
	g_assert (go_range_fractile_inter (xs, 3, &res, 0.5) == 0);
	g_assert (res == 2);
	g_assert (go_range_fractile_inter_nonconst (xs, 3, &res, 0.5) == 0);
	g_assert (res == 2);
	g_assert (go_range_median_inter (xs, 3, &res) == 0);
	g_assert (res == 2);
}

static void
distribution_type_tests (void)
{
	int i;

	for (i = 0; i < GO_DISTRIBUTION_MAX; i++) {
		const char *name = go_distribution_type_to_string ((GODistributionType) i);
		g_assert (name != NULL);
		g_assert (go_distribution_type_from_string (name) == i);
	}
	g_assert (go_distribution_type_to_string (GO_DISTRIBUTION_INVALID) == NULL);
	g_assert (go_distribution_type_to_string (GO_DISTRIBUTION_MAX) == NULL);
	g_assert (go_distribution_type_to_string ((GODistributionType) 12345) == NULL);
}

/* ------------------------------------------------------------------------- */

static void
basic_math_tests (void)
{
	int n;

	/* Classification.  */
	g_assert (go_finite (0) && go_finite (1e308) && go_finite (-1e308));
	g_assert (!go_finite (go_nan));
	g_assert (!go_finite (go_pinf) && !go_finite (go_ninf));

	/* Epsilon helpers move by one ulp; zero and infinities are fixed.  */
	g_assert (go_add_epsilon (0) == 0);
	g_assert (go_add_epsilon (go_pinf) == go_pinf);
	g_assert (go_add_epsilon (go_ninf) == go_ninf);
	g_assert (go_sub_epsilon (0) == 0);
	g_assert (go_sub_epsilon (go_pinf) == go_pinf);
	g_assert (go_sub_epsilon (go_ninf) == go_ninf);
	g_assert (go_add_epsilon (1) > 1);
	g_assert (go_sub_epsilon (1) < 1);
	g_assert (go_sub_epsilon (go_add_epsilon (1)) == 1);
	g_assert (go_add_epsilon (go_sub_epsilon (1)) == 1);
	g_assert (go_add_epsilon (-1) > -1);
	g_assert (go_sub_epsilon (-1) < -1);

	/* Powers of two and ten.  */
	g_assert (go_pow2 (0) == 1);
	g_assert (go_pow2 (10) == 1024);
	for (n = -1074; n <= 1023; n++)
		g_assert (go_pow2 (n) == ldexp (1.0, n));
	for (n = -323; n <= 308; n++) {
		char buf[16];
		snprintf (buf, sizeof buf, "1e%d", n);
		g_assert (go_pow10 (n) == go_ascii_strtod (buf, NULL));
	}
	g_assert (go_pow10 (309) == go_pinf);
	g_assert (go_pow10 (-324) == 0);

	/* go_pow is pow with extra care for base 10.  */
	g_assert (go_pow (10, 3) == 1000);
	g_assert (go_pow (10, -3) == 0.001);
	g_assert (go_pow (10, 308) == go_pow10 (308));
	g_assert (go_pow (2, 10) == 1024);
	g_assert (fabs (go_pow (2, 0.5) - M_SQRT2) < 1e-15);

	/* go_rint.  */
#ifdef HAVE_RINT
	g_assert (go_rint (2.5) == 2);
	g_assert (go_rint (3.5) == 4);
	g_assert (go_rint (-2.5) == -2);
	{
		double d;
		for (d = -10; d <= 10; d += 0.25)
			g_assert (go_rint (d) == rint (d));
	}
#else
	g_assert (go_rint (2.5) == 3);
	g_assert (go_rint (-2.5) == -3);
#endif

	/* The "fake" rounding variants allow a one-ulp grace interval.  */
	g_assert (go_fake_floor (go_sub_epsilon (1)) == 1);
	g_assert (go_fake_floor (0.5) == 0);
	g_assert (go_fake_ceil (go_add_epsilon (1)) == 1);
	g_assert (go_fake_ceil (0.5) == 1);
	g_assert (go_fake_round (go_sub_epsilon (0.5)) == 1);
	g_assert (go_fake_round (2.5) == 3);
	g_assert (go_fake_round (-2.5) == -3);
	g_assert (go_fake_trunc (go_sub_epsilon (2)) == 2);
	g_assert (go_fake_trunc (-go_add_epsilon (2)) == -2);
	g_assert (go_fake_trunc (1.75) == 1);
	g_assert (go_fake_trunc (-1.75) == -1);

	/* Range reduction for the trigonometric functions.  */
	{
		int k;
		double xr;

		xr = go_reduce_pi (0.3, 0, &k);
		g_assert (fabs (xr - 0.3) < 1e-15 && k == 0);

		xr = go_reduce_pi (M_PI + 0.1, 0, &k);
		g_assert (fabs (xr - 0.1) < 1e-14 && k == 1);

		xr = go_reduce_pi (2 * M_PI + 0.2, 0, &k);
		g_assert (fabs (xr - 0.2) < 1e-14 && k == 0);

		xr = go_reduce_pi (-0.3, 0, &k);
		g_assert (fabs (xr + 0.3) < 1e-15 && k == 0);

		xr = go_reduce_pi (go_pinf, 0, &k);
		g_assert (isnan (xr) && k == 0);
	}

	/* Continued fractions and Stern-Brocot trees.  */
	{
		int num, den;

		go_continued_fraction (0.5, 10, &num, &den);
		g_assert (num == 1 && den == 2);
		go_continued_fraction (-0.75, 4, &num, &den);
		g_assert (num == -3 && den == 4);
		go_continued_fraction (2.25, 100, &num, &den);
		g_assert (num == 9 && den == 4);
		go_continued_fraction (M_PI, 100, &num, &den);
		g_assert (num == 22 && den == 7);
		go_continued_fraction (12345, 100, &num, &den);
		g_assert (num == 12345 && den == 1);

		go_stern_brocot (0, 4, &num, &den);
		g_assert (num == 0 && den == 1);
		go_stern_brocot (0.25, 4, &num, &den);
		g_assert (num == 1 && den == 4);
		go_stern_brocot (0.75, 4, &num, &den);
		g_assert (num == 3 && den == 4);
		go_stern_brocot (0.9, 10, &num, &den);
		g_assert (num == 9 && den == 10);
		go_stern_brocot (1, 10, &num, &den);
		g_assert (num == 1 && den == 1);
	}
}

/* ------------------------------------------------------------------------- */

static void
range_basic_tests (void)
{
	double xs[] = { 3, 1, 2 };
	double res;

	/* Sums and extremes.  */
	g_assert (go_range_sum (xs, 3, &res) == 0 && res == 6);
	g_assert (go_range_sumsq (xs, 3, &res) == 0 && res == 14);
	g_assert (go_range_average (xs, 3, &res) == 0 && res == 2);
	g_assert (go_range_min (xs, 3, &res) == 0 && res == 1);
	g_assert (go_range_max (xs, 3, &res) == 0 && res == 3);
	g_assert (go_range_maxabs (xs, 3, &res) == 0 && res == 3);

	{
		double neg[] = { -3, 1, -2 };
		g_assert (go_range_min (neg, 3, &res) == 0 && res == -3);
		g_assert (go_range_max (neg, 3, &res) == 0 && res == 1);
		g_assert (go_range_maxabs (neg, 3, &res) == 0 && res == 3);
	}

	/* Empty ranges.  */
	res = go_nan;
	g_assert (go_range_sum (xs, 0, &res) == 0 && res == 0);
	g_assert (go_range_sumsq (xs, 0, &res) == 0 && res == 0);
	g_assert (go_range_average (xs, 0, &res) != 0);
	g_assert (go_range_min (xs, 0, &res) != 0);
	g_assert (go_range_max (xs, 0, &res) != 0);
	g_assert (go_range_maxabs (xs, 0, &res) != 0);

	/* Shape predicates.  */
	{
		double same[] = { 2, 2, 2 };
		double inc[] = { 1, 2, 3 };
		double flat[] = { 1, 2, 2 };
		double dec[] = { 3, 2, 1 };
		double mixed[] = { 1, 3, 2 };
		double nanv[] = { go_nan, 2, 3 };

		g_assert (go_range_constant (same, 3) == 1);
		g_assert (go_range_constant (flat, 3) == 0);
		g_assert (go_range_constant (xs, 0) == 1);
		g_assert (go_range_constant (nanv, 3) == 0);

		g_assert (go_range_increasing (inc, 3) == 1);
		g_assert (go_range_increasing (flat, 3) == 0);
		g_assert (go_range_increasing (dec, 3) == 0);
		g_assert (go_range_increasing (xs, 0) == 0);
		g_assert (go_range_increasing (nanv, 3) == 1);

		g_assert (go_range_decreasing (dec, 3) == 1);
		g_assert (go_range_decreasing (flat, 3) == 0);
		g_assert (go_range_decreasing (inc, 3) == 0);

		g_assert (go_range_vary_uniformly (inc, 3) == 1);
		g_assert (go_range_vary_uniformly (dec, 3) == 1);
		g_assert (go_range_vary_uniformly (mixed, 3) == 0);
	}

	/* Sorting makes a sorted copy and leaves the input alone.  */
	{
		double src[] = { 3, 1, 2 };
		double *ys = go_range_sort (src, 3);
		g_assert (ys != NULL);
		g_assert (ys[0] == 1 && ys[1] == 2 && ys[2] == 3);
		g_assert (src[0] == 3 && src[1] == 1 && src[2] == 2);
		g_free (ys);
		g_assert (go_range_sort (src, 0) == NULL);
		g_assert (go_range_sort (src, -1) == NULL);
	}

	/* Interpolated fractiles.  */
	{
		double one[] = { 7 };
		double two[] = { 2, 4 };
		double four[] = { 4, 1, 3, 2 };

		g_assert (go_range_fractile_inter (xs, 3, &res, 0) == 0 && res == 1);
		g_assert (go_range_fractile_inter (xs, 3, &res, 0.25) == 0 && res == 1.5);
		g_assert (go_range_fractile_inter (xs, 3, &res, 0.75) == 0 && res == 2.5);
		g_assert (go_range_fractile_inter (xs, 3, &res, 1) == 0 && res == 3);
		g_assert (go_range_fractile_inter (xs, 3, &res, 1.5) != 0);
		g_assert (go_range_fractile_inter (xs, 3, &res, -0.5) != 0);
		g_assert (go_range_fractile_inter (one, 1, &res, 0.25) == 0 && res == 7);
		g_assert (go_range_fractile_inter (two, 2, &res, 0.25) == 0 && res == 2.5);
		g_assert (go_range_median_inter (four, 4, &res) == 0 && res == 2.5);
	}
}

/* ------------------------------------------------------------------------- */

static void
accumulator_tests (void)
{
	GOAccumulator *acc;
	GOQuad q;
	void *state;
	unsigned i;

	/* A naive sum of ten copies of 0.1 misses the exact 1.  */
	acc = go_accumulator_new ();
	state = go_accumulator_start ();
	for (i = 0; i < 10; i++)
		go_accumulator_add (acc, 0.1);
	g_assert (go_accumulator_value (acc) == 1);

	go_accumulator_clear (acc);
	g_assert (go_accumulator_value (acc) == 0);

	go_quad_init (&q, 1e100);
	go_accumulator_add_quad (acc, &q);
	go_quad_init (&q, 1);
	go_accumulator_add_quad (acc, &q);
	go_quad_init (&q, -1e100);
	go_accumulator_add_quad (acc, &q);
	g_assert (go_accumulator_value (acc) == 1);

	go_accumulator_free (acc);
	go_accumulator_end (state);
}

/* ------------------------------------------------------------------------- */

static void
distribution_tests (void)
{
	int t;

	/* Generic identities hold for every distribution.  */
	for (t = 0; t < GO_DISTRIBUTION_MAX; t++) {
		GODistribution *dist = go_distribution_new ((GODistributionType) t);
		double x, dens, cum, surv, hz, ch;

		g_assert (dist != NULL);
		g_assert (go_distribution_get_distribution_type (dist) == t);
		g_assert (go_distribution_get_distribution_name (dist) != NULL);

		go_distribution_scale (dist, 0, 1);

		switch (t) {
		case GO_DISTRIBUTION_UNIFORM:	x = 0.5; break;
		case GO_DISTRIBUTION_LOGNORMAL:	x = 2; break;
		default:			x = 1; break;
		}

		dens = go_distribution_get_density (dist, x);
		cum = go_distribution_get_cumulative (dist, x);
		surv = go_distribution_get_survival (dist, x);
		hz = go_distribution_get_hazard (dist, x);
		ch = go_distribution_get_cumulative_hazard (dist, x);

		g_assert (fabs (surv - (1 - cum)) < 1e-12);
		g_assert (fabs (ch + log (surv)) < 1e-12);
		g_assert (fabs (hz - dens / surv) < 1e-12 * (1 + fabs (hz)));
		g_assert (fabs (go_distribution_get_inverse_survival (dist, 0.25) -
				go_distribution_get_ppf (dist, 0.75)) < 1e-12);
		g_assert (fabs (go_distribution_get_ppf (dist, cum) - x) <
			  1e-9 * (1 + fabs (x)));

		g_object_unref (dist);
	}

	/* Closed forms.  */
	{
		GODistribution *dist;

		dist = go_distribution_new (GO_DISTRIBUTION_NORMAL);
		go_distribution_scale (dist, 0, 1);
		g_assert (fabs (go_distribution_get_density (dist, 0) -
				1 / sqrt (2 * M_PI)) < 1e-15);
		g_assert (fabs (go_distribution_get_cumulative (dist, 0) - 0.5) < 1e-15);
		g_assert (fabs (go_distribution_get_ppf (dist, 0.5)) < 1e-15);
		g_object_unref (dist);

		dist = go_distribution_new (GO_DISTRIBUTION_UNIFORM);
		go_distribution_scale (dist, 10, 4);
		g_assert (go_distribution_get_density (dist, 11) == 0.25);
		g_assert (go_distribution_get_density (dist, 9) == 0);
		g_assert (go_distribution_get_density (dist, 14) == 0);
		g_assert (go_distribution_get_cumulative (dist, 9) == 0);
		g_assert (go_distribution_get_cumulative (dist, 12) == 0.5);
		g_assert (go_distribution_get_cumulative (dist, 15) == 1);
		g_assert (go_distribution_get_ppf (dist, 0.25) == 11);
		g_assert (go_distribution_get_survival (dist, 12) == 0.5);
		g_assert (go_distribution_get_inverse_survival (dist, 0.25) == 13);
		g_object_unref (dist);

		dist = go_distribution_new (GO_DISTRIBUTION_CAUCHY);
		go_distribution_scale (dist, 0, 1);
		g_assert (fabs (go_distribution_get_density (dist, 0) -
				1 / M_PI) < 1e-15);
		g_assert (fabs (go_distribution_get_cumulative (dist, 0) - 0.5) < 1e-15);
		g_assert (fabs (go_distribution_get_ppf (dist, 0.5)) < 1e-15);
		g_object_unref (dist);

		dist = go_distribution_new (GO_DISTRIBUTION_LOGNORMAL);
		go_distribution_scale (dist, 0, 1);
		g_assert (fabs (go_distribution_get_ppf (dist, 0.5) - 1) < 1e-15);
		g_object_unref (dist);
	}

	g_assert (go_distribution_new (GO_DISTRIBUTION_INVALID) == NULL);
}

/* ------------------------------------------------------------------------- */

static void
matrix3x3_tests (void)
{
	GOMatrix3x3 id, m, mt, p, q;
	double x, y, z;

	go_matrix3x3_from_euler (&id, 0, 0, 0);
	g_assert (id.a11 == 1 && id.a22 == 1 && id.a33 == 1);
	g_assert (id.a12 == 0 && id.a13 == 0 && id.a21 == 0);
	g_assert (id.a23 == 0 && id.a31 == 0 && id.a32 == 0);

	/* The transform is mat * (x,y,z).  */
	{
		GOMatrix3x3 t = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
		x = 1; y = 2; z = 3;
		go_matrix3x3_transform (&t, x, y, z, &x, &y, &z);
		g_assert (x == 1 * 1 + 2 * 2 + 3 * 3);
		g_assert (y == 4 * 1 + 5 * 2 + 6 * 3);
		g_assert (z == 7 * 1 + 8 * 2 + 9 * 3);
	}

	/* Identity is a no-op transform.  */
	x = 1; y = 2; z = 3;
	go_matrix3x3_transform (&id, x, y, z, &x, &y, &z);
	g_assert (x == 1 && y == 2 && z == 3);

	/* The transposed Euler matrix is the transpose.  */
	go_matrix3x3_from_euler (&m, 0.3, 0.4, 0.5);
	go_matrix3x3_from_euler_transposed (&mt, 0.3, 0.4, 0.5);
	g_assert (fabs (m.a11 - mt.a11) < 1e-15);
	g_assert (fabs (m.a12 - mt.a21) < 1e-15);
	g_assert (fabs (m.a13 - mt.a31) < 1e-15);
	g_assert (fabs (m.a21 - mt.a12) < 1e-15);
	g_assert (fabs (m.a22 - mt.a22) < 1e-15);
	g_assert (fabs (m.a23 - mt.a32) < 1e-15);
	g_assert (fabs (m.a31 - mt.a13) < 1e-15);
	g_assert (fabs (m.a32 - mt.a23) < 1e-15);
	g_assert (fabs (m.a33 - mt.a33) < 1e-15);

	/* ...so m * m^T is the identity.  */
	go_matrix3x3_multiply (&p, &m, &mt);
	g_assert (fabs (p.a11 - 1) < 1e-14 && fabs (p.a12) < 1e-14 && fabs (p.a13) < 1e-14);
	g_assert (fabs (p.a21) < 1e-14 && fabs (p.a22 - 1) < 1e-14 && fabs (p.a23) < 1e-14);
	g_assert (fabs (p.a31) < 1e-14 && fabs (p.a32) < 1e-14 && fabs (p.a33 - 1) < 1e-14);

	/* Multiplication order: (m * n) v == m (n v).  */
	{
		GOMatrix3x3 n, mn;
		double ax, ay, az, bx, by, bz;

		go_matrix3x3_from_euler (&n, 0.7, -0.2, 1.1);
		go_matrix3x3_multiply (&mn, &m, &n);
		x = 2; y = -1; z = 0.5;
		go_matrix3x3_transform (&n, x, y, z, &ax, &ay, &az);
		go_matrix3x3_transform (&m, ax, ay, az, &bx, &by, &bz);
		go_matrix3x3_transform (&mn, x, y, z, &ax, &ay, &az);
		g_assert (fabs (ax - bx) < 1e-14 && fabs (ay - by) < 1e-14 &&
			  fabs (az - bz) < 1e-14);
	}

	/* Multiplication by the identity changes nothing.  */
	go_matrix3x3_multiply (&q, &m, &id);
	g_assert (matrix3x3_error (&q, &m) < 1e-15);
	go_matrix3x3_multiply (&q, &id, &m);
	g_assert (matrix3x3_error (&q, &m) < 1e-15);
}

/* ------------------------------------------------------------------------- */

int
main (G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv)
{
	libgoffice_init ();

	basic_math_tests ();
	trig_tests ();
	strto_tests ();
	strto_errno_tests ();
	fft_tests ();
	pseudo_inverse_tests ();
	euler_tests ();
	matrix3x3_tests ();
	cspline_tests ();
	range_basic_tests ();
	range_fractile_tests ();
	range_devsq_tests ();
	accumulator_tests ();
	distribution_tests ();
	distribution_type_tests ();

	libgoffice_shutdown ();

	return 0;
}
