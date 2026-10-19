#include <goffice/goffice.h>
#include <float.h>
#include <math.h>

static int fail = 0;

static void
test1d_main (const char *fmt, double d, const char *expected, gboolean qfail)
{
	GString *res = g_string_new (NULL);

	if (strchr (fmt, '!')) {
		if (d != go_strtod (expected, NULL)) {
			g_printerr ("Sanity check failed for \"%s\"\n", expected);
			fail++;
		}
	}

	go_dtoa (res, fmt, d);
	if (g_str_equal (res->str, expected)) {
		if (qfail)
			g_printerr ("Unexpected success for %a \"\%s\"\n",
				    d, fmt);
	} else {
		g_printerr ("Failed for %a \"%s\" (got \"%s\", expected \"%s\")%s\n",
			    d, fmt, res->str, expected,
			    qfail ? " (expected)" : "");
		if (!qfail)
			fail++;
	}
	g_string_free (res, TRUE);
}

static void
test1d (const char *fmt, double d, const char *expected)
{
	test1d_main (fmt, d, expected, FALSE);
}

static void
test1d_fail (const char *fmt, double d, const char *expected)
{
	test1d_main (fmt, d, expected, TRUE);
}

// Check that go_dtoa correctly identifies the string
// This isn't likely to work for denormals.
static void
test1d_shortest (double d)
{
	int e;
	gboolean qpow2 = fabs (frexp (d, &e)) == 0.5;
	char *candidate;
	GString *res;

	for (int dig = 2; dig <= 17; dig++) {
		char *epos;

		candidate = g_strdup_printf ("%.*g", dig, d);
		epos = strchr (candidate, 'e');
		if (epos && fabs (d) > 1e-4 && fabs (d) < 1e17) {
			// Reject candidate -- we don't want e-notation
			g_free (candidate);
			continue;
		}

		if (0) {
			// Get rid of extra 0 in exponent
			if (epos && (epos[1] == '-' || epos[1] == '+') &&
			    epos[2] == '0' && epos[3]) {
				memmove (epos + 2, epos + 3, strlen (epos + 2));
			}

			// Get rid of '+' in exponent
			if (epos && epos[1] == '+')
				memmove (epos + 1, epos + 2, strlen (epos + 1));
		}

		if (go_strtod (candidate, NULL) == d)
			break;

		if (qpow2 && strchr (candidate, '.')) {
			// Try adding one to last digit
			char *p = strchr (candidate, 'e');
			if (!p) p = strchr (candidate, 0);
			while (p[-1] == '9') {
				p[-1] = '0';
				p--;
			}
			p[-1]++;
			if (go_strtod (candidate, NULL) == d)
				break;
		}
		g_free (candidate);
		candidate = NULL;
	}

	if (!candidate) {
		g_printerr ("Unexpected lack of round-trip for %a\n", d);
		fail++;
		return;
	}

	res = g_string_new (NULL);
	go_dtoa (res, "!g", d);

	if (go_strtod (res->str, NULL) != d) {
		fail++;
		g_printerr ("Round-trip failure for shortest %a (got \"%s\", expected \"%s\")\n",
			    d, res->str, candidate);
	} else if (g_str_equal (res->str, candidate)) {
		// Got the candidate
	} else if (res->len == strlen (candidate)) {
		// No worse than candidate
		if (0 && !strchr (res->str, 'e') && strchr (candidate, 'e')) {
			// Good, we got a non-e version
		} else {
			g_printerr ("Candidate \"%s\" differs from result \"%s\".\n",
				    candidate, res->str);
		}
	} else if (res->len < strlen (candidate)) {
		// This is a failure of the test, not of go_dtoa
		g_printerr ("Candidate \"%s\" is sub-optimal vs. \"%s\".\n",
			    candidate, res->str);
	} else {
		fail++;
		g_printerr ("Failure for shortest %a (got \"%s\", expected \"%s\")\n",
			    d, res->str, candidate);
	}

	g_free (candidate);
	g_string_free (res, TRUE);
}

