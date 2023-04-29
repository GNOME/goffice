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


int
main (void)
{
	test1d (".15g", ldexp(1, -44), "5.6843418860808e-14");
	test1d (".16g", ldexp(1, -44), "5.684341886080801e-14");
	test1d (".17g", ldexp(1, -44), "5.6843418860808015e-14");
	test1d_fail ("!g", ldexp(1, -44), "5.684341886080802e-14");

	return fail;
}
