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
decimal_eq (double x, double y)
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
		bad ();
		return 0;
	}
}

// Standard set of (positive) values to test.  Specific function might
// need more.
static _Decimal64 values64[] = {
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
static const int nvalues64 = G_N_ELEMENTS (values64);


static void
test1rounding (_Decimal64 x)
{
	_Decimal64 f = floorD (x);
	_Decimal64 c = ceilD (x);
	_Decimal64 r = roundD (x);
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

		g_printerr ("Error: 0x%08lx: %.16Wg -> (%.16Wg , %.16Wg , %.16Wg)\n", d64, x, f, r, c);
		bad ();
	}
}

static void
test_rounding (void)
{
	start_section ("rounding operations (floor, ceil, round)");

	for (int v = 0; v < nvalues64; v++) {
		_Decimal64 x = values64[v];
		test1rounding (x);
		test1rounding (-x);
	}

	end_section ();
}

static void
test1prop (_Decimal64 x)
{
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

static void
test_properties (void)
{
	start_section ("properties (isnan, finite, signbit)");

	for (int v = 0; v < nvalues64; v++) {
		_Decimal64 x = values64[v];
		test1prop (x);
		test1prop (-x);
	}

	end_section ();
}

static void
test_copysign (void)
{
	start_section ("copysign");

	for (int v1 = 0; v1 < nvalues64; v1++) {
		for (int s1 = 0; s1 <= 1; s1++) {
			_Decimal64 x1 = values64[v1];
			if (s1) x1 = -x1;
			for (int v2 = 0; v2 < nvalues64; v2++) {
				for (int s2 = 0; s2 <= 1; s2++) {
					_Decimal64 x2 = values64[v2];
					if (s2) x2 = -x2;

					test_eq (copysignD (x1, x2),
						 copysign (x1, x2));
				}
			}
		}
	}

	end_section ();
}

static void
test_oneargs (void)
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
	};

	start_section ("one-arg functions");

	for (int f = 0; f < (int)G_N_ELEMENTS (funcs); f++) {
		g_printerr ("  testing %s\n", funcs[f].name);
		for (int v = 0; v < nvalues64; v++) {
			for (int s = 0; s <= 1; s++) {
				_Decimal64 x = values64[v], y;
				if (s) x = -x;
				double dx = x, dy;
				int ok;

				y = funcs[f].fn_decimal (x);
				dy = funcs[f].fn_decimal (dx);

				ok = (!!finiteD (y) == !!finite (dy) &&
				      !!isnanD (y) == !!isnan (dy) &&
				      !!signbitD (y) == !!signbit (dy) &&
				      (y == 0) == (dy == 0));

				if (ok && y) {
					_Decimal64 d = y - (_Decimal64)dy;
					ok = fabsD (d / y) < 1e-10dd;
				}

				if (ok)
					good ();
				else
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
	libgoffice_init ();

#ifdef GOFFICE_WITH_DECIMAL64
	test_rounding ();
	test_properties ();
	test_copysign ();
	test_oneargs ();
#else
	g_printerr ("Not compiled with Decimal64 support, so no testing.\n");
#endif

	libgoffice_shutdown ();

	return n_bad ? 1 : 0;
}