#ifdef GOFFICE_WITH_LONG_DOUBLE
static void
test1ld (const char *fmt, long double d, const char *expected)
{
	GString *res = g_string_new (NULL);

	go_dtoa (res, fmt, d);
	if (!g_str_equal (res->str, expected)) {
		g_printerr ("Failed for %La \"%s\" (got \"%s\", expected \"%s\")\n",
			    d, fmt, res->str, expected);
		fail++;
	}
	g_string_free (res, TRUE);
}
#endif

// Exact halfway cases.  Like C's printf, go_dtoa must round those to even
// (or away from zero if the '^' flag is given).
//
// Regression test for fmt_fp: the number is expanded in base 10^9 "limbs"
// and when the rounding position was exactly at the start of a limb, the
// parity of the preceding digit (which lives in the previous limb) was not
// taken into account.  So "%.0f" of 1.5 gave "1" and of 3.5 gave "3".
// Upstream musl fixed this in 2016 (51ab6db4, "fix float formatting of
// some exact halfway cases").

// n + 0.5 with no fractional digits.
static void
test_half_integer (gint64 n)
{
	// Value is exactly representable as long as n < 2^52
	double d = n + 0.5;
	gint64 even = (n & 1) ? n + 1 : n;
	gint64 away = n + 1;
	char *expected;

	expected = g_strdup_printf ("%" G_GINT64_FORMAT, even);
	test1d (".0f", d, expected);
	g_free (expected);

	expected = g_strdup_printf ("%" G_GINT64_FORMAT, away);
	test1d ("^.0f", d, expected);
	g_free (expected);

	if (n == 0)
		return; // Avoid the sign of zero

	expected = g_strdup_printf ("-%" G_GINT64_FORMAT, even);
	test1d (".0f", -d, expected);
	g_free (expected);

	expected = g_strdup_printf ("-%" G_GINT64_FORMAT, away);
	test1d ("^.0f", -d, expected);
	g_free (expected);
}

// m / 1024 with m odd has exactly ten decimals, the last being 5.  With
// nine decimals requested, the rounding position is the first digit of the
// second limb of the fraction.
static void
test_half_nine_decimals (gint64 m)
{
	double d = m / 1024.0;
	gint64 n = m * 9765625;      // d in units of 1e-10; ends in 5
	gint64 q = n / 10;           // truncated to units of 1e-9
	gint64 even = (q & 1) ? q + 1 : q;
	gint64 away = q + 1;
	gint64 limb = 1000000000;
	char *expected;

	expected = g_strdup_printf ("%" G_GINT64_FORMAT ".%09" G_GINT64_FORMAT,
				    even / limb, even % limb);
	test1d (".9f", d, expected);
	g_free (expected);

	expected = g_strdup_printf ("%" G_GINT64_FORMAT ".%09" G_GINT64_FORMAT,
				    away / limb, away % limb);
	test1d ("^.9f", d, expected);
	g_free (expected);

	expected = g_strdup_printf ("-%" G_GINT64_FORMAT ".%09" G_GINT64_FORMAT,
				    even / limb, even % limb);
	test1d (".9f", -d, expected);
	g_free (expected);
}

