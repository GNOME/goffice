/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-item.h :  
 *
 * Copyright (C) 2008-2009 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_ITEM_H
#define GOC_ITEM_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GOC_TYPE_ITEM	(goc_item_get_type ())
#define GOC_ITEM(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_ITEM, GocItem))
#define GOC_IS_ITEM(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_ITEM))
#define GOC_IS_ITEM_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GOC_TYPE_ITEM))
#define GOC_ITEM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOC_TYPE_ITEM, GocItemClass))

GType goc_item_get_type (void);

typedef struct {
	GTypeInterface		   base;

} GocItemClientClass;

GocItem		*goc_item_new		(GocGroup *parent, GType type, const gchar *first_arg_name, ...);
void		 goc_item_set		(GocItem *item, const gchar *first_arg_name, ...);
double		 goc_item_distance	(GocItem *item, double x, double y, GocItem **near_item);
void		 goc_item_draw		(GocItem const *item, cairo_t *cr);
gboolean	 goc_item_draw_region	(GocItem const *item, cairo_t *cr,
									 double x0, double y0, double x1, double y1);
cairo_operator_t
			 goc_item_get_operator  (GocItem *item);
void		 goc_item_move			(GocItem *item, double x, double y);

void		 goc_item_invalidate	(GocItem *item);
void		 goc_item_show			(GocItem *item);
void		 goc_item_hide			(GocItem *item);
gboolean	 goc_item_is_visible	(GocItem *item);
void		 goc_item_get_bounds	(GocItem const *item,
									 double *x0, double *y0,
									 double *x1, double *y1);
void		 goc_item_update_bounds	(GocItem *item);
void		 goc_item_bounds_changed (GocItem *item);
void		 goc_item_parent_changed (GocItem *item);
void		 goc_item_grab		(GocItem *item);
void		 goc_item_ungrab	(GocItem *item);
void		 goc_item_raise		(GocItem *item, int n);
void		 goc_item_lower		(GocItem *item, int n);
void		 goc_item_lower_to_bottom (GocItem *item);
void		 goc_item_raise_to_top	(GocItem *item);

G_END_DECLS

#endif  /* GOC_ITEM_H */