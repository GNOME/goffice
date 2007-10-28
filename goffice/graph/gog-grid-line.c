/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-grid-line.c
 *
 * Copyright (C) 2004 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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
#include <goffice/graph/gog-axis.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/graph/gog-chart-map.h>
#include <goffice/graph/gog-data-allocator.h>
#include <goffice/graph/gog-grid-line.h>
#include <goffice/graph/gog-renderer.h>
#include <goffice/graph/gog-style.h>
#include <goffice/graph/gog-styled-object.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/graph/gog-view.h>
#include <goffice/data/go-data.h>

#include <goffice/math/go-math.h>
#include <goffice/utils/go-format.h>

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#endif

#include <glib/gi18n-lib.h>

#include <gsf/gsf-impl-utils.h>
#include <math.h>

struct _GogGridLine {
	GogStyledObject	base;

	gboolean is_minor;
};

typedef GogStyledObjectClass GogGridLineClass;

static GType gog_grid_line_view_get_type (void);

enum {
	GRID_LINE_PROP_0,
	GRID_LINE_PROP_IS_MINOR,
};

gboolean
gog_grid_line_is_minor (GogGridLine *ggl)
{
	g_return_val_if_fail (IS_GOG_GRID_LINE (ggl), FALSE);

	return ggl->is_minor;
}

static void
gog_grid_line_init_style (GogStyledObject *gso, GogStyle *style)
{
	style->interesting_fields = GOG_STYLE_LINE | GOG_STYLE_FILL;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, FALSE);
}

static void
gog_grid_line_set_property (GObject *obj, guint param_id,
			    GValue const *value, GParamSpec *pspec)
{
	GogGridLine *grid_line = GOG_GRID_LINE (obj);

	switch (param_id) {
		case GRID_LINE_PROP_IS_MINOR:
			grid_line->is_minor = g_value_get_boolean (value);
			break;

		default: 
			G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
			return; /* NOTE : RETURN */
	}

	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_grid_line_get_property (GObject *obj, guint param_id,
			    GValue *value, GParamSpec *pspec)
{
	GogGridLine *grid_line = GOG_GRID_LINE (obj);

	switch (param_id) {
		case GRID_LINE_PROP_IS_MINOR:
			g_value_set_boolean (value, grid_line->is_minor);
			break;

		default: 
			G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
			 break;
	}
}

static void
gog_grid_line_class_init (GogGridLineClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) klass;
	GogObjectClass *gog_klass = (GogObjectClass *) klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) klass;

	gobject_klass->set_property = gog_grid_line_set_property;
	gobject_klass->get_property = gog_grid_line_get_property;
	gog_klass->view_type	= gog_grid_line_view_get_type ();
	style_klass->init_style = gog_grid_line_init_style;
	
	g_object_class_install_property (gobject_klass, GRID_LINE_PROP_IS_MINOR,
		g_param_spec_boolean ("is-minor", 
			_("Is-minor"),
			_("Are these minor grid lines"), 
			FALSE, 
			GSF_PARAM_STATIC | G_PARAM_READWRITE));
}

GSF_CLASS (GogGridLine, gog_grid_line,
	   gog_grid_line_class_init, NULL,
	   GOG_STYLED_OBJECT_TYPE)

/************************************************************************/

typedef GogViewClass	GogGridLineViewClass;

#define GOG_GRID_LINE_VIEW_TYPE		(gog_grid_line_view_get_type ())
#define GOG_GRID_LINE_VIEW(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_GRID_LINE_VIEW_TYPE, GogGridLineView))
#define IS_GOG_GRID_LINE_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_GRID_LINE_VIEW_TYPE))

