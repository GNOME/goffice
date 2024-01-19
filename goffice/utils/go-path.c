/*
 * go-path.c
 *
 * Copyright © 2002 University of Southern California
 * Copyright © 2005 Red Hat, Inc.
 * Copyright © 2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

/* the five following does not belong there, but they don't in any other .c file */

/**
 * GOPoint:
 * @x: horizontal position.
 * @y: vertical position.
 **/

/**
 * GORect:
 * @top: top.
 * @left: left.
 * @bottom: bottom.
 * @right: right.
 **/

/**
 * GOAnchorType:
 * @GO_ANCHOR_CENTER: anchor center.
 * @GO_ANCHOR_NORTH: anchor top.
 * @GO_ANCHOR_NORTH_WEST: anchor top left.
 * @GO_ANCHOR_NORTH_EAST: anchor top left.
 * @GO_ANCHOR_SOUTH: anchor bottom.
 * @GO_ANCHOR_SOUTH_WEST: anchor bottom left.
 * @GO_ANCHOR_SOUTH_EAST: anchor bottom left.
 * @GO_ANCHOR_WEST: anchor left.
 * @GO_ANCHOR_EAST: anchor right.
 * @GO_ANCHOR_N: anchor top.
 * @GO_ANCHOR_NW: anchor top left.
 * @GO_ANCHOR_NE: anchor top left.
 * @GO_ANCHOR_S: anchor bottom.
 * @GO_ANCHOR_SW: anchor bottom left.
 * @GO_ANCHOR_SE: anchor bottom left.
 * @GO_ANCHOR_W: anchor left.
 * @GO_ANCHOR_E: anchor right.
 * @GO_ANCHOR_BASELINE_CENTER: anchor centered on baseline
 * @GO_ANCHOR_BASELINE_WEST: anchor left on baseline
 * @GO_ANCHOR_BASELINE_EAST: anchor right on baseline
 * @GO_ANCHOR_B: synonym for GO_ANCHOR_BASELINE_CENTER
 * @GO_ANCHOR_BW: synonym for GO_ANCHOR_BASELINE_WEST
 * @GO_ANCHOR_BE: synonym for GO_ANCHOR_BASELINE_EAST
 **/

/**
 * GODrawingAnchor:
 * @pos_pts: position in points.
 * @direction: #GODrawingAnchorDir
 **/

/**
 * GODrawingAnchorDir:
 * @GOD_ANCHOR_DIR_UNKNOWN: unknown.
 * @GOD_ANCHOR_DIR_UP_LEFT: up left.
 * @GOD_ANCHOR_DIR_UP_RIGHT: up right.
 * @GOD_ANCHOR_DIR_DOWN_LEFT: down left.
 * @GOD_ANCHOR_DIR_DOWN_RIGHT: down right.
 * @GOD_ANCHOR_DIR_NONE_MASK: mask for none.
 * @GOD_ANCHOR_DIR_H_MASK: horizontal mask.
 * @GOD_ANCHOR_DIR_RIGHT: right
 * @GOD_ANCHOR_DIR_V_MASK: vertical mask.
 * @GOD_ANCHOR_DIR_DOWN: down
 **/

/************************************/

/**
 * GOPathOptions:
 * @GO_PATH_OPTIONS_SNAP_COORDINATES: round coordinates to avoid aliasing in pixbufs.
 * @GO_PATH_OPTIONS_SNAP_WIDTH: round width so that it correpond to a round pixels number.
 * @GO_PATH_OPTIONS_SHARP: use raw coordinates.
 **/

/**
 * GOPathPoint:
 * @x: horizontal position.
 * @y: vertical dimension.
 **/

/**
 * GOPathDirection:
 * @GO_PATH_DIRECTION_FORWARD: go through the pass from start to end.
 * @GO_PATH_DIRECTION_BACKWARD:  go through the pass from end to start.
 **/

#define GO_PATH_DEFAULT_BUFFER_SIZE 64

typedef struct _GOPathData	 GOPathData;
typedef struct _GOPathDataBuffer GOPathDataBuffer;

