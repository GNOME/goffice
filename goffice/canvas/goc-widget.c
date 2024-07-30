/*
 * goc-widget.c :
 *
 * Copyright (C) 2009 Jean Brefort (jean.brefort@normalesup.org)
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

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>

#include <math.h>

/******************************************************************************
 * GocOffscreenBox: code mostly copied from gtk+/tests/gtkoffscreenbox.[c,h]
 ******************************************************************************/

#define GOC_TYPE_OFFSCREEN_BOX              (goc_offscreen_box_get_type ())
#define GOC_OFFSCREEN_BOX(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), GOC_TYPE_OFFSCREEN_BOX, GocOffscreenBox))
#define GOC_OFFSCREEN_BOX_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GOC_TYPE_OFFSCREEN_BOX, GocOffscreenBoxClass))
#define GOC_IS_OFFSCREEN_BOX(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GOC_TYPE_OFFSCREEN_BOX))
#define GOC_IS_OFFSCREEN_BOX_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GOC_TYPE_OFFSCREEN_BOX))
#define GOC_OFFSCREEN_BOX_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GOC_TYPE_OFFSCREEN_BOX, GocOffscreenBoxClass))

typedef struct _GocOffscreenBox	  GocOffscreenBox;
typedef GtkBinClass  GocOffscreenBoxClass;

static GObjectClass *goc_offscreen_box_parent_class;

struct _GocOffscreenBox
{
	GtkBin base;
	GtkWidget *child;
	GdkWindow *offscreen_window;
	gdouble angle, scale;
};

struct _GocOffscreenBoxClass
{
	GtkBinClass parent_class;
};

static GType	   goc_offscreen_box_get_type  (void);

static void
to_child (GocOffscreenBox *offscreen_box,
          double widget_x, double widget_y,
          double *x_out, double *y_out)
{
/*	GtkAllocation child_area;*/
	double x, y/*, xr, yr*/;
/*	double cos_angle, sin_angle;*/

	x = widget_x;
	y = widget_y;

#if 0
	/* rotation not supported for now */
	if (offscreen_box->child && gtk_widget_get_visible (offscreen_box->child)) {
		gtk_widget_get_allocation (offscreen_box->child, &child_area);
		y -= child_area.height;
	}

	gtk_widget_get_allocation (offscreen_box->child2, &child_area);

	x -= child_area.width / 2;
	y -= child_area.height / 2;

	cos_angle = cos (-offscreen_box->angle);
	sin_angle = sin (-offscreen_box->angle);

	xr = x * cos_angle - y * sin_angle;
	yr = x * sin_angle + y * cos_angle;
	x = xr;
	y = yr;

	x += child_area.width / 2;
	y += child_area.height / 2;
#endif

	*x_out = x / offscreen_box->scale;
	*y_out = y / offscreen_box->scale;
}

static void
to_parent (GocOffscreenBox *offscreen_box,
	     double offscreen_x, double offscreen_y,
	     double *x_out, double *y_out)
{
/*  GtkAllocation child_area;*/
	double x, y/*, xr, yr*/;
/*  double cos_angle, sin_angle;*/

/*  gtk_widget_get_allocation (offscreen_box->child2, &child_area);*/

	x = offscreen_x * offscreen_box->scale;
	y = offscreen_y * offscreen_box->scale;

/*  x -= child_area.width / 2;
  y -= child_area.height / 2;

  cos_angle = cos (offscreen_box->angle);
  sin_angle = sin (offscreen_box->angle);

  xr = x * cos_angle - y * sin_angle;
  yr = x * sin_angle + y * cos_angle;
  x = xr;
  y = yr;

  x += child_area.width / 2;
  y += child_area.height / 2;

  if (offscreen_box->child1 && gtk_widget_get_visible (offscreen_box->child1))
    {
      gtk_widget_get_allocation (offscreen_box->child1, &child_area);
      y += child_area.height;
    }*/

	*x_out = x;
	*y_out = y;
}

static void
offscreen_window_to_parent (GdkWindow       *offscreen_window,
			    double           offscreen_x,
			    double           offscreen_y,
			    double          *parent_x,
			    double          *parent_y,
			    GocOffscreenBox *offscreen_box)
{
	to_parent (offscreen_box, offscreen_x, offscreen_y, parent_x, parent_y);
}

