/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-reg-eqn.c :  
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <goffice/goffice-config.h>
#include <goffice/graph/gog-reg-curve.h>
#include <goffice/graph/gog-reg-eqn.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-style.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/graph/gog-view.h>
#include <goffice/gtk/goffice-gtk.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkspinbutton.h>

typedef GogStyledObjectClass GogRegEqnClass;

static GType gog_reg_eqn_view_get_type (void);

static GObjectClass *reg_eqn_parent_klass;

enum {
	REG_EQN_PROP_0,
	REG_EQN_PROP_HPOS,
	REG_EQN_PROP_VPOS,
	REG_EQN_SHOW_EQ,
	REG_EQN_SHOW_R2
};

static void
cb_text_pos_changed (GtkSpinButton *spinbutton, GogObject *gobj)
{
	char const* property = (char const*) g_object_get_data (G_OBJECT (spinbutton), "prop");
	g_object_set (G_OBJECT (gobj), property, gtk_spin_button_get_value_as_int (spinbutton), NULL);
	gog_object_emit_changed (gobj, FALSE);
}

static void
cb_text_visibility_changed (GtkToggleButton *button, GogObject *gobj)
{
	char const* property = (char const*) g_object_get_data (G_OBJECT (button), "prop");
	g_object_set (G_OBJECT (gobj), property, gtk_toggle_button_get_active (button), NULL);
	gog_object_emit_changed (gobj, FALSE);
}

static void
gog_reg_eqn_set_property (GObject *obj, guint param_id,
			GValue const *value, GParamSpec *pspec)
{
	GogRegEqn *reg_eqn = GOG_REG_EQN (obj);

	switch (param_id) {
	case REG_EQN_PROP_HPOS:
		reg_eqn->hpos = g_value_get_int (value);
		break;
	case REG_EQN_PROP_VPOS:
		reg_eqn->vpos = g_value_get_int (value);
		break;
	case REG_EQN_SHOW_EQ:
		reg_eqn->show_eq = g_value_get_boolean (value);
		break;
	case REG_EQN_SHOW_R2:
		reg_eqn->show_r2 = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return;
	}
}

static void
gog_reg_eqn_get_property (GObject *obj, guint param_id,
			GValue *value, GParamSpec *pspec)
{
	GogRegEqn *reg_eqn = GOG_REG_EQN (obj);

	switch (param_id) {
	case REG_EQN_PROP_HPOS:
		g_value_set_int (value, reg_eqn->hpos);
		break;
	case REG_EQN_PROP_VPOS:
		g_value_set_int (value, reg_eqn->vpos);
		break;
	case REG_EQN_SHOW_EQ:
		g_value_set_boolean (value, reg_eqn->show_eq);
		break;
	case REG_EQN_SHOW_R2:
		g_value_set_boolean (value, reg_eqn->show_r2);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return;
	}
}

static void
gog_reg_eqn_init_style (GogStyledObject *gso, GogStyle *style)
{
	style->interesting_fields = GOG_STYLE_OUTLINE | GOG_STYLE_FILL |
								GOG_STYLE_FONT;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, FALSE);
}

