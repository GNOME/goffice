/*
 * go-plugin-service.c: Plugin services - reading XML info, activating, etc.
 *                   (everything independent of plugin loading method)
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
#include "go-plugin-service.h"
#include "go-plugin-service-impl.h"

#include <goffice/app/error-info.h>
#include <goffice/app/go-plugin.h>
#include <goffice/app/file.h>
#include <goffice/app/file-priv.h>
#include <goffice/app/io-context.h>
#include <goffice/utils/go-glib-extras.h>
#include <goffice/utils/go-libxml-extras.h>

#include <gsf/gsf-input.h>
#include <gsf/gsf-output.h>
#include <libxml/globals.h>
#include <gsf/gsf-impl-utils.h>
#include <gsf/gsf-utils.h>
#include <glib/gi18n-lib.h>

#include <string.h>

/**
 * GOPluginServiceFileOpenerCallbacks:
 * @plugin_func_file_probe: probes the file, may be %NULL.
 * @plugin_func_file_open: opens and reads the file.
 **/

/**
 * GOPluginServiceFileSaverCallbacks:
 * @plugin_func_file_save: saves the file.
 **/

/**
 * GOPluginServiceGeneralCallbacks:
 * @plugin_func_init: initializes the service.
 * @plugin_func_cleanup: service cleanup.
 **/

/**
 * GOPluginServicePluginLoaderCallbacks:
 * @plugin_func_get_loader_type: returns a #GType for a function loader.
 * Used by gnumeric in the Python and Perl plugins.
 **/

/**
 * GOPluginServiceClass:
 * @g_object_class: base class.
 * @read_xml: read XML node containing the service description.
 * @activate: actviates the service.
 * @deactivate: deactivates the service.
 * @get_description: gets the service description.
 **/

/**
 * GOPluginServiceGObjectLoaderClass:
 * @plugin_service_class: parent class.
 * @pending: has service instances by type names.
 **/

#define CXML2C(s) ((char const *)(s))
#define CC2XML(s) ((xmlChar const *)(s))

static char *
xml2c (xmlChar *src)
{
	char *dst = g_strdup (CXML2C (src));
	xmlFree (src);
	return dst;
}

static GHashTable *services = NULL;

static GHashTable *
get_plugin_file_savers_hash (GOPlugin *plugin)
{
	GHashTable *hash;

	hash = g_object_get_data (G_OBJECT (plugin), "file_savers_hash");
	if (hash == NULL) {
		hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
		g_object_set_data_full (
			G_OBJECT (plugin), "file_savers_hash",
			hash, (GDestroyNotify) g_hash_table_destroy);
	}

	return hash;
}


static void
go_plugin_service_init (GObject *obj)
{
	GOPluginService *service = GO_PLUGIN_SERVICE (obj);

	service->id = NULL;
	service->is_active = FALSE;
	service->is_loaded = FALSE;
	service->plugin = NULL;
	service->cbs_ptr = NULL;
	service->saved_description = NULL;
}

static void
go_plugin_service_finalize (GObject *obj)
{
	GOPluginService *service = GO_PLUGIN_SERVICE (obj);
	GObjectClass *parent_class;

	g_free (service->id);
	service->id = NULL;
	g_free (service->saved_description);
	service->saved_description = NULL;

	parent_class = g_type_class_peek (G_TYPE_OBJECT);
	parent_class->finalize (obj);
}

static void
go_plugin_service_class_init (GObjectClass *gobject_class)
{
	GOPluginServiceClass *plugin_service_class = GO_PLUGIN_SERVICE_CLASS (gobject_class);

	gobject_class->finalize = go_plugin_service_finalize;
	plugin_service_class->read_xml = NULL;
	plugin_service_class->activate = NULL;
	plugin_service_class->deactivate = NULL;
	plugin_service_class->get_description = NULL;
}

GSF_CLASS (GOPluginService, go_plugin_service,
	   go_plugin_service_class_init, go_plugin_service_init,
           G_TYPE_OBJECT)


/****************************************************************************/

/*
 * GOPluginServiceGeneral
 */

typedef struct{
	GOPluginServiceClass plugin_service_class;
} GOPluginServiceGeneralClass;

struct _GOPluginServiceGeneral {
	GOPluginService plugin_service;
	GOPluginServiceGeneralCallbacks cbs;
};


static void
go_plugin_service_general_init (GObject *obj)
{
	GOPluginServiceGeneral *service_general = GO_PLUGIN_SERVICE_GENERAL (obj);

	GO_PLUGIN_SERVICE (obj)->cbs_ptr = &service_general->cbs;
	service_general->cbs.plugin_func_init = NULL;
	service_general->cbs.plugin_func_cleanup = NULL;
}

static void
go_plugin_service_general_activate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GOPluginServiceGeneral *service_general = GO_PLUGIN_SERVICE_GENERAL (service);
	GOErrorInfo *error = NULL;

	GO_INIT_RET_ERROR_INFO (ret_error);
	go_plugin_service_load (service, &error);
	if (error != NULL) {
		*ret_error = go_error_info_new_str_with_details (
		             _("Error while loading plugin service."),
		             error);
		return;
	}
	g_return_if_fail (service_general->cbs.plugin_func_init != NULL);
	service_general->cbs.plugin_func_init (service, &error);
	if (error != NULL) {
		*ret_error = go_error_info_new_str_with_details (
		             _("Initializing function inside plugin returned error."),
		             error);
		return;
	}
	service->is_active = TRUE;
}

