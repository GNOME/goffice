/*
 * go-data.h :
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

#ifndef GO_DATA_H
#define GO_DATA_H

#include <goffice/data/goffice-data.h>
#include <goffice/goffice.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GO_TYPE_DATA	(go_data_get_type ())
#define GO_DATA(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DATA, GOData))
#define GO_IS_DATA(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DATA))

GType 		go_data_get_type 		(void);
GOData *	go_data_dup			(GOData const *src);
gboolean  	go_data_eq			(GOData const *a, GOData const *b);
GOFormat const *go_data_preferred_fmt 		(GOData const *dat);
GODateConventions const *go_data_date_conv	(GOData const *dat);
gboolean	go_data_is_valid		(GOData const *dat);

char *		go_data_serialize		(GOData const *dat, gpointer user);
gboolean  	go_data_unserialize		(GOData *dat, char const *str, gpointer user);
void	  	go_data_emit_changed  		(GOData *dat);

double *	go_data_get_values		(GOData *data);
void		go_data_get_bounds		(GOData *data, double *minimum, double *maximum);
gboolean	go_data_is_increasing		(GOData *data);
gboolean	go_data_is_decreasing		(GOData *data);
gboolean	go_data_is_varying_uniformly	(GOData *data);
gboolean	go_data_has_value	    (GOData const *data);

unsigned int 	go_data_get_n_dimensions 	(GOData *data);
unsigned int	go_data_get_n_values		(GOData *data);

unsigned int	go_data_get_vector_size 	(GOData *data);
void		go_data_get_matrix_size		(GOData *data, unsigned int *n_rows, unsigned int *n_columns);

double		go_data_get_scalar_value	(GOData *data);
double		go_data_get_vector_value 	(GOData *data, unsigned int column);
double		go_data_get_matrix_value 	(GOData *data, unsigned int row, unsigned int column);

char *		go_data_get_scalar_string	(GOData *data);
char *		go_data_get_vector_string	(GOData *data, unsigned int column);
char *		go_data_get_matrix_string	(GOData *data, unsigned int row, unsigned int column);

PangoAttrList *	go_data_get_scalar_markup	(GOData *data);
PangoAttrList *	go_data_get_vector_markup	(GOData *data, unsigned int column);
PangoAttrList *	go_data_get_matrix_markup	(GOData *data, unsigned int row, unsigned int column);

/*************************************************************************/

#define GO_TYPE_DATA_SCALAR	(go_data_scalar_get_type ())
#define GO_DATA_SCALAR(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DATA_SCALAR, GODataScalar))
#define GO_IS_DATA_SCALAR(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DATA_SCALAR))

GType go_data_scalar_get_type (void);

double      go_data_scalar_get_value  (GODataScalar *val);
char const *go_data_scalar_get_str    (GODataScalar *val);
PangoAttrList const *go_data_scalar_get_markup    (GODataScalar *val);

/*************************************************************************/

#define GO_TYPE_DATA_VECTOR	(go_data_vector_get_type ())
#define GO_DATA_VECTOR(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DATA_VECTOR, GODataVector))
#define GO_IS_DATA_VECTOR(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DATA_VECTOR))

GType go_data_vector_get_type (void);

int	 go_data_vector_get_len    (GODataVector *vec);
double	*go_data_vector_get_values (GODataVector *vec);
double	 go_data_vector_get_value  (GODataVector *vec, unsigned i);
char	*go_data_vector_get_str    (GODataVector *vec, unsigned i);
PangoAttrList *go_data_vector_get_markup (GODataVector *vec, unsigned i);
void	 go_data_vector_get_minmax (GODataVector *vec, double *min, double *max);
gboolean go_data_vector_increasing (GODataVector *vec);
gboolean go_data_vector_decreasing (GODataVector *vec);
gboolean go_data_vector_vary_uniformly (GODataVector *vec);

/*************************************************************************/

#define GO_TYPE_DATA_MATRIX	(go_data_matrix_get_type ())
#define GO_DATA_MATRIX(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_DATA_MATRIX, GODataMatrix))
#define GO_IS_DATA_MATRIX(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_DATA_MATRIX))

GType go_data_matrix_get_type (void);

GODataMatrixSize	 go_data_matrix_get_size    (GODataMatrix *mat);
int 	 go_data_matrix_get_rows   (GODataMatrix *mat);
int 	 go_data_matrix_get_columns (GODataMatrix *mat);
double	*go_data_matrix_get_values (GODataMatrix *mat);
double	 go_data_matrix_get_value  (GODataMatrix *mat, unsigned i, unsigned j);
char	*go_data_matrix_get_str    (GODataMatrix *mat, unsigned i, unsigned j);
PangoAttrList *go_data_matrix_get_markup (GODataMatrix *mat, unsigned i, unsigned j);
void	 go_data_matrix_get_minmax (GODataMatrix *mat, double *min, double *max);

G_END_DECLS

#endif /* GO_DATA_H */
