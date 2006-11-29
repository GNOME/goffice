/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-gaussian.h :  
 *
 * Copyright (C) 2006 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
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

#ifndef GOG_GAUSSIAN_H
#define GOG_GAUSSIAN_H

#include <goffice/graph/gog-reg-curve.h>
#include <goffice/utils/go-regression.h>

G_BEGIN_DECLS

typedef  struct {
	GogRegCurve base;
	double *x_vals, *y_vals;
} GogGaussianCurve;

typedef GogRegCurveClass GogGaussianCurveClass;

#define GOG_GAUSSIAN_CURVE_TYPE	(gog_gaussian_curve_get_type ())
#define GOG_GAUSSIAN_CURVE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_GAUSSIAN_CURVE_TYPE, GogGaussianCurve))
#define GOG_IS_GAUSSIAN_CURVE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_GAUSSIAN_CURVE_TYPE))

GType gog_gaussian_curve_get_type (void);
void  gog_gaussian_curve_register_type (GTypeModule *module);

G_END_DECLS

#endif	/* GOG_GAUSSIAN_H */
