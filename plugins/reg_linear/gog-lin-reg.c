/* vm: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * gog-lin-reg.c :
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
#include "gog-lin-reg.h"
#include "gog-polynom-reg.h"
#include "gog-log-reg.h"
#include "gog-exp-reg.h"
#include "gog-power-reg.h"
#include <goffice/data/go-data.h>
#include <goffice/graph/gog-series-impl.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-persist.h>
#include <glib/gi18n-lib.h>

#include <gsf/gsf-impl-utils.h>

GOFFICE_PLUGIN_MODULE_HEADER;

#define GOG_LIN_REG_CURVE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOG_TYPE_LIN_REG_CURVE, GogLinRegCurveClass))

static GogObjectClass *gog_lin_reg_curve_parent_klass;

enum {
	REG_LIN_REG_CURVE_PROP_0,
	REG_LIN_REG_CURVE_PROP_AFFINE,
	REG_LIN_REG_CURVE_PROP_DIMS,
};

static void
gog_lin_reg_curve_update (GogObject *obj)
{
	GogLinRegCurve *rc = GOG_LIN_REG_CURVE (obj);
	GogSeries *series = GOG_SERIES (obj->parent);
	double const *y_vals, *x_vals = NULL;
	int used, nb;

	if (!gog_series_is_valid (series))
		return;

	if (rc->affine) {
		GogPlot *plot = gog_series_get_plot (series);
		GogAxis *xaxis = plot ? gog_plot_get_axis (plot, GOG_AXIS_X) : NULL;
		GOFormat *fmt = xaxis ? gog_axis_get_effective_format (xaxis) : NULL;
		gboolean is_date = fmt && go_format_is_date (fmt) > 0;
		double L, H;
		gog_axis_get_bounds (xaxis, &L, &H);
		rc->use_days_var = is_date;
		rc->xbasis = L;
	} else
		rc->use_days_var = FALSE;

	nb = gog_series_get_xy_data (series, &x_vals, &y_vals);
	used = (y_vals)? (GOG_LIN_REG_CURVE_GET_CLASS(rc))->build_values (rc, x_vals, y_vals, nb): 0;
	if (used > 1) {
		go_regression_stat_t *stats = go_regression_stat_new ();
		GORegressionResult res =
			(GOG_LIN_REG_CURVE_GET_CLASS(rc))->lin_reg_func (rc->x_vals, rc->dims,
						rc->y_vals, used, rc->affine, rc->base.a, stats);
		if (res == GO_REG_ok) {
			rc->base.R2 = stats->sqr_r;
		} else for (nb = 0; nb <= rc->dims; nb++)
			rc->base.a[nb] = go_nan;
		go_regression_stat_destroy (stats);
	} else {
		rc->base.R2 = go_nan;
		for (nb = 0; nb <= rc->dims; nb++)
			rc->base.a[nb] = go_nan;
	}
	g_free (rc->base.equation);
	rc->base.equation = NULL;
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static double
gog_lin_reg_curve_get_value_at (GogRegCurve *curve, double x)
{
	return curve->a[0] + curve->a[1] * x;
}

static gchar const*
gog_lin_reg_curve_get_equation (GogRegCurve *curve)
{
	if (!curve->equation) {
		GogLinRegCurve *lin = GOG_LIN_REG_CURVE (curve);
		const char *uminus = "\xe2\x88\x92";
		double a = curve->a[1];
		double b = curve->a[0];
		const char *var = "x";
		const char *times = "";

		if (lin->use_days_var) {
			var = _("#days");
			// thin-space x thin-space
			times = "\xe2\x80\x89\xc3\x97\xe2\x80\x89";
			b += a * lin->xbasis;
		}

		if (lin->affine)
			curve->equation =
				g_strdup_printf ("y = %s%g%s%s %s %g",
						 (a < 0 ? uminus : ""),
						 fabs (a),
						 times, var,
						 (b < 0 ? uminus : "+"),
						 fabs (b));
		else
			curve->equation =
				g_strdup_printf ("y = %s%g%s",
						 (a < 0 ? uminus : ""),
						 fabs (a),
						 var);
	}
	return curve->equation;
}

static int
gog_lin_reg_curve_build_values (GogLinRegCurve *rc, double const *x_vals, double const *y_vals, int n)
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
		if (!go_finite (x) || !go_finite (y)) {
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

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
#include <goffice/gtk/goffice-gtk.h>

static void
affine_toggled_cb (GtkToggleButton *btn, GObject *obj)
{
	g_object_set (obj, "affine", gtk_toggle_button_get_active (btn), NULL);
}

static void
gog_lin_reg_curve_populate_editor (GogRegCurve *reg_curve, gpointer table)
{
	GtkWidget *w;
	GogLinRegCurve *lin = GOG_LIN_REG_CURVE (reg_curve);

	w = gtk_check_button_new_with_label (_("Affine"));
	gtk_widget_set_tooltip_text (w, _("Uncheck to force zero intercept"));
	gtk_widget_show (w);
	gtk_grid_attach_next_to (table, w, NULL, GTK_POS_BOTTOM, 1, 3);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), lin->affine);
	g_signal_connect (G_OBJECT (w), "toggled", G_CALLBACK (affine_toggled_cb), lin);
}
#endif

static char const *
gog_lin_reg_curve_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name scatter plot objects
	 * eg The 2nd plot in a chart will be called
	 * 	Linear regression2 */
	return N_("Linear regression");
}

