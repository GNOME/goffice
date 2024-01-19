/*
 * go-data.c:
 *
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <goffice/goffice-config.h>
#include "go-data.h"
#include "go-data-impl.h"
#include <goffice/math/go-math.h>
#include <goffice/math/go-rangefunc.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>

/**
 * GODataFlags:
 * @GO_DATA_CACHE_IS_VALID: data in cache are valid.
 * @GO_DATA_IS_EDITABLE: data can be edited.
 * @GO_DATA_SIZE_CACHED: cached size is valid.
 * @GO_DATA_HAS_VALUE: object is not empty.
 **/

/**
 * GODataClass:
 * @base: base class.
 * @dup: duplicates the #GOData.
 * @eq: tests if the data are equal.
 * @preferred_fmt: gets the preferred format.
 * @date_conv: gets the #GODateConventions.
 * @serialize: serializes.
 * @unserialize: unserializes.
 * @emit_changed: signals the data have changed.
 * @get_n_dimensions: gets the dimensions number.
 * @get_sizes: gets the sizes.
 * @get_values: gets the values.
 * @get_bounds: gets the bounds.
 * @get_value: gets a value.
 * @get_string: gets a string.
 * @get_markup: gets the #PangoAttrList* for the string.
 * @is_valid: checks if the data are valid.
 **/

/**
 * GODataScalarClass:
 * @base:  base class.
 * @get_value: gets the value.
 * @get_str: gets the string.
 * @get_markup: gets the #PangoAttrList* for the string.
 **/

/**
 * GODataVectorClass:
 * @base: base class.
 * @load_len: loads the vector length.
 * @load_values: loads the values in the cache.
 * @get_value: gets a value.
 * @get_str: gets a string.
 * @get_markup: gets the #PangoAttrList* for the string.
 **/

/**
 * GODataMatrixClass:
 * @base: base class.
 * @load_size: loads the matrix length.
 * @load_values: loads the values in the cache.
 * @get_value: gets a value.
 * @get_str: gets a string.
 * @get_markup: gets the #PangoAttrList* for the string.
 **/

#define GO_DATA_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GO_TYPE_DATA, GODataClass))
#define GO_IS_DATA_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GO_TYPE_DATA))
#define GO_DATA_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GO_TYPE_DATA, GODataClass))

/**
 * GODataMatrixSize:
 * @rows: rows number, negative if dirty, includes missing values.
 * @columns: columns number, negative if dirty, includes missing values.
 **/

enum {
	CHANGED,
	LAST_SIGNAL
};

static gulong go_data_signals [LAST_SIGNAL] = { 0, };

/* trivial fall back */
static GOData *
go_data_dup_real (GOData const *src)
{
	gpointer user = NULL;  /* FIXME? */
	char   *str = go_data_serialize (src, user);
	GOData *dst = g_object_new (G_OBJECT_TYPE (src), NULL);
	if (dst != NULL)
		go_data_unserialize (dst, str, user);
	g_free (str);

	return dst;
}

static void
go_data_init (GOData *data)
{
	data->flags = 0;
}

#if 0
static GObjectClass *parent_klass;
static void
go_data_finalize (GOData *obj)
{
	g_warning ("finalize");
	(parent_klass->finalize) (obj);
}
#endif

static void
go_data_class_init (GODataClass *klass)
{
	go_data_signals [CHANGED] = g_signal_new ("changed",
		G_TYPE_FROM_CLASS (klass),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GODataClass, changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE, 0);
	klass->dup = go_data_dup_real;
#if 0
	{
		GObjectClass *gobj_klass = (GObjectClass *)klass;
		gobj_klass->finalize = go_data_finalize;
		parent_klass = g_type_class_peek_parent (klass);
	}
#endif
}

GSF_CLASS_ABSTRACT (GOData, go_data,
		    go_data_class_init, go_data_init,
		    G_TYPE_OBJECT)

/**
 * go_data_dup:
 * @src: #GOData
 *
 * Returns: (transfer full): A deep copy of @src.
 **/
GOData *
go_data_dup (GOData const *src)
{
	if (src != NULL) {
		GODataClass const *klass = GO_DATA_GET_CLASS (src);
		g_return_val_if_fail (klass != NULL, NULL);
		return (*klass->dup) (src);
	}
	return NULL;
}

