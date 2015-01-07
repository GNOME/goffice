/*
 * go-data-impl.h :
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#ifndef GO_DATA_IMPL_H
#define GO_DATA_IMPL_H

#include <goffice/data/goffice-data.h>
#include <goffice/data/go-data.h>
#include <goffice/utils/go-format.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
	GO_DATA_CACHE_IS_VALID =	1 << 0,
	GO_DATA_IS_EDITABLE =		1 << 1,
	GO_DATA_SIZE_CACHED =		1 << 2,
	GO_DATA_HAS_VALUE = 1 << 3
} GODataFlags;

struct _GOData {
	GObject		base;
	gint32		flags; /* dunno what to do with these yet */
};

typedef struct {
	GObjectClass base;

	GOData *	(*dup)	    		(GOData const *src);
	gboolean 	(*eq)	    		(GOData const *a, GOData const *b);
	GOFormat *	(*preferred_fmt) 	(GOData const *dat);
	GODateConventions const *(*date_conv) 	(GOData const *dat);

	char *		(*serialize)	    	(GOData const *dat, gpointer user);
	gboolean   	(*unserialize)	    	(GOData *dat, char const *str, gpointer user);
	void	   	(*emit_changed)  	(GOData *dat);

	unsigned int	(*get_n_dimensions)	(GOData *data);
	void		(*get_sizes)		(GOData *data, unsigned int *sizes);
	double *	(*get_values)		(GOData *data);
	void		(*get_bounds)		(GOData *data, double *minimum, double *maximum);
	double		(*get_value)		(GOData *data, unsigned int *coordinates);
	char *		(*get_string)		(GOData *data, unsigned int *coordinates);
	PangoAttrList * (*get_markup)		(GOData *data, unsigned int *coordinates);
	gboolean	(*is_valid)		(GOData const *data);

	/* signals */
	void (*changed)	(GOData *dat);
} GODataClass;

struct _GODataScalar {
	GOData base;

	double value;
};

typedef struct {
	GODataClass base;
	double       (*get_value)  (GODataScalar *scalar);
	char const  *(*get_str)	   (GODataScalar *scalar);
	PangoAttrList const *(*get_markup) (GODataScalar *data);
} GODataScalarClass;

#define GO_DATA_VECTOR_LEN_CACHED GO_DATA_SIZE_CACHED

struct _GODataVector {
	GOData base;

	int len;	/* negative if dirty, includes missing values */
	double *values;	/* NULL = inititialized/unsupported, nan = missing */
	double minimum, maximum;
};
typedef struct {
	GODataClass base;

	void	 (*load_len)    (GODataVector *vec);
	void	 (*load_values) (GODataVector *vec);
	double	 (*get_value)   (GODataVector *vec, unsigned i);
	char	*(*get_str)	(GODataVector *vec, unsigned i);
	PangoAttrList *(*get_markup) (GODataVector *data, unsigned i);
} GODataVectorClass;

#define	GO_DATA_MATRIX_SIZE_CACHED GO_DATA_SIZE_CACHED

struct _GODataMatrix {
	GOData base;

	GODataMatrixSize size;	/* negative if dirty, includes missing values */
	double *values;	/* NULL = uninitialized/unsupported, nan = missing */
	double minimum, maximum;
};

typedef struct {
	GODataClass base;

	void	 (*load_size)    (GODataMatrix *vec);
	void	 (*load_values) (GODataMatrix *vec);
	double	 (*get_value)   (GODataMatrix *mat, unsigned i, unsigned j);
	char	*(*get_str)	(GODataMatrix *mat, unsigned i, unsigned j);
	PangoAttrList *(*get_markup) (GODataMatrix *mat, unsigned i, unsigned j);
} GODataMatrixClass;

G_END_DECLS

#endif /* GO_DATA_IMPL_H */