static void
go_plugin_service_general_deactivate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GOPluginServiceGeneral *service_general = GO_PLUGIN_SERVICE_GENERAL (service);
	GOErrorInfo *error = NULL;

	GO_INIT_RET_ERROR_INFO (ret_error);
	g_return_if_fail (service_general->cbs.plugin_func_cleanup != NULL);
	service_general->cbs.plugin_func_cleanup (service, &error);
	if (error != NULL) {
		*ret_error = go_error_info_new_str_with_details (
		             _("Cleanup function inside plugin returned error."),
		             error);
		return;
	}
	service->is_active = FALSE;
}

static char *
go_plugin_service_general_get_description (GOPluginService *service)
{
	return g_strdup (_("General"));
}

static void
go_plugin_service_general_class_init (GObjectClass *gobject_class)
{
	GOPluginServiceClass *plugin_service_class = GO_PLUGIN_SERVICE_CLASS (gobject_class);

	plugin_service_class->activate = go_plugin_service_general_activate;
	plugin_service_class->deactivate = go_plugin_service_general_deactivate;
	plugin_service_class->get_description = go_plugin_service_general_get_description;
}

GSF_CLASS (GOPluginServiceGeneral, go_plugin_service_general,
           go_plugin_service_general_class_init, go_plugin_service_general_init,
           GO_TYPE_PLUGIN_SERVICE)

/****************************************************************************/

/*
 * GOPluginServiceResource
 */

typedef struct{
	GOPluginServiceClass plugin_service_class;
} GOPluginServiceResourceClass;

struct _GOPluginServiceResource {
	GOPluginService plugin_service;
	char *id;
	GString *value;
};

static GObjectClass *go_plugin_service_resource_parent_class;

static void
go_plugin_service_resource_init (GObject *obj)
{
}

static void
go_plugin_service_resource_finalize (GObject *obj)
{
	GOPluginServiceResource *sr = GO_PLUGIN_SERVICE_RESOURCE (obj);

	if (sr->value) {
		g_string_free (sr->value, TRUE);
		sr->value = NULL;
	}

	g_free (sr->id);
	sr->id = NULL;

	go_plugin_service_resource_parent_class->finalize (obj);
}

static void
go_plugin_service_resource_activate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GOPluginServiceResource *sr = GO_PLUGIN_SERVICE_RESOURCE (service);
	if (sr->value) {
		go_rsm_register_file (sr->id, sr->value->str, sr->value->len);
		service->is_active = TRUE;
	}
}


static void
go_plugin_service_resource_deactivate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GOPluginServiceResource *sr = GO_PLUGIN_SERVICE_RESOURCE (service);
	if (sr->value) {
		go_rsm_unregister_file (sr->id);
		service->is_active = FALSE;
	}
}

static char *
go_plugin_service_resource_get_description (GOPluginService *service)
{
	return g_strdup (_("Resource"));
}

static void
go_plugin_service_resource_read_xml (GOPluginService *service, xmlNode *tree, GOErrorInfo **ret_error)
{
	GOPluginServiceResource *sr = GO_PLUGIN_SERVICE_RESOURCE (service);
	char *data = NULL;
	gsize length;
	xmlChar *file;

	GO_INIT_RET_ERROR_INFO (ret_error);

	sr->id = xml2c (go_xml_node_get_cstr (tree, "id"));
	if (!sr->id)
		goto error;

	file = go_xml_node_get_cstr (tree, "file");
	if (file) {
		char *absfile;
		gboolean ok;

		if (!g_path_is_absolute (CXML2C (file))) {
			char const *dir = go_plugin_get_dir_name
				(go_plugin_service_get_plugin (service));
			absfile = g_build_filename (dir, CXML2C (file), NULL);
		} else
			absfile = g_strdup (CXML2C (file));
		xmlFree (file);
		ok = g_file_get_contents (absfile, &data, &length, NULL);
		g_free (absfile);

		if (!ok)
			goto error;
	} else {
		data = xml2c (go_xml_node_get_cstr (tree, "data"));
		length = strlen (data);
	}
	if (!data)
		goto error;

	/* No encoding case */
	sr->value = g_string_sized_new (length);
	g_string_append_len (sr->value, data, length);
	g_free (data);
	return;

 error:
	*ret_error = go_error_info_new_str (_("Invalid resource service"));
	g_free (data);
}

static void
go_plugin_service_resource_class_init (GObjectClass *gobject_class)
{
	GOPluginServiceClass *plugin_service_class = GO_PLUGIN_SERVICE_CLASS (gobject_class);

	go_plugin_service_resource_parent_class =
		g_type_class_peek_parent (gobject_class);

	gobject_class->finalize = go_plugin_service_resource_finalize;
	plugin_service_class->activate = go_plugin_service_resource_activate;
	plugin_service_class->deactivate = go_plugin_service_resource_deactivate;
	plugin_service_class->get_description = go_plugin_service_resource_get_description;
	plugin_service_class->read_xml = go_plugin_service_resource_read_xml;
}

GSF_CLASS (GOPluginServiceResource, go_plugin_service_resource,
           go_plugin_service_resource_class_init,
	   go_plugin_service_resource_init,
           GO_TYPE_PLUGIN_SERVICE)

/****************************************************************************/

/*
 * GOPluginServiceFileOpener
 */

typedef struct _GOPluginFileOpener GOPluginFileOpener;
static GOPluginFileOpener *go_plugin_file_opener_new (GOPluginService *service);

struct _InputFileSaveInfo {
	gchar *saver_id_str;
	GOFileFormatLevel format_level;
};

typedef struct _InputFileSaveInfo InputFileSaveInfo;


typedef struct{
	GOPluginServiceClass plugin_service_class;
} GOPluginServiceFileOpenerClass;

struct _GOPluginServiceFileOpener {
	GOPluginService plugin_service;

