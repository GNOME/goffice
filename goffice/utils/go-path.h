/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-path.h
 *
 * Copyright (C) 2006 Emmanuel Pacaud (emmanuel.pacaud@lapp.in2p3.fr)
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

#ifndef GO_PATH_H
#define GO_PATH_H

#include <glib.h>
#include <goffice/graph/gog-style.h>

typedef struct _GOPath 		  GOPath;

#define IS_GO_PATH(x) ((x) != NULL)

GOPath *go_path_new 	      	(void);
GOPath *go_path_new_with_size 	(int size);
void 	go_path_clear	      	(GOPath *path);
void    go_path_free          	(GOPath *path);
void    go_path_set_sharp  	(GOPath *path, gboolean sharp);

/* Pathes */
void go_path_move_to 		(GOPath *path, double x, double y);
void go_path_line_to 		(GOPath *path, double x, double y);
void go_path_curve_to 		(GOPath *path, double x0, double y0, 
			 		       double x1, double y1, 
					       double x2, double y2);
void go_path_close 		(GOPath *path);
void go_path_add_arc 		(GOPath *path, double cx, double cy, 
			 		       double rx, double ry, 
					       double th0, double th1);
void go_path_set_marker_flag 	(GOPath *path);

/* Shapes */
void go_path_add_rectangle  	(GOPath *path, double x, double y, 
			     		       double width, double height);
void go_path_add_ring_wedge 	(GOPath *path, double cx, double xy,
			 	               double rx_out, double ry_out, 
					       double rx_in, double ry_in, 
					       double th0, double th1);
void go_path_add_pie_wedge  	(GOPath *path, double cx, double cy, 
			 	               double rx, double ry, 
					       double th0, double th1);
void go_path_add_vertical_bar	(GOPath *path, double x, double y,
			 		       double width, double height);	 
void go_path_add_horizontal_bar	(GOPath *path, double x, double y,
			 		       double width, double height);	 
void go_path_add_style	    	(GOPath *path, GogStyle const *style);

/* Helper functions */
GOPath *go_path_new_rectangle 	(double x, double y, double width, double height);

#endif 
