/*
 * test-format.c: unit tests for the code in goffice/utils/go-format.c.
 *
 * Only the ordinary, double-based functions are tested here.  The long
 * double and decimal64 variants are generated from the same source and do
 * not need separate tests.
 */

#include <goffice/goffice.h>
#include <gsf/gsf-output-memory.h>
#include <pango/pangocairo.h>
#include <locale.h>
#include <math.h>
#include <string.h>

#define UNICODE_MINUS "\xe2\x88\x92"
#define PI_SYMBOL "\xf0\x9d\x9c\x8b"

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */

/* Format strings are kept in canonical (unlocalized) form in the tests. */
static char *
localize_format (const char *str)
{
	char *res = go_format_str_localize (str);
	g_assert_nonnull (res);
	return res;
}

static GOFormat *
new_format (const char *canonical)
{
	char *str = localize_format (canonical);
	GOFormat *fmt = go_format_new_from_XL (str);
	g_free (str);
	g_assert_nonnull (fmt);
	return fmt;
}

/* Likewise for numbers that contain a decimal point or thousands separator. */
static char *
localize_number (const char *str)
{
	GString *res = g_string_new (NULL);
	const GString *decimal = go_locale_get_decimal ();
	const GString *thousand = go_locale_get_thousand ();
	const char *p;

	g_assert_nonnull (decimal);
	g_assert_nonnull (thousand);

	for (p = str; *p; p++) {
		switch (*p) {
		case '.':
			go_string_append_gstring (res, decimal);
			break;
		case ',':
			go_string_append_gstring (res, thousand);
			break;
		default:
			g_string_append_c (res, *p);
			break;
		}
	}

	return g_string_free (res, FALSE);
}

static char *
delocalize_format (GOFormat const *fmt)
{
	char *str = go_format_str_delocalize (go_format_as_XL (fmt));
	g_assert_nonnull (str);
	return str;
}

static void
assert_format_string (GOFormat const *fmt, const char *canonical)
{
	char *actual = delocalize_format (fmt);
	g_assert_cmpstr (actual, ==, canonical);
	g_free (actual);
}

static char *
format_value_for (GOFormat const *fmt, double val)
{
	char *res = go_format_value (fmt, val);
	g_assert_nonnull (res);
	return res;
}

static void
assert_format_value (const char *canonical_fmt, double val,
		     const char *canonical_expected)
{
	GOFormat *fmt = new_format (canonical_fmt);
	char *expected = localize_number (canonical_expected);
	char *actual = format_value_for (fmt, val);

	g_assert_cmpstr (actual, ==, expected);

	g_free (actual);
	g_free (expected);
	go_format_unref (fmt);
}

static GOFormatNumberError
render_value (GOFormat const *fmt, GString *str, double val, char type,
	      const char *sval, GOColor *color, int col_width,
	      GODateConventions const *conv, gboolean unicode_minus)
{
	return go_format_value_gstring (NULL, str,
		go_format_measure_strlen, go_font_metrics_unit,
		fmt, val, type, sval, color, col_width, conv, unicode_minus);
}

static void
assert_rendered (const char *canonical_fmt, double val, char type,
		 const char *sval, GOColor *color, int col_width,
		 GODateConventions const *conv, gboolean unicode_minus,
		 GOFormatNumberError expected_err,
		 const char *canonical_expected)
{
	GOFormat *fmt = new_format (canonical_fmt);
	GString *str = g_string_new (NULL);
	GOFormatNumberError err;
	char *expected = canonical_expected
		? localize_number (canonical_expected) : NULL;

	err = render_value (fmt, str, val, type, sval, color,
			    col_width, conv, unicode_minus);

	g_assert_cmpint (err, ==, expected_err);
	if (expected)
		g_assert_cmpstr (str->str, ==, expected);

	g_free (expected);
	g_string_free (str, TRUE);
	go_format_unref (fmt);
}

typedef struct {
	const char *format;
	unsigned count;
} ForeachData;

static void
count_format (gpointer key, gpointer value, gpointer user_data)
{
	ForeachData *data = user_data;
	GOFormat const *fmt = value;

	g_assert_nonnull (key);
	g_assert_nonnull (fmt);
	g_assert_cmpstr (key, ==, go_format_as_XL (fmt));

	if (strcmp (key, data->format) == 0)
		data->count++;
}

static char *
odf_output (const char *canonical_fmt, int cond_part, const char *name,
	    gboolean with_extension, gboolean pretty_print,
	    gboolean expected_ok)
{
	GOFormat *fmt = new_format (canonical_fmt);
	GsfOutput *mem = gsf_output_memory_new ();
	GsfXMLOut *xml = gsf_xml_out_new (mem);
	const guint8 *bytes;
	gsf_off_t size;
	gboolean ok;
	char *res;

	g_object_set (G_OBJECT (xml), "pretty-print", pretty_print, NULL);
	gsf_xml_out_start_element (xml, "root");
	ok = go_format_output_to_odf (xml, fmt, cond_part, name,
				      with_extension);
	gsf_xml_out_end_element (xml);
	g_assert_cmpint (ok, ==, expected_ok);

	if (pretty_print) {
		gboolean still_pretty = FALSE;
		g_object_get (G_OBJECT (xml), "pretty-print", &still_pretty, NULL);
		g_assert_true (still_pretty);
	}

	g_object_unref (xml);
	gsf_output_close (mem);

	bytes = gsf_output_memory_get_bytes (GSF_OUTPUT_MEMORY (mem));
	size = gsf_output_size (mem);
	res = g_strndup ((const char *)bytes, size);

	g_object_unref (mem);
	go_format_unref (fmt);
	return res;
}

static void
assert_contains (const char *haystack, const char *needle)
{
	if (!strstr (haystack, needle))
		g_assertion_message_cmpstr (G_LOG_DOMAIN, __FILE__, __LINE__,
			G_STRFUNC, "strstr (haystack, needle) != NULL",
			haystack, "contains", needle);
}

/* ------------------------------------------------------------------------- */
/* go_render_general                                                         */
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

	if (expected)
		g_assert_cmpstr (str->str, ==, expected);

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

	test_general_format_1 (-9.5, 5, "-9.5");
	test_general_format_1 (-9.5, 4, "-9.5");
	test_general_format_1 (-9.5, 3, "-10");

	test_general_format_1 (-9.25, 5, "-9.25");
	test_general_format_1 (-9.25, 4, "-9.3");
	test_general_format_1 (-9.25, 2, "-9");

	test_general_format_1 (0.125, 10, "0.125");
	test_general_format_1 (0.125, 5, "0.125");
	test_general_format_1 (0.125, 4, "0.13");
	test_general_format_1 (0.125, 3, "0.1");
	test_general_format_1 (0.125, 2, "0");
	test_general_format_1 (0.125, 1, "0");

	test_general_format_1 (-0.125, 6, "-0.125");
	test_general_format_1 (-0.125, 5, "-0.13");
	test_general_format_1 (-0.125, 4, "-0.1");
	test_general_format_1 (-0.125, 3, "-0");
	test_general_format_1 (-0.125, 2, "-0");

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

#if 0
	test_general_format_1 (6861116509411105.0, 20, "6.86111650941111E+15");
	test_general_format_1 (6861116509411105.0, 19, "6.8611165094111E+15");
	test_general_format_1 (6861116509411105.0, 18, "6.861116509411E+15");
	test_general_format_1 (6861116509411105.0, 17, "6.86111650941E+15");
	test_general_format_1 (6861116509411105.0, 16, "6.8611165094E+15");
	test_general_format_1 (6861116509411105.0, 15, "6.861116509E+15");
	test_general_format_1 (6861116509411105.0, 14, "6.86111651E+15");
#endif

	/* Only 15 digits needed.  Lots of terminating zeros.  */
	test_general_format_1 (75776.21, -1, "75776.21");
}

static void
test_general_format_unicode_minus (void)
{
	GString *str = g_string_new (NULL);

	go_render_general (NULL, str, go_format_measure_strlen,
			   go_font_metrics_unit, -9.5, -1,
			   TRUE, 0, 0);
	g_assert_cmpstr (str->str, ==, UNICODE_MINUS "9.5");

	g_string_free (str, TRUE);
}

/* ------------------------------------------------------------------------- */
/* Creation, interning, and reference counting                               */
/* ------------------------------------------------------------------------- */

static void
test_format_creation (void)
{
	GOFormat *a = new_format ("0.00");
	GOFormat *b = new_format ("0.00");
	GOFormat *c = go_format_ref (a);
	GOFormat *other = new_format ("0.0");

	g_assert_true (go_format_eq (a, b));
	g_assert_true (a == b);
	g_assert_true (go_format_eq (a, c));
	g_assert_false (go_format_eq (a, other));

	go_format_unref (other);

	g_assert_cmpstr (go_format_as_XL (a), ==,
			 go_format_as_XL (b));
	g_assert_true (go_format_as_XL (a) != NULL);

	go_format_unref (c);
	go_format_unref (b);
	go_format_unref (a);
}

static void
test_format_special_strings (void)
{
	GOFormat *invalid = go_format_new_from_XL ("[");
	GOFormat *empty = new_format ("");
	GOFormat *text = new_format ("@");
	GOFormat *general = new_format ("General");

	g_assert_nonnull (invalid);
	g_assert_true (go_format_is_invalid (invalid));
	g_assert_cmpint (go_format_get_family (invalid), ==, GO_FORMAT_UNKNOWN);
	g_assert_cmpstr (go_format_as_XL (invalid), ==, "[");

	g_assert_false (go_format_is_invalid (empty));
	g_assert_cmpstr (go_format_as_XL (empty), ==, "");
	g_assert_cmpint (go_format_get_family (empty), ==, GO_FORMAT_UNKNOWN);

	g_assert_true (go_format_is_text (text));
	g_assert_cmpint (go_format_get_family (text), ==, GO_FORMAT_TEXT);

	g_assert_true (go_format_is_general (general));
	g_assert_cmpint (go_format_get_family (general), ==,
			 GO_FORMAT_GENERAL);

	go_format_unref (invalid);
	go_format_unref (empty);
	go_format_unref (text);
	go_format_unref (general);
}

static void
test_format_singletons (void)
{
	struct {
		GOFormat *(*get) (void);
		GOFormatMagic magic;
	} const magic_formats[] = {
		{ go_format_default_date, GO_FORMAT_MAGIC_SHORT_DATE },
		{ go_format_default_time, GO_FORMAT_MAGIC_SHORT_TIME },
		{ go_format_default_date_time, GO_FORMAT_MAGIC_SHORT_DATETIME }
	};
	GOFormat *fmt;
	unsigned i;

	g_assert_true (go_format_eq (go_format_general (), go_format_general ()));
	g_assert_true (go_format_eq (go_format_empty (), go_format_empty ()));

	g_assert_true (go_format_is_general (go_format_general ()));
	g_assert_cmpstr (go_format_as_XL (go_format_empty ()), ==, "");

	g_assert_true (go_format_get_family (go_format_default_percentage ()) ==
		       GO_FORMAT_PERCENTAGE);

	/* The money and accounting defaults come from the generated builtins,
	 * whose currency depends on the locale.  */
	{
		struct {
			GOFormat *(*get) (void);
			GOFormatFamily family;
			int index;
		} const defaults[] = {
			{ go_format_default_money, GO_FORMAT_CURRENCY, 2 },
			{ go_format_default_accounting, GO_FORMAT_ACCOUNTING, 2 }
		};
		unsigned j;

		for (j = 0; j < G_N_ELEMENTS (defaults); j++) {
			const char *builtin =
				_go_format_builtins (defaults[j].family)[defaults[j].index];
			GOFormat *expected;

			g_assert_nonnull (builtin);
			g_assert_cmpstr (go_format_as_XL (defaults[j].get ()), ==,
					 builtin);
			expected = new_format (builtin);
			g_assert_true (go_format_eq (defaults[j].get (), expected));
			go_format_unref (expected);
		}
	}

	for (i = 0; i < G_N_ELEMENTS (magic_formats); i++) {
		fmt = magic_formats[i].get ();
		g_assert_true (go_format_eq (fmt, magic_formats[i].get ()));
		g_assert_cmpint (go_format_get_magic (fmt), ==,
				 magic_formats[i].magic);
	}
}

static void
test_format_foreach (void)
{
	ForeachData data;
	GOFormat *fmt;

	data.format = "0.00000000000000000001";
	data.count = 0;

	fmt = new_format (data.format);
	go_format_foreach (count_format, &data);
	g_assert_cmpuint (data.count, >, 0);
	go_format_unref (fmt);
}

/* ------------------------------------------------------------------------- */
/* Predicates                                                                */
/* ------------------------------------------------------------------------- */

static void
test_format_predicates (void)
{
	GOFormat *number = new_format ("0.00");
	GOFormat *conditional = new_format ("[>0]0.00;@");
	GOFormat *fill = new_format ("*0.00");
	GOFormat *general_with_color = new_format ("[Red]General");
	GOFormat *prefixed_general = new_format ("\"x\"General");
	GOFormat *text = new_format ("@");

	g_assert_true (go_format_is_simple (number));
	g_assert_false (go_format_is_simple (conditional));

	g_assert_false (go_format_is_var_width (number));
	g_assert_true (go_format_is_var_width (fill));
	g_assert_true (go_format_is_var_width (general_with_color));
	g_assert_true (go_format_is_var_width (go_format_general ()));
	g_assert_false (go_format_is_var_width (text));

	g_assert_true (go_format_is_general (general_with_color));
	g_assert_false (go_format_is_general (prefixed_general));

	g_assert_false (go_format_is_invalid (number));
	g_assert_false (go_format_is_text (number));
	g_assert_false (go_format_is_markup (number));

	go_format_unref (number);
	go_format_unref (conditional);
	go_format_unref (fill);
	go_format_unref (general_with_color);
	go_format_unref (prefixed_general);
	go_format_unref (text);
}

static void
test_format_families (void)
{
	struct {
		const char *format;
		GOFormatFamily family;
	} const cases[] = {
		{ "General", GO_FORMAT_GENERAL },
		{ "0.00", GO_FORMAT_NUMBER },
		/* Currency and accounting are distinguished by get_details,
		 * not by the coarse family of the parsed format.  */
		{ "$#,##0.00", GO_FORMAT_NUMBER },
		{ "_($* #,##0.00_);_($* (#,##0.00);_($* \"-\"??_);_(@_)",
		  GO_FORMAT_NUMBER },
		{ "yyyy-mm-dd", GO_FORMAT_DATE },
		{ "hh:mm", GO_FORMAT_TIME },
		{ "0.00%", GO_FORMAT_PERCENTAGE },
		{ "# ?/?", GO_FORMAT_FRACTION },
		{ "0.00E+00", GO_FORMAT_SCIENTIFIC },
		{ "@", GO_FORMAT_TEXT }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		GOFormat *fmt = new_format (cases[i].format);
		g_assert_cmpint (go_format_get_family (fmt), ==,
				 cases[i].family);
		go_format_unref (fmt);
	}
}

