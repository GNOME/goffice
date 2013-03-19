/*
 * go-plugin-loader.c: Base class for plugin loaders.
 *
 * Author: Zbigniew Chyla (cyba@gnome.pl)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#include <goffice/goffice-config.h>
#include <goffice/app/go-plugin-loader.h>
#include <goffice/app/go-plugin.h>
#include <goffice/app/go-plugin-service.h>
#include <goffice/app/error-info.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

#define PL_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_INTERFACE ((o), GO_TYPE_PLUGIN_LOADER, GOPluginLoaderClass))

/**
 * GOPluginLoaderClass:
 * @base: base interface.
 * @load_base: loads the plugin without activating any service.
 * @unload_base: unloads the plugin if possible.
 * @set_attributes: sets attiributes for the module.
 * @service_load: loads a service.
 * @service_unload: unloads a service if possible.
 * @load_service_file_opener: loads a file opener service.
 * @unload_service_file_opener: unloads a file opener service.
 * @load_service_file_saver: loads a file saver service.
 * @unload_service_file_saver: unloads a file saver service.
 * @load_service_plugin_loader: loads a plugin loader service.
 * @unload_service_plugin_loader: unloads a plugin loader service.
 **/

gboolean
go_plugin_loader_is_base_loaded (GOPluginLoader *loader)
{
	return g_object_get_data (G_OBJECT (loader), "is-base-loaded") != NULL;
}

/**
 * go_plugin_loader_get_plugin:
 * @l: #GOPluginLoader
 *
 * Returns: (transfer full): the plugin
 **/
GOPlugin *
go_plugin_loader_get_plugin (GOPluginLoader *l)
{
	return g_object_get_data (G_OBJECT (l), "plugin");
}

void
go_plugin_loader_set_plugin (GOPluginLoader *l, GOPlugin *p)
{
	g_object_set_data (G_OBJECT (l), "plugin", p);
}

void
go_plugin_loader_set_attributes (GOPluginLoader *loader, GHashTable *attrs,
				 GOErrorInfo **err)
{
	g_return_if_fail (GO_IS_PLUGIN_LOADER (loader));

	GO_INIT_RET_ERROR_INFO (err);
	if (PL_GET_CLASS (loader)->set_attributes)
		PL_GET_CLASS (loader)->set_attributes (loader, attrs, err);
	else
		*err = go_error_info_new_printf (_("Loader has no set_attributes method.\n"));
}

void
go_plugin_loader_load_base (GOPluginLoader *loader, GOErrorInfo **err)
{
	GOPluginLoaderClass *go_plugin_loader_class;

	g_return_if_fail (GO_IS_PLUGIN_LOADER (loader));
	g_return_if_fail (!go_plugin_loader_is_base_loaded (loader));

	go_plugin_loader_class = PL_GET_CLASS (loader);
	if (go_plugin_loader_class->load_base != NULL)
		go_plugin_loader_class->load_base (loader, err);
	else
		*err = go_error_info_new_printf (_("Loader has no load_base method.\n"));
	if (*err == NULL)
		g_object_set_data (G_OBJECT (loader), "is-base-loaded", GINT_TO_POINTER (1));
}

void
go_plugin_loader_unload_base (GOPluginLoader *loader, GOErrorInfo **err)
{
	GOPluginLoaderClass *go_plugin_loader_class;

	g_return_if_fail (GO_IS_PLUGIN_LOADER (loader));

	go_plugin_loader_class = PL_GET_CLASS (loader);
	if (go_plugin_loader_class->unload_base != NULL) {
		go_plugin_loader_class->unload_base (loader, err);
		if (*err == NULL)
			g_object_set_data (G_OBJECT (loader), "is-base-loaded", NULL);
	}
}

