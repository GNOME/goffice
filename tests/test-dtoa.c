#include <goffice/goffice.h>

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

	for (int dig = 0; dig <= 17; dig++) {
		char *epos;

		candidate = g_strdup_printf ("%.*g", dig, d);
		epos = strchr (candidate, 'e');

		// Get rid of extra 0 in exponent
		if (epos && (epos[1] == '-' || epos[1] == '+') &&
		    epos[2] == '0' && epos[3]) {
			memmove (epos + 2, epos + 3, strlen (epos + 2));
		}

		// Get rid of '+' in exponent
		if (epos && epos[1] == '+')
			memmove (epos + 1, epos + 2, strlen (epos + 1));

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
	} else if (res->len == strlen (candidate)) {
		// No worse than candidate
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




int
main (void)
{
	libgoffice_init ();
	(void)test1d_fail;

	test1d (".15g", ldexp(1, -44), "5.6843418860808e-14");
	test1d (".16g", ldexp(1, -44), "5.684341886080801e-14");
	test1d (".17g", ldexp(1, -44), "5.6843418860808015e-14");

	for (int p = -1; p >= -1021; p--) {
		double p2 = ldexp(1, p);
		test1d_shortest (p2);
		test1d_shortest (nextafter (p2, go_pinf));
		test1d_shortest (nextafter (p2, go_ninf));
	}

	for (int i = -999; i <= 999; i++)
		test1d_shortest (i);

	test1d_shortest (1e1);
	test1d_shortest (1e2);
	test1d_shortest (1e3);
	test1d_shortest (1e4);
	test1d_shortest (1e5);
	test1d_shortest (1e6);
	test1d_shortest (1e7);
	test1d_shortest (1e8);
	test1d_shortest (1e9);
	test1d_shortest (1e10);

	{
		GRand *grand = g_rand_new ();
		for (int i = 0; i < 100000; i++) {
			double d = g_rand_double (grand) * 1e10;
			test1d_shortest (d);
		}
		g_rand_free (grand);
	}

	return fail;
}