static void
test_format_date_time_predicates (void)
{
	GOFormat *fmt;

	fmt = new_format ("mm/dd/yyyy");
	g_assert_cmpint (go_format_is_date (fmt), ==, 1);
	g_assert_cmpint (go_format_is_time (fmt), ==, 0);
	g_assert_cmpint (go_format_month_before_day (fmt), ==, 1);
	g_assert_true (go_format_has_year (fmt));
	g_assert_true (go_format_has_month (fmt));
	g_assert_true (go_format_has_day (fmt));
	g_assert_false (go_format_has_hour (fmt));
	g_assert_false (go_format_has_minute (fmt));
	go_format_unref (fmt);

	fmt = new_format ("yyyy-mm-dd");
	g_assert_cmpint (go_format_is_date (fmt), ==, 1);
	g_assert_cmpint (go_format_month_before_day (fmt), ==, 2);
	go_format_unref (fmt);

	fmt = new_format ("dd-mm-yyyy");
	g_assert_cmpint (go_format_is_date (fmt), ==, 1);
	g_assert_cmpint (go_format_month_before_day (fmt), ==, 0);
	go_format_unref (fmt);

	fmt = new_format ("yyyy-mm-dd hh:mm");
	g_assert_cmpint (go_format_is_date (fmt), ==, 2);
	g_assert_cmpint (go_format_is_time (fmt), ==, 0);
	g_assert_true (go_format_has_hour (fmt));
	g_assert_true (go_format_has_minute (fmt));
	go_format_unref (fmt);

	fmt = new_format ("hh:mm");
	g_assert_cmpint (go_format_is_date (fmt), ==, 0);
	g_assert_cmpint (go_format_is_time (fmt), ==, 1);
	g_assert_true (go_format_has_hour (fmt));
	g_assert_true (go_format_has_minute (fmt));
	g_assert_cmpint (go_format_month_before_day (fmt), ==, -1);
	go_format_unref (fmt);

	fmt = new_format ("[h]:mm:ss");
	g_assert_cmpint (go_format_is_time (fmt), ==, 2);
	go_format_unref (fmt);

	fmt = new_format ("0.00");
	g_assert_cmpint (go_format_is_date (fmt), ==, 0);
	g_assert_cmpint (go_format_is_time (fmt), ==, 0);
	g_assert_cmpint (go_format_month_before_day (fmt), ==, -1);
	g_assert_false (go_format_has_year (fmt));
	g_assert_false (go_format_has_month (fmt));
	g_assert_false (go_format_has_day (fmt));
	g_assert_false (go_format_has_hour (fmt));
	g_assert_false (go_format_has_minute (fmt));
	go_format_unref (fmt);
}

/* ------------------------------------------------------------------------- */
/* Magic formats                                                             */
/* ------------------------------------------------------------------------- */

static void
test_format_magic (void)
{
	struct {
		GOFormatMagic magic;
		const char *format;
	} const cases[] = {
		{ GO_FORMAT_MAGIC_LONG_DATE, "[$-f800]dddd, mmmm dd, yyyy" },
		{ GO_FORMAT_MAGIC_MEDIUM_DATE, "[$-f8f1]d-mmm-yy" },
		{ GO_FORMAT_MAGIC_SHORT_DATE, "[$-f8f2]m/d/yy" },
		{ GO_FORMAT_MAGIC_SHORT_DATETIME, "[$-f8fa]m/d/yy h:mm" },
		{ GO_FORMAT_MAGIC_LONG_TIME, "[$-f400]h:mm:ss AM/PM" },
		{ GO_FORMAT_MAGIC_MEDIUM_TIME, "[$-f4f1]h:mm AM/PM" },
		{ GO_FORMAT_MAGIC_SHORT_TIME, "[$-f4f2]hh:mm" }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		GOFormat *fmt = go_format_new_magic (cases[i].magic);
		g_assert_nonnull (fmt);
		g_assert_cmpint (go_format_get_magic (fmt), ==, cases[i].magic);
		g_assert_cmpstr (go_format_as_XL (fmt), ==, cases[i].format);
		go_format_unref (fmt);
	}

	g_assert_null (go_format_new_magic ((GOFormatMagic)0x1234));
}

/* ------------------------------------------------------------------------- */
/* Conditional formats                                                       */
/* ------------------------------------------------------------------------- */

static void
test_format_specialize_simple (void)
{
	GOFormat *fmt = new_format ("0.00;-0.00;@");
	GOFormat *positive = new_format ("0.00");
	GOFormat *negative = new_format ("-0.00");
	GOFormat *text = new_format ("@");
	gboolean inhibit;

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 5, 'F', &inhibit)), ==,
		go_format_as_XL (positive));
	g_assert_false (inhibit);

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, -5, 'F', &inhibit)), ==,
		go_format_as_XL (negative));
	g_assert_true (inhibit);

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 0, 'F', &inhibit)), ==,
		go_format_as_XL (positive));
	g_assert_false (inhibit);

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 0, 'S', &inhibit)), ==,
		go_format_as_XL (text));
	g_assert_true (inhibit);

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 0, 'B', &inhibit)), ==,
		go_format_as_XL (text));
	g_assert_true (inhibit);

	go_format_unref (text);
	go_format_unref (negative);
	go_format_unref (positive);
	go_format_unref (fmt);
}

static void
test_format_specialize_conditions (void)
{
	GOFormat *fmt = new_format ("[>10]0.00;[<=10]0;@");
	GOFormat *gt = new_format ("[>10]0.00");
	GOFormat *le = new_format ("[<=10]0");
	gboolean inhibit;

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 20, 'F', &inhibit)), ==,
		go_format_as_XL (gt));
	g_assert_false (inhibit);

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 10, 'F', &inhibit)), ==,
		go_format_as_XL (le));
	g_assert_false (inhibit);

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, -10, 'F', &inhibit)), ==,
		go_format_as_XL (le));
	g_assert_false (inhibit);

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 0, 'S', &inhibit)), ==, "@");
	g_assert_false (inhibit);

	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 0, 'B', NULL)), ==, "@");

	go_format_unref (gt);
	go_format_unref (le);
	go_format_unref (fmt);

	fmt = new_format ("[=0]\"zero\";[<>0]\"other\";@");
	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 0, 'F', NULL)), ==, "[=0]\"zero\"");
	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 1, 'F', NULL)), ==, "[<>0]\"other\"");
	go_format_unref (fmt);

	fmt = new_format ("[<0]\"negative\";[>=0]\"positive\";@");
	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, -1, 'F', &inhibit)), ==,
		"[<0]\"negative\"");
	g_assert_true (inhibit);
	g_assert_cmpstr (go_format_as_XL (
		go_format_specialize (fmt, 1, 'F', &inhibit)), ==,
		"[>=0]\"positive\"");
	g_assert_false (inhibit);
	go_format_unref (fmt);
}

/* ------------------------------------------------------------------------- */
/* Value rendering                                                           */
/* ------------------------------------------------------------------------- */

