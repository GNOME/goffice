/*
 * goc-canvas.c:
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
#include <glib/gi18n-lib.h>
#include <math.h>

typedef struct {
	 GocItem *invalidated_item;
	 cairo_region_t *invalid_region;
	 gboolean done;
 } GocCanvasPrivate;

/**
 * SECTION: goc-canvas
 * @short_description: The canvas widget
 *
 * The canvas widget used in gnumeric.
 **/


/**
 * GocDirection:
 * @GOC_DIRECTION_LTR: Left to right direction
 * @GOC_DIRECTION_RTL: Right to left direction
 * @GOC_DIRECTION_MAX: First invalid value
 **/

static GObjectClass *parent_klass;

enum {
	GOC_PROP_0 = 0,
	GOC_PROP_WIDTH,
	GOC_PROP_HEIGHT
};

#ifdef GOFFICE_WITH_GTK

static gboolean
goc_canvas_draw (GtkWidget *widget, cairo_t *cr)
{
	double x0, y0, x1, y1;
	double ax0, ay0, ax1, ay1;
	double clip_x1, clip_y1, clip_x2, clip_y2;
	GocCanvas *canvas = GOC_CANVAS (widget);
	GdkEventExpose *event = (GdkEventExpose *) gtk_get_current_event ();
	GocCanvasPrivate *priv = (GocCanvasPrivate *) canvas->priv;
	cairo_rectangle_list_t *l = cairo_copy_clip_rectangle_list (cr);
	int i, x, y;

	if (GOC_IS_ITEM (priv->invalidated_item) && priv->invalid_region) {
		/* evaluate the cairo clipped region and compare with the saved one */
		cairo_region_t *region;
		cairo_rectangle_int_t rect[l->num_rectangles];
	    for (i= 0; i  < l->num_rectangles; i++) {
			rect[i].x = l->rectangles[i].x;
			rect[i].y = l->rectangles[i].y;
			rect[i].width = l->rectangles[i].width;
			rect[i].height = l->rectangles[i].height;
		}
		region = cairo_region_create_rectangles (rect, l->num_rectangles);
		if (cairo_region_equal (priv->invalid_region, region)) {
			cairo_rectangle_list_destroy (l);
			cairo_region_destroy (region);
			/* looks like each time we call gtk_widget_queue_draw*,
			   the draw event is fired twice */
			if (priv->done) {
				priv->invalidated_item = NULL;
				cairo_region_destroy (priv->invalid_region);
				priv->invalid_region = NULL;
			} else {
				goc_item_draw (priv->invalidated_item, cr);
				priv->done = TRUE;
			}
			return TRUE;
		}

		cairo_region_destroy (region);
	}

	x = gtk_adjustment_get_value (gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (canvas)));
	y = gtk_adjustment_get_value (gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (canvas)));
	cairo_translate (cr, -x, -y);
	goc_item_get_bounds (GOC_ITEM (canvas->root),&x0, &y0, &x1, &y1);
	for (i= 0; i  < l->num_rectangles; i++) {
		cairo_save (cr);
		cairo_rectangle (cr, l->rectangles[i].x, l->rectangles[i].y,
		                 l->rectangles[i].width, l->rectangles[i].height);
		cairo_clip (cr);
		clip_x1 = l->rectangles[i].x;
		clip_y1 = l->rectangles[i].y;
		clip_x2 = clip_x1 + l->rectangles[i].width;
		clip_y2 = clip_y1 + l->rectangles[i].height;

		if (canvas->direction == GOC_DIRECTION_RTL) {
			ax1 = (double) (canvas->width - clip_x1) / canvas->pixels_per_unit + canvas->scroll_x1;
			ax0 = (double) (canvas->width - clip_x2) / canvas->pixels_per_unit + canvas->scroll_x1;
		} else {
			ax0 = (double) clip_x1 / canvas->pixels_per_unit + canvas->scroll_x1;
			ax1 = ((double) clip_x1 + event->area.width) / canvas->pixels_per_unit + canvas->scroll_x1;
		}
		ay0 = (double) clip_y1 / canvas->pixels_per_unit + canvas->scroll_y1;
		ay1 = (double) clip_y2 / canvas->pixels_per_unit + canvas->scroll_y1;
		if (x0 <= ax1 && x1 >= ax0 && y0 <= ay1 && y1 >= ay0) {
			canvas->cur_event = (GdkEvent *) event;
			goc_item_draw_region (GOC_ITEM (canvas->root), cr, ax0, ay0, ax1, ay1);
		}
		cairo_restore (cr);
	}
	cairo_rectangle_list_destroy (l);
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
	item = goc_canvas_get_item_at (canvas, x, y);
	if (item) {
		gboolean result;
		canvas->cur_event = (GdkEvent *) event;
		if (event->type == GDK_2BUTTON_PRESS)
			return GOC_ITEM_GET_CLASS (item)->button2_pressed (item, event->button, x, y);
		result = GOC_ITEM_GET_CLASS (item)->button_pressed (item, event->button, x, y);
		canvas->cur_event = NULL;
		return result;
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
		gboolean result;
		canvas->cur_event = (GdkEvent *) event;
		result = GOC_ITEM_GET_CLASS (item)->button_released (item, event->button, x, y);
		canvas->cur_event = NULL;
		return result;
	}
	return FALSE;
}

