/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- *//* vim: set sw=8: */
#ifndef GOD_DRAWING_RENDERER_CAIRO_H
#define GOD_DRAWING_RENDERER_CAIRO_H

/*
 * god-drawing-renderer-cairo.h: MS Office Graphic Object support
 *
 * Author:
 *    Michael Meeks (michael@ximian.com)
 *    Jody Goldberg (jody@gnome.org)
 *    Christopher James Lahey <clahey@ximian.com>
 *
 * (C) 1998-2003 Michael Meeks, Jody Goldberg, Chris Lahey
 */

#include <goffice/drawing/god-drawing.h>
#include <cairo.h>

G_BEGIN_DECLS

void                     god_drawing_renderer_cairo_render       (GodDrawing   *drawing,
								  cairo_t      *context);

G_END_DECLS

#endif /* GOD_DRAWING_RENDERER_CAIRO_H */