static void
test_format_value_numbers (void)
{
	assert_format_value ("0", 1234.567, "1235");
	assert_format_value ("0", -1234.567, "-1235");
	assert_format_value ("0.0", 1234.567, "1234.6");
	assert_format_value ("0.00", 1234.567, "1234.57");
	assert_format_value ("#,##0.00", 1234.567, "1,234.57");
	assert_format_value ("#,##0.00", -1234.567, "-1,234.57");
	assert_format_value ("0.00%", 1.234567, "123.46%");
	assert_format_value ("0.00E+00", 1234.567, "1.23E+03");
	assert_format_value ("0.00E+00", -1234.567, "-1.23E+03");
	assert_format_value ("# ?/?", 1.5, "1 1/2");
	/* Two-digit numerator and denominator fields are space padded.  */
	assert_format_value ("# ?\?/?\?", 1.5, "1  1/2 ");
}

static void
test_format_value_special (void)
{
	GOFormat *fmt;
	char *str;

	/* Invalid formats render as a row of hashes.  */
	fmt = go_format_new_from_XL ("[");
	str = format_value_for (fmt, 1);
	g_assert_cmpstr (str, ==, "#####");
	g_free (str);
	go_format_unref (fmt);

	/* Markup formats cannot render numbers.  */
	fmt = go_format_new_from_XL ("@[bold=1:0:2]");
	str = format_value_for (fmt, 1);
	g_assert_cmpstr (str, ==, "#####");
	g_free (str);
	go_format_unref (fmt);

	/* An empty format contributes no characters.  */
	fmt = go_format_empty ();
	str = format_value_for (fmt, 1);
	g_assert_cmpstr (str, ==, "");
	g_free (str);

	/* A general format uses the general renderer.  */
	fmt = go_format_general ();
	str = format_value_for (fmt, 1.0 / 3);
	g_assert_cmpstr (str, ==, "0.3333333333333333");
	g_free (str);
	/* Both formats above are borrowed singletons.  */
}

static void
test_format_value_gstring_types (void)
{
	assert_rendered ("@", 0, 'S', "hello", NULL, -1, NULL, FALSE,
			 GO_FORMAT_NUMBER_OK, "hello");
	assert_rendered ("@", 0, 'B', "TRUE", NULL, -1, NULL, FALSE,
			 GO_FORMAT_NUMBER_OK, "TRUE");

	/* Non-text formats simply copy the string value.  */
	assert_rendered ("0", 0, 'S', "abc", NULL, -1, NULL, FALSE,
			 GO_FORMAT_NUMBER_OK, "abc");

	/* The conditional part chosen depends on the value type.  */
	assert_rendered ("[>10]0;@", 20, 'S', "text", NULL, -1, NULL, FALSE,
			 GO_FORMAT_NUMBER_OK, "text");
	assert_rendered ("[>10]0;@", 20, 'F', NULL, NULL, -1, NULL, FALSE,
			 GO_FORMAT_NUMBER_OK, "20");

	/* A non-float type copies its string value, even for an empty format.  */
	assert_rendered ("", 0, 'S', "ignored", NULL, -1, NULL, FALSE,
			 GO_FORMAT_NUMBER_OK, "ignored");
}

/* ALLOW_NEGATIVE_TIMES is a build-time option without an accessor.  It
 * governs negative dates as well as negative elapsed times, so probe both
 * kinds of value.  */
static gboolean
negative_date_times_allowed (void)
{
	GOFormat *date = new_format ("yyyy-mm-dd");
	GOFormat *time = new_format ("[h]:mm:ss");
	GString *str = g_string_new (NULL);
	gboolean allowed;

	allowed = render_value (date, str, -1, 'F', NULL, NULL, -1, NULL, FALSE)
		== GO_FORMAT_NUMBER_OK;
	if (allowed)
		allowed = render_value (time, str, -1, 'F', NULL, NULL,
					-1, NULL, FALSE) == GO_FORMAT_NUMBER_OK;

	g_string_free (str, TRUE);
	go_format_unref (time);
	go_format_unref (date);
	return allowed;
}

static void
test_format_value_dates (void)
{
	GODateConventions conventional = { FALSE };
	GODateConventions mac = { TRUE };

	assert_rendered ("yyyy-mm-dd", 1, 'F', NULL, NULL, -1,
			 &conventional, FALSE,
			 GO_FORMAT_NUMBER_OK, "1900-01-01");
	assert_rendered ("yyyy-mm-dd hh:mm", 1.5, 'F', NULL, NULL, -1,
			 &conventional, FALSE,
			 GO_FORMAT_NUMBER_OK, "1900-01-01 12:00");
	assert_rendered ("yyyy-mm-dd", 1, 'F', NULL, NULL, -1,
			 &mac, FALSE,
			 GO_FORMAT_NUMBER_OK, "1904-01-02");
}

static void
test_format_value_negative_date_time (void)
{
	if (!negative_date_times_allowed ()) {
		g_test_skip ("negative dates and times are disabled");
		return;
	}

	assert_rendered ("yyyy-mm-dd", -1, 'F', NULL, NULL, -1, NULL, FALSE,
			 GO_FORMAT_NUMBER_OK, "1899-12-30");
	assert_rendered ("[h]:mm:ss", -1, 'F', NULL, NULL, -1, NULL, FALSE,
			 GO_FORMAT_NUMBER_OK, "-24:00:00");
}

static void
test_format_value_colors (void)
{
	GOFormat *fmt = new_format ("[Red]0.00;[Blue]-0.00;@");
	GString *str = g_string_new (NULL);
	GOColor color = 0;

	g_assert_cmpint (render_value (fmt, str, 12.5, 'F', NULL, &color,
				       -1, NULL, FALSE), ==,
			 GO_FORMAT_NUMBER_OK);
	g_assert_cmpstr (str->str, ==, "12.50");
	g_assert_true (color == GO_COLOR_RED);

	g_assert_cmpint (render_value (fmt, str, -12.5, 'F', NULL, &color,
				       -1, NULL, FALSE), ==,
			 GO_FORMAT_NUMBER_OK);
	g_assert_cmpstr (str->str, ==, "-12.50");
	g_assert_true (color == GO_COLOR_BLUE);

	g_string_free (str, TRUE);
	go_format_unref (fmt);
}

static void
test_format_value_unicode_minus (void)
{
	assert_rendered ("0.0", -1.25, 'F', NULL, NULL, -1, NULL, FALSE,
			 GO_FORMAT_NUMBER_OK, "-1.3");
	assert_rendered ("0.0", -1.25, 'F', NULL, NULL, -1, NULL, TRUE,
			 GO_FORMAT_NUMBER_OK, UNICODE_MINUS "1.3");
	assert_rendered ("0.00E+00", -1.25, 'F', NULL, NULL, -1, NULL, TRUE,
			 GO_FORMAT_NUMBER_OK, UNICODE_MINUS "1.25E+00");
}

static void
test_format_value_pi_slash (void)
{
	GOFormat *fmt;
	GOFormatDetails details;
	char *expected;
	char *str;

	if (!go_format_allow_pi_slash ()) {
		g_test_skip ("pi-based fraction formats are disabled");
		return;
	}

	fmt = new_format ("0.00 pi/180");
	str = format_value_for (fmt, G_PI_2);
	expected = localize_number ("90.00 " PI_SYMBOL "/180");
	g_assert_cmpstr (str, ==, expected);

	g_free (expected);
	g_free (str);

	go_format_get_details (fmt, &details, NULL);
	g_assert_cmpint (details.family, ==, GO_FORMAT_FRACTION);
	g_assert_true (details.pi_scale);
	go_format_details_finalize (&details);
	go_format_unref (fmt);
}