typedef enum _GOPathAction {
	GO_PATH_ACTION_MOVE_TO 		= 0,
	GO_PATH_ACTION_LINE_TO 		= 1,
	GO_PATH_ACTION_CURVE_TO 	= 2,
	GO_PATH_ACTION_CLOSE_PATH 	= 3,
	GO_PATH_ACTION_CHANGE_STATE
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
 * Returns: the path with an incremented references count.
 **/
GOPath *
go_path_ref (GOPath *path)
{
	g_return_val_if_fail (path != NULL, NULL);

	path->refs++;
	return path;
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
	GOPathDataBuffer *buffer;
	int i;

	g_return_if_fail (GO_IS_PATH (path));

	buffer = path->data_buffer_tail;
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

/**
 * go_path_interpret:
 * @path: #GOPath
 * @direction: #GOPathDirection
 * @move_to: (scope call): the callback for move to.
 * @line_to: (scope call): the callback for drawing a line.
 * @curve_to: (scope call): the callback for drawing a bezier cubic spline.
 * @close_path: (scope call): the callback for closing the path.
 * @closure: data to pass as first argument to the callbacks.
 *
 * This function can be used to draw a path or for other purposes.
 * To draw using cairo, the closure argument should be a valid #cairo_t.
 **/
void
go_path_interpret (GOPath const		*path,
		   GOPathDirection 	 direction,
		   GOPathMoveToFunc 	 move_to,
		   GOPathLineToFunc 	 line_to,
		   GOPathCurveToFunc 	 curve_to,
		   GOPathClosePathFunc 	 close_path,
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
						move_to (closure, &points[0]);
						break;
					case GO_PATH_ACTION_LINE_TO:
						line_to (closure, &points[0]);
						break;
					case GO_PATH_ACTION_CURVE_TO:
						curve_to (closure, &points[0], &points[1], &points[2]);
						break;
					case GO_PATH_ACTION_CLOSE_PATH:
					default:
						close_path (closure);
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

/**
 * go_path_interpret_full:
 * @path: #GOPath
 * @start: index of the first action to interpret
 * @end: index of the last action to interpret
 * @direction: #GOPathDirection
 * @move_to: (scope call): the callback for move to.
 * @line_to: (scope call): the callback for drawing a line.
 * @curve_to: (scope call): the callback for drawing a bezier cubic spline.
 * @close_path: (scope call): the callback for closing the path.
 * @closure: data to pass as first argument to the callbacks.
 *
 * This function can be used to draw a portion path or for other purposes.
 * Only actions between start and end will be executed. If start or end is
 * negative, it is not taken into account.
 * Since: 0.10.5
 **/
void
go_path_interpret_full (GOPath const		*path,
                        gssize				 start,
                        gssize				 end,
                        GOPathDirection 	 direction,
                        GOPathMoveToFunc 	 move_to,
                        GOPathLineToFunc 	 line_to,
                        GOPathCurveToFunc 	 curve_to,
                        GOPathClosePathFunc  close_path,
                        void				*closure)
{
	GOPathDataBuffer *buffer;
	GOPathAction action, next_action;
	GOPathPoint *points;
	GOPathPoint *prev_control_points = NULL;
	gboolean forward = (direction == GO_PATH_DIRECTION_FORWARD);
	int index;
	gssize cur;

	if (path == NULL || start >= end)
		return;

	if (forward) {
		cur = 0;
		for (buffer = path->data_buffer_head; buffer != NULL; buffer = buffer->next) {
			int i;
			points = buffer->points;

			for (i = 0; i != buffer->n_actions; i++) {

				action = buffer->actions[i];

				if (end > 0 && cur > end)
					return;
				else if (cur == start)
					switch (action) {
					case GO_PATH_ACTION_MOVE_TO:
					case GO_PATH_ACTION_LINE_TO:
						move_to (closure, &points[0]);
						break;
					case GO_PATH_ACTION_CURVE_TO:
						move_to (closure, &points[2]);
						break;
					case GO_PATH_ACTION_CLOSE_PATH:
					default:
						break;
					}
				else if (cur > start)
					switch (action) {
					case GO_PATH_ACTION_MOVE_TO:
						move_to (closure, &points[0]);
						break;
					case GO_PATH_ACTION_LINE_TO:
						line_to (closure, &points[0]);
						break;
					case GO_PATH_ACTION_CURVE_TO:
						curve_to (closure, &points[0], &points[1], &points[2]);
						break;
					case GO_PATH_ACTION_CLOSE_PATH:
					default:
						close_path (closure);
						break;
					}
				points += action_n_args[action];
				cur++;
			}
		}
		return;
	}

	next_action = GO_PATH_ACTION_MOVE_TO;
	cur = 0;
	for (buffer = path->data_buffer_head; buffer != NULL; buffer = buffer->next)
		cur += buffer->n_actions;

	for (buffer = path->data_buffer_tail; buffer != NULL; buffer = buffer->previous) {
		int i;

		points = buffer->points + buffer->n_points;

		for (i = buffer->n_actions - 1; i != -1; i--) {
			cur--;
			action = next_action;
			next_action = buffer->actions[i];

			points -= action_n_args[next_action];

			index = next_action == GO_PATH_ACTION_CURVE_TO ? 2 : 0;

			if (end > 0 && cur > end)
				continue;
			else if (cur == end)
				switch (action) {
				case GO_PATH_ACTION_MOVE_TO:
				case GO_PATH_ACTION_LINE_TO:
				case GO_PATH_ACTION_CURVE_TO:
					(*move_to) (closure, &points[index]);
					break;
				case GO_PATH_ACTION_CLOSE_PATH:
				default:
					break;
				}
			else {
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
				if (cur < start)
					return;
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

/**
 * go_path_to_cairo:
 * @path: #GOPath
 * @direction: #GOPathDirection
 * @cr: #cairo_t
 *
 * Renders the path to the cairo context using its current settings.
 **/

void
go_path_to_cairo (GOPath const *path, GOPathDirection direction, cairo_t *cr)
{
	go_path_interpret (path, direction,
			   (GOPathMoveToFunc) go_path_cairo_move_to,
			   (GOPathLineToFunc) go_path_cairo_line_to,
			   (GOPathCurveToFunc) go_path_cairo_curve_to,
			   (GOPathClosePathFunc) cairo_close_path, cr);
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

/**
 * go_path_copy:
 * @path: (nullable): #GOPath
 *
 * Returns: (transfer full) (nullable): a new #GOPath identical to @path.
 **/
GOPath *
go_path_copy (GOPath const *path)
{
	GOPath *new_path;

	if (path == NULL)
		return NULL;
	new_path = go_path_new ();
	new_path->options = path->options;
	go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
			   (GOPathMoveToFunc) go_path_append_move_to,
			   (GOPathLineToFunc) go_path_append_line_to,
			   (GOPathCurveToFunc) go_path_append_curve_to,
			   (GOPathClosePathFunc) go_path_append_close, new_path);
	return new_path;
}


/**
 * go_path_copy_restricted:
 * @path: #GOPath
 * @start: the first action to copy
 * @end: the second action to copy
 *
 * Copies actions between start and end will be copied inside a new #GOPath.
 * If start or end is negative, it is not taken into account.
 *
 * Since: 0.10.5
 * Returns: (transfer full): a new #GOPath.
 **/
GOPath *
go_path_copy_restricted (GOPath const *path, gssize start, gssize end)
{
	GOPath *new_path;
	if (path == NULL)
		return NULL;
	new_path = go_path_new ();
	new_path->options = path->options;
	go_path_interpret_full (path, start, end, GO_PATH_DIRECTION_FORWARD,
			   (GOPathMoveToFunc) go_path_append_move_to,
			   (GOPathLineToFunc) go_path_append_line_to,
			   (GOPathCurveToFunc) go_path_append_curve_to,
			   (GOPathClosePathFunc) go_path_append_close, new_path);
	return new_path;
}

/**
 * go_path_append: (skip)
 * @path1: #GOPath
 * @path2: #GOPath
 *
 * Appends @path2 at the end of @path1.
 * Returns: @path1
 */
GOPath *
go_path_append (GOPath *path1, GOPath const *path2)
{
	if (path2 == NULL)
		return path1;
	if (path1 == NULL)
		return go_path_copy (path2);
	go_path_interpret (path2, GO_PATH_DIRECTION_FORWARD,
			   (GOPathMoveToFunc) go_path_append_move_to,
			   (GOPathLineToFunc) go_path_append_line_to,
			   (GOPathCurveToFunc) go_path_append_curve_to,
			   (GOPathClosePathFunc) go_path_append_close, path1);
	return path1;
}

/******************************************************************************/
struct PathScaleClosure {
	GOPath *path;
	double scale_x, scale_y;
};

static void
go_path_scale_move_to (struct PathScaleClosure *closure,
                        GOPathPoint const *point)
{
	go_path_move_to (closure->path,
	                 point->x * closure->scale_x,
	                 point->y * closure->scale_y);
}

static void
go_path_scale_line_to (struct PathScaleClosure *closure,
                        GOPathPoint const *point)
{
	go_path_line_to (closure->path,
	                 point->x * closure->scale_x,
	                 point->y * closure->scale_y);
}

static void
go_path_scale_curve_to (struct PathScaleClosure *closure,
                         GOPathPoint const *point0,
                         GOPathPoint const *point1,
                         GOPathPoint const *point2)
{
	go_path_curve_to (closure->path,
	                  point0->x * closure->scale_x,
	                  point0->y * closure->scale_y,
	                  point1->x * closure->scale_x,
	                  point1->y * closure->scale_y,
	                  point2->x * closure->scale_x,
	                  point2->y * closure->scale_y);
}

static void
go_path_scale_close (struct PathScaleClosure *closure)
{
	go_path_close (closure->path);
}

/**
 * go_path_scale:
 * @path: #GOPath
 * @scale_x: horizontal scale.
 * @scale_y: vertical scale.
 *
 * Builds a scaled.
 * Returns: (transfer full): the scaled path.
 **/
GOPath *
go_path_scale (GOPath *path, double scale_x, double scale_y)
{
	struct PathScaleClosure closure;
	closure.path = go_path_new ();
	closure.scale_x = scale_x;
	closure.scale_y = scale_y;
	go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
	                   (GOPathMoveToFunc) go_path_scale_move_to,
	                   (GOPathLineToFunc) go_path_scale_line_to,
	                   (GOPathCurveToFunc) go_path_scale_curve_to,
	                   (GOPathClosePathFunc) go_path_scale_close, &closure);
	return closure.path;
}

/******************************************************************************/

struct PathSvgClosure {
	GString *str;
	char last_op;
};

static void
go_path_svg_move_to (struct PathSvgClosure *closure,
                        GOPathPoint const *point)
{
	if (closure->last_op != 'M') {
		g_string_append (closure->str, " M");
		closure->last_op = 'M';
	}
	g_string_append_printf (closure->str, " %g %g", point->x, point->y);
}

static void
go_path_svg_line_to (struct PathSvgClosure *closure,
                        GOPathPoint const *point)
{
	if (closure->last_op != 'L') {
		g_string_append (closure->str, " L");
		closure->last_op = 'L';
	}
	g_string_append_printf (closure->str, " %g %g", point->x, point->y);
}

static void
go_path_svg_curve_to (struct PathSvgClosure *closure,
                         GOPathPoint const *point0,
                         GOPathPoint const *point1,
                         GOPathPoint const *point2)
{
	if (closure->last_op != 'C') {
		g_string_append (closure->str, " C");
		closure->last_op = 'C';
	}
	g_string_append_printf (closure->str, " %g %g %g %g %g %g",
	                        point0->x, point0->y,
	                        point1->x, point1->y,
	                        point2->x, point2->y);
}

static void
go_path_svg_close (struct PathSvgClosure *closure)
{
	g_string_append (closure->str, " Z");
	closure->last_op = 'Z';
}

/**
 * go_path_to_svg:
 * @path: a #GOPath
 *
 * Builds an svg path from @path.
 * Returns: (transfer full): the svg:d string.
 **/
char *
go_path_to_svg (GOPath *path)
{
	struct PathSvgClosure closure;
	closure.str = g_string_new ("");
	closure.last_op = 0;
	go_path_interpret (path, GO_PATH_DIRECTION_FORWARD,
	                   (GOPathMoveToFunc) go_path_svg_move_to,
	                   (GOPathLineToFunc) go_path_svg_line_to,
	                   (GOPathCurveToFunc) go_path_svg_curve_to,
	                   (GOPathClosePathFunc) go_path_svg_close, &closure);

	return g_string_free (closure.str, FALSE);
}

/*******************************************************************************
  * Paths from string
 ******************************************************************************/

typedef struct {
	char const *src;
	GOPath *path;
	GHashTable const *variables;
	double lastx, lasty;
	gboolean relative, clockwise, line_to, horiz;
} PathParseState;

static void
skip_spaces (PathParseState *state)
{
	while (*state->src == ' ')
		(state->src)++;
}

static void
skip_comma_and_spaces (PathParseState *state)
{
	while (*state->src == ' ' || *state->src == ',')
		(state->src)++;
}

static gboolean
parse_value (PathParseState *state, double *x)
{
	char const *end, *c;
	gboolean integer_part = FALSE;
	gboolean fractional_part = FALSE;
	gboolean exponent_part = FALSE;
	double mantissa = 0.0;
	double exponent =0.0;
	double divisor;
	gboolean mantissa_sign = 1.0;
	gboolean exponent_sign = 1.0;

	c = state->src;

	if (*c == '?' || *c == '$') {
		char *var;
		double *val;
		state->src++;
		while (*state->src != 0 && *state->src != ' ' && *state->src != ',')
			state->src++;
		var = g_strndup (c, state->src - c);
		if (state->variables == NULL || ((val = g_hash_table_lookup ((GHashTable *) state->variables, var)) == NULL)) {
			g_free (var);
			return FALSE;
		}
		*x = *val;
		g_free (var);
		return TRUE;
	}

	if (*c == '-') {
		mantissa_sign = -1.0;
		c++;
	} else if (*c == '+')
		c++;

	if (*c >= '0' && *c <= '9') {
		integer_part = TRUE;
		mantissa = *c - '0';
		c++;

		while (*c >= '0' && *c <= '9') {
			mantissa = mantissa * 10.0 + *c - '0';
			c++;
		}
	}


	if (*c == '.')
		c++;
	else if (!integer_part)
		return FALSE;

	if (*c >= '0' && *c <= '9') {
		fractional_part = TRUE;
		mantissa += (*c - '0') * 0.1;
		divisor = 0.01;
		c++;

		while (*c >= '0' && *c <= '9') {
			mantissa += (*c - '0') * divisor;
			divisor *= 0.1;
			c++;
		}
	}

	if (!fractional_part && !integer_part)
		return FALSE;

	end = c;

	if (*c == 'E' || *c == 'e') {
		c++;

		if (*c == '-') {
			exponent_sign = -1.0;
			c++;
		} else if (*c == '+')
			c++;

		if (*c >= '0' && *c <= '9') {
			exponent_part = TRUE;
			exponent = *c - '0';
			c++;

			while (*c >= '0' && *c <= '9') {
				exponent = exponent * 10.0 + *c - '0';
				c++;
			}
		}

	}

	if (exponent_part) {
		end = c;
		*x = mantissa_sign * mantissa * pow (10.0, exponent_sign * exponent);
	} else
		*x = mantissa_sign * mantissa;

	state->src = end;

	return TRUE;
}

static gboolean
parse_values (PathParseState *state, unsigned int n_values, double *values)
{
	char const *ptr = state->src;
	unsigned int i;

	skip_comma_and_spaces (state);

	for (i = 0; i < n_values; i++) {
		if (!parse_value (state, &values[i])) {
			state->src = ptr;
			return FALSE;
		}
		skip_comma_and_spaces (state);
	}

	return TRUE;
}

static void
emit_function_2 (PathParseState *state,
                 void (*path_func) (GOPath *, double, double))
{
	double values[2];

	skip_spaces (state);

	while (parse_values (state, 2, values)) {
		if (state->relative) {
			state->lastx += values[0];
			state->lasty += values[1];
		} else {
			state->lastx = values[0];
			state->lasty = values[1];
		}
		path_func (state->path, state->lastx, state->lasty);
	}
}

static void
emit_function_6 (PathParseState *state,
                 void (*path_func) (GOPath *, double, double, double ,double, double, double))
{
	double values[6];

	skip_spaces (state);

	while (parse_values (state, 6, values)) {
		if (state->relative) {
			values[0] += state->lastx;
			values[1] += state->lasty;
			values[2] += state->lastx;
			values[3] += state->lasty;
			state->lastx += values[4];
			state->lasty += values[5];
		} else {
			state->lastx = values[4];
			state->lasty = values[5];
		}
		path_func (state->path, values[0], values[1], values[2], values[3], state->lastx, state->lasty);
	}
}

static void
emit_function_8 (PathParseState *state,
                 void (*path_func) (PathParseState *, double, double, double ,double, double, double, double ,double))
{
	double values[8];

	skip_spaces (state);

	while (parse_values (state, 8, values)) {
		if (state->relative) {
			values[0] += state->lastx;
			values[1] += state->lasty;
			values[2] += state->lastx;
			values[3] += state->lasty;
			values[4] += state->lastx;
			values[5] += state->lasty;
			state->lastx += values[6];
			state->lasty += values[7];
		} else {
			state->lastx = values[6];
			state->lasty = values[7];
		}
		path_func (state, values[0], values[1], values[2], values[3], values[4], values[5], state->lastx, state->lasty);
	}
}

static void
emit_quadratic (PathParseState *state)
{
	double values[4];

	skip_spaces (state);

	while (parse_values (state, 4, values)) {
		if (state->relative) {
			values[0] += state->lastx;
			values[1] += state->lasty;
			values[2] += state->lastx;
			values[3] += state->lasty;
		}
		go_path_curve_to (state->path,
		                  (state->lastx + 2 * values[0]) / 3.,
		                  (state->lasty + 2 * values[1]) / 3.,
		                  (2 * values[0] + values[2]) / 3.,
		                  (2 * values[1] + values[3]) / 3.,
		                  values[2], values[3]);
		state->lastx += values[2];
		state->lasty += values[3];
	}
}

static void
_path_arc (PathParseState *state,
	     double cx, double cy,
	     double rx, double ry,
	     double th0, double th1)
{
	double th_arc, th, th_delta, t;
	double x, y;
	int i, n_segs;

	if (state->line_to)
		go_path_line_to (state->path, cx + rx * cos (th0), cy + ry * sin (th0));
	else
		go_path_move_to (state->path, cx + rx * cos (th0), cy + ry * sin (th0));
	if (state->clockwise) {
		while (th1 < th0)
			th1 += 2 * M_PI;
	} else {
		while (th1 > th0)
			th0 += 2 * M_PI;
	}
	th_arc = th1 - th0;
	n_segs = ceil (fabs (th_arc / (M_PI * 0.5 + 0.001)));
	th_delta = th_arc / n_segs;
	t = (8.0 / 3.0) * sin (th_delta * 0.25) * sin (th_delta * 0.25) / sin (th_delta * 0.5);
	th = th0;
	for (i = 0; i < n_segs; i++) {
		x = cx + rx * cos (th + th_delta);
		y = cy + ry * sin (th + th_delta);
		go_path_curve_to (state->path,
				  cx + rx * (cos (th) - t * sin (th)),
				  cy + ry * (sin (th) + t * cos (th)),
				  x + rx * t * sin (th + th_delta),
				  y - ry * t * cos (th + th_delta),
				  x, y);
		th += th_delta;
	}
	state->lastx = cx + rx * sin (th1);
	state->lasty = cy + ry * sin (th1);
}

static void
go_path_arc_degrees (PathParseState *state)
{
	double values[6];

	skip_spaces (state);

	while (parse_values (state, 6, values)) {
		if (state->relative) {
			values[0] += state->lastx;
			values[1] += state->lasty;
		}
		_path_arc (state, values[0], values[1], values[2], values[3],
		           values[4] * M_PI / 180., values[5] * M_PI / 180.);
	}
}

static void
go_path_arc_full (PathParseState *state, double x1, double y1, double x2, double y2,
                     double x3, double y3, double x4, double y4)
{
	double cx, cy, rx, ry, th0, th1;
	cx = (x1 + x2) / 2.;
	cy = (y1 + y2) / 2;
	rx = MAX (x1, x2) - cx;
	ry = MAX (y1, y2) - cy;
	th0 = atan2 ((y3 - cy) / ry, (x3 - cx) / rx);
	th1 = atan2 ((y4 - cy) / ry, (x4 - cx) / rx);
	if (state->clockwise) {
		/* we need th1 > th0 */
		while (th1 < th0)
			th1 += 2 * M_PI;
	} else {
		/* we need th1 > th0 */
		while (th1 > th0)
			th0 += 2 * M_PI;
	}
	_path_arc (state, cx, cy, rx, ry, th0, th1);
}

static void
go_path_quadrant (PathParseState *state)
{
	double values[2], cx, cy, rx, ry, th0, th1;

	skip_spaces (state);

	while (parse_values (state, 2, values)) {
		if (state->relative) {
			values[0] += state->lastx;
			values[1] += state->lasty;
		}
		/* evaluating center and radius */
		if (state->horiz) {
			cx = state->lastx;
			cy = values[1];
			rx = fabs (values[0] - cx);
			ry = fabs (state->lasty - cy);
		} else {
			cx = values[0];
			cy = state->lasty;
			rx = fabs (state->lastx - cx);
			ry = fabs (values[1] - cy);
		}
		/* now the angles */
		th0 = atan2 (state->lasty - cy, state->lastx - cx);
		th1 = atan2 (values[1] - cy, values[0] - cx);
		/* and finally the direction */
		state->clockwise = (values[1] - cy) * (state->lastx - cx) - (state->lasty - cy) * (values[0] - cx) > 0.;
		_path_arc (state, cx, cy, rx, ry, th0, th1);

		state->lastx = values[0];
		state->lasty = values[1];
		state->horiz = !state->horiz;
	}
}

/**
 * go_path_new_from_svg:
 * @src: (nullable): an SVG path.
 *
 * Returns: (transfer full) (nullable): the newly allocated #GOPath.
 **/
GOPath *
go_path_new_from_svg (char const *src)
{
	PathParseState state;

	if (src == NULL)
		return NULL;

	state.path = go_path_new ();
	state.src = (char *) src;
	state.lastx = state.lasty = 0.;
	state.variables = NULL;

	skip_spaces (&state);

	while (*state.src != '\0') {
		switch (*state.src) {
		case 'A':
			state.src++;
			state.relative = FALSE;
			break;
		case 'a':
			state.src++;
			state.relative = TRUE;
			break;
		case 'M':
			state.src++;
			state.relative = FALSE;
			emit_function_2 (&state, go_path_move_to);
			break;
		case 'm':
			state.src++;
			state.relative = TRUE;
			emit_function_2 (&state, go_path_move_to);
			break;
		case 'H':
			state.src++;
			state.relative = FALSE;
			break;
		case 'h':
			state.src++;
			state.relative = TRUE;
			break;
		case 'L':
			state.src++;
			state.relative = FALSE;
			emit_function_2 (&state, go_path_line_to);
			break;
		case 'l':
			state.src++;
			state.relative = TRUE;
			emit_function_2 (&state, go_path_line_to);
			break;
		case 'C':
			state.src++;
			state.relative = FALSE;
			emit_function_6 (&state, go_path_curve_to);
			break;
		case 'c':
			state.src++;
			state.relative = TRUE;
			emit_function_6 (&state, go_path_curve_to);
			break;
		case 'Q':
			state.src++;
			state.relative = FALSE;
			emit_quadratic (&state);
			break;
		case 'q':
			state.src++;
			state.relative = TRUE;
			emit_quadratic (&state);
			break;
		case 'S':
			state.src++;
			state.relative = FALSE;
			break;
		case 's':
			state.src++;
			state.relative = TRUE;
			break;
		case 'T':
			state.src++;
			state.relative = FALSE;
			break;
		case 't':
			state.src++;
			state.relative = TRUE;
			break;
		case 'V':
			state.src++;
			state.relative = FALSE;
			break;
		case 'v':
			state.src++;
			state.relative = TRUE;
			break;
		case 'Z':
		case 'z':
			state.src++;
			go_path_close (state.path);
			break;
		default:
			go_path_free (state.path);
			return NULL;
		}
		skip_spaces (&state);
	}
	return state.path;
}

/**
 * go_path_new_from_odf_enhanced_path:
 * @src: (nullable): an ODF enhanced path.
 * @variables: environment
 *
 * Returns: (transfer full) (nullable): the newly allocated #GOPath
 * or %NULL on error.
 **/
GOPath *
go_path_new_from_odf_enhanced_path (char const *src, GHashTable const *variables)
{
	PathParseState state;

	if (src == NULL)
		return NULL;

	state.path = go_path_new ();
	state.src = (char *) src;
	state.lastx = state.lasty = 0.;
	state.variables = variables;
	state.relative = FALSE;

	skip_spaces (&state);

	while (*state.src != '\0') {
		switch (*state.src) {
		case 'A':
			state.src++;
			state.clockwise = FALSE;
			state.line_to = TRUE;
			emit_function_8 (&state, go_path_arc_full);
			break;
		case 'B':
			state.src++;
			state.clockwise = FALSE;
			state.line_to = FALSE;
			emit_function_8 (&state, go_path_arc_full);
			break;
		case 'C':
			state.src++;
			emit_function_6 (&state, go_path_curve_to);
			break;
		case 'F':
			state.src++; /* ignore */
			break;
		case 'L':
			state.src++;
			emit_function_2 (&state, go_path_line_to);
			break;
		case 'M':
			state.src++;
			emit_function_2 (&state, go_path_move_to);
			break;
		case 'N': /* new sub path, just ignore */
			state.src++;
			break;
		case 'Q':
			state.src++;
			emit_quadratic (&state);
			break;
		case 'S':
			state.src++; /* ignore */
			break;
		case 'T':
			state.src++;
			state.clockwise = TRUE;
			state.line_to = TRUE;
			go_path_arc_degrees (&state);
			break;
		case 'U':
			state.src++;
			state.clockwise = TRUE;
			state.line_to = FALSE;
			go_path_arc_degrees (&state);
			break;
		case 'V':
			state.src++;
			state.clockwise = TRUE;
			state.line_to = FALSE;
			emit_function_8 (&state, go_path_arc_full);
			break;
		case 'W':
			state.src++;
			state.clockwise = TRUE;
			state.line_to = TRUE;
			emit_function_8 (&state, go_path_arc_full);
			break;
		case 'X':
			/* assuming that the horizontal/vertical alternance only applies
			 * when the additional letters are omitted */
			state.src++;
			state.horiz = TRUE;
			go_path_quadrant (&state);
			break;
		case 'Y':
			state.src++;
			state.horiz = FALSE;
			go_path_quadrant (&state);
			break;
		case 'Z':
			state.src++;
			go_path_close (state.path);
			break;
		default:
			go_path_free (state.path);
			return NULL;
		}
		skip_spaces (&state);
	}
	if (state.path->data_buffer_head->n_actions == 0) {
		go_path_free (state.path);
		return NULL;
	}
	return state.path;
}
