/*
 * go-matrix3x3.c
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

#include <goffice/goffice-config.h>
#include <goffice/math/go-matrix3x3.h>
#include <math.h>

void
go_matrix3x3_transform (GOMatrix3x3 *mat,
				gdouble xo, gdouble yo, gdouble zo,
				gdouble *x, gdouble *y, gdouble *z)
{
	*x = xo * mat->a11 + yo * mat->a12 + zo * mat->a13;
	*y = xo * mat->a21 + yo * mat->a22 + zo * mat->a23;
	*z = xo * mat->a31 + yo * mat->a32 + zo * mat->a33;
}

void
go_matrix3x3_from_euler (GOMatrix3x3 *mat,
				gdouble Psi, gdouble Theta, gdouble Phi)
{
	gdouble sp = sin(Psi);
	gdouble cp = cos(Psi);
	gdouble st = sin(Theta);
	gdouble ct = cos(Theta);
	gdouble sf = sin(Phi);
	gdouble cf = cos(Phi);
	mat->a11 = cf * cp - sf * sp * ct;
	mat->a12 = - cp * sf - sp * cf * ct;
	mat->a13 = st * sp;
	mat->a21 =  sp * cf + cp * sf * ct;
	mat->a22 = cf * cp * ct - sf * sp;
	mat->a23 = - st * cp;
	mat->a31 = st * sf;
	mat->a32 = st * cf;
	mat->a33 = ct;
}

void
go_matrix3x3_from_euler_transposed (GOMatrix3x3 *mat,
				double Psi, double Theta, double Phi)
{
	gdouble sp = sin(Psi);
	gdouble cp = cos(Psi);
	gdouble st = sin(Theta);
	gdouble ct = cos(Theta);
	gdouble sf = sin(Phi);
	gdouble cf = cos(Phi);
	mat->a11 = cf * cp - sf * sp * ct;
	mat->a21 = - cp * sf - sp * cf * ct;
	mat->a31 = st * sp;
	mat->a12 =  sp * cf + cp * sf * ct;
	mat->a22 = cf * cp * ct - sf * sp;
	mat->a32 = - st * cp;
	mat->a13 = st * sf;
	mat->a23 = st * cf;
	mat->a33 = ct;
}

#define MATRIX_THRESHOLD .999999999
void
go_matrix3x3_to_euler (GOMatrix3x3 const *mat,
				double *Psi, double *Theta, double *Phi)
{
	if (fabs(mat->a33) > MATRIX_THRESHOLD) {
		*Theta = (mat->a33 > 0.) ? 0. : 2 * M_PI;
		*Psi = 0.;
		if (fabs(mat->a11) > MATRIX_THRESHOLD)
			*Phi = (mat->a11 > 0.) ? 0. : 2 * M_PI;
		else
			*Phi = (mat->a21 > 0.) ? acos(mat->a11) : - acos(mat->a11);
	} else {
		double st, si, co;
		*Theta = acos(mat->a33);
		st = sin(*Theta);
		si = mat->a13 / st;
		co = - mat->a23 / st;
		if (fabs(co) > MATRIX_THRESHOLD)
			*Psi = (co > 0.) ? 0. : 2 * M_PI;
		else
			*Psi = (si > 0.) ? acos(co) : - acos(co);
		si = mat->a31 / st;
		co = mat->a32 / st;
		if (fabs(co) > MATRIX_THRESHOLD)
			*Phi = (co > 0.) ? 0. : 2 * M_PI;
		else
			*Phi = (si > 0.) ? acos(co) : - acos(co);
	}
}

void
go_matrix3x3_multiply (GOMatrix3x3 *dest,
				GOMatrix3x3 const *src1, GOMatrix3x3 const *src2)
{
	double a11, a12, a13, a21, a22, a23, a31, a32, a33;

	if (!dest || !src1 || !src2)
		return;

	a11 = src1->a11 * src2->a11 + src1->a12 * src2->a21 + src1->a13 * src2->a31;
	a12 = src1->a11 * src2->a12 + src1->a12 * src2->a22 + src1->a13 * src2->a32;
	a13 = src1->a11 * src2->a13 + src1->a12 * src2->a23 + src1->a13 * src2->a33;
	a21 = src1->a21 * src2->a11 + src1->a22 * src2->a21 + src1->a23 * src2->a31;
	a22 = src1->a21 * src2->a12 + src1->a22 * src2->a22 + src1->a23 * src2->a32;
	a23 = src1->a21 * src2->a13 + src1->a22 * src2->a23 + src1->a23 * src2->a33;
	a31 = src1->a31 * src2->a11 + src1->a32 * src2->a21 + src1->a33 * src2->a31;
	a32 = src1->a31 * src2->a12 + src1->a32 * src2->a22 + src1->a33 * src2->a32;
	a33 = src1->a31 * src2->a13 + src1->a32 * src2->a23 + src1->a33 * src2->a33;
	dest->a11 = a11;
	dest->a12 = a12;
	dest->a13 = a13;
	dest->a21 = a21;
	dest->a22 = a22;
	dest->a23 = a23;
	dest->a31 = a31;
	dest->a32 = a32;
	dest->a33 = a33;
}
