/*
 * gog-data-allocator.h :
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

#ifndef GOG_DATA_ALLOCATOR_H
#define GOG_DATA_ALLOCATOR_H

#include <goffice/goffice.h>

G_BEGIN_DECLS

#define GOG_TYPE_DATA_EDITOR		(gog_data_editor_get_type ())
#define GOG_DATA_EDITOR(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_DATA_EDITOR, GogDataEditor))
#define GOG_IS_DATA_EDITOR(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_DATA_EDITOR))
#define GOG_DATA_EDITOR_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_DATA_EDITOR, GogDataEditorClass))
#define GOG_IS_DATA_EDITOR_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_DATA_EDITOR))
#define GOG_DATA_EDITOR_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_INTERFACE ((o), GOG_TYPE_DATA_EDITOR, GogDataEditorClass))

GType gog_data_editor_get_type (void);

typedef struct {
	GTypeInterface		   base;

	void     (*set_format)       (GogDataEditor *editor,
				      GOFormat const *fmt);
	void     (*set_value_double) (GogDataEditor *editor, double val,
				      GODateConventions const *date_conv);
} GogDataEditorClass;

void gog_data_editor_set_format (GogDataEditor *editor,
				 GOFormat const *fmt);
void gog_data_editor_set_value_double (GogDataEditor *editor, double val,
				       GODateConventions const *date_conv);



typedef struct {
	GTypeInterface		   base;

	void	        (*allocate) (GogDataAllocator *a, GogPlot *plot);
	GogDataEditor * (*editor)   (GogDataAllocator *a, GogDataset *set,
				     int dim_i, GogDataType data_type);
} GogDataAllocatorClass;

#define GOG_TYPE_DATA_ALLOCATOR		(gog_data_allocator_get_type ())
#define GOG_DATA_ALLOCATOR(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_DATA_ALLOCATOR, GogDataAllocator))
#define GOG_IS_DATA_ALLOCATOR(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_DATA_ALLOCATOR))
#define GOG_DATA_ALLOCATOR_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GOG_TYPE_DATA_ALLOCATOR, GogDataAllocatorClass))
#define GOG_IS_DATA_ALLOCATOR_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GOG_TYPE_DATA_ALLOCATOR))
#define GOG_DATA_ALLOCATOR_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_INTERFACE ((o), GOG_TYPE_DATA_ALLOCATOR, GogDataAllocatorClass))

GType gog_data_allocator_get_type (void);

void	 gog_data_allocator_allocate (GogDataAllocator *dalloc, GogPlot *plot);
GogDataEditor *gog_data_allocator_editor (GogDataAllocator *dalloc,
					  GogDataset *set,
					  int dim_i, GogDataType data_type);

G_END_DECLS

#endif /* GOG_DATA_ALLOCATOR_H */