	gint priority;
	gboolean has_probe;
	gboolean encoding_dependent;
	gchar *description;
	GSList *suffixes;	/* list of char * */
	GSList *mimes;		/* list of char * */

	GOFileOpener *opener;
	GOPluginServiceFileOpenerCallbacks cbs;
};


static void
go_plugin_service_file_opener_init (GObject *obj)
{
	GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (obj);

	GO_PLUGIN_SERVICE (obj)->cbs_ptr = &service_file_opener->cbs;
	service_file_opener->description = NULL;
	service_file_opener->suffixes = NULL;
	service_file_opener->mimes = NULL;
	service_file_opener->opener = NULL;
	service_file_opener->cbs.plugin_func_file_probe = NULL;
	service_file_opener->cbs.plugin_func_file_open = NULL;
}

static void
go_plugin_service_file_opener_finalize (GObject *obj)
{
	GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (obj);
	GObjectClass *parent_class;

	g_free (service_file_opener->description);
	service_file_opener->description = NULL;
	g_slist_free_full (service_file_opener->suffixes, g_free);
	service_file_opener->suffixes = NULL;
	g_slist_free_full (service_file_opener->mimes, g_free);
	service_file_opener->mimes = NULL;
	if (service_file_opener->opener != NULL) {
		g_object_unref (service_file_opener->opener);
		service_file_opener->opener = NULL;
	}

	parent_class = g_type_class_peek (GO_TYPE_PLUGIN_SERVICE);
	parent_class->finalize (obj);
}

static void
go_plugin_service_file_opener_read_xml (GOPluginService *service, xmlNode *tree, GOErrorInfo **ret_error)
{
	int priority;
	gboolean has_probe;
	gboolean encoding_dependent;
	xmlNode *information_node;
	gchar *description;

	GO_INIT_RET_ERROR_INFO (ret_error);
	if (go_xml_node_get_int (tree, "priority", &priority))
		priority = CLAMP (priority, 0, 100);
	else
		priority = 50;

	if (!go_xml_node_get_bool (tree, "probe", &has_probe))
		has_probe = TRUE;
	if (!go_xml_node_get_bool (tree, "encoding_dependent", &encoding_dependent))
		encoding_dependent = FALSE;

	information_node = go_xml_get_child_by_name (tree, "information");
	if (information_node != NULL) {
		xmlNode *node = go_xml_get_child_by_name_by_lang
			(information_node, "description");
		description = node ? xml2c (xmlNodeGetContent (node)) : NULL;
	} else {
		description = NULL;
	}
	if (description != NULL) {
		GSList *suffixes = NULL, *mimes = NULL;
		xmlNode *list, *node;
		GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (service);

		list = go_xml_get_child_by_name (tree, "suffixes");
		if (list != NULL) {
			for (node = list->xmlChildrenNode; node != NULL; node = node->next) {
				char *tmp;

				if (strcmp (node->name, "suffix"))
					continue;

				tmp = xml2c (xmlNodeGetContent (node));
				if (!tmp)
					continue;

				GO_SLIST_PREPEND (suffixes, tmp);
			}
		}
		GO_SLIST_REVERSE (suffixes);

		list = go_xml_get_child_by_name (tree, "mime-types");
		if (list != NULL) {
			for (node = list->xmlChildrenNode; node != NULL; node = node->next) {
				char *tmp;

				if (strcmp (node->name, "mime-type"))
					continue;

				tmp = xml2c (xmlNodeGetContent (node));
				if (!tmp)
					continue;

				GO_SLIST_PREPEND (mimes, tmp);
			}
		}
		GO_SLIST_REVERSE (mimes);

		service_file_opener->priority = priority;
		service_file_opener->has_probe = has_probe;
		service_file_opener->encoding_dependent	= encoding_dependent;
		service_file_opener->description = description;
		service_file_opener->suffixes	= suffixes;
		service_file_opener->mimes	= mimes;
	} else {
		*ret_error = go_error_info_new_str (_("File opener has no description"));
	}
}

static void
go_plugin_service_file_opener_activate (GOPluginService *service,
					GOErrorInfo **ret_error)
{
	GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (service);

	GO_INIT_RET_ERROR_INFO (ret_error);
	service_file_opener->opener = GO_FILE_OPENER (go_plugin_file_opener_new (service));
	go_file_opener_register (service_file_opener->opener,
				  service_file_opener->priority);
	service->is_active = TRUE;
}

static void
go_plugin_service_file_opener_deactivate (GOPluginService *service,
					  GOErrorInfo **ret_error)
{
	GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (service);

	GO_INIT_RET_ERROR_INFO (ret_error);
	go_file_opener_unregister (service_file_opener->opener);
	service->is_active = FALSE;
}

static char *
go_plugin_service_file_opener_get_description (GOPluginService *service)
{
	GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (service);

	return g_strdup_printf (
		_("File opener - %s"), service_file_opener->description);
}

static void
go_plugin_service_file_opener_class_init (GObjectClass *gobject_class)
{
	GOPluginServiceClass *plugin_service_class = GO_PLUGIN_SERVICE_CLASS (gobject_class);

	gobject_class->finalize = go_plugin_service_file_opener_finalize;
	plugin_service_class->read_xml = go_plugin_service_file_opener_read_xml;
	plugin_service_class->activate = go_plugin_service_file_opener_activate;
	plugin_service_class->deactivate = go_plugin_service_file_opener_deactivate;
	plugin_service_class->get_description = go_plugin_service_file_opener_get_description;
}

GSF_CLASS (GOPluginServiceFileOpener, go_plugin_service_file_opener,
           go_plugin_service_file_opener_class_init,
	   go_plugin_service_file_opener_init,
           GO_TYPE_PLUGIN_SERVICE)


/*** GOPluginFileOpener class ***/

