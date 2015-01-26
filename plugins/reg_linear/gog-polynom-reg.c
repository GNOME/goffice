/* vm: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 * gog-polynom-reg.c :
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
#include "gog-polynom-reg.h"
#include <goffice/graph/gog-reg-curve.h>
#include <goffice/math/go-math.h>
#include <goffice/math/go-regression.h>
#include <glib/gi18n-lib.h>

#include <gsf/gsf-impl-utils.h>

#define UNICODE_MINUS_SIGN_C 0x2212
static int minus_utf8_len;
static char minus_utf8[6];

static GogObjectClass *gog_polynom_reg_curve_parent_klass;

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
		g_free (rc->x_vals[i]);
		rc->x_vals[i] = g_new (double, n);
	}
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

static const char *const exponent[10] = {
	"\xE2\x81\xB0",
	"\xC2\xB9",
	"\xC2\xB2",
	"\xC2\xB3",
	"\xE2\x81\xB4",
	"\xE2\x81\xB5",
	"\xE2\x81\xB6",
	"\xE2\x81\xB7",
	"\xE2\x81\xB8",
	"\xE2\x81\xB9"
};

/* Add an exponent, i.e., a super-scripted number.  */
static void
append_exponent (GString *res, unsigned int i)
{
	if (i >= 10) {
		append_exponent (res, i / 10);
		i %= 10;
	}
	g_string_append (res, exponent[i]);
}


static void
append_number (GString *res, double c, gboolean suppress1)
{
	size_t prelen = res->len;

	g_string_append_printf (res, "%g", c);

	if (suppress1 && res->len == prelen + 1 && res->str[prelen] == '1')
		g_string_truncate (res, prelen);
	else {
		/* Handle the minuses as in -1.2222e-3.  */
		size_t i;
		for (i = prelen; i < res->len; i++)
			if (res->str[i] == '-') {
				res->str[i] = minus_utf8[0];
				g_string_insert_len (res, i + 1, minus_utf8 + 1, minus_utf8_len - 1);
				i += minus_utf8_len - 1;
			}
	}
}


static gchar const*
gog_polynom_reg_curve_get_equation (GogRegCurve *curve)
{
	if (!curve->equation) {
		GogLinRegCurve *lin = GOG_LIN_REG_CURVE (curve);
		GString *str = g_string_new ("y =");
		int i, lasti = (lin->affine ? 0 : 1), j = 0;

		for (i = lin->dims; i >= lasti; i--) {
			double c_i = curve->a[i];

			if (c_i == 0.) /* a very rare event */
				continue;

			/* Break after every three terms present.  */
			if (j > 0 && j % 3 == 0)
				g_string_append_c (str, '\n');
			j++;

			g_string_append_c (str, ' ');
			if (j != 1) {
				if (c_i >= 0)
					g_string_append_c (str, '+');
				else {
					g_string_append_len (str, minus_utf8, minus_utf8_len);
					c_i = -c_i;
				}
				g_string_append_c (str, ' ');
			}

			append_number (str, c_i, i >= 1);

			if (i >= 1) {
				g_string_append_c (str, 'x');
				if (i > 1)
					append_exponent (str, i);
			}
		}
		if (j == 0) {
			/* Nothing whatsoever.  */
			g_string_append (str, " 0");
		}

		curve->equation = g_string_free (str, FALSE);
	}
	return curve->equation;
}

#ifdef GOFFICE_WITH_GTK
#include <gtk/gtk.h>
static void
order_changed_cb (GtkSpinButton *btn, GObject *obj)
{
	g_object_set (obj, "dims", gtk_spin_button_get_value_as_int (btn), NULL);
}

static void
gog_polynom_reg_curve_populate_editor (GogRegCurve *reg_curve, gpointer table)
{
	GtkWidget *l, *w;
	GogLinRegCurve *lin = GOG_LIN_REG_CURVE (reg_curve);

	((GogRegCurveClass*) gog_polynom_reg_curve_parent_klass)->populate_editor (reg_curve, table);
	l = gtk_label_new (_("Order:"));
	gtk_misc_set_alignment (GTK_MISC (l), 0., 0.5);
	gtk_label_set_justify (GTK_LABEL (l), GTK_JUSTIFY_LEFT);
	gtk_widget_show (l);
	gtk_grid_attach_next_to (table, l, NULL, GTK_POS_BOTTOM, 1, 1);
	w = gtk_spin_button_new_with_range (2, 10, 1);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (w), 0);
	gtk_widget_show (w);
	gtk_grid_attach_next_to (table, w, l, GTK_POS_RIGHT, 1, 1);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), lin->dims);
	g_signal_connect (G_OBJECT (w), "value-changed", G_CALLBACK (order_changed_cb), lin);
}
#endif

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
	lin_reg_klass->max_dims = 10;

	reg_curve_klass->get_value_at = gog_polynom_reg_curve_get_value_at;
	reg_curve_klass->get_equation = gog_polynom_reg_curve_get_equation;
#ifdef GOFFICE_WITH_GTK
	reg_curve_klass->populate_editor = gog_polynom_reg_curve_populate_editor;
#endif

	gog_object_klass->type_name	= gog_polynom_reg_curve_type_name;

	minus_utf8_len = g_unichar_to_utf8 (UNICODE_MINUS_SIGN_C, minus_utf8);
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
	GOG_TYPE_LIN_REG_CURVE)
