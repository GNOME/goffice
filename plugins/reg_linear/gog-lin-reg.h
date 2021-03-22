/*
 * gog-lin-reg.h :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOG_LIN_REG_H
#define GOG_LIN_REG_H

#include <goffice/graph/gog-reg-curve.h>
#include <goffice/math/go-regression.h>

G_BEGIN_DECLS

typedef  struct {
	GogRegCurve base;
	gboolean affine;
	double **x_vals, *y_vals;
	int dims;
	gboolean use_days_var;
	double xbasis;
} GogLinRegCurve;

typedef struct {
	GogRegCurveClass base;

	GORegressionResult (*lin_reg_func) (double **xss, int dim,
				    const double *ys, int n,
				    gboolean affine,
				    double *res,
				    go_regression_stat_t *stat);
	int (*build_values) (GogLinRegCurve *rc, double const *x_vals,
					double const *y_vals, int n);
	int max_dims;
} GogLinRegCurveClass;

#define GOG_TYPE_LIN_REG_CURVE	(gog_lin_reg_curve_get_type ())
#define GOG_LIN_REG_CURVE(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_LIN_REG_CURVE, GogLinRegCurve))
#define GOG_IS_LIN_REG_CURVE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_LIN_REG_CURVE))

GType gog_lin_reg_curve_get_type (void);
void  gog_lin_reg_curve_register_type (GTypeModule *module);

G_END_DECLS

#endif	/* GOG_LIN_REG_H */
