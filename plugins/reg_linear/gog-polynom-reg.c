/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-polynom-reg.c :  
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-polynom-reg.h"
#include <goffice/graph/gog-reg-curve.h>
#include <goffice/utils/go-math.h>
#include <goffice/utils/go-regression.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktable.h>
#include <gtk/gtkspinbutton.h>

#include <gsf/gsf-impl-utils.h>

static GogObjectClass *gog_polynom_reg_curve_parent_klass;

static void order_changed_cb (GtkSpinButton *btn, GObject *obj)
{
	g_object_set (obj, "dims", gtk_spin_button_get_value_as_int (btn), NULL);
}

static int
gog_polynom_reg_curve_build_values (GogLinRegCurve *rc, double const *x_vals,
					double const *y_vals, int n)
{
	double x, y, xx;
	double xmin, xmax;
	int i, j, used;

	gog_reg_curve_get_bounds (&rc->base, &xmin, &xmax);
	if (rc->x_vals == NULL)
		rc->x_vals = g_new0 (double*, rc->dims);
	for (i = 0; i < rc->dims; i++) {
		if (rc->x_vals[i] != NULL)
			g_free (rc->x_vals[i]);
		rc->x_vals[i] = g_new (double, n);
	}
	if (rc->y_vals != NULL)
		g_free (rc->y_vals);
	rc->y_vals = g_new (double, n);
	for (i = 0, used = 0; i < n; i++) {
		x = (x_vals)? x_vals[i]: i;
		y = y_vals[i];
		if (!go_finite (x) || !go_finite (y)) {
			if (rc->base.skip_invalid)
				continue;
			used = 0;
			break;
		}
		if (x < xmin || x > xmax)
			continue;
		xx = 1.;
		for (j = 0; j < rc->dims; j++) {
			xx *= x;
			rc->x_vals[j][used] = xx;
		}
		rc->y_vals[used] = y;
		used++;
	}
	return (used > rc->dims)?  used: 0;
}

static double
gog_polynom_reg_curve_get_value_at (GogRegCurve *curve, double x)
{
	GogLinRegCurve *lin = GOG_LIN_REG_CURVE (curve);
	double result = curve->a[0] + curve->a[1] * x, xx = x;
	int i;
	for (i = 1; i < lin->dims;) {
		xx *= x;
		i++;
		result += xx * curve->a[i];
	}
	return result;
}

static char *exponent[10] = {"\xE2\x81\xB0","\xC2\xB9","\xC2\xB2","\xC2\xB3",
	"\xE2\x81\xB4","\xE2\x81\xB5","\xE2\x81\xB6", "\xE2\x81\xB7",
	"\xE2\x81\xB8","\xE2\x81\xB9"};
static gchar const*
gog_polynom_reg_curve_get_equation (GogRegCurve *curve)
{
	if (!curve->equation) {
		GogLinRegCurve *lin = GOG_LIN_REG_CURVE (curve);
		GString *str = g_string_new ("y =");
		int i = lin->dims, j = 1;
		gboolean first = TRUE;
		while (--i > 1) {
			if (curve->a[i] == 0.) /* a very rare event */
				continue;
			j++;
			if (j % 3 == 3)
				g_string_append (str, "\n");
			if (first) {
				if (curve->a[i] > 0.)
					g_string_append_printf(str, " %gx%s", curve->a[i], exponent[i]);
				else
					g_string_append_printf(str, " \xE2\x88\x92%gx%s", -curve->a[i], exponent[i]);
			} else {
				first = FALSE;
				if (curve->a[i] > 0.)
					g_string_append_printf(str, " + %gx%s", curve->a[i], exponent[i]);
				else
					g_string_append_printf(str, " \xE2\x88\x92 %gx%s", -curve->a[i], exponent[i]);
			}
		}
		if (curve->a[1] != 0.) {
			j++;
			if (j % 3 == 3)
				g_string_append (str, "\n");
			if (first) {
				if (curve->a[1] > 0.)
					g_string_append_printf(str, " %gx", curve->a[1]);
				else
					g_string_append_printf(str, " \xE2\x88\x92%gx", -curve->a[1]);
				first = FALSE;
			} else {
				if (curve->a[1] > 0.)
					g_string_append_printf(str, " + %gx", curve->a[1]);
				else
					g_string_append_printf(str, " \xE2\x88\x92 %gx", -curve->a[1]);
			}
		}
		if (lin->affine && curve->a[0] != 0.) {
			j++;
			if (j % 3 == 3)
				g_string_append (str, "\n");
			if (first) {
				if (curve->a[0] > 0.)
					g_string_append_printf(str, " %g", curve->a[0]);
				else
					g_string_append_printf(str, " \xE2\x88\x92%g", -curve->a[0]);
			} else {
				if (curve->a[0] > 0.)
					g_string_append_printf(str, " + %g", curve->a[0]);
				else
					g_string_append_printf(str, " \xE2\x88\x92 %g", -curve->a[0]);
			}
		}
		curve->equation = g_string_free (str, FALSE);
	}
	return curve->equation;
}

