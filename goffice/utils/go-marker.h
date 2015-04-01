/*
 * gog-marker.h
 *
 * Copyright (C) 2003-2007 Emmanuel Pacaud <emmanuel.pacaud@lapp.in2p3.fr>
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

#ifndef GO_MARKER_H
#define GO_MARKER_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GO_TYPE_MARKER	  	(go_marker_get_type ())
#define GO_MARKER(o)		(G_TYPE_CHECK_INSTANCE_CAST((o), GO_TYPE_MARKER, GOMarker))
#define GO_IS_MARKER(o)		(G_TYPE_CHECK_INSTANCE_TYPE((o), GO_TYPE_MARKER))

typedef enum {
	GO_MARKER_NONE,
	GO_MARKER_SQUARE,
	GO_MARKER_DIAMOND,
	GO_MARKER_TRIANGLE_DOWN,
	GO_MARKER_TRIANGLE_UP,
	GO_MARKER_TRIANGLE_RIGHT,
	GO_MARKER_TRIANGLE_LEFT,
	GO_MARKER_CIRCLE,
	GO_MARKER_X,
	GO_MARKER_CROSS,
	GO_MARKER_ASTERISK,
	GO_MARKER_BAR,
	GO_MARKER_HALF_BAR,
	GO_MARKER_BUTTERFLY,
	GO_MARKER_HOURGLASS,
	GO_MARKER_LEFT_HALF_BAR,
	GO_MARKER_MAX
} GOMarkerShape;

GType go_marker_get_type (void);

GOMarkerShape    go_marker_shape_from_str       (char const *name);
char const      *go_marker_shape_as_str         (GOMarkerShape shape);
GOMarkerShape 	 go_marker_get_shape		(GOMarker const *m);
void 		 go_marker_set_shape 		(GOMarker *m, GOMarkerShape shape);
gboolean	 go_marker_is_closed_shape		(GOMarker const *m);
GOColor 	 go_marker_get_outline_color	(GOMarker const *m);
void		 go_marker_set_outline_color	(GOMarker *m, GOColor color);
GOColor		 go_marker_get_fill_color	(GOMarker const *m);
void		 go_marker_set_fill_color	(GOMarker *m, GOColor color);
int		 go_marker_get_size		(GOMarker const *m);
void		 go_marker_set_size		(GOMarker *m, int size);
double		 go_marker_get_outline_width	(GOMarker const *m);

void		 go_marker_assign 		(GOMarker *dst, GOMarker const *src);
GOMarker *	 go_marker_dup 			(GOMarker const *src);
GOMarker * 	 go_marker_new 			(void);

void 		 go_marker_render 		(GOMarker const *marker, cairo_t *cr,
						 double x, double y, double scale);
cairo_surface_t *go_marker_create_cairo_surface (GOMarker const *marker, cairo_t *cr, double scale,
						 double *width, double *height);

G_END_DECLS

#endif /* GO_MARKER_H */
