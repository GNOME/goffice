#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/data/goffice-data.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-locale.h>
#include <math.h>

static void
count_changes (G_GNUC_UNUSED GObject *object, gpointer data)
{
	guint *count = data;
	(*count)++;
}

typedef struct {
	guint calls;
	guint destroys;
} TranslateState;

static char const *
translate_string (char const *str, gpointer data)
{
	TranslateState *state = data;
	state->calls++;
	return str;
}

static void
count_notify (gpointer data)
{
	TranslateState *state = data;
	state->destroys++;
}

static void
test_scalar_val (void)
{
	GOData *data = go_data_scalar_val_new (42.5);
	GOData *copy = go_data_dup (data);
	char *str;

	g_assert_true (GO_IS_DATA_SCALAR_VAL (data));
	g_assert_cmpuint (go_data_get_n_dimensions (data), ==, 0);
	g_assert_cmpuint (go_data_get_n_values (data), ==, 1);
	g_assert_cmpfloat (go_data_get_scalar_value (data), ==, 42.5);
	g_assert_true (go_data_eq (data, copy));

	str = go_data_get_scalar_string (data);
	g_assert_cmpstr (str, ==, "42.5");
	g_free (str);

	g_object_unref (copy);
	g_object_unref (data);
}

static void
test_scalar_str (void)
{
	GOData *data = go_data_scalar_str_new_copy ("hello");
	GOData *copy = go_data_dup (data);
	GOData *other = go_data_scalar_str_new_copy ("world");
	guint changes = 0;

	g_assert_true (GO_IS_DATA_SCALAR_STR (data));
	g_assert_cmpstr (go_data_scalar_get_str (GO_DATA_SCALAR (data)), ==, "hello");
	g_assert_cmpstr (go_data_scalar_get_str (GO_DATA_SCALAR (copy)), ==, "hello");
	g_assert_true (go_data_eq (data, copy));
	g_assert_false (go_data_eq (data, other));
	g_assert_true (isnan (go_data_get_scalar_value (data)));

	g_signal_connect (data, "changed", G_CALLBACK (count_changes), &changes);
	go_data_scalar_str_set_str (GO_DATA_SCALAR_STR (data), g_strdup ("world"), TRUE);
	g_assert_cmpuint (changes, ==, 1);
	g_assert_cmpstr (go_data_scalar_get_str (GO_DATA_SCALAR (data)), ==, "world");

	g_object_unref (other);
	g_object_unref (copy);
	g_object_unref (data);
}

static void
test_scalar_val_io (void)
{
	GOData *data = go_data_scalar_val_new (12.5);
	guint changes = 0;
	char *str;

	str = go_data_serialize (data, NULL);
	g_assert_cmpstr (str, ==, "12.5");
	g_free (str);

	g_signal_connect (data, "changed", G_CALLBACK (count_changes), &changes);
	g_assert_true (go_data_unserialize (data, "7.25", NULL));
	g_assert_cmpfloat (go_data_get_scalar_value (data), ==, 7.25);
	g_assert_cmpuint (changes, ==, 1);

	g_assert_false (go_data_unserialize (data, "not-a-number", NULL));
	g_assert_cmpfloat (go_data_get_scalar_value (data), ==, 7.25);
	g_assert_cmpuint (changes, ==, 1);

	g_object_unref (data);
}

static void
test_scalar_str_io (void)
{
	GOData *data = go_data_scalar_str_new_copy ("first");
	guint changes = 0;
	char *str;

	str = go_data_serialize (data, NULL);
	g_assert_cmpstr (str, ==, "first");
	g_free (str);

	g_signal_connect (data, "changed", G_CALLBACK (count_changes), &changes);
	g_assert_true (go_data_unserialize (data, "second", NULL));
	g_assert_cmpstr (go_data_scalar_get_str (GO_DATA_SCALAR (data)), ==, "second");
	g_assert_cmpuint (changes, ==, 1);

	g_object_unref (data);
}