static void
test_ties (void)
{
	static const gint64 bases[] = {
		0, 999999990, 1999999990, 2999999990,
		999999999990, 999999999999990
	};

	for (unsigned ui = 0; ui < G_N_ELEMENTS (bases); ui++) {
		for (gint64 i = 0; i <= 20; i++)
			test_half_integer (bases[ui] + i);
	}
	for (gint64 n = 21; n <= 2000; n++)
		test_half_integer (n);

	for (gint64 m = 1; m <= 4095; m += 2)
		test_half_nine_decimals (m);

	// A few explicit ones, including e and g formats where the rounding
	// position is at a limb boundary as well.
	test1d (".0f", 0.5, "0");
	test1d (".0f", 1.5, "2");
	test1d (".0f", 2.5, "2");
	test1d (".0f", 3.5, "4");
	test1d (".0f", 1000000001.5, "1000000002");
	test1d (".0f", 1000000002.5, "1000000002");
	test1d (".0f", 999999999.5, "1000000000");
	test1d (".0e", 1.5, "2e+00");
	test1d (".0e", 2.5, "2e+00");
	test1d (".0e", 3.5, "4e+00");
	test1d (".0e", -3.5, "-4e+00");
	test1d (".3e", 8893.5, "8.894e+03");
	test1d (".3e", 8892.5, "8.892e+03");
	test1d (".1g", 2.5, "2");
	test1d (".1g", 3.5, "4");
	test1d (".4g", 4643.5, "4644");
	test1d (".4g", 4642.5, "4642");
	test1d (".4g", -4643.5, "-4644");
	test1d (".16g", -2615649855492091.5, "-2615649855492092");
	test1d (".16g", 2615649855492092.5, "2615649855492092");

#ifdef GOFFICE_WITH_LONG_DOUBLE
	test1ld (".0Lf", 0.5L, "0");
	test1ld (".0Lf", 1.5L, "2");
	test1ld (".0Lf", 2.5L, "2");
	test1ld (".0Lf", 3.5L, "4");
	test1ld ("^.0Lf", 2.5L, "3");
	test1ld (".4Lg", 4643.5L, "4644");
	test1ld (".4Lg", 4642.5L, "4642");
	test1ld (".16Lg", -2615649855492091.5L, "-2615649855492092");
#endif
}


// Hex floats.  For these the precision is the number of hex digits after the
// point, and rounding is to nearest, ties to even.
//
// Regression test for fmt_fp: the code that did the rounding counted the hex
// digits in the mantissa, which gave the wrong answer when the number of
// mantissa bits is not a multiple of four.  For the x87 format (64 bits) that
// meant that a precision of 15 did no rounding and printed 16 digits.  See
// musl commits cfa0a54c and 24ebbbde.

#if defined(GOFFICE_WITH_LONG_DOUBLE) && LDBL_MANT_DIG == 64
// The expected output for the number 1.<frac63>, with 63 bits after the
// binary point, formatted with "%.<p>La" (or "%La" for p < 0).
static char *
hex_expected (guint64 frac63, int p, gboolean neg)
{
	const guint64 one = 1;
	// The 63 bits as 16 hex digits.
	guint64 digits = frac63 << 1;
	guint64 lead = 1;
	GString *res = g_string_new (neg ? "-0x" : "0x");
	int nd;

	if (p < 0) {
		// Print everything but trailing zeros
		nd = 16;
		while (nd > 0 && ((digits >> (64 - 4 * nd)) & 15) == 0)
			nd--;
	} else if (p >= 16) {
		nd = p;  // All digits, and then zeros
	} else {
		guint64 q, rem, half, total;
		gboolean up;

		if (p == 0) {
			q = 0;
			rem = digits;
			half = one << 63;
		} else {
			int sh = 64 - 4 * p;
			q = digits >> sh;
			rem = digits & ((one << sh) - 1);
			half = one << (sh - 1);
		}
		// The digit that decides ties is the last one kept, or the
		// leading 1 if there are none.
		up = rem > half || (rem == half && ((p == 0 ? lead : q) & 1));
		total = (lead << (4 * p)) | q;
		if (up)
			total++;
		lead = total >> (4 * p);
		digits = p == 0 ? 0 : total << (64 - 4 * p);
		nd = p;
	}

	g_string_append_printf (res, "%d", (int)lead);
	if (nd > 0) {
		g_string_append_c (res, '.');
		for (int i = 0; i < nd; i++) {
			int dgt = i < 16 ? (int)((digits >> (60 - 4 * i)) & 15) : 0;
			g_string_append_c (res, "0123456789abcdef"[dgt]);
		}
	}
	g_string_append (res, "p+0");

	return g_string_free (res, FALSE);
}

static void
test_hex_ld_one (guint64 frac63, int p)
{
	long double x = 1.0L + ldexpl ((long double)frac63, -63);
	char fmt[20];
	char *expected;

	if (p < 0)
		strcpy (fmt, "La");
	else
		sprintf (fmt, ".%dLa", p);

	expected = hex_expected (frac63, p, FALSE);
	test1ld (fmt, x, expected);
	g_free (expected);

	expected = hex_expected (frac63, p, TRUE);
	test1ld (fmt, -x, expected);
	g_free (expected);
}

