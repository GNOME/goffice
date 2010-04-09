/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goffice-canvas.h :
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

#ifndef GOFFICE_CANVAS_H
#define GOFFICE_CANVAS_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _GocCanvas	GocCanvas;
typedef struct _GocItem		GocItem;
typedef struct _GocGroup	GocGroup;
typedef struct _GocArc		GocArc;
typedef struct _GocLine		GocLine;
typedef struct _GocPath		GocPath;
typedef struct _GocPixbuf	GocPixbuf;
typedef struct _GocPolyline	GocPolyline;
typedef struct _GocPolygon	GocPolygon;
typedef struct _GocRectangle	GocRectangle;
typedef struct _GocCircle	GocCircle;
typedef struct _GocEllipse	GocEllipse;
typedef struct _GocStyledItem	GocStyledItem;
typedef struct _GocText		GocText;
typedef struct _GocWidget	GocWidget;
typedef struct _GocGraph	GocGraph;

G_END_DECLS

#include <goffice/goffice.h>

#include <goffice/canvas/goc-canvas.h>
#include <goffice/canvas/goc-item.h>
#include <goffice/canvas/goc-styled-item.h>
#include <goffice/canvas/goc-structs.h>
#include <goffice/canvas/goc-utils.h>

#include <goffice/canvas/goc-circle.h>
#include <goffice/canvas/goc-ellipse.h>
#include <goffice/canvas/goc-graph.h>
#include <goffice/canvas/goc-group.h>
#include <goffice/canvas/goc-arc.h>
#include <goffice/canvas/goc-line.h>
#include <goffice/canvas/goc-path.h>
#include <goffice/canvas/goc-pixbuf.h>
#include <goffice/canvas/goc-polyline.h>
#include <goffice/canvas/goc-polygon.h>
#include <goffice/canvas/goc-rectangle.h>
#include <goffice/canvas/goc-text.h>
#include <goffice/canvas/goc-widget.h>

#endif  /* GOFFICE_CANVAS_H */
