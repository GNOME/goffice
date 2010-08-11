/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-graph.c :
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
 * SECTION:goc-graph
 * @short_description: Graph.
 *
 * #GocLine implements #GogGraph embedding in the canvas.
**/

static GObjectClass *parent_klass;

enum {
	GRAPH_PROP_0,
	GRAPH_PROP_X,
	GRAPH_PROP_Y,
	GRAPH_PROP_H,
	GRAPH_PROP_W,
	GRAPH_PROP_GRAPH,
	GRAPH_PROP_RENDERER
};

static void
goc_graph_set_property (GObject *obj, guint param_id,
			GValue const *value, GParamSpec *pspec)
{
	GocGraph *graph = GOC_GRAPH (obj);
	gboolean setup_renderer = FALSE;

	switch (param_id) {
	case GRAPH_PROP_X: {
		double x = g_value_get_double (value);
		if (x == graph->x)
			return;
		graph->x = x;
		break;
	}
	case GRAPH_PROP_Y: {
		double y = g_value_get_double (value);
		if (y == graph->y)
			return;
		graph->y = y;
		break;
	}
	case GRAPH_PROP_H: {
		double h = g_value_get_double (value);
		if (h == graph->h)
			return;
		graph->h = h;
		break;
	}
	case GRAPH_PROP_W: {
		double w = g_value_get_double (value);
		if (w == graph->w)
			return;
		graph->w = w;
		break;
	}
	case GRAPH_PROP_GRAPH:
		if (graph->renderer != NULL)
			g_object_unref (graph->renderer);
		graph->renderer = gog_renderer_new (g_value_get_object (value));
		setup_renderer = graph->renderer != NULL;
		break;

	case GRAPH_PROP_RENDERER:
		if (graph->renderer != NULL)
			g_object_unref (graph->renderer);
		graph->renderer = GOG_RENDERER (g_value_get_object (value));
		if (graph->renderer != NULL) {
			g_object_ref (graph->renderer);
			setup_renderer = TRUE;
		}
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		return; /* NOTE : RETURN */
	}

	if (setup_renderer)
		g_signal_connect_object (G_OBJECT (graph->renderer),
			"request-update",
			G_CALLBACK (goc_item_invalidate),
			graph, G_CONNECT_SWAPPED);
	goc_item_bounds_changed (GOC_ITEM (graph));
}

static void
goc_graph_get_property (GObject *obj, guint param_id,
			GValue *value, GParamSpec *pspec)
{
	GocGraph *graph = GOC_GRAPH (obj);

	switch (param_id) {
	case GRAPH_PROP_X:
		g_value_set_double (value, graph->x);
		break;
	case GRAPH_PROP_Y:
		g_value_set_double (value, graph->y);
		break;
	case GRAPH_PROP_H:
		g_value_set_double (value, graph->h);
		break;
	case GRAPH_PROP_W:
		g_value_set_double (value, graph->w);
		break;
	case GRAPH_PROP_RENDERER:
		g_value_set_object (value, graph->renderer);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		break;
	}
}

static void
goc_graph_finalize (GObject *obj)
{
	GocGraph *graph = GOC_GRAPH (obj);

	if (graph->renderer != NULL) {
		g_object_unref (graph->renderer);
		graph->renderer = NULL;
	}
	if (graph->coords.timer_id) {
		g_source_remove (graph->coords.timer_id);
		graph->coords.timer_id = 0;
	}
	(*parent_klass->finalize) (obj);
}

static double
goc_graph_distance (GocItem *item, double x, double y, GocItem **near_item)
{
	GocGraph *graph = GOC_GRAPH (item);
	double dx, dy;
	x -= graph->x;
	y -= graph->y;
	if (x < 0) {
		dx = -x;
	} else if (x < graph->w) {
		dx = 0;
	} else {
		dx = x - graph->w;
	}
	if (y < 0) {
		dy = -y;
	} else if (y < graph->h) {
		dy = 0;
	} else {
		dy = y - graph->h;
	}
	*near_item = item;
	return hypot (dx, dy);
}

static void
goc_graph_draw (GocItem const *item, cairo_t *cr)
{
	GocGraph *graph = GOC_GRAPH (item);
	GocCanvas *canvas = item->canvas;
	cairo_surface_t *surf;
	double x0, y0 = item->y0;
	if (graph->renderer == NULL)
		return;
	if (goc_canvas_get_direction (item->canvas) == GOC_DIRECTION_RTL) {
		x0 = item->x1;
		goc_group_adjust_coords (item->parent, &x0, &y0);
		x0 = canvas->width - (int) (x0 - canvas->scroll_x1) * canvas->pixels_per_unit;
	} else {
		x0 = item->x0;
		goc_group_adjust_coords (item->parent, &x0, &y0);
		x0 = go_fake_round ((x0 - canvas->scroll_x1) * canvas->pixels_per_unit);
	}
	cairo_save (cr);
	cairo_translate (cr, x0,
	                 (int) (y0 - canvas->scroll_y1) * canvas->pixels_per_unit);
	/* scaling only there gives a better rendering, and allows for caching */
	gog_renderer_update (graph->renderer,
				      go_fake_round (graph->w * canvas->pixels_per_unit),
				      go_fake_round (graph->h * canvas->pixels_per_unit));
	surf = gog_renderer_get_cairo_surface (graph->renderer);
	cairo_set_source_surface (cr, surf, 0., 0.);
	cairo_paint (cr);
	cairo_restore (cr);
}