static gboolean
motion_cb (GocCanvas *canvas, GdkEventMotion *event, G_GNUC_UNUSED gpointer data)
{
	double x, y;
	GocItem *item;
	gboolean result = FALSE;

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
		result = klass->motion (item, x, y);
	}
	canvas->cur_event = NULL;
	return result;
}

static gboolean
key_release_cb (GocCanvas *canvas, GdkEventKey* event, G_GNUC_UNUSED gpointer data)
{
	return (canvas->grabbed_item != NULL)?
		GOC_ITEM_GET_CLASS (canvas->grabbed_item)->key_released (canvas->grabbed_item, event): FALSE;
}

static gboolean
key_press_cb (GocCanvas *canvas, GdkEventKey* event, G_GNUC_UNUSED gpointer data)
{
	return (canvas->grabbed_item != NULL)?
		GOC_ITEM_GET_CLASS (canvas->grabbed_item)->key_pressed (canvas->grabbed_item, event): FALSE;
}

static gboolean
enter_notify_cb (GocCanvas *canvas, GdkEventCrossing* event, G_GNUC_UNUSED gpointer data)
{
	double x, y;
	GocItem *item;
	gboolean result = FALSE;

	if (event->window != gtk_layout_get_bin_window (&canvas->base))
		return TRUE;
	x = (canvas->direction == GOC_DIRECTION_RTL)?
		canvas->scroll_x1 +  (canvas->width - event->x) / canvas->pixels_per_unit:
		canvas->scroll_x1 +  event->x / canvas->pixels_per_unit;
	y = canvas->scroll_y1 + event->y / canvas->pixels_per_unit;
	item = goc_canvas_get_item_at (canvas, x, y);
	if (item) {
		canvas->cur_event = (GdkEvent *) event;
		canvas->last_item = item;
		result = GOC_ITEM_GET_CLASS (item)->enter_notify (item, x, y);
	}
	canvas->cur_event = NULL;
	return result;
}

static gboolean
leave_notify_cb (GocCanvas *canvas, GdkEventCrossing* event, G_GNUC_UNUSED gpointer data)
{
	double x, y;
	gboolean result = FALSE;

	if (event->window != gtk_layout_get_bin_window (&canvas->base))
		return TRUE;
	x = (canvas->direction == GOC_DIRECTION_RTL)?
		canvas->scroll_x1 +  (canvas->width - event->x) / canvas->pixels_per_unit:
		canvas->scroll_x1 +  event->x / canvas->pixels_per_unit;
	y = canvas->scroll_y1 + event->y / canvas->pixels_per_unit;
	if (canvas->last_item) {
		canvas->cur_event = (GdkEvent *) event;
		result = GOC_ITEM_GET_CLASS (canvas->last_item)->leave_notify (canvas->last_item, x, y);
		canvas->last_item = NULL;
		return result;
	}
	canvas->cur_event = NULL;
	return result;
}

