/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-path.c
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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
 *
 * This code is an adaptation of the path code which can be found in
 * the cairo library (http://cairographics.org).
 *
 * Contributor(s):
 *	Carl D. Worth <cworth@cworth.org>
 *	Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
 */

#include <goffice/math/go-math.h>
#include <goffice/utils/go-path.h>
#include <goffice/utils/go-cairo.h>
#include <string.h>

/**
 * GOPathOptions
 * @GO_PATH_OPTIONS_SNAP_COORDINATES: round coordinates to avoid aliasing in pixbufs.
 * @GO_PATH_OPTIONS_SNAP_WIDTH: round width so that it correpond to a round pixels number.
 * @GO_PATH_OPTIONS_SHARP: use raw coordinates.
 **/

#define GO_PATH_DEFAULT_BUFFER_SIZE 64

typedef struct _GOPathData	 GOPathData;
typedef struct _GOPathDataBuffer GOPathDataBuffer;

typedef enum _GOPathAction {
	GO_PATH_ACTION_MOVE_TO 		= 0,
	GO_PATH_ACTION_LINE_TO 		= 1,
	GO_PATH_ACTION_CURVE_TO 	= 2,
	GO_PATH_ACTION_CLOSE_PATH 	= 3
} GOPathAction;

static int action_n_args[4] = { 1, 1, 3, 0};

struct _GOPathDataBuffer {
	int n_points;
	int n_actions;

	GOPathAction 	*actions;
	GOPathPoint 	*points;

	struct _GOPathDataBuffer *next;
	struct _GOPathDataBuffer *previous;
};

struct _GOPath {
	GOPathDataBuffer *data_buffer_head;
	GOPathDataBuffer *data_buffer_tail;

	GOPathOptions 	  options;
	unsigned	  refs; 
};

static void
go_path_data_buffer_free (GOPathDataBuffer *buffer)
{
	g_free (buffer->points);
	g_free (buffer->actions);
	g_free (buffer);
}

static void
go_path_data_buffer_clear (GOPathDataBuffer *buffer)
{
	buffer->n_points = 0;
	buffer->n_actions = 0;
}

static GOPathDataBuffer *
go_path_data_buffer_new (void)
{
	GOPathDataBuffer *buffer;

	buffer = g_new (GOPathDataBuffer, 1);
	if (buffer == NULL) {
		g_warning ("[GOPathDataBuffer::new] can't create data buffer");
		return NULL;
	}

	buffer->points = g_new (GOPathPoint, GO_PATH_DEFAULT_BUFFER_SIZE);
	if (buffer->points == NULL) {
		g_warning ("[GOPathDataBuffer::new] can't create data buffer");
		g_free (buffer);
		return NULL;
	}

	buffer->actions = g_new (GOPathAction, GO_PATH_DEFAULT_BUFFER_SIZE);
	if (buffer->actions == NULL) {
		g_warning ("[GOPathDataBuffer::new] can't create data buffer");
		g_free (buffer->points);
		g_free (buffer);
		return NULL;
	}

	buffer->n_points = 0;
	buffer->n_actions  =0;
	buffer->next = NULL;
	buffer->previous = NULL;

	return buffer;
}

static GOPathDataBuffer *
go_path_add_data_buffer (GOPath *path)
{
	GOPathDataBuffer *buffer;

	buffer = go_path_data_buffer_new ();
	if (buffer == NULL)
		return NULL;

	if (path->data_buffer_head == NULL) {
		path->data_buffer_head = buffer;
		path->data_buffer_tail = buffer;
		return buffer;
	}
	buffer->previous = path->data_buffer_tail;
	path->data_buffer_tail->next = buffer;
	path->data_buffer_tail = buffer;

	return buffer;
}

GOPath *
go_path_new (void)
{
	GOPath *path;

	path = g_new (GOPath, 1);
	if (path == NULL) {
		g_warning ("[GOPath::new] can't create path");
		return NULL;
	}
	path->data_buffer_tail = NULL;
	path->data_buffer_head = NULL;
	path->options = 0;

	if (go_path_add_data_buffer (path) == NULL) {
		g_free (path);
		return NULL;
	}

	path->refs = 1;

	return path;
}

void
go_path_clear (GOPath *path)
{
	GOPathDataBuffer *buffer;

	g_return_if_fail (GO_IS_PATH (path));

	if (path->data_buffer_head == NULL)
		return;

	while (path->data_buffer_head->next != NULL) {
		buffer = path->data_buffer_head->next->next;
		go_path_data_buffer_free (path->data_buffer_head->next);
		path->data_buffer_head->next = buffer;
	}
	go_path_data_buffer_clear (path->data_buffer_head);
	path->data_buffer_tail = path->data_buffer_head;
}

/**
 * go_path_free:
 * @path: a #GOPath
 *
 * Decrements references count and frees all memory allocated for @path if
 * references count reaches 0.
 **/

void
go_path_free (GOPath *path)
{
	GOPathDataBuffer *buffer;

	g_return_if_fail (path != NULL);

	path->refs--;
	if (path->refs > 0)
		return;

	while (path->data_buffer_head != NULL) {
		buffer = path->data_buffer_head->next;
		go_path_data_buffer_free (path->data_buffer_head);
		path->data_buffer_head = buffer;
	}
	g_free (path);
}

/**
 * go_path_ref:
 * @path: a #GOPath
 *
 * Increments references count to @path.
 **/

void
go_path_ref (GOPath *path)
{
	g_return_if_fail (path != NULL);

	path->refs++;
}

GType
go_path_get_type (void)
{
    static GType type_path = 0;

    if (!type_path)
	type_path = g_boxed_type_register_static
	    ("GOPath",
	     (GBoxedCopyFunc) go_path_ref,
	     (GBoxedFreeFunc) go_path_free);

    return type_path;
}

/**
 * go_path_set_options:
 * @path: a #GOPath
 * @options: #GOPathOptions
 *
 * Change the rendering options for @path using
 *	%GO_PATH_OPTIONS_SNAP_COORDINATES
 *	%GO_PATH_OPTIONS_SNAP_WIDTH
 *	%GO_PATH_OPTIONS_SHARP
 **/
void
go_path_set_options (GOPath *path, GOPathOptions options)
{
	g_return_if_fail (GO_IS_PATH (path));

	path->options = options;
}

GOPathOptions
go_path_get_options (GOPath const *path)
{
	g_return_val_if_fail (GO_IS_PATH (path), 0);

	return path->options;
}

static void
go_path_add_points (GOPath *path, GOPathAction action,
		    GOPathPoint *points, int n_points)
{
	GOPathDataBuffer *buffer = path->data_buffer_tail;
	int i;

	g_return_if_fail (GO_IS_PATH (path));

	if (buffer->n_actions + 1 > GO_PATH_DEFAULT_BUFFER_SIZE
	    || buffer->n_points + n_points > GO_PATH_DEFAULT_BUFFER_SIZE)
		buffer = go_path_add_data_buffer (path);

	buffer->actions[buffer->n_actions++] = action;

	for (i = 0; i < n_points; i++)
		buffer->points[buffer->n_points++] = points[i];
}

void
go_path_move_to (GOPath *path, double x, double y)
{
	GOPathPoint point;

	point.x = GO_CAIRO_CLAMP (x);
	point.y = GO_CAIRO_CLAMP (y);
	go_path_add_points (path, GO_PATH_ACTION_MOVE_TO, &point, 1);
}

void
go_path_line_to (GOPath *path, double x, double y)
{
	GOPathPoint point;

	point.x = GO_CAIRO_CLAMP (x);
	point.y = GO_CAIRO_CLAMP (y);
	go_path_add_points (path, GO_PATH_ACTION_LINE_TO, &point, 1);
}

void
go_path_curve_to (GOPath *path,
		  double x0, double y0,
		  double x1, double y1,
		  double x2, double y2)
{
	GOPathPoint points[3];

	points[0].x = GO_CAIRO_CLAMP (x0);
	points[0].y = GO_CAIRO_CLAMP (y0);
	points[1].x = GO_CAIRO_CLAMP (x1);
	points[1].y = GO_CAIRO_CLAMP (y1);
	points[2].x = GO_CAIRO_CLAMP (x2);
	points[2].y = GO_CAIRO_CLAMP (y2);
	go_path_add_points (path, GO_PATH_ACTION_CURVE_TO, points, 3);
}

void
go_path_close (GOPath *path)
{
	go_path_add_points (path, GO_PATH_ACTION_CLOSE_PATH, NULL, 0);
}

static void
_ring_wedge (GOPath *path,
	     double cx, double cy,
	     double rx_out, double ry_out,
	     double rx_in, double ry_in,
	     double th0, double th1,
	     gboolean arc_to)
{
	double th_arc, th_out, th_in, th_delta, t, r;
	double x, y;
	int i, n_segs;
	gboolean fill;
	gboolean draw_in, ellipse = FALSE;

	g_return_if_fail (GO_IS_PATH (path));

	if (rx_out < rx_in) {
		r = rx_out;
		rx_out = rx_in;
		rx_in = r;
	}
	if (ry_out < ry_in) {
		r = ry_out;
		ry_out = ry_in;
		ry_in = r;
	}
	/* Here we tolerate slightly negative values for inside radius
	 * when deciding to fill. We use outside radius for comparison. */
	fill = rx_in >= -(rx_out * 1e-6) && ry_in >= -(ry_out * 1e-6);

	/* Here also we use outside radius for comparison. If inside radius
	 * is high enough, we'll emit an arc, otherwise we just use lines to
	 * wedge center. */
	draw_in = fill && (rx_in > rx_out * 1e-6) && (ry_in > ry_out * 1e-6);

	if (go_add_epsilon (th1 - th0) >= 2 * M_PI) {
		ellipse = TRUE;
		th1 = th0 + 2 * M_PI;
	} else if (go_add_epsilon (th0 - th1) >= 2 * M_PI) {
		ellipse = TRUE;
		th0 = th1 + 2 * M_PI;
	}

	th_arc = th1 - th0;
	n_segs = ceil (fabs (th_arc / (M_PI * 0.5 + 0.001)));

	if (arc_to && !fill)
		go_path_line_to (path, cx + rx_out * cos (th0), cy + ry_out * sin (th0));
	else
		go_path_move_to (path, cx + rx_out * cos (th0), cy + ry_out * sin (th0));

	th_delta = th_arc / n_segs;
	t = (8.0 / 3.0) * sin (th_delta * 0.25) * sin (th_delta * 0.25) / sin (th_delta * 0.5);

	th_out = th0;
	for (i = 0; i < n_segs; i++) {
		x = cx + rx_out * cos (th_out + th_delta);
		y = cy + ry_out * sin (th_out + th_delta);
		go_path_curve_to (path,
				  cx + rx_out * (cos (th_out) - t * sin (th_out)),
				  cy + ry_out * (sin (th_out) + t * cos (th_out)),
				  x + rx_out * t * sin (th_out + th_delta),
				  y - ry_out * t * cos (th_out + th_delta),
				  x, y);
		th_out += th_delta;
	}

	if (!fill)
		return;

	if (ellipse) {
		go_path_close (path);
		if (!draw_in)
			return;
	}

	if (!ellipse) {
		if (draw_in)
			go_path_line_to (path,
					 cx + rx_in * cos (th1),
					 cy + ry_in * sin (th1));
		else
			go_path_line_to (path, cx, cy);
	}

	if (draw_in) {
		th_in = th1;
		for (i = 0; i < n_segs; i++) {
			x = cx + rx_in * cos (th_in - th_delta);
			y = cy + ry_in * sin (th_in - th_delta);
			go_path_curve_to (path,
					  cx + rx_in * (cos (th_in) + t * sin (th_in)),
					  cy + ry_in * (sin (th_in) - t * cos (th_in)),
					  x - rx_in * t * sin (th_in - th_delta),
					  y + ry_in * t * cos (th_in - th_delta),
					  x, y);
			th_in -= th_delta;
		}
	}

	go_path_close (path);
}

void
go_path_ring_wedge (GOPath *path,
		    double cx, double cy,
		    double rx_out, double ry_out,
		    double rx_in, double ry_in,
		    double th0, double th1)
{
	_ring_wedge (path, cx, cy, rx_out, ry_out, rx_in, ry_in, th0, th1, FALSE);
}

void
go_path_pie_wedge (GOPath *path,
		   double cx, double cy,
		   double rx, double ry,
		   double th0, double th1)
{
	_ring_wedge (path, cx, cy, rx, ry, 0.0, 0.0, th0, th1, FALSE);
}

void
go_path_arc (GOPath *path,
	     double cx, double cy,
	     double rx, double ry,
	     double th0, double th1)
{
	_ring_wedge (path, cx, cy, rx, ry, -1.0, -1.0, th0, th1, FALSE);
}

void
go_path_arc_to (GOPath *path,
		double cx, double cy,
		double rx, double ry,
		double th0, double th1)
{
	_ring_wedge (path, cx, cy, rx, ry, -1.0, -1.0, th0, th1, TRUE);
}

void
go_path_rectangle (GOPath *path,
		   double x, double y,
		   double width, double height)
{
	go_path_move_to (path, x, y);
	go_path_line_to (path, x + width, y);
	go_path_line_to (path, x + width, y + height);
	go_path_line_to (path, x, y + height);
	go_path_close (path);
}

void
go_path_interpret (GOPath const		*path,
		   GOPathDirection 	 direction,
		   GOPathMoveToFunc 	*move_to,
		   GOPathLineToFunc 	*line_to,
		   GOPathCurveToFunc 	*curve_to,
		   GOPathClosePathFunc 	*close_path,
		   void *closure)
{
	GOPathDataBuffer *buffer;
	GOPathAction action, next_action;
	GOPathPoint *points;
	GOPathPoint *prev_control_points = NULL;
	gboolean forward = (direction == GO_PATH_DIRECTION_FORWARD);
	int index;

	if (path == NULL)
		return;

	if (forward) {
		for (buffer = path->data_buffer_head; buffer != NULL; buffer = buffer->next) {
			int i;
			points = buffer->points;

			for (i = 0; i != buffer->n_actions; i++) {

				action = buffer->actions[i];

				switch (action) {
					case GO_PATH_ACTION_MOVE_TO:
						(*move_to) (closure, &points[0]);
						break;
					case GO_PATH_ACTION_LINE_TO:
						(*line_to) (closure, &points[0]);
						break;
					case GO_PATH_ACTION_CURVE_TO:
						(*curve_to) (closure, &points[0], &points[1], &points[2]);
						break;
					case GO_PATH_ACTION_CLOSE_PATH:
					default:
						(*close_path) (closure);
						break;
				}
				points += action_n_args[action];
			}
		}
		return;
	}

	next_action = GO_PATH_ACTION_MOVE_TO;

	for (buffer = path->data_buffer_tail; buffer != NULL; buffer = buffer->previous) {
		int i;

		points = buffer->points + buffer->n_points;

		for (i = buffer->n_actions - 1; i != -1; i--) {
			action = next_action;
			next_action = buffer->actions[i];

			points -= action_n_args[next_action];

			index = next_action == GO_PATH_ACTION_CURVE_TO ? 2 : 0;

			switch (action) {
				case GO_PATH_ACTION_MOVE_TO:
					(*move_to) (closure, &points[index]);
					break;
				case GO_PATH_ACTION_LINE_TO:
					(*line_to) (closure, &points[index]);
					break;
				case GO_PATH_ACTION_CURVE_TO:
					(*curve_to) (closure,
						     &prev_control_points[1],
						     &prev_control_points[0],
						     &points[index]);
					break;
				case GO_PATH_ACTION_CLOSE_PATH:
				default:
					(*close_path) (closure);
					break;
			}

			prev_control_points = &points[0];
		}
	}
}

static void
go_path_cairo_move_to (cairo_t *cr, GOPathPoint const *point)
{
	cairo_move_to (cr, point->x, point->y);
}

static void
go_path_cairo_line_to (cairo_t *cr, GOPathPoint const *point)
{
	cairo_line_to (cr, point->x, point->y);
}

static void
go_path_cairo_curve_to (cairo_t *cr, GOPathPoint const *point0,
			GOPathPoint const *point1, GOPathPoint const *point2)
{
	cairo_curve_to (cr, point0->x, point0->y, point1->x, point1->y, point2->x, point2->y);
}

/** go_path_to_cairo:
 * @path: #GOPath
 * @direction: #GOPathDirection
 * @cr: #cairo_t
 *
 * Renders the path to the cairo context using its current settings.
 */

void
go_path_to_cairo (GOPath const *path, GOPathDirection direction, cairo_t *cr)
{
	go_path_interpret (path, direction,
			   (GOPathMoveToFunc *) go_path_cairo_move_to,
			   (GOPathLineToFunc *) go_path_cairo_line_to,
			   (GOPathCurveToFunc *) go_path_cairo_curve_to,
			   (GOPathClosePathFunc *) cairo_close_path, cr);
}

static void
go_path_append_move_to (GOPath *path, GOPathPoint const *point)
{
	go_path_move_to (path, point->x, point->y);
}

static void
go_path_append_line_to (GOPath *path, GOPathPoint const *point)
{
	go_path_line_to (path, point->x, point->y);
}

static void
go_path_append_curve_to (GOPath *path, GOPathPoint const *point0,
			GOPathPoint const *point1, GOPathPoint const *point2)
{
	go_path_curve_to (path, point0->x, point0->y, point1->x, point1->y, point2->x, point2->y);
}

static void
go_path_append_close (GOPath *path)
{
	go_path_close (path);
}

/** go_path_copy:
 * @path: #GOPath
 *
 * Returns: a new #GOPath identical to @path.
 */

GOPath *go_path_copy (GOPath const *path)
{
	GOPath *new_path = go_path_new ();
	new_path->options = path->options;
	go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
			   (GOPathMoveToFunc *) go_path_append_move_to,
			   (GOPathLineToFunc *) go_path_append_line_to,
			   (GOPathCurveToFunc *) go_path_append_curve_to,
			   (GOPathClosePathFunc *) go_path_append_close, new_path);
	return new_path;
}

/**
 * go_path_append:
 * @path1: #GOPath
 * @path2: #GOPath
 *
 * Appends @path2 at the end of @path1.
 * Returns: @path1
 */
GOPath *go_path_append (GOPath *path1, GOPath const *path2)
{
	go_path_interpret (path2, GO_PATH_DIRECTION_FORWARD,
			   (GOPathMoveToFunc *) go_path_append_move_to,
			   (GOPathLineToFunc *) go_path_append_line_to,
			   (GOPathCurveToFunc *) go_path_append_curve_to,
			   (GOPathClosePathFunc *) go_path_append_close, path1);
	return path1;
}