static void
test_format_value_fill (void)
{
	GOFormat *fmt = new_format ("*0.00");
	GString *str = g_string_new (NULL);
	char *expected = localize_number ("00000.50");

	g_assert_cmpint (render_value (fmt, str, 1.5, 'F', NULL, NULL,
				       8, NULL, FALSE), ==, GO_FORMAT_NUMBER_OK);
	g_assert_cmpstr (str->str, ==, expected);

	g_free (expected);
	g_string_free (str, TRUE);
	go_format_unref (fmt);
}

/* ------------------------------------------------------------------------- */
/* Width measurement                                                         */
/* ------------------------------------------------------------------------- */

static void
test_format_measures (void)
{
	GString *str = g_string_new ("abc");
	PangoFontMap *font_map = pango_cairo_font_map_new ();
	PangoContext *context = pango_font_map_create_context (font_map);
	PangoLayout *layout = pango_layout_new (context);

	g_assert_cmpint (go_format_measure_zero (str, layout), ==, 0);
	g_assert_cmpint (go_format_measure_strlen (str, layout), ==, 3);

	/* Unlike the other helpers, this measures the layout, not @str.  */
	pango_layout_set_text (layout, str->str, -1);
	g_assert_cmpint (go_format_measure_pango (str, layout), >, 0);

	g_string_assign (str, "\xc3\xa5""bc");
	g_assert_cmpint (go_format_measure_strlen (str, layout), ==, 3);

	g_string_free (str, TRUE);
	g_object_unref (layout);
	g_object_unref (context);
	g_object_unref (font_map);
}

/* ------------------------------------------------------------------------- */
/* Precision helpers                                                         */
/* ------------------------------------------------------------------------- */

static void
assert_precision_change (GOFormat *(*change) (GOFormat const *),
			 const char *from, const char *to)
{
	GOFormat *fmt = new_format (from);
	GOFormat *res = change (fmt);

	if (to) {
		g_assert_nonnull (res);
		assert_format_string (res, to);
		go_format_unref (res);
	} else {
		g_assert_null (res);
	}

	go_format_unref (fmt);
}

static void
test_format_precision (void)
{
	assert_precision_change (go_format_inc_precision, "0", "0.0");
	assert_precision_change (go_format_inc_precision, "0.0", "0.00");
	assert_precision_change (go_format_inc_precision, "0.00", "0.000");
	assert_precision_change (go_format_inc_precision, "#,##0.00", "#,##0.000");
	assert_precision_change (go_format_inc_precision, "0.00%", "0.000%");
	assert_precision_change (go_format_inc_precision, "0.00E+00", "0.000E+00");
	assert_precision_change (go_format_inc_precision, "General", NULL);
	assert_precision_change (go_format_inc_precision, "@", NULL);
	assert_precision_change (go_format_inc_precision, "#", "#.0");

	assert_precision_change (go_format_dec_precision, "0.00", "0.0");
	assert_precision_change (go_format_dec_precision, "0.0", "0");
	assert_precision_change (go_format_dec_precision, "#,##0.00", "#,##0.0");
	assert_precision_change (go_format_dec_precision, "0.00%", "0.0%");
	assert_precision_change (go_format_dec_precision, "0.00E+00", "0.0E+00");
	assert_precision_change (go_format_dec_precision, "0", NULL);
	assert_precision_change (go_format_dec_precision, "General", NULL);
	assert_precision_change (go_format_dec_precision, "@", NULL);
	assert_precision_change (go_format_dec_precision, "#", NULL);

	assert_precision_change (go_format_toggle_1000sep, "0.00", "#,##0.00");
	assert_precision_change (go_format_toggle_1000sep, "#,##0.00", "0.00");
	assert_precision_change (go_format_toggle_1000sep, "0", "#,##0");
	assert_precision_change (go_format_toggle_1000sep, "General", "#,##0");
	assert_precision_change (go_format_toggle_1000sep, "0%", "#,##0%");
}

/* ------------------------------------------------------------------------- */
/* Format details                                                            */
/* ------------------------------------------------------------------------- */

static void
test_generate_number_str (void)
{
	GString *str = g_string_new (NULL);

	go_format_generate_number_str (str, 0, 0, FALSE, FALSE, FALSE,
				       NULL, NULL);
	g_assert_cmpstr (str->str, ==, "#");
	g_string_truncate (str, 0);

	go_format_generate_number_str (str, 1, 2, FALSE, FALSE, FALSE,
				       NULL, NULL);
	g_assert_cmpstr (str->str, ==, "0.00");
	g_string_truncate (str, 0);

	go_format_generate_number_str (str, 1, 2, TRUE, FALSE, FALSE,
				       NULL, NULL);
	g_assert_cmpstr (str->str, ==, "#,##0.00");
	g_string_truncate (str, 0);

	go_format_generate_number_str (str, 4, 0, TRUE, FALSE, FALSE,
				       NULL, NULL);
	g_assert_cmpstr (str->str, ==, "0,000");
	g_string_truncate (str, 0);

	go_format_generate_number_str (str, 1, 2, FALSE, TRUE, FALSE,
				       NULL, NULL);
	g_assert_cmpstr (str->str, ==, "0.00;[Red]0.00");
	g_string_truncate (str, 0);

	go_format_generate_number_str (str, 1, 2, FALSE, FALSE, TRUE,
				       NULL, NULL);
	g_assert_cmpstr (str->str, ==, "0.00_);(0.00)");
	g_string_truncate (str, 0);

	go_format_generate_number_str (str, 1, 2, TRUE, TRUE, TRUE,
				       "$", "%");
	g_assert_cmpstr (str->str, ==, "$#,##0.00%_);[Red]($#,##0.00%)");
	g_string_truncate (str, 0);

	/* Out-of-range counts are clamped.  */
	go_format_generate_number_str (str, -5, -5, FALSE, FALSE, FALSE,
				       NULL, NULL);
	g_assert_cmpstr (str->str, ==, "#");

	g_string_free (str, TRUE);
}

static void
test_details_defaults (void)
{
	GOFormatDetails details;
	unsigned i;

	for (i = GO_FORMAT_GENERAL; i <= GO_FORMAT_SPECIAL; i++) {
		GOFormatFamily family = (GOFormatFamily)i;

		go_format_details_init (&details, family);
		g_assert_cmpint (details.family, ==, (int)family);
		g_assert_cmpint (details.magic, ==, GO_FORMAT_MAGIC_NONE);
		g_assert_cmpint (details.num_decimals, ==, 0);
		g_assert_cmpint (details.exponent_step, ==, 1);
		g_assert_cmpint (details.exponent_digits, ==, 2);
		g_assert_false (details.exponent_sign_forced);
		g_assert_cmpint (details.min_digits, ==,
				 family == GO_FORMAT_FRACTION ? 0 : 1);
		g_assert_true (details.split_fraction);
		g_assert_cmpint (details.denominator_max_digits, ==, 1);
		g_assert_cmpint (details.denominator, ==, 8);
		g_assert_true (details.automatic_denominator);
		g_assert_cmpint (details.thousands_sep, ==,
				 family == GO_FORMAT_CURRENCY ||
				 family == GO_FORMAT_ACCOUNTING);
		g_assert_null (details.appended_SI_unit);
	}
}

