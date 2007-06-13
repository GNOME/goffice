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

#include <goffice/utils/go-path-impl.h>

#define GO_PATH_DEFAULT_BUFFER_SIZE 64

static void
go_path_data_buffer_free (GOPathDataBuffer *buffer)
{
	g_message ("[GOPathDataBuffer::free] %p", buffer);
	g_free (buffer->data);
	g_free (buffer);
}

static void
go_path_data_buffer_clear (GOPathDataBuffer *buffer)
{
	g_message ("[GOPathDataBuffer::clear] %p", buffer);
	buffer->num_data = 0;
}

static GOPathDataBuffer *
go_path_data_buffer_new (int size) 
{
	GOPathDataBuffer *buffer;
	
	if (size <= 0)
		size = GO_PATH_DEFAULT_BUFFER_SIZE;
	
	buffer = g_new (GOPathDataBuffer, 1);
	if (buffer == NULL) {
		g_warning ("[GOPathDataBuffer::new] can't create data buffer");
		return NULL;
	}

	buffer->max_data = size;
	buffer->num_data = 0;
	buffer->data = g_new (GOPathData, size);
	if (buffer->data == NULL) {
		g_warning ("[GOPathDataBuffer::new] can't create data buffer");
		g_free (buffer);
		return NULL;
	}
	buffer->next = NULL;

	g_message ("[GOPathDataBuffer::new] %p, size = %d", buffer, size);

	return buffer;
}

static GOPathDataBuffer *
go_path_add_data_buffer (GOPath *path, int size)
{
	GOPathDataBuffer *buffer;

	buffer = go_path_data_buffer_new (size);
	if (buffer == NULL)
		return NULL;
	
	if (path->data_buffer_head == NULL) {
		path->data_buffer_head = buffer;
		path->data_buffer_tail = buffer;
		return buffer;
	}
	path->data_buffer_tail->next = buffer;
	path->data_buffer_tail = buffer;

	return buffer;
}

GOPath *
go_path_new_with_size (int size)
{
	GOPath *path;
	
	path = g_new (GOPath, 1);
	if (path == NULL) {
		g_warning ("[GOPath::new_with_size] can't create path");
		return NULL;
	}
	path->data_buffer_tail = NULL;
	path->data_buffer_head = NULL;
	path->sharp = FALSE;

	g_message ("[GOPath::new] %p", path);
	
	if (go_path_add_data_buffer (path, size) == NULL) {
		g_free (path);
		return NULL;
	}
	
	return path;
}

GOPath *
go_path_new (void)
{
	return go_path_new_with_size (0);
}

