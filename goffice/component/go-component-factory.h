/*
 * go-component-factory.h :
 *
 * Copyright (C) 2005 Jean Brefort (jean.brefort@normalesup.org)
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

#ifndef GO_COMPONENT_FACTORY_H
#define GO_COMPONENT_FACTORY_H

G_BEGIN_DECLS

typedef enum
{
	GO_MIME_PRIORITY_INVALID = -1,
	GO_MIME_PRIORITY_DISPLAY,
	GO_MIME_PRIORITY_PRINT,
	GO_MIME_PRIORITY_PARTIAL,
	GO_MIME_PRIORITY_FULL,
	GO_MIME_PRIORITY_NATIVE,
} GOMimePriority;

typedef struct
{
	GOMimePriority priority;
	char* component_type_name;
	gboolean support_clipboard;
} GOMimeType;

GSList *go_components_get_mime_types (void);
GOMimePriority go_components_get_priority (char const *mime_type);
gboolean go_components_support_clipboard (char const *mime_type);
void go_components_add_mime_type (char *mime, GOMimePriority priority, char const *service_id);
void go_components_set_mime_suffix (char const *mime, char const *suffix);
char const *go_components_get_mime_suffix (char const *mime);
void go_components_add_filter (GtkFileChooser *chooser);

void _goc_plugin_services_init (void);
void _goc_plugin_services_shutdown (void);

G_END_DECLS

#endif	/* GOFFICE_COMPONENT_FACTORY_H */