static void
test_details_generate (void)
{
	struct {
		GOFormatFamily family;
		const char *expected;
	} const cases[] = {
		{ GO_FORMAT_GENERAL, "General" },
		{ GO_FORMAT_NUMBER, "0" },
		{ GO_FORMAT_PERCENTAGE, "0%" },
		{ GO_FORMAT_SCIENTIFIC, "0E-00" },
		{ GO_FORMAT_FRACTION, "# ?/?" },
		{ GO_FORMAT_TEXT, "@" },
		{ GO_FORMAT_DATE, "" },
		{ GO_FORMAT_TIME, "" },
		{ GO_FORMAT_SPECIAL, "" },
		{ GO_FORMAT_UNKNOWN, "" }
	};
	GOFormatDetails *details;
	GString *str = g_string_new (NULL);
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		details = go_format_details_new (cases[i].family);
		g_string_truncate (str, 0);
		go_format_generate_str (str, details);
		g_assert_cmpstr (str->str, ==, cases[i].expected);
		go_format_details_free (details);
	}

	/* CURRENCY and ACCOUNTING need an explicit currency to be interesting. */
	details = go_format_details_new (GO_FORMAT_CURRENCY);
	details->currency = _go_format_currencies () + 1;
	details->num_decimals = 2;
	g_string_truncate (str, 0);
	go_format_generate_str (str, details);
	g_assert_cmpstr (str->str, ==, "$#,##0.00");
	go_format_details_free (details);

	details = go_format_details_new (GO_FORMAT_ACCOUNTING);
	details->currency = _go_format_currencies () + 1;
	details->num_decimals = 2;
	g_string_truncate (str, 0);
	go_format_generate_str (str, details);
	g_assert_cmpstr (str->str, ==,
		"_($* #,##0.00_);_($* (#,##0.00);_($* \"-\"??_);_(@_)");
	go_format_details_free (details);

	/* Fractions, fixed and automatic denominators.  */
	details = go_format_details_new (GO_FORMAT_FRACTION);
	details->split_fraction = FALSE;
	details->automatic_denominator = FALSE;
	details->denominator = 16;
	details->numerator_min_digits = 1;
	g_string_truncate (str, 0);
	go_format_generate_str (str, details);
	g_assert_cmpstr (str->str, ==, "?0/16");
	go_format_details_free (details);

	g_string_free (str, TRUE);
}

static void
test_details_generate_ee (void)
{
	GOFormatDetails *details;
	GString *str;

	if (!go_format_allow_ee_markup ()) {
		g_test_skip ("EE markup is disabled");
		return;
	}

	details = go_format_details_new (GO_FORMAT_SCIENTIFIC);
	details->exponent_step = 3;
	details->exponent_digits = 3;
	details->exponent_sign_forced = TRUE;
	details->simplify_mantissa = TRUE;
	details->scale = 3;
	details->use_markup = TRUE;

	str = g_string_new (NULL);
	go_format_generate_str (str, details);
	g_assert_cmpstr (str->str, ==, "###'EE+000");

	g_string_free (str, TRUE);
	go_format_details_free (details);
}

static void
test_get_details (void)
{
	struct {
		const char *format;
		GOFormatFamily family;
		gboolean exact;
	} const cases[] = {
		{ "0.00", GO_FORMAT_NUMBER, TRUE },
		{ "#,##0.0", GO_FORMAT_NUMBER, TRUE },
		{ "0.00%", GO_FORMAT_PERCENTAGE, TRUE },
		{ "0.00E+00", GO_FORMAT_SCIENTIFIC, TRUE },
		{ "# ?/?", GO_FORMAT_FRACTION, TRUE },
		{ "# ?\?/?\?", GO_FORMAT_FRACTION, TRUE },
		{ "1 1/2", GO_FORMAT_FRACTION, FALSE },
		{ "@", GO_FORMAT_TEXT, FALSE },
		{ "General", GO_FORMAT_GENERAL, FALSE },
		{ "mm/dd/yyyy", GO_FORMAT_DATE, FALSE },
		{ "hh:mm", GO_FORMAT_TIME, FALSE }
	};
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (cases); i++) {
		GOFormat *fmt = new_format (cases[i].format);
		GOFormatDetails details;
		gboolean exact = FALSE;
		GString *str = g_string_new (NULL);

		go_format_get_details (fmt, &details, &exact);

		g_assert_cmpint (details.family, ==, cases[i].family);
		g_assert_cmpint (exact, ==, cases[i].exact);

		if (exact) {
			char *localized = localize_format (cases[i].format);
			go_format_generate_str (str, &details);
			g_assert_cmpstr (str->str, ==, localized);
			g_free (localized);
		}

		g_string_free (str, TRUE);
		go_format_details_finalize (&details);
		go_format_unref (fmt);
	}
}

static void
test_get_details_si (void)
{
	GOFormat *fmt;
	GOFormatDetails details;
	gboolean exact = FALSE;
	GString *str;

	if (!go_format_allow_si ()) {
		g_test_skip ("SI suffixes are disabled");
		return;
	}

	fmt = new_format ("0.00ESI+00\"m\"");
	go_format_get_details (fmt, &details, &exact);
	g_assert_cmpint (details.family, ==, GO_FORMAT_SCIENTIFIC);
	g_assert_true (exact);

	str = g_string_new (NULL);
	go_format_generate_str (str, &details);
	g_assert_cmpstr (str->str, ==, "0.00ESI+00\"m\"");

	g_string_free (str, TRUE);
	go_format_details_finalize (&details);
	go_format_unref (fmt);
}

static void
test_get_details_fields (void)
{
	GOFormat *fmt;
	GOFormatDetails details;

	fmt = new_format ("#,##0.000");
	go_format_get_details (fmt, &details, NULL);
	g_assert_cmpint (details.min_digits, ==, 1);
	g_assert_cmpint (details.num_decimals, ==, 3);
	g_assert_true (details.thousands_sep);
	g_assert_false (details.negative_red);
	g_assert_false (details.negative_paren);
	go_format_unref (fmt);

	fmt = new_format ("#,##0.00;[Red](#,##0.00)");
	go_format_get_details (fmt, &details, NULL);
	g_assert_true (details.thousands_sep);
	g_assert_true (details.negative_red);
	g_assert_false (details.negative_paren);
	go_format_unref (fmt);

	fmt = new_format ("_($* #,##0.00_);_($* (#,##0.00);_($* \"-\"??_);_(@_)");
	go_format_get_details (fmt, &details, NULL);
	g_assert_cmpint (details.family, ==, GO_FORMAT_ACCOUNTING);
	g_assert_true (details.thousands_sep);
	g_assert_true (details.negative_paren);
	go_format_unref (fmt);

	fmt = new_format ("$#,##0.00");
	go_format_get_details (fmt, &details, NULL);
	g_assert_cmpint (details.family, ==, GO_FORMAT_CURRENCY);
	g_assert_nonnull (details.currency);
	g_assert_cmpstr (details.currency->symbol, ==, "$");
	g_assert_true (details.currency->precedes);
	go_format_unref (fmt);

	fmt = new_format ("hh:mm:ss.000");
	go_format_get_details (fmt, &details, NULL);
	g_assert_cmpint (details.family, ==, GO_FORMAT_TIME);
	g_assert_cmpint (details.num_decimals, ==, 3);
	go_format_unref (fmt);

}

