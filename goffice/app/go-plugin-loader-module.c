/*
 * plugin-loader-module.c: Support for "g_module" (shared libraries) plugins.
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
#include "go-plugin-loader-module.h"
#include "module-plugin-defs.h"

#include <goffice/app/file.h>
#include <goffice/app/go-plugin.h>
#include <goffice/app/go-plugin-service.h>
#include <goffice/app/go-plugin-loader.h>
#include <goffice/app/error-info.h>

#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-output.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlmemory.h>
#include <glib/gi18n-lib.h>
#include <string.h>

static void go_plugin_loader_module_set_attributes (GOPluginLoader *loader, GHashTable *attrs, GOErrorInfo **ret_error);
static void go_plugin_loader_module_load_base (GOPluginLoader *loader, GOErrorInfo **ret_error);
static void go_plugin_loader_module_unload_base (GOPluginLoader *loader, GOErrorInfo **ret_error);

static void go_plugin_loader_module_load_service_file_opener (GOPluginLoader *loader, GOPluginService *service, GOErrorInfo **ret_error);
static void go_plugin_loader_module_load_service_file_saver (GOPluginLoader *loader, GOPluginService *service, GOErrorInfo **ret_error);
static void go_plugin_loader_module_load_service_plugin_loader (GOPluginLoader *loader, GOPluginService *service, GOErrorInfo **ret_error);

static GHashTable *go_plugin_loader_module_known_deps = NULL;

void
go_plugin_loader_module_register_version (char const *id, char const *version)
{
	if (NULL == go_plugin_loader_module_known_deps)
		go_plugin_loader_module_known_deps =
			g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_replace (go_plugin_loader_module_known_deps,
		(gpointer)id, (gpointer)version);
}

static void
go_plugin_loader_module_set_attributes (GOPluginLoader *loader, GHashTable *attrs,
					GOErrorInfo **err)
{
	GOPluginLoaderModule *loader_module = GO_PLUGIN_LOADER_MODULE (loader);
	gchar *module_file_name = g_hash_table_lookup (attrs, "module_file");

	if (module_file_name != NULL)
		loader_module->module_file_name = g_strdup (module_file_name);
	else
		*err = go_error_info_new_str ( _("Module file name not given."));
}

static GOErrorInfo *
check_version (GOPluginModuleDepend const *deps, guint32 num_deps, char const *module_file)
{
	unsigned i;
	char const *ver;

	if (deps == NULL && num_deps != 0)
		return go_error_info_new_printf (
			_("Module \"%s\" has an inconsistent dependency list."),
			module_file);

	g_return_val_if_fail (go_plugin_loader_module_known_deps != NULL, NULL);

	for (i = 0; i < num_deps ; i++) {
		if (deps[i].key == NULL)
			return go_error_info_new_printf (
				_("Module \"%s\" depends on an invalid null dependency."),
				module_file);
		ver = g_hash_table_lookup (go_plugin_loader_module_known_deps, deps[i].key);
		if (ver == NULL)
			return go_error_info_new_printf (
				_("Module \"%s\" depends on an unknown dependency '%s'."),
				module_file, deps[i].key);

		if (strcmp (ver, deps[i].version))
			return go_error_info_new_printf (
				_("Module \"%s\" was built with version %s of %s, but this executable supplied version %s."),
				module_file, deps[i].version, deps[i].key, ver);
	}
	return NULL;
}

static void
go_plugin_loader_module_load_base (GOPluginLoader *loader, GOErrorInfo **err)
{
	GOPluginLoaderModule *loader_module = GO_PLUGIN_LOADER_MODULE (loader);
	gchar *full_module_file_name;
	GModule *handle;
	GOPluginModuleHeader const *go_plugin_header = NULL;
	GOPluginModuleDepend const *go_plugin_depends = NULL;

	GO_INIT_RET_ERROR_INFO (err);
	if (!g_module_supported ()) {
		*err = go_error_info_new_str (
			_("Dynamic module loading is not supported in this system."));
		return;
	}

	full_module_file_name = g_build_filename (go_plugin_get_dir_name (
		go_plugin_loader_get_plugin (loader)),
		loader_module->module_file_name, NULL);

	if (go_debug_flag ("plugin"))
		g_printerr ("Loading module file %s\n",
			    full_module_file_name);

	handle = g_module_open (full_module_file_name, G_MODULE_BIND_LAZY);
	if (handle == NULL) {
		*err = go_error_info_new_printf (
			_("Unable to open module file \"%s\"."),
			full_module_file_name);
		go_error_info_add_details (*err, go_error_info_new_str (g_module_error()));
	} else {
		g_module_symbol (handle, "go_plugin_header", (gpointer) &go_plugin_header);
		g_module_symbol (handle, "go_plugin_depends", (gpointer) &go_plugin_depends);
		if (go_plugin_header == NULL) {
			*err = go_error_info_new_printf (
				_("Module \"%s\" doesn't contain (\"go_plugin_header\" symbol)."),
				full_module_file_name);
		} else if (go_plugin_header->magic_number != GOFFICE_MODULE_PLUGIN_MAGIC_NUMBER) {
			*err = go_error_info_new_printf (
				_("Module \"%s\" has an invalid magic number."),
				full_module_file_name);
		} else if (NULL == (*err = check_version (go_plugin_depends,  go_plugin_header->num_depends, full_module_file_name))) {
			loader_module->handle = handle;
			g_module_symbol (handle, "go_plugin_init", (gpointer)&loader_module->plugin_init);
			g_module_symbol (handle, "go_plugin_shutdown", (gpointer)&loader_module->plugin_shutdown);
			if (loader_module->plugin_init != NULL)
				(loader_module->plugin_init) (go_plugin_loader_get_plugin (loader), NULL);
		}

		if (*err != NULL) {
			if (go_debug_flag ("plugin"))
				g_printerr ("Error while loading module\n");
			g_module_close (handle);
		}
	}
	g_free (full_module_file_name);
}

static void
go_plugin_loader_module_unload_base (GOPluginLoader *loader, GOErrorInfo **ret_error)
{
	GOPluginLoaderModule *loader_module = GO_PLUGIN_LOADER_MODULE (loader);

	GO_INIT_RET_ERROR_INFO (ret_error);
	if (loader_module->plugin_shutdown != NULL) {
		loader_module->plugin_shutdown (go_plugin_loader_get_plugin (loader), NULL);
	}
	if (go_debug_flag ("plugin")) {
		char *full_module_file_name =
			g_build_filename
			(go_plugin_get_dir_name (go_plugin_loader_get_plugin (loader)),
			 loader_module->module_file_name,
			 NULL);

		g_printerr ("Unloading %s\n", full_module_file_name);
		g_free (full_module_file_name);
	}
	if (!g_module_close (loader_module->handle)) {
		*ret_error = go_error_info_new_printf (
			_("Unable to close module file \"%s\"."),
			loader_module->module_file_name);
		go_error_info_add_details (*ret_error, go_error_info_new_str (g_module_error()));
	}
	loader_module->handle = NULL;
	loader_module->plugin_init = NULL;
	loader_module->plugin_shutdown = NULL;
}

static void
go_plugin_loader_module_init (GOPluginLoaderModule *loader_module)
{
	g_return_if_fail (GO_IS_PLUGIN_LOADER_MODULE (loader_module));

	loader_module->module_file_name = NULL;
	loader_module->handle = NULL;
}

static void
go_plugin_loader_module_finalize (GObject *obj)
{
	GOPluginLoaderModule *loader_module = GO_PLUGIN_LOADER_MODULE (obj);
	g_free (loader_module->module_file_name);
	loader_module->module_file_name = NULL;
	G_OBJECT_CLASS (g_type_class_peek (G_TYPE_OBJECT))->finalize (obj);
}

static void
go_plugin_loader_module_class_init (GObjectClass *gobject_class)
{
	gobject_class->finalize = go_plugin_loader_module_finalize;
	go_plugin_loader_module_register_version ("goffice", GOFFICE_API_VERSION);
}

static void
go_plugin_loader_init (GOPluginLoaderClass *go_plugin_loader_class)
{
	go_plugin_loader_class->set_attributes = go_plugin_loader_module_set_attributes;
	go_plugin_loader_class->load_base = go_plugin_loader_module_load_base;
	go_plugin_loader_class->unload_base = go_plugin_loader_module_unload_base;
	go_plugin_loader_class->load_service_file_opener = go_plugin_loader_module_load_service_file_opener;
	go_plugin_loader_class->load_service_file_saver = go_plugin_loader_module_load_service_file_saver;
	go_plugin_loader_class->load_service_plugin_loader = go_plugin_loader_module_load_service_plugin_loader;
}

GSF_CLASS_FULL (GOPluginLoaderModule, go_plugin_loader_module,
	   NULL, NULL, go_plugin_loader_module_class_init, NULL,
	   go_plugin_loader_module_init, G_TYPE_OBJECT, 0,
	   GSF_INTERFACE (go_plugin_loader_init, GO_TYPE_PLUGIN_LOADER))

/*
 * Service - file_opener
 */