#define TYPE_GO_PLUGIN_FILE_OPENER             (go_plugin_file_opener_get_type ())
#define GO_PLUGIN_FILE_OPENER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_GO_PLUGIN_FILE_OPENER, GOPluginFileOpener))
#define GO_PLUGIN_FILE_OPENER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GO_PLUGIN_FILE_OPENER, GOPluginFileOpenerClass))
#define GO_IS_PLUGIN_FILE_OPENER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_GO_PLUGIN_FILE_OPENER))

GType go_plugin_file_opener_get_type (void);

typedef struct {
	GOFileOpenerClass parent_class;
} GOPluginFileOpenerClass;

struct _GOPluginFileOpener {
	GOFileOpener parent;

	GOPluginService *service;
};

static void
go_plugin_file_opener_init (GOPluginFileOpener *fo)
{
	fo->service = NULL;
}

static gboolean
go_plugin_file_opener_can_probe (GOFileOpener const *fo, GOFileProbeLevel pl)
{
	GOPluginFileOpener *pfo = GO_PLUGIN_FILE_OPENER (fo);
	GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (pfo->service);
	if (pl == GO_FILE_PROBE_FILE_NAME)
		return service_file_opener->suffixes != NULL;
	return service_file_opener->has_probe;
}

static gboolean
go_plugin_file_opener_probe (GOFileOpener const *fo, GsfInput *input,
                               GOFileProbeLevel pl)
{
	GOPluginFileOpener *pfo = GO_PLUGIN_FILE_OPENER (fo);
	GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (pfo->service);

	g_return_val_if_fail (GSF_IS_INPUT (input), FALSE);

	if (pl == GO_FILE_PROBE_FILE_NAME && service_file_opener->suffixes != NULL) {
		GSList *ptr;
		gchar const *extension;
		gchar *lowercase_extension;

		if (gsf_input_name (input) == NULL)
			return FALSE;
		extension = gsf_extension_pointer (gsf_input_name (input));
		if (extension == NULL)
			return FALSE;

		lowercase_extension = g_utf8_strdown (extension, -1);
		for (ptr = service_file_opener->suffixes; ptr != NULL ; ptr = ptr->next)
			if (0 == strcmp (lowercase_extension, ptr->data))
				break;
		g_free (lowercase_extension);
		return ptr != NULL;
	}

	if (service_file_opener->has_probe) {
		GOErrorInfo *ignored_error = NULL;

		go_plugin_service_load (pfo->service, &ignored_error);
		if (ignored_error != NULL) {
			go_error_info_print (ignored_error);
			go_error_info_free (ignored_error);
			return FALSE;
		} else if (service_file_opener->cbs.plugin_func_file_probe == NULL) {
			return FALSE;
		} else {
			gboolean res = service_file_opener->cbs.plugin_func_file_probe (fo, pfo->service, input, pl);
			gsf_input_seek (input, 0, G_SEEK_SET);
			return res;
		}
	} else {
		return FALSE;
	}
}

static void
go_plugin_file_opener_open (GOFileOpener const *fo, gchar const *enc,
			     GOIOContext *io_context,
			     GoView *view,
			     GsfInput *input)

{
	GOPluginFileOpener *pfo = GO_PLUGIN_FILE_OPENER (fo);
	GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (pfo->service);
	GOErrorInfo *error = NULL;

	g_return_if_fail (GSF_IS_INPUT (input));

	go_plugin_service_load (pfo->service, &error);
	if (error != NULL) {
		go_io_error_info_set (io_context, error);
		go_io_error_push (io_context, go_error_info_new_str (
		                        _("Error while reading file.")));
		return;
	}

	g_return_if_fail (service_file_opener->cbs.plugin_func_file_open != NULL);
	service_file_opener->cbs.plugin_func_file_open (fo, pfo->service, io_context, view, input, enc);
}

static void
go_plugin_file_opener_class_init (GOPluginFileOpenerClass *klass)
{
	GOFileOpenerClass *go_file_opener_klass = GO_FILE_OPENER_CLASS (klass);

	go_file_opener_klass->can_probe = go_plugin_file_opener_can_probe;
	go_file_opener_klass->probe = go_plugin_file_opener_probe;
	go_file_opener_klass->open = go_plugin_file_opener_open;
}

GSF_CLASS (GOPluginFileOpener, go_plugin_file_opener,
	   go_plugin_file_opener_class_init, go_plugin_file_opener_init,
	   GO_TYPE_FILE_OPENER)

static GSList *
go_str_slist_dup (GSList *l)
{
	GSList *res = NULL;
	for ( ; l != NULL ; l = l->next)
		res = g_slist_prepend (res, g_strdup (l->data));
	return g_slist_reverse (res);
}

static GOPluginFileOpener *
go_plugin_file_opener_new (GOPluginService *service)
{
	GOPluginServiceFileOpener *service_file_opener = GO_PLUGIN_SERVICE_FILE_OPENER (service);
	GOPluginFileOpener *fo;
	gchar *opener_id;

	opener_id = g_strconcat (
		go_plugin_get_id (service->plugin), ":", service->id, NULL);
	fo = GO_PLUGIN_FILE_OPENER (g_object_new (TYPE_GO_PLUGIN_FILE_OPENER, NULL));
	go_file_opener_setup (GO_FILE_OPENER (fo), opener_id,
		service_file_opener->description,
		go_str_slist_dup (service_file_opener->suffixes),
		go_str_slist_dup (service_file_opener->mimes),
		service_file_opener->encoding_dependent, NULL, NULL);
	fo->service = service;
	g_free (opener_id);

	return fo;
}

/*** -- ***/


/*
 * GOPluginServiceFileSaver
 */

