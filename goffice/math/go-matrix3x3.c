/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
