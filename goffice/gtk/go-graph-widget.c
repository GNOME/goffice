/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-graph-widget.c : 
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <goffice-config.h>
#include "go-graph-widget.h"
#include <gtk/gtkdrawingarea.h>
#include <goffice/graph/gog-object.h>
#include <goffice/graph/gog-renderer-pixbuf.h>
#include <goffice/utils/go-math.h>

#include <gsf/gsf-impl-utils.h>


struct  _GOGraphWidget{
	GtkDrawingArea	base;

	GogRendererPixbuf *renderer;
	GogGraph *graph;
	GogChart *chart; /* first chart crated on init */
	gboolean aspect_ratio_set;
	double aspect_ratio, width, height, xoffset, yoffset;

	/* Idle handler ID */
	guint idle_id;
};

typedef GtkDrawingAreaClass GOGraphWidgetClass;

static GtkWidgetClass *graph_parent_klass;

/* Size allocation handler for the widget */
static void
go_graph_widget_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	GOGraphWidget *w = GO_GRAPH_WIDGET (widget);
	w->width = allocation->width;
	w->height = allocation->height;
	if (!w->aspect_ratio_set) {
		w->aspect_ratio = w->height / w->width;
		if (!isnan (w->aspect_ratio) && go_finite (w->aspect_ratio))
			w->aspect_ratio_set = TRUE;
	}
	if (w->height > w->width * w->aspect_ratio) {
		w->yoffset = (w->height - w->width * w->aspect_ratio) / 2.;
		w->height = w->width * w->aspect_ratio;
		w->xoffset = 0;
	} else {
		w->xoffset = (w->width - w->height / w->aspect_ratio) / 2.;
		w->width = w->height / w->aspect_ratio;
		w->yoffset = 0;
	}
	gog_renderer_pixbuf_update (w->renderer, w->width, w->height, 1.0);
	graph_parent_klass->size_allocate (widget, allocation);
}

static gboolean
go_graph_widget_expose_event (GtkWidget *widget, GdkEventExpose *event)
{
	GOGraphWidget *w = GO_GRAPH_WIDGET (widget);
	GdkPixbuf *pixbuf;
	GdkRectangle display_rect, draw_rect;
	GdkRegion *draw_region;

	if (w->idle_id)
		return TRUE;
	pixbuf = gog_renderer_pixbuf_get (w->renderer);
	display_rect.x = w->xoffset;
	display_rect.y = w->yoffset;
	display_rect.width  = w->width;
	display_rect.height = w->height;
	draw_region = gdk_region_rectangle (&display_rect);
	gdk_region_intersect (draw_region, event->region);
	if (!gdk_region_empty (draw_region)) {
		gdk_region_get_clipbox (draw_region, &draw_rect);
		gdk_draw_pixbuf (widget->window, NULL, pixbuf,
			/* pixbuf 0, 0 is at pix_rect.x, pix_rect.y */
			     draw_rect.x - display_rect.x,
			     draw_rect.y - display_rect.y,
			     draw_rect.x,
			     draw_rect.y,
			     draw_rect.width,
			     draw_rect.height,
			     GDK_RGB_DITHER_NORMAL, 0, 0);
	}
	gdk_region_destroy (draw_region);
	return FALSE;
}

static void
go_graph_widget_finalize (GObject *object)
{
	GOGraphWidget *w = GO_GRAPH_WIDGET (object);
	g_object_unref (w->graph);
	g_object_unref (w->renderer);
	(G_OBJECT_CLASS (graph_parent_klass))->finalize (object);
}

static void
go_graph_widget_class_init (GOGraphWidgetClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

	graph_parent_klass = (GtkWidgetClass *) g_type_class_peek_parent (klass);

	object_class->finalize = go_graph_widget_finalize;
	widget_class->size_allocate = go_graph_widget_size_allocate;
	widget_class->expose_event = go_graph_widget_expose_event;
}

static gint
idle_handler (GOGraphWidget *w)
{
	GDK_THREADS_ENTER ();

	gog_renderer_pixbuf_update (w->renderer, w->width, w->height, 1.0);

	/* Reset idle id */
	w->idle_id = 0;
	gtk_widget_queue_draw (GTK_WIDGET (w));

	GDK_THREADS_LEAVE ();

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
go_graph_widget_init (GOGraphWidget *w)
{
	w->graph = (GogGraph *) g_object_new (GOG_GRAPH_TYPE, NULL);
	w->renderer = g_object_new (GOG_RENDERER_PIXBUF_TYPE,
					  "model", w->graph,
					  NULL);
	g_signal_connect_swapped (w->renderer, "request_update",
		G_CALLBACK (go_graph_widget_request_update), w);
	/* by default, create one chart and add it to the graph */
	w->chart = (GogChart *) 
			gog_object_add_by_name (GOG_OBJECT (w->graph), "Chart", NULL);
	w->aspect_ratio_set = FALSE;
	w->idle_id = 0;
}

GtkWidget *
go_graph_widget_new (void)
{
	return GTK_WIDGET (g_object_new (GO_GRAPH_WIDGET_TYPE, NULL));
}

GSF_CLASS (GOGraphWidget, go_graph_widget,
	   go_graph_widget_class_init, go_graph_widget_init,
	   gtk_drawing_area_get_type ())

GogGraph *
go_graph_widget_get_graph (GOGraphWidget *widget)
{
	g_return_val_if_fail (IS_GO_GRAPH_WIDGET (widget), NULL);
	return widget->graph;
}

GogChart *
go_graph_widget_get_chart (GOGraphWidget *widget)
{
	g_return_val_if_fail (IS_GO_GRAPH_WIDGET (widget), NULL);
	return widget->chart;
}