void
go_path_clear (GOPath *path)
{
	GOPathDataBuffer *buffer;
	
	g_return_if_fail (path != NULL);

	g_message ("[GOPath::free] %p", path);

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
 * Frees all memory allocated for @path.
 **/

void
go_path_free (GOPath *path)
{
	GOPathDataBuffer *buffer;
	
	g_return_if_fail (path != NULL);

	g_message ("[GOPath::free] %p", path);
	
	while (path->data_buffer_head != NULL) {
		buffer = path->data_buffer_head->next;
		go_path_data_buffer_free (path->data_buffer_head);
		path->data_buffer_head = buffer;
	}
	g_free (path);
}

/**
 * go_path_set_sharp:
 * @path: a #GOPath
 * @sharp: TRUE for trying to keep line sharp
 *
 * Sets the sharp flag for all elements of @path. If @sharp
 * is TRUE, rendering will try to keep line sharp, for example
 * by aligning straight 1 pixel width lines on half pixels.
 **/

void
go_path_set_sharp (GOPath *path, gboolean sharp)
{
	g_return_if_fail (path != NULL);

	path->sharp = sharp;
}

static void
go_path_add_data (GOPath *path, GOPathData const *data, int num_data)
{
	GOPathDataBuffer *buffer = path->data_buffer_tail;
	int i,j;
	
	g_return_if_fail (path != NULL);

	if (buffer == NULL ||
	    buffer->num_data + num_data > buffer->max_data) 
		buffer = go_path_add_data_buffer (path, GO_PATH_DEFAULT_BUFFER_SIZE);
	if (buffer == NULL)
		return;

	for (i = buffer->num_data, j = 0; i < buffer->num_data + num_data; i++, j++) 
		buffer->data[i] = data[j];
	buffer->num_data += num_data;
}

void
go_path_move_to (GOPath *path, double x, double y)
{
	GOPathData data;

	data.point.x = x;
	data.point.y = y;
	data.action = GO_PATH_ACTION_MOVE_TO;
	go_path_add_data (path, &data, 1);
}

void
go_path_line_to (GOPath *path, double x, double y)
{
	GOPathData data;

	data.point.x = x;
	data.point.y = y;
	data.action = GO_PATH_ACTION_LINE_TO;
	go_path_add_data (path, &data, 1);
}

void
go_path_curve_to (GOPath *path, double x0, double y0, double x1, double y1, double x2, double y2)
{
	GOPathData data[3];

	data[0].point.x = x0;
	data[0].point.y = y0;
	data[0].action = GO_PATH_ACTION_CURVE_TO;
	data[1].point.x = x0;
	data[1].point.y = y0;
	data[1].action = GO_PATH_ACTION_CURVE_TO;
	data[2].point.x = x0;
	data[2].point.y = y0;
	data[2].action = GO_PATH_ACTION_CURVE_TO;
	go_path_add_data (path, data, 3);
}

void
go_path_close (GOPath *path)
{
	GOPathData data;

	data.action = GO_PATH_ACTION_CLOSE_PATH;
	go_path_add_data (path, &data, 1);
}

void
go_path_add_arc (GOPath *path, 
		 double cx, double cy, 
		 double rx, double ry, 
		 double th0, double th1)
{
	GOPathData data[3];

	data[0].point.x = cx;
	data[0].point.y = cy;
	data[0].action = GO_PATH_ACTION_ARC;
	data[1].point.x = rx;
	data[1].point.y = ry;
	data[1].action = GO_PATH_ACTION_ARC;
	data[2].point.x = th0;
	data[2].point.y = th1;
	data[2].action = GO_PATH_ACTION_ARC;
	go_path_add_data (path, data, 3);
}

/**
 * go_path_set_marker_flag:
 * @path: a #GOPath
 *
 * Sets the marker flag for the current point.
 **/

void
go_path_set_marker_flag (GOPath *path)
{
	GOPathDataBuffer *buffer = path->data_buffer_tail;

	g_return_if_fail (path != NULL);

	if (buffer != NULL &&
	    buffer->num_data > 0)
		buffer->data[buffer->num_data - 1].action |= GO_PATH_FLAG_MARKER;
}

void
go_path_add_rectangle (GOPath *path, double x, double y, double width, double height) 
{
	GOPathData data[2];

	data[0].point.x = x;
	data[0].point.y = y;
	data[0].action = GO_PATH_ACTION_RECTANGLE;
	data[1].point.x = width;
	data[1].point.y = height;
	data[1].action = GO_PATH_ACTION_RECTANGLE;
	go_path_add_data (path, data, 2);
}

void 
go_path_add_ring_wedge (GOPath *path, 
			double cx, double cy,
			double rx_out, double ry_out, 
			double rx_in, double ry_in, 
			double th0, double th1)
{
	GOPathData data[4];

	data[0].point.x = cx;
	data[0].point.y = cy;
	data[0].action = GO_PATH_ACTION_RING_WEDGE;
	data[1].point.x = rx_out;
	data[1].point.y = ry_out;
	data[1].action = GO_PATH_ACTION_RING_WEDGE;
	data[2].point.x = rx_in;
	data[2].point.y = ry_in;
	data[2].action = GO_PATH_ACTION_RING_WEDGE;
	data[3].point.x = th0;
	data[3].point.y = th1;
	data[3].action = GO_PATH_ACTION_RING_WEDGE;
	go_path_add_data (path, data, 4);
}

void 
go_path_add_pie_wedge (GOPath *path, 
		       double cx, double cy, 
		       double rx, double ry, 
		       double th0, double th1)
{
	GOPathData data[3];

	data[0].point.x = cx;
	data[0].point.y = cy;
	data[0].action = GO_PATH_ACTION_PIE_WEDGE;
	data[1].point.x = rx;
	data[1].point.y = ry;
	data[1].action = GO_PATH_ACTION_PIE_WEDGE;
	data[2].point.x = th0;
	data[2].point.y = th1;
	data[2].action = GO_PATH_ACTION_PIE_WEDGE;
	go_path_add_data (path, data, 3);
}

void 
go_path_add_style (GOPath *path, GogStyle const *style)
{
	GOPathData data;

	data.action = GO_PATH_ACTION_STYLE;
	data.style = style;
	
	go_path_add_data (path, &data, 1);
}

GOPath *
go_path_new_rectangle (double x, double y, double width, double height)
{
	path = go_path_new_with_size (2);
	
	go_path_set_sharp (path, TRUE);
	go_path_add_rectangle (path, x, y, width, height);
	return path;
}