static void
test_scalar_markup (void)
{
	GOData *val = go_data_scalar_val_new (1);
	GOData *str = go_data_scalar_str_new_copy ("text");

	g_assert_null (go_data_get_scalar_markup (val));
	g_assert_null (go_data_get_scalar_markup (str));

	g_object_unref (str);
	g_object_unref (val);
}

static void
test_vector_val (void)
{
	double values[] = { 1, 2, 4 };
	GOData *data = go_data_vector_val_new_copy (values, G_N_ELEMENTS (values));
	double *cached;
	double min, max;

	g_assert_true (GO_IS_DATA_VECTOR_VAL (data));
	g_assert_cmpuint (go_data_get_n_dimensions (data), ==, 1);
	g_assert_cmpuint (go_data_get_vector_size (data), ==, 3);
	g_assert_cmpuint (go_data_get_n_values (data), ==, 3);
	g_assert_cmpfloat (go_data_get_vector_value (data, 1), ==, 2);

	go_data_get_bounds (data, &min, &max);
	g_assert_cmpfloat (min, ==, 1);
	g_assert_cmpfloat (max, ==, 4);
	g_assert_true (go_data_is_increasing (data));
	g_assert_true (go_data_is_varying_uniformly (data));
	g_assert_false (go_data_is_decreasing (data));

	cached = go_data_get_values (data);
	g_assert_nonnull (cached);
	g_assert_cmpfloat (cached[2], ==, 4);

	g_object_unref (data);
}

static void
test_vector_str (void)
{
	char const *strings[] = { "1", "2", "not-a-number" };
	GOData *data = go_data_vector_str_new_copy (strings, G_N_ELEMENTS (strings));
	TranslateState state = { 0, 0 };
	char *str;

	g_assert_true (GO_IS_DATA_VECTOR_STR (data));
	g_assert_cmpint (go_data_vector_get_len (GO_DATA_VECTOR (data)), ==, 3);
	g_assert_cmpfloat (go_data_get_vector_value (data, 0), ==, 1);
	g_assert_cmpfloat (go_data_get_vector_value (data, 1), ==, 2);
	g_assert_true (isnan (go_data_get_vector_value (data, 2)));

	str = go_data_get_vector_string (data, 1);
	g_assert_cmpstr (str, ==, "2");
	g_free (str);

	go_data_vector_str_set_translate_func (GO_DATA_VECTOR_STR (data),
					       translate_string, &state, NULL);
	str = go_data_get_vector_string (data, 1);
	g_assert_cmpstr (str, ==, "2");
	g_assert_cmpuint (state.calls, ==, 1);
	g_free (str);

	g_object_unref (data);
}

static void
test_vector_val_io (void)
{
	double values[] = { 1, 2, 4 };
	GOData *data = go_data_vector_val_new_copy (values, G_N_ELEMENTS (values));
	GOData *copy = go_data_vector_val_new_copy (values, G_N_ELEMENTS (values));
	guint changes = 0;
	char *str;

	str = go_data_serialize (data, NULL);
	g_assert_true (go_data_unserialize (copy, str, NULL));
	g_free (str);

	g_assert_cmpuint (go_data_get_vector_size (copy), ==, 3);
	g_assert_cmpfloat (go_data_get_vector_value (copy, 0), ==, 1);
	g_assert_cmpfloat (go_data_get_vector_value (copy, 2), ==, 4);

	g_signal_connect (copy, "changed", G_CALLBACK (count_changes), &changes);
	g_assert_true (go_data_unserialize (copy, "5", NULL));
	g_assert_cmpuint (go_data_get_vector_size (copy), ==, 1);
	g_assert_cmpfloat (go_data_get_vector_value (copy, 0), ==, 5);
	g_assert_cmpuint (changes, ==, 1);

	g_assert_false (go_data_unserialize (copy, "x", NULL));
	g_assert_cmpuint (go_data_get_vector_size (copy), ==, 1);
	g_assert_cmpfloat (go_data_get_vector_value (copy, 0), ==, 5);
	g_assert_cmpuint (changes, ==, 1);

	g_object_unref (copy);
	g_object_unref (data);
}