static void
gog_grid_line_xy_render (GogGridLine *grid_line, GogView *view,
			 GogAxis *axis, GogAxisTick *ticks, unsigned int tick_nbr,
			 GogViewAllocation const *plot_area,
			 gboolean stripes)
{
	GogAxisMap *map = NULL;
	GogAxisType axis_type = gog_axis_get_atype (axis);
	GogViewAllocation rect = {.0, .0, .0, .0};
	GOPath *path;
	unsigned int i;
	gboolean stripe_started = FALSE;

	switch (axis_type) {
		case GOG_AXIS_X:
			map = gog_axis_map_new (axis, plot_area->x, plot_area->w);
			break;
		case GOG_AXIS_Y:
			map = gog_axis_map_new (axis, plot_area->y +plot_area->h, -plot_area->h);
			break;
		default:
			return;
	}

	path = go_path_new ();
	go_path_set_options (path, GO_PATH_OPTIONS_SHARP);

	switch (axis_type) {
		case GOG_AXIS_X:
			if (stripes) {
				rect.y = plot_area->y;
				rect.h = plot_area->h;
				for (i = 0; i < tick_nbr; i++)
					if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
					    grid_line->is_minor) {
						if (stripe_started) {
							rect.w = gog_axis_map_to_view (map, ticks[i].position) -
								rect.x;
							go_path_rectangle (path, rect.x, rect.y, rect.w, rect.h);
							stripe_started = FALSE;
						} else {
							rect.x = gog_axis_map_to_view (map, ticks[i].position);
							stripe_started = TRUE;
						}
					}
				if (stripe_started) {
					rect.w = (plot_area->x + plot_area->w) - rect.x;
					go_path_rectangle (path, rect.x, rect.y, rect.w, rect.h);
				}
				gog_renderer_fill_shape (view->renderer, path);
			} else {
				double x;

				for (i = 0; i < tick_nbr; i++)
					if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
					    (ticks[i].type == GOG_AXIS_TICK_MINOR && grid_line->is_minor)) {
						x = gog_axis_map_to_view (map, ticks[i].position);
						go_path_move_to (path, x, plot_area->y);
						go_path_line_to (path, x, plot_area->y + plot_area->h);
					}
				gog_renderer_stroke_shape (view->renderer, path);
			}
			break;
		case GOG_AXIS_Y:
			if (stripes) {
				rect.x = plot_area->x;
				rect.w = plot_area->w;
				for (i = 0; i < tick_nbr; i++)
					if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
					    grid_line->is_minor) {
						if (stripe_started) {
							rect.h = gog_axis_map_to_view (map, ticks[i].position) -
								rect.y;
							go_path_rectangle (path, rect.x, rect.y, rect.w, rect.h);
							stripe_started = FALSE;
						} else {
							rect.y = gog_axis_map_to_view (map, ticks[i].position);
							stripe_started = TRUE;
						}
					}
				if (stripe_started) {
					rect.h = plot_area->y - rect.y;
					go_path_rectangle (path, rect.x, rect.y, rect.w, rect.h);
				}
				gog_renderer_fill_shape (view->renderer, path);
			} else {
				double y;

				for (i = 0; i < tick_nbr; i++) {
					if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
					    (ticks[i].type == GOG_AXIS_TICK_MINOR && grid_line->is_minor)) {
						y = gog_axis_map_to_view (map, ticks[i].position);
						go_path_move_to (path, plot_area->x, y);
						go_path_line_to (path, plot_area->x + plot_area->w, y);
					}
				}
				gog_renderer_stroke_shape (view->renderer, path);
			}
			break;
		default:
			break;
	}

	go_path_free (path);
	gog_axis_map_free (map);
}