typedef struct {
	gboolean (*module_func_file_probe) (GOFileOpener const *fo, GsfInput *input,
					    GOFileProbeLevel pl);
	void (*module_func_file_open) (GOFileOpener const *fo, GOIOContext *io_context,
				       GoView *view,
				       GsfInput *input);
} ServiceLoaderDataFileOpener;

static gboolean
go_plugin_loader_module_func_file_probe (GOFileOpener const *fo, GOPluginService *service,
					  GsfInput *input, GOFileProbeLevel pl)
{
	ServiceLoaderDataFileOpener *loader_data;

	g_return_val_if_fail (GO_IS_PLUGIN_SERVICE_FILE_OPENER (service), FALSE);
	g_return_val_if_fail (input != NULL, FALSE);

	loader_data = g_object_get_data (G_OBJECT (service), "loader_data");
	return loader_data->module_func_file_probe (fo, input, pl);
}

static void
go_plugin_loader_module_func_file_open (GOFileOpener const *fo, GOPluginService *service,
					 GOIOContext *io_context,
					 GoView *view,
					GsfInput  *input,
					char const *enc)
{
	ServiceLoaderDataFileOpener *loader_data;

	g_return_if_fail (GO_IS_PLUGIN_SERVICE_FILE_OPENER (service));
	g_return_if_fail (input != NULL);

	loader_data = g_object_get_data (G_OBJECT (service), "loader_data");
	if (go_file_opener_is_encoding_dependent (fo)) {
		void (*module_func_file_open) (GOFileOpener const *fo, char const *enc,
					       GOIOContext *io_context,
					       GoView *view,
					       GsfInput *input);
		module_func_file_open = (gpointer)(loader_data->module_func_file_open);
		module_func_file_open (fo, enc, io_context,
				       view, input);
	} else
		loader_data->module_func_file_open (fo, io_context,
						    view, input);
}

