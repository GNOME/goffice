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


static void
test1rounding (_Decimal64 x)
{
	_Decimal64 f = floorD (x);
	_Decimal64 c = ceilD (x);
	_Decimal64 r = roundD (x);
	double dx = x;

	if (f <= x && x <= c &&
	    (c == f || c == f + 1) &&
	    (r == f || r == c) &&
	    r <= x + 0.5dd &&
	    r >= x - 0.5dd &&
	    double_eq (f, floor (dx)) &&
	    double_eq (r, round (dx)) &&
	    double_eq (c, ceil (dx))) {
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
	_Decimal64 values64[] = {
		0.dd, 3.14dd, 0.123dd, 0.05dd, 1.5dd, 0.567dd, 999999999.5dd,
		100.dd, 1e20dd, 0.01dd, 1e-20dd, 0.1dd,
		0.3333333333333333dd, 0.5555555555555555dd, 0.9999999999999999dd,
		INFINITY,
		1e15dd, 999999999999999.9dd, 999999999999999.0dd,
		1e15dd + 1, 1e22dd,
		2.5dd, 1.5dd, // make sure we don't round-ties-to-even
	};
	const int nvalues64 = G_N_ELEMENTS (values64);

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

	if (!!qnan == !!isnan (dx) &&
	    !!qfinite == !!finite (dx) &&
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
	_Decimal64 values64[] = {
		0.dd, 3.14dd, 0.123dd, 0.05dd, 1.5dd, 0.567dd, 999999999.5dd,
		100.dd, 1e20dd, 0.01dd, 1e-20dd, 0.1dd,
		0.3333333333333333dd, 0.5555555555555555dd, 0.9999999999999999dd,
		INFINITY, NAN,
		1e15dd, 999999999999999.9dd, 999999999999999.0dd,
		1e15dd + 1, 1e22dd,
	};
	const int nvalues64 = G_N_ELEMENTS (values64);

	start_section ("properties (isnan, finite, signbit)");

	for (int v = 0; v < nvalues64; v++) {
		_Decimal64 x = values64[v];
		test1prop (x);
		test1prop (-x);
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
#else
	g_printerr ("Not compiled with Decimal64 support, so no testing.\n");
#endif

	libgoffice_shutdown ();

	return n_bad ? 1 : 0;
}