static void
goc_graph_update_bounds (GocItem *item)
{
	GocGraph *graph = GOC_GRAPH (item);
	item->x0 = graph->x;
	item->y0 = graph->y;
	item->x1 = graph->x + graph->w;
	item->y1 = graph->y + graph->h;
	gog_renderer_update (graph->renderer, 
				      (int) (graph->w * item->canvas->pixels_per_unit),
				      (int) (graph->h * item->canvas->pixels_per_unit));
}

static char *
format_coordinate (GogAxis *axis, GOFormat *fmt, double x)
{
	GString *res = g_string_sized_new (20);
	int width = (fmt && !go_format_is_general (fmt)) ? -1 : 8; /* FIXME? */
	const GODateConventions *date_conv = gog_axis_get_date_conv (axis);
	GOFormatNumberError err = go_format_value_gstring
		(NULL, res,
		 go_format_measure_strlen,
		 go_font_metrics_unit,
		 fmt,
		 x, 'F', NULL,
		 NULL,
		 width, date_conv, TRUE);
	if (err) {
		/* Invalid number for format.  */
		g_string_assign (res, "#####");
	}

	return g_string_free (res, FALSE);
}

static void
goc_graph_do_tooltip (GocGraph *graph)
{
	GogView *view;
	char *buf = NULL, *s1 = NULL, *s2 = NULL;
	GogObject *obj;
	GogChart *chart;
	GogViewAllocation alloc;
	double xpos, ypos;
	GogChartMap *map;
	GogAxis *x_axis, *y_axis;
	GogAxisSet set;
	GSList *l;
	GOFormat *format;
	GocItem *item = (GocItem *)graph;
	double x = graph->coords.x;
	double y = graph->coords.y;

	/* translate x and y tovalues relative to the graph */
	xpos = graph->x;
	ypos = graph->y;
	goc_group_adjust_coords (item->parent, &xpos, &ypos);
	x -= xpos;
	y -= ypos;

	/* get the GogView at the cursor position */
	g_object_get (G_OBJECT (graph->renderer), "view", &view, NULL);
	g_object_unref (view); /* we don't need a reference */
	gog_view_get_view_at_point (view, x, y, &obj, NULL);
	if (!obj)
		goto tooltip;
	chart = GOG_CHART (gog_object_get_parent_typed (obj, GOG_TYPE_CHART));
	if (!chart || gog_chart_is_3d (chart))
		goto tooltip;
	set = gog_chart_get_axis_set (chart) & GOG_AXIS_SET_FUNDAMENTAL;
	/* get the plot allocation */
	l = gog_object_get_children (GOG_OBJECT (chart), gog_object_find_role_by_name (GOG_OBJECT (chart), "Plot"));
	if (l == NULL)
		return;
	view = gog_view_find_child_view (view, GOG_OBJECT (l->data));
	g_slist_free (l);
	if (!view)
		return;
	alloc = view->allocation;
	switch (set) {
	case GOG_AXIS_SET_XY:
		/* get the axis */
		l = gog_chart_get_axes (chart, GOG_AXIS_X);
		x_axis = GOG_AXIS (l->data);
		g_slist_free (l);
		l = gog_chart_get_axes (chart, GOG_AXIS_Y);
		y_axis = GOG_AXIS (l->data);
		g_slist_free (l);
		break;
	case GOG_AXIS_SET_RADAR:
		/* get the axis */
		l = gog_chart_get_axes (chart, GOG_AXIS_CIRCULAR);
		x_axis = GOG_AXIS (l->data);
		g_slist_free (l);
		l = gog_chart_get_axes (chart, GOG_AXIS_RADIAL);
		y_axis = GOG_AXIS (l->data);
		g_slist_free (l);
		break;
	default:
		buf = gog_view_get_tip_at_point (view, x, y);
		goto tooltip;
	}
	map = gog_chart_map_new (chart, &alloc, x_axis, y_axis, NULL, FALSE);
	if (gog_chart_map_is_valid (map) &&
				x >= alloc.x && x < alloc.x + alloc.w &&
				y >= alloc.y && y < alloc.y + alloc.h) {
		gog_chart_map_view_to_2D (map, x, y, &x, &y);
		if (gog_axis_is_discrete (x_axis)) {
			GODataVector *labels = GO_DATA_VECTOR (gog_axis_get_labels (x_axis, NULL));
			if (labels && x <= go_data_vector_get_len (labels))
				s1 = go_data_vector_get_str (labels, x - 1);
			if (!s1 || *s1 == 0) {
				g_free (s1);
				s1 = format_coordinate (x_axis, NULL, x);
			}
		} else {
			format = gog_axis_get_effective_format (x_axis);
			s1 = format_coordinate (x_axis, format, x);
		}
		if (gog_axis_is_discrete (y_axis)) {
			GOData *labels = gog_axis_get_labels (y_axis, NULL);
			if (labels)
				s2 = go_data_vector_get_str (GO_DATA_VECTOR (labels), y - 1);
			if (!s2 || *s2 == 0) {
				g_free (s2);
				s2 = format_coordinate (y_axis, NULL, y);
			}
		} else {
			format = gog_axis_get_effective_format (y_axis);
			s2 = format_coordinate (y_axis, format, y);
		}
		/* Note to translators: the following is a format string for a pair of coordinates */
		buf = g_strdup_printf (_("(%s,%s)"), s1, s2);
		g_free (s1);
		g_free (s2);
	}
	gog_chart_map_free (map);
tooltip:
	gtk_widget_set_tooltip_markup (GTK_WIDGET (item->canvas), buf);
	g_free (buf);
}