typedef struct _GOPluginFileSaver GOPluginFileSaver;
static GOPluginFileSaver *go_plugin_file_saver_new (GOPluginService *service);


typedef struct{
	GOPluginServiceClass plugin_service_class;
} GOPluginServiceFileSaverClass;

struct _GOPluginServiceFileSaver {
	GOPluginService plugin_service;

	gchar *file_extension;
	gchar *mime_type;
	GOFileFormatLevel format_level;
	gchar *description;
	gint   default_saver_priority;
	GOFileSaveScope save_scope;
	gboolean overwrite_files;
	gboolean interactive_only;
	gboolean sheet_selection;

	GOFileSaver *saver;
	GOPluginServiceFileSaverCallbacks cbs;
};


static void
go_plugin_service_file_saver_init (GObject *obj)
{
	GOPluginServiceFileSaver *service_file_saver = GO_PLUGIN_SERVICE_FILE_SAVER (obj);

	GO_PLUGIN_SERVICE (obj)->cbs_ptr = &service_file_saver->cbs;
	service_file_saver->file_extension = NULL;
	service_file_saver->mime_type = NULL;
	service_file_saver->description = NULL;
	service_file_saver->cbs.plugin_func_file_save = NULL;
	service_file_saver->saver = NULL;
}

static void
go_plugin_service_file_saver_finalize (GObject *obj)
{
	GOPluginServiceFileSaver *service_file_saver = GO_PLUGIN_SERVICE_FILE_SAVER (obj);
	GObjectClass *parent_class;

	g_free (service_file_saver->file_extension);
	g_free (service_file_saver->mime_type);
	g_free (service_file_saver->description);
	if (service_file_saver->saver != NULL)
		g_object_unref (service_file_saver->saver);

	parent_class = g_type_class_peek (GO_TYPE_PLUGIN_SERVICE);
	parent_class->finalize (obj);
}

static void
go_plugin_service_file_saver_read_xml (GOPluginService *service, xmlNode *tree, GOErrorInfo **ret_error)
{
	xmlNode *information_node;
	gchar *description;

	GO_INIT_RET_ERROR_INFO (ret_error);
	information_node = go_xml_get_child_by_name (tree, "information");
	if (information_node != NULL) {
		xmlNode *node = go_xml_get_child_by_name_by_lang
			(information_node, "description");
		description = node ? xml2c (xmlNodeGetContent (node)) : NULL;
	} else {
		description = NULL;
	}

	if (description != NULL) {
		int scope = GO_FILE_SAVE_WORKBOOK;
		int level = GO_FILE_FL_WRITE_ONLY;
		GOPluginServiceFileSaver *psfs =
			GO_PLUGIN_SERVICE_FILE_SAVER (service);

		psfs->file_extension =
			xml2c (go_xml_node_get_cstr (tree, "file_extension"));

		psfs->mime_type =
			xml2c (go_xml_node_get_cstr (tree, "mime_type"));

		psfs->description = description;

		(void)go_xml_node_get_enum
			(tree, "format_level",
			 GO_TYPE_FILE_FORMAT_LEVEL, &level);
		psfs->format_level = (GOFileFormatLevel)level;

		if (!go_xml_node_get_int (tree, "default_saver_priority", &(psfs->default_saver_priority)))
			psfs->default_saver_priority = -1;

		(void)go_xml_node_get_enum
			(tree, "save_scope",
			 GO_TYPE_FILE_SAVE_SCOPE, &scope);
		psfs->save_scope = (GOFileSaveScope)scope;

		if (!go_xml_node_get_bool (tree, "overwrite_files", &(psfs->overwrite_files)))
			psfs->overwrite_files = TRUE;

		if (!go_xml_node_get_bool (tree, "interactive_only", &(psfs->interactive_only)))
			psfs->interactive_only = FALSE;

		if (!go_xml_node_get_bool (tree, "sheet_selection", &(psfs->sheet_selection)))
			psfs->sheet_selection = FALSE;
	} else {
		*ret_error = go_error_info_new_str (_("File saver has no description"));
	}
}

static void
go_plugin_service_file_saver_activate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GOPluginServiceFileSaver *service_file_saver = GO_PLUGIN_SERVICE_FILE_SAVER (service);
	GHashTable *file_savers_hash;

	GO_INIT_RET_ERROR_INFO (ret_error);
	service_file_saver->saver = GO_FILE_SAVER (go_plugin_file_saver_new (service));
	if (service_file_saver->default_saver_priority < 0) {
		go_file_saver_register (service_file_saver->saver);
	} else {
		go_file_saver_register_as_default (service_file_saver->saver,
						   service_file_saver->default_saver_priority);
	}
	file_savers_hash = get_plugin_file_savers_hash (service->plugin);
	g_assert (g_hash_table_lookup (file_savers_hash, service->id) == NULL);
	g_hash_table_insert (file_savers_hash, g_strdup (service->id), service_file_saver->saver);
	service->is_active = TRUE;
}

static void
go_plugin_service_file_saver_deactivate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GOPluginServiceFileSaver *service_file_saver = GO_PLUGIN_SERVICE_FILE_SAVER (service);
	GHashTable *file_savers_hash;

	GO_INIT_RET_ERROR_INFO (ret_error);
	file_savers_hash = get_plugin_file_savers_hash (service->plugin);
	g_hash_table_remove (file_savers_hash, service->id);
	go_file_saver_unregister (service_file_saver->saver);
	service->is_active = FALSE;
}

static char *
go_plugin_service_file_saver_get_description (GOPluginService *service)
{
	GOPluginServiceFileSaver *service_file_saver = GO_PLUGIN_SERVICE_FILE_SAVER (service);

	return g_strdup_printf (
		_("File saver - %s"), service_file_saver->description);
}

