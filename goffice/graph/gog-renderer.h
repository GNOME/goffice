/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-renderer.h : An abstract interface for rendering engines
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#ifndef GOG_RENDERER_H
#define GOG_RENDERER_H

#include <goffice/graph/goffice-graph.h>
#include <goffice/utils/go-geometry.h>
#include <goffice/utils/go-image.h>
#include <glib-object.h>

#include <gsf/gsf.h>
#include <libart_lgpl/libart.h>

#if 1 /* def GOFFICE_WITH_GTK breaks without goffice-config.h included */
#include <gtk/gtkenums.h>
#include <goffice/gtk/goffice-gtk.h>
#include <gdk/gdk.h>
#else
/* VILE HACK VILE HACK VILE HACK */
typedef struct HACK_GdkPixbuf GdkPixbuf;
typedef enum
{
  GTK_ANCHOR_CENTER,
  GTK_ANCHOR_NORTH,
  GTK_ANCHOR_NORTH_WEST,
  GTK_ANCHOR_NORTH_EAST,
  GTK_ANCHOR_SOUTH,
  GTK_ANCHOR_SOUTH_WEST,
  GTK_ANCHOR_SOUTH_EAST,
  GTK_ANCHOR_WEST,
  GTK_ANCHOR_EAST,
  GTK_ANCHOR_N		= GTK_ANCHOR_NORTH,
  GTK_ANCHOR_NW		= GTK_ANCHOR_NORTH_WEST,
  GTK_ANCHOR_NE		= GTK_ANCHOR_NORTH_EAST,
  GTK_ANCHOR_S		= GTK_ANCHOR_SOUTH,
  GTK_ANCHOR_SW		= GTK_ANCHOR_SOUTH_WEST,
  GTK_ANCHOR_SE		= GTK_ANCHOR_SOUTH_EAST,
  GTK_ANCHOR_W		= GTK_ANCHOR_WEST,
  GTK_ANCHOR_E		= GTK_ANCHOR_EAST
} GtkAnchorType;
/* VILE HACK VILE HACK VILE HACK */
#endif

#include <cairo.h>
#ifdef CAIRO_HAS_SVG_SURFACE
#define GOG_RENDERER_CAIRO_WITH_SVG
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
#define GOG_RENDERER_CAIRO_WITH_PDF
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#define GOG_RENDERER_CAIRO_WITH_PS
#endif

G_BEGIN_DECLS

/* We need to define an hair line width for the svg and gnome_print renderer. 
 * 0.5 pt is approx. the dot size of a 150 dpi printer, if the plot is 
 * printed at scale 1:1 */
#define GOG_RENDERER_HAIR_LINE_WIDTH	0.5

#define GOG_RENDERER_GRIP_SIZE	4

#define GOG_RENDERER_TYPE	  (gog_renderer_get_type ())
#define GOG_RENDERER(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), GOG_RENDERER_TYPE, GogRenderer))
#define IS_GOG_RENDERER(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), GOG_RENDERER_TYPE))
#define GOG_RENDERER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GOG_RENDERER_TYPE, GogRendererClass))

GType gog_renderer_get_type            (void); 

void  gog_renderer_request_update (GogRenderer *r);

void  gog_renderer_push_style     	(GogRenderer *rend, GogStyle const *style);
void  gog_renderer_push_selection_style (GogRenderer *renderer);
void  gog_renderer_pop_style      	(GogRenderer *rend);

void  gog_renderer_push_clip 	  (GogRenderer *rend, ArtVpath *clip_path);
void  gog_renderer_pop_clip       (GogRenderer *rend);

ArtVpath * gog_renderer_get_rectangle_vpath 	(GogViewAllocation const *rect);
ArtBpath * gog_renderer_get_ring_wedge_bpath	(double cx, double cy, 
						 double rx_out, double ry_out,
						 double rx_in, double ry_in,
						 double th0, double th1);

void  gog_renderer_draw_sharp_path	(GogRenderer *rend, ArtVpath *path);
void  gog_renderer_draw_sharp_polygon   (GogRenderer *rend, ArtVpath *path, gboolean narrow);
void  gog_renderer_draw_sharp_rectangle (GogRenderer *r, GogViewAllocation const *rect);

void  gog_renderer_draw_ring_wedge  	(GogRenderer *rend, double cx, double cy,
					 double rx_out, double ry_out, 
					 double rx_in, double ry_in,
					 double th0, double th1,
					 gboolean narrow);
void  gog_renderer_draw_path      	(GogRenderer *rend, ArtVpath const *path);
void  gog_renderer_draw_polygon   	(GogRenderer *rend, ArtVpath const *path, gboolean narrow);
void  gog_renderer_draw_rectangle 	(GogRenderer *rend, GogViewAllocation const *rect);
void  gog_renderer_draw_bezier_path     (GogRenderer *rend, ArtBpath const *path);

void  gog_renderer_draw_text	  (GogRenderer *rend, char const *text,
				   GogViewAllocation const *pos, GtkAnchorType anchor,
				   gboolean use_markup);
void  gog_renderer_draw_marker	  (GogRenderer *rend, double x, double y);
void  gog_renderer_draw_grip	  (GogRenderer *renderer, double x, double y);

#define gog_renderer_in_grip(x,y,grip_x,grip_y) ((x) >= ((grip_x) - (GOG_RENDERER_GRIP_SIZE)) && \
						 (x) <= ((grip_x) + (GOG_RENDERER_GRIP_SIZE)) && \
						 (y) >= ((grip_y) - (GOG_RENDERER_GRIP_SIZE)) && \
						 (y) <= ((grip_y) + (GOG_RENDERER_GRIP_SIZE)))

void  gog_renderer_get_text_OBR   (GogRenderer *rend, char const *text, 
				   gboolean use_markup, GOGeometryOBR *obr);
void  gog_renderer_get_text_AABR  (GogRenderer *rend, char const *text, 
				   gboolean use_markup, GOGeometryAABR *aabr);

#define gog_renderer_draw_arc(r,cx,cy,rx,ry,th0,th1) \
	gog_renderer_draw_ring_wedge (r,cx,cy,rx,ry,-1.,-1.,th0,th1,FALSE)
#define gog_renderer_draw_pie_wedge(r,cx,cy,rx,ry,th0,th1,narrow) \
	gog_renderer_draw_ring_wedge (r,cx,cy,rx,ry,0.,0.,th0,th1,narrow)

/* measurement */
double gog_renderer_line_size	  	(GogRenderer const *r, double width);
double gog_renderer_pt2r_x   	  	(GogRenderer const *r, double d);
double gog_renderer_pt2r_y   	  	(GogRenderer const *r, double d);
double gog_renderer_pt2r   	  	(GogRenderer const *r, double d);

/* utilities for cairo/libart transition */
GogRenderer 	*gog_renderer_new_for_pixbuf 	(GogGraph *graph);
gboolean	 gog_renderer_update		(GogRenderer *renderer, double w, double h, double zoom);
GdkPixbuf 	*gog_renderer_get_pixbuf 	(GogRenderer *renderer); 

GogRenderer 	*gog_renderer_new_for_format 	(GogGraph *graph, GOImageFormat format);
gboolean 	 gog_renderer_export_image 	(GogRenderer *renderer, GOImageFormat format,
						 GsfOutput *output, double x_dpi, double y_dpi);

G_END_DECLS

#endif /* GOG_RENDERER_H */
