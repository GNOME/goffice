/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-widget.c :
 *
 * Copyright (C) 2009 Jean Brefort (jean.brefort@normalesup.org)
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
#include <goffice/goffice.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#include <math.h>

/**
 * SECTION:goc-widget
 * @short_description: Widgets.
 *
 * #GocWidget implements widgets embedding in the canvas.
**/

static GocItemClass *widget_parent_class;

enum {
	WIDGET_PROP_0,
	WIDGET_PROP_WIDGET,
	WIDGET_PROP_X,
	WIDGET_PROP_Y,
	WIDGET_PROP_W,
	WIDGET_PROP_H
};


static gboolean
enter_notify_cb (G_GNUC_UNUSED GtkWidget *w, GdkEventCrossing *event, GocWidget *item)
{
	item->base.canvas->cur_event = (GdkEvent *) event;
	return GOC_ITEM_GET_CLASS (item->base.parent)->enter_notify (GOC_ITEM (item->base.parent),
			(event->x + item->x) / item->base.canvas->pixels_per_unit + item->base.canvas->scroll_x1,
			(event->y + item->y) / item->base.canvas->pixels_per_unit + item->base.canvas->scroll_y1);
}

static gboolean
button_press_cb (G_GNUC_UNUSED GtkWidget *w, GdkEventButton *event, GocWidget *item)
{
	item->base.canvas->cur_event = (GdkEvent *) event;
	if (event->button != 3)
		return FALSE;
	return GOC_ITEM_GET_CLASS (item->base.parent)->button_pressed (GOC_ITEM (item->base.parent),
			event->button,
			(event->x + item->x) / item->base.canvas->pixels_per_unit + item->base.canvas->scroll_x1,
			(event->y + item->y) / item->base.canvas->pixels_per_unit + item->base.canvas->scroll_y1);
}

static void
goc_widget_update_bounds (GocItem *item)
{
	GocWidget *widget = GOC_WIDGET (item);
	item->x0 = widget->x;
	item->y0 = widget->y;
	item->x1 = widget->x + widget->w;
	item->y1 = widget->y + widget->h;
}

static void
cb_canvas_changed (GocWidget *item, G_GNUC_UNUSED GParamSpec *pspec,
		   G_GNUC_UNUSED gpointer user)
{
	GtkWidget *parent, *w = item->widget;
	GocItem *gitem = (GocItem *)item;

	if (!w)
		return;

	parent = gtk_widget_get_parent (w);
	if (parent == (GtkWidget *)gitem->canvas)
		return;

	g_object_ref (w);
	if (parent)
		gtk_container_remove (GTK_CONTAINER (parent), w);
	if (gitem->canvas)
		gtk_layout_put (GTK_LAYOUT (gitem->canvas), w,
				item->x, item->y);
	g_object_unref (w);
}

static void
goc_widget_notify_scrolled (GocItem *item)
{
	GocGroup const *parent;
	double x0, y0, x1, y1;
	GocWidget *widget = GOC_WIDGET (item);

	parent = item->parent;
	if (!parent)
		return;

	if (!item->cached_bounds) {
		goc_widget_update_bounds (GOC_ITEM (item)); /* don't care about const */
		item->cached_bounds = TRUE;
	}
	x0 = item->x0;
	y0 = item->y0;
	x1 = item->x1;
	y1 = item->y1;
	goc_group_adjust_bounds (parent, &x0, &y0, &x1, &y1);
	x0 = (x0 - item->canvas->scroll_x1) * item->canvas->pixels_per_unit;
	y0 = (y0 - item->canvas->scroll_y1) * item->canvas->pixels_per_unit;
	x1 = (x1 - item->canvas->scroll_x1) * item->canvas->pixels_per_unit;
	y1 = (y1 - item->canvas->scroll_y1) * item->canvas->pixels_per_unit;
	gtk_widget_set_size_request (widget->widget, x1 - x0, y1 - y0);
	/* ensure we don't wrap throught he infinite */
	if (x0 < G_MININT)
		x0 = G_MININT;
	else if (x1 > G_MAXINT)
		x0 -= x1 - G_MAXINT;
	if (y0 < G_MININT)
		y0 = G_MININT;
	else if (y1 > G_MAXINT)
		y0 -= y1 - G_MAXINT;
	gtk_layout_move (GTK_LAYOUT (item->canvas), widget->widget, x0, y0);
}

static void
goc_widget_set_widget (GocWidget *item, GtkWidget *widget)
{
	if (widget == item->widget)
		return;

	if (item->widget) {
		GtkWidget *parent = gtk_widget_get_parent (item->widget);

		g_signal_handlers_disconnect_by_func
			(item->widget, G_CALLBACK (enter_notify_cb), item);
		g_signal_handlers_disconnect_by_func
			(item->widget, G_CALLBACK (button_press_cb), item);

		if (parent)
			gtk_container_remove (GTK_CONTAINER (parent),
					      item->widget);

		g_object_unref (item->widget);
	}

	item->widget = widget;

	if (widget) {
		g_object_ref (item->widget);
		gtk_widget_show (widget);
		if (GOC_ITEM (item)->canvas)
			gtk_layout_put (GTK_LAYOUT (GOC_ITEM (item)->canvas),
					widget, item->x, item->y);
		goc_widget_notify_scrolled (GOC_ITEM (item));
		/* we need to propagate some signals to the parent item */
		g_signal_connect (widget, "enter-notify-event",
				  G_CALLBACK (enter_notify_cb), item);
		g_signal_connect (widget, "button-press-event",
				  G_CALLBACK (button_press_cb), item);
	}
}

