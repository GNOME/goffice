/*
 * gog-renderer.h : An abstract interface for rendering engines
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
 */
#ifndef GOG_RENDERER_H
#define GOG_RENDERER_H

#include <goffice/goffice.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef GOFFICE_WITH_LASEM
#include <lsmdomview.h>
#endif

#include <cairo.h>

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
void  gog_renderer_draw_rotated_rectangle (GogRenderer *rend, GogViewAllocation const *rect, gboolean rotate_bg);
void  gog_renderer_stroke_rectangle 	(GogRenderer *rend, GogViewAllocation const *rect);
void  gog_renderer_fill_rectangle 	(GogRenderer *rend, GogViewAllocation const *rect);

void  gog_renderer_draw_grip	  		(GogRenderer *renderer, double x, double y);
void  gog_renderer_draw_selection_rectangle	(GogRenderer *renderer, GogViewAllocation const *rectangle);

#define gog_renderer_in_grip(x,y,grip_x,grip_y) ((x) >= ((grip_x) - (GOG_RENDERER_GRIP_SIZE)) && \
						 (x) <= ((grip_x) + (GOG_RENDERER_GRIP_SIZE)) && \
						 (y) >= ((grip_y) - (GOG_RENDERER_GRIP_SIZE)) && \
						 (y) <= ((grip_y) + (GOG_RENDERER_GRIP_SIZE)))

void  gog_renderer_draw_marker	  (GogRenderer *rend, double x, double y);

typedef enum
{
  GO_JUSTIFY_LEFT,
  GO_JUSTIFY_RIGHT,
  GO_JUSTIFY_CENTER,
  GO_JUSTIFY_FILL
} GoJustification;

void  gog_renderer_draw_text	  (GogRenderer *rend, char const *text,
				   GogViewAllocation const *pos,
				   GOAnchorType anchor,
				   gboolean use_markup,
                                   GoJustification justification, double width);

void  gog_renderer_draw_data_label (GogRenderer *rend, GogSeriesLabelElt const *elt,
                                    GogViewAllocation const *pos, GOAnchorType anchor,
                                    GOStyle *legend_style);

void  gog_renderer_draw_gostring  (GogRenderer *rend,
				   GOString *str,
				   GogViewAllocation const *pos,
				   GOAnchorType anchor,
                                   GoJustification justification, double width);

void  gog_renderer_get_gostring_OBR   (GogRenderer *rend, GOString *str,
				       GOGeometryOBR *obr, double max_width);
void  gog_renderer_get_text_OBR   (GogRenderer *rend, char const *text,
				   gboolean use_markup, GOGeometryOBR *obr,
                                   double max_width);
void gog_renderer_get_gostring_AABR (GogRenderer *rend, GOString *str,
                                     GOGeometryAABR *aabr, double max_width);
void  gog_renderer_get_text_AABR  (GogRenderer *rend, char const *text,
				   gboolean use_markup, GOGeometryAABR *aabr,
                                   double max_width);

void  gog_renderer_draw_color_map	(GogRenderer *rend, GogAxisColorMap const *map,
	                                 int discrete, gboolean horizontal,
	                                 GogViewAllocation const *rect);

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

double		 gog_renderer_get_scale		(GogRenderer *renderer);
#ifdef GOFFICE_WITH_LASEM
void		 gog_renderer_draw_equation	(GogRenderer *renderer, LsmDomView *mathml_view,
						 double x, double y);
#endif

G_END_DECLS

#endif /* GOG_RENDERER_H */