static void
go_plugin_service_file_saver_class_init (GObjectClass *gobject_class)
{
	GOPluginServiceClass *plugin_service_class = GO_PLUGIN_SERVICE_CLASS (gobject_class);

	gobject_class->finalize = go_plugin_service_file_saver_finalize;
	plugin_service_class->read_xml = go_plugin_service_file_saver_read_xml;
	plugin_service_class->activate = go_plugin_service_file_saver_activate;
	plugin_service_class->deactivate = go_plugin_service_file_saver_deactivate;
	plugin_service_class->get_description = go_plugin_service_file_saver_get_description;
}

GSF_CLASS (GOPluginServiceFileSaver, go_plugin_service_file_saver,
           go_plugin_service_file_saver_class_init, go_plugin_service_file_saver_init,
           GO_TYPE_PLUGIN_SERVICE)


/*** GOPluginFileSaver class ***/

#define TYPE_GO_PLUGIN_FILE_SAVER             (go_plugin_file_saver_get_type ())
#define GO_PLUGIN_FILE_SAVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_GO_PLUGIN_FILE_SAVER, GOPluginFileSaver))
#define GO_PLUGIN_FILE_SAVER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_GO_PLUGIN_FILE_SAVER, GOPluginFileSaverClass))
#define GO_IS_PLUGIN_FILE_SAVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_GO_PLUGIN_FILE_SAVER))

GType go_plugin_file_saver_get_type (void);

typedef struct {
	GOFileSaverClass parent_class;
} GOPluginFileSaverClass;

struct _GOPluginFileSaver {
	GOFileSaver parent;

	GOPluginService *service;
};

static void
go_plugin_file_saver_init (GOPluginFileSaver *fs)
{
	fs->service = NULL;
}

static void
go_plugin_file_saver_save (GOFileSaver const *fs, GOIOContext *io_context,
			    GoView const *view,
			    GsfOutput *output)
{
	GOPluginFileSaver *pfs = GO_PLUGIN_FILE_SAVER (fs);
	GOPluginServiceFileSaver *service_file_saver = GO_PLUGIN_SERVICE_FILE_SAVER (pfs->service);
	GOErrorInfo *error = NULL;

	g_return_if_fail (GSF_IS_OUTPUT (output));

	go_plugin_service_load (pfs->service, &error);
	if (error != NULL) {
		go_io_error_info_set (io_context, error);
		go_io_error_push (io_context, go_error_info_new_str (
		                        _("Error while loading plugin for saving.")));
		if (!gsf_output_error (output))
			gsf_output_set_error (output, 0, _("Failed to load plugin for saving"));
		return;
	}

	g_return_if_fail (service_file_saver->cbs.plugin_func_file_save != NULL);
	service_file_saver->cbs.plugin_func_file_save (fs, pfs->service, io_context, view, output);
}

static void
go_plugin_file_saver_class_init (GOPluginFileSaverClass *klass)
{
	GOFileSaverClass *go_file_saver_klass = GO_FILE_SAVER_CLASS (klass);

	go_file_saver_klass->save = go_plugin_file_saver_save;
}

GSF_CLASS (GOPluginFileSaver, go_plugin_file_saver,
	   go_plugin_file_saver_class_init, go_plugin_file_saver_init,
	   GO_TYPE_FILE_SAVER)

static GOPluginFileSaver *
go_plugin_file_saver_new (GOPluginService *service)
{
	GOPluginFileSaver *pfs;
	GOPluginServiceFileSaver *psfs = GO_PLUGIN_SERVICE_FILE_SAVER (service);
	gchar *saver_id;

	saver_id = g_strconcat (go_plugin_get_id (service->plugin),
				":",
				service->id,
				NULL);
	pfs = GO_PLUGIN_FILE_SAVER (g_object_new
				   (TYPE_GO_PLUGIN_FILE_SAVER,
				    "id", saver_id,
				    "extension", psfs->file_extension,
				    "mime-type", psfs->mime_type,
				    "description", psfs->description,
				    "format-level", psfs->format_level,
				    "overwrite", psfs->overwrite_files,
				    "interactive-only", psfs->interactive_only,
				    "scope", psfs->save_scope,
				    "sheet-selection", psfs->sheet_selection,
				    NULL));

	pfs->service = service;
	g_free (saver_id);

	return pfs;
}

/*
 * GOPluginServicePluginLoader
 */

typedef struct{
	GOPluginServiceClass plugin_service_class;
} GOPluginServicePluginLoaderClass;

struct _GOPluginServicePluginLoader {
	GOPluginService plugin_service;
	GOPluginServicePluginLoaderCallbacks cbs;
};


static void
go_plugin_service_plugin_loader_init (GObject *obj)
{
	GOPluginServicePluginLoader *service_plugin_loader = GO_PLUGIN_SERVICE_PLUGIN_LOADER (obj);

	GO_PLUGIN_SERVICE (obj)->cbs_ptr = &service_plugin_loader->cbs;
}

GType
go_plugin_service_plugin_loader_generate_type (GOPluginService *service,
					       GOErrorInfo **ret_error)
{
	GOPluginServicePluginLoader *service_plugin_loader = GO_PLUGIN_SERVICE_PLUGIN_LOADER (service);
	GOErrorInfo *error = NULL;
	GType loader_type;

	GO_INIT_RET_ERROR_INFO (ret_error);
	go_plugin_service_load (service, &error);
	if (error == NULL) {
		loader_type = service_plugin_loader->cbs.plugin_func_get_loader_type (
			service, &error);
		if (error == NULL)
			return loader_type;
		*ret_error = error;
	} else {
		*ret_error = go_error_info_new_str_with_details (
		             _("Error while loading plugin service."),
		             error);
	}
	return G_TYPE_NONE;
}