static void
size_changed_cb (GocCanvas *canvas, GtkAllocation *alloc, G_GNUC_UNUSED gpointer data)
{
	if (alloc->width != canvas->width) {
		canvas->width = alloc->width;
		g_object_notify (G_OBJECT (canvas), "width");
	}
	if (alloc->height != canvas->height) {
		canvas->height = alloc->height;
		g_object_notify (G_OBJECT (canvas), "height");
	}
}

static void
goc_canvas_realize (GtkWidget *widget)
{
	GocCanvas *canvas = GOC_CANVAS (widget);

	GTK_WIDGET_CLASS (parent_klass)->realize (widget);

	GOC_ITEM_GET_CLASS (canvas->root)->realize (GOC_ITEM (canvas->root));
}

static void
goc_canvas_unrealize (GtkWidget *widget)
{
	GocCanvas *canvas = GOC_CANVAS (widget);

	GOC_ITEM_GET_CLASS (canvas->root)->unrealize (GOC_ITEM (canvas->root));

	GTK_WIDGET_CLASS (parent_klass)->unrealize (widget);
}

#endif

static void
goc_canvas_finalize (GObject *obj)
{
	GocCanvas *canvas = GOC_CANVAS (obj);
	GocCanvasPrivate *priv = (GocCanvasPrivate *) canvas->priv;
	g_object_unref (canvas->root);
	g_free (priv);
	parent_klass->finalize (obj);
}

static void
goc_canvas_dispose (GObject *obj)
{
	GocCanvas *canvas = GOC_CANVAS (obj);
	g_object_run_dispose (G_OBJECT (canvas->root));
	parent_klass->dispose (obj);
}

static void
goc_canvas_get_property (GObject *obj, guint param_id,
			 GValue *value, GParamSpec *pspec)
{
	GocCanvas *canvas = GOC_CANVAS (obj);

	switch (param_id) {
	case GOC_PROP_WIDTH:
		g_value_set_int (value, canvas->width);
		break;

	case GOC_PROP_HEIGHT:
		g_value_set_int (value, canvas->height);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}
}

static void
goc_canvas_class_init (GObjectClass *klass)
{
#ifdef GOFFICE_WITH_GTK
	GtkWidgetClass *widget_klass = (GtkWidgetClass *) klass;
#endif

	parent_klass = g_type_class_peek_parent (klass);
	klass->finalize = goc_canvas_finalize;
	klass->dispose = goc_canvas_dispose;
	klass->get_property = goc_canvas_get_property;

	g_object_class_install_property
		(klass, GOC_PROP_WIDTH,
		 g_param_spec_int ("width", _("Width"),
				   _("The width of the canvas"),
				   G_MININT, G_MAXINT, 0,
				   GSF_PARAM_STATIC | G_PARAM_READABLE));
	g_object_class_install_property
		(klass, GOC_PROP_HEIGHT,
		 g_param_spec_int ("height", _("Height"),
				   _("The height of the canvas"),
				   G_MININT, G_MAXINT, 0,
				   GSF_PARAM_STATIC | G_PARAM_READABLE));

#ifdef GOFFICE_WITH_GTK
#ifdef HAVE_GTK_WIDGET_CLASS_SET_CSS_NAME
	gtk_widget_class_set_css_name (widget_klass, "canvas");
#endif
	widget_klass->draw = goc_canvas_draw;
	widget_klass->realize = goc_canvas_realize;
	widget_klass->unrealize = goc_canvas_unrealize;
#endif
}