/**
 * go_data_eq:
 * @a: (nullable): #GOData
 * @b: (nullable): #GOData
 *
 * Returns: %TRUE if @a and @b are the same
 **/
gboolean
go_data_eq (GOData const *a, GOData const *b)
{
	if (a == b)
		return TRUE;
	else {
		GODataClass *a_klass = GO_DATA_GET_CLASS (a);
		GODataClass *b_klass = GO_DATA_GET_CLASS (b);

		g_return_val_if_fail (a_klass != NULL, FALSE);
		g_return_val_if_fail (a_klass->eq != NULL, FALSE);

		if (a_klass != b_klass)
			return FALSE;

		return (*a_klass->eq) (a, b);
	}
}

/**
 * go_data_preferred_fmt:
 * @dat: #GOData
 *
 * Caller is responsible for unrefing the result.
 *
 * Returns: the fmt preferred by the data
 **/
GOFormat const *
go_data_preferred_fmt (GOData const *dat)
{
	GODataClass const *klass = GO_DATA_GET_CLASS (dat);
	g_return_val_if_fail (klass != NULL, NULL);
	if (klass->preferred_fmt)
		return klass->preferred_fmt (dat);
	return NULL;
}

/**
 * go_data_date_conv:
 * @dat: #GOData
 *
 * Returns: the date conventions used by the data, or %NULL if not determined.
 **/
GODateConventions const *
go_data_date_conv (GOData const *dat)
{
	GODataClass const *klass = GO_DATA_GET_CLASS (dat);
	g_return_val_if_fail (klass != NULL, NULL);
	if (klass->date_conv)
		return klass->date_conv (dat);
	return NULL;
}


/**
 * go_data_serialize:
 * @dat: #GOData
 * @user: a gpointer describing the context.
 *
 * NOTE: This is the _source_ not the content.  (I.e., this refers to the
 * expression, not its current value.)
 *
 * Returns: a string representation of the data source that the caller is
 * 	responsible for freeing
 **/
char *
go_data_serialize (GOData const *dat, gpointer user)
{
	GODataClass const *klass = GO_DATA_GET_CLASS (dat);
	g_return_val_if_fail (klass != NULL, NULL);
	return (*klass->serialize) (dat, user);
}

/**
 * go_data_unserialize:
 * @dat: #GOData
 * @str: string to parse
 * @user: a gpointer describing the context.
 *
 * De-serializes the source information returned from go_data_serialize.
 *
 * Returns: %FALSE on error.
 **/
gboolean
go_data_unserialize (GOData *dat, char const *str, gpointer user)
{
	GODataClass const *klass = GO_DATA_GET_CLASS (dat);
	g_return_val_if_fail (klass != NULL, FALSE);
	/* an empty string is not valid, see #46 */
	g_return_val_if_fail (str && *str, FALSE);
	return (*klass->unserialize) (dat, str, user);
}

gboolean
go_data_is_valid (GOData const *data)
{
	GODataClass const *klass = GO_DATA_GET_CLASS (data);
	g_return_val_if_fail (klass != NULL, FALSE);
	return (klass->is_valid != NULL)? (*klass->is_valid) (data): FALSE;
}

/**
 * go_data_emit_changed:
 * @dat: #GOData
 *
 * protected utility to emit a 'changed' signal
 **/
void
go_data_emit_changed (GOData *dat)
{
	GODataClass const *klass = GO_DATA_GET_CLASS (dat);

	g_return_if_fail (klass != NULL);

	if (klass->emit_changed)
		(*klass->emit_changed) (dat);

	g_signal_emit (G_OBJECT (dat), go_data_signals [CHANGED], 0);
}

typedef enum {
	GO_DATA_VARIATION_CHECK_INCREASING,
	GO_DATA_VARIATION_CHECK_DECREASING,
	GO_DATA_VARIATION_CHECK_UNIFORMLY
} GODataVariationCheck;

static gboolean
go_data_check_variation (GOData *data, GODataVariationCheck check)
{
	double *values;
	unsigned int n_values;

	g_return_val_if_fail (GO_IS_DATA (data), FALSE);

	values = go_data_get_values (data);
	if (values == NULL)
		return FALSE;

	n_values = go_data_get_n_values (data);
	if (n_values < 1)
		return FALSE;

	switch (check) {
		case GO_DATA_VARIATION_CHECK_UNIFORMLY:
			return go_range_vary_uniformly (values, n_values);
		case GO_DATA_VARIATION_CHECK_INCREASING:
			return go_range_increasing (values, n_values);
		default:
			return go_range_decreasing (values, n_values);
	}
}

