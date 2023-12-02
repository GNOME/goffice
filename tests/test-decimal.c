#include <goffice/goffice.h>

static int n_bad;

#ifdef GOFFICE_WITH_DECIMAL64

// There does not seem to be a way to teach these warnings about the
// "W" modifier than we have hooked into libc's printf.
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
/* ------------------------------------------------------------------------- */

static int n_section_good, n_section_bad;

static int
double_eq (double x, double y)
{
	return memcmp (&x, &y, sizeof(x)) == 0;
}

static int
decimal_eq (_Decimal64 x, _Decimal64 y)
{
	if (signbitD (x) != signbitD (y))
		return FALSE;

	if (isnanD (x) != isnanD (y))
		return FALSE;
	else if (isnanD (x))
		return TRUE;

	return x == y;
}

static void
start_section (const char *header)
{
	g_printerr ("-----------------------------------------------------------------------------\n");
	g_printerr ("Testing %s\n\n", header);

	n_section_good = n_section_bad = 0;
}

static void
end_section (void)
{
	if (n_section_bad)
		g_printerr ("\n");
	g_printerr ("For this section: good: %d, bad: %d\n\n",
		    n_section_good, n_section_bad);
}

static void
good (void)
{
	n_section_good++;
}

static void
bad (void)
{
	n_section_bad++;
	n_bad++;
}

static int
test_eq (_Decimal64 a, _Decimal64 b)
{
	if (decimal_eq (a, b)) {
		good ();
		return 1;
	} else {
		g_printerr ("%.16Wg vs %.16Wg\n", a, b);
		bad ();
		return 0;
	}
}

static _Decimal64 *
basic_corpus (int *n)
{
	static const _Decimal64 values64[] = {
		0.dd, 3.14dd, 0.123dd, 0.05dd, 1.5dd, 0.567dd, 999999999.5dd,
		100.dd, 1e20dd, 0.01dd, 1e-20dd, 0.1dd,
		0.3333333333333333dd, 0.5555555555555555dd, 0.9999999999999999dd,
		INFINITY, NAN,
		1e15dd, 999999999999999.9dd, 999999999999999.0dd,
		1e15dd + 1, 1e22dd,
		2.5dd, 1.5dd, // make sure we don't round-ties-to-even

		// Out of double range
		DECIMAL64_MAX,
		DECIMAL64_MIN,
	};
	_Decimal64 *res, *p;
	size_t i;

	*n = 2 * G_N_ELEMENTS (values64);
	res = g_new (_Decimal64, *n);

	for (i = 0, p = res; i < G_N_ELEMENTS (values64); i++) {
		*p++ = values64[i];
		*p++ = -values64[i];
	}

	return res;
}

static void
test_rounding (_Decimal64 const *corpus, int ncorpus)
{
	start_section ("rounding operations (floor, ceil, round, trunc)");

	for (int v = 0; v < ncorpus; v++) {
		_Decimal64 x = corpus[v];

		_Decimal64 f = floorD (x);
		_Decimal64 c = ceilD (x);
		_Decimal64 r = roundD (x);
		_Decimal64 t = truncD (x);
		double dx = x;
		int sanity;
		int qunderflow = (dx == 0) && (x != 0);
		int qoverflow = finiteD (x) && !finite (dx);
		int ok;

		sanity = isnanD (x)
			? 1
			: (f <= x && x <= c &&
			   (c == f || c == f + 1) &&
			   (r == f || r == c) &&
			   (x < 0 ? t == c : t == f) &&
			   r <= x + 0.5dd &&
			   r >= x - 0.5dd);
		if (qunderflow)
			ok = (r == 0 && (f == 0 || c == 0));
		else if (qoverflow)
			ok = (f == x && f == x);
		else
			ok = (double_eq (f, floor (dx)) &&
			      double_eq (r, round (dx)) &&
			      double_eq (c, ceil (dx)));

		if (ok && sanity) {
			good ();
		} else {
			uint64_t d64;
			memcpy (&d64, &x, sizeof (d64));

			g_printerr ("Error: 0x%08lx: %.16Wg -> (%.16Wg , %.16Wg , %.16Wg , %.16Wg)\n",
				    d64, x, f, r, c, t);
			bad ();
		}
	}

	end_section ();
}

