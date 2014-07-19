/*
 * gog-trend-line.c :
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
#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

enum {
	TREND_LINE_PROP_0,
	TREND_LINE_PROP_HAS_LEGEND
};
static GObjectClass *trend_line_parent_klass;

static void
gog_trend_line_set_property (GObject *obj, guint param_id,
			 GValue const *value, GParamSpec *pspec)
{
	gboolean b_tmp;

	switch (param_id) {
	case TREND_LINE_PROP_HAS_LEGEND :
		b_tmp = g_value_get_boolean (value);
		if (GPOINTER_TO_INT (g_object_get_data (obj, "has-legend")) ^ b_tmp) {
			GogSeries *series = GOG_SERIES (gog_object_get_parent (GOG_OBJECT (obj)));
			g_object_set_data (obj, "has-legend", GINT_TO_POINTER (b_tmp));
			if (series->plot != NULL)
				gog_plot_request_cardinality_update (series->plot);
		}
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}

	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_trend_line_get_property (GObject *obj, guint param_id,
			 GValue *value, GParamSpec *pspec)
{
	switch (param_id) {
	case TREND_LINE_PROP_HAS_LEGEND :
		g_value_set_boolean (value, GPOINTER_TO_INT (g_object_get_data (obj, "has-legend")));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

#ifdef GOFFICE_WITH_GTK
static void
cb_show_in_legend (GtkToggleButton *b, GObject *line)
{
	g_object_set (line,
		"has-legend", gtk_toggle_button_get_active (b),
		NULL);
}

static void
gog_trend_line_populate_editor (GogObject *gobj,
				GOEditor *editor,
				GogDataAllocator *dalloc,
				GOCmdContext *cc)
{
	GtkWidget *w, *box;
	GogTrendLine *line = GOG_TREND_LINE (gobj);

	box = go_editor_get_page (editor, _("Properties"));
	if (!box)
		box = go_editor_get_page (editor, _("Details"));
	if (!box) {
		box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
		gtk_container_set_border_width (GTK_CONTAINER (box), 12);
		gtk_widget_show_all (box);
		go_editor_add_page (editor, box, _("Legend"));
	}
	w = gtk_check_button_new_with_mnemonic (_("_Show in Legend"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
		gog_trend_line_has_legend (line));
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_show_in_legend), gobj);
	if (GTK_IS_BOX (box))
		gtk_box_pack_start (GTK_BOX (box), w, FALSE, FALSE, 0);
	else if (GTK_IS_GRID (box)) {
		GtkGrid *grid = GTK_GRID (box);
		gtk_grid_insert_row (grid, 1);
		gtk_grid_attach (grid, w, 0, 1, 2, 1);
	} else if (GTK_IS_CONTAINER (box))
		gtk_container_add (GTK_CONTAINER (box), w);
	else
		g_warning ("Unsupported container");
	gtk_widget_show (w);

	(GOG_OBJECT_CLASS (trend_line_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
}
#endif

static void
gog_trend_line_init_style (GogStyledObject *gso, GOStyle *style)
{
	style->interesting_fields = GO_STYLE_LINE;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, GO_STYLE_LINE);
}

static char const *
gog_trend_line_type_name (GogObject const *gobj)
{
	return N_("Trend Line");
}

static void
gog_trend_line_class_init (GogObjectClass *gog_klass)
{
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;
	GObjectClass *gobject_klass = (GObjectClass *) gog_klass;
	trend_line_parent_klass = g_type_class_peek_parent (gog_klass);

	style_klass->init_style = gog_trend_line_init_style;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor	= gog_trend_line_populate_editor;
#endif
	gog_klass->type_name	= gog_trend_line_type_name;
	gobject_klass->set_property	= gog_trend_line_set_property;
	gobject_klass->get_property	= gog_trend_line_get_property;
	g_object_class_install_property (gobject_klass, TREND_LINE_PROP_HAS_LEGEND,
		g_param_spec_boolean ("has-legend",
			_("Has-legend"),
			_("Should the trend line show up in legends"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
gog_trend_line_init (GObject *obj)
{
	g_object_set_data (obj, "has-legend", GINT_TO_POINTER (TRUE));
}

GSF_CLASS_ABSTRACT (GogTrendLine, gog_trend_line,
	   gog_trend_line_class_init, gog_trend_line_init,
	   GOG_TYPE_STYLED_OBJECT)

GogTrendLine *
gog_trend_line_new_by_type (GogTrendLineType const *type)
{
	GogTrendLine *res;

	g_return_val_if_fail (type != NULL, NULL);

	res = gog_trend_line_new_by_name (type->engine);
	if (res != NULL && type->properties != NULL)
		g_hash_table_foreach (type->properties,
			(GHFunc) gog_object_set_arg, res);
	return res;
}

gboolean
gog_trend_line_has_legend (GogTrendLine const *line)
{
	return GPOINTER_TO_INT (g_object_get_data (G_OBJECT (line), "has-legend"));
}