static void
offscreen_window_from_parent (GdkWindow       *window,
			      double           parent_x,
			      double           parent_y,
			      double          *offscreen_x,
			      double          *offscreen_y,
			      GocOffscreenBox *offscreen_box)
{
	to_child (offscreen_box, parent_x, parent_y, offscreen_x, offscreen_y);
}

static GdkWindow *
pick_offscreen_child (GdkWindow *offscreen_window,
		      double widget_x, double widget_y,
		      GocOffscreenBox *offscreen_box)
{
	GtkAllocation child_area;
	double x, y;

	/* Check for child */
	if (offscreen_box->child &&
	    gtk_widget_get_visible (offscreen_box->child)) {
		to_child (offscreen_box, widget_x, widget_y, &x, &y);

		gtk_widget_get_allocation (offscreen_box->child, &child_area);

		if (x >= 0 && x < child_area.width &&
		    y >= 0 && y < child_area.height)
			return offscreen_box->offscreen_window;
	}

	return NULL;
}

static void
goc_offscreen_box_realize (GtkWidget *widget)
{
	GocOffscreenBox *offscreen_box = GOC_OFFSCREEN_BOX (widget);
	GtkAllocation allocation, child_area;
	GtkStyleContext *context;
	GdkWindow *window;
	GdkWindowAttr attributes;
	gint attributes_mask;

	gtk_widget_set_realized (widget, TRUE);

	gtk_widget_get_allocation (widget, &allocation);

	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.event_mask = gtk_widget_get_events (widget)
		| GDK_EXPOSURE_MASK
		| GDK_POINTER_MOTION_MASK
		| GDK_BUTTON_PRESS_MASK
		| GDK_BUTTON_RELEASE_MASK
		| GDK_SCROLL_MASK
		| GDK_ENTER_NOTIFY_MASK
		| GDK_LEAVE_NOTIFY_MASK;

	attributes.visual = gtk_widget_get_visual (widget);
	attributes.wclass = GDK_INPUT_OUTPUT;

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

	window = gdk_window_new (gtk_widget_get_parent_window (widget),
	                         &attributes, attributes_mask);
	gtk_widget_set_window (widget, window);
	if (gdk_screen_is_composited (gdk_window_get_screen (window)))
		gdk_window_set_composited (window, TRUE);
	gdk_window_set_user_data (window, widget);

	g_signal_connect (window, "pick-embedded-child",
	                  G_CALLBACK (pick_offscreen_child), offscreen_box);

	attributes.window_type = GDK_WINDOW_OFFSCREEN;

	attributes.x = attributes.y = 0;
	if (offscreen_box->child &&
	    gtk_widget_get_visible (offscreen_box->child)) {
		gtk_widget_get_allocation (offscreen_box->child, &child_area);

		attributes.width = child_area.width;
		attributes.height = child_area.height;
	}
	offscreen_box->offscreen_window = gdk_window_new (gdk_screen_get_root_window (gtk_widget_get_screen (widget)),
	                                                  &attributes, attributes_mask);
	gdk_window_set_user_data (offscreen_box->offscreen_window, widget);
	if (offscreen_box->child)
		gtk_widget_set_parent_window (offscreen_box->child,
		                              offscreen_box->offscreen_window);

	gdk_offscreen_window_set_embedder (offscreen_box->offscreen_window,
	                                   window);

	g_signal_connect (offscreen_box->offscreen_window, "to-embedder",
	                  G_CALLBACK (offscreen_window_to_parent), offscreen_box);
	g_signal_connect (offscreen_box->offscreen_window, "from-embedder",
	                  G_CALLBACK (offscreen_window_from_parent), offscreen_box);

	context = gtk_widget_get_style_context (widget);
	gtk_style_context_set_background (context, window);
	gtk_style_context_set_background (context, offscreen_box->offscreen_window);

	gdk_window_show (offscreen_box->offscreen_window);
}

static void
goc_offscreen_box_unrealize (GtkWidget *widget)
{
	GocOffscreenBox *offscreen_box = GOC_OFFSCREEN_BOX (widget);

	gdk_offscreen_window_set_embedder (offscreen_box->offscreen_window, NULL);

	if (offscreen_box->child) {
		gtk_widget_unrealize (offscreen_box->child);
		gtk_widget_set_parent_window (offscreen_box->child, NULL);
	}

	gdk_window_set_user_data (offscreen_box->offscreen_window, NULL);
	gdk_window_destroy (offscreen_box->offscreen_window);
	offscreen_box->offscreen_window = NULL;

	GTK_WIDGET_CLASS (goc_offscreen_box_parent_class)->unrealize (widget);
}

