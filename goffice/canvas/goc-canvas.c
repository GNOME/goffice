/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-canvas.c :
 *
 * Copyright (C) 2008-2009 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
#include <math.h>

#ifndef HAVE_GTK_LAYOUT_GET_BIN_WINDOW
#       define gtk_layout_get_bin_window(x) (x)->bin_window
#endif

static GObjectClass *parent_klass;

static gboolean
expose_cb (GocCanvas *canvas, GdkEventExpose *event, G_GNUC_UNUSED gpointer data)
{
	double x0, y0, x1, y1;
	double ax0, ay0, ax1, ay1;

       if (event->count)
		return TRUE;
	goc_item_get_bounds (GOC_ITEM (canvas->root),&x0, &y0, &x1, &y1);
	if (canvas->direction == GOC_DIRECTION_RTL) {
		ax1 = (double)  (canvas->width - event->area.x) / canvas->pixels_per_unit + canvas->scroll_x1;
		ax0 = (double) (canvas->width - event->area.x - event->area.width) / canvas->pixels_per_unit + canvas->scroll_x1;
	} else {
		ax0 = (double) event->area.x / canvas->pixels_per_unit + canvas->scroll_x1;
		ax1 = ((double) event->area.x + event->area.width) / canvas->pixels_per_unit + canvas->scroll_x1;
	}
	ay0 = (double) event->area.y / canvas->pixels_per_unit + canvas->scroll_y1;
	ay1 = ((double) event->area.y + event->area.height) / canvas->pixels_per_unit + canvas->scroll_y1;
	if (x0 <= ax1 && x1 >= ax0 && y0 <= ay1 && y1 >= ay0) {
		cairo_t *cr = gdk_cairo_create (event->window);
		canvas->cur_event = (GdkEvent *) event;
		goc_item_draw_region (GOC_ITEM (canvas->root), cr, ax0, ay0, ax1, ay1);
		cairo_destroy (cr);
		canvas->cur_event = NULL;
	}
	return TRUE;
}

static gboolean
button_press_cb (GocCanvas *canvas, GdkEventButton *event, G_GNUC_UNUSED gpointer data)
{
	double x, y;
	GocItem *item;

	if (event->window != gtk_layout_get_bin_window (&canvas->base))
		return TRUE;
	x = (canvas->direction == GOC_DIRECTION_RTL)?
		canvas->scroll_x1 +  (canvas->width - event->x) / canvas->pixels_per_unit:
		canvas->scroll_x1 +  event->x / canvas->pixels_per_unit;
	y = canvas->scroll_y1 + event->y / canvas->pixels_per_unit;
	item = goc_canvas_get_item_at (canvas, x, y);;
	if (item) {
		canvas->cur_event = (GdkEvent *) event;
		if (event->type == GDK_2BUTTON_PRESS)
			return GOC_ITEM_GET_CLASS (item)->button2_pressed (item, event->button, x, y);
		return GOC_ITEM_GET_CLASS (item)->button_pressed (item, event->button, x, y);
		canvas->cur_event = NULL;
	}
	return FALSE;
}

static gboolean
button_release_cb (GocCanvas *canvas, GdkEventButton *event, G_GNUC_UNUSED gpointer data)
{
	double x, y;
	GocItem *item;

	if (event->window != gtk_layout_get_bin_window (&canvas->base))
		return TRUE;
	x = (canvas->direction == GOC_DIRECTION_RTL)?
		canvas->scroll_x1 +  (canvas->width - event->x) / canvas->pixels_per_unit:
		canvas->scroll_x1 +  event->x / canvas->pixels_per_unit;
	y = canvas->scroll_y1 + event->y / canvas->pixels_per_unit;
	item = (canvas->grabbed_item != NULL)?
		canvas->grabbed_item:
		goc_canvas_get_item_at (canvas, x, y);
	if (item) {
		canvas->cur_event = (GdkEvent *) event;
		return GOC_ITEM_GET_CLASS (item)->button_released (item, event->button, x, y);
		canvas->cur_event = NULL;
	}
	return FALSE;
}

