/*
 * gog-logfit.c :
 *
 * Copyright (C) 2005-2006 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/app/module-plugin-defs.h>
#include "gog-logfit.h"
#include <goffice/data/go-data.h>
#include <goffice/graph/gog-series-impl.h>
#include <goffice/math/go-math.h>
#include <goffice/math/go-rangefunc.h>
#include <glib/gi18n-lib.h>

#include <gsf/gsf-impl-utils.h>

GOFFICE_PLUGIN_MODULE_HEADER;

static void
gog_log_fit_curve_update (GogObject *obj)
{
	GogRegCurve *rc = GOG_REG_CURVE (obj);
	GogSeries *series = GOG_SERIES (obj->parent);
	double const *y_vals, *x_vals = NULL;
	double *tx_vals, *ty_vals, x, y;
	int i, used = 0, nb;
	double xmin, xmax;

	if (!gog_series_is_valid (series))
		return;

	nb = gog_series_get_xy_data (series, &x_vals, &y_vals);
	if (nb > 0) {
		gog_reg_curve_get_bounds (rc, &xmin, &xmax);
		tx_vals = g_new (double, nb);
		ty_vals = g_new (double, nb);
		for (i = 0, used = 0; i < nb; i++) {
			x = (x_vals)? x_vals[i]: i;
			y = y_vals ? y_vals[i] : go_nan;
			if (!go_finite (x) || !go_finite (y)) {
				if (rc->skip_invalid)
					continue;
				used = 0;
				break;
			}
			if (x < xmin || x > xmax)
				continue;
			tx_vals[used] = x;
			ty_vals[used] = y;
			used++;
		}
		if (used > 4) {
			GORegressionResult res = go_logarithmic_fit (tx_vals, ty_vals,
									used, rc->a);
			if (res == GO_REG_ok) {
				go_range_devsq (ty_vals, used, &x);
				rc->R2 = (x - rc->a[4]) / x;
			} else for (nb = 0; nb < 5; nb++)
				rc->a[nb] = go_nan;
		} else {
			rc->R2 = go_nan;
			for (nb = 0; nb < 5; nb++)
				rc->a[nb] = go_nan;
		}
		g_free (tx_vals);
		g_free (ty_vals);
	} else {
		rc->R2 = go_nan;
		for (i = 0; i < 5; i++)
			rc->a[i] = go_nan;
	}
	g_free (rc->equation);
	rc->equation = NULL;
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static double
gog_log_fit_curve_get_value_at (GogRegCurve *curve, double x)
{
	return (curve->a[0] > 0.)?
		curve->a[1] + curve->a[2] * log (x - curve->a[3]):
		curve->a[1] + curve->a[2] * log (curve->a[3] - x);
}

static gchar const*
gog_log_fit_curve_get_equation (GogRegCurve *curve)
{
	if (!curve->equation) {
		if (curve->a[0] > 0.)
			curve->equation = (curve->a[3] < 0.)?
				((curve-> a[1] < 0)?
					((curve->a[2] < 0)?
						g_strdup_printf("y = \xE2\x88\x92%g \xE2\x88\x92 %g ln(x + %g)", -curve->a[1], -curve->a[2], -curve->a[3]):
						g_strdup_printf("y = \xE2\x88\x92%g + %g ln(x + %g)", -curve->a[1], curve->a[2], -curve->a[3])):
					((curve->a[2] < 0)?
						g_strdup_printf("y = %g \xE2\x88\x92 %g ln(x + %g)", curve->a[1], -curve->a[2], -curve->a[3]):
						g_strdup_printf("y = %g + %g ln(x + %g)", curve->a[1], curve->a[2], -curve->a[3]))):
				((curve-> a[1] < 0)?
					((curve->a[2] < 0)?
						g_strdup_printf("y = \xE2\x88\x92%g \xE2\x88\x92 %g ln(x \xE2\x88\x92 %g)", -curve->a[1], -curve->a[2], curve->a[3]):
						g_strdup_printf("y = \xE2\x88\x92%g + %g ln(x \xE2\x88\x92 %g)", -curve->a[1], curve->a[2], curve->a[3])):
					((curve->a[2] < 0)?
						g_strdup_printf("y = %g \xE2\x88\x92 %g ln(x \xE2\x88\x92 %g)", curve->a[1], -curve->a[2], curve->a[3]):
						g_strdup_printf("y = %g + %g ln(x \xE2\x88\x92% g)", curve->a[1], curve->a[2], curve->a[3])));
		else
			curve->equation = (curve->a[3] < 0.)?
				((curve-> a[1] < 0)?
					((curve->a[2] < 0)?
						g_strdup_printf("y = \xE2\x88\x92%g \xE2\x88\x92 %g ln(\xE2\x88\x92%g \xE2\x88\x92 x)", -curve->a[1], -curve->a[2], -curve->a[3]):
						g_strdup_printf("y = \xE2\x88\x92%g + %g ln(\xE2\x88\x92%g \xE2\x88\x92 x)", -curve->a[1], curve->a[2], -curve->a[3])):
					((curve->a[2] < 0)?
						g_strdup_printf("y = %g \xE2\x88\x92 %g ln(\xE2\x88\x92%g \xE2\x88\x92 x)", curve->a[1], -curve->a[2], -curve->a[3]):
						g_strdup_printf("y = %g + %g ln(\xE2\x88\x92%g \xE2\x88\x92 x)", curve->a[1], curve->a[2], -curve->a[3]))):
				((curve-> a[1] < 0)?
					((curve->a[2] < 0)?
						g_strdup_printf("y = \xE2\x88\x92%g \xE2\x88\x92 %g ln(%g \xE2\x88\x92 x)", -curve->a[1], -curve->a[2], curve->a[3]):
						g_strdup_printf("y = \xE2\x88\x92%g + %g ln(%g \xE2\x88\x92 x)", -curve->a[1], curve->a[2], curve->a[3])):
					((curve->a[2] < 0)?
						g_strdup_printf("y = %g \xE2\x88\x92 %g ln(%g\xE2\x88\x92 x)", curve->a[1], -curve->a[2], curve->a[3]):
						g_strdup_printf("y = %g + %g ln(%g \xE2\x88\x92 x)", curve->a[1], curve->a[2], curve->a[3])));
	}
	return curve->equation;
}

static char const *
gog_log_fit_curve_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name logarithmic fits
	 * eg The 2nd fit for a series will be called
	 * 	Log fit2 */
	return N_("Log fit");
}