static void
goc_offscreen_box_size_request (GtkWidget      *widget,
				GtkRequisition *requisition)
{
	GocOffscreenBox *offscreen_box = GOC_OFFSCREEN_BOX (widget);

	requisition->width = requisition->height = 0;

	if (offscreen_box->child
	    && gtk_widget_get_visible (offscreen_box->child)) {
		GtkRequisition child_requisition;

		gtk_widget_get_preferred_size (offscreen_box->child,
		                               &child_requisition, NULL);

		requisition->width = offscreen_box->scale * child_requisition.width;
		requisition->height = offscreen_box->scale * child_requisition.height;
	}

	/* FIXME: should we take rotation into account? Original code does not */
}

static void
goc_offscreen_box_get_preferred_width (GtkWidget *widget,
                                       gint      *minimum,
                                       gint      *natural)
{
	GtkRequisition requisition;

	goc_offscreen_box_size_request (widget, &requisition);

	*minimum = *natural = requisition.width;
}

static void
goc_offscreen_box_get_preferred_height (GtkWidget *widget,
                                        gint      *minimum,
                                        gint      *natural)
{
	GtkRequisition requisition;

	goc_offscreen_box_size_request (widget, &requisition);

	*minimum = *natural = requisition.height;
}

static void
goc_offscreen_box_size_allocate (GtkWidget     *widget,
				 GtkAllocation *allocation)
{
	GocOffscreenBox *offscreen_box;

	offscreen_box = GOC_OFFSCREEN_BOX (widget);

	gtk_widget_set_allocation (widget, allocation);

	if (gtk_widget_get_realized (widget)) {
		/* See https://bugzilla.gnome.org/show_bug.cgi?id=698758 */
		guint h = MIN (allocation->height, 0x7fff);
		guint w = MIN (allocation->width, 0x7fff);
		gdk_window_move_resize (gtk_widget_get_window (widget),
		                        allocation->x,
		                        allocation->y,
		                        w, h);
	}

	if (offscreen_box->child
	    && gtk_widget_get_visible (offscreen_box->child)) {
		GtkRequisition child_requisition;
		GtkAllocation child_allocation;

		gtk_widget_get_preferred_size (offscreen_box->child,
					       &child_requisition, NULL);
		child_allocation.x = 0;
		child_allocation.y = 0;
		child_allocation.width = child_requisition.width;
		child_allocation.height = child_requisition.height;

		if (gtk_widget_get_realized (widget))
			gdk_window_move_resize (offscreen_box->offscreen_window,
						child_allocation.x,
						child_allocation.y,
						child_allocation.width,
						child_allocation.height);

		gtk_widget_size_allocate (offscreen_box->child, &child_allocation);
	}
}

static gboolean
goc_offscreen_box_damage (GtkWidget      *widget,
                          GdkEventExpose *event)
{
	gdk_window_invalidate_rect (gtk_widget_get_window (widget),
	                            NULL, FALSE);

	return TRUE;
}

static void
goc_offscreen_box_add (GtkContainer *container,
		       GtkWidget    *widget)
{
	GocOffscreenBox *offscreen_box = GOC_OFFSCREEN_BOX (container);

	if (!offscreen_box->child) {
		((GtkContainerClass *) goc_offscreen_box_parent_class)->add (container, widget);
		gtk_widget_set_parent_window (widget, offscreen_box->offscreen_window);
		offscreen_box->child = widget;
	}
}

static void
goc_offscreen_box_remove (GtkContainer *container,
			  GtkWidget    *widget)
{
	GocOffscreenBox *offscreen_box = GOC_OFFSCREEN_BOX (container);
	gboolean was_visible;

	was_visible = gtk_widget_get_visible (widget);

	if (offscreen_box->child == widget) {
		((GtkContainerClass *) goc_offscreen_box_parent_class)->remove (container, widget);

		offscreen_box->child = NULL;

		if (was_visible &&
		    gtk_widget_get_visible (GTK_WIDGET (container)))
			gtk_widget_queue_resize (GTK_WIDGET (container));
	}
}