static gboolean
motion_cb (GocCanvas *canvas, GdkEventMotion *event, G_GNUC_UNUSED gpointer data)
{
	double x, y;
	GocItem *item;

	if (event->window != gtk_layout_get_bin_window (&canvas->base))
		return TRUE;

	x = (canvas->direction == GOC_DIRECTION_RTL)?
		canvas->scroll_x1 +  (canvas->width - event->x) / canvas->pixels_per_unit:
		canvas->scroll_x1 +  event->x / canvas->pixels_per_unit;
	y = canvas->scroll_y1 + event->y / canvas->pixels_per_unit;
	if (canvas->grabbed_item != NULL)
		item = canvas->grabbed_item;
	else
		item = goc_canvas_get_item_at (canvas, x, y);
	canvas->cur_event = (GdkEvent *) event;
	if (canvas->last_item && item != canvas->last_item) {
		GOC_ITEM_GET_CLASS (canvas->last_item)->leave_notify (canvas->last_item, x, y);
		canvas->last_item = NULL;
	}
	if (item) {
		GocItemClass *klass = GOC_ITEM_GET_CLASS (item);
		if (item != canvas->last_item) {
			canvas->last_item = item;
			klass->enter_notify (item, x, y);
		}
		return klass->motion (item, x, y);
	}
	canvas->cur_event = NULL;
	return FALSE;
}

static void
destroy_cb (GocCanvas *canvas, G_GNUC_UNUSED gpointer data)
{
}

static gboolean
key_release_cb (GocCanvas *canvas, GdkEventKey* event, G_GNUC_UNUSED gpointer data)
{
	return FALSE;
}

static gboolean
key_press_cb (GocCanvas *canvas, GdkEventKey* event, G_GNUC_UNUSED gpointer data)
{
	return FALSE;
}

static gboolean
enter_notify_cb (GocCanvas *canvas, GdkEventCrossing* event, G_GNUC_UNUSED gpointer data)
{
	double x, y;
	GocItem *item;

	if (event->window != gtk_layout_get_bin_window (&canvas->base))
		return TRUE;
	x = (canvas->direction == GOC_DIRECTION_RTL)?
		canvas->scroll_x1 +  (canvas->width - event->x) / canvas->pixels_per_unit:
		canvas->scroll_x1 +  event->x / canvas->pixels_per_unit;
	y = canvas->scroll_y1 + event->y / canvas->pixels_per_unit;
	item = goc_canvas_get_item_at (canvas, x, y);;
	if (item) {
		canvas->last_item = item;
		return GOC_ITEM_GET_CLASS (item)->enter_notify (item, x, y);
	}
	return FALSE;
}

static gboolean
leave_notify_cb (GocCanvas *canvas, GdkEventCrossing* event, G_GNUC_UNUSED gpointer data)
{
	double x, y;

	if (event->window != gtk_layout_get_bin_window (&canvas->base))
		return TRUE;
	x = (canvas->direction == GOC_DIRECTION_RTL)?
		canvas->scroll_x1 +  (canvas->width - event->x) / canvas->pixels_per_unit:
		canvas->scroll_x1 +  event->x / canvas->pixels_per_unit;
	y = canvas->scroll_y1 + event->y / canvas->pixels_per_unit;
	if (canvas->last_item) {
		gboolean result = GOC_ITEM_GET_CLASS (canvas->last_item)->leave_notify (canvas->last_item, x, y);
		canvas->last_item = NULL;
		return result;
	}
	return FALSE;
}

static void
size_changed_cb (GocCanvas *canvas, GtkAllocation *alloc, G_GNUC_UNUSED gpointer data)
{
	canvas->width = alloc->width;
	canvas->height = alloc->height;
}

static void
realize_cb (GocCanvas *canvas, G_GNUC_UNUSED gpointer data)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (canvas->root);
	klass->realize (GOC_ITEM (canvas->root));
}

static void
unrealize_cb (GocCanvas *canvas, G_GNUC_UNUSED gpointer data)
{
	GocItemClass *klass = GOC_ITEM_GET_CLASS (canvas->root);
	klass->unrealize (GOC_ITEM (canvas->root));
}