static char *
make_function_name (const GOPluginService *service, const char *suffix)
{
	const char *id = go_plugin_service_get_id (service);
	size_t idlen = strlen (id);
	size_t slen = strlen (suffix);
	size_t pad = 4;
	char *res = g_new (char, idlen + slen + pad);

	/* This is concatenation with padding.  Linux dlsym seems to like
	   to read beyond the end.  */
	strcpy (res, id);
	strcpy (res + idlen, suffix);
	memset (res + idlen + slen, 0, pad);

	return res;
}

static void
go_plugin_loader_module_load_service_file_opener (GOPluginLoader *loader,
						  GOPluginService *service,
						  GOErrorInfo **ret_error)
{
	GOPluginLoaderModule *loader_module = GO_PLUGIN_LOADER_MODULE (loader);
	gchar *func_name_file_probe, *func_name_file_open;
	gpointer module_func_file_probe = NULL, module_func_file_open = NULL;

	g_return_if_fail (GO_IS_PLUGIN_SERVICE_FILE_OPENER (service));

	GO_INIT_RET_ERROR_INFO (ret_error);
	func_name_file_probe = make_function_name (service, "_file_probe");
	g_module_symbol (loader_module->handle, func_name_file_probe, &module_func_file_probe);
	func_name_file_open = make_function_name (service, "_file_open");
	g_module_symbol (loader_module->handle, func_name_file_open, &module_func_file_open);
	if (module_func_file_open != NULL) {
		GOPluginServiceFileOpenerCallbacks *cbs;
		ServiceLoaderDataFileOpener *loader_data;

		cbs = go_plugin_service_get_cbs (service);
		cbs->plugin_func_file_probe = go_plugin_loader_module_func_file_probe;
		cbs->plugin_func_file_open = go_plugin_loader_module_func_file_open;

		loader_data = g_new (ServiceLoaderDataFileOpener, 1);
		loader_data->module_func_file_probe = module_func_file_probe;
		loader_data->module_func_file_open = module_func_file_open;
		g_object_set_data_full (G_OBJECT (service),
					"loader_data", loader_data, g_free);
	} else {
		*ret_error = go_error_info_new_printf (
			_("Module file \"%s\" has invalid format."),
			loader_module->module_file_name);
		go_error_info_add_details (*ret_error, go_error_info_new_printf (
			_("File doesn't contain \"%s\" function."), func_name_file_open));
	}
	g_free (func_name_file_probe);
	g_free (func_name_file_open);
}

/*
 * Service - file_saver
 */

typedef struct {
	void (*module_func_file_save) (GOFileSaver const *fs, GOIOContext *io_context,
				       GoView const *view,
				       GsfOutput *output);
} ServiceLoaderDataFileSaver;

