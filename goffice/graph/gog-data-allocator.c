/*
 * gog-data-allocator.c :
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

#include <goffice/goffice-config.h>
#include <goffice/graph/gog-data-allocator.h>
#include <goffice/graph/gog-object.h>
#include <goffice/graph/gog-graph.h>
#include <goffice/data/go-data.h>

/**
 * GogDataEditorClass:
 * @base: base interface.
 * @set_format: sets the #GOFormat
 * @set_value_double: sets a value as a double.
 *
 * Interface to edit #GOData.
 **/

/**
 * GogDataAllocatorClass:
 * @base: base interface.
 * @allocate: provides data to a #GogPlot.
 * @editor: returns the #GogDataEditor.
 **/

GType
gog_data_allocator_get_type (void)
{
	static GType gog_data_allocator_type = 0;

	if (!gog_data_allocator_type) {
		static GTypeInfo const gog_data_allocator_info = {
			sizeof (GogDataAllocatorClass),	/* class_size */
			NULL,		/* base_init */
			NULL,		/* base_finalize */
		};

		gog_data_allocator_type = g_type_register_static (G_TYPE_INTERFACE,
			"GogDataAllocator", &gog_data_allocator_info, 0);
	}

	return gog_data_allocator_type;
}

/**
 * gog_data_allocator_allocate:
 * @dalloc: a #GogDataAllocator
 * @plot:  a #GogPlot
 *
 **/

void
gog_data_allocator_allocate (GogDataAllocator *dalloc, GogPlot *plot)
{
	g_return_if_fail (GOG_IS_DATA_ALLOCATOR (dalloc));
	GOG_DATA_ALLOCATOR_GET_CLASS (dalloc)->allocate (dalloc, plot);
}

/**
 * gog_data_allocator_editor:
 * @dalloc: a #GogDataAllocator
 * @set:
 * @dim_i:
 * @data_type:
 *
 * returns: (transfer full): a #GtkWidget.
 **/

GogDataEditor *
gog_data_allocator_editor (GogDataAllocator *dalloc, GogDataset *set,
			   int dim_i, GogDataType data_type)
{
	GogDataAllocatorClass *klass;
	g_return_val_if_fail (GOG_IS_DATA_ALLOCATOR (dalloc), NULL);

	klass = GOG_DATA_ALLOCATOR_GET_CLASS (dalloc);
	/* Extra type checks not really needed.  */
	return GOG_DATA_EDITOR
		(GTK_WIDGET
		 (klass->editor (dalloc, set, dim_i, data_type)));
}

GType
gog_data_editor_get_type (void)
{
	static GType gog_data_editor_type = 0;

	if (!gog_data_editor_type) {
		static GTypeInfo const gog_data_editor_info = {
			sizeof (GogDataEditorClass),	/* class_size */
			NULL,		/* base_init */
			NULL,		/* base_finalize */
		};

		gog_data_editor_type = g_type_register_static (G_TYPE_INTERFACE,
			"GogDataEditor", &gog_data_editor_info, 0);
	}

	return gog_data_editor_type;
}

void
gog_data_editor_set_format (GogDataEditor *editor, GOFormat const *fmt)
{
	GogDataEditorClass *klass;
	g_return_if_fail (GOG_IS_DATA_EDITOR (editor));

	klass = GOG_DATA_EDITOR_GET_CLASS (editor);
	if (klass->set_format)
		klass->set_format (editor, fmt);
}

void
gog_data_editor_set_value_double (GogDataEditor *editor, double val,
				  GODateConventions const *date_conv)
{
	GogDataEditorClass *klass;
	g_return_if_fail (GOG_IS_DATA_EDITOR (editor));

	klass = GOG_DATA_EDITOR_GET_CLASS (editor);
	klass->set_value_double (editor, val, date_conv);
}