static void
test_properties (_Decimal64 const *corpus, int ncorpus)
{
	start_section ("properties (isnan, finite, signbit)");

	for (int v = 0; v < ncorpus; v++) {
		_Decimal64 x = corpus[v];

		int qnan = isnanD (x);
		int qfinite = finiteD (x);
		int qsign = signbit (x);
		double dx = x;

		if (!!qnan == (x != x) &&
		    !!qfinite == (fabsD (x) <= DECIMAL64_MAX) &&
		    !!qsign == !!signbit (dx)) {
			good ();
		} else {
			uint64_t d64;
			memcpy (&d64, &x, sizeof (d64));

			g_printerr ("Error: 0x%08lx: %.16Wg -> %d %d %d\n",
				    d64, x, qnan, qfinite, qsign);
			bad ();
		}
	}

	end_section ();
}

static void
test_copysign (_Decimal64 const *corpus, int ncorpus)
{
	start_section ("copysign");

	for (int v1 = 0; v1 < ncorpus; v1++) {
		_Decimal64 x1 = corpus[v1];
		for (int v2 = 0; v2 < ncorpus; v2++) {
			_Decimal64 x2 = corpus[v2];

			_Decimal64 y = copysignD (x1, x2);
			if (decimal_eq (fabsD (y), fabsD (x1)) &&
			    signbitD (y) == signbitD (x2))
				good ();
			else {
				g_printerr ("Failed for %.16Wg  %.16Wg\n", x1, x2);
				bad ();
			}
		}
	}

	end_section ();
}


static void
test_nextafter (void)
{
	start_section ("nextafter");

	test_eq (nextafterD (0, 4), DECIMAL64_MIN);
	test_eq (nextafterD (nextafterD (0, +4), -4), 0.dd);
	test_eq (nextafterD (nextafterD (0, -4), +4), -0.dd);
	test_eq (nextafterD (1111111111111111.dd, INFINITY), 1111111111111112.dd);
	test_eq (nextafterD (-1111111111111111.dd, INFINITY), -1111111111111110.dd);
	test_eq (nextafterD (1000000000000000.dd, INFINITY), 1000000000000001.dd);
	test_eq (nextafterD (1000000000000000.dd, -INFINITY),999999999999999.9dd);
	test_eq (nextafterD (1000000000000000e-10dd, INFINITY), 1000000000000001e-10dd);
	test_eq (nextafterD (1000000000000000e-10dd, -INFINITY), 999999999999999.9e-10dd);
	test_eq (nextafterD (1000000000000000e+99dd, INFINITY), 1000000000000001e+99dd);
	test_eq (nextafterD (1000000000000000e+99dd, -INFINITY), 999999999999999.9e+99dd);
	test_eq (nextafterD (1000000000000001.dd, INFINITY), 1000000000000002.dd);
	test_eq (nextafterD (999999999999999.dd, +INFINITY), 999999999999999.1dd);
	test_eq (nextafterD (999999999999999.9dd, +INFINITY), 1000000000000000.dd);
	test_eq (nextafterD (INFINITY, INFINITY), INFINITY);
	test_eq (nextafterD (-INFINITY, -INFINITY), -INFINITY);
	test_eq (nextafterD (INFINITY, NAN), NAN);

	end_section ();
}

static void
test_modf (_Decimal64 const *corpus, int ncorpus)
{
	start_section ("modf");

	for (int v = 0; v < ncorpus; v++) {
		_Decimal64 x = corpus[v];
		_Decimal64 y, z;

		z = modfD (x, &y);

		test_eq (y, copysignD (truncD (x), x));
		test_eq (z, copysignD (fabsD (x) == go_pinfD ? 0 : x - truncD (x), x));
	}

	end_section ();
}


