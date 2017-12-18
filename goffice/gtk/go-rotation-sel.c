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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>

struct _GORotationSel {
	GtkGrid		 grid;
	GtkBuilder	*gui;
	int		 angle;

	GtkSpinButton	*rotate_spinner;
	GocCanvas       *rotate_canvas;
	GocItem		*rotate_marks[25];
	GocItem		*line;
	GtkWidget       *text_widget;
	GocItem		*text;
	int		 rot_width0, rot_width1;
	gulong		 motion_handle;
	gboolean full;

	// Styling
	GOColor          tick_color[2][2];         // [selected][major]
	GOColor          tick_outline_color[2][2]; // [selected][major]
	double           tick_outline_width[2][2]; // [selected][major]
	double           tick_size[2][2];          // [selected][major]
	GOColor          line_color;
	double           line_width;
};

typedef struct {
	GtkGridClass parent_class;
	void (* rotation_changed) (GORotationSel *grs, int angle);
} GORotationSelClass;

enum {
	ROTATION_CHANGED,
	LAST_SIGNAL
};

static guint grs_signals[LAST_SIGNAL] = { 0 };
static GObjectClass *grs_parent_class;

static void
grs_reload_style (GORotationSel *grs)
{
	int i;
#if GTK_CHECK_VERSION(3,20,0)
	GtkStyleContext *ctxt_canvas =
		gtk_widget_get_style_context (GTK_WIDGET (grs->rotate_canvas));
#endif

	for (i = 0; i < 4; i++) {
		gboolean qSelected = (i & 1);
		gboolean qMajor = ((i >> 1) & 1);
#if GTK_CHECK_VERSION(3,20,0)
		GtkStyleContext *ctxt = go_style_context_from_selector
			(ctxt_canvas, qSelected ? "ticks:selected" : "ticks");
		GdkRGBA *c, *oc;
		int ow, mw;

		gtk_style_context_add_class (ctxt,
					     qMajor ? "major" : "minor");
		gtk_style_context_get (ctxt,
				       gtk_style_context_get_state (ctxt),
				       "color", &c,
				       "outline-color", &oc,
				       "outline-width", &ow,
				       "min-width", &mw,
				       NULL);
		grs->tick_color[qSelected][qMajor] = GO_COLOR_FROM_GDK_RGBA (*c);
		grs->tick_outline_color[qSelected][qMajor] = GO_COLOR_FROM_GDK_RGBA (*oc);
		grs->tick_outline_width[qSelected][qMajor] = ow;
		// We highjack min-width to mean 2*radius
		grs->tick_size[qSelected][qMajor] = mw / 2.0;
		gdk_rgba_free (c);
		gdk_rgba_free (oc);
		g_object_unref (ctxt);
#else
		grs->tick_color[qSelected][qMajor] = GO_COLOR_BLACK;
		grs->tick_outline_color[qSelected][qMajor] = GO_COLOR_BLACK;
		grs->tick_outline_width[qSelected][qMajor] = 1;
		grs->tick_size[qSelected][qMajor] = (qMajor ? 4 : 3);
#endif
	}

#if GTK_CHECK_VERSION(3,20,0)
	{
		GtkStyleContext *ctxt = go_style_context_from_selector
			(ctxt_canvas, "line");
		GdkRGBA *c;
		int ow;
		gtk_style_context_get (ctxt,
				       gtk_style_context_get_state (ctxt),
				       "color", &c,
				       "outline-width", &ow,
				       NULL);
		grs->line_color = GO_COLOR_FROM_GDK_RGBA (*c);
		grs->line_width = ow;
		gdk_rgba_free (c);
		g_object_unref (ctxt);
	}
#else
	grs->line_color = GO_COLOR_BLACK;
	grs->line_width = 2;
#endif
}