static void
gog_log_fit_curve_class_init (GogRegCurveClass *reg_curve_klass)
{
	GogObjectClass *gog_object_klass = (GogObjectClass *) reg_curve_klass;

	gog_object_klass->update = gog_log_fit_curve_update;
	gog_object_klass->type_name	= gog_log_fit_curve_type_name;

	reg_curve_klass->get_value_at = gog_log_fit_curve_get_value_at;
	reg_curve_klass->get_equation = gog_log_fit_curve_get_equation;
}

static void
gog_log_fit_curve_init (GogRegCurve *model)
{
	int i;
	model->a = g_new (double, 5);
	model->R2 = go_nan;
	for (i = 0; i < 5; i++)
		model->a[i] = go_nan;
}

GSF_DYNAMIC_CLASS (GogLogFitCurve, gog_log_fit_curve,
	gog_log_fit_curve_class_init, gog_log_fit_curve_init,
	GOG_TYPE_REG_CURVE)

/* Plugin initialization */

G_MODULE_EXPORT void
go_plugin_init (GOPlugin *plugin, GOCmdContext *cc)
{
	GTypeModule *module = go_plugin_get_type_module (plugin);
	gog_log_fit_curve_register_type (module);
}

G_MODULE_EXPORT void
go_plugin_shutdown (GOPlugin *plugin, GOCmdContext *cc)
{
}