static void
gog_reg_eqn_populate_editor (GogObject *gobj, 
			  GogEditor *editor, 
			  GogDataAllocator *dalloc, 
			  GOCmdContext *cc)
{
	GtkWidget *w;
	GladeXML *gui;
	GogRegEqn *reg_eqn = GOG_REG_EQN (gobj);

	gui = go_libglade_new ("gog-reg-eqn-prefs.glade", "reg-eqn-prefs", NULL, cc);
	if (gui == NULL)
		return;

	gog_editor_add_page (editor, 
			     glade_xml_get_widget (gui, "reg-eqn-prefs"),
			     _("Details"));

	w = glade_xml_get_widget (gui, "show_eq");
	g_object_set_data (G_OBJECT (w), "prop", (void*) "show-eq");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), reg_eqn->show_eq);
	g_signal_connect (w, "toggled", G_CALLBACK (cb_text_visibility_changed), gobj);

	w = glade_xml_get_widget (gui, "show_r2");
	g_object_set_data (G_OBJECT (w), "prop", (void*) "show-r2");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), reg_eqn->show_r2);
	g_signal_connect (w, "toggled", G_CALLBACK (cb_text_visibility_changed), gobj);

	w = glade_xml_get_widget (gui, "hpos");
	g_object_set_data (G_OBJECT (w), "prop", (void*) "text_hpos");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), reg_eqn->hpos);
	g_signal_connect (w, "value_changed", G_CALLBACK (cb_text_pos_changed), gobj);

	w = glade_xml_get_widget (gui, "vpos");
	g_object_set_data (G_OBJECT (w), "prop", (void*) "text_vpos");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), reg_eqn->vpos);
	g_signal_connect (w, "value_changed", G_CALLBACK (cb_text_pos_changed), gobj);

	(GOG_OBJECT_CLASS(reg_eqn_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
}

static char const *
gog_reg_eqn_type_name (GogObject const *gobj)
{
	return N_("Regression Equation");
}

static void
gog_reg_eqn_class_init (GogObjectClass *gog_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *)gog_klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) gog_klass;
	reg_eqn_parent_klass = g_type_class_peek_parent (gog_klass);

	gog_klass->populate_editor	= gog_reg_eqn_populate_editor;
	style_klass->init_style = gog_reg_eqn_init_style;
	gog_klass->type_name	= gog_reg_eqn_type_name;
	gog_klass->view_type	= gog_reg_eqn_view_get_type ();
	gobject_klass->set_property	= gog_reg_eqn_set_property;
	gobject_klass->get_property	= gog_reg_eqn_get_property;
	g_object_class_install_property (gobject_klass, REG_EQN_PROP_HPOS,
		g_param_spec_int ("text_hpos", "text_hpos",
			"Horizontal text position",
			0, 100, 1, 
			G_PARAM_READWRITE|GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, REG_EQN_PROP_VPOS,
		g_param_spec_int ("text_vpos", "text_vpos",
			"Vertical text position",
			0, 100, 1, 
			G_PARAM_READWRITE|GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, REG_EQN_SHOW_EQ,
		g_param_spec_boolean ("show-eq", NULL,
			"Show the equation on the graph",
			FALSE, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, REG_EQN_SHOW_R2,
		g_param_spec_boolean ("show-r2", NULL,
			"Show the correlation coefficient on the graph",
			FALSE, G_PARAM_READWRITE | GOG_PARAM_PERSISTENT));
}

static void
gog_reg_eqn_init (GogRegEqn *reg_eqn)
{
	reg_eqn->hpos = 1;
	reg_eqn->vpos = 1;
	reg_eqn->show_eq = TRUE;
	reg_eqn->show_r2 = TRUE;
}

GSF_CLASS (GogRegEqn, gog_reg_eqn,
	   gog_reg_eqn_class_init, gog_reg_eqn_init,
	   GOG_STYLED_OBJECT_TYPE)

/****************************************************************************/

typedef GogView		GogRegEqnView;
typedef GogViewClass	GogRegEqnViewClass;

#define GOG_REG_EQN_VIEW_TYPE	(gog_reg_eqn_view_get_type ())
#define GOG_REG_EQN_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_REG_CURVE_VIEW_TYPE, GogRegEqnView))
#define IS_GOG_REG_EQN_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_REG_EQN_VIEW_TYPE))

static void
gog_reg_eqn_view_render (GogView *view, GogViewAllocation const *bbox)
{
	double text_width, text_height;
	char const *eq = NULL;
	char *r2 = NULL;
	GogViewRequisition eqreq, r2req;
	GogViewAllocation alloc, result;
	/* We should have a more intelligent padding */
	double padding = 8.;
	GogRegEqn *re = GOG_REG_EQN (view->model);
	GogRegCurve *rc = GOG_REG_CURVE ((GOG_OBJECT (re))->parent);
	GogStyle *style = GOG_STYLED_OBJECT (re)->style;;

	text_width = text_height = 0;
	gog_renderer_push_style (view->renderer, style);
	if (re->show_eq) {
		eq = gog_reg_curve_get_equation (rc);
		gog_renderer_measure_text (view->renderer, eq, &eqreq);
		text_width = eqreq.w;
		text_height = eqreq.h;
	} else
		text_width = text_height = 0;
	if (re->show_r2) {
		r2 = g_strdup_printf ("R² = %g", gog_reg_curve_get_R2 (rc));
		gog_renderer_measure_text (view->renderer, r2, &r2req);
		if (text_width < r2req.w)
			text_width = r2req.w;
		text_height += r2req.h;
	}
	alloc.x = view->residual.x + (view->residual.w - text_width) * re->hpos / 100. - padding;
	alloc.y = view->residual.y + (view->residual.h - text_height) * re->vpos / 100. - padding;
	alloc.w = text_width + 2. * padding;
	alloc.h = text_height + 2. * padding;
	gog_renderer_draw_rectangle (view->renderer, &alloc, bbox);
	alloc.x += padding;
	alloc.y += padding;
	if (re->show_eq) {
		alloc.w = eqreq.w;
		alloc.h = eqreq.h;
		gog_renderer_draw_text (view->renderer, eq, &alloc, GTK_ANCHOR_NW, &result);
		alloc.y += alloc.h;
	}
	if (re->show_r2) {
		alloc.w = r2req.w;
		alloc.h = r2req.h;
		gog_renderer_draw_text (view->renderer, r2, &alloc, GTK_ANCHOR_NW, &result);
		g_free (r2);
	}
	gog_renderer_pop_style (view->renderer);
}

static void
gog_reg_eqn_view_class_init (GogRegEqnViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;

	view_klass->render	  = gog_reg_eqn_view_render;
}

static GSF_CLASS (GogRegEqnView, gog_reg_eqn_view,
		  gog_reg_eqn_view_class_init, NULL,
		  GOG_VIEW_TYPE)