gboolean
go_data_is_increasing (GOData *data)
{
	return go_data_check_variation (data, GO_DATA_VARIATION_CHECK_INCREASING);
}

gboolean
go_data_is_decreasing (GOData *data)
{
	return go_data_check_variation (data, GO_DATA_VARIATION_CHECK_DECREASING);
}

gboolean
go_data_is_varying_uniformly (GOData *data)
{
	return go_data_check_variation (data, GO_DATA_VARIATION_CHECK_UNIFORMLY);
}

gboolean
go_data_has_value (GOData const *data)
{
	g_return_val_if_fail (GO_IS_DATA (data), FALSE);
	if (!(data->flags & GO_DATA_CACHE_IS_VALID))
		go_data_get_values (GO_DATA (data));
	return data->flags & GO_DATA_HAS_VALUE;
}

unsigned int
go_data_get_n_dimensions (GOData *data)
{
	GODataClass const *data_class;

	g_return_val_if_fail (GO_IS_DATA (data), 0);

	data_class = GO_DATA_GET_CLASS (data);
	g_return_val_if_fail (data_class->get_n_dimensions != NULL, 0);

	return data_class->get_n_dimensions (data);
}

unsigned int
go_data_get_n_values (GOData *data)
{
	GODataClass const *data_class;
	unsigned int n_values;
	unsigned int n_dimensions;
	unsigned int *sizes;
	unsigned int i;

	g_return_val_if_fail (GO_IS_DATA (data), 0);

	data_class = GO_DATA_GET_CLASS (data);
	g_return_val_if_fail (data_class->get_n_dimensions != NULL, 0);

	n_dimensions = data_class->get_n_dimensions (data);
	if (n_dimensions < 1)
		return 1;

	sizes = g_newa (unsigned int, n_dimensions);

	g_return_val_if_fail (data_class->get_sizes != NULL, 0);

	data_class->get_sizes (data, sizes);

	n_values = 1;
	for (i = 0; i < n_dimensions; i++)
		n_values *= sizes[i];

	return n_values;
}

static void
go_data_get_sizes (GOData *data, unsigned int n_dimensions, unsigned int *sizes)
{
	GODataClass const *data_class;
	unsigned int actual_n_dimensions;

	g_return_if_fail (n_dimensions > 0);
	g_return_if_fail (sizes != NULL);

	data_class = GO_DATA_GET_CLASS (data);

	g_return_if_fail (data_class->get_n_dimensions != NULL);

	actual_n_dimensions = data_class->get_n_dimensions (data);
	if (actual_n_dimensions != n_dimensions) {
		unsigned int i;

		g_warning ("[GOData::get_sizes] Number of dimension mismatch (requested %d - actual %d)",
			   n_dimensions, actual_n_dimensions);

		for (i = 0; i < n_dimensions; i++)
			sizes[i] = 0;

		return;
	}

	data_class->get_sizes (data, sizes);
}

unsigned int
go_data_get_vector_size (GOData *data)
{
	unsigned int size;

	g_return_val_if_fail (GO_IS_DATA (data), 0);

	go_data_get_sizes (data, 1, &size);

	return size;
}

void
go_data_get_matrix_size (GOData *data, unsigned int *n_rows, unsigned int *n_columns)
{
	unsigned int sizes[2];

	if (!GO_IS_DATA (data)) {
		if (n_columns != NULL)
			*n_columns = 0;

		if (n_rows != NULL)
			*n_rows = 0;

		g_return_if_fail (GO_IS_DATA (data));
	}

	go_data_get_sizes (data, 2, sizes);

	if (n_columns != NULL)
		*n_columns = sizes[0];

	if (n_rows != NULL)
		*n_rows = sizes[1];
}

double *
go_data_get_values (GOData *data)
{
	GODataClass const *data_class;

	g_return_val_if_fail (GO_IS_DATA (data), NULL);

	data_class = GO_DATA_GET_CLASS (data);

	g_return_val_if_fail (data_class->get_values != NULL, NULL);

	return data_class->get_values (data);
}