static void
test_hex_ld (void)
{
	static const guint64 fracs[] = {
		0,
		1,
		G_GUINT64_CONSTANT (0x4000000000000000), // 1.8
		G_GUINT64_CONSTANT (0x7123456789abcdef),
		G_GUINT64_CONSTANT (0x7fffffffffffffff), // Just below 2
		G_GUINT64_CONSTANT (0x5555555555555555),
	};
	GRand *rand = g_rand_new_with_seed (42);

	for (unsigned ui = 0; ui < G_N_ELEMENTS (fracs); ui++)
		for (int p = -1; p <= 18; p++)
			test_hex_ld_one (fracs[ui], p);

	for (int i = 0; i < 200; i++) {
		guint64 frac63 = (((guint64)g_rand_int (rand) << 32) |
				  g_rand_int (rand)) >> 1;
		for (int p = -1; p <= 18; p++)
			test_hex_ld_one (frac63, p);
	}

	// Exact ties, with the last kept digit both odd and even.
	for (int p = 0; p <= 15; p++) {
		for (int i = 0; i < 20; i++) {
			guint64 q = ((guint64)g_rand_int (rand) << 32) |
				g_rand_int (rand);
			guint64 half = G_GUINT64_CONSTANT (1) << (63 - 4 * p);
			guint64 digits;

			if (p == 0)
				digits = half;
			else
				digits = ((q >> (4 * p)) << (64 - 4 * p)) | half;
			test_hex_ld_one (digits >> 1, p);
		}
	}

	g_rand_free (rand);

	// A few spelled out.  The mantissa 1.7123456789abcdef (63 bits after
	// the point) is 0x1.e2468acf13579bde as 16 hex digits.
	{
		long double x = 1.0L + ldexpl
			((long double)G_GUINT64_CONSTANT (0x7123456789abcdef), -63);
		test1ld ("La", x, "0x1.e2468acf13579bdep+0");
		test1ld (".16La", x, "0x1.e2468acf13579bdep+0");
		test1ld (".15La", x, "0x1.e2468acf13579bep+0");
		test1ld (".14La", x, "0x1.e2468acf13579cp+0");
		test1ld (".17La", x, "0x1.e2468acf13579bde0p+0");
	}
	test1ld (".0La", 1.5L, "0x2p+0");
	test1ld (".0La", 1.25L, "0x1p+0");
	test1ld (".0La", 2.5L, "0x1p+1");
}
#endif

static void
test_hex (void)
{
	// Ties: the digit before the rounding position decides
	test1d (".1a", 1.09375, "0x1.2p+0");      // 0x1.18 -> odd 1, round up
	test1d (".1a", 1.03125, "0x1.0p+0");      // 0x1.08 -> even 0, stay
	test1d (".1a", 1.15625, "0x1.2p+0");      // 0x1.28 -> even 2, stay
	test1d (".0a", 1.5, "0x2p+0");            // leading 1 is odd
	test1d (".0a", 1.25, "0x1p+0");
	test1d (".0a", -1.5, "-0x2p+0");
	test1d (".0a", 1.75, "0x2p+0");

	// Not ties
	test1d (".3a", 1.0 / 3, "0x1.555p-2");
	test1d (".3a", 2.0 / 3, "0x1.555p-1");
	test1d (".1a", 1.99609375, "0x2.0p+0");   // 0x1.ff carries into lead
	test1d (".1a", -1.99609375, "-0x2.0p+0");
	test1d (".2a", 1.99609375, "0x1.ffp+0");

	// Padding
	test1d (".4a", 1.5, "0x1.8000p+0");
	test1d (".20a", 1.5, "0x1.80000000000000000000p+0");

	// No precision
	test1d ("a", 1.0, "0x1p+0");
	test1d ("a", 0.5, "0x1p-1");
	test1d ("a", 1.5, "0x1.8p+0");
	test1d ("a", -255.5, "-0x1.ffp+7");
	test1d ("A", 255.5, "0X1.FFP+7");
	test1d (".1A", 1.09375, "0X1.2P+0");

#if defined(GOFFICE_WITH_LONG_DOUBLE) && LDBL_MANT_DIG == 64
	test_hex_ld ();
#endif
}