static void
test_get_details_fields_si (void)
{
	GOFormat *fmt;
	GOFormatDetails details;
	gboolean exact = FALSE;

	if (!go_format_allow_si ()) {
		g_test_skip ("SI suffixes are disabled");
		return;
	}

	fmt = new_format ("0.00ESI+00\"m\"");
	/* The SI unit is only recovered when "exact" is requested.  */
	go_format_get_details (fmt, &details, &exact);
	g_assert_true (exact);
	g_assert_true (details.append_SI);
	g_assert_cmpstr (details.appended_SI_unit, ==, "m");
	g_assert_true (details.exponent_sign_forced);
	g_assert_cmpint (details.exponent_digits, ==, 2);
	go_format_details_finalize (&details);
	go_format_unref (fmt);
}

static void
test_details_boxed_type (void)
{
	GType type = go_format_details_get_type ();
	GOFormatDetails *details = go_format_details_new (GO_FORMAT_NUMBER);
	GOFormatDetails *copy = g_boxed_copy (type, details);

	g_assert_true (type != G_TYPE_INVALID);
	g_assert_true (copy == details);
	g_assert_cmpuint (copy->ref_count, ==, 2);

	g_boxed_free (type, copy);
	g_assert_cmpuint (details->ref_count, ==, 1);
	go_format_details_free (details);

	go_format_details_free (NULL);
}

static void
test_format_boxed_type (void)
{
	GType type = go_format_get_type ();
	GOFormat *fmt = new_format ("0.0000");
	GOFormat *copy = g_boxed_copy (type, fmt);

	g_assert_true (type != G_TYPE_INVALID);
	g_assert_true (copy == fmt);

	g_boxed_free (type, copy);
	go_format_unref (fmt);
}

/* ------------------------------------------------------------------------- */
/* Localization                                                              */
/* ------------------------------------------------------------------------- */

static void
test_format_localization (void)
{
	char *str;
	GOFormat *fmt;

	str = go_format_str_localize ("#,##0.00");
	g_assert_nonnull (str);
	g_assert_cmpstr (str, ==, "#,##0.00");
	g_free (str);

	str = go_format_str_localize ("General");
	g_assert_nonnull (str);
	g_assert_cmpstr (str, ==, "General");
	g_free (str);

	str = go_format_str_localize ("[Red]0.00");
	g_assert_nonnull (str);
	g_assert_cmpstr (str, ==, "[Red]0.00");
	g_free (str);

	str = go_format_str_delocalize ("#,##0.00");
	g_assert_nonnull (str);
	g_assert_cmpstr (str, ==, "#,##0.00");
	g_free (str);

	str = go_format_str_delocalize ("General");
	g_assert_nonnull (str);
	g_assert_cmpstr (str, ==, "General");
	g_free (str);

	str = go_format_str_localize ("[");
	g_assert_null (str);
	str = go_format_str_delocalize ("[");
	g_assert_null (str);

	/* A localize/delocalize round trip preserves simple number formats.  */
	str = go_format_str_localize ("#,##0.00");
	fmt = go_format_new_from_XL (str);
	g_assert_nonnull (fmt);
	g_free (str);
	assert_format_string (fmt, "#,##0.00");
	go_format_unref (fmt);
}

/* ------------------------------------------------------------------------- */
/* Palette and currencies                                                    */
/* ------------------------------------------------------------------------- */

static void
test_format_palette (void)
{
	static const char *const names[] = {
		"Black", "White", "Red", "Green", "Blue", "Yellow",
		"Magenta", "Cyan"
	};
	int i;
	char *name;

	for (i = 1; i <= 56; i++) {
		GOColor color = go_format_palette_color_of_index (i);
		int index;

		g_assert_cmpint (color, !=, 0);
		index = go_format_palette_index_from_color (color);
		g_assert_cmpint (index, >=, 1);
		g_assert_cmpint (index, <=, 56);
		g_assert_true (go_format_palette_color_of_index (index) == color);

		name = go_format_palette_name_of_index (i);
		g_assert_nonnull (name);
		if (i <= 8)
			g_assert_cmpstr (name, ==, names[i - 1]);
		else {
			char *expected = g_strdup_printf ("Color%d", i);
			g_assert_cmpstr (name, ==, expected);
			g_free (expected);
		}
		g_free (name);
	}

	/* Out-of-range colors are documented to map to zero.  */
	g_assert_cmpint (go_format_palette_color_of_index (57), ==, 0);

	g_assert_cmpint (go_format_palette_index_from_color (
		GO_COLOR_FROM_RGB (0x12, 0x34, 0x56)), >=, 1);
}

static void
test_format_currencies (void)
{
	GOFormatCurrency const *currencies = _go_format_currencies ();
	GOFormatCurrency const *currency = go_format_locale_currency ();
	GType type = go_format_currency_get_type ();
	GOFormatCurrency *copy;
	int i;

	g_assert_nonnull (currencies);
	g_assert_nonnull (currency);
	g_assert_nonnull (currency->symbol);
	g_assert_nonnull (currency->description);

	g_assert_cmpstr (currencies[0].symbol, ==, "");
	g_assert_cmpstr (currencies[1].symbol, ==, "$");
	g_assert_true (currencies[1].precedes);

	for (i = 1; currencies[i].symbol; i++) {
		g_assert_cmpuint (strlen (currencies[i].symbol), >, 0);
		g_assert_nonnull (currencies[i].description);
	}
	g_assert_cmpint (i, >, 1);

	g_assert_true (type != G_TYPE_INVALID);
	copy = g_boxed_copy (type, currency);
	g_assert_true (copy == currency);
	g_boxed_free (type, copy);
}

/* ------------------------------------------------------------------------- */
/* Built-in formats, markup, and ODF                                         */
/* ------------------------------------------------------------------------- */

static void
test_format_builtins (void)
{
	int fam;

	for (fam = GO_FORMAT_GENERAL; fam <= GO_FORMAT_TEXT; fam++) {
		char const *const *builtins = _go_format_builtins ((GOFormatFamily)fam);
		int i;

		g_assert_nonnull (builtins);
		for (i = 0; builtins[i]; i++) {
			GOFormat *fmt = go_format_new_from_XL (builtins[i]);
			g_assert_nonnull (fmt);
			g_assert_false (go_format_is_invalid (fmt));
			go_format_unref (fmt);
		}
		g_assert_cmpint (i, >, 0);
	}

	g_assert_null (_go_format_builtins (GO_FORMAT_SPECIAL));
	g_assert_null (_go_format_builtins (GO_FORMAT_UNKNOWN));
}

static gboolean
count_attribute (PangoAttribute *attr, gpointer user_data)
{
	unsigned *count = user_data;
	(void)attr;
	(*count)++;
	/* Keep the attribute: go_format_get_markup returns a borrowed list.  */
	return FALSE;
}

static void
test_format_markup (void)
{
	PangoAttrList *attrs = pango_attr_list_new ();
	PangoAttribute *attr;
	GOFormat *fmt;
	const PangoAttrList *got;

	attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
	attr->start_index = 0;
	attr->end_index = 3;
	pango_attr_list_insert (attrs, attr);

	attr = pango_attr_foreground_new (0xff00, 0x0000, 0x0000);
	attr->start_index = 1;
	attr->end_index = 2;
	pango_attr_list_insert (attrs, attr);

	fmt = go_format_new_markup (attrs, FALSE);
	g_assert_nonnull (fmt);
	g_assert_true (go_format_is_markup (fmt));
	g_assert_true (go_format_is_simple (fmt));
	g_assert_cmpint (go_format_get_family (fmt), ==, GO_FORMAT_MARKUP);

	got = go_format_get_markup (fmt);
	g_assert_nonnull (got);
	g_assert_true (got != attrs);

	{
		unsigned count = 0;
		pango_attr_list_filter ((PangoAttrList *)got,
					(PangoAttrFilterFunc)count_attribute,
					&count);
		g_assert_cmpuint (count, >, 0);
	}

	go_format_unref (fmt);

	fmt = go_format_new_from_XL ("@[bold=1:0:2]");
	g_assert_true (go_format_is_markup (fmt));
	g_assert_nonnull (go_format_get_markup (fmt));
	g_assert_true (g_str_has_prefix (go_format_as_XL (fmt), "@["));
	go_format_unref (fmt);
}