void
go_data_get_bounds (GOData *data, double *minimum, double *maximum)
{
	GODataClass const *data_class;
	double dummy;

	g_return_if_fail (GO_IS_DATA (data));
	g_return_if_fail (minimum != NULL && maximum != NULL);

	if (maximum == NULL)
		maximum = &dummy;
	else if (minimum == NULL)
		minimum = &dummy;

	data_class = GO_DATA_GET_CLASS (data);

	g_return_if_fail (data_class->get_bounds != NULL);

	data_class->get_bounds (data, minimum, maximum);
}

static double
go_data_get_value (GOData *data, unsigned int n_coordinates, unsigned int *coordinates)
{
	GODataClass const *data_class;
	unsigned int n_dimensions;

	g_return_val_if_fail (GO_IS_DATA (data), go_nan);

	data_class = GO_DATA_GET_CLASS (data);

	n_dimensions = data_class->get_n_dimensions (data);
	if (n_dimensions != n_coordinates) {
		g_warning ("[GOData::get_value] Wrong number of coordinates (given %d - needed %d)",
			   n_coordinates, n_dimensions);

		return go_nan;
	}

	return data_class->get_value (data, coordinates);
}

double
go_data_get_scalar_value (GOData *data)
{
	return go_data_get_value (data, 0, NULL);
}

double
go_data_get_vector_value (GOData *data, unsigned int column)
{
	return go_data_get_value (data, 1, &column);
}

double
go_data_get_matrix_value (GOData *data, unsigned int row, unsigned int column)
{
	unsigned int coordinates[2];

	coordinates[0] = column;
	coordinates[1] = row;

	return go_data_get_value (data, 2, coordinates);
}

static char *
go_data_get_string (GOData *data, unsigned int n_coordinates, unsigned int *coordinates)
{
	GODataClass const *data_class;
	unsigned int n_dimensions;

	g_return_val_if_fail (GO_IS_DATA (data), NULL);

	data_class = GO_DATA_GET_CLASS (data);

	n_dimensions = data_class->get_n_dimensions (data);
	if (n_dimensions != n_coordinates) {
		g_warning ("[GOData::get_string] Wrong number of coordinates (given %d - needed %d)",
			   n_coordinates, n_dimensions);

		return NULL;
	}

	return data_class->get_string (data, coordinates);
}

char *
go_data_get_scalar_string (GOData *data)
{
	return go_data_get_string (data, 0, NULL);
}

char *
go_data_get_vector_string (GOData *data, unsigned int column)
{
	return go_data_get_string (data, 1, &column);
}

char *
go_data_get_matrix_string (GOData *data, unsigned int row, unsigned int column)
{
	unsigned int coordinates[2];

	coordinates[0] = column;
	coordinates[1] = row;

	return go_data_get_string (data, 2, coordinates);
}

static PangoAttrList *
go_data_get_markup (GOData *data, unsigned int n_coordinates, unsigned int *coordinates)
{
	GODataClass const *data_class;
	unsigned int n_dimensions;

	g_return_val_if_fail (GO_IS_DATA (data), NULL);

	data_class = GO_DATA_GET_CLASS (data);

	n_dimensions = data_class->get_n_dimensions (data);
	if (n_dimensions != n_coordinates) {
		g_warning ("[GOData::get_markup] Wrong number of coordinates (given %d - needed %d)",
			   n_coordinates, n_dimensions);

		return NULL;
	}

	return (data_class->get_markup)? data_class->get_markup (data, coordinates): NULL;
}

PangoAttrList *
go_data_get_scalar_markup (GOData *data)
{
	return go_data_get_markup (data, 0, NULL);
}

PangoAttrList *
go_data_get_vector_markup (GOData *data, unsigned int column)
{
	return go_data_get_markup (data, 1, &column);
}

PangoAttrList *
go_data_get_matrix_markup (GOData *data, unsigned int row, unsigned int column)
{
	unsigned int coordinates[2];

	coordinates[0] = column;
	coordinates[1] = row;

	return go_data_get_markup (data, 2, coordinates);
}

/*************************************************************************/

