/*
 * A font selector dialog.
 *
 * Authors:
 *   Morten Welinder (terra@gnome.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */
#include <goffice/goffice-config.h>
#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

struct GOFontSelDialog_ {
	GtkDialog parent;
	GOFontSel *gfs;
};

typedef struct GOFontSelDialogClass_ {
	GtkDialogClass base;
} GOFontSelDialogClass;

static GObjectClass *gfsd_parent_class;

enum {
	PROP_0,

	GFSD_GTK_FONT_CHOOSER_PROP_FIRST           = 0x4000,
	GFSD_GTK_FONT_CHOOSER_PROP_FONT,
	GFSD_GTK_FONT_CHOOSER_PROP_FONT_DESC,
	GFSD_GTK_FONT_CHOOSER_PROP_PREVIEW_TEXT,
	GFSD_GTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY,
#if GTK_CHECK_VERSION(3,24,0)
	GFSD_GTK_FONT_CHOOSER_PROP_LEVEL,
	GFSD_GTK_FONT_CHOOSER_PROP_LANGUAGE,
	GFSD_GTK_FONT_CHOOSER_PROP_FONT_FEATURES,
#endif
	GFSD_GTK_FONT_CHOOSER_PROP_LAST
};

static void
gfsd_set_property (GObject         *object,
		   guint            prop_id,
		   const GValue    *value,
		   GParamSpec      *pspec)
{
	GOFontSelDialog *gfsd = GO_FONT_SEL_DIALOG (object);
	(void)prop_id;
	g_object_set_property (G_OBJECT (gfsd->gfs), pspec->name, value);
}

static void
gfsd_get_property (GObject         *object,
		   guint            prop_id,
		   GValue          *value,
		   GParamSpec      *pspec)
{
	GOFontSelDialog *gfsd = GO_FONT_SEL_DIALOG (object);
	(void)prop_id;
	g_object_get_property (G_OBJECT (gfsd->gfs), pspec->name, value);
}

static void
delegate_notify (GObject *gfs, GParamSpec *pspec, gpointer gfsd)
{
	gpointer iface =
		g_type_interface_peek (g_type_class_peek (G_OBJECT_TYPE (gfs)),
				       GTK_TYPE_FONT_CHOOSER);
	if (g_object_interface_find_property (iface, pspec->name))
		g_object_notify_by_pspec (gfsd, pspec);
}

static void
delegate_font_activated (GtkFontChooser *gfs,
                         const gchar    *fontname,
                         GtkFontChooser *gfsd)
{
	g_signal_emit_by_name (gfsd, "font-activated", 0, fontname);
}

static void
gfsd_init (GOFontSelDialog *gfsd)
{
	GtkDialog *dialog = GTK_DIALOG (gfsd);
	GtkWidget *gfs = g_object_new (GO_TYPE_FONT_SEL, NULL);
	gfsd->gfs = GO_FONT_SEL (gfs);
	gtk_widget_show (gfs);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area (dialog)),
			   gfs);
	gtk_dialog_add_button (dialog, GTK_STOCK_OK, GTK_RESPONSE_OK);
	gtk_dialog_add_button (dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	g_signal_connect (gfs, "notify",
			  G_CALLBACK (delegate_notify), gfsd);
	g_signal_connect (gfs, "font-activated",
			  G_CALLBACK (delegate_font_activated), gfsd);

	g_object_set (gfsd, "title", _("Pick a Font"), NULL);
}

static void
gfsd_class_init (GObjectClass *klass)
{
	klass->set_property = gfsd_set_property;
	klass->get_property = gfsd_get_property;

	gfsd_parent_class = g_type_class_peek_parent (klass);

	g_object_class_override_property (klass,
					  GFSD_GTK_FONT_CHOOSER_PROP_FONT,
					  "font");
	g_object_class_override_property (klass,
					  GFSD_GTK_FONT_CHOOSER_PROP_FONT_DESC,
					  "font-desc");
	g_object_class_override_property (klass,
					  GFSD_GTK_FONT_CHOOSER_PROP_PREVIEW_TEXT,
					  "preview-text");
	g_object_class_override_property (klass,
					  GFSD_GTK_FONT_CHOOSER_PROP_SHOW_PREVIEW_ENTRY,
					  "show-preview-entry");
#if GTK_CHECK_VERSION(3,24,0)
	g_object_class_override_property (klass,
					  GFSD_GTK_FONT_CHOOSER_PROP_LEVEL,
					  "level");
	g_object_class_override_property (klass,
					  GFSD_GTK_FONT_CHOOSER_PROP_LANGUAGE,
					  "language");
	g_object_class_override_property (klass,
					  GFSD_GTK_FONT_CHOOSER_PROP_FONT_FEATURES,
					  "font-features");
#endif
}

static void
gfsd_font_chooser_set_filter_func (GtkFontChooser    *chooser,
				   GtkFontFilterFunc  filter_func,
				   gpointer           filter_data,
				   GDestroyNotify     data_destroy)
{
	GOFontSelDialog *gfsd = GO_FONT_SEL_DIALOG (chooser);
	gtk_font_chooser_set_filter_func (GTK_FONT_CHOOSER (gfsd->gfs),
					  filter_func,
					  filter_data,
					  data_destroy);
}

static PangoFontFamily *
gfsd_font_chooser_get_font_family (GtkFontChooser *chooser)
{
	GOFontSelDialog *gfsd = GO_FONT_SEL_DIALOG (chooser);
	return gtk_font_chooser_get_font_family (GTK_FONT_CHOOSER (gfsd->gfs));
}

static int
gfsd_font_chooser_get_font_size (GtkFontChooser *chooser)
{
	GOFontSelDialog *gfsd = GO_FONT_SEL_DIALOG (chooser);
	return gtk_font_chooser_get_font_size (GTK_FONT_CHOOSER (gfsd->gfs));
}

static PangoFontFace *
gfsd_font_chooser_get_font_face (GtkFontChooser *chooser)
{
	GOFontSelDialog *gfsd = GO_FONT_SEL_DIALOG (chooser);
	return gtk_font_chooser_get_font_face (GTK_FONT_CHOOSER (gfsd->gfs));
}

static void
gfsd_font_chooser_iface_init (GtkFontChooserIface *iface)
{
	iface->get_font_family = gfsd_font_chooser_get_font_family;
	iface->get_font_face = gfsd_font_chooser_get_font_face;
	iface->get_font_size = gfsd_font_chooser_get_font_size;
	iface->set_filter_func = gfsd_font_chooser_set_filter_func;
}

GSF_CLASS_FULL (GOFontSelDialog, go_font_sel_dialog,
		NULL, NULL, gfsd_class_init, NULL,
		gfsd_init, GTK_TYPE_DIALOG, 0,
		GSF_INTERFACE (gfsd_font_chooser_iface_init, GTK_TYPE_FONT_CHOOSER);
	)
#if 0
;
#endif

GtkWidget *
go_font_sel_dialog_new (void)
{
	return g_object_new (GO_TYPE_FONT_SEL_DIALOG, NULL);
}
