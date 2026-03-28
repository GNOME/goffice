#include <goffice/goffice.h>

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

#define REFTESTl(a_,f_,r_, txt_)					\
	do {								\
		long double a = (a_);					\
		long double r = (r_);					\
		long double fa = f_(a);					\
		g_printerr ("%s(%Lg) = %Lg  [%Lg]\n",			\
			    txt_, a, fa, r);				\
		g_assert (copysignl (1,r) == copysignl (1,fa));		\
		if (isnanl (r))						\
			g_assert (isnanl (fa));				\
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

		gboolean good = isnanl (value) == isnanl (v) && signbit (value) == signbit (v);
		if (good && !isnanl (value) && (long double)value != v)
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
		g_printerr ("%s(\"%s\") = %Wg using %d chars\n", func, txt, v, nactual);
		if (nactual != n) {
			g_printerr ("Expected %d characters\n", n);
			abort ();
		}

		gboolean good = isnanD (valueD) == isnanD (v) && signbit (valueD) == signbit (v);
		if (good && !isnan (valueD) && valueD != v)
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

int
main (int argc, char **argv)
{
	libgoffice_init ();

	trig_tests ();
	strto_tests ();

	libgoffice_shutdown ();

	return 0;
}
