/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-styled-item.h :
 *
 * Copyright (C) 2009 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GOC_STYLED_ITEM_H
#define GOC_STYLED_ITEM_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GocStyledItem {
	GocItem	base;

	GOStyle	*style;
	gboolean scale_line_width;
};

typedef struct _GocStyledItemClass GocStyledItemClass;
struct _GocStyledItemClass {
	/*< private >*/
	GocItemClass base;

	/*< public >*/
	/* virtual */
	void	  (*init_style)     	(GocStyledItem *item, GOStyle *style);

	/*< private >*/
	/* signal */
	void (*style_changed) (GocStyledItem *item, GOStyle const *new_style);
};

#define GOC_TYPE_STYLED_ITEM	(goc_styled_item_get_type ())
#define GOC_STYLED_ITEM(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_STYLED_ITEM, GocStyledItem))
#define GOC_IS_STYLED_ITEM(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_STYLED_ITEM))
#define GOC_STYLED_ITEM_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GOC_TYPE_STYLED_ITEM, GocStyledItemClass))

GType     goc_styled_item_get_type (void);

gboolean  goc_styled_item_set_cairo_line  (GocStyledItem const *gsi, cairo_t *cr);

G_END_DECLS

#endif  /* GOC_STYLED_ITEM_H */
