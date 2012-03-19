/*
 * go-graph-item-impl.h :  Implementation details for the abstract graph-item
 * 			interface
 *
 * Copyright (C) 2001 Zbigniew Chyla (cyba@gnome.pl)
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

#ifndef GO_PLUGIN_SERVICE_IMPL_H
#define GO_PLUGIN_SERVICE_IMPL_H

#include <goffice/app/goffice-app.h>
#include <glib-object.h>
#include <libxml/tree.h>

G_BEGIN_DECLS

struct _GOPluginService {
	GObject   g_object;

	char   *id;
	GOPlugin *plugin;
	gboolean is_loaded;

	/* protected */
	gpointer cbs_ptr;
	gboolean is_active;

	/* private */
	char *saved_description;
};

typedef struct {
	GObjectClass g_object_class;

	void (*read_xml) (GOPluginService *service, xmlNode *tree, GOErrorInfo **ret_error);
	void (*activate) (GOPluginService *service, GOErrorInfo **ret_error);
	void (*deactivate) (GOPluginService *service, GOErrorInfo **ret_error);
	char *(*get_description) (GOPluginService *service);
} GOPluginServiceClass;

#define GO_PLUGIN_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GO_TYPE_PLUGIN_SERVICE, GOPluginServiceClass))
#define GO_PLUGIN_SERVICE_GET_CLASS(o) GO_PLUGIN_SERVICE_CLASS (G_OBJECT_GET_CLASS (o))

typedef struct {
	GOPluginServiceClass plugin_service_class;
	GHashTable *pending; /* has service instances by type names */
} GOPluginServiceGObjectLoaderClass;

struct _GOPluginServiceGObjectLoader {
	GOPluginService plugin_service;
};

#define GO_PLUGIN_SERVICE_GOBJECT_LOADER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GO_TYPE_PLUGIN_SERVICE, GOPluginServiceGObjectLoaderClass))
#define GO_PLUGIN_SERVICE_GOBJECT_LOADER_GET_CLASS(o) GO_PLUGIN_SERVICE_GOBJECT_LOADER_CLASS (G_OBJECT_GET_CLASS (o))

typedef struct {
	GOPluginServiceClass plugin_service_class;
} GOPluginServiceSimpleClass;

struct _GOPluginServiceSimple {
	GOPluginService plugin_service;
};

G_END_DECLS

#endif /* GO_PLUGIN_SERVICE_IMPL_H */

