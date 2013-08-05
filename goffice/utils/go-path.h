/*
 * go-path.h
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

#ifndef GO_PATH_H
#define GO_PATH_H

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	GO_PATH_DIRECTION_FORWARD,
	GO_PATH_DIRECTION_BACKWARD
} GOPathDirection;

typedef enum {
	GO_PATH_OPTIONS_SNAP_COORDINATES 	= 1<<0,
	GO_PATH_OPTIONS_SNAP_WIDTH		= 1<<1,
	GO_PATH_OPTIONS_SHARP			= 3
} GOPathOptions;

typedef struct {
	double x;
	double y;
} GOPathPoint;

typedef void (*GOPathMoveToFunc) 	(void *closure, GOPathPoint const *point);
typedef void (*GOPathLineToFunc) 	(void *closure, GOPathPoint const *point);
typedef void (*GOPathCurveToFunc) 	(void *closure, GOPathPoint const *point0,
					 		GOPathPoint const *point1,
							GOPathPoint const *point2);
typedef void (*GOPathClosePathFunc) 	(void *closure);

GType   go_path_get_type (void);

#define GO_TYPE_PATH go_path_get_type ()
#define GO_IS_PATH(x) ((x) != NULL)

GOPath *go_path_new 	      	(void);
GOPath *go_path_new_from_svg    (char const *src);
GOPath *go_path_new_from_odf_enhanced_path (char const *src, GHashTable const *variables);
void 	go_path_clear	      	(GOPath *path);
GOPath *go_path_ref          	(GOPath *path);
void    go_path_free          	(GOPath *path);

void    	go_path_set_options  	(GOPath *path, GOPathOptions options);
GOPathOptions   go_path_get_options  	(GOPath const *path);

void 	go_path_move_to 	(GOPath *path, double x, double y);
void 	go_path_line_to 	(GOPath *path, double x, double y);
void 	go_path_curve_to 	(GOPath *path, double x0, double y0,
				 	       double x1, double y1,
				 	       double x2, double y2);
void 	go_path_close 		(GOPath *path);

void 	go_path_ring_wedge 	(GOPath *path, double cx, double cy,
				 	       double rx_out, double ry_out,
					       double rx_in, double ry_in,
					       double th0, double th1);
void    go_path_pie_wedge 	(GOPath *path, double cx, double cy,
				 	       double rx, double ry,
				 	       double th0, double th1);
void    go_path_arc	 	(GOPath *path,
				 double cx, double cy,
				 double rx, double ry,
				 double th0, double th1);
void    go_path_arc_to	 	(GOPath *path,
				 double cx, double cy,
				 double rx, double ry,
				 double th0, double th1);
void	go_path_rectangle	(GOPath *path, double x, double y,
				 double width, double height);

void	go_path_interpret	(GOPath const *path,
				 GOPathDirection direction,
				 GOPathMoveToFunc move_to,
				 GOPathLineToFunc line_to,
				 GOPathCurveToFunc curve_to,
				 GOPathClosePathFunc close_path,
				 void *closure);
void	go_path_interpret_full (GOPath const *path,
	             gssize start, gssize end,
				 GOPathDirection direction,
				 GOPathMoveToFunc move_to,
				 GOPathLineToFunc line_to,
				 GOPathCurveToFunc curve_to,
				 GOPathClosePathFunc close_path,
				 void *closure);

void    go_path_to_cairo	(GOPath const *path,
				 GOPathDirection direction,
				 cairo_t *cr);

GOPath *go_path_copy		(GOPath const *path);
GOPath *go_path_copy_restricted	(GOPath const *path, gssize start, gssize end);
GOPath *go_path_append		(GOPath *path1, GOPath const *path2);
GOPath *go_path_scale       (GOPath *path, double scale_x, double scale_y);
char   *go_path_to_svg      (GOPath *path);

G_END_DECLS

#endif
