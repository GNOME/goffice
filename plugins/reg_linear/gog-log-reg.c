/*
 * gog-log-reg.c :
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

#include <goffice/goffice-config.h>
#include "gog-log-reg.h"
#include <goffice/math/go-math.h>
#include <goffice/math/go-regression.h>
#include <glib/gi18n-lib.h>

#include <gsf/gsf-impl-utils.h>

static int
gog_log_reg_curve_build_values (GogLinRegCurve *rc, double const *x_vals, double const *y_vals, int n)
{
	int i, used;
	double x, y;
	double xmin, xmax;
	gog_reg_curve_get_bounds (&rc->base, &xmin, &xmax);
	if (rc->x_vals == NULL)
		rc->x_vals = g_new0 (double*, 1);
	g_free (*rc->x_vals);
	*rc->x_vals = g_new (double, n);
	g_free (rc->y_vals);
	rc->y_vals = g_new (double, n);
	for (i = 0, used = 0; i < n; i++) {
		x = (x_vals)? x_vals[i]: i + 1;
		y = y_vals[i];
		if (x <= 0. || !go_finite (x) || !go_finite (y)) {
			if (rc->base.skip_invalid)
				continue;
			used = 0;
			break;
		}
		if (x < xmin || x > xmax)
			continue;
		rc->x_vals[0][used] = x;
		rc->y_vals[used] = y;
		used++;
	}
	return used;
}

static double
gog_log_reg_curve_get_value_at (GogRegCurve *curve, double x)
{
	return curve->a[0] + curve->a[1] * log (x);
}

static gchar const*
gog_log_reg_curve_get_equation (GogRegCurve *curve)
{
	if (!curve->equation) {
		GogLinRegCurve *lin = GOG_LIN_REG_CURVE (curve);
		if (lin->affine)
			curve->equation = (curve->a[0] < 0.)?
				((curve->a[1] < 0)?
					g_strdup_printf ("y = \xE2\x88\x92%g ln(x) \xE2\x88\x92 %g", -curve->a[1], -curve->a[0]):
					g_strdup_printf ("y = %g ln(x) \xE2\x88\x92 %g", curve->a[1], -curve->a[0])):
				((curve->a[1] < 0)?
					g_strdup_printf ("y = \xE2\x88\x92%g ln(x) + %g", -curve->a[1], curve->a[0]):
					g_strdup_printf ("y = %g ln(x) + %g", curve->a[1], curve->a[0]));
		else
			curve->equation = (curve->a[1] < 0)?
				g_strdup_printf ("y = \xE2\x88\x92%g ln(x)", -curve->a[1]):
				g_strdup_printf ("y = %g ln(x)", curve->a[1]);
	}
	return curve->equation;
}

static char const *
gog_log_reg_curve_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name scatter plot objects
	 * eg The 2nd plot in a chart will be called
	 * 	Logarithmic regression2 */
	return N_("Logarithmic regression");
}

static void
gog_log_reg_curve_class_init (GogRegCurveClass *reg_curve_klass)
{
	GogLinRegCurveClass *lin_reg_klass = (GogLinRegCurveClass *) reg_curve_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) reg_curve_klass;

	lin_reg_klass->lin_reg_func = go_logarithmic_regression;
	lin_reg_klass->build_values = gog_log_reg_curve_build_values;

	reg_curve_klass->get_value_at = gog_log_reg_curve_get_value_at;
	reg_curve_klass->get_equation = gog_log_reg_curve_get_equation;

	gog_object_klass->type_name	= gog_log_reg_curve_type_name;
}

GSF_DYNAMIC_CLASS (GogLogRegCurve, gog_log_reg_curve,
	gog_log_reg_curve_class_init, NULL,
	GOG_TYPE_LIN_REG_CURVE)