// Sign, width and padding with the shortest representation.  They used to be
// ignored, except that '+' worked for f-notation but not for e-notation.
static void
test_shortest_flags (void)
{
	test1d ("+!g", 1.5, "+1.5");
	test1d ("+!g", 1e30, "+1e+30");
	test1d ("+!e", 1.5, "+1.5e+00");
	test1d ("+!f", 1.5, "+1.5");
	test1d ("+!g", -1.5, "-1.5");
	test1d ("+!g", 0.0, "+0");
	test1d ("+!E", 1e-30, "+1E-30");

	test1d ("!10g", 1.5, "       1.5");
	test1d ("!10f", 1.5, "       1.5");
	test1d ("!10e", 1.5, "   1.5e+00");
	test1d ("-!10g", 1.5, "1.5       ");
	test1d ("-!10e", 1.5, "1.5e+00   ");
	test1d ("!010g", 1.5, "00000001.5");
	test1d ("!010g", -1.5, "-0000001.5");
	test1d ("+!010g", 1.5, "+0000001.5");
	test1d ("!012f", -1.5e-5, "-0000.000015");
	test1d ("!010e", 1.5, "0001.5e+00");
	test1d ("-!010g", 1.5, "1.5       ");	// '-' wins over '0'
	test1d ("+!10g", 1.5, "      +1.5");

	// A width that is too small does not truncate
	test1d ("!5g", 1e30, "1e+30");
	test1d ("!3g", 1e30, "1e+30");
	test1d ("!1g", 12345.0, "12345");

	// Not finite: no zero padding
	test1d ("!10g", INFINITY, "       inf");
	test1d ("!010g", INFINITY, "       inf");
	test1d ("!8g", -INFINITY, "    -inf");
	test1d ("+!g", INFINITY, "+inf");
	{
		// Not test1d: its sanity check cannot work for NaN
		GString *res = g_string_new (NULL);
		go_dtoa (res, "-!8g", (double)NAN);
		if (!g_str_equal (res->str, "nan     ")) {
			g_printerr ("Failed for NaN \"-!8g\" (got \"%s\")\n", res->str);
			fail++;
		}
		g_string_free (res, TRUE);
	}

	// The same flags without '!' are of course the usual printf ones
	test1d ("+g", 1e30, "+1e+30");
	test1d ("010g", -1.5, "-0000001.5");

#ifdef GOFFICE_WITH_LONG_DOUBLE
	test1ld ("+!Lg", 1.5L, "+1.5");
	test1ld ("+!Le", 1.5L, "+1.5e+00");
	test1ld ("!10Lg", 1.5L, "       1.5");
	test1ld ("!010Lg", -1.5L, "-0000001.5");
#endif
}

// '*' for width and precision.
static void
test_star (void)
{
	struct { const char *fmt; int w; int p; const char *want; } cases[] = {
		{ "*g", 8, 0, "     1.5" },
		{ "*g", -8, 0, "1.5     " },
		{ "-*g", -8, 0, "1.5     " },
		{ "*.*f", 8, 2, "    1.50" },
		{ "*.*f", 8, -1, "1.500000" },
		{ "*.*f", 0, 0, "2" },
		{ "!*g", 8, 0, "     1.5" },
		{ "0*.*f", 8, 2, "00001.50" },
	};

	for (unsigned ui = 0; ui < G_N_ELEMENTS (cases); ui++) {
		GString *res = g_string_new (NULL);
		const char *fmt = cases[ui].fmt;
		if (strstr (fmt, ".*"))
			go_dtoa (res, fmt, cases[ui].w, cases[ui].p, 1.5);
		else
			go_dtoa (res, fmt, cases[ui].w, 1.5);
		if (!g_str_equal (res->str, cases[ui].want)) {
			g_printerr ("Failed for \"%s\" with %d and %d (got \"%s\", expected \"%s\")\n",
				    fmt, cases[ui].w, cases[ui].p,
				    res->str, cases[ui].want);
			fail++;
		}
		g_string_free (res, TRUE);
	}
}