#define GO_DATA_SCALAR_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST ((k), GO_TYPE_DATA_SCALAR, GODataScalarClass))
#define GO_IS_DATA_SCALAR_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GO_TYPE_DATA_SCALAR))
#define GO_DATA_SCALAR_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GO_TYPE_DATA_SCALAR, GODataScalarClass))

static unsigned int
_data_scalar_get_n_dimensions (GOData *data)
{
	return 0;
}

static double *
_data_scalar_get_values (GOData *data)
{
	GODataScalar *scalar = (GODataScalar *) data;

	go_data_scalar_get_value (scalar);

	return &scalar->value;
}

static void
_data_scalar_get_bounds (GOData *data, double *minimum, double *maximum)
{
	GODataScalar *scalar = (GODataScalar *) data;

	go_data_scalar_get_value (scalar);

	*minimum = scalar->value;
	*maximum = scalar->value;
}

static double
_data_scalar_get_value (GOData *data, unsigned int *coordinates)
{
	return go_data_scalar_get_value ((GODataScalar *) data);
}

static char *
_data_scalar_get_string (GOData *data, unsigned int *coordinates)
{
	return g_strdup (go_data_scalar_get_str ((GODataScalar *) data));
}

static PangoAttrList *
_data_scalar_get_markup (GOData *data, unsigned int *coordinates)
{
	return pango_attr_list_copy ((PangoAttrList *) go_data_scalar_get_markup ((GODataScalar *) data));
}

static void
go_data_scalar_class_init (GODataClass *data_class)
{
	data_class->get_n_dimensions = 	_data_scalar_get_n_dimensions;
	data_class->get_values =	_data_scalar_get_values;
	data_class->get_bounds =	_data_scalar_get_bounds;
	data_class->get_value =		_data_scalar_get_value;
	data_class->get_string =	_data_scalar_get_string;
	data_class->get_markup =	_data_scalar_get_markup;
}

GSF_CLASS_ABSTRACT (GODataScalar, go_data_scalar,
		    go_data_scalar_class_init, NULL,
		    GO_TYPE_DATA)

double
go_data_scalar_get_value (GODataScalar *scalar)
{
	GODataScalarClass const *klass = GO_DATA_SCALAR_GET_CLASS (scalar);
	g_return_val_if_fail (klass != NULL, 0.); /* TODO: make this a nan */
	scalar->value = (*klass->get_value) (scalar);

	return scalar->value;
}

char const *
go_data_scalar_get_str (GODataScalar *scalar)
{
	GODataScalarClass const *klass = GO_DATA_SCALAR_GET_CLASS (scalar);
	g_return_val_if_fail (klass != NULL, "");
	return (*klass->get_str) (scalar);
}

PangoAttrList const *
go_data_scalar_get_markup (GODataScalar *scalar)
{
	GODataScalarClass const *klass = GO_DATA_SCALAR_GET_CLASS (scalar);
	g_return_val_if_fail (klass != NULL, NULL);
	return (klass->get_markup)? (*klass->get_markup) (scalar): NULL;
}

/*************************************************************************/

#define GO_DATA_VECTOR_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST ((k), GO_TYPE_DATA_VECTOR, GODataVectorClass))
#define GO_IS_DATA_VECTOR_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GO_TYPE_DATA_VECTOR))
#define GO_DATA_VECTOR_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GO_TYPE_DATA_VECTOR, GODataVectorClass))

static void
_data_vector_emit_changed (GOData *data)
{
	data->flags &= ~(GO_DATA_CACHE_IS_VALID | GO_DATA_VECTOR_LEN_CACHED | GO_DATA_HAS_VALUE);
}

static unsigned int
_data_vector_get_n_dimensions (GOData *data)
{
	return 1;
}

static void
_data_vector_get_sizes (GOData *data, unsigned int *sizes)
{
	GODataVector *vector = (GODataVector *) data;

	sizes[0] = go_data_vector_get_len (vector);
}

static double *
_data_vector_get_values (GOData *data)
{
	return go_data_vector_get_values ((GODataVector *) data);
}

static void
_data_vector_get_bounds (GOData *data, double *minimum, double *maximum)
{
	go_data_vector_get_minmax ((GODataVector *) data, minimum, maximum);
}

static double
_data_vector_get_value (GOData *data, unsigned int *coordinates)
{
	return go_data_vector_get_value ((GODataVector *) data, coordinates[0]);
}