static void
goc_offscreen_box_class_init (GocOffscreenBoxClass *klass)
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

	goc_offscreen_box_parent_class = g_type_class_peek_parent (klass);

	widget_class->realize = goc_offscreen_box_realize;
	widget_class->unrealize = goc_offscreen_box_unrealize;
	widget_class->get_preferred_width = goc_offscreen_box_get_preferred_width;
	widget_class->get_preferred_height = goc_offscreen_box_get_preferred_height;
	widget_class->size_allocate = goc_offscreen_box_size_allocate;

	g_signal_override_class_closure (g_signal_lookup ("damage-event", GTK_TYPE_WIDGET),
		           GOC_TYPE_OFFSCREEN_BOX,
		           g_cclosure_new (G_CALLBACK (goc_offscreen_box_damage),
		                           NULL, NULL));
	container_class->add = goc_offscreen_box_add;
	container_class->remove = goc_offscreen_box_remove;
}

static void
goc_offscreen_box_init (GocOffscreenBox *offscreen_box)
{
	gtk_widget_set_has_window (GTK_WIDGET (offscreen_box), TRUE);
}

GSF_CLASS (GocOffscreenBox, goc_offscreen_box,
	   goc_offscreen_box_class_init, goc_offscreen_box_init,
	   GTK_TYPE_BIN)

static void
goc_offscreen_box_set_scale (GocOffscreenBox  *offscreen_box,
			     gdouble           scale)
{
	g_return_if_fail (GOC_IS_OFFSCREEN_BOX (offscreen_box));

	offscreen_box->scale = scale;
	gtk_widget_queue_resize (GTK_WIDGET (offscreen_box));

	/* TODO: Really needs to resent pointer events if over the scaled window */
}

/******************************************************************************
 * end of copied code
 ******************************************************************************/

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
goc_widget_connect_signals (GtkWidget *widget, GocWidget *item,
			    gboolean do_connect)
{
	if (GTK_IS_CONTAINER (widget)) {
		GList *children = gtk_container_get_children (GTK_CONTAINER (widget));
		GList *ptr;
		for (ptr = children; ptr; ptr = ptr->next) {
			GtkWidget *child = ptr->data;
			goc_widget_connect_signals (child, item, do_connect);
		}
		g_list_free (children);
	}

	if (do_connect) {
		g_signal_connect (widget, "enter-notify-event",
				  G_CALLBACK (enter_notify_cb), item);
		g_signal_connect (widget, "button-press-event",
				  G_CALLBACK (button_press_cb), item);
	} else {
		g_signal_handlers_disconnect_by_func
			(widget, G_CALLBACK (enter_notify_cb), item);
		g_signal_handlers_disconnect_by_func
			(widget, G_CALLBACK (button_press_cb), item);
	}
}

// Get rid of the off-screen box, but keep the reference to the widget, if
// any.
static void
goc_widget_destroy_ofbox (GocWidget *item)
{
	if (!item->ofbox)
		return;

	if (item->widget) {
		// Remove the widget from the ofbox.  Keep the ref.
		goc_widget_connect_signals (item->widget, item, FALSE);
		gtk_container_remove (GTK_CONTAINER (item->ofbox), item->widget);
	}

	// Destroy the ofbox
	gtk_widget_destroy (item->ofbox);
	g_object_unref (item->ofbox);
	item->ofbox = NULL;
}