static void
test_oneargs (_Decimal64 const *corpus, int ncorpus)
{
	static const struct {
		const char *name;
		_Decimal64 (*fn_decimal) (_Decimal64);
		double (*fn_double) (double);
	} funcs[] = {
		{ "acosD", acosD, acos },
		{ "acoshD", acoshD, acosh },
		{ "asinD", asinD, asin },
		{ "asinhD", asinhD, asinh },
		{ "atanD", atanD, atan },
		{ "atanhD", atanhD, atanh },
		{ "cbrtD", cbrtD, cbrt },
		// { "ceilD", ceilD, ceil },
		{ "cosD", cosD, cos },
		{ "coshD", coshD, cosh },
		{ "erfD", erfD, erf },
		{ "erfcD", erfcD, erfc },
		{ "expD", expD, exp },
		{ "expm1D", expm1D, expm1 },
		{ "fabsD", fabsD, fabs },
		// { "floorD", floorD, floor },
		{ "lgammaD", lgammaD, lgamma },
		{ "log10D", log10D, log10 },
		{ "log1pD", log1pD, log1p },
		{ "log2D", log2D, log2 },
		{ "logD", logD, log },
		// { "roundD", roundD, round },
		{ "sinD", sinD, sin },
		{ "sinhD", sinhD, sinh },
		{ "sqrtD", sqrtD, sqrt },
		{ "tanD", tanD, tan },
		{ "tanhD", tanhD, tanh },
		// { "truncD", truncD, trunc },
	};

	start_section ("one-arg functions");

	for (int f = 0; f < (int)G_N_ELEMENTS (funcs); f++) {
		g_printerr ("  testing %s\n", funcs[f].name);
		for (int v = 0; v < ncorpus; v++) {
			_Decimal64 x = corpus[v], y;
			double dx = x, dy;
			int ok;
			int qunderflow = (dx == 0) && (x != 0);
			int qoverflow = finiteD (x) && !finite (dx);

			if (qunderflow || qoverflow)
				continue;

			y = funcs[f].fn_decimal (x);
			dy = funcs[f].fn_double (dx);

			//g_printerr ("%.16Wg  %.16Wg\n", x, y);
			//g_printerr ("%.16g  %.16g\n", dx, dy);

			ok = (!!finiteD (y) == !!finite (dy) &&
			      !!isnanD (y) == !!isnan (dy) &&
			      !!signbitD (y) == !!signbit (dy) &&
			      (y == 0) == (dy == 0));

			if (ok && finite (dy) && y != 0) {
				_Decimal64 d = y - (_Decimal64)dy;
				ok = fabsD (d / y) < 1e-10dd;
			}

			if (ok)
				good ();
			else {
				g_printerr ("Failed for %.16Wg\n", x);
				test_eq (y, dy);
			}
		}
	}

	end_section ();
}


/* ------------------------------------------------------------------------- */

#endif

int
main (int argc, char **argv)
{
#ifdef GOFFICE_WITH_DECIMAL64
	_Decimal64 *corpus;
	int ncorpus;

	libgoffice_init ();

	corpus = basic_corpus (&ncorpus);

	test_rounding (corpus, ncorpus);
	test_properties (corpus, ncorpus);
	test_copysign (corpus, ncorpus);
	test_nextafter ();
	test_oneargs (corpus, ncorpus);
	test_modf (corpus, ncorpus);

	if (n_bad)
		g_printerr ("A total of %d failures.\n", n_bad);
	else
		g_printerr ("Pass.\n");

	g_free (corpus);

	libgoffice_shutdown ();

#else
	g_printerr ("Not compiled with Decimal64 support, so no testing.\n");
#endif

	return n_bad ? 1 : 0;
}
