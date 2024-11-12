/*
 * gog-3d-box.h :
 *
 * Copyright (C) 2007 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/graph/gog-3d-box.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/math/go-math.h>
#include <goffice/utils/go-persist.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

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
cb_g3d_update (GO3DRotationSel *g3d, GObject *gobj)
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
cb_box_psi_changed (GtkScale *scale_widget, GdkEventButton *event,
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
cb_box_theta_changed (GtkScale *scale_widget, GdkEventButton *event,
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
cb_box_phi_changed (GtkScale *scale_widget, GdkEventButton *event,
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
			  GOEditor *editor,
			  GogDataAllocator *dalloc,
			  GOCmdContext *cc)
{
	GtkWidget *w;
	GtkWidget *g3d;
	GtkBuilder *gui;
	Gog3DBox *box = GOG_3D_BOX(gobj);

	g3d = go_3d_rotation_sel_new ();
	go_3d_rotation_sel_set_matrix (GO_3D_ROTATION_SEL (g3d), &box->mat);
	go_3d_rotation_sel_set_fov (GO_3D_ROTATION_SEL (g3d), box->fov);
	go_editor_add_page (editor, g3d, _("Rotation"));

	gui = go_gtk_builder_load_internal ("res:go:graph/gog-3d-box-prefs.ui", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	g_object_connect (G_OBJECT (g3d),
		"signal::realize",   G_CALLBACK (cb_g3d_update), gobj, /* why do we need that? */
		"signal::matrix-changed", G_CALLBACK (cb_matrix_changed), gobj,
		"signal::fov-changed",    G_CALLBACK (cb_fov_changed), gobj,
		NULL);

	w = go_gtk_builder_get_widget (gui, "psi-scale");
	gtk_range_set_value (GTK_RANGE (w), box->psi * 180 / M_PI);
	g_object_connect (G_OBJECT (w),
		"signal::button-release-event", G_CALLBACK (cb_box_psi_changed), gobj,
		"signal::key-release-event",    G_CALLBACK (cb_box_psi_changed), gobj,
		NULL);
	g_signal_connect (G_OBJECT (g3d),
	                  "psi-changed",
	                  G_CALLBACK (cb_g3d_change_psi),
	                  GTK_RANGE (w));

	w = go_gtk_builder_get_widget (gui, "theta-scale");
	gtk_range_set_value (GTK_RANGE (w), box->theta * 180 / M_PI);
	g_object_connect (G_OBJECT (w),
		"signal::button-release-event", G_CALLBACK (cb_box_theta_changed), gobj,
		"signal::key-release-event",    G_CALLBACK (cb_box_theta_changed), gobj,
		NULL);
	g_signal_connect (G_OBJECT (g3d),
	                  "theta-changed",
	                  G_CALLBACK (cb_g3d_change_theta),
	                  GTK_RANGE (w));

	w = go_gtk_builder_get_widget (gui, "phi-scale");
	gtk_range_set_value (GTK_RANGE (w), box->phi * 180 / M_PI);
	g_object_connect (G_OBJECT (w),
		"signal::button-release-event", G_CALLBACK (cb_box_phi_changed), gobj,
		"signal::key-release-event",    G_CALLBACK (cb_box_phi_changed), gobj,
		NULL);
	g_signal_connect (G_OBJECT (g3d),
	                  "phi-changed",
	                  G_CALLBACK (cb_g3d_change_phi),
	                  GTK_RANGE (w));

	w = go_gtk_builder_get_widget (gui, "gog-3d-box-prefs");
	g_object_set_data_full (G_OBJECT (w),
		"state", gui, (GDestroyNotify) g_object_unref);

	go_editor_add_page (editor, w, _("Advanced"));
}

#endif

static void
gog_3d_box_set_property (GObject *obj, guint param_id,
			GValue const *value, GParamSpec *pspec)
{
	Gog3DBox *box = GOG_3D_BOX (obj);

	switch (param_id) {
	case BOX3D_PROP_PSI:
		box->psi = g_value_get_int (value) * M_PI / 180.;
		break;
	case BOX3D_PROP_THETA:
		box->theta = g_value_get_int (value) * M_PI / 180.;
		break;
	case BOX3D_PROP_PHI:
		box->phi = g_value_get_int (value) * M_PI / 180.;
		break;
	case BOX3D_PROP_FOV:
		box->fov = g_value_get_int (value) * M_PI / 180.;
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
		g_value_set_int (value, (int) (box->psi * 180. / M_PI));
		break;
	case BOX3D_PROP_THETA:
		g_value_set_int (value, (int) (box->theta * 180. / M_PI));
		break;
	case BOX3D_PROP_PHI:
		g_value_set_int (value, (int) (box->phi * 180. / M_PI));
		break;
	case BOX3D_PROP_FOV:
		g_value_set_int (value, (int) (box->fov * 180. / M_PI));
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
#ifdef GOFFICE_WITH_GTK
	GogObjectClass *gog_klass   = (GogObjectClass *) klass;
#endif

	gobject_klass->set_property = gog_3d_box_set_property;
	gobject_klass->get_property = gog_3d_box_get_property;

	/* Using 3.141593 instead of M_PI to avoid rounding errors */
	g_object_class_install_property (gobject_klass, BOX3D_PROP_PSI,
		g_param_spec_int ("psi",
			"Psi",
			_("Euler angle psi"),
			0, 360, 70,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE
			| GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX3D_PROP_THETA,
		g_param_spec_int ("theta",
			"Theta",
			_("Euler angle theta"),
			0, 360, 10,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE
			| GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX3D_PROP_PHI,
		g_param_spec_int ("phi",
			"Phi",
			_("Euler angle phi"),
			0, 360, 270,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE
			| GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, BOX3D_PROP_FOV,
		g_param_spec_int ("fov",
			"FoV",
			_("Field of view"),
			0, 90, 10,
			G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE
			| GO_PARAM_PERSISTENT));

#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor = gog_3d_box_populate_editor;
	gog_klass->view_type = gog_3d_box_view_get_type ();
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

typedef GogViewClass Gog3DBoxViewClass;

GSF_CLASS (Gog3DBox, gog_3d_box,
	   gog_3d_box_class_init, gog_3d_box_init,
	   GOG_TYPE_OBJECT)

/* Gog3DViewClass*/

GSF_CLASS (Gog3DBoxView, gog_3d_box_view,
	   NULL, NULL,
	   GOG_TYPE_VIEW)