static void
goc_canvas_init (GocCanvas *canvas)
{
#ifdef GOFFICE_WITH_GTK
	GtkWidget *w = GTK_WIDGET (canvas);
#endif

	canvas->root = GOC_GROUP (g_object_new (GOC_TYPE_GROUP, NULL));
	canvas->root->base.canvas = canvas;
	canvas->pixels_per_unit = 1.;
	canvas->priv = g_new0 (GocCanvasPrivate, 1);
#ifdef GOFFICE_WITH_GTK
	gtk_widget_add_events (w,
	                       GDK_POINTER_MOTION_MASK |
	                       GDK_BUTTON_MOTION_MASK |
	                       GDK_BUTTON_PRESS_MASK |
	                       GDK_2BUTTON_PRESS |
	                       GDK_BUTTON_RELEASE_MASK |
	                       GDK_KEY_PRESS_MASK |
	                       GDK_KEY_RELEASE_MASK |
	                       GDK_ENTER_NOTIFY_MASK |
	                       GDK_LEAVE_NOTIFY_MASK |
	                       GDK_SMOOTH_SCROLL_MASK |
	                       GDK_SCROLL_MASK
	                       );
	g_signal_connect (G_OBJECT (w), "button-press-event", G_CALLBACK (button_press_cb), NULL);
	g_signal_connect (G_OBJECT (w), "button-release-event", G_CALLBACK (button_release_cb), NULL);
	g_signal_connect (G_OBJECT (w), "motion-notify-event", G_CALLBACK (motion_cb), NULL);
	g_signal_connect (G_OBJECT (w), "key_press_event", (GCallback) key_press_cb, NULL);
	g_signal_connect (G_OBJECT (w), "key_release_event", (GCallback) key_release_cb, NULL);
	g_signal_connect (G_OBJECT (w), "size-allocate", (GCallback) size_changed_cb, NULL);
	g_signal_connect (G_OBJECT (w), "enter-notify-event", G_CALLBACK (enter_notify_cb), NULL);
	g_signal_connect (G_OBJECT (w), "leave-notify-event", G_CALLBACK (leave_notify_cb), NULL);
#endif
}

#ifdef GOFFICE_WITH_GTK
#define CANVAS_BASE_TYPE GTK_TYPE_LAYOUT
#else
#define CANVAS_BASE_TYPE G_TYPE_OBJECT
#endif


GSF_CLASS (GocCanvas, goc_canvas,
	   goc_canvas_class_init, goc_canvas_init,
	   CANVAS_BASE_TYPE)
#if 0
;
#endif

/**
 * goc_canvas_get_root:
 * @canvas: #GocCanvas
 *
 * Returns: (transfer none): the top level item of @canvas, always a #GocGroup.
 **/
GocGroup*
goc_canvas_get_root (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), NULL);
	return canvas->root;
}

/**
 * goc_canvas_get_width:
 * @canvas: #GocCanvas
 *
 * Returns: the width of the widget visible region.
 **/
int
goc_canvas_get_width (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), 0);
	return canvas->width;
}

/**
 * goc_canvas_get_height:
 * @canvas: #GocCanvas
 *
 * Returns: the height of the widget visible region.
 **/
int
goc_canvas_get_height (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), 0);
	return canvas->height;
}

/**
 * goc_canvas_scroll_to:
 * @canvas: #GocCanvas
 * @x: the horizontal position
 * @y: the vertical position
 *
 * Scrolls the canvas so that the origin of the visible region is at (@x,@y).
 * The origin position depends on the canvas direction (see #GocDirection).
 **/
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
#ifdef GOFFICE_WITH_GTK
	gtk_widget_queue_draw_area (GTK_WIDGET (canvas),
					    0, 0, G_MAXINT, G_MAXINT);
#endif
}

/**
 * goc_canvas_get_scroll_position:
 * @canvas: #GocCanvas
 * @x: where to store the horizontal position
 * @y: where to store the vertical position
 *
 * Retrieves the origin of the visible region of the canvas.
 **/
void
goc_canvas_get_scroll_position (GocCanvas *canvas, double *x, double *y)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas));
	if (x)
		*x = canvas->scroll_x1;
	if (y)
		*y = canvas->scroll_y1;
}

/**
 * goc_canvas_set_pixels_per_unit:
 * @canvas: #GocCanvas
 * @pixels_per_unit: the new scale
 *
 * Sets the scale as the number of pixels used for each unit when
 * displaying the canvas.
 **/
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
#ifdef GOFFICE_WITH_GTK
	gtk_widget_queue_draw_area (GTK_WIDGET (canvas),
					    0, 0, G_MAXINT, G_MAXINT);
#endif
}

/**
 * goc_canvas_get_pixels_per_unit:
 * @canvas: #GocCanvas
 *
 * Returns: how many pixels are used for each unit when displaying the canvas.
 **/
double
goc_canvas_get_pixels_per_unit (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), 0);
	return canvas->pixels_per_unit;
}

