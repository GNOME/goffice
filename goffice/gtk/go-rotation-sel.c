/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-rotation-sel.c: A widget to select a text orientation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/gtk/go-gtk-compat.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>

struct _GORotationSel {
	GtkHBox		 box;
	GtkBuilder	*gui;
	int		 angle;

	GtkSpinButton	*rotate_spinner;
	GocCanvas       *rotate_canvas;
	GocItem		*rotate_marks[13];
	GocItem		*line;
	GtkWidget       *text_widget;
	GocItem		*text;
	int		 rot_width, rot_height;
	gulong		 motion_handle;
};

typedef struct {
	GtkHBoxClass parent_class;
	void (* rotation_changed) (GORotationSel *grs, int angle);
} GORotationSelClass;

enum {
	ROTATION_CHANGED,
	LAST_SIGNAL
};

static guint grs_signals[LAST_SIGNAL] = { 0 };
static GObjectClass *grs_parent_class;

static void
cb_rotate_changed (GORotationSel *grs)
{
	GOColor colour;
	int i;

	go_rotation_sel_set_rotation (grs,
		gtk_spin_button_get_value_as_int (grs->rotate_spinner) % 360);

	for (i = 0 ; i <= 12 ; i++)
		if (grs->rotate_marks[i] != NULL) {
			GOStyle *style = go_styled_object_get_style (GO_STYLED_OBJECT (grs->rotate_marks[i]));
			colour = (grs->angle == (i-6)*15) ? GO_COLOR_GREEN : GO_COLOR_BLACK;
			if (style->fill.pattern.back != colour){
				style->fill.pattern.back = colour;
				goc_item_invalidate (grs->rotate_marks[i]);
			}
		}

	if (grs->line != NULL) {
		double rad = grs->angle * M_PI / 180.;
		goc_item_set (grs->line,
		              "x0", 15 + cos (rad) * grs->rot_width,
		              "y0", 100 - sin (rad) * grs->rot_width,
		              "x1", 15 + cos (rad) * 72.,
		              "y1", 100 - sin (rad) * 72.,
		              NULL);
	}

	if (grs->text) {
		double x = 15.0;
		double y = 100.0;
		double rad = grs->angle * M_PI / 180.;
		double w = grs->rot_width * cos (fabs (rad)) + grs->rot_height * sin (fabs (rad));
		double h = grs->rot_width * sin (fabs (rad)) + grs->rot_height * cos (fabs (rad));
		x -= grs->rot_height * sin (fabs (rad)) / 2;
		y -= grs->rot_height * cos (rad) / 2;
		if (rad >= 0)
			y -= grs->rot_width * sin (rad);
		goc_item_set (grs->text, "x", x, "y", y,
		              "width", w, "height", h, NULL);
		gtk_label_set_angle (GTK_LABEL (grs->text_widget),
			(grs->angle + 360) % 360);
	}
}

static void
cb_rotate_canvas_realize (GocCanvas *canvas, GORotationSel *grs)
{
	GocGroup  *group = goc_canvas_get_root (canvas);
	int i;
	GOStyle *go_style;
	GtkStyle *style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (canvas)));
	style->bg[GTK_STATE_NORMAL] = style->white;
	gtk_widget_set_style (GTK_WIDGET (canvas), style);
	g_object_unref (style);

	for (i = 0 ; i <= 12 ; i++) {
		double rad = (i-6) * M_PI / 12.;
		double x = 15 + cos (rad) * 80.;
		double y = 100 - sin (rad) * 80.;
		double size = (i % 3) ? 3.0 : 4.0;
		GocItem *item = goc_item_new (group,
			GOC_TYPE_CIRCLE,
			"x", x,	"y", y,
			"radius", size,
			NULL);
		go_style = go_styled_object_get_style (GO_STYLED_OBJECT (item));
		go_style->line.width = 1.;
		go_style->line.color = GO_COLOR_BLACK;
		go_style->fill.pattern.back = GO_COLOR_BLACK;
		grs->rotate_marks[i] = item;
	}
	grs->line = goc_item_new (group, GOC_TYPE_LINE, NULL);
	go_style = go_styled_object_get_style (GO_STYLED_OBJECT (grs->line));
	go_style->line.width = 2.;
	go_style->line.color = GO_COLOR_BLACK;

	{
		int w, h;
		GtkWidget *tw = grs->text_widget = gtk_label_new (_("Text"));
		PangoAttrList *attrs = pango_attr_list_new ();
		PangoAttribute *attr = pango_attr_scale_new (1.3);
		attr->start_index = 0;
		attr->end_index = -1;
		pango_attr_list_insert (attrs, attr);
#ifdef DEBUG_ROTATION
		attr = pango_attr_background_new (0xffff, 0, 0);
		attr->start_index = 0;
		attr->end_index = -1;
		pango_attr_list_insert (attrs, attr);
#endif
		gtk_label_set_attributes (GTK_LABEL (tw), attrs);
		pango_attr_list_unref (attrs);

		pango_layout_get_pixel_size (gtk_label_get_layout (GTK_LABEL (tw)), &w, &h);
		grs->rot_width  = w;
		grs->rot_height = h;

		grs->text = goc_item_new (group, GOC_TYPE_WIDGET,
			"widget", tw, NULL);
		gtk_widget_show (tw);
	}

	cb_rotate_changed (grs);
}

