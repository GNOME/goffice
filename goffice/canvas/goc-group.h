/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goc-group.h :
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

#ifndef GOC_GROUP_H
#define GOC_GROUP_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

struct _GocGroup {
	GocItem			 base;

	double			 x, y;  /* group offset */
	GList			*children;
};

typedef struct _GocGroupClass GocGroupClass;
struct _GocGroupClass
{
	GocItemClass base;
};

#define GOC_TYPE_GROUP	(goc_group_get_type ())
#define GOC_GROUP(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOC_TYPE_GROUP, GocGroup))
#define GOC_IS_GROUP(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOC_TYPE_GROUP))

GType goc_group_get_type (void);

GocGroup *goc_group_new (GocGroup *parent);
void goc_group_clear (GocGroup *group);
void goc_group_add_child (GocGroup *parent, GocItem *item);
void goc_group_remove_child (GocGroup *parent, GocItem *item);
void goc_group_adjust_bounds (GocGroup const *group, double *x0, double *y0, double *x1, double *y1);
void goc_group_adjust_coords (GocGroup const *group, double *x, double *y);
void goc_group_cairo_transform (GocGroup const *group, cairo_t *cr, double x, double y);

G_END_DECLS

#endif  /* GOC_GROUP_H */
