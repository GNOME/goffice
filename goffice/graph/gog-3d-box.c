/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-3d-box.h :
 *
 * Copyright (C) 2007 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/graph/gog-3d-box.h>
#include <goffice/math/go-math.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <goffice/graph/gog-chart.h>

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/gtk/go-3d-rotation-sel.h>
#endif

typedef GogObjectClass Gog3DBoxClass;

enum {
	BOX3D_PROP_0,
	BOX3D_PROP_PSI,
	BOX3D_PROP_THETA,
	BOX3D_PROP_PHI,
	BOX3D_PROP_FOV,
};

#ifdef GOFFICE_WITH_GTK

static gboolean
cb_g3d_update (GO3DRotationSel *g3d, GdkEventExpose *event, GObject *gobj)
{
	Gog3DBox *box = GOG_3D_BOX (gobj);
	go_3d_rotation_sel_set_matrix (g3d, &box->mat);
	return FALSE;
}

static void
cb_matrix_changed (GO3DRotationSel *g3d, GObject *gobj)
{
	Gog3DBox *box = GOG_3D_BOX (gobj);

	go_3d_rotation_sel_get_matrix (g3d, &box->mat);
	box->psi = go_3d_rotation_sel_get_psi (g3d);
	box->theta = go_3d_rotation_sel_get_theta (g3d);
	box->phi = go_3d_rotation_sel_get_phi (g3d);
	
	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (gobj)),
	                         TRUE);
}

static void
cb_fov_changed (GO3DRotationSel *g3d, int angle, GObject *gobj)
{
	Gog3DBox *box = GOG_3D_BOX (gobj);

	box->fov = angle * M_PI / 180;
	
	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (gobj)),
	                         TRUE);
}

static gboolean
cb_box_psi_changed (GtkHScale *scale_widget, GdkEventButton *event,
                    GObject *gobj)
{
	Gog3DBox *box = GOG_3D_BOX (gobj);

	box->psi = gtk_range_get_value (GTK_RANGE (scale_widget)) * M_PI / 180;
	go_matrix3x3_from_euler (&box->mat, box->psi, box->theta, box->phi);

	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (gobj)),
	                         TRUE);
	return FALSE;
}

static gboolean
cb_box_theta_changed (GtkHScale *scale_widget, GdkEventButton *event,
                      GObject *gobj)
{
	Gog3DBox *box = GOG_3D_BOX (gobj);

	box->theta = gtk_range_get_value (GTK_RANGE (scale_widget)) * M_PI / 180;
	go_matrix3x3_from_euler (&box->mat, box->psi, box->theta, box->phi);

	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (gobj)),
	                         TRUE);
	return FALSE;
}

static gboolean
cb_box_phi_changed (GtkHScale *scale_widget, GdkEventButton *event,
                    GObject *gobj)
{
	Gog3DBox *box = GOG_3D_BOX (gobj);

	box->phi = gtk_range_get_value (GTK_RANGE (scale_widget)) * M_PI / 180;
	go_matrix3x3_from_euler (&box->mat, box->psi, box->theta, box->phi);

	gog_object_emit_changed (gog_object_get_parent (GOG_OBJECT (gobj)),
	                         TRUE);
	return FALSE;
}

static void
cb_g3d_change_psi (GO3DRotationSel *g3d, int angle, GObject *gobj)
{
	g_signal_handlers_block_matched (GTK_RANGE (gobj), G_SIGNAL_MATCH_FUNC,
		0, 0, 0, G_CALLBACK (cb_box_psi_changed), 0);
	gtk_range_set_value (GTK_RANGE (gobj), angle);
	g_signal_handlers_unblock_matched (GTK_RANGE (gobj), G_SIGNAL_MATCH_FUNC,
		0, 0, 0, G_CALLBACK (cb_box_psi_changed), 0);
}

static void
cb_g3d_change_theta (GO3DRotationSel *g3d, int angle, GObject *gobj)
{
	g_signal_handlers_block_matched (GTK_RANGE (gobj), G_SIGNAL_MATCH_FUNC,
		0, 0, 0, G_CALLBACK (cb_box_theta_changed), 0);
	gtk_range_set_value (GTK_RANGE (gobj), angle);
	g_signal_handlers_unblock_matched (GTK_RANGE (gobj), G_SIGNAL_MATCH_FUNC,
		0, 0, 0, G_CALLBACK (cb_box_theta_changed), 0);
}

static void
cb_g3d_change_phi (GO3DRotationSel *g3d, int angle, GObject *gobj)
{
	g_signal_handlers_block_matched (GTK_RANGE (gobj), G_SIGNAL_MATCH_FUNC,
		0, 0, 0, G_CALLBACK (cb_box_phi_changed), 0);
	gtk_range_set_value (GTK_RANGE (gobj), angle);
	g_signal_handlers_unblock_matched (GTK_RANGE (gobj), G_SIGNAL_MATCH_FUNC,
		0, 0, 0, G_CALLBACK (cb_box_phi_changed), 0);
}