static void
gog_grid_line_radial_render (GogGridLine *grid_line, GogView *view,
			     GogAxis *axis, GogAxisTick *ticks, unsigned int tick_nbr,
			     GogChart *chart,
			     GogViewAllocation const *plot_area,
			     gboolean stripes)
{
	GogAxis *circular_axis;
	GogAxisMap *map = NULL;
	GogChartMap *c_map;
	GogChartMapPolarData *parms;
	GSList *axis_list;
	GOPath *path;
	double x, y;
	unsigned int i, j;
	gboolean stripe_started = FALSE;

	if (gog_chart_get_axis_set (chart) == GOG_AXIS_SET_UNKNOWN)
		return;

	axis_list = gog_chart_get_axes (chart, GOG_AXIS_CIRCULAR);
	if (axis_list == NULL)
		return;
	circular_axis = GOG_AXIS (axis_list->data);
	g_slist_free (axis_list);

	c_map = gog_chart_map_new (GOG_CHART (chart), plot_area,
				   circular_axis, axis, NULL, FALSE);

	parms = gog_chart_map_get_polar_parms (c_map);
	path = go_path_new ();

	if (gog_axis_is_discrete (circular_axis)) {
		unsigned step_nbr;
		double start, stop;
		double maximum, minimum;

		map = gog_chart_map_get_axis_map (c_map, 0);
		gog_axis_map_get_extents (map, &start, &stop);
		step_nbr = go_rint (parms->th1 - parms->th0) + 1;
		if (stripes) {
			for (i = 0; i < tick_nbr; i++) {
				if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
				    grid_line->is_minor) {
					if (stripe_started) {
						for (j = 0; j <= step_nbr; j++) {
							gog_chart_map_2D_to_view (c_map,
										  step_nbr - j + parms->th0 ,
										  ticks[i].position,
										  &x, &y);
							go_path_line_to (path, x, y);
						}
						go_path_close (path);
						stripe_started = FALSE;
					} else {
						for (j = 0; j <= step_nbr; j++) {
							gog_chart_map_2D_to_view (c_map,
										  j + parms->th0 ,
										  ticks[i].position,
										  &x, &y);
							if (j == 0)
								go_path_move_to (path, x, y);
							else
								go_path_line_to (path, x, y);
						}
						stripe_started = TRUE;
					}
				}
			}
			if (stripe_started) {
				map = gog_chart_map_get_axis_map (c_map, 1);
				gog_axis_map_get_bounds (map, &minimum, &maximum);
				for (j = 0; j <= step_nbr; j++) {
					gog_chart_map_2D_to_view (c_map, step_nbr - j + parms->th0 ,
								  maximum, &x, &y);
					go_path_line_to (path, x, y);
				}
				go_path_close (path);
			}
			gog_renderer_fill_shape (view->renderer, path);
		} else {
			for (i = 0; i < tick_nbr; i++) {
				if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
				    (ticks[i].type == GOG_AXIS_TICK_MINOR && grid_line->is_minor)) {
					for (j = 0; j <= step_nbr; j++) {
						gog_chart_map_2D_to_view (c_map,
									  j + parms->th0 ,
									  ticks[i].position,
									  &x, &y);
						if (j == 0)
							go_path_move_to (path, x, y);
						else
							go_path_line_to (path, x, y);
					}
				}
			}
			gog_renderer_stroke_shape (view->renderer, path);
		}
	} else {
		double position;
		double previous_position = 0;
		double maximum, minimum;

		map = gog_chart_map_get_axis_map (c_map, 1);
		if (stripes) {
			for (i = 0; i < tick_nbr; i++) {
				if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
				    grid_line->is_minor) {
					position = gog_axis_map (map, ticks[i].position);
					if (stripe_started) {
						go_path_ring_wedge (path,
								    parms->cx, parms->cy,
								    position * parms->rx,
								    position * parms->ry,
								    previous_position * parms->rx,
								    previous_position * parms->ry,
								    -parms->th1, -parms->th0);
						stripe_started = FALSE;
					} else {
						previous_position = position;
						stripe_started = TRUE;
					}
				}
			}
			if (stripe_started) {
				gog_axis_map_get_bounds (map, &minimum, &maximum);
				position = gog_axis_map (map, maximum);
				go_path_ring_wedge (path,
						    parms->cx, parms->cy,
						    position * parms->rx,
						    position * parms->ry,
						    previous_position * parms->rx,
						    previous_position * parms->ry,
						    -parms->th1, -parms->th0);
			}
			gog_renderer_fill_shape (view->renderer, path);
		} else {
			for (i = 0; i < tick_nbr; i++) {
				if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
				    (ticks[i].type == GOG_AXIS_TICK_MINOR && grid_line->is_minor)) {
					position = gog_axis_map (map, ticks[i].position);
					go_path_arc (path, parms->cx, parms->cy,
						     position * parms->rx,
						     position * parms->ry,
						     -parms->th1, -parms->th0);
				}
			}
			gog_renderer_stroke_shape (view->renderer, path);
		}
	}
	go_path_free (path);
	gog_chart_map_free (c_map);
}