static void
test_odf_style_map (void)
{
	GOFormat *fmt = new_format ("[>10]0.00;[<=10]0");
	char *str;

	str = go_format_odf_style_map (fmt, 0);
	g_assert_nonnull (str);
	g_assert_cmpstr (str, ==, "value()>10");
	g_free (str);

	str = go_format_odf_style_map (fmt, 1);
	g_assert_nonnull (str);
	g_assert_cmpstr (str, ==, "value()<=10");
	g_free (str);

	str = go_format_odf_style_map (fmt, -1);
	g_assert_null (str);

	go_format_unref (fmt);
}

static void
test_odf_output (void)
{
	char *xml;

	xml = odf_output ("0.00", 0, "N1", FALSE, FALSE, TRUE);
	assert_contains (xml, "<number:number-style style:name=\"N1\">");
	assert_contains (xml, "number:decimal-places=\"2\"");
	assert_contains (xml, "number:grouping=\"false\"");
	assert_contains (xml, "number:min-integer-digits=\"1\"");
	g_free (xml);

	xml = odf_output ("#,##0.00", 0, "N2", FALSE, TRUE, TRUE);
	assert_contains (xml, "number:grouping=\"true\"");
	g_free (xml);

	xml = odf_output ("0.00%", 0, "P1", TRUE, FALSE, TRUE);
	assert_contains (xml, "<number:percentage-style style:name=\"P1\">");
	assert_contains (xml, "<number:text>%</number:text>");
	g_free (xml);

	xml = odf_output ("yyyy-mm-dd", 0, "D1", FALSE, FALSE, TRUE);
	assert_contains (xml, "<number:date-style style:name=\"D1\"");
	assert_contains (xml, "number:format-source=\"fixed\"");
	assert_contains (xml, "<number:year");
	g_free (xml);

	/* General formats are written, but reported as not fully supported.  */
	xml = odf_output ("General", 0, "G1", TRUE, FALSE, FALSE);
	assert_contains (xml, "<number:number-style style:name=\"G1\">");
	g_free (xml);

	xml = odf_output ("[>10]0.00;[<=10]0", 0, "C1", TRUE, FALSE, TRUE);
	assert_contains (xml, "style:name=\"C1-0\"");
	assert_contains (xml, "style:name=\"C1-1\"");
	assert_contains (xml, "style:condition=\"value()&gt;10\"");
	assert_contains (xml, "style:condition=\"value()&lt;=10\"");
	g_free (xml);
}

static void
test_allowed_extensions (void)
{
	/* These are configuration switches: exercise them without asserting
	 * which extensions happen to be enabled.  */
	(void)go_format_allow_ee_markup ();
	(void)go_format_allow_si ();
	(void)go_format_allow_pi_slash ();
}

/* ------------------------------------------------------------------------- */

static void
register_format_tests (void)
{
	g_test_add_func ("/utils/go-format/general", test_general_format);
	g_test_add_func ("/utils/go-format/general/unicode-minus",
		test_general_format_unicode_minus);

	g_test_add_func ("/utils/go-format/creation", test_format_creation);
	g_test_add_func ("/utils/go-format/special-strings",
		test_format_special_strings);
	g_test_add_func ("/utils/go-format/singletons", test_format_singletons);
	g_test_add_func ("/utils/go-format/foreach", test_format_foreach);

	g_test_add_func ("/utils/go-format/predicates", test_format_predicates);
	g_test_add_func ("/utils/go-format/families", test_format_families);
	g_test_add_func ("/utils/go-format/date-time-predicates",
		test_format_date_time_predicates);

	g_test_add_func ("/utils/go-format/magic", test_format_magic);

	g_test_add_func ("/utils/go-format/specialize/simple",
		test_format_specialize_simple);
	g_test_add_func ("/utils/go-format/specialize/conditions",
		test_format_specialize_conditions);

	g_test_add_func ("/utils/go-format/value/numbers",
		test_format_value_numbers);
	g_test_add_func ("/utils/go-format/value/special",
		test_format_value_special);
	g_test_add_func ("/utils/go-format/value/types",
		test_format_value_gstring_types);
	g_test_add_func ("/utils/go-format/value/dates",
		test_format_value_dates);
	g_test_add_func ("/utils/go-format/value/negative-date-time",
		test_format_value_negative_date_time);
	g_test_add_func ("/utils/go-format/value/colors",
		test_format_value_colors);
	g_test_add_func ("/utils/go-format/value/unicode-minus",
		test_format_value_unicode_minus);
	g_test_add_func ("/utils/go-format/value/fill",
		test_format_value_fill);
	g_test_add_func ("/utils/go-format/value/pi-slash",
		test_format_value_pi_slash);

	g_test_add_func ("/utils/go-format/measures", test_format_measures);
	g_test_add_func ("/utils/go-format/precision", test_format_precision);

	g_test_add_func ("/utils/go-format/details/generate-number-str",
		test_generate_number_str);
	g_test_add_func ("/utils/go-format/details/defaults",
		test_details_defaults);
	g_test_add_func ("/utils/go-format/details/generate",
		test_details_generate);
	g_test_add_func ("/utils/go-format/details/generate-ee",
		test_details_generate_ee);
	g_test_add_func ("/utils/go-format/details/get", test_get_details);
	g_test_add_func ("/utils/go-format/details/get-si", test_get_details_si);
	g_test_add_func ("/utils/go-format/details/fields",
		test_get_details_fields);
	g_test_add_func ("/utils/go-format/details/fields-si",
		test_get_details_fields_si);
	g_test_add_func ("/utils/go-format/details/boxed-type",
		test_details_boxed_type);
	g_test_add_func ("/utils/go-format/boxed-type", test_format_boxed_type);

	g_test_add_func ("/utils/go-format/localization",
		test_format_localization);
	g_test_add_func ("/utils/go-format/palette", test_format_palette);
	g_test_add_func ("/utils/go-format/currencies",
		test_format_currencies);

	g_test_add_func ("/utils/go-format/builtins", test_format_builtins);
	g_test_add_func ("/utils/go-format/markup", test_format_markup);
	g_test_add_func ("/utils/go-format/odf/style-map", test_odf_style_map);
	g_test_add_func ("/utils/go-format/odf/output", test_odf_output);
	g_test_add_func ("/utils/go-format/extensions", test_allowed_extensions);
}

int
main (int argc, char **argv)
{
	/*
	 * Formats and numbers are rendered using locale-dependent separators.
	 * Pin the locale so the expectations above are reproducible.
	 */
	setlocale (LC_ALL, "C");

	g_test_init (&argc, &argv, NULL);
	libgoffice_init ();

	register_format_tests ();

	int res = g_test_run ();
	libgoffice_shutdown ();
	return res;
}
