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
#include "gog-exp-reg.h"
#include <goffice/math/go-math.h>
#include <goffice/math/go-regression.h>
#include <glib/gi18n-lib.h>

#include <gsf/gsf-impl-utils.h>

static double
gog_exp_reg_curve_get_value_at (GogRegCurve *curve, double x)
{
	return exp (curve->a[0] + curve->a[1] * x);
}

static gchar const*
gog_exp_reg_curve_get_equation (GogRegCurve *curve)
{
	if (!curve->equation) {
		GogLinRegCurve *lin = GOG_LIN_REG_CURVE (curve);
		if (lin->affine)
			curve->equation = (curve->a[0] < 0.)?
				((curve->a[1] < 0.)?
					g_strdup_printf ("ln(y) = \xE2\x88\x92%gx \xE2\x88\x92 %g", -curve->a[1], -curve->a[0]):
					g_strdup_printf ("ln(y) = %gx \xE2\x88\x92% g", curve->a[1], -curve->a[0])):
				((curve->a[1] < 0.)?
					g_strdup_printf ("ln(y) = \xE2\x88\x92%gx + %g", -curve->a[1], curve->a[0]):
					g_strdup_printf ("ln(y) = %gx + %g", curve->a[1], curve->a[0]));
		else
			curve->equation = (curve->a[1] < 0.)?
				g_strdup_printf ("ln(y) = \xE2\x88\x92%gx", -curve->a[1]):
				g_strdup_printf ("ln(y) = %gx", curve->a[1]);
	}
	return curve->equation;
}

static char const *
gog_exp_reg_curve_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name scatter plot objects
	 * eg The 2nd plot in a chart will be called
	 * 	Exponential regression2 */
	return N_("Exponential regression");
}

static void
gog_exp_reg_curve_class_init (GogRegCurveClass *reg_curve_klass)
{
	GogLinRegCurveClass *lin_reg_klass = (GogLinRegCurveClass *) reg_curve_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) reg_curve_klass;

	lin_reg_klass->lin_reg_func = go_exponential_regression_as_log;

	reg_curve_klass->get_value_at = gog_exp_reg_curve_get_value_at;
	reg_curve_klass->get_equation = gog_exp_reg_curve_get_equation;

	gog_object_klass->type_name	= gog_exp_reg_curve_type_name;
}

GSF_DYNAMIC_CLASS (GogExpRegCurve, gog_exp_reg_curve,
	gog_exp_reg_curve_class_init, NULL,
	GOG_TYPE_LIN_REG_CURVE)
