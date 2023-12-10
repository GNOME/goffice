/*
 * go-graph-widget.c:
 *
 * Copyright (C) 2003-2005 Jean Brefort (jean.brefort@normalesup.org)
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <goffice-config.h>
#include "go-graph-widget.h"
#include <goffice/graph/gog-graph.h>
#include <goffice/graph/gog-object.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/math/go-math.h>

#include <gsf/gsf-impl-utils.h>

/**
 * GOGraphWidgetSizeMode:
 * @GO_GRAPH_WIDGET_SIZE_MODE_FIT: fits the allocated size, conserving the aspect
 * ratio.
 * @GO_GRAPH_WIDGET_SIZE_MODE_FIT_WIDTH: fits the width, conserving the aspect
 * ratio.
 * @GO_GRAPH_WIDGET_SIZE_MODE_FIT_HEIGHT: fits the width, conserving the aspect
 * ratio.
 * @GO_GRAPH_WIDGET_SIZE_MODE_FIXED_SIZE: use original size.
 **/

static void go_graph_widget_request_update (GOGraphWidget *w);

/**
 * SECTION: go-graph-widget
 * @short_description: Widget showing a #GogGraph.
 *
 * Utility widget for showing a #GogGraph.
 */

enum {
	GRAPH_WIDGET_PROP_0,
	GRAPH_WIDGET_PROP_ASPECT_RATIO,
	GRAPH_WIDGET_PROP_GRAPH,
	GRAPH_WIDGET_PROP_HRES,
	GRAPH_WIDGET_PROP_VRES
};

struct  _GOGraphWidget{
	GtkLayout base;

	GogRenderer *renderer;
	GogGraph *graph;
	double aspect_ratio;
	guint width, height, xoffset, yoffset;
	int requested_width, requested_height;
	int button_press_x, button_press_y;
	gboolean button_pressed;
	double hres, vres;

	/* Idle handler ID */
	guint idle_id;

	GOGraphWidgetSizeMode size_mode;
};

struct _GOGraphWidgetClass {
	GtkLayoutClass base_class;
};

static GtkWidgetClass *graph_parent_klass;

static void
update_image_rect (GOGraphWidget *gw,
		   GtkAllocation allocation)
{
	gw->width = gw->height = -1;

	switch (gw->size_mode) {
		case GO_GRAPH_WIDGET_SIZE_MODE_FIT:
			gw->width = allocation.width;
			gw->height = allocation.height;
			break;

		case GO_GRAPH_WIDGET_SIZE_MODE_FIT_WIDTH:
			gw->width = allocation.width;
			break;

		case GO_GRAPH_WIDGET_SIZE_MODE_FIT_HEIGHT:
			gw->height = allocation.height;
			break;

		case GO_GRAPH_WIDGET_SIZE_MODE_FIXED_SIZE:
			gw->width = gw->requested_width;
			gw->height = gw->requested_height;
			break;

		default:
			g_assert_not_reached ();
	}

	if (gw->aspect_ratio > 0.) {
		g_assert (gw->size_mode != GO_GRAPH_WIDGET_SIZE_MODE_FIXED_SIZE);

		if ((gw->size_mode == GO_GRAPH_WIDGET_SIZE_MODE_FIT &&
		     gw->height > gw->width * gw->aspect_ratio) ||
		    gw->size_mode == GO_GRAPH_WIDGET_SIZE_MODE_FIT_WIDTH) {
			gw->height = gw->width * gw->aspect_ratio;
		} else {
			gw->width = gw->height / gw->aspect_ratio;
		}
	}

	gw->yoffset = MAX (0, (int) (allocation.height - gw->height) / 2);
	gw->xoffset = MAX (0, (int) (allocation.width - gw->width) / 2);

	gog_graph_set_size (gw->graph, gw->width * 72. / gw->hres, gw->height * 72. / gw->vres);
	gog_renderer_update (gw->renderer, gw->width, gw->height);
}


/* Size allocation handler for the widget */
static void
go_graph_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GOGraphWidget *w = GO_GRAPH_WIDGET (widget);

	update_image_rect (GO_GRAPH_WIDGET (widget), *allocation);

	gtk_layout_set_size (GTK_LAYOUT (widget), w->width + w->xoffset, w->height + w->yoffset);

	GTK_WIDGET_CLASS (graph_parent_klass)->size_allocate (widget, allocation);
}