/**
 * goc_canvas_invalidate:
 * @canvas: #GocCanvas
 * @x0: minimum x coordinate of the invalidated region in canvas coordinates
 * @y0: minimum y coordinate of the invalidated region in canvas coordinates
 * @x1: maximum x coordinate of the invalidated region in canvas coordinates
 * @y1: maximum y coordinate of the invalidated region in canvas coordinates
 *
 * Invalidates a region of the canvas. The canvas will be redrawn only if
 * the invalidated region intersects the visible area.
 **/
void
goc_canvas_invalidate (GocCanvas *canvas, double x0, double y0, double x1, double y1)
{
	if (!goc_canvas_get_realized (canvas))
		return;
#ifdef GOFFICE_WITH_GTK
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
	if (x1 > x0 && y1 > y0) /* invalidate with an extra one pixel wide margin */
		gtk_widget_queue_draw_area (GTK_WIDGET (canvas),
					    (int) floor (x0) - 1, (int) floor (y0) - 1,
					    (int) ceil (x1) - (int) floor (x0) + 2,
		                            (int) ceil (y1) - (int) floor (y0) + 2);
#endif
}


/**
 * goc_canvas_invalidate_region:
 * @canvas: #GocCanvas
 * @item: the item to redraw
 * @region: the region to redraw
 *
 * Invalidates a region of the canvas. Only @item will be redrawn if the next
 * draw event is called with a cairo contest clipped to @region. Used in
 * gnumeric for the walking ants cursor.
 **/
void
goc_canvas_invalidate_region (GocCanvas *canvas, GocItem *item, cairo_region_t *region)
{
	GocCanvasPrivate *priv;

	g_return_if_fail (GOC_IS_CANVAS (canvas));
	g_return_if_fail (item && region);

	priv = (GocCanvasPrivate *) canvas->priv;
	if (priv->invalid_region != NULL)
		cairo_region_destroy (priv->invalid_region);
	priv->invalidated_item = item;
	priv->invalid_region = cairo_region_reference (region);
	priv->done = FALSE;
#ifdef GOFFICE_WITH_GTK
	gtk_widget_queue_draw_region (GTK_WIDGET (canvas), region);
#endif
}

/**
 * goc_canvas_get_realized:
 * @canvas: #GocCanvas
 *
 * Returns: %TRUE if the canvas has been realized as a #GtkWidget.
 **/
gboolean
goc_canvas_get_realized (GocCanvas *canvas)
{
#ifdef GOFFICE_WITH_GTK
	return gtk_widget_get_realized (GTK_WIDGET (canvas));
#else
	return FALSE;
#endif
}


/**
 * goc_canvas_get_item_at:
 * @canvas: #GocCanvas
 * @x: horizontal position
 * @y: vertical position
 *
 * Returns: (transfer none): the #GocItem displayed at (@x,@y) if any.
 **/
GocItem*
goc_canvas_get_item_at (GocCanvas *canvas, double x, double y)
{
	GocItem *result = NULL;
	double d = goc_item_distance (GOC_ITEM (canvas->root), x, y, &result);
	return (d == 0.)? result: NULL;
}

/**
 * goc_canvas_grab_item:
 * @canvas: #GocCanvas
 * @item: #GocItem
 *
 * Grabs #GocItem. All subsequent events will be passed to #GocItem. This
 * function fails if an item is already grabbed.
 **/
void
goc_canvas_grab_item (GocCanvas *canvas, GocItem *item)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas) && canvas->grabbed_item == NULL);
	canvas->grabbed_item = item;
}

/**
 * goc_canvas_ungrab_item:
 * @canvas: #GocCanvas
 *
 * Ungrabs the currently grabbed #GocItem. This function fails
 * if no item is grabbed.
 **/
void
goc_canvas_ungrab_item (GocCanvas *canvas)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas) && canvas->grabbed_item != NULL);
	canvas->grabbed_item = NULL;
}

/**
 * goc_canvas_get_grabbed_item:
 * @canvas: #GocCanvas
 *
 * Returns: (transfer none): The currently grabbed #GocItem.
 **/
GocItem *
goc_canvas_get_grabbed_item (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), NULL);
	return canvas->grabbed_item;
}

/**
 * goc_canvas_set_document:
 * @canvas: #GocCanvas
 * @document: #GODoc
 *
 * Associates the #GODoc with the #GocCanvas. This is needed to use images
 * when filling styled items (see #GocStyledItem).
 **/