void
go_plugin_loader_load_service (GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err)
{
	GOPluginLoaderClass *klass;
	void (*load_service_method) (GOPluginLoader *, GOPluginService *, GOErrorInfo **) = NULL;

	g_return_if_fail (GO_IS_PLUGIN_LOADER (l));
	g_return_if_fail (GO_IS_PLUGIN_SERVICE (s));
	g_return_if_fail (go_plugin_loader_is_base_loaded (l));

	GO_INIT_RET_ERROR_INFO (err);

	klass = PL_GET_CLASS (l);
	if (klass->service_load && (klass->service_load) (l, s, err))
		return;

	if (GO_IS_PLUGIN_SERVICE_FILE_OPENER (s)) {
		load_service_method = klass->load_service_file_opener;
	} else if (GO_IS_PLUGIN_SERVICE_FILE_SAVER (s)) {
		load_service_method = klass->load_service_file_saver;
	} else if (GO_IS_PLUGIN_SERVICE_PLUGIN_LOADER (s)) {
		load_service_method = klass->load_service_plugin_loader;
	} else if (GO_IS_PLUGIN_SERVICE_SIMPLE (s)) {
		load_service_method = NULL;
	} else {
		*err = go_error_info_new_printf (_("Service '%s' not supported by loader."),
			G_OBJECT_TYPE_NAME (s));
	}
	if (load_service_method != NULL)
		load_service_method (l, s, err);

	if (*err == NULL) {
		gpointer num_services = g_object_get_data (G_OBJECT (l), "num-services");
		g_object_set_data (G_OBJECT (l), "num-services",
		    GINT_TO_POINTER (GPOINTER_TO_INT (num_services) + 1));
	}
}

void
go_plugin_loader_unload_service (GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err)
{
	GOPluginLoaderClass *klass;
	void (*unload_service_method) (GOPluginLoader *, GOPluginService *, GOErrorInfo **) = NULL;
	GOErrorInfo *error = NULL;

	g_return_if_fail (GO_IS_PLUGIN_LOADER (l));
	g_return_if_fail (GO_IS_PLUGIN_SERVICE (s));

	GO_INIT_RET_ERROR_INFO (err);

	klass = PL_GET_CLASS (l);
	if (klass->service_unload && (klass->service_unload) (l, s, err))
		return;

	if (GO_IS_PLUGIN_SERVICE_FILE_OPENER (s)) {
		unload_service_method = klass->unload_service_file_opener;
	} else if (GO_IS_PLUGIN_SERVICE_FILE_SAVER (s)) {
		unload_service_method = klass->unload_service_file_saver;
	} else if (GO_IS_PLUGIN_SERVICE_PLUGIN_LOADER (s)) {
		unload_service_method = klass->unload_service_plugin_loader;
	} else if (GO_IS_PLUGIN_SERVICE_SIMPLE (s)) {
		unload_service_method = NULL;
	} else
		*err = go_error_info_new_printf (_("Service '%s' not supported by loader."),
			G_OBJECT_TYPE_NAME (s));

	if (unload_service_method != NULL)
		unload_service_method (l, s, &error);
	if (error == NULL) {
		gpointer num_services = g_object_get_data (G_OBJECT (l), "num-services");
		g_return_if_fail (num_services != NULL);
		g_object_set_data (G_OBJECT (l), "num-services",
		    GINT_TO_POINTER (GPOINTER_TO_INT (num_services) - 1));
		if (GPOINTER_TO_INT (num_services) == 1) {
			go_plugin_loader_unload_base (l, &error);
			go_error_info_free (error);
		}
	} else
		*err = error;
}

GType
go_plugin_loader_get_type (void)
{
	static GType go_plugin_loader_type = 0;

	if (!go_plugin_loader_type) {
		static GTypeInfo const go_plugin_loader_info = {
			sizeof (GOPluginLoaderClass),	/* class_size */
			NULL,				/* base_init */
			NULL,				/* base_finalize */
		};

		go_plugin_loader_type = g_type_register_static (G_TYPE_INTERFACE,
			"GOPluginLoader", &go_plugin_loader_info, 0);
	}

	return go_plugin_loader_type;
}