static char *
_data_vector_get_string (GOData *data, unsigned int *coordinates)
{
	return go_data_vector_get_str ((GODataVector *) data, coordinates[0]);
}

static PangoAttrList *
_data_vector_get_markup (GOData *data, unsigned int *coordinates)
{
	return go_data_vector_get_markup ((GODataVector *) data, coordinates[0]);
}

static void
go_data_vector_class_init (GODataClass *data_class)
{
	data_class->emit_changed = 	_data_vector_emit_changed;
	data_class->get_n_dimensions = 	_data_vector_get_n_dimensions;
	data_class->get_sizes =		_data_vector_get_sizes;
	data_class->get_values =	_data_vector_get_values;
	data_class->get_bounds =	_data_vector_get_bounds;
	data_class->get_value =		_data_vector_get_value;
	data_class->get_string =	_data_vector_get_string;
	data_class->get_markup =	_data_vector_get_markup;
}

GSF_CLASS_ABSTRACT (GODataVector, go_data_vector,
		    go_data_vector_class_init, NULL,
		    GO_TYPE_DATA)

int
go_data_vector_get_len (GODataVector *vec)
{
	if (!vec)
		return 0;
	if (! (vec->base.flags & GO_DATA_VECTOR_LEN_CACHED)) {
		GODataVectorClass const *klass = GO_DATA_VECTOR_GET_CLASS (vec);

		g_return_val_if_fail (klass != NULL, 0);

		(*klass->load_len) (vec);

		g_return_val_if_fail (vec->base.flags & GO_DATA_VECTOR_LEN_CACHED, 0);
	}

	return vec->len;
}

double *
go_data_vector_get_values (GODataVector *vec)
{
	if (! (vec->base.flags & GO_DATA_CACHE_IS_VALID)) {
		GODataVectorClass const *klass = GO_DATA_VECTOR_GET_CLASS (vec);

		g_return_val_if_fail (klass != NULL, NULL);

		(*klass->load_values) (vec);

		{
			double min, max;
			go_data_get_bounds (&vec->base, &min, &max);
			if (go_finite (min) && go_finite (max) && min <= max)
				vec->base.flags |= GO_DATA_HAS_VALUE;

		}

		g_return_val_if_fail (vec->base.flags & GO_DATA_CACHE_IS_VALID, NULL);
	}

	return vec->values;
}

double
go_data_vector_get_value (GODataVector *vec, unsigned i)
{
	if (! (vec->base.flags & GO_DATA_CACHE_IS_VALID)) {
		GODataVectorClass const *klass = GO_DATA_VECTOR_GET_CLASS (vec);
		g_return_val_if_fail (klass != NULL, go_nan);
		return (*klass->get_value) (vec, i);
	}

	g_return_val_if_fail ((int)i < vec->len, go_nan);
	return vec->values [i];
}

char *
go_data_vector_get_str (GODataVector *vec, unsigned i)
{
	GODataVectorClass const *klass = GO_DATA_VECTOR_GET_CLASS (vec);
	char *res;

	g_return_val_if_fail (klass != NULL, g_strdup (""));
	if (! (vec->base.flags & GO_DATA_VECTOR_LEN_CACHED)) {
		(*klass->load_len) (vec);
		g_return_val_if_fail (vec->base.flags & GO_DATA_VECTOR_LEN_CACHED, g_strdup (""));
	}
	g_return_val_if_fail ((int)i < vec->len, g_strdup (""));

	res = (*klass->get_str) (vec, i);
	if (res == NULL)
		return g_strdup ("");
	return res;
}

PangoAttrList *
go_data_vector_get_markup (GODataVector *vec, unsigned i)
{
	GODataVectorClass const *klass = GO_DATA_VECTOR_GET_CLASS (vec);

	g_return_val_if_fail (klass != NULL, NULL);
	if (! (vec->base.flags & GO_DATA_VECTOR_LEN_CACHED)) {
		(*klass->load_len) (vec);
		g_return_val_if_fail (vec->base.flags & GO_DATA_VECTOR_LEN_CACHED, NULL);
	}
	g_return_val_if_fail ((int)i < vec->len, NULL);

	return (klass->get_markup)? (*klass->get_markup) (vec, i): NULL;
}

