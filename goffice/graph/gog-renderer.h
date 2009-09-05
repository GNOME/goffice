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

#include <goffice/goffice.h>

#include <gsf/gsf.h>

#ifdef GOFFICE_WITH_GTK
#include <gdk/gdk.h>
#else
typedef struct _GdkPixbuf GdkPixbuf;
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
#endif

#ifdef GOFFICE_WITH_LASEM
#include <lsmmathmlview.h>
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
#define GOG_RENDERER_HAIRLINE_WIDTH_PTS	0.5

#define GOG_RENDERER_GRIP_SIZE	4

#define GOG_TYPE_RENDERER	  (gog_renderer_get_type ())
#define GOG_RENDERER(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), GOG_TYPE_RENDERER, GogRenderer))
#define GOG_IS_RENDERER(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), GOG_TYPE_RENDERER))
#define GOG_RENDERER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), GOG_TYPE_RENDERER, GogRendererClass))

GType gog_renderer_get_type            (void);

/* measurement */
double gog_renderer_line_size	  	(GogRenderer const *r, double width);
double gog_renderer_pt2r_x   	  	(GogRenderer const *r, double d);
double gog_renderer_pt2r_y   	  	(GogRenderer const *r, double d);
double gog_renderer_pt2r   	  	(GogRenderer const *r, double d);

double gog_renderer_get_hairline_width_pts	(GogRenderer const *rend);

void  gog_renderer_stroke_serie    	(GogRenderer *renderer, GOPath const *path);
void  gog_renderer_fill_serie		(GogRenderer *renderer, GOPath const *path, GOPath const *close_path);

void  gog_renderer_draw_shape 		(GogRenderer *renderer, GOPath const *path);
void  gog_renderer_stroke_shape		(GogRenderer *renderer, GOPath const *path);
void  gog_renderer_fill_shape 		(GogRenderer *renderer, GOPath const *path);

void  gog_renderer_draw_circle		(GogRenderer *rend, double x, double y, double r);
void  gog_renderer_stroke_circle 	(GogRenderer *rend, double x, double y, double r);
void  gog_renderer_fill_circle	 	(GogRenderer *rend, double x, double y, double r);

void  gog_renderer_draw_rectangle	(GogRenderer *rend, GogViewAllocation const *rect);
void  gog_renderer_stroke_rectangle 	(GogRenderer *rend, GogViewAllocation const *rect);
void  gog_renderer_fill_rectangle 	(GogRenderer *rend, GogViewAllocation const *rect);

void  gog_renderer_draw_grip	  		(GogRenderer *renderer, double x, double y);
void  gog_renderer_draw_selection_rectangle	(GogRenderer *renderer, GogViewAllocation const *rectangle);

#define gog_renderer_in_grip(x,y,grip_x,grip_y) ((x) >= ((grip_x) - (GOG_RENDERER_GRIP_SIZE)) && \
						 (x) <= ((grip_x) + (GOG_RENDERER_GRIP_SIZE)) && \
						 (y) >= ((grip_y) - (GOG_RENDERER_GRIP_SIZE)) && \
						 (y) <= ((grip_y) + (GOG_RENDERER_GRIP_SIZE)))

void  gog_renderer_draw_marker	  (GogRenderer *rend, double x, double y);

void  gog_renderer_draw_text	  (GogRenderer *rend, char const *text,
				   GogViewAllocation const *pos, GtkAnchorType anchor,
				   gboolean use_markup);

void  gog_renderer_get_text_OBR   (GogRenderer *rend, char const *text,
				   gboolean use_markup, GOGeometryOBR *obr);
void  gog_renderer_get_text_AABR  (GogRenderer *rend, char const *text,
				   gboolean use_markup, GOGeometryAABR *aabr);

void  gog_renderer_push_style     	(GogRenderer *rend, GOStyle const *style);
void  gog_renderer_pop_style      	(GogRenderer *rend);

void  gog_renderer_push_clip 	  	(GogRenderer *rend, GOPath const *path);
void  gog_renderer_push_clip_rectangle 	(GogRenderer *rend, double x, double y, double w, double h);
void  gog_renderer_pop_clip       	(GogRenderer *rend);


/* Rendering with image cache */
gboolean	 gog_renderer_update		(GogRenderer *renderer, double w, double h);
cairo_surface_t *gog_renderer_get_cairo_surface	(GogRenderer *renderer);
GdkPixbuf 	*gog_renderer_get_pixbuf 	(GogRenderer *renderer);
void  		 gog_renderer_request_update 	(GogRenderer *renderer);

/* One time rendering */
gboolean	 gog_renderer_render_to_cairo 	(GogRenderer *renderer, cairo_t *cairo,
						 double width, double height);
gboolean 	 gog_renderer_export_image 	(GogRenderer *renderer, GOImageFormat format,
						 GsfOutput *output, double x_dpi, double y_dpi);

GogRenderer 	*gog_renderer_new 		(GogGraph *graph);

#ifdef GOFFICE_WITH_LASEM
void		 gog_renderer_draw_equation	(GogRenderer *renderer, LsmMathmlView *mathml_view,
						 double x, double y);
#endif

G_END_DECLS

#endif /* GOG_RENDERER_H */