static void
goc_widget_notify_scrolled (GocItem *item)
{
	GocGroup const *parent;
	double x0, y0, x1, y1;
	GocWidget *widget = GOC_WIDGET (item);
	int w, h;

	parent = item->parent;
	if (!parent)
		return;

	gtk_widget_set_size_request (widget->widget, 1, 1);
	gtk_widget_get_preferred_width (widget->widget, &w, NULL);
	gtk_widget_get_preferred_height (widget->widget, &h, NULL);
	if (w > widget-> w || h > widget->h) {
		widget->scale = fmin (widget->w / w, widget->h / h);
		if (widget->scale == 0)
			widget->scale = 1.;
		w = widget->w / widget->scale;
		h = widget->h / widget->scale;
	} else
		widget->scale = 1.;
	gtk_widget_set_size_request (widget->widget, widget->w / widget->scale, widget->h / widget->scale);
	if (!item->cached_bounds) {
		goc_widget_update_bounds (GOC_ITEM (item)); /* don't care about const */
		item->cached_bounds = TRUE;
	}
	x0 = item->x0;
	y0 = item->y0;
	x1 = item->x1;
	y1 = item->y1;
	goc_group_adjust_bounds (parent, &x0, &y0, &x1, &y1);
	if (item->canvas->direction == GOC_DIRECTION_LTR) {
		x0 = (x0 - item->canvas->scroll_x1) * item->canvas->pixels_per_unit;
		x1 = (x1 - item->canvas->scroll_x1) * item->canvas->pixels_per_unit;
	} else {
		double tmp = x1;
		x1 = item->canvas->width - (x0 - item->canvas->scroll_x1) * item->canvas->pixels_per_unit;
		x0 = item->canvas->width - (tmp - item->canvas->scroll_x1) * item->canvas->pixels_per_unit;
	}
	y0 = (y0 - item->canvas->scroll_y1) * item->canvas->pixels_per_unit;
	y1 = (y1 - item->canvas->scroll_y1) * item->canvas->pixels_per_unit;
	/* ensure we don't wrap throught he infinite */
	if (x0 < G_MININT)
		x0 = G_MININT;
	else if (x1 > G_MAXINT)
		x0 -= x1 - G_MAXINT;
	if (y0 < G_MININT)
		y0 = G_MININT;
	else if (y1 > G_MAXINT)
		y0 -= y1 - G_MAXINT;
	if (x1 >= 0 && x0 <= item->canvas->width && y1 >= 0 && y0 <= item->canvas->height) {
		if (widget->ofbox) {
			goc_offscreen_box_set_scale (GOC_OFFSCREEN_BOX (widget->ofbox),
					                     item->canvas->pixels_per_unit * widget->scale);
			gtk_widget_set_size_request (widget->ofbox, go_fake_floor (x1 - x0), go_fake_floor (y1 - y0));
			gtk_layout_move (GTK_LAYOUT (item->canvas), widget->ofbox, x0, y0);
		} else {
			gtk_widget_show (widget->widget);
			widget->ofbox = GTK_WIDGET (g_object_new (GOC_TYPE_OFFSCREEN_BOX, NULL));
			gtk_container_add (GTK_CONTAINER (widget->ofbox), widget->widget);
			gtk_widget_show (widget->ofbox);
			g_object_ref (widget->ofbox);
			goc_offscreen_box_set_scale (GOC_OFFSCREEN_BOX (widget->ofbox),
					                     item->canvas->pixels_per_unit * widget->scale);
			gtk_widget_set_size_request (widget->ofbox, go_fake_floor (x1 - x0), go_fake_floor (y1 - y0));
			gtk_layout_put (GTK_LAYOUT (item->canvas),
			                widget->ofbox, x0, y0);
			/* we need to propagate some signals to the parent item */
			goc_widget_connect_signals (widget->widget, widget, TRUE);
		}
	} else {
		goc_widget_destroy_ofbox (widget);
	}
}

static void
goc_widget_realize (GocItem *item)
{
	goc_widget_notify_scrolled (item);
	widget_parent_class->realize (item);
}

static void
cb_canvas_changed (GocWidget *item, G_GNUC_UNUSED GParamSpec *pspec,
		   G_GNUC_UNUSED gpointer user)
{
	GtkWidget *parent, *box = item->ofbox;
	GocItem *gitem = (GocItem *)item;

	if (!box)
		return;

	parent = gtk_widget_get_parent (box);
	if (parent == (GtkWidget *)gitem->canvas)
		return;

	goc_widget_destroy_ofbox (item);

	goc_widget_notify_scrolled (GOC_ITEM (item));
}

static void
goc_widget_set_widget (GocWidget *item, GtkWidget *widget)
{
	if (widget == item->widget)
		return;

	goc_widget_destroy_ofbox (item);
	if (item->widget)
		g_object_unref (item->widget);

	item->widget = widget;

	if (widget) {
		g_object_ref (widget);
		goc_widget_notify_scrolled (GOC_ITEM (item));
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
	} else if (y < widget->y + widget->h) {
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
	GocOffscreenBox *ofbox = GOC_OFFSCREEN_BOX (widget->ofbox);
	int x, y;

	if (!widget->ofbox)
		return; /* the widget has no allocation */
	gtk_container_child_get (GTK_CONTAINER (item->canvas), widget->ofbox,
	                         "x", &x, "y", &y, NULL);
	cairo_save (cr);
	cairo_translate (cr, x, y);
	cairo_scale (cr, ofbox->scale, ofbox->scale);
	gtk_widget_draw (ofbox->child, cr);
	cairo_new_path (cr);
	cairo_restore (cr);
}

static GdkWindow *
goc_widget_get_window (GocItem *item)
{
	return gtk_widget_get_window (GOC_WIDGET (item)->ofbox);
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
	item_klass->get_window = goc_widget_get_window;
	item_klass->realize = goc_widget_realize;
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

