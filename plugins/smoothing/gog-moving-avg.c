/*
 * smoothing/gog-moving-avg.c :
 *
 * Copyright (C) 2006 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-moving-avg.h"
#include <goffice/app/go-plugin.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-persist.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

enum {
	MOVING_AVG_PROP_0,
	MOVING_AVG_PROP_SPAN,
	MOVING_AVG_PROP_XAVG,
};

static GObjectClass *moving_avg_parent_klass;

static void
gog_moving_avg_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogMovingAvg *ma = GOG_MOVING_AVG (obj);
	switch (param_id) {
	case MOVING_AVG_PROP_SPAN:
		g_value_set_int (value, ma->span);
		break;
	case MOVING_AVG_PROP_XAVG:
		g_value_set_boolean (value, ma->xavg);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_moving_avg_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogMovingAvg *ma = GOG_MOVING_AVG (obj);
	switch (param_id) {
	case MOVING_AVG_PROP_SPAN:
		ma->span = g_value_get_int (value);
		gog_object_request_update (GOG_OBJECT (obj));
		break;
	case MOVING_AVG_PROP_XAVG:
		ma->xavg = g_value_get_boolean (value);
		gog_object_request_update (GOG_OBJECT (obj));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#include <gtk/gtk.h>

static void
span_changed_cb (GtkSpinButton *button, GObject *ma)
{
	g_object_set (ma, "span", gtk_spin_button_get_value_as_int (button), NULL);
}

static void
xavg_toggled_cb (GtkToggleButton *button, GObject *ma)
{
	g_object_set (ma, "xavg", gtk_toggle_button_get_active (button), NULL);
}

static void
gog_moving_avg_populate_editor (GogObject *obj,
				GOEditor *editor,
				GogDataAllocator *dalloc,
				GOCmdContext *cc)
{
	GogMovingAvg *ma = GOG_MOVING_AVG (obj);
	GtkBuilder *gui =
		go_gtk_builder_load ("res:go:smoothing/gog-moving-avg.ui",
				    GETTEXT_PACKAGE, cc);
	GtkWidget *w = go_gtk_builder_get_widget (gui, "span");
	gtk_widget_set_tooltip_text (w, _("Number of values from which to calculate an average"));
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (w), 2, G_MAXINT);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), ma->span);
	g_signal_connect (G_OBJECT (w), "value-changed", G_CALLBACK (span_changed_cb), obj);
	w = go_gtk_builder_get_widget (gui, "xavg");
	gtk_widget_set_tooltip_text (w, _("Whether to average x values as well or use the last one"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), ma->xavg);
	g_signal_connect (G_OBJECT (w), "toggled", G_CALLBACK (xavg_toggled_cb), obj);
	w = go_gtk_builder_get_widget (gui, "mv-avg-prefs");
	go_editor_add_page (editor, w, _("Properties"));
	g_object_unref (gui);

	(GOG_OBJECT_CLASS (moving_avg_parent_klass)->populate_editor) (obj, editor, dalloc, cc);
}
#endif

static void
gog_moving_avg_update (GogObject *obj)
{
	GogMovingAvg *ma = GOG_MOVING_AVG (obj);
	GogSeries *series = GOG_SERIES (obj->parent);
	double const *y_vals, *x_vals;
	double xtot = 0., ytot = 0.;
	int nb, i, j, invalid;

	g_free (ma->base.x);
	ma->base.x = NULL;
	g_free (ma->base.y);
	ma->base.y = NULL;
	if (!gog_series_is_valid (series))
		return;

	nb = gog_series_get_xy_data (series, &x_vals, &y_vals);
	if (nb < ma->span || y_vals == NULL)
		return;
	ma->base.nb = nb - ma->span + 1;
	ma->base.x = g_new (double, ma->base.nb);
	ma->base.y = g_new (double, ma->base.nb);
	invalid = ma->span;
	for (i = 0, j = 1 - ma->span; i < nb; i++, j++) {
		if ((x_vals && !go_finite (x_vals[i])) || !go_finite (y_vals[i])) {
			invalid = ma->span;
			xtot = ytot = 0;
			if (j >= 0)
				ma->base.x[j] = ma->base.y[j] = go_nan;
			continue;
		}
		if (invalid == 0) {
			xtot -= (x_vals)? x_vals[i - ma->span]: i - ma->span;
			ytot -= y_vals[i - ma->span];
		} else
			invalid --;
		xtot += (x_vals)? x_vals[i]: i;
		ytot += y_vals[i];
		if (j >= 0) {
			ma->base.x[j] = (ma->xavg)
				? ((invalid == 0) ? xtot / ma->span: go_nan)
				: ((x_vals)? x_vals[i]: i);
			ma->base.y[j] = (invalid == 0)
				? ytot / ma->span
				: go_nan;
		}
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static char const *
gog_moving_avg_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name moving averge smoothed curves objects
	 * eg The 2nd one for a series will be called
	 * 	Moving average2 */
	return N_("Moving average");
}

static void
gog_moving_avg_class_init (GogSmoothedCurveClass *curve_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) curve_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) curve_klass;
	moving_avg_parent_klass = g_type_class_peek_parent (curve_klass);

	gobject_klass->get_property = gog_moving_avg_get_property;
	gobject_klass->set_property = gog_moving_avg_set_property;
#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor = gog_moving_avg_populate_editor;
#endif
	gog_object_klass->update = gog_moving_avg_update;
	gog_object_klass->type_name	= gog_moving_avg_type_name;

	g_object_class_install_property (gobject_klass, MOVING_AVG_PROP_SPAN,
		g_param_spec_int ("span",
			_("Span"),
			_("Number of averaged values"),
			2, G_MAXINT, 3,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, MOVING_AVG_PROP_XAVG,
		g_param_spec_boolean ("xavg",
			_("Average X"),
			_("Use averaged x values"),
		       	TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
gog_moving_avg_init (GogMovingAvg *model)
{
	model->span = 3;
	model->xavg = TRUE;
}

GSF_DYNAMIC_CLASS (GogMovingAvg, gog_moving_avg,
	gog_moving_avg_class_init, gog_moving_avg_init,
	GOG_TYPE_SMOOTHED_CURVE)