static void
go_plugin_service_plugin_loader_activate (GOPluginService *service,
					  GOErrorInfo **ret_error)
{
	gchar *full_id;

	GO_INIT_RET_ERROR_INFO (ret_error);
	full_id = g_strconcat (
		go_plugin_get_id (service->plugin), ":", service->id, NULL);
	go_plugins_register_loader (full_id, service);
	g_free (full_id);
	service->is_active = TRUE;
}

static void
go_plugin_service_plugin_loader_deactivate (GOPluginService *service,
					    GOErrorInfo **ret_error)
{
	gchar *full_id;

	GO_INIT_RET_ERROR_INFO (ret_error);
	full_id = g_strconcat (
		go_plugin_get_id (service->plugin), ":", service->id, NULL);
	go_plugins_unregister_loader (full_id);
	g_free (full_id);
	service->is_active = FALSE;
}

static char *
go_plugin_service_plugin_loader_get_description (GOPluginService *service)
{
	return g_strdup (_("Plugin loader"));
}

static void
go_plugin_service_plugin_loader_class_init (GObjectClass *gobject_class)
{
	GOPluginServiceClass *plugin_service_class = GO_PLUGIN_SERVICE_CLASS (gobject_class);

	plugin_service_class->activate = go_plugin_service_plugin_loader_activate;
	plugin_service_class->deactivate = go_plugin_service_plugin_loader_deactivate;
	plugin_service_class->get_description = go_plugin_service_plugin_loader_get_description;
}

GSF_CLASS (GOPluginServicePluginLoader, go_plugin_service_plugin_loader,
           go_plugin_service_plugin_loader_class_init,
	   go_plugin_service_plugin_loader_init,
           GO_TYPE_PLUGIN_SERVICE)

/**************************************************************************
 * GOPluginServiceGObjectLoader
 */

static char *
go_plugin_service_gobject_loader_get_description (GOPluginService *service)
{
	return g_strdup (_("GObject loader"));
}

static void
go_plugin_service_gobject_loader_read_xml (GOPluginService *service,
					G_GNUC_UNUSED xmlNode *tree,
					G_GNUC_UNUSED GOErrorInfo **ret_error)
{
	GOPluginServiceGObjectLoaderClass *gobj_loader_class = GO_PLUGIN_SERVICE_GOBJECT_LOADER_GET_CLASS (service);
	g_return_if_fail (gobj_loader_class->pending != NULL);
	g_hash_table_replace (gobj_loader_class->pending, service->id, service);
}

static void
go_plugin_service_gobject_loader_class_init (GOPluginServiceGObjectLoaderClass *gobj_loader_class)
{
	GOPluginServiceClass *psc = GO_PLUGIN_SERVICE_CLASS (gobj_loader_class);

	psc->get_description	= go_plugin_service_gobject_loader_get_description;
	psc->read_xml		= go_plugin_service_gobject_loader_read_xml;
	gobj_loader_class->pending = NULL;
}

GSF_CLASS (GOPluginServiceGObjectLoader, go_plugin_service_gobject_loader,
           go_plugin_service_gobject_loader_class_init, NULL,
           GO_TYPE_PLUGIN_SERVICE_SIMPLE)

/**************************************************************************
 * GOPluginServiceSimple
 */

static void
go_plugin_service_simple_activate (GOPluginService *service, GOErrorInfo **ret_error)
{
	service->is_active = TRUE;
}

static void
go_plugin_service_simple_deactivate (GOPluginService *service, GOErrorInfo **ret_error)
{
	service->is_active = FALSE;
}

static void
go_plugin_service_simple_class_init (GObjectClass *gobject_class)
{
	GOPluginServiceClass *psc = GO_PLUGIN_SERVICE_CLASS (gobject_class);

	psc->activate		= go_plugin_service_simple_activate;
	psc->deactivate		= go_plugin_service_simple_deactivate;
}

GSF_CLASS (GOPluginServiceSimple, go_plugin_service_simple,
           go_plugin_service_simple_class_init,
	   NULL,
           GO_TYPE_PLUGIN_SERVICE)

/* ---------------------------------------------------------------------- */

void
go_plugin_service_load (GOPluginService *service, GOErrorInfo **ret_error)
{
	g_return_if_fail (GO_IS_PLUGIN_SERVICE (service));

	GO_INIT_RET_ERROR_INFO (ret_error);

	if (service->is_loaded)
		return;
	go_plugin_load_service (service->plugin, service, ret_error);
	if (*ret_error == NULL)
		service->is_loaded = TRUE;
}

void
go_plugin_service_unload (GOPluginService *service, GOErrorInfo **ret_error)
{
	GOErrorInfo *error = NULL;

	g_return_if_fail (GO_IS_PLUGIN_SERVICE (service));

	GO_INIT_RET_ERROR_INFO (ret_error);
	if (!service->is_loaded) {
		return;
	}
	go_plugin_unload_service (service->plugin, service, &error);
	if (error == NULL) {
		service->is_loaded = FALSE;
	} else {
		*ret_error = error;
	}
}