static void
goc_canvas_finalize (GObject *obj)
{
	GocCanvas *canvas = GOC_CANVAS (obj);
	g_object_unref (G_OBJECT (canvas->root));
	(parent_klass->finalize) (obj);
}

static void
goc_canvas_class_init (GObjectClass *klass)
{
	parent_klass = g_type_class_peek_parent (klass);
	klass->finalize = goc_canvas_finalize;
}

static void
goc_canvas_init (GocCanvas *canvas)
{
	GtkWidget *w = GTK_WIDGET (canvas);

	canvas->root = GOC_GROUP (g_object_new (GOC_TYPE_GROUP, NULL));
	canvas->root->base.canvas = canvas;
	canvas->pixels_per_unit = 1.;
	gtk_widget_add_events (w,
			   GDK_POINTER_MOTION_MASK |
			   GDK_BUTTON_MOTION_MASK |
			   GDK_BUTTON_PRESS_MASK |
			   GDK_2BUTTON_PRESS |
			   GDK_BUTTON_RELEASE_MASK |
			   GDK_KEY_PRESS_MASK |
			   GDK_KEY_RELEASE_MASK |
			   GDK_ENTER_NOTIFY_MASK
			   );
	g_signal_connect (G_OBJECT (w), "button-press-event", G_CALLBACK (button_press_cb), NULL);
	g_signal_connect (G_OBJECT (w), "button-release-event", G_CALLBACK (button_release_cb), NULL);
	g_signal_connect (G_OBJECT (w), "motion-notify-event", G_CALLBACK (motion_cb), NULL);
	g_signal_connect (G_OBJECT (w), "destroy", G_CALLBACK (destroy_cb), NULL);
	g_signal_connect (G_OBJECT (w), "expose-event", G_CALLBACK (expose_cb), NULL);
	g_signal_connect (G_OBJECT (w), "key_press_event", (GCallback) key_press_cb, NULL);
	g_signal_connect (G_OBJECT (w), "key_release_event", (GCallback) key_release_cb, NULL);
	g_signal_connect (G_OBJECT (w), "size-allocate", (GCallback) size_changed_cb, NULL);
	g_signal_connect (G_OBJECT (w), "enter-notify-event", G_CALLBACK (enter_notify_cb), NULL);
	g_signal_connect (G_OBJECT (w), "leave-notify-event", G_CALLBACK (leave_notify_cb), NULL);
	g_signal_connect (G_OBJECT (w), "realize", G_CALLBACK (realize_cb), NULL);
	g_signal_connect (G_OBJECT (w), "unrealize", G_CALLBACK (unrealize_cb), NULL);
}

GSF_CLASS (GocCanvas, goc_canvas,
	   goc_canvas_class_init, goc_canvas_init,
	   GTK_TYPE_LAYOUT)

GocGroup*
goc_canvas_get_root (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), NULL);
	return canvas->root;
}

int
goc_canvas_get_width (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), 0);
	return canvas->width;
}

int
goc_canvas_get_height (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), 0);
	return canvas->height;
}

void
goc_canvas_scroll_to (GocCanvas *canvas, double x, double y)
{
	GocItemClass *klass;
	g_return_if_fail (GOC_IS_CANVAS (canvas));
	if (x == canvas->scroll_x1 && canvas->scroll_y1 == y)
		return;
	klass = GOC_ITEM_GET_CLASS (canvas->root);
	canvas->scroll_x1 = x;
	canvas->scroll_y1 = y;
	klass->notify_scrolled (GOC_ITEM (canvas->root));
	gtk_widget_queue_draw_area (GTK_WIDGET (canvas),
					    0, 0, G_MAXINT, G_MAXINT);
}

void
goc_canvas_get_scroll_position (GocCanvas *canvas, double *x, double *y)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas));
	if (x)
		*x = canvas->scroll_x1;
	if (y)
		*y = canvas->scroll_y1;
}

