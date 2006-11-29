/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-gaussian.c :  
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
 
#include <goffice/goffice-config.h>
#include "gog-gaussian.h"
#include <goffice/data/go-data.h>
#include <goffice/utils/go-math.h>
#include <goffice/utils/go-rangefunc.h>
#include <glib/gi18n-lib.h>

#include <gsf/gsf-impl-utils.h>

static RegressionResult
Gauss (double * x, double * params, double *f)
{
	double dx;
	dx = *x - params[1];
	*f = params[0] * exp (- exp (params[2]) * dx * dx);
	return REG_ok;
}

static void
gog_gaussian_curve_update (GogObject *obj)
{
	GogRegCurve *rc = GOG_REG_CURVE (obj);
	GogSeries *series = GOG_SERIES (obj->parent);
	double const *y_vals, *x_vals = NULL;
	double *tx_vals, *ty_vals, **xx_vals, x, y, t;
	int i, used, nb;
	double xmin, xmax, *errors, chi = 0., *sdterrors = NULL;
	GODataVector *errors_data;

	g_return_if_fail (gog_series_is_valid (series));

	nb = gog_series_get_xy_data (series, &x_vals, &y_vals);
	gog_reg_curve_get_bounds (rc, &xmin, &xmax);
	tx_vals = g_new (double, nb);
	ty_vals = g_new (double, nb);
	xx_vals = g_new (double*, nb);
	for (i = 0, used = 0; i < nb; i++) {
		x = (x_vals)? x_vals[i]: i;
		y = y_vals[i];
		if (!go_finite (x) || !go_finite (y)) {
			if (rc->skip_invalid)
				continue;
			used = 0;
			break;
		}
		if (x < xmin || x > xmax)
			continue;
		tx_vals[used] = x;
		xx_vals[used]  = tx_vals + used;
		ty_vals[used] = y;
		used++;
	}
	/* get errors */
	errors_data = gog_reg_curve_get_errors (rc);
	if (errors_data) {
		nb = go_data_vector_get_len (errors_data);
		sdterrors = go_data_vector_get_values (errors_data);
		if (nb > used)
			nb = used;
		else if (used > nb)
			used = nb;
	}
	if (used > 3) {
		/* Calculate initial guesses (this does not use errors !)*/
		go_range_max (ty_vals, used, rc->a);
		go_range_sum (ty_vals, used, &x);
		y = 0.;
		for (i = 0; i < used; i++) {
			y += tx_vals[i] * ty_vals[i];
		}
		rc->a[1] = y / x;
		y = 0.;
		for (i = 0; i < used; i++)
			if (ty_vals[i] > 0) {
				t = tx_vals[i] - rc->a[1];
				y += t * t * ty_vals[i];
			}
		rc->a[2] = log (0.5 * x / y);
		errors = g_new0 (double, used);
		if (REG_ok != go_non_linear_regression (Gauss, xx_vals, rc->a,
									ty_vals, sdterrors, used, 3 ,&chi, errors)) {
			for (nb = 0; nb < 3; nb++)
				rc->a[nb] = go_nan;
		}
printf("chi=%g dof=%d\n",chi,used-3);
		g_free (errors);
	} else {
		rc->R2 = go_nan;
		for (nb = 0; nb < 3; nb++)
			rc->a[nb] = go_nan;
	}
	g_free (xx_vals);
	g_free (tx_vals);
	g_free (ty_vals);
	if (rc->equation) {
		g_free (rc->equation);
		rc->equation = NULL;
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static double
gog_gaussian_curve_get_value_at (GogRegCurve *curve, double x)
{
	double dx = x - curve->a[1];
	return curve->a[0] * exp (- exp (curve->a[2]) * dx * dx);
}

static gchar const*
gog_gaussian_curve_get_equation (GogRegCurve *curve)
{
	if (!curve->equation) {
		curve->equation = g_strdup_printf ("y = %g exp (-%g (x - %g)²)",
			curve->a[0], exp (curve->a[2]), curve->a[1]);
	}
	return curve->equation;
}

static char const *
gog_gaussian_curve_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name scatter plot objects
	 * eg The 2nd plot in a chart will be called
	 * 	Gaussian fit2 */
	return N_("Gaussian fit");
}

static void
gog_gaussian_curve_class_init (GogRegCurveClass *reg_curve_klass)
{
	GogObjectClass *gog_object_klass = (GogObjectClass *) reg_curve_klass;

	gog_object_klass->update = gog_gaussian_curve_update;
	gog_object_klass->type_name	= gog_gaussian_curve_type_name;

	reg_curve_klass->get_value_at = gog_gaussian_curve_get_value_at;
	reg_curve_klass->get_equation = gog_gaussian_curve_get_equation;
}

static void
gog_gaussian_curve_init (GogRegCurve *model)
{
	int i;
	model->a = g_new (double, 3);
	model->R2 = go_nan;
	model->valid_r2 = FALSE;
	for (i = 0; i < 3; i++)
		model->a[i] = go_nan;
	model->use_errors = TRUE;
}

GSF_DYNAMIC_CLASS (GogGaussianCurve, gog_gaussian_curve,
	gog_gaussian_curve_class_init, gog_gaussian_curve_init,
	GOG_REG_CURVE_TYPE)
