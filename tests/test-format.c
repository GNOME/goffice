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