static void
set_rot_from_point (GORotationSel *grs, double x, double y)
{
	double degrees;
	x -= 15.;	if (x < 0.) x = 0.;
	y -= 100.;

	degrees = atan2 (-y, x) * 180 / M_PI;

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (grs->rotate_spinner),
		go_fake_round (degrees));
}

static gboolean
cb_rotate_motion_notify_event (G_GNUC_UNUSED GocCanvas *canvas, GdkEventMotion *event,
			       GORotationSel *grs)
{
	set_rot_from_point (grs,  event->x, event->y);
	return TRUE;
}

static gboolean
cb_rotate_canvas_button (GocCanvas *canvas, GdkEventButton *event, GORotationSel *grs)
{
	if (event->type == GDK_BUTTON_PRESS) {
		set_rot_from_point (grs, event->x, event->y);
		if (grs->motion_handle == 0) {
			gdk_pointer_grab (gtk_layout_get_bin_window (&canvas->base), FALSE,
				GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				NULL, NULL, event->time);

			grs->motion_handle = g_signal_connect (G_OBJECT (canvas), "motion_notify_event",
				G_CALLBACK (cb_rotate_motion_notify_event), grs);
		}
		return TRUE;
	} else if (event->type == GDK_BUTTON_RELEASE) {
		if (grs->motion_handle != 0) {
			gdk_display_pointer_ungrab (gtk_widget_get_display (GTK_WIDGET (canvas)),
				event->time);
			g_signal_handler_disconnect (canvas, grs->motion_handle);
			grs->motion_handle = 0;
		}
		return TRUE;
	} else
		return FALSE;
}

static void
grs_init (GORotationSel *grs)
{
	GtkWidget *w;

	grs->gui = go_gtk_builder_new ("go-rotation-sel.ui", GETTEXT_PACKAGE, NULL);
	if (grs->gui == NULL)
		return;

	grs->angle = 0;
	grs->line  = NULL;
	grs->text  = NULL;
	grs->text_widget = NULL;
	grs->rotate_canvas = GOC_CANVAS (g_object_new (GOC_TYPE_CANVAS, NULL));
	gtk_container_add (GTK_CONTAINER (go_gtk_builder_get_widget (grs->gui,
		"rotate_canvas_container")),
		GTK_WIDGET (grs->rotate_canvas));
	gtk_widget_show (GTK_WIDGET (grs->rotate_canvas));
	memset (grs->rotate_marks, 0, sizeof (grs->rotate_marks));
	w = go_gtk_builder_get_widget (grs->gui, "rotate_spinner");
	grs->rotate_spinner = GTK_SPIN_BUTTON (w);
	g_signal_connect_swapped (G_OBJECT (w), "value-changed",
		G_CALLBACK (cb_rotate_changed), grs);

	grs->motion_handle = 0;
	g_object_connect (G_OBJECT (grs->rotate_canvas),
		"signal::realize",		G_CALLBACK (cb_rotate_canvas_realize), grs,
		"signal::button_press_event",	G_CALLBACK (cb_rotate_canvas_button), grs,
		"signal::button_release_event", G_CALLBACK (cb_rotate_canvas_button), grs,
		NULL);
	gtk_spin_button_set_value (grs->rotate_spinner, grs->angle);

	w = go_gtk_builder_get_widget (grs->gui, "toplevel");
	gtk_box_pack_start (GTK_BOX (grs), w, TRUE, TRUE, 0);
	gtk_widget_show_all (GTK_WIDGET (grs));
}

static void
grs_finalize (GObject *obj)
{
	GORotationSel *grs = GO_ROTATION_SEL (obj);

	if (grs->gui) {
		g_object_unref (G_OBJECT (grs->gui));
		grs->gui = NULL;
	}

	grs_parent_class->finalize (obj);
}

static void
grs_class_init (GObjectClass *klass)
{
	GObjectClass *gobj_class = (GObjectClass *) klass;
	gobj_class->finalize = grs_finalize;

	grs_parent_class = g_type_class_peek (gtk_hbox_get_type ());
	grs_signals [ROTATION_CHANGED] = g_signal_new ("rotation-changed",
		G_OBJECT_CLASS_TYPE (klass),	G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GORotationSelClass, rotation_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__INT,
		G_TYPE_NONE, 1, G_TYPE_INT);
}

GSF_CLASS (GORotationSel, go_rotation_sel,
	   grs_class_init, grs_init, GTK_TYPE_HBOX)

GtkWidget *
go_rotation_sel_new (void)
{
	return g_object_new (GO_TYPE_ROTATION_SEL, NULL);
}

int
go_rotation_sel_get_rotation (GORotationSel const *grs)
{
	g_return_val_if_fail (GO_IS_ROTATION_SEL (grs), 0);
	return grs->angle;
}

void
go_rotation_sel_set_rotation (GORotationSel *grs, int angle)
{
	g_return_if_fail (GO_IS_ROTATION_SEL (grs));
	if (grs->angle != angle) {
		grs->angle = angle;
		gtk_spin_button_set_value (grs->rotate_spinner, grs->angle);
		g_signal_emit (G_OBJECT (grs),
			grs_signals [ROTATION_CHANGED], 0, grs->angle);
	}
}
