/*
 * goc-item.h :
 *
 * Copyright (C) 2008-2009 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_ITEM_H
#define GOC_ITEM_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GocItem {
	GObject			 base;

	GocCanvas		*canvas;
	GocGroup		*parent;
	gboolean		 cached_bounds;
	gboolean		 visible;
	gboolean		 realized;
	double			 x0, y0, x1, y1; /* the bounds */
	cairo_operator_t	 op;
	cairo_matrix_t		transform; /* unused for now */
	gboolean		transformed; /* TRUE if the matrix is not identity */

	/* FIXME: Next development release needs to add style context and
	   room for expansion.  */

	gpointer		 priv;
};

typedef struct _GocItemClass GocItemClass;
struct _GocItemClass {
	GObjectClass	 base;

	double			(*distance) (GocItem *item,
					     double x, double y, GocItem **near_item);
	void			(*draw) (GocItem const *item, cairo_t *cr);
	gboolean		(*draw_region) (GocItem const *item, cairo_t *cr,
						double x0, double y0, double x1, double y1);
	void			(*update_bounds) (GocItem *item);
	/* events related functions */
	gboolean		(*button_pressed) (GocItem *item, int button, double x, double y);
	gboolean		(*button2_pressed) (GocItem *item, int button, double x, double y);
	gboolean		(*button_released) (GocItem *item, int button, double x, double y);
	gboolean		(*motion) (GocItem *item, double x, double y);
	gboolean		(*enter_notify) (GocItem *item, double x, double y);
	gboolean		(*leave_notify) (GocItem *item, double x, double y);
	void			(*realize) (GocItem *item);
	void			(*unrealize) (GocItem *item);
	void			(*notify_scrolled) (GocItem *item);
#ifdef GOFFICE_WITH_GTK
	gboolean		(*key_pressed) (GocItem *item, GdkEventKey* ev);
	gboolean		(*key_released) (GocItem *item, GdkEventKey* ev);
	GdkWindow*		(*get_window) (GocItem *item);
#endif
	void		(*copy) (GocItem *source, GocItem *dest);

	/* <private> */
	void (*reserved2) (void);
	void (*reserved3) (void);
	void (*reserved4) (void);
};

#define GOC_TYPE_ITEM	(goc_item_get_type ())
#define GOC_ITEM(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_ITEM, GocItem))
#define GOC_IS_ITEM(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_ITEM))
#define GOC_IS_ITEM_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GOC_TYPE_ITEM))
#define GOC_ITEM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOC_TYPE_ITEM, GocItemClass))

GType goc_item_get_type (void);

GocItem		*goc_item_new		(GocGroup *parent, GType type, const gchar *first_arg_name, ...);
void		 goc_item_destroy	(GocItem *item);
void		 goc_item_set		(GocItem *item, const gchar *first_arg_name, ...);
double		 goc_item_distance	(GocItem *item, double x, double y, GocItem **near_item);
void		 goc_item_draw		(GocItem const *item, cairo_t *cr);
gboolean	 goc_item_draw_region	(GocItem const *item, cairo_t *cr,
					 double x0, double y0, double x1, double y1);

void		 goc_item_invalidate	(GocItem *item);
void		 goc_item_show		(GocItem *item);
void		 goc_item_hide		(GocItem *item);
void		 goc_item_set_visible   (GocItem *item, gboolean visible);
gboolean	 goc_item_is_visible	(GocItem *item);
void		 goc_item_get_bounds	(GocItem const *item,
					 double *x0, double *y0,
					 double *x1, double *y1);
GocGroup	*goc_item_get_parent    (GocItem *item);
#ifdef GOFFICE_WITH_GTK
GdkWindow       *goc_item_get_window    (GocItem *item);
#endif
void		 goc_item_bounds_changed (GocItem *item);
void		 goc_item_grab		(GocItem *item);
void		 goc_item_ungrab	(GocItem *item);
void		 goc_item_raise		(GocItem *item, int n);
void		 goc_item_lower		(GocItem *item, int n);
void		 goc_item_lower_to_bottom (GocItem *item);
void		 goc_item_raise_to_top	(GocItem *item);
void		 _goc_item_realize      (GocItem *item);
void		 _goc_item_unrealize    (GocItem *item);
void		 _goc_item_transform    (GocItem const *item, cairo_t *cr,
		                             gboolean scaled);

void		 goc_item_set_operator  (GocItem *item, cairo_operator_t op);
cairo_operator_t goc_item_get_operator  (GocItem *item);
void		 goc_item_set_transform (GocItem *item, cairo_matrix_t *m);
void		 goc_item_copy (GocItem *dest, GocItem *source);
GocItem		*goc_item_duplicate (GocItem *item, GocGroup *parent);

#ifdef GOFFICE_WITH_GTK
GtkStyleContext *goc_item_get_style_context (const GocItem *item);
#endif

G_END_DECLS

#endif  /* GOC_ITEM_H */