GOPluginService *
go_plugin_service_new (GOPlugin *plugin, xmlNode *tree, GOErrorInfo **ret_error)
{
	GOPluginService *service = NULL;
	char *type_str;
	GOErrorInfo *service_error = NULL;
	GOPluginServiceCreate ctor;

	g_return_val_if_fail (GO_IS_PLUGIN (plugin), NULL);
	g_return_val_if_fail (tree != NULL, NULL);
	g_return_val_if_fail (strcmp (tree->name, "service") == 0, NULL);

	GO_INIT_RET_ERROR_INFO (ret_error);
	type_str = go_xml_node_get_cstr (tree, "type");
	if (type_str == NULL) {
		*ret_error = go_error_info_new_str (_("No \"type\" attribute on \"service\" element."));
		return NULL;
	}

	ctor = g_hash_table_lookup (services, type_str);
	if (ctor == NULL) {
		*ret_error = go_error_info_new_printf (_("Unknown service type: %s."), type_str);
		xmlFree (type_str);
		return NULL;
	}
	xmlFree (type_str);

	service = g_object_new (ctor(), NULL);
	service->plugin = plugin;
	service->id = xml2c (go_xml_node_get_cstr (tree, "id"));
	if (service->id == NULL)
		service->id = xmlStrdup ("default");

	if (GO_PLUGIN_SERVICE_GET_CLASS (service)->read_xml != NULL) {
		GO_PLUGIN_SERVICE_GET_CLASS (service)->read_xml (service, tree, &service_error);
		if (service_error != NULL) {
			*ret_error = go_error_info_new_str_with_details (
				_("Error reading service information."), service_error);
			g_object_unref (service);
			service = NULL;
		}
	}

	return service;
}

char const *
go_plugin_service_get_id (const GOPluginService *service)
{
	g_return_val_if_fail (GO_IS_PLUGIN_SERVICE (service), NULL);

	return service->id;
}

char const *
go_plugin_service_get_description (GOPluginService *service)
{
	g_return_val_if_fail (GO_IS_PLUGIN_SERVICE (service), NULL);

	if (service->saved_description == NULL) {
		service->saved_description = GO_PLUGIN_SERVICE_GET_CLASS (service)->get_description (service);
	}

	return service->saved_description;
}

/**
 * go_plugin_service_get_plugin:
 * @service: #GOPluginService
 *
 * Returns: (transfer none): the plugin offering @service
 **/
GOPlugin *
go_plugin_service_get_plugin (GOPluginService *service)
{
	g_return_val_if_fail (GO_IS_PLUGIN_SERVICE (service), NULL);

	return service->plugin;
}

/**
 * go_plugin_service_get_cbs:
 * @service: #GOPluginService
 *
 * Returns: (transfer none): the callbacks for the service
 **/
gpointer
go_plugin_service_get_cbs (GOPluginService *service)
{
	g_return_val_if_fail (GO_IS_PLUGIN_SERVICE (service), NULL);
	g_return_val_if_fail (service->cbs_ptr != NULL, NULL);

	return service->cbs_ptr;
}

void
go_plugin_service_activate (GOPluginService *service, GOErrorInfo **ret_error)
{
	g_return_if_fail (GO_IS_PLUGIN_SERVICE (service));

	GO_INIT_RET_ERROR_INFO (ret_error);
	if (service->is_active) {
		return;
	}
#ifdef PLUGIN_ALWAYS_LOAD
	{
		GOErrorInfo *load_error = NULL;

		go_plugin_service_load (service, &load_error);
		if (load_error != NULL) {
			*ret_error = go_error_info_new_str_with_details (
				_("We must load service before activating it (PLUGIN_ALWAYS_LOAD is set) "
				  "but loading failed."), load_error);
			return;
		}
	}
#endif
	GO_PLUGIN_SERVICE_GET_CLASS (service)->activate (service, ret_error);
}

void
go_plugin_service_deactivate (GOPluginService *service, GOErrorInfo **ret_error)
{
	g_return_if_fail (GO_IS_PLUGIN_SERVICE (service));

	GO_INIT_RET_ERROR_INFO (ret_error);
	if (!service->is_active) {
		return;
	}
	GO_PLUGIN_SERVICE_GET_CLASS (service)->deactivate (service, ret_error);
	if (*ret_error == NULL) {
		GOErrorInfo *ignored_error = NULL;

		service->is_active = FALSE;
		/* FIXME */
		go_plugin_service_unload (service, &ignored_error);
		go_error_info_free (ignored_error);
	}
}

/*****************************************************************************/

void
_go_plugin_services_init (void)
{
	static struct {
		char const *type_str;
		GOPluginServiceCreate ctor;
	} const builtin_services[] = {
		{ "general",	      go_plugin_service_general_get_type},
		{ "resource",         go_plugin_service_resource_get_type},
		{ "file_opener",      go_plugin_service_file_opener_get_type},
		{ "file_saver",	      go_plugin_service_file_saver_get_type},
		{ "plugin_loader",    go_plugin_service_plugin_loader_get_type},
/* base classes, not really for direct external use,
 * put here for expositional purposes
 */
#if 0
		{ "gobject_loader",   go_plugin_service_gobject_loader_get_type}
		{ "simple",	      go_plugin_service_simple_get_type}
#endif
	};
	unsigned i;

	g_return_if_fail (services == NULL);

	services = g_hash_table_new (g_str_hash, g_str_equal);
	for (i = 0; i < G_N_ELEMENTS (builtin_services); i++)
		go_plugin_service_define (builtin_services[i].type_str,
					  builtin_services[i].ctor);
}

void
go_plugin_services_shutdown (void)
{
	g_return_if_fail (services != NULL);
	g_hash_table_destroy (services);
	services = NULL;
}

/**
 * go_plugin_service_define:
 * @type_str:  char const *
 * @ctor: (scope async): #GOPluginServiceCreate
 *
 * Allow the definition of new service types
 **/
void
go_plugin_service_define (char const *type_str, GOPluginServiceCreate ctor)
{
	g_return_if_fail (services != NULL);

	g_return_if_fail (NULL == g_hash_table_lookup (services, type_str));

	g_hash_table_insert (services, (gpointer)type_str, ctor);
}
