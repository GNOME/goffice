/*
 * go-line.h :
 *
 * Copyright (C) 2004-2006 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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

#ifndef GO_LINE_H
#define GO_LINE_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	double		 offset;
	unsigned int	 n_dash;
	double		*dash;
	/* <private> */
	unsigned int ref_count;
} GOLineDashSequence;
GType go_line_dash_sequence_get_type (void);

GOLineDashType		 go_line_dash_from_str		(char const *name);
char const 		*go_line_dash_as_str		(GOLineDashType type);
char const 		*go_line_dash_as_label 		(GOLineDashType type);
double			 go_line_dash_get_length	(GOLineDashType type);
GOLineDashSequence	*go_line_dash_get_sequence 	(GOLineDashType type, double scale);
void 		  	 go_line_dash_sequence_free	(GOLineDashSequence *sequence);

GOLineInterpolation	 go_line_interpolation_from_str		(char const *name);
char const 		*go_line_interpolation_as_str		(GOLineInterpolation type);
char const 		*go_line_interpolation_as_label		(GOLineInterpolation type);
gboolean		 go_line_interpolation_supports_radial  (GOLineInterpolation type);
gboolean		 go_line_interpolation_auto_skip	(GOLineInterpolation type);


typedef enum {
	GO_ARROW_NONE,
	GO_ARROW_KITE,
	GO_ARROW_OVAL
	/* GO_ARROW_STEALTH */
	/* GO_ARROW_DIAMOND */
	/* GO_ARROW_OPEN */
} GOArrowType;

typedef struct {
	GOArrowType typ;
	double a, b, c;
} GOArrow;

GType go_arrow_get_type (void);
#define GO_ARROW_TYPE (go_arrow_get_type())

char const *go_arrow_type_as_str (GOArrowType typ);
GOArrowType go_arrow_type_from_str (const char *name);

GOArrow *go_arrow_dup (GOArrow *src);
void go_arrow_init (GOArrow *res, GOArrowType typ,
		    double a, double b, double c);
void go_arrow_clear (GOArrow *dst);
void go_arrow_init_kite (GOArrow *dst, double a, double b, double c);
void go_arrow_init_oval (GOArrow *dst, double ra, double rb);

gboolean go_arrow_equal (const GOArrow *a, const GOArrow *b);

void go_arrow_draw (const GOArrow *arrow, cairo_t *cr,
		    double *dx, double *dy, double phi);

G_END_DECLS

#endif /* GO_LINE_H */