static gboolean
goc_graph_timer (GocGraph *graph)
{
	goc_graph_do_tooltip (graph);
	graph->coords.timer_id = 0;
	return FALSE;
}

static gboolean
goc_graph_motion (GocItem *item, double x, double y)
{
	GocGraph *graph = GOC_GRAPH (item);

	/*
	 * Do not allow more than 20 updates per second.  We do this by
	 * scheduling the actual update in a timeout.
	 */
	if (graph->coords.timer_id == 0) {
		graph->coords.timer_id =
			g_timeout_add (50,
				       (GSourceFunc)goc_graph_timer,
				       graph);
	}

	/* When the timer first, use the last (x,y) we have.  */
	graph->coords.x = x;
	graph->coords.y = y;

	return ((GocItemClass*) parent_klass)->motion (item, x, y);
}

static gboolean
goc_graph_leave_notify (GocItem *item, double x, double y)
{
	GocGraph *graph = GOC_GRAPH (item);

	if (graph->coords.timer_id) {
		g_source_remove (graph->coords.timer_id);
		graph->coords.timer_id = 0;
	}
	gtk_widget_set_tooltip_text (GTK_WIDGET (item->canvas), NULL);
	return ((GocItemClass*) parent_klass)->leave_notify (item, x, y);
}

static void
goc_graph_class_init (GocItemClass *item_klass)
{
	GObjectClass *obj_klass = (GObjectClass *) item_klass;

	parent_klass = g_type_class_peek_parent (obj_klass);

	obj_klass->set_property = goc_graph_set_property;
	obj_klass->get_property = goc_graph_get_property;
	obj_klass->finalize	    = goc_graph_finalize;

	g_object_class_install_property (obj_klass, GRAPH_PROP_X,
		g_param_spec_double ("x",
			_("x"),
			_("The graph left position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, GRAPH_PROP_Y,
		g_param_spec_double ("y",
			_("y"),
			_("The graph top position"),
			-G_MAXDOUBLE, G_MAXDOUBLE, 0.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, GRAPH_PROP_H,
		 g_param_spec_double ("height",
			_("Height"),
			_("Height"),
			0, G_MAXDOUBLE, 100.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, GRAPH_PROP_W,
		 g_param_spec_double ("width",
			_("Width"),
			_("Width"),
			0, G_MAXDOUBLE, 100.,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
	g_object_class_install_property (obj_klass, GRAPH_PROP_GRAPH,
		g_param_spec_object ("graph",
			_("Graph"),
			_("The GogGraph this object displays"),
			GOG_TYPE_GRAPH,
			GSF_PARAM_STATIC | G_PARAM_WRITABLE));
	g_object_class_install_property (obj_klass, GRAPH_PROP_RENDERER,
		g_param_spec_object ("renderer",
			_("Renderer"),
			_("The GogRenderer being displayed"),
			GOG_TYPE_RENDERER,
			GSF_PARAM_STATIC | G_PARAM_READWRITE));

	item_klass->draw = goc_graph_draw;
	item_klass->update_bounds = goc_graph_update_bounds;
	item_klass->distance = goc_graph_distance;
	item_klass->leave_notify = goc_graph_leave_notify;
	item_klass->motion = goc_graph_motion;
}

GSF_CLASS (GocGraph, goc_graph,
	   goc_graph_class_init, NULL,
	   GOC_TYPE_ITEM)