static void
gog_3d_box_populate_editor (GogObject *gobj, 
			  GogEditor *editor, 
			  GogDataAllocator *dalloc, 
			  GOCmdContext *cc)
{
	GtkWidget *w;
	GtkWidget *g3d;
	GladeXML *gui;
	Gog3DBox *box = GOG_3D_BOX(gobj);

	g3d = go_3d_rotation_sel_new ();
	go_3d_rotation_sel_set_matrix (GO_3D_ROTATION_SEL (g3d), &box->mat);
	go_3d_rotation_sel_set_fov (GO_3D_ROTATION_SEL (g3d), box->fov);
	gog_editor_add_page (editor, g3d, _("Rotation"));

	gui = go_libglade_new ("gog-3d-box-prefs.glade", "gog_3d_box_prefs", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	g_signal_connect (G_OBJECT (g3d),
	                  "expose-event",
	                  G_CALLBACK (cb_g3d_update),
	                  GOG_3D_BOX (gobj));
	g_signal_connect (G_OBJECT (g3d),
	                  "matrix-changed",
	                  G_CALLBACK (cb_matrix_changed),
	                  GOG_3D_BOX (gobj));
	g_signal_connect (G_OBJECT (g3d),
	                  "fov-changed",
			  G_CALLBACK (cb_fov_changed),
	                  GOG_3D_BOX (gobj));
	
	w = glade_xml_get_widget (gui, "psi_scale");
	gtk_range_set_value (GTK_RANGE (w), box->psi * 180 / M_PI);
	g_signal_connect (G_OBJECT (w),
	                  "button-release-event",
	                   G_CALLBACK (cb_box_psi_changed),
	                   GOG_3D_BOX (gobj));
	g_signal_connect (G_OBJECT (g3d),
	                  "psi-changed",
	                  G_CALLBACK (cb_g3d_change_psi),
	                  GTK_RANGE (w));

	w = glade_xml_get_widget (gui, "theta_scale");
	gtk_range_set_value (GTK_RANGE (w), box->theta * 180 / M_PI);
	g_signal_connect (G_OBJECT (w),
	                  "button-release-event",
	                  G_CALLBACK (cb_box_theta_changed),
	                  GOG_3D_BOX (gobj));
	g_signal_connect (G_OBJECT (g3d),
	                  "theta-changed",
	                  G_CALLBACK (cb_g3d_change_theta),
	                  GTK_RANGE (w));

	w = glade_xml_get_widget (gui, "phi_scale");
	gtk_range_set_value (GTK_RANGE (w), box->phi * 180 / M_PI);
	g_signal_connect (G_OBJECT (w),
	                  "button-release-event",
	                  G_CALLBACK (cb_box_phi_changed),
	                  GOG_3D_BOX (gobj));
	g_signal_connect (G_OBJECT (g3d),
	                  "phi-changed",
	                  G_CALLBACK (cb_g3d_change_phi),
	                  GTK_RANGE (w));

	w = glade_xml_get_widget (gui, "gog_3d_box_prefs");
	g_object_set_data_full (G_OBJECT (w),
		"state", gui, (GDestroyNotify)g_object_unref);
	
	gog_editor_add_page (editor, w, _("Advanced"));
}

#endif

static void
gog_3d_box_set_property (GObject *obj, guint param_id,
			GValue const *value, GParamSpec *pspec)
{
	Gog3DBox *box = GOG_3D_BOX (obj);

	switch (param_id) {
	case BOX3D_PROP_PSI:
		box->psi = g_value_get_double (value);
		break;
	case BOX3D_PROP_THETA:
		box->theta = g_value_get_double (value);
		break;
	case BOX3D_PROP_PHI:
		box->phi = g_value_get_double (value);
		break;
	case BOX3D_PROP_FOV:
		box->fov = g_value_get_double (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return;
	}
	go_matrix3x3_from_euler (&box->mat, box->psi, box->theta, box->phi);
}

static void
gog_3d_box_get_property (GObject *obj, guint param_id,
			GValue *value, GParamSpec *pspec)
{
	Gog3DBox *box = GOG_3D_BOX (obj);

	switch (param_id) {
	case BOX3D_PROP_PSI:
		g_value_set_double (value, box->psi);
		break;
	case BOX3D_PROP_THETA: 
		g_value_set_double (value, box->theta);
		break;
	case BOX3D_PROP_PHI:
		g_value_set_double (value, box->phi);
		break;
	case BOX3D_PROP_FOV:
		g_value_set_double (value, box->fov);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		break;
	}
}

static void
gog_3d_box_class_init (Gog3DBoxClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) klass;
	GogObjectClass *gog_klass   = (GogObjectClass *) klass;

	gobject_klass->set_property = gog_3d_box_set_property;
	gobject_klass->get_property = gog_3d_box_get_property;
	
	/* Using 3.141593 instead of M_PI to avoid rounding errors */
	g_object_class_install_property (gobject_klass, BOX3D_PROP_PSI,
		g_param_spec_double ("psi", 
			"Psi",
			_("Euler angle psi"),
			0.0, 2 * 3.141593, 70. / 180. * M_PI, 
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE
			| GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX3D_PROP_THETA,
		g_param_spec_double ("theta", 
			"Theta",
			_("Euler angle theta"),
			0.0, 3.141593, 10. / 180. * M_PI, 
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE
			| GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX3D_PROP_PHI,
		g_param_spec_double ("phi", 
			"Phi",
			_("Euler angle phi"),
			0.0, 2 * 3.141593, 270. / 180. * M_PI, 
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE
			| GOG_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX3D_PROP_FOV,
		g_param_spec_double ("fov", 
			"FoV",
			_("Field of view"),
			0.0, 3.141593, 10. / 180. * M_PI,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE
			| GOG_PARAM_PERSISTENT));

#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_3d_box_populate_editor;
#endif
}

static void
gog_3d_box_init (Gog3DBox *box)
{
	box->fov = 10. / 180. * M_PI;
	box->psi = 70. / 180. * M_PI;
	box->theta = 10. / 180. * M_PI;
	box->phi = 270. / 180. * M_PI;
	go_matrix3x3_from_euler (&box->mat, box->psi, box->theta, box->phi);
}

GSF_CLASS (Gog3DBox, gog_3d_box,
	   gog_3d_box_class_init, gog_3d_box_init,
	   GOG_OBJECT_TYPE)