static gboolean
go_graph_widget_draw (GtkWidget *widget, cairo_t *cairo)
{
	GOGraphWidget *w = GO_GRAPH_WIDGET (widget);
	cairo_surface_t *surface;


	surface = gog_renderer_get_cairo_surface (w->renderer);
	if (surface != NULL) {
		cairo_rectangle (cairo, w->xoffset, w->yoffset, w->width, w->height);
		cairo_clip (cairo);
		cairo_set_source_surface (cairo, surface, w->xoffset, w->yoffset);
		cairo_paint (cairo);
	}

	return FALSE;
}

static gboolean
go_graph_widget_button_press_event (GtkWidget *widget,
				    GdkEventButton *event)
{
	GOGraphWidget *gw = GO_GRAPH_WIDGET (widget);

	if (event->type == GDK_BUTTON_PRESS) {
		gw->button_pressed = TRUE;

		gdk_window_get_device_position (gtk_widget_get_window (widget),
		                                event->device,
		                                &gw->button_press_x,
		                                &gw->button_press_y,
		                                NULL);
	}

	if (GTK_WIDGET_CLASS (graph_parent_klass)->button_press_event != NULL) {
		return (GTK_WIDGET_CLASS (graph_parent_klass))->button_press_event (widget, event);
	}

	return FALSE;
}

static gboolean
go_graph_widget_button_release_event (GtkWidget *widget,
				      GdkEventButton *event)
{
	GOGraphWidget *gw = GO_GRAPH_WIDGET (widget);

	if (event->type == GDK_BUTTON_RELEASE) {
		gw->button_pressed = FALSE;
	}

	if (GTK_WIDGET_CLASS (graph_parent_klass)->button_release_event != NULL) {
		return (GTK_WIDGET_CLASS (graph_parent_klass))->button_release_event (widget, event);
	}

	return FALSE;
}

static gboolean
go_graph_widget_motion_notify_event (GtkWidget *widget,
				     GdkEventMotion *event)
{
	GOGraphWidget *gw = GO_GRAPH_WIDGET (widget);
	int x, y;
	double newval;

	if (gw->button_pressed) {
		GtkAdjustment *hadjustment, *vadjustment;
		hadjustment = gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (gw));
		vadjustment = gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (gw));
		gdk_window_get_device_position (gtk_widget_get_window (widget),
		                                 event->device, &x, &y, NULL);

		if (hadjustment != NULL) {
			newval = gtk_adjustment_get_value (hadjustment) - (x - gw->button_press_x);
			newval = CLAMP (newval, 0,
			                gtk_adjustment_get_upper (hadjustment) -
			                gtk_adjustment_get_page_size (hadjustment));
			gtk_adjustment_set_value (hadjustment, newval);
		}

		if (vadjustment != NULL) {
			newval = gtk_adjustment_get_value (vadjustment) - (y - gw->button_press_y);
			newval = CLAMP (newval, 0,
			                gtk_adjustment_get_upper (vadjustment) -
			                gtk_adjustment_get_page_size (vadjustment));
			gtk_adjustment_set_value (vadjustment, newval);
		}

		gw->button_press_x = x;
		gw->button_press_y = y;
	}

	if (GTK_WIDGET_CLASS (graph_parent_klass)->motion_notify_event != NULL) {
		return (GTK_WIDGET_CLASS (graph_parent_klass))->motion_notify_event (widget, event);
	}

	return FALSE;
}

static void
go_graph_widget_finalize (GObject *object)
{
	GOGraphWidget *w = GO_GRAPH_WIDGET (object);
	if (w->idle_id)
		g_source_remove (w->idle_id);
	g_object_unref (w->graph);
	g_object_unref (w->renderer);
	(G_OBJECT_CLASS (graph_parent_klass))->finalize (object);
}

static gint
idle_handler (GOGraphWidget *w)
{
	GtkAllocation allocation;

	/* Reset idle id */
	w->idle_id = 0;
	gtk_widget_get_allocation (GTK_WIDGET (w), &allocation);
	update_image_rect (w, allocation);
	gtk_widget_queue_draw (GTK_WIDGET (w));

	return FALSE;
}

static void
go_graph_widget_request_update (GOGraphWidget *w)
{
	if (!w->idle_id)
		w->idle_id = g_idle_add_full (GDK_PRIORITY_REDRAW - 20,
						   (GSourceFunc) idle_handler, w, NULL);
}

