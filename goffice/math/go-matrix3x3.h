/*
 * go-matrix3x3.h
 *
 * Copyright (C) 2007 Jean Brefort (jean.brefort@normalesup.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#ifndef GO_MATRIX3X3_H
#define GO_MATRIX3X3_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	gdouble a11, a12, a13, a21, a22, a23, a31, a32, a33;
} GOMatrix3x3;

void go_matrix3x3_transform (GOMatrix3x3 *mat,
				double xo, double yo, double zo,
				double *x, double *y, double *z);

void go_matrix3x3_from_euler (GOMatrix3x3 *mat,
				double Psi, double Theta, double Phi);

void go_matrix3x3_from_euler_transposed (GOMatrix3x3 *mat,
				double Psi, double Theta, double Phi);

void go_matrix3x3_to_euler (GOMatrix3x3 const *mat,
				double *Psi, double *Theta, double *Phi);

void go_matrix3x3_multiply (GOMatrix3x3 *dest,
				GOMatrix3x3 const *src1, GOMatrix3x3 const *src2);

G_END_DECLS

#endif /* GO_MATRIX3X3_H */