static void
test_vector_val_dup (void)
{
	double values[] = { 1, 2, 4 };
	GOData *data = go_data_vector_val_new (values, G_N_ELEMENTS (values), NULL);
	GOData *copy = go_data_dup (data);

	g_assert_nonnull (copy);
	g_assert_cmpfloat (go_data_get_vector_value (copy, 0), ==, 1);

	/* go_data_dup() promises a deep copy.  */
	values[0] = 99;
	g_assert_cmpfloat (go_data_get_vector_value (data, 0), ==, 99);
	g_assert_cmpfloat (go_data_get_vector_value (copy, 0), ==, 1);

	g_object_unref (copy);
	g_object_unref (data);
}

static void
test_vector_val_nan_bounds (void)
{
	double values[] = { 1, go_nan, 4 };
	GOData *data = go_data_vector_val_new_copy (values, G_N_ELEMENTS (values));
	double min, max;

	go_data_get_bounds (data, &min, &max);
	g_assert_cmpfloat (min, ==, 1);
	g_assert_cmpfloat (max, ==, 4);
	g_assert_true (go_data_is_increasing (data));

	g_object_unref (data);
}

static void
test_vector_str_io (void)
{
	char const *strings[] = { "one", "two" };
	GOData *data = go_data_vector_str_new_copy (strings, G_N_ELEMENTS (strings));
	GOData *copy = go_data_vector_str_new_copy (strings, G_N_ELEMENTS (strings));
	guint changes = 0;
	char *str, *value;

	str = go_data_serialize (data, NULL);
	g_signal_connect (copy, "changed", G_CALLBACK (count_changes), &changes);
	g_assert_true (go_data_unserialize (copy, str, NULL));
	g_free (str);

	g_assert_cmpuint (changes, ==, 1);
	g_assert_cmpuint (go_data_get_vector_size (copy), ==, 2);
	value = go_data_get_vector_string (copy, 0);
	g_assert_cmpstr (value, ==, "one");
	g_free (value);
	value = go_data_get_vector_string (copy, 1);
	g_assert_cmpstr (value, ==, "two");
	g_free (value);

	g_object_unref (copy);
	g_object_unref (data);
}

static void
test_vector_str_quotes (void)
{
	char const *strings[] = { "a\"b", "c", "d\\e" };
	GOData *data = go_data_vector_str_new_copy (strings, G_N_ELEMENTS (strings));
	GOData *copy = go_data_vector_str_new_copy (strings, G_N_ELEMENTS (strings));
	char *str, *value;

	str = go_data_serialize (data, NULL);
	g_assert_true (go_data_unserialize (copy, str, NULL));
	g_free (str);

	g_assert_cmpuint (go_data_get_vector_size (copy), ==, 3);
	value = go_data_get_vector_string (copy, 0);
	g_assert_cmpstr (value, ==, "a\"b");
	g_free (value);
	value = go_data_get_vector_string (copy, 1);
	g_assert_cmpstr (value, ==, "c");
	g_free (value);
	value = go_data_get_vector_string (copy, 2);
	g_assert_cmpstr (value, ==, "d\\e");
	g_free (value);

	g_assert_false (go_data_unserialize (copy, "\"unterminated", NULL));
	g_assert_cmpuint (go_data_get_vector_size (copy), ==, 3);
	value = go_data_get_vector_string (copy, 0);
	g_assert_cmpstr (value, ==, "a\"b");
	g_free (value);

	g_object_unref (copy);
	g_object_unref (data);
}

static void
test_matrix_val (void)
{
	double values[] = { 1, 2, 3, 4 };
	GOData *data = go_data_matrix_val_new (values, 2, 2, NULL);
	unsigned rows, columns;
	double min, max;
	char *str;

	g_assert_true (GO_IS_DATA_MATRIX_VAL (data));
	g_assert_cmpuint (go_data_get_n_dimensions (data), ==, 2);
	g_assert_cmpuint (go_data_get_n_values (data), ==, 4);
	go_data_get_matrix_size (data, &rows, &columns);
	g_assert_cmpuint (rows, ==, 2);
	g_assert_cmpuint (columns, ==, 2);
	g_assert_cmpfloat (go_data_get_matrix_value (data, 0, 0), ==, 1);
	g_assert_cmpfloat (go_data_get_matrix_value (data, 0, 1), ==, 2);
	g_assert_cmpfloat (go_data_get_matrix_value (data, 1, 0), ==, 3);

	go_data_get_bounds (data, &min, &max);
	g_assert_cmpfloat (min, ==, 1);
	g_assert_cmpfloat (max, ==, 4);

	str = go_data_get_matrix_string (data, 1, 1);
	g_assert_cmpstr (str, ==, "4");
	g_free (str);

	g_object_unref (data);
}