void
go_data_vector_get_minmax (GODataVector *vec, double *min, double *max)
{
	if (! (vec->base.flags & GO_DATA_CACHE_IS_VALID)) {
		GODataVectorClass const *klass = GO_DATA_VECTOR_GET_CLASS (vec);

		g_return_if_fail (klass != NULL);

		(*klass->load_values) (vec);

		g_return_if_fail (vec->base.flags & GO_DATA_CACHE_IS_VALID);
	}

	if (min != NULL)
		*min = vec->minimum;
	if (max != NULL)
		*max = vec->maximum;
}

gboolean
go_data_vector_increasing (GODataVector *vec)
{
	double *data = go_data_vector_get_values (vec);
	int length = go_data_vector_get_len (vec);
	return go_range_increasing (data, length);
}

gboolean
go_data_vector_decreasing (GODataVector *vec)
{
	double *data = go_data_vector_get_values (vec);
	int length = go_data_vector_get_len (vec);
	return go_range_decreasing (data, length);
}

gboolean
go_data_vector_vary_uniformly (GODataVector *vec)
{
	double *data = go_data_vector_get_values (vec);
	int length = go_data_vector_get_len (vec);
	return go_range_vary_uniformly (data, length);
}

/*************************************************************************/

#define GO_DATA_MATRIX_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST ((k), GO_TYPE_DATA_MATRIX, GODataMatrixClass))
#define GO_IS_DATA_MATRIX_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GO_TYPE_DATA_MATRIX))
#define GO_DATA_MATRIX_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GO_TYPE_DATA_MATRIX, GODataMatrixClass))

static void
_data_matrix_emit_changed (GOData *data)
{
	data->flags &= ~(GO_DATA_CACHE_IS_VALID | GO_DATA_MATRIX_SIZE_CACHED | GO_DATA_HAS_VALUE);
}

static unsigned int
_data_matrix_get_n_dimensions (GOData *data)
{
	return 2;
}

static void
_data_matrix_get_sizes (GOData *data, unsigned int *sizes)
{
	GODataMatrix *matrix = (GODataMatrix *) data;
	GODataMatrixSize size;

	size = go_data_matrix_get_size (matrix);

	sizes[0] = size.columns;
	sizes[1] = size.rows;
}

static double *
_data_matrix_get_values (GOData *data)
{
	return go_data_matrix_get_values ((GODataMatrix *) data);
}

static void
_data_matrix_get_bounds (GOData *data, double *minimum, double *maximum)
{
	go_data_matrix_get_minmax ((GODataMatrix *) data, minimum, maximum);
}

static double
_data_matrix_get_value (GOData *data, unsigned int *coordinates)
{
	return go_data_matrix_get_value ((GODataMatrix *) data, coordinates[1], coordinates[0]);
}

static char *
_data_matrix_get_string (GOData *data, unsigned int *coordinates)
{
	return go_data_matrix_get_str ((GODataMatrix *) data, coordinates[1], coordinates[0]);
}

static PangoAttrList *
_data_matrix_get_markup (GOData *data, unsigned int *coordinates)
{
	return go_data_matrix_get_markup ((GODataMatrix *) data, coordinates[1], coordinates[0]);
}

static void
go_data_matrix_class_init (GODataClass *data_class)
{
	data_class->emit_changed = 	_data_matrix_emit_changed;
	data_class->get_n_dimensions = 	_data_matrix_get_n_dimensions;
	data_class->get_sizes =		_data_matrix_get_sizes;
	data_class->get_values =	_data_matrix_get_values;
	data_class->get_bounds =	_data_matrix_get_bounds;
	data_class->get_value =		_data_matrix_get_value;
	data_class->get_string =	_data_matrix_get_string;
	data_class->get_markup =	_data_matrix_get_markup;
}

GSF_CLASS_ABSTRACT (GODataMatrix, go_data_matrix,
		    go_data_matrix_class_init, NULL,
		    GO_TYPE_DATA)

/**
 * go_data_matrix_get_size: (skip)
 * @mat: #GODataMatrix
 *
 * Returns: the matrix size
 **/
GODataMatrixSize
go_data_matrix_get_size (GODataMatrix *mat)
{
	static GODataMatrixSize null_size = {0, 0};
	if (!mat)
		return null_size;
	if (! (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED)) {
		GODataMatrixClass const *klass = GO_DATA_MATRIX_GET_CLASS (mat);

		g_return_val_if_fail (klass != NULL, null_size);

		(*klass->load_size) (mat);

		g_return_val_if_fail (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED, null_size);
	}

	return mat->size;
}

