/*
 * goc-canvas.h :
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_CANVAS_H
#define GOC_CANVAS_H

#include <goffice/goffice.h>
#ifdef GOFFICE_WITH_GTK
#include <gdk/gdk.h>
#endif

G_BEGIN_DECLS

typedef enum {
	GOC_DIRECTION_LTR,
	GOC_DIRECTION_RTL,
	GOC_DIRECTION_MAX
} GocDirection;

struct _GocCanvas {
#ifdef GOFFICE_WITH_GTK
	GtkLayout base;
#else
	GObject base;
#endif
	double scroll_x1, scroll_y1;
	double pixels_per_unit;
	int width, height;
	GocGroup *root;
	GocItem *grabbed_item;
	GocItem	*last_item;
	GODoc *document;
#ifdef GOFFICE_WITH_GTK
	GdkEvent *cur_event;
#endif
	GocDirection direction;
	gpointer priv;
};

#ifdef GOFFICE_WITH_GTK
typedef GtkLayoutClass GocCanvasClass;
#else
typedef GObjectClass GocCanvasClass;
#endif

#define GOC_TYPE_CANVAS	(goc_canvas_get_type ())
#define GOC_CANVAS(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_CANVAS, GocCanvas))
#define GOC_IS_CANVAS(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_CANVAS))

GType goc_canvas_get_type (void);

GocGroup	*goc_canvas_get_root (GocCanvas *canvas);
int		 goc_canvas_get_width (GocCanvas *canvas);
int		 goc_canvas_get_height (GocCanvas *canvas);
void		 goc_canvas_scroll_to (GocCanvas *canvas, double x, double y);
void		 goc_canvas_get_scroll_position (GocCanvas *canvas, double *x, double *y);
void		 goc_canvas_set_pixels_per_unit (GocCanvas *canvas, double pixels_per_unit);
double		 goc_canvas_get_pixels_per_unit (GocCanvas *canvas);
void		 goc_canvas_invalidate (GocCanvas *canvas, double x0, double y0, double x1, double y1);
void		 goc_canvas_invalidate_region (GocCanvas *canvas, GocItem *item, cairo_region_t *region);
gboolean         goc_canvas_get_realized (GocCanvas *canvas);
GocItem		*goc_canvas_get_item_at (GocCanvas *canvas, double x, double y);
void		 goc_canvas_grab_item (GocCanvas *canvas, GocItem *item);
void		 goc_canvas_ungrab_item (GocCanvas *canvas);
GocItem		*goc_canvas_get_grabbed_item (GocCanvas *canvas);
void		 goc_canvas_set_document (GocCanvas *canvas, GODoc *document);
GODoc		*goc_canvas_get_document (GocCanvas *canvas);
#ifdef GOFFICE_WITH_GTK
GdkEvent	*goc_canvas_get_cur_event (GocCanvas *canvas);
#endif
void		 goc_canvas_set_direction (GocCanvas *canvas, GocDirection direction);
GocDirection     goc_canvas_get_direction (GocCanvas *canvas);
void		 goc_canvas_w2c (GocCanvas *canvas, int x, int y, double *x_, double *y_);
void		 goc_canvas_c2w (GocCanvas *canvas, double x, double y, int *x_, int *y_);
void		 goc_canvas_render (GocCanvas *canvas, cairo_t *cr, double x0, double y0, double x1, double y1);
void		 goc_canvas_get_bounds (GocCanvas *canvas, double *x0, double *y0, double *x1, double *y1);

void _goc_canvas_remove_item (GocCanvas *canvas, GocItem *item);

G_END_DECLS

#endif  /* GOC_CANVAS_H */