static void
test_matrix_val_io (void)
{
	double values[] = { 1, 2, 3, 4 };
	GOData *data = go_data_matrix_val_new (values, 2, 2, NULL);
	GOData *copy = go_data_matrix_val_new (values, 2, 2, NULL);
	guint changes = 0;
	unsigned rows, columns;
	char *str;

	str = go_data_serialize (data, NULL);
	g_signal_connect (copy, "changed", G_CALLBACK (count_changes), &changes);
	g_assert_true (go_data_unserialize (copy, str, NULL));
	g_free (str);

	g_assert_cmpuint (changes, ==, 1);
	go_data_get_matrix_size (copy, &rows, &columns);
	g_assert_cmpuint (rows, ==, 2);
	g_assert_cmpuint (columns, ==, 2);
	g_assert_cmpfloat (go_data_get_matrix_value (copy, 0, 1), ==, 2);
	g_assert_cmpfloat (go_data_get_matrix_value (copy, 1, 0), ==, 3);

	g_assert_true (go_data_unserialize (copy, "9", NULL));
	g_assert_cmpuint (changes, ==, 2);
	go_data_get_matrix_size (copy, &rows, &columns);
	g_assert_cmpuint (rows, ==, 1);
	g_assert_cmpuint (columns, ==, 1);
	g_assert_cmpfloat (go_data_get_matrix_value (copy, 0, 0), ==, 9);

	g_assert_false (go_data_unserialize (copy, "x", NULL));
	go_data_get_matrix_size (copy, &rows, &columns);
	g_assert_cmpuint (rows, ==, 1);
	g_assert_cmpuint (columns, ==, 1);
	g_assert_cmpfloat (go_data_get_matrix_value (copy, 0, 0), ==, 9);
	g_assert_cmpuint (changes, ==, 2);

	g_object_unref (copy);
	g_object_unref (data);
}

static void
test_matrix_value_without_size_query (void)
{
	double values[] = { 1, 2, 3, 4 };
	GOData *data = go_data_matrix_val_new (values, 2, 2, NULL);

	g_assert_cmpfloat (go_data_get_matrix_value (data, 0, 0), ==, 1);

	g_object_unref (data);
}

static void
test_matrix_val_dup (void)
{
	double values[] = { 1, 2, 3, 4 };
	GOData *data = go_data_matrix_val_new (values, 2, 2, NULL);
	GOData *copy = go_data_dup (data);

	g_assert_nonnull (copy);
	go_data_get_matrix_size (data, NULL, NULL);
	go_data_get_matrix_size (copy, NULL, NULL);
	g_assert_cmpfloat (go_data_get_matrix_value (copy, 0, 0), ==, 1);

	/* go_data_dup() promises a deep copy.  */
	values[0] = 99;
	g_assert_cmpfloat (go_data_get_matrix_value (data, 0, 0), ==, 99);
	g_assert_cmpfloat (go_data_get_matrix_value (copy, 0, 0), ==, 1);

	g_object_unref (copy);
	g_object_unref (data);
}

static void
test_matrix_val_nan_bounds (void)
{
	double values[] = { 1, go_nan, 3, 4 };
	GOData *data = go_data_matrix_val_new (values, 2, 2, NULL);
	double min, max;

	go_data_get_bounds (data, &min, &max);
	g_assert_cmpfloat (min, ==, 1);
	g_assert_cmpfloat (max, ==, 4);

	g_object_unref (data);
}

