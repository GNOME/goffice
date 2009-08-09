/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-canvas-impl.h :
 *
 * Copyright (C) 2008-2009 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifndef GOC_CANVAS_IMPL_H
#define GOC_CANVAS_IMPL_H

#include <goffice/goffice.h>

struct _GocCanvas {
	GtkLayout base;
	double scroll_x1, scroll_y1;
	double pixels_per_unit;
	double width, height;
	int wwidth, wheight;
	GocGroup *root;
	GocItem *grabbed_item;
	GocItem	*last_item;
	GODoc *document;
	GdkEvent *cur_event;
};

typedef struct {
	GtkLayoutClass parent;
	
} GocCanvasClass;

#endif  /* GOC_CANVAS_IMPL_H */