static void
cb_rotate_changed (GORotationSel *grs)
{
	int i, maxi = grs->full? 23: 12;
	double x0 = grs->full? 100.: 15.;
	int angle;

	go_rotation_sel_set_rotation (grs,
		gtk_spin_button_get_value_as_int (grs->rotate_spinner) % 360);
	angle = (grs->angle < -90)? grs->angle + 360: grs->angle;

	for (i = 0 ; i <= maxi ; i++) {
		int mark_angle = (i - 6) * 15;
		gboolean qSelected = (angle == mark_angle);
		gboolean qMajor = (i % 3 == 0);
		GocItem *item = grs->rotate_marks[i];

		if (item != NULL) {
			GOStyledObject *so = GO_STYLED_OBJECT (item);
			GOStyle *style = go_styled_object_get_style (so);
			GOColor color = grs->tick_color[qSelected][qMajor];
			GOColor outline_color = grs->tick_outline_color[qSelected][qMajor];
			int outline_width = grs->tick_outline_width[qSelected][qMajor];
			double radius = grs->tick_size[qSelected][qMajor];
			if (style->fill.pattern.back != color ||
			    style->line.color != outline_color ||
			    style->line.width != outline_width) {
				style->line.color = outline_color;
				style->line.width = outline_width;
				style->fill.pattern.back = color;
				go_styled_object_style_changed (so);
			}
			if (GOC_CIRCLE (item)->radius != radius) {
				g_object_set (item, "radius", radius, NULL);
				goc_item_invalidate (item);
			}
		}
	}

	if (grs->line != NULL) {
		double c = go_cospi (grs->angle / 180.);
		double s = go_sinpi (grs->angle / 180.);
		goc_item_set (grs->line,
		              "x0",  x0 + c * grs->rot_width0,
		              "y0", 100 - s * grs->rot_width0,
		              "x1",  x0 + c * grs->rot_width1,
		              "y1", 100 - s * grs->rot_width1,
		              NULL);
	}

	if (grs->text) {
		double rad = -grs->angle * M_PI / 180.;
		if (rad < 0)
			rad += 2 * M_PI;
		goc_item_set (grs->text, "x", x0, "y", 100.,
		              "rotation", rad, NULL);
	}
}

static void
cb_rotate_canvas_realize (GocCanvas *canvas, GORotationSel *grs)
{
	GocGroup  *group = goc_canvas_get_root (canvas);
	int i, maxint = grs->full? 23: 12;
	GOStyle *go_style;
	double x0 = grs->full? 100.: 15.;
	double mov = 0;

	for (i = 0 ; i <= maxint; i++) {
		int mark_angle = (i - 6) * 15;
		double x = x0 + go_cospi (mark_angle / 180.0) * 80.;
		double y = 100 - go_sinpi (mark_angle / 180.0) * 80.;
		gboolean qMajor = (i % 3 == 0);
		double size = grs->tick_size[FALSE][qMajor];
		GocItem *item = goc_item_new (group,
			GOC_TYPE_CIRCLE,
			"x", x,	"y", y,
			"radius", size,
			NULL);
		grs->rotate_marks[i] = item;
	}
	grs->line = goc_item_new (group, GOC_TYPE_LINE, NULL);
	go_style = go_styled_object_get_style (GO_STYLED_OBJECT (grs->line));
	go_style->line.width = grs->line_width;
	go_style->line.color = grs->line_color;

	{
		double x, y, w, h;
		PangoContext *ctxt = gtk_widget_get_pango_context (GTK_WIDGET (canvas));
		PangoFontDescription const *desc = pango_context_get_font_description (ctxt);
		PangoAttrList *attrs = pango_attr_list_new ();
		PangoAttribute *attr = pango_attr_size_new (1.3 * pango_font_description_get_size (desc));
		attr->start_index = 0;
		attr->end_index = -1;
		pango_attr_list_insert (attrs, attr);
#ifdef DEBUG_ROTATION
		attr = pango_attr_background_new (0xffff, 0, 0);
		attr->start_index = 0;
		attr->end_index = -1;
		pango_attr_list_insert (attrs, attr);
#endif
		grs->text = goc_item_new (group, GOC_TYPE_TEXT,
			"text", _("Text"), "attributes", attrs,
		         "anchor", GO_ANCHOR_W, "x", x0, "y", 100., NULL);
		goc_item_get_bounds (grs->text, &x, &y, &w, &h);
		pango_attr_list_unref (attrs);

		grs->rot_width0  = w - x;
	}

	for (i = 0; i < 4; i++) {
		gboolean qSelected = (i & 1);
		gboolean qMajor = ((i >> 1) & 1);
		double ov = grs->tick_outline_width[qSelected][qMajor];
		if (ov > mov)
			mov = ov;
	}
	grs->rot_width1 = 73 - mov;

	cb_rotate_changed (grs);
}

