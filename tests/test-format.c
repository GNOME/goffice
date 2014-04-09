#include <goffice/goffice.h>
#include <string.h>

/* ------------------------------------------------------------------------- */

static void
test_general_format_1 (double val, int width, const char *expected)
{
	GString *str = g_string_new (NULL);

	go_render_general (NULL, str,
			   go_format_measure_strlen,
			   go_font_metrics_unit,
			   val, width,
			   FALSE, 0, 0);

	g_printerr ("go_render_general: %.17g %d -> \"%s\"\n",
		    val, width, str->str);

	if (expected && strcmp (str->str, expected) != 0) {
		g_printerr ("Expected \"%s\"\n", expected);
		g_assert (0);
	}

	g_string_free (str, TRUE);
}

static void
test_general_format (void)
{
	test_general_format_1 (nextafter (1.999999999, 2), 12, "1.999999999");
	test_general_format_1 (nextafter (1.999999999, 2), 11, "1.999999999");
	test_general_format_1 (nextafter (1.999999999, 2), 10, "2");
	test_general_format_1 (nextafter (1.999999999, 2),  9, "2");

	test_general_format_1 (1.999999999, 12, "1.999999999");
	test_general_format_1 (1.999999999, 11, "1.999999999");
	test_general_format_1 (1.999999999, 10, "2");
	test_general_format_1 (1.999999999,  9, "2");

	test_general_format_1 (999999999, 9, "999999999");
	test_general_format_1 (999999999, 8, "1E+09");
	test_general_format_1 (-999999999, 10, "-999999999");
	test_general_format_1 (-999999999, 9, "-1E+09");

	test_general_format_1 (0, 5, "0");
	test_general_format_1 (0, 3, "0");
	test_general_format_1 (0, 2, "0");
	test_general_format_1 (0, 1, "0");

	test_general_format_1 (9.25, 5, "9.25");
	test_general_format_1 (9.25, 3, "9.3");
	test_general_format_1 (9.25, 2, "9");
	test_general_format_1 (9.25, 1, "9");

	test_general_format_1 (9.5, 5, "9.5");
	test_general_format_1 (9.5, 3, "9.5");
	test_general_format_1 (9.5, 2, "10");
	test_general_format_1 (9.5, 1, NULL);

	test_general_format_1 (-9.5, 5, "-9.5");
	test_general_format_1 (-9.5, 4, "-9.5");
	test_general_format_1 (-9.5, 3, "-10");
	test_general_format_1 (-9.5, 2, NULL);

	test_general_format_1 (-9.25, 5, "-9.25");
	test_general_format_1 (-9.25, 4, "-9.3");
	test_general_format_1 (-9.25, 2, "-9");
	test_general_format_1 (-9.25, 1, NULL);

	test_general_format_1 (0.125, 10, "0.125");
	test_general_format_1 (0.125, 5, "0.125");
	test_general_format_1 (0.125, 4, "0.13");
	test_general_format_1 (0.125, 3, "0.1");
	test_general_format_1 (0.125, 3, "0.1");
	test_general_format_1 (0.125, 2, "0");
	test_general_format_1 (0.125, 1, "0");

	test_general_format_1 (-0.125, 6, "-0.125");
	test_general_format_1 (-0.125, 5, "-0.13");
	test_general_format_1 (-0.125, 4, "-0.1");
	test_general_format_1 (-0.125, 3, "-0");
	test_general_format_1 (-0.125, 2, "-0");
	test_general_format_1 (-0.125, 1, NULL);

	test_general_format_1 (1e-20, 25, "1E-20");
	test_general_format_1 (1e-20, 20, "1E-20");
	test_general_format_1 (1e-20, 15, "1E-20");
	test_general_format_1 (1e-20,  5, "1E-20");
	test_general_format_1 (1e-20,  4, "0");

	test_general_format_1 (1.0 / 3, 19, "0.3333333333333333");
	test_general_format_1 (1.0 / 3, 18, "0.3333333333333333");
	test_general_format_1 (1.0 / 3, 17, "0.333333333333333");
	test_general_format_1 (1.0 / 3, 10, "0.33333333");

	test_general_format_1 (0.12509999, 11, "0.12509999");
	test_general_format_1 (0.12509999, 10, "0.12509999");
	test_general_format_1 (0.12509999, 9, "0.1251");
	test_general_format_1 (0.12509999, 8, "0.1251");
	test_general_format_1 (0.12509999, 7, "0.1251");
	test_general_format_1 (0.12509999, 6, "0.1251");

	test_general_format_1 (0.12509999001, 13, "0.12509999001");
	test_general_format_1 (0.12509999001, 12, "0.12509999");
	test_general_format_1 (0.12509999001, 11, "0.12509999");
	test_general_format_1 (0.12509999001, 10, "0.12509999");
	test_general_format_1 (0.12509999001, 9, "0.1251");
	test_general_format_1 (0.12509999001, 8, "0.1251");
	test_general_format_1 (0.12509999001, 7, "0.1251");
	test_general_format_1 (0.12509999001, 6, "0.1251");
}

/* ------------------------------------------------------------------------- */

int
main (int argc, char **argv)
{
	libgoffice_init ();

	test_general_format ();

	libgoffice_shutdown ();

	return 0;
}