static void
go_plugin_loader_module_func_file_save (GOFileSaver const *fs, GOPluginService *service,
					 GOIOContext *io_context,
					 GoView const *view,
					 GsfOutput *output)
{
	ServiceLoaderDataFileSaver *loader_data;

	g_return_if_fail (GO_IS_PLUGIN_SERVICE_FILE_SAVER (service));
	g_return_if_fail (GSF_IS_OUTPUT (output));

	loader_data = g_object_get_data (G_OBJECT (service), "loader_data");
	loader_data->module_func_file_save (fs, io_context,
		view, output);
}

static void
go_plugin_loader_module_load_service_file_saver (GOPluginLoader *loader,
						 GOPluginService *service,
						 GOErrorInfo **ret_error)
{
	GOPluginLoaderModule *loader_module = GO_PLUGIN_LOADER_MODULE (loader);
	gchar *func_name_file_save;
	gpointer module_func_file_save = NULL;

	g_return_if_fail (GO_IS_PLUGIN_SERVICE_FILE_SAVER (service));

	GO_INIT_RET_ERROR_INFO (ret_error);
	func_name_file_save = make_function_name (service, "_file_save");
	g_module_symbol (loader_module->handle, func_name_file_save, &module_func_file_save);
	if (module_func_file_save != NULL) {
		GOPluginServiceFileSaverCallbacks *cbs;
		ServiceLoaderDataFileSaver *loader_data;

		cbs = go_plugin_service_get_cbs (service);
		cbs->plugin_func_file_save = go_plugin_loader_module_func_file_save;

		loader_data = g_new (ServiceLoaderDataFileSaver, 1);
		loader_data->module_func_file_save = module_func_file_save;
		g_object_set_data_full (G_OBJECT (service),
					"loader_data", loader_data, g_free);
	} else {
		*ret_error = go_error_info_new_printf (
			_("Module file \"%s\" has invalid format."),
			loader_module->module_file_name);
		go_error_info_add_details (*ret_error, go_error_info_new_printf (
			_("File doesn't contain \"%s\" function."),
			func_name_file_save));
	}
	g_free (func_name_file_save);
}

/*
 * Service - plugin_loader
 */

typedef struct {
	GType (*module_func_get_loader_type) (GOErrorInfo **ret_error);
} ServiceLoaderDataPluginLoader;

static GType
go_plugin_loader_module_func_get_loader_type (GOPluginService *service,
					       GOErrorInfo **ret_error)
{
	ServiceLoaderDataPluginLoader *loader_data;
	GOErrorInfo *error = NULL;
	GType loader_type;

	g_return_val_if_fail (GO_IS_PLUGIN_SERVICE_PLUGIN_LOADER (service), 0);

	GO_INIT_RET_ERROR_INFO (ret_error);
	loader_data = g_object_get_data (G_OBJECT (service), "loader_data");
	loader_type = loader_data->module_func_get_loader_type (&error);
	if (error == NULL) {
		return loader_type;
	} else {
		*ret_error = error;
		return (GType) 0;
	}
}

static void
go_plugin_loader_module_load_service_plugin_loader (GOPluginLoader *loader,
						    GOPluginService *service,
						    GOErrorInfo **ret_error)
{
	GOPluginLoaderModule *loader_module = GO_PLUGIN_LOADER_MODULE (loader);
	gchar *func_name_get_loader_type;
	gpointer module_func_get_loader_type = NULL;

	g_return_if_fail (GO_IS_PLUGIN_SERVICE_PLUGIN_LOADER (service));

	GO_INIT_RET_ERROR_INFO (ret_error);
	func_name_get_loader_type =
		make_function_name (service, "_get_loader_type");
	g_module_symbol (loader_module->handle, func_name_get_loader_type,
			 &module_func_get_loader_type);
	if (module_func_get_loader_type != NULL) {
		GOPluginServicePluginLoaderCallbacks *cbs;
		ServiceLoaderDataPluginLoader *loader_data;

		cbs = go_plugin_service_get_cbs (service);
		cbs->plugin_func_get_loader_type = go_plugin_loader_module_func_get_loader_type;

		loader_data = g_new (ServiceLoaderDataPluginLoader, 1);
		loader_data->module_func_get_loader_type = module_func_get_loader_type;
		g_object_set_data_full (G_OBJECT (service),
					"loader_data", loader_data, g_free);
	} else
		*ret_error = go_error_info_new_printf (
			_("Module doesn't contain \"%s\" function."),
			func_name_get_loader_type);
	g_free (func_name_get_loader_type);
}