int
go_data_matrix_get_rows (GODataMatrix *mat)
{
	if (!mat)
		return 0;
	if (! (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED)) {
		GODataMatrixClass const *klass = GO_DATA_MATRIX_GET_CLASS (mat);

		g_return_val_if_fail (klass != NULL, 0);

		(*klass->load_size) (mat);

		g_return_val_if_fail (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED, 0);
	}

	return mat->size.rows;
}

int
go_data_matrix_get_columns (GODataMatrix *mat)
{
	if (!mat)
		return 0;
	if (! (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED)) {
		GODataMatrixClass const *klass = GO_DATA_MATRIX_GET_CLASS (mat);

		g_return_val_if_fail (klass != NULL, 0);

		(*klass->load_size) (mat);

		g_return_val_if_fail (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED, 0);
	}

	return mat->size.columns;
}

double *
go_data_matrix_get_values (GODataMatrix *mat)
{
	if (! (mat->base.flags & GO_DATA_CACHE_IS_VALID)) {
		GODataMatrixClass const *klass = GO_DATA_MATRIX_GET_CLASS (mat);

		g_return_val_if_fail (klass != NULL, NULL);

		(*klass->load_values) (mat);

		{
			double min, max;
			go_data_get_bounds (&mat->base, &min, &max);
			if (go_finite (min) && go_finite (max) && min <= max)
				mat->base.flags |= GO_DATA_HAS_VALUE;
		}

		g_return_val_if_fail (mat->base.flags & GO_DATA_CACHE_IS_VALID, NULL);
	}

	return mat->values;
}

double
go_data_matrix_get_value (GODataMatrix *mat, unsigned i, unsigned j)
{
	g_return_val_if_fail (((int)i < mat->size.rows) && ((int)j < mat->size.columns), go_nan);
	if (! (mat->base.flags & GO_DATA_CACHE_IS_VALID)) {
		GODataMatrixClass const *klass = GO_DATA_MATRIX_GET_CLASS (mat);
		g_return_val_if_fail (klass != NULL, go_nan);
		return (*klass->get_value) (mat, i, j);
	}

	return mat->values[i * mat->size.columns + j];
}

char *
go_data_matrix_get_str (GODataMatrix *mat, unsigned i, unsigned j)
{
	GODataMatrixClass const *klass = GO_DATA_MATRIX_GET_CLASS (mat);
	char *res;

	g_return_val_if_fail (klass != NULL, g_strdup (""));
	if (! (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED)) {
		(*klass->load_size) (mat);
		g_return_val_if_fail (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED, g_strdup (""));
	}
	g_return_val_if_fail (((int)i < mat->size.rows) && ((int)j < mat->size.columns), g_strdup (""));

	res = (*klass->get_str) (mat, i, j);
	if (res == NULL)
		return g_strdup ("");
	return res;
}

PangoAttrList *
go_data_matrix_get_markup (GODataMatrix *mat, unsigned i, unsigned j)
{
	GODataMatrixClass const *klass = GO_DATA_MATRIX_GET_CLASS (mat);

	g_return_val_if_fail (klass != NULL, NULL);
	if (! (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED)) {
		(*klass->load_size) (mat);
		g_return_val_if_fail (mat->base.flags & GO_DATA_MATRIX_SIZE_CACHED, NULL);
	}
	g_return_val_if_fail (((int)i < mat->size.rows) && ((int)j < mat->size.columns), NULL);

	return (klass->get_markup)? (*klass->get_markup) (mat, i, j): NULL;
}

void
go_data_matrix_get_minmax (GODataMatrix *mat, double *min, double *max)
{
	if (! (mat->base.flags & GO_DATA_CACHE_IS_VALID)) {
		GODataMatrixClass const *klass = GO_DATA_MATRIX_GET_CLASS (mat);

		g_return_if_fail (klass != NULL);

		(*klass->load_values) (mat);

		g_return_if_fail (mat->base.flags & GO_DATA_CACHE_IS_VALID);
	}

	if (min != NULL)
		*min = mat->minimum;
	if (max != NULL)
		*max = mat->maximum;
}