// Invalid formats
static int n_critical;

static void
count_critical (const gchar *domain, GLogLevelFlags level,
		const gchar *message, gpointer user_data)
{
	n_critical++;
}

static void
test_bad_format (const char *fmt)
{
	GString *res = g_string_new ("X");
	GLogFunc old_handler;
	GLogLevelFlags old_fatal;

	old_fatal = g_log_set_always_fatal (G_LOG_FATAL_MASK);
	old_handler = g_log_set_default_handler (count_critical, NULL);
	n_critical = 0;
	go_dtoa (res, fmt, 1.5, 1.5);
	g_log_set_default_handler (old_handler, NULL);
	g_log_set_always_fatal (old_fatal);

	if (n_critical != 1) {
		g_printerr ("Failed for \"%s\" (expected one critical warning, got %d)\n",
			    fmt, n_critical);
		fail++;
	}
	// In particular, no 0 and no NUL byte
	if (res->len != 1 || !g_str_equal (res->str, "X")) {
		g_printerr ("Failed for \"%s\" (destination was changed to \"%s\")\n",
			    fmt, res->str);
		fail++;
	}
	g_string_free (res, TRUE);
}

static void
test_bad_formats (void)
{
	// No conversion.
	test_bad_format ("");
	test_bad_format (".3");
	test_bad_format ("10");
	test_bad_format ("+");
	test_bad_format ("!");
	test_bad_format (".");
	test_bad_format ("-10.5");

	// Things printf has, but which we do not
	test_bad_format ("lf");
	test_bad_format ("#.3g");
	test_bad_format (" g");
	test_bad_format ("'g");
	test_bad_format ("d");
	test_bad_format ("xg");

	// Flags go first
	test_bad_format ("10!g");
	test_bad_format (".3+g");

	// Numbers that are too big, including ones that overflow an int
	test_bad_format ("1000000000g");
	test_bad_format ("99999999999g");
	test_bad_format ("99999999999999999999999g");
	test_bad_format (".1000000000g");
	test_bad_format (".99999999999g");

#ifndef GOFFICE_WITH_LONG_DOUBLE
	test_bad_format ("Lg");
	test_bad_format ("!Lg");
#endif
#ifndef GOFFICE_WITH_DECIMAL64
	test_bad_format ("Dg");
	test_bad_format ("!Dg");
#endif
}


int
main (void)
{
	GRand *grand;

	libgoffice_init ();
	(void)test1d_fail;

	test1d (".15g", ldexp(1, -44), "5.6843418860808e-14");
	test1d (".16g", ldexp(1, -44), "5.684341886080801e-14");
	test1d (".17g", ldexp(1, -44), "5.6843418860808015e-14");

	test_ties ();
	test_hex ();
	test_shortest_flags ();
	test_star ();
	test_bad_formats ();

	for (int p = -1; p >= -1021; p--) {
		double p2 = ldexp(1, p);
		test1d_shortest (p2);
		test1d_shortest (nextafter (p2, go_pinf));
		test1d_shortest (nextafter (p2, go_ninf));
	}

	for (int i = -99999; i <= 99999; i++)
		test1d_shortest (i);

	for (int i = 1; i <= 50; i++) {
		test1d_shortest (go_pow10 (i));
		test1d_shortest (-go_pow10 (i));
	}

	for (int n = -9999; n <= 9999; n++) {
		for (int ld = 0; ld <= 10; ld++) {
			test1d_shortest (n / go_pow10 (ld));
		}
	}

	grand = g_rand_new ();

	for (int i = 0; i < 100000; i++) {
		double n = g_rand_int_range (grand, -9999, +9999);
		double d = go_pow10 (g_rand_int_range (grand, 0, 10));
		test1d_shortest (n / d);
	}

	for (int i = 0; i < 100000; i++) {
		double d = g_rand_double (grand) * 1e10;
		test1d_shortest (d);
	}

	g_rand_free (grand);

	return fail;
}