void
goc_canvas_set_document (GocCanvas *canvas, GODoc *document)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas));
	canvas->document = document;
}

/**
 * goc_canvas_get_document:
 * @canvas: #GocCanvas
 *
 * Returns: (transfer none): The #GODoc associated with the #GocCanvas.
 **/
GODoc*
goc_canvas_get_document (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), NULL);
	return canvas->document;
}

#ifdef GOFFICE_WITH_GTK
/**
 * goc_canvas_get_cur_event:
 * @canvas: #GocCanvas
 *
 * Returns: (transfer none): The #GdkEvent being processed.
 **/
GdkEvent*
goc_canvas_get_cur_event (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), NULL);
	return canvas->cur_event;
}
#endif

/**
 * goc_canvas_set_direction:
 * @canvas: #GocCanvas
 * @direction: #GocDirection
 *
 * Sets the direction used by the canvas.
 **/
void
goc_canvas_set_direction (GocCanvas *canvas, GocDirection direction)
{
	g_return_if_fail (GOC_IS_CANVAS (canvas) && direction < GOC_DIRECTION_MAX);
	canvas->direction = direction;
	goc_canvas_invalidate (canvas, -G_MAXDOUBLE, -G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE);
}

/**
 * goc_canvas_get_direction:
 * @canvas: #GocCanvas
 *
 * Returns: the current canvas direction.
 **/
GocDirection
goc_canvas_get_direction (GocCanvas *canvas)
{
	g_return_val_if_fail (GOC_IS_CANVAS (canvas), GOC_DIRECTION_MAX);
	return canvas->direction;
}

/**
 * goc_canvas_w2c:
 * @canvas: #GocCanvas
 * @x: the horizontal position as a widget coordinate.
 * @y: the vertical position as a widget coordinate.
 * @x_: where to store the horizontal position as a canvas coordinate.
 * @y_: where to store the vertical position as a canvas coordinate.
 *
 * Retrieves the canvas coordinates given the position in the widget.
 **/
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

/**
 * goc_canvas_c2w:
 * @canvas: #GocCanvas
 * @x: the horizontal position as a canvas coordinate.
 * @y: the vertical position as a canvas coordinate.
 * @x_: where to store the horizontal position as a widget coordinate.
 * @y_: where to store the vertical position as a widget coordinate.
 *
 * Retrieves the position in the widget given the canvas coordinates.
 **/
void
goc_canvas_c2w (GocCanvas *canvas, double x, double y, int *x_, int *y_)
{
	if (x_) {
		if (canvas->direction == GOC_DIRECTION_RTL)
			*x_ = go_fake_round (canvas->width - (x - canvas->scroll_x1) * canvas->pixels_per_unit);
		else
			*x_ = go_fake_round ((x - canvas->scroll_x1) * canvas->pixels_per_unit);
	}
	if (y_)
		*y_ = go_fake_round ((y - canvas->scroll_y1) * canvas->pixels_per_unit);
}

void
goc_canvas_render (GocCanvas *canvas, cairo_t *cr, double x0, double y0, double x1, double y1)
{
	goc_item_draw_region (GOC_ITEM (canvas->root), cr, x0, y0, x1, y1);
}

void
goc_canvas_get_bounds (GocCanvas *canvas, double *x0, double *y0, double *x1, double *y1)
{
	goc_item_get_bounds (GOC_ITEM (canvas->root), x0, y0, x1, y1);
}

/*
 * Item is leaving this canvas.  Get rid of direct pointers.
 */
void
_goc_canvas_remove_item (GocCanvas *canvas, GocItem *item)
{
	GocCanvasPrivate *priv;

	g_return_if_fail (GOC_IS_CANVAS (canvas));
	g_return_if_fail (GOC_IS_ITEM (item));

	priv = (GocCanvasPrivate *) canvas->priv;

	if (canvas->last_item == item)
		canvas->last_item = NULL;
	if (canvas->grabbed_item == item)
		canvas->grabbed_item = NULL;
	if (priv->invalidated_item == item) {
		if (priv->invalid_region != NULL) {
			cairo_region_destroy (priv->invalid_region);
			priv->invalid_region = NULL;
		}
		priv->invalidated_item = NULL;
	}
}