static void
goc_widget_set_property (GObject *obj, guint param_id,
			 GValue const *value, GParamSpec *pspec)
{
	GocWidget *item = GOC_WIDGET (obj);

	switch (param_id) {
	case WIDGET_PROP_WIDGET: {
		GtkWidget *widget = GTK_WIDGET (g_value_get_object (value));
		goc_widget_set_widget (item, widget);
		return;
	}
	case WIDGET_PROP_X:
		item->x = g_value_get_double (value);
		break;

	case WIDGET_PROP_Y:
		item->y = g_value_get_double (value);
		break;

	case WIDGET_PROP_W:
		item->w = g_value_get_double (value);
		break;

	case WIDGET_PROP_H:
		item->h = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}
	if (item->widget) {
		goc_item_bounds_changed (GOC_ITEM (item));
		goc_widget_notify_scrolled (GOC_ITEM (item));
	}

}

static void
goc_widget_get_property (GObject *obj, guint param_id,
			 GValue *value, GParamSpec *pspec)
{
	GocWidget *item = GOC_WIDGET (obj);

	switch (param_id) {
	case WIDGET_PROP_WIDGET:
		g_value_set_object (value, item->widget);
		break;

	case WIDGET_PROP_X:
		g_value_set_double (value, item->x);
		break;

	case WIDGET_PROP_Y:
		g_value_set_double (value, item->y);
		break;

	case WIDGET_PROP_W:
		g_value_set_double (value, item->w);
		break;

	case WIDGET_PROP_H:
		g_value_set_double (value, item->h);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static double
goc_widget_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocWidget *widget = GOC_WIDGET (item);
	double dx, dy;
	if (x < widget->x) {
		dx = widget->x - x;
	} else if (x < widget->x + widget->w) {
		dx = 0;
	} else {
		dx = x - widget->x - widget->w;
	}
	if (y < widget->y) {
		dy = widget->y - y;
	} else if (x < widget->y + widget->h) {
		dy = 0;
	} else {
		dy = y - widget->y - widget->h;
	}
	*near_item = item;
	return hypot (dx, dy);
}

static void
goc_widget_draw (GocItem const *item, cairo_t *cr)
{
	GocWidget *widget = GOC_WIDGET (item);
	if (item->canvas->cur_event == NULL)
		return; /* we do not draw for off-screen rendering, at least until this is implemented in gtk+ */
	 gtk_container_propagate_expose (GTK_CONTAINER (item->canvas),
					widget->widget,
					(GdkEventExpose *) goc_canvas_get_cur_event (item->canvas));
}

static void
goc_widget_dispose (GObject *object)
{
	GocWidget *item = GOC_WIDGET (object);
	goc_widget_set_widget (item, NULL);
	((GObjectClass *)widget_parent_class)->dispose (object);
}

static void
goc_widget_init (GocWidget *item)
{
	item->x = item->y = -G_MAXINT; /* so that it is not visible until the position is set */
	g_signal_connect (item, "notify::canvas",
			  G_CALLBACK (cb_canvas_changed),
			  NULL);
}

static void
goc_widget_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;

	widget_parent_class = g_type_class_peek_parent (item_klass);

	obj_klass->get_property = goc_widget_get_property;
	obj_klass->set_property = goc_widget_set_property;
	obj_klass->dispose = goc_widget_dispose;

	g_object_class_install_property (obj_klass, WIDGET_PROP_WIDGET,
		g_param_spec_object ("widget",
			_("Widget"),
			_("A pointer to the embedded widget"),
			GTK_TYPE_WIDGET,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, WIDGET_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The widget left position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, WIDGET_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The widget top position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	/* Setting max size to 2048 which should be enough for all uses */
	g_object_class_install_property (obj_klass, WIDGET_PROP_W,
		g_param_spec_double ("width",
			_("Width"),
			_("The widget width"),
			0., 2048., 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, WIDGET_PROP_H,
		g_param_spec_double ("height",
			_("Height"),
			_("The widget height"),
			0., 2048., 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->distance = goc_widget_distance;
	item_klass->draw = goc_widget_draw;
	item_klass->update_bounds = goc_widget_update_bounds;
	item_klass->notify_scrolled = goc_widget_notify_scrolled;
}

GSF_CLASS (GocWidget, goc_widget,
	   goc_widget_class_init, goc_widget_init,
	   GOC_TYPE_ITEM)

void
goc_widget_set_bounds (GocWidget *widget, double left, double top, double width, double height)
{
	GocItem *item = GOC_ITEM (widget);
	goc_item_invalidate (item);
	widget->x = left;
	widget->y = top;
	widget->w = width;
	widget->h = height;
	goc_item_bounds_changed (item);
	if (widget->widget)
		goc_widget_notify_scrolled (item);
	goc_item_invalidate (item);
}