static void
gog_grid_line_circular_render (GogGridLine *grid_line, GogView *view,
			       GogAxis *axis, GogAxisTick *ticks, unsigned int tick_nbr,
			       GogChart *chart,
			       GogViewAllocation const *plot_area,
			       gboolean stripes)
{
	GogAxis *radial_axis;
	GogAxisMap *map = NULL;
	GogChartMap *c_map;
	GogChartMapPolarData *parms;
	GSList *axis_list;
	GOPath *path;
	double start, stop;
	double x0, y0, x1, y1;
	unsigned int i;
	gboolean stripe_started = FALSE;

	if (gog_chart_get_axis_set (chart) == GOG_AXIS_SET_UNKNOWN)
		return;

	axis_list = gog_chart_get_axes (chart, GOG_AXIS_RADIAL);
	if (axis_list == NULL)
		return;

	radial_axis = GOG_AXIS (axis_list->data);
	g_slist_free (axis_list);

	c_map = gog_chart_map_new (GOG_CHART (chart), plot_area,
				   axis, radial_axis, NULL, FALSE);

	parms = gog_chart_map_get_polar_parms (c_map);
	map = gog_chart_map_get_axis_map (c_map, 1);
	gog_axis_map_get_extents (map, &start, &stop);

	path = go_path_new ();

	if (stripes) {
		double last_theta = 0, theta, r_in, r_out;
		double delta_theta = parms->th1 - parms->th0;
		double maximum, minimum;

		r_in = gog_axis_map (map, start);
		r_out = gog_axis_map (map, stop);
		map = gog_chart_map_get_axis_map (c_map, 0);
		for (i = 0; i < tick_nbr; i++)
			if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
			    grid_line->is_minor) {
				if (stripe_started) {
					theta = gog_axis_map (map, ticks[i].position);
					go_path_ring_wedge (path,
							    parms->cx, parms->cy,
							    r_in * parms->rx,
							    r_in * parms->ry,
							    r_out * parms->rx,
							    r_out * parms->ry,
							    -parms->th0 - theta * delta_theta,
							    -parms->th0 - last_theta * delta_theta);
					stripe_started = FALSE;
				} else {
					last_theta = gog_axis_map (map, ticks[i].position);
					stripe_started = TRUE;
				}
			}
		if (stripe_started) {
			gog_axis_map_get_bounds (map, &minimum, &maximum);
			theta = gog_axis_map (map, maximum);
			go_path_ring_wedge (path,
					    parms->cx, parms->cy,
					    r_in * parms->rx,
					    r_in * parms->ry,
					    r_out * parms->rx,
					    r_out * parms->ry,
					    -parms->th0 - theta * delta_theta,
					    -parms->th0 - last_theta * delta_theta);
		}
		gog_renderer_fill_shape (view->renderer, path);
	} else {
		gog_chart_map_2D_to_view (c_map, 0, start, &x0, &y0);
		for (i = 0; i < tick_nbr; i++)
			if ((ticks[i].type == GOG_AXIS_TICK_MAJOR && !grid_line->is_minor) ||
			    (ticks[i].type == GOG_AXIS_TICK_MINOR && grid_line->is_minor)) {
				gog_chart_map_2D_to_view (c_map, ticks[i].position, stop,
							  &x1, &y1);
				go_path_move_to (path, x0, y0);
				go_path_line_to (path, x1, y1);
			}
		gog_renderer_stroke_shape (view->renderer, path);
	}
	go_path_free (path);
	gog_chart_map_free (c_map);
}

static void
gog_grid_line_view_render (GogView *view, gboolean stripes)
{
	GogGridLine *grid_line = GOG_GRID_LINE (view->model);
	GogAxis *axis;
	GogChart *chart;
	GogView *chart_view;
	GogStyle *style;
	GogAxisType axis_type;
	GogAxisTick *ticks;
	unsigned tick_nbr;
	GogViewAllocation const *plot_area;

	axis = GOG_AXIS (view->model->parent);
	g_return_if_fail (axis != NULL);
	chart = GOG_CHART (view->model->parent->parent);
	g_return_if_fail (chart != NULL);
	g_return_if_fail (view->parent != NULL);
	chart_view = GOG_VIEW (view->parent->parent);
	g_return_if_fail (chart_view != NULL);

	axis_type = gog_axis_get_atype (axis);
	tick_nbr = gog_axis_get_ticks (axis, &ticks);
	if (tick_nbr < 1)
		return;

	plot_area = gog_chart_view_get_plot_area (chart_view);
	style = gog_styled_object_get_style (GOG_STYLED_OBJECT (grid_line));
	gog_renderer_push_style (view->renderer, style);

	if ((!stripes && gog_style_is_line_visible (style)) ||
	    (stripes && (style->fill.type != GOG_FILL_STYLE_NONE))) {
		switch (axis_type) {
			case GOG_AXIS_X:
			case GOG_AXIS_Y:
				gog_grid_line_xy_render (grid_line, view,
							 axis, ticks, tick_nbr,
							 plot_area, stripes);
				break;

			case GOG_AXIS_RADIAL: 
				gog_grid_line_radial_render (grid_line, view, 
							     axis, ticks, tick_nbr,
							     chart, plot_area, stripes);
				break;
					      
			case GOG_AXIS_CIRCULAR: 
				gog_grid_line_circular_render (grid_line, view, 
							       axis, ticks, tick_nbr,
							       chart, plot_area, stripes);
				break;

			default:
				break;
		}
	}

	gog_renderer_pop_style (view->renderer);
}

void
gog_grid_line_view_render_stripes (GogView *view)
{
	gog_grid_line_view_render (view, TRUE);
}

void
gog_grid_line_view_render_lines (GogView *view)
{
	gog_grid_line_view_render (view, FALSE);
}

static GSF_CLASS (GogGridLineView, gog_grid_line_view,
	   NULL /* gog_grid_line_view_class_init */, NULL,
	   GOG_VIEW_TYPE)