static void
gog_polynom_reg_curve_populate_editor (GogRegCurve *reg_curve, GtkTable *table)
{
	int rows, columns;
	GtkWidget *w;
	GogLinRegCurve *lin = GOG_LIN_REG_CURVE (reg_curve);

	((GogRegCurveClass*) gog_polynom_reg_curve_parent_klass)->populate_editor (reg_curve, table);
	g_object_get (G_OBJECT (table), "n-rows", &rows, "n-columns", &columns, NULL);
	gtk_table_resize (table, rows + 1, columns);
	w = gtk_label_new (_("Order:"));
	gtk_label_set_justify (GTK_LABEL (w), GTK_JUSTIFY_LEFT);
	gtk_widget_show (w);
	gtk_table_attach (table, w, 0, 1, rows, rows + 1, 0, 0, 0, 0);
	w = gtk_spin_button_new_with_range (2, 10, 1);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (w), 0);
	gtk_widget_show (w);
	gtk_table_attach (table, w, 1, columns, rows, rows + 1, GTK_FILL | GTK_EXPAND, 0, 0, 0);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), lin->dims);
	g_signal_connect (G_OBJECT (w), "value-changed", G_CALLBACK (order_changed_cb), lin);
}

static char const *
gog_polynom_reg_curve_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name scatter plot objects
	 * eg The 2nd plot in a chart will be called
	 * 	Polynomial regression2 */
	return N_("Polynomial regression");
}

static void
gog_polynom_reg_curve_class_init (GogRegCurveClass *reg_curve_klass)
{
	GogLinRegCurveClass *lin_reg_klass = (GogLinRegCurveClass *) reg_curve_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) reg_curve_klass;

	gog_polynom_reg_curve_parent_klass = g_type_class_peek_parent (reg_curve_klass);

	lin_reg_klass->build_values = gog_polynom_reg_curve_build_values;

	reg_curve_klass->get_value_at = gog_polynom_reg_curve_get_value_at;
	reg_curve_klass->get_equation = gog_polynom_reg_curve_get_equation;
	reg_curve_klass->populate_editor = gog_polynom_reg_curve_populate_editor;

	gog_object_klass->type_name	= gog_polynom_reg_curve_type_name;
}

static void
gog_polynom_reg_curve_init (GogLinRegCurve *model)
{
	GogRegCurve *curve = GOG_REG_CURVE (model);
	g_free (curve->a);
	curve->a = g_new (double, 3);
	curve->a[0] = curve->a[1] = curve->a[2] = go_nan;
	model->dims = 2;
}

GSF_DYNAMIC_CLASS (GogPolynomRegCurve, gog_polynom_reg_curve,
	gog_polynom_reg_curve_class_init, gog_polynom_reg_curve_init,
	GOG_LIN_REG_CURVE_TYPE)
