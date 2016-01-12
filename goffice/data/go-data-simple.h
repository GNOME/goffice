/*
 * go-data-simple.h :
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
#ifndef GO_DATA_SIMPLE_H
#define GO_DATA_SIMPLE_H

#include <goffice/data/goffice-data.h>
#include <goffice/data/go-data.h>

G_BEGIN_DECLS

#define GO_TYPE_DATA_SCALAR_VAL  (go_data_scalar_val_get_type ())
#define GO_DATA_SCALAR_VAL(o)	 (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DATA_SCALAR_VAL, GODataScalarVal))
#define GO_IS_DATA_SCALAR_VAL(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DATA_SCALAR_VAL))

typedef struct _GODataScalarVal GODataScalarVal;
GType	 go_data_scalar_val_get_type (void);
GOData	*go_data_scalar_val_new      (double val);

#define GO_TYPE_DATA_SCALAR_STR  (go_data_scalar_str_get_type ())
#define GO_DATA_SCALAR_STR(o)	 (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DATA_SCALAR_STR, GODataScalarStr))
#define GO_IS_DATA_SCALAR_STR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DATA_SCALAR_STR))

typedef struct _GODataScalarStr GODataScalarStr;
GType	 go_data_scalar_str_get_type (void);
GOData	*go_data_scalar_str_new      (char const *str, gboolean needs_free);
GOData	*go_data_scalar_str_new_copy (char const *str);
void     go_data_scalar_str_set_str  (GODataScalarStr *str,
				      char const *text, gboolean needs_free);

#define GO_TYPE_DATA_VECTOR_VAL  (go_data_vector_val_get_type ())
#define GO_DATA_VECTOR_VAL(o)	 (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DATA_VECTOR_VAL, GODataVectorVal))
#define GO_IS_DATA_VECTOR_VAL(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DATA_VECTOR_VAL))

typedef struct _GODataVectorVal GODataVectorVal;
GType	 go_data_vector_val_get_type (void);
GOData	*go_data_vector_val_new      (double *val, unsigned n, GDestroyNotify   notify);
GOData  *go_data_vector_val_new_copy (double *val, unsigned n);
#define GO_TYPE_DATA_VECTOR_STR  (go_data_vector_str_get_type ())
#define GO_DATA_VECTOR_STR(o)	 (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DATA_VECTOR_STR, GODataVectorStr))
#define GO_IS_DATA_VECTOR_STR(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DATA_VECTOR_STR))

typedef struct _GODataVectorStr GODataVectorStr;
GType	go_data_vector_str_get_type	      (void);
GOData *go_data_vector_str_new		      (char const * const *str, unsigned n, GDestroyNotify notify);
GOData  *go_data_vector_str_new_copy      (char const * const *str, unsigned n);
void    go_data_vector_str_set_translate_func (GODataVectorStr *vector,
					       GOTranslateFunc  func,
					       gpointer         data,
					       GDestroyNotify   notify);
void go_data_vector_str_set_translation_domain (GODataVectorStr *vector,
						char const      *domain);

#define GO_TYPE_DATA_MATRIX_VAL  (go_data_matrix_val_get_type ())
#define GO_DATA_MATRIX_VAL(o)	 (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DATA_MATRIX_VAL, GODataMatrixVal))
#define GO_IS_DATA_MATRIX_VAL(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DATA_MATRIX_VAL))

typedef struct _GODataMatrixVal GODataMatrixVal;
GType	 go_data_matrix_val_get_type (void);
GOData	*go_data_matrix_val_new      (double *val, unsigned rows, unsigned columns, GDestroyNotify   notify);

G_END_DECLS

#endif /* GO_DATA_SIMPLE_H */