static void
test_matrix_val_ragged (void)
{
	double values[] = { 1, 2, 3, 4 };
	GOData *data = go_data_matrix_val_new (values, 2, 2, NULL);
	char *ragged = g_strdup_printf ("1%c2%c3",
					go_locale_get_col_sep (),
					go_locale_get_row_sep ());

	g_assert_false (go_data_unserialize (data, ragged, NULL));
	g_free (ragged);

	/* A failed parse must not destroy the previous contents.  */
	go_data_get_matrix_size (data, NULL, NULL);
	g_assert_cmpfloat (go_data_get_matrix_value (data, 0, 0), ==, 1);
	g_assert_cmpfloat (go_data_get_matrix_value (data, 1, 1), ==, 4);

	g_object_unref (data);
}

static void
test_container_markup (void)
{
	double values[] = { 1, 2, 4 };
	GOData *vector = go_data_vector_val_new_copy (values, G_N_ELEMENTS (values));
	GOData *matrix = go_data_matrix_val_new (values, 1, 3, NULL);

	g_assert_null (go_data_get_vector_markup (vector, 0));
	g_assert_null (go_data_get_matrix_markup (matrix, 0, 0));

	g_object_unref (matrix);
	g_object_unref (vector);
}

static void
test_has_value (void)
{
	double finite_values[] = { 1, 2 };
	double nan_values[] = { go_nan, go_nan };
	double matrix_values[] = { 1, 2, 3, 4 };
	double nan_matrix_values[] = { go_nan, go_nan, go_nan, go_nan };
	GOData *finite = go_data_scalar_val_new (1);
	GOData *nan = go_data_scalar_val_new (go_nan);
	GOData *string = go_data_scalar_str_new_copy ("text");
	GOData *vector = go_data_vector_val_new_copy (finite_values,
						      G_N_ELEMENTS (finite_values));
	GOData *nan_vector = go_data_vector_val_new_copy (nan_values,
							 G_N_ELEMENTS (nan_values));
	GOData *matrix = go_data_matrix_val_new (matrix_values, 2, 2, NULL);
	GOData *nan_matrix = go_data_matrix_val_new (nan_matrix_values, 2, 2, NULL);

	g_assert_true (go_data_has_value (finite));
	g_assert_false (go_data_has_value (nan));
	g_assert_false (go_data_has_value (string));
	g_assert_true (go_data_has_value (vector));
	g_assert_false (go_data_has_value (nan_vector));
	g_assert_true (go_data_has_value (matrix));
	g_assert_false (go_data_has_value (nan_matrix));

	g_object_unref (nan_matrix);
	g_object_unref (matrix);
	g_object_unref (nan_vector);
	g_object_unref (vector);
	g_object_unref (string);
	g_object_unref (nan);
	g_object_unref (finite);
}

static void
test_has_value_after_unserialize (void)
{
	GOData *data = go_data_scalar_val_new (go_nan);

	g_assert_false (go_data_has_value (data));
	g_assert_true (go_data_unserialize (data, "3.5", NULL));
	g_assert_true (go_data_has_value (data));

	g_object_unref (data);
}

static void
test_vector_str_translate_notify (void)
{
	char const *strings[] = { "one" };
	GOData *data = go_data_vector_str_new_copy (strings, G_N_ELEMENTS (strings));
	TranslateState first = { 0, 0 };
	TranslateState second = { 0, 0 };
	char *str;

	go_data_vector_str_set_translate_func (GO_DATA_VECTOR_STR (data),
					       translate_string, &first,
					       count_notify);
	str = go_data_get_vector_string (data, 0);
	g_assert_cmpstr (str, ==, "one");
	g_free (str);
	g_assert_cmpuint (first.calls, ==, 1);
	g_assert_cmpuint (first.destroys, ==, 0);

	go_data_vector_str_set_translate_func (GO_DATA_VECTOR_STR (data),
					       translate_string, &second,
					       count_notify);
	g_assert_cmpuint (first.destroys, ==, 1);

	str = go_data_get_vector_string (data, 0);
	g_assert_cmpstr (str, ==, "one");
	g_free (str);
	g_assert_cmpuint (first.calls, ==, 1);
	g_assert_cmpuint (second.calls, ==, 1);

	g_object_unref (data);
	g_assert_cmpuint (second.destroys, ==, 1);
}