void
goc_canvas_set_pixels_per_unit (GocCanvas *canvas, double pixels_per_unit)
{
	GocItemClass *klass;
	g_return_if_fail (GOC_IS_CANVAS (canvas));
	if (pixels_per_unit == canvas->pixels_per_unit)
		return;
	klass = GOC_ITEM_GET_CLASS (canvas->root);
	canvas->pixels_per_unit = pixels_per_unit;
	klass->notify_scrolled (GOC_ITEM (canvas->root));
	gtk_widget_queue_draw_area (GTK_WIDGET (canvas),
					    0, 0, G_MAXINT, G_MAXINT);
}

double
goc_canvas_get_pixels_per_unit (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), 0);
	return canvas->pixels_per_unit;
}

void
goc_canvas_invalidate (GocCanvas *canvas, double x0, double y0, double x1, double y1)
{
	if (!GTK_WIDGET_REALIZED (canvas))
		return;
	x0 = (x0 - canvas->scroll_x1) * canvas->pixels_per_unit;
	y0 = (y0 - canvas->scroll_y1) * canvas->pixels_per_unit;
	x1 = (x1 - canvas->scroll_x1) * canvas->pixels_per_unit;
	y1 = (y1 - canvas->scroll_y1) * canvas->pixels_per_unit;
	if (x0 < 0.)
		x0 = 0;
	if (y0 < 0)
		y0 = 0;
	if (x1 > canvas->width)
		x1 = canvas->width;
	if (y1 > canvas->height)
		y1 = canvas->height;
	if (canvas->direction == GOC_DIRECTION_RTL) {
		double tmp = x0;
		x0 = canvas->width - x1;
		x1 = canvas->width - tmp;
	}
	if (x1 > x0 && y1 > y0)
		gtk_widget_queue_draw_area (GTK_WIDGET (canvas),
					    (int) floor (x0), (int) floor (y0),
					    (int) ceil (x1), (int) ceil (y1));
}

GocItem*
goc_canvas_get_item_at (GocCanvas *canvas, double x, double y)
{
	GocItem *result = NULL;
	double d = goc_item_distance (GOC_ITEM (canvas->root), x, y, &result);
	return (d == 0.)? result: NULL;
}

void
goc_canvas_grab_item (GocCanvas *canvas, GocItem *item)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas) && canvas->grabbed_item == NULL);
	canvas->grabbed_item = item;
}

void
goc_canvas_ungrab_item (GocCanvas *canvas)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas) && canvas->grabbed_item != NULL);
	canvas->grabbed_item = NULL;
}

GocItem *
goc_canvas_get_grabbed_item (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), NULL);
	return canvas->grabbed_item;
}

void
goc_canvas_set_document (GocCanvas *canvas, GODoc *document)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas));
	canvas->document = document;
}

GODoc*
goc_canvas_get_document (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), NULL);
	return canvas->document;
}

GdkEvent*
goc_canvas_get_cur_event (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), NULL);
	return canvas->cur_event;
}

void
goc_canvas_set_direction (GocCanvas *canvas, GocDirection direction)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas) && direction < GOC_DIRECTION_MAX);
	canvas->direction = direction;
	goc_canvas_invalidate (canvas, -G_MAXDOUBLE, -G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE);
}

GocDirection
goc_canvas_get_direction (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), GOC_DIRECTION_MAX);
	return canvas->direction;
}

void
goc_canvas_w2c (GocCanvas *canvas, int x, int y, double *x_, double *y_)
{
	if (x_) {
		if (canvas->direction == GOC_DIRECTION_RTL)
			*x_ = (double)  (canvas->width - x) / canvas->pixels_per_unit + canvas->scroll_x1;
		else
			*x_ = (double) x / canvas->pixels_per_unit + canvas->scroll_x1;
	}
	if (y_)
		*y_ = (double) y / canvas->pixels_per_unit + canvas->scroll_y1;
}

void
goc_canvas_c2w (GocCanvas *canvas, double x, double y, int *x_, int *y_)
{
	if (x_) {
		if (canvas->direction == GOC_DIRECTION_RTL)
			*x_ = canvas->width - (x - canvas->scroll_x1) * canvas->pixels_per_unit;
		else
			*x_ = (x - canvas->scroll_x1) * canvas->pixels_per_unit;
	}
	if (y_)
		*y_ = (y - canvas->scroll_y1) * canvas->pixels_per_unit;
}