static void
set_rot_from_point (GORotationSel *grs, double x, double y)
{
	double degrees;
	x -= grs->full? 100.: 15.;	if (!grs->full && x < 0.) x = 0.;
	y -= 100.;

	degrees = go_atan2pi (-y, x) * 180;

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
			gdk_device_grab (gdk_event_get_device ((GdkEvent *) event),
			                 gtk_layout_get_bin_window (&canvas->base),
			                 GDK_OWNERSHIP_APPLICATION, FALSE,
					 GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
					 NULL, event->time);

			grs->motion_handle = g_signal_connect (G_OBJECT (canvas), "motion_notify_event",
				G_CALLBACK (cb_rotate_motion_notify_event), grs);
		}
		return TRUE;
	} else if (event->type == GDK_BUTTON_RELEASE) {
		if (grs->motion_handle != 0) {
			gdk_device_ungrab (gdk_event_get_device ((GdkEvent *) event),
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

	grs->gui = go_gtk_builder_load_internal ("res:go:gtk/go-rotation-sel.ui", GETTEXT_PACKAGE, NULL);
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
	gtk_grid_attach (GTK_GRID (grs), w, 0, 0, 1, 1);

	_go_gtk_widget_add_css_provider (GTK_WIDGET (grs->rotate_canvas));

	grs_reload_style (grs);

	gtk_widget_show_all (GTK_WIDGET (grs));
}

static void
grs_finalize (GObject *obj)
{
	GORotationSel *grs = GO_ROTATION_SEL (obj);

	if (grs->gui) {
		g_object_unref (grs->gui);
		grs->gui = NULL;
	}

	grs_parent_class->finalize (obj);
}

static void
grs_class_init (GObjectClass *klass)
{
	klass->finalize = grs_finalize;

	grs_parent_class = g_type_class_peek (gtk_box_get_type ());
	grs_signals [ROTATION_CHANGED] = g_signal_new ("rotation-changed",
		G_OBJECT_CLASS_TYPE (klass),	G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GORotationSelClass, rotation_changed),
		NULL, NULL,
		g_cclosure_marshal_VOID__INT,
		G_TYPE_NONE, 1, G_TYPE_INT);

#ifdef HAVE_GTK_WIDGET_CLASS_SET_CSS_NAME
	gtk_widget_class_set_css_name ((GtkWidgetClass *)klass, "rotationselector");
#endif
}

GSF_CLASS (GORotationSel, go_rotation_sel,
	   grs_class_init, grs_init, GTK_TYPE_GRID)

GtkWidget *
go_rotation_sel_new (void)
{
	return g_object_new (GO_TYPE_ROTATION_SEL, NULL);
}

GtkWidget *
go_rotation_sel_new_full (void)
{
	GORotationSel *grs = g_object_new (GO_TYPE_ROTATION_SEL, NULL);
	GtkAdjustment *adj = gtk_spin_button_get_adjustment (grs->rotate_spinner);
	gtk_adjustment_set_lower (adj, -180.);
	gtk_adjustment_set_upper (adj, 180.);
	grs->full = TRUE;
	g_object_set (gtk_builder_get_object (grs->gui, "rotate_canvas_container"), "width-request", 200, NULL);
	return (GtkWidget *) grs;
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
