/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-path-impl.h
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

#ifndef GO_PATH_IMPL_H
#define GO_PATH_IMPL_H

#include <goffice/utils/go-path.h>

typedef enum _GOPathAction {
	GO_PATH_ACTION_MOVE_TO 		= 0,
	GO_PATH_ACTION_LINE_TO 		= 1,
	GO_PATH_ACTION_CURVE_TO 	= 2,
	GO_PATH_ACTION_CLOSE_PATH 	= 3
} GOPathAction; 

typedef struct _GOPathData	 GOPathData;
typedef struct _GOPathDataBuffer GOPathDataBuffer;

struct _GOPathData {
	GOPathAction action;
	GOPoint point;
};

struct _GOPathDataBuffer {
	int max_data;
	int num_data;

	GOPathData *data;

	struct _GOPathDataBuffer *next;
};

struct _GOPath {
	GOPathDataBuffer *data_buffer_head;
	GOPathDataBuffer *data_buffer_tail;

	gboolean sharp;
};

#endif
