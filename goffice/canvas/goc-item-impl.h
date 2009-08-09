/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-item-impl.h :  
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

#ifndef GOC_ITEM_IMPL_H
#define GOC_ITEM_IMPL_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GocItem {
	GObject			 base;

	GocCanvas		*canvas;
	GocGroup		*parent;
	gboolean		 cached_bounds;
	gboolean		 needs_redraw;
	gboolean		 visible;
	double			 x0, y0, x1, y1; /* the bounds */
};

typedef struct {
	GObjectClass	 base;

	double			(*distance) (GocItem *item,
								 double x, double y, GocItem **near_item);
	void			(*draw) (GocItem const *item, cairo_t *cr);
	gboolean		(*draw_region) (GocItem const *item, cairo_t *cr,
									double x0, double y0, double x1, double y1);
	void			(*move) (GocItem *item, double x, double y);
	void			(*update_bounds) (GocItem *item);
	void			(*parent_changed) (GocItem *item);
	cairo_operator_t
					(*get_operator) (GocItem *item);
	// events related functions
	gboolean		(*button_pressed) (GocItem *item, int button, double x, double y);
	gboolean		(*button2_pressed) (GocItem *item, int button, double x, double y);
	gboolean		(*button_released) (GocItem *item, int button, double x, double y);
	gboolean		(*motion) (GocItem *item, double x, double y);
	gboolean		(*enter_notify) (GocItem *item, double x, double y);
	gboolean		(*leave_notify) (GocItem *item, double x, double y);
	void			(*realize) (GocItem *item);
	void			(*unrealize) (GocItem *item);
	gboolean		(*key_pressed) (GocItem *item, GdkEventKey* ev);
	gboolean		(*key_released) (GocItem *item, GdkEventKey* ev);
	void			(*notify_scrolled) (GocItem *item);
} GocItemClass ;

G_END_DECLS

#endif  /* GOC_ITEM_IMPL_H */
