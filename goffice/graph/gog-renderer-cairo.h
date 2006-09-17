/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-renderer-cairo.h : 
 *
 * Copyright (C) 2005 Emmanuel Pacaud (emmanuel.pacaud@univ-poitiers.fr)
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
#ifndef GOG_RENDERER_CAIRO_H
#define GOG_RENDERER_CAIRO_H

#include <goffice/graph/goffice-graph.h>

#include <gsf/gsf.h>

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define GOG_RENDERER_CAIRO_TYPE		(gog_renderer_cairo_get_type ())
#define GOG_RENDERER_CAIRO(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_RENDERER_CAIRO_TYPE, GogRendererCairo))
#define IS_GOG_RENDERER_CAIRO(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_RENDERER_CAIRO_TYPE))

typedef struct _GogRendererCairo GogRendererCairo;

GType      gog_renderer_cairo_get_type		(void);
GdkPixbuf *gog_renderer_cairo_get_pixbuf   	(GogRendererCairo *crend);
gboolean   gog_renderer_cairo_update		(GogRendererCairo *crend, int w, int h, double zoom);

G_END_DECLS

#endif /* GOG_RENDERER_CAIRO_H */