static void
go_graph_widget_set_property (GObject *obj, guint param_id,
			     GValue const *value, GParamSpec *pspec)
{
	GOGraphWidget *w = GO_GRAPH_WIDGET (obj);

	switch (param_id) {
	case GRAPH_WIDGET_PROP_ASPECT_RATIO:
		w->aspect_ratio = g_value_get_double (value);
		break;
	case GRAPH_WIDGET_PROP_GRAPH:
		w->graph = (GogGraph *) g_value_dup_object (value);
		w->renderer = gog_renderer_new (w->graph);
		g_signal_connect_swapped (w->renderer, "request_update",
			G_CALLBACK (go_graph_widget_request_update), w);
		break;
	case GRAPH_WIDGET_PROP_HRES:
		w->hres = g_value_get_double (value);
		break;
	case GRAPH_WIDGET_PROP_VRES:
		w->vres = g_value_get_double (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gtk_widget_queue_resize (GTK_WIDGET (obj));
}

static void
go_graph_widget_get_property (GObject *obj, guint param_id,
			     GValue *value, GParamSpec *pspec)
{
	GOGraphWidget *w = GO_GRAPH_WIDGET (obj);

	switch (param_id) {
	case GRAPH_WIDGET_PROP_ASPECT_RATIO:
		g_value_set_double (value, w->aspect_ratio);
		break;
	case GRAPH_WIDGET_PROP_GRAPH:
		g_value_set_object (value, w->graph);
		break;
	case GRAPH_WIDGET_PROP_HRES:
		g_value_set_double (value, w->hres);
		break;
	case GRAPH_WIDGET_PROP_VRES:
		g_value_set_double (value, w->vres);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
go_graph_widget_class_init (GOGraphWidgetClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

	graph_parent_klass = (GtkWidgetClass *) g_type_class_peek_parent (klass);

	object_class->finalize = go_graph_widget_finalize;
	object_class->get_property = go_graph_widget_get_property;
	object_class->set_property = go_graph_widget_set_property;
	widget_class->size_allocate = go_graph_widget_size_allocate;
	widget_class->draw = go_graph_widget_draw;
	widget_class->button_press_event = go_graph_widget_button_press_event;
	widget_class->button_release_event = go_graph_widget_button_release_event;
	widget_class->motion_notify_event = go_graph_widget_motion_notify_event;
	g_object_class_install_property (object_class,
		GRAPH_WIDGET_PROP_ASPECT_RATIO,
		g_param_spec_double ("aspect-ratio", "aspect-ratio",
			"Aspect ratio for rendering the graph, used only if greater than 0.",
			-G_MAXDOUBLE, G_MAXDOUBLE, -1., G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
	g_object_class_install_property (object_class,
		GRAPH_WIDGET_PROP_GRAPH,
		g_param_spec_object ("graph", "graph",
			"The graph to render.",
			gog_graph_get_type (),
			G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
		GRAPH_WIDGET_PROP_HRES,
		g_param_spec_double ("hres", "hres",
			"Assumed horizontal screen resolution in pixels per inch.",
			1., G_MAXDOUBLE, 96., G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
		GRAPH_WIDGET_PROP_VRES,
		g_param_spec_double ("vres", "vres",
			"Assumed vertical screen resolution in pixels per inch.",
			1., G_MAXDOUBLE, 96., G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
go_graph_widget_init (GOGraphWidget *w)
{
	gtk_widget_add_events (GTK_WIDGET (w), GDK_POINTER_MOTION_MASK |
					       GDK_BUTTON_PRESS_MASK |
					       GDK_BUTTON_RELEASE_MASK);
	w->hres = w->vres = 96.;
}

/**
 * go_graph_widget_set_size_mode:
 * @widget: #GOGraphWidget
 * @size_mode: #GOGraphWidgetSizeMode
 * @width: in pixels
 * @height: in pixels
 *
 * Sets the size mode of the #GOGraphWidget.
 * It is used to determine the size and position of the drawn
 * chart. The following sizing modes are supported:
 *
 * GO_GRAPH_WIDGET_SIZE_MODE_FIT, aspect ratio set.
 * The aspect ratio is guaranteed to be maintained,
 * i.e. the graph is never squeezed, and will
 * always fit into the visible area.
 *
 * GO_GRAPH_WIDGET_SIZE_MODE_FIT, no aspect ratio set.
 * The aspect ratio is adapted to make the graph
 * exactly fit into the visible area.
 *
 * GO_GRAPH_WIDGET_SIZE_MODE_FIT_WIDTH, aspect ratio set.
 * The aspect ratio is guaranteed to be maintained,
 * i.e. the graph is never squezzed, and will
 * always occupy the whole width of the visible area.
 *
 * GO_GRAPH_WIDGET_SIZE_MODE_FIT_HEIGHT, aspect ratio set.
 * The aspect ratio is guaranteed to be maintained,
 * i.e. the graph is never squezzed, and will
 * always occupy the whole height of the visible area.
 *
 * GO_GRAPH_WIDGET_SIZE_MODE_FIT_FIXED_SIZE, no aspect ratio set.
 * The graph will occupy the area specified by width/height,
 * its aspect ratio will be determined by height/width.
 **/
void
go_graph_widget_set_size_mode (GOGraphWidget         *widget,
			       GOGraphWidgetSizeMode  size_mode,
			       int                    width,
			       int                    height)
{
	GtkAllocation allocation;

	g_return_if_fail (GO_IS_GRAPH_WIDGET (widget));
	g_return_if_fail (size_mode >= GO_GRAPH_WIDGET_SIZE_MODE_FIT &&
			  size_mode <= GO_GRAPH_WIDGET_SIZE_MODE_FIXED_SIZE);
	g_return_if_fail (!(width >= 0 && height < 0));
	g_return_if_fail (!(width < 0 && height >= 0));
	g_return_if_fail (!(width >= 0 && size_mode != GO_GRAPH_WIDGET_SIZE_MODE_FIXED_SIZE));
	g_return_if_fail (!(width < 0 && size_mode == GO_GRAPH_WIDGET_SIZE_MODE_FIXED_SIZE));

	widget->size_mode = size_mode;
	widget->requested_width = width;
	widget->requested_height = height;

	gtk_widget_get_allocation (GTK_WIDGET (widget), &allocation);
	update_image_rect (widget, allocation);
}

/**
 * go_graph_widget_new:
 * @graph: #GogGraph
 *
 * Creates a new #GOGraphWidget with an embedded #GogGraph.
 * If graph is NULL, the graph will be auto-created, and a
 * #GogChart will be added graph.
 *
 * Returns: the newly created #GOGraphWidget.
 **/
GtkWidget *
go_graph_widget_new (GogGraph *graph)
{
	GtkWidget *ret;
	gboolean self_owned = FALSE;

	if (graph == NULL) {
		self_owned = TRUE;

		graph = (GogGraph *) g_object_new (GOG_TYPE_GRAPH, NULL);
		gog_object_add_by_name (GOG_OBJECT (graph), "Chart", NULL);
	}

	ret = GTK_WIDGET (g_object_new (GO_TYPE_GRAPH_WIDGET, "graph", graph, NULL));
	go_graph_widget_set_size_mode (GO_GRAPH_WIDGET (ret), GO_GRAPH_WIDGET_SIZE_MODE_FIT, -1, -1);

	if (self_owned)
		g_object_unref (graph);

	return ret;
}

GSF_CLASS (GOGraphWidget, go_graph_widget,
	   go_graph_widget_class_init, go_graph_widget_init,
	   gtk_layout_get_type ())

/**
 * go_graph_widget_get_graph:
 * @widget: #GOGraphWidget
 *
 * Returns: (transfer none): the #GogGraph embedded in the widget.
 **/
GogGraph *
go_graph_widget_get_graph (GOGraphWidget *widget)
{
	g_return_val_if_fail (GO_IS_GRAPH_WIDGET (widget), NULL);
	return widget->graph;
}

/**
 * go_graph_widget_get_chart:
 * @widget: #GOGraphWidget
 *
 * Returns: (transfer none): the #GogChart created by go_graph_widget_new().
 **/
GogChart *
go_graph_widget_get_chart (GOGraphWidget *widget)
{
	g_return_val_if_fail (GO_IS_GRAPH_WIDGET (widget), NULL);
	return (GogChart *) gog_object_get_child_by_name (
						GOG_OBJECT (widget->graph), "Chart");
}

/**
 * go_graph_widget_get_renderer:
 * @widget: #GOGraphWidget
 *
 * Returns: (transfer none): the #GogRenderer used by the widget.
 **/
GogRenderer *go_graph_widget_get_renderer (GOGraphWidget *widget)
{
	g_return_val_if_fail (GO_IS_GRAPH_WIDGET (widget), NULL);
	return widget->renderer;
}
