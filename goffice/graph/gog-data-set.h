/*
 * gog-data-set.h :
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

#ifndef GOG_DATA_SET_H
#define GOG_DATA_SET_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

typedef struct {
	GOData	   *data;
	GogDataset *set;
	int	    dim_i;
	/*<private>*/
	gulong	    handler;
} GogDatasetElement;

typedef struct {
	GTypeInterface		   base;

	GogDatasetElement *(*get_elem) (GogDataset const *set, int dim_i);
	void (*set_dim)     (GogDataset *set, int dim_i,
			     GOData *val, GError **err);
	void (*dims)	    (GogDataset const *set, int *first, int *last);
	void (*dim_changed) (GogDataset *set, int dim_i);
} GogDatasetClass;

#define GOG_TYPE_DATASET		(gog_dataset_get_type ())
#define GOG_DATASET(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_DATASET, GogDataset))
#define GOG_IS_DATASET(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_DATASET))
#define GOG_DATASET_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_DATASET, GogDatasetClass))
#define GOG_IS_DATASET_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_DATASET))
#define GOG_DATASET_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_INTERFACE ((o), GOG_TYPE_DATASET, GogDatasetClass))

GType gog_dataset_get_type (void);

void    gog_dataset_dims    (GogDataset const *set, int *first, int *last);
GOData *gog_dataset_get_dim (GogDataset const *set, int dim_i);
void	gog_dataset_set_dim (GogDataset *set, int dim_i,
			     GOData *val, GError **err);

/* protected */
void 		   gog_dataset_finalize (GogDataset *set);
void 		   gog_dataset_parent_changed (GogDataset *set, gboolean was_set);
GogDatasetElement *gog_dataset_get_elem (GogDataset const *set, int dim_i);
void gog_dataset_set_dim_internal (GogDataset *set, int dim_i,
				   GOData *val, GogGraph *graph);

void gog_dataset_dup_to_simple (GogDataset const *src, GogDataset *dst);

G_END_DECLS

#endif /* GOG_DATA_SET_H */