static void
test_legacy_unserialize (void)
{
	double unused_values[] = { 0 };
	char const *unused_strings[] = { "x" };
	char sep = go_locale_get_col_sep ();
	GOData *vector = go_data_vector_val_new_copy (unused_values, 1);
	GOData *strings = go_data_vector_str_new_copy (unused_strings, 1);
	GOData *matrix = go_data_matrix_val_new (unused_values, 1, 1, NULL);
	char *text, *value;
	unsigned rows, columns;

	/* Unquoted values and the locale's column separator were accepted before.  */
	text = g_strdup_printf ("1%c2%c3", sep, sep);
	g_assert_true (go_data_unserialize (vector, text, NULL));
	g_free (text);
	g_assert_cmpuint (go_data_get_vector_size (vector), ==, 3);
	g_assert_cmpfloat (go_data_get_vector_value (vector, 2), ==, 3);

	text = g_strdup_printf ("one%ctwo", sep);
	g_assert_true (go_data_unserialize (strings, text, NULL));
	g_free (text);
	g_assert_cmpuint (go_data_get_vector_size (strings), ==, 2);
	value = go_data_get_vector_string (strings, 1);
	g_assert_cmpstr (value, ==, "two");
	g_free (value);

	/* A single row has no row separator to establish its width.  */
	text = g_strdup_printf ("1%c2%c3", sep, sep);
	g_assert_true (go_data_unserialize (matrix, text, NULL));
	g_free (text);
	go_data_get_matrix_size (matrix, &rows, &columns);
	g_assert_cmpuint (rows, ==, 1);
	g_assert_cmpuint (columns, ==, 3);
	g_assert_cmpfloat (go_data_get_matrix_value (matrix, 0, 2), ==, 3);

	g_object_unref (matrix);
	g_object_unref (strings);
	g_object_unref (vector);
}

int
main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);
	libgoffice_init ();

	g_test_add_func ("/data/scalar/val", test_scalar_val);
	g_test_add_func ("/data/scalar/str", test_scalar_str);
	g_test_add_func ("/data/scalar/val-io", test_scalar_val_io);
	g_test_add_func ("/data/scalar/str-io", test_scalar_str_io);
	g_test_add_func ("/data/scalar/markup", test_scalar_markup);
	g_test_add_func ("/data/vector/val", test_vector_val);
	g_test_add_func ("/data/vector/str", test_vector_str);
	g_test_add_func ("/data/vector/val-io", test_vector_val_io);
	g_test_add_func ("/data/vector/val-dup", test_vector_val_dup);
	g_test_add_func ("/data/vector/val-nan-bounds", test_vector_val_nan_bounds);
	g_test_add_func ("/data/vector/str-io", test_vector_str_io);
	g_test_add_func ("/data/vector/str-quotes", test_vector_str_quotes);
	g_test_add_func ("/data/matrix/val", test_matrix_val);
	g_test_add_func ("/data/matrix/val-io", test_matrix_val_io);
	g_test_add_func ("/data/matrix/value-without-size-query",
			 test_matrix_value_without_size_query);
	g_test_add_func ("/data/matrix/val-dup", test_matrix_val_dup);
	g_test_add_func ("/data/matrix/val-nan-bounds", test_matrix_val_nan_bounds);
	g_test_add_func ("/data/matrix/val-ragged", test_matrix_val_ragged);
	g_test_add_func ("/data/markup", test_container_markup);
	g_test_add_func ("/data/has-value", test_has_value);
	g_test_add_func ("/data/has-value/after-unserialize",
			 test_has_value_after_unserialize);
	g_test_add_func ("/data/vector/str-translate-notify",
			 test_vector_str_translate_notify);
	g_test_add_func ("/data/legacy-unserialize", test_legacy_unserialize);

	int res = g_test_run ();
	libgoffice_shutdown ();
	return res;
}