static void
gog_lin_reg_curve_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogLinRegCurve *rc = GOG_LIN_REG_CURVE (obj);
	switch (param_id) {
	case REG_LIN_REG_CURVE_PROP_AFFINE:
		g_value_set_boolean (value, rc->affine);
		break;
	case REG_LIN_REG_CURVE_PROP_DIMS:
		g_value_set_uint (value, rc->dims);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_lin_reg_curve_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogLinRegCurve *rc = GOG_LIN_REG_CURVE (obj);
	switch (param_id) {
	case REG_LIN_REG_CURVE_PROP_AFFINE:
		rc->affine = g_value_get_boolean (value);
		break;
	case REG_LIN_REG_CURVE_PROP_DIMS: {
		int max_dims = ((GogLinRegCurveClass *) G_OBJECT_GET_CLASS (rc))->max_dims;
		if (rc->x_vals) {
			int i;
			for (i = 0; i < rc->dims; i++){
				g_free (rc->x_vals[i]);
			}
		}
		g_free (rc->x_vals);
		rc->x_vals = NULL;
		rc->dims = g_value_get_uint (value);
		if (rc->dims > max_dims) {
			g_warning ("Invalid value %u for the \"dims\" property\n", rc->dims);
			rc->dims = max_dims;
		}
		g_free (rc->base.a);
		rc->base.a = g_new (double, rc->dims + 1);
		break;
	}

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_request_update (GOG_OBJECT (obj));
}

static void
gog_lin_reg_curve_finalize (GObject *obj)
{
	GogLinRegCurve *rc = GOG_LIN_REG_CURVE (obj);
	int i;
	if (rc->x_vals) {
		for (i = 0; i < rc->dims; i++)
			g_free (rc->x_vals[i]);
	}
	g_free (rc->x_vals);
	g_free (rc->y_vals);
	(G_OBJECT_CLASS (gog_lin_reg_curve_parent_klass))->finalize (obj);
}

static void
gog_lin_reg_curve_class_init (GogRegCurveClass *reg_curve_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) reg_curve_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) reg_curve_klass;
	GogLinRegCurveClass *lin_klass = (GogLinRegCurveClass *) reg_curve_klass;

	gog_lin_reg_curve_parent_klass = g_type_class_peek_parent (reg_curve_klass);

	gobject_klass->finalize = gog_lin_reg_curve_finalize;
	gobject_klass->get_property = gog_lin_reg_curve_get_property;
	gobject_klass->set_property = gog_lin_reg_curve_set_property;

	gog_object_klass->update = gog_lin_reg_curve_update;
	gog_object_klass->type_name	= gog_lin_reg_curve_type_name;

	reg_curve_klass->get_value_at = gog_lin_reg_curve_get_value_at;
	reg_curve_klass->get_equation = gog_lin_reg_curve_get_equation;
#ifdef GOFFICE_WITH_GTK
	reg_curve_klass->populate_editor = gog_lin_reg_curve_populate_editor;
#endif

	lin_klass->lin_reg_func = go_linear_regression;
	lin_klass->build_values = gog_lin_reg_curve_build_values;
	lin_klass->max_dims = 1;

	g_object_class_install_property (gobject_klass, REG_LIN_REG_CURVE_PROP_AFFINE,
		g_param_spec_boolean ("affine",
			_("Affine"),
			_("If true, a non-zero constant is allowed"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, REG_LIN_REG_CURVE_PROP_DIMS,
		g_param_spec_uint ("dims",
			_("Dims"),
			_("Number of x-vectors"),
		       	1, 10, 1,
			GSF_PARAM_STATIC | G_PARAM_READWRITE|GO_PARAM_PERSISTENT));
}

static void
gog_lin_reg_curve_init (GogLinRegCurve *model)
{
	model->base.a = g_new (double, 2);
	model->base.a[0] = model->base.a[1] = model->base.R2 = go_nan;
	model->affine = TRUE;
	model->x_vals = NULL;
	model->y_vals = NULL;
	model->dims = 1;
	model->use_days_var = FALSE;
	model->xbasis = 0;
}

GSF_DYNAMIC_CLASS (GogLinRegCurve, gog_lin_reg_curve,
	gog_lin_reg_curve_class_init, gog_lin_reg_curve_init,
	GOG_TYPE_REG_CURVE)

/* Plugin initialization */

G_MODULE_EXPORT void
go_plugin_init (GOPlugin *plugin, GOCmdContext *cc)
{
	GTypeModule *module = go_plugin_get_type_module (plugin);
	gog_lin_reg_curve_register_type (module);
	gog_polynom_reg_curve_register_type (module);
	gog_log_reg_curve_register_type (module);
	gog_exp_reg_curve_register_type (module);
	gog_power_reg_curve_register_type (module);
}

G_MODULE_EXPORT void
go_plugin_shutdown (GOPlugin *plugin, GOCmdContext *cc)
{
}
