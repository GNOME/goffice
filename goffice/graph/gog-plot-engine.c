/*
 * go-plot-engine.c :
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
#include <goffice/graph/gog-plot-engine.h>
#include <goffice/graph/gog-plot-impl.h>
#include <goffice/graph/gog-theme.h>
#include <goffice/graph/gog-trend-line.h>
#include <goffice/graph/gog-chart.h>
#include <goffice/app/go-plugin-service.h>
#include <goffice/app/go-plugin-service-impl.h>
#include <goffice/app/error-info.h>
#include <goffice/utils/go-libxml-extras.h>

#include <glib/gi18n-lib.h>
#include <gsf/gsf-impl-utils.h>
#include <string.h>

/**
 * GogPlotFamily:
 * @name: family name.
 * @sample_image_file: sample image for the graph editor.
 * @priority: priority.
 * @axis_set: #GogAxisSet used.
 * @types: known types in the family.
 *
 * Plot types family.
 **/

/**
 * GogPlotType:
 * @family: plot family.
 * @engine: plot engine.
 * @name: plot type name.
 * @sample_image_file: sample image for the graph editor.
 * @description: untranslated description.
 * @col: column where the plot type appears in the table inside the graph editor.
 * @row: row.
 * @properties: plot type properties.
 **/

static gboolean debug;

static GSList *refd_plugins;

/***************************************************************************/
/* Support plot engines in plugins */

#define GOG_TYPE_PLOT_ENGINE_SERVICE  (gog_plot_engine_service_get_type ())
#define GOG_PLOT_ENGINE_SERVICE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_PLOT_ENGINE_SERVICE, GogPlotEngineService))
#define GOG_IS_PLOT_ENGINE_SERVICE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_PLOT_ENGINE_SERVICE))

static GType gog_plot_engine_service_get_type (void);

typedef GOPluginServiceGObjectLoader	GogPlotEngineService;
typedef GOPluginServiceGObjectLoaderClass GogPlotEngineServiceClass;

static GHashTable *pending_engines = NULL;

static char *
gog_plot_engine_service_get_description (GOPluginService *service)
{
	return g_strdup (_("Plot Engine"));
}

static void
gog_plot_engine_service_class_init (GOPluginServiceGObjectLoaderClass *gobj_loader_class)
{
	GOPluginServiceClass *ps_class = GO_PLUGIN_SERVICE_CLASS (gobj_loader_class);

	ps_class->get_description = gog_plot_engine_service_get_description;

	gobj_loader_class->pending =
		pending_engines = g_hash_table_new (g_str_hash, g_str_equal);
}

GSF_CLASS (GogPlotEngineService, gog_plot_engine_service,
           gog_plot_engine_service_class_init, NULL,
           GO_TYPE_PLUGIN_SERVICE_GOBJECT_LOADER)

GogPlot *
gog_plot_new_by_name (char const *id)
{
	GType type = g_type_from_name (id);

	if (type == 0) {
		GOErrorInfo *err = NULL;
		GOPluginService *service =
			pending_engines
			? g_hash_table_lookup (pending_engines, id)
			: NULL;
		GOPlugin *plugin;

		if (!service || !service->is_active)
			return NULL;

		g_return_val_if_fail (!service->is_loaded, NULL);

		go_plugin_service_load (service, &err);
		type = g_type_from_name (id);

		if (err != NULL) {
			go_error_info_print (err);
			go_error_info_free	(err);
		}

		g_return_val_if_fail (type != 0, NULL);

		/*
		 * The plugin defined a gtype so it must not be unloaded.
		 */
		plugin = go_plugin_service_get_plugin (service);
		refd_plugins = g_slist_prepend (refd_plugins, plugin);
		g_object_ref (plugin);
		go_plugin_use_ref (plugin);
	}
	g_return_val_if_fail (g_type_is_a (type, GOG_TYPE_PLOT), NULL);

	return g_object_new (type, NULL);
}

/***************************************************************************/
/* Use a plugin service to define where to find plot types */

#define GOG_TYPE_PLOT_TYPE_SERVICE  (gog_plot_type_service_get_type ())
#define GOG_PLOT_TYPE_SERVICE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_PLOT_TYPE_SERVICE, GogPlotTypeService))
#define GOG_IS_PLOT_TYPE_SERVICE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_PLOT_TYPE_SERVICE))

GType gog_plot_type_service_get_type (void);

typedef struct {
	GOPluginServiceSimple	base;

	GSList	*families, *types, *paths;
} GogPlotTypeService;

typedef struct{
	GOPluginServiceSimpleClass	base;
} GogPlotTypeServiceClass;

static GHashTable *pending_plot_type_files = NULL;
static GObjectClass *plot_type_parent_klass;

static void
cb_pending_plot_types_load (char const *path,
			    GogPlotTypeService *service,
			    G_GNUC_UNUSED gpointer ignored)
{
	xmlNode *ptr, *prop;
	xmlDoc *doc;
	GogPlotFamily *family = NULL;
	GogPlotType *type;
	int col, row, priority;
	xmlChar *name, *image_file, *description, *engine;
	xmlChar *axis_set_str;
	GogAxisSet axis_set;

	if (debug)
		g_printerr ("Loading %s\n", path);
	doc = go_xml_parse_file (path);
	g_return_if_fail (doc != NULL && doc->xmlRootNode != NULL);

	/* do the families before the types so that they are available */
	for (ptr = doc->xmlRootNode->xmlChildrenNode; ptr ; ptr = ptr->next)
		if (!xmlIsBlankNode (ptr) && ptr->name && !strcmp (ptr->name, "Family")) {
			name	    = xmlGetProp (ptr, "_name");
			if (name) {
				if (NULL == gog_plot_family_by_name  (name)) {
					image_file  = xmlGetProp (ptr, "sample_image_file");
					if (!go_xml_node_get_int (ptr, "priority", &priority))
						priority = 0;
					axis_set_str = xmlGetProp (ptr, "axis_set");
					axis_set = gog_axis_set_from_str (axis_set_str);
					if (axis_set_str != NULL)
						xmlFree (axis_set_str);
					else
						g_warning ("[GogPlotTypeService::plot_types_load] missing axis set type");
					family = gog_plot_family_register (name, image_file, priority, axis_set);
					if (family != NULL)
						service->families = g_slist_prepend (service->families, family);
					if (image_file != NULL) xmlFree (image_file);
				}
				xmlFree (name);
			}
		}

	for (ptr = doc->xmlRootNode->xmlChildrenNode; ptr ; ptr = ptr->next)
		if (!xmlIsBlankNode (ptr) && ptr->name && !strcmp (ptr->name, "Type")) {
			name   = xmlGetProp (ptr, "family");
			if (name != NULL) {
				family = gog_plot_family_by_name  (name);
				xmlFree (name);
				if (family == NULL)
					continue;
			}

			name	    = xmlGetProp (ptr, "_name");
			image_file  = xmlGetProp (ptr, "sample_image_file");
			description = xmlGetProp (ptr, "_description");
			engine	    = xmlGetProp (ptr, "engine");
			if (go_xml_node_get_int (ptr, "col", &col) &&
			    go_xml_node_get_int (ptr, "row", &row)) {
				type = gog_plot_type_register (family, col, row,
					name, image_file, description, engine);
				if (type != NULL) {
					service->types = g_slist_prepend (service->types, type);
					for (prop = ptr->xmlChildrenNode ; prop != NULL ; prop = prop->next)
						if (!xmlIsBlankNode (prop) &&
						    prop->name && !strcmp (prop->name, "property")) {
							xmlChar *prop_name = xmlGetProp (prop, "name");

							if (prop_name == NULL) {
								g_warning ("missing name for property entry");
								continue;
							}

							if (type->properties == NULL)
								type->properties =
									g_hash_table_new_full (g_str_hash, g_str_equal,
											       xmlFree, xmlFree);
							g_hash_table_replace (type->properties,
								prop_name, xmlNodeGetContent (prop));
						}
				}
			}
			if (name != NULL) xmlFree (name);
			if (image_file != NULL) xmlFree (image_file);
			if (description != NULL) xmlFree (description);
			if (engine != NULL) xmlFree (engine);
		}

	xmlFreeDoc (doc);
}

static void
pending_plot_types_load (void)
{
	if (pending_plot_type_files != NULL) {
		GHashTable *tmp = pending_plot_type_files;
		pending_plot_type_files = NULL;
		g_hash_table_foreach (tmp,
			(GHFunc) cb_pending_plot_types_load, NULL);
		g_hash_table_destroy (tmp);
	}
}

static void
gog_plot_type_service_read_xml (GOPluginService *service, xmlNode *tree, GOErrorInfo **ret_error)
{
	char    *path, *tmp;
	xmlNode *ptr;
	GSList *paths = NULL;

	for (ptr = tree->xmlChildrenNode; ptr != NULL; ptr = ptr->next)
		if (0 == xmlStrcmp (ptr->name, "file") &&
		    NULL != (path = xmlNodeGetContent (ptr))) {
			if (!g_path_is_absolute (path)) {
				char const *dir = go_plugin_get_dir_name (
					go_plugin_service_get_plugin (service));
				tmp = g_build_filename (dir, path, NULL);
			} else
				tmp = g_strdup (path);
			xmlFree (path);
			path = tmp;
			paths = g_slist_append (paths, path);
		}
	GOG_PLOT_TYPE_SERVICE (service)->paths = paths;
}

static void
gog_plot_type_service_activate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GSList *l = GOG_PLOT_TYPE_SERVICE (service)->paths;
	if (l && pending_plot_type_files == NULL)
		pending_plot_type_files = g_hash_table_new_full (
			g_str_hash, g_str_equal, g_free, g_object_unref);
	while (l) {
		g_object_ref (service);
		g_hash_table_replace (pending_plot_type_files, g_strdup (l->data), service);
		l = l->next;
	}
	service->is_active = TRUE;
}

static void
gog_plot_type_service_deactivate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GogPlotTypeService *plot_service = GOG_PLOT_TYPE_SERVICE (service);
	GSList *l = plot_service->families;
	while (l) {
		gog_plot_family_unregister ((GogPlotFamily *) l->data);
		l = l->next;
	}
	g_slist_free (plot_service->families);
	plot_service->families = NULL;

	g_slist_free (plot_service->types);
	plot_service->types = NULL;

	if (pending_plot_type_files) {
		GSList *l;
		for (l = plot_service->paths; l; l = l->next) {
			const char *path = l->data;
			g_hash_table_remove (pending_plot_type_files, path);
		}

		if (g_hash_table_size (pending_plot_type_files) == 0) {
			g_hash_table_destroy (pending_plot_type_files);
			pending_plot_type_files = NULL;
		}
	}
	service->is_active = FALSE;
}

static char *
gog_plot_type_service_get_description (GOPluginService *service)
{
	return g_strdup (_("Plot Type"));
}

static void gog_plot_family_free (GogPlotFamily *family);
static void gog_plot_type_free (GogPlotType *type);

static void
gog_plot_type_service_finalize (GObject *obj)
{
	GogPlotTypeService *service = GOG_PLOT_TYPE_SERVICE (obj);
	GSList *ptr;

	for (ptr = service->families ; ptr != NULL ; ptr = ptr->next)
		gog_plot_family_free ((GogPlotFamily *) ptr->data);
	g_slist_free (service->families);
	service->families = NULL;

	/* plot types are freed by gog_plot_family_free, so no need to
	free anything but the list here */
	g_slist_free (service->types);
	service->types = NULL;

	for (ptr = service->paths ; ptr != NULL ; ptr = ptr->next) {
		g_hash_table_remove (pending_engines, ptr->data);
		g_free (ptr->data);
	}
	g_slist_free (service->paths);
	service->paths = NULL;

	(plot_type_parent_klass->finalize) (obj);
}

static void
gog_plot_type_service_init (GObject *obj)
{
	GogPlotTypeService *service = GOG_PLOT_TYPE_SERVICE (obj);

	service->families = NULL;
	service->types = NULL;
	service->paths = NULL;
}

static void
gog_plot_type_service_class_init (GObjectClass *gobject_klass)
{
	GOPluginServiceClass *ps_class = GO_PLUGIN_SERVICE_CLASS (gobject_klass);

	plot_type_parent_klass = g_type_class_peek_parent (gobject_klass);
	gobject_klass->finalize		= gog_plot_type_service_finalize;
	ps_class->read_xml		= gog_plot_type_service_read_xml;
	ps_class->get_description	= gog_plot_type_service_get_description;
	ps_class->activate		= gog_plot_type_service_activate;
	ps_class->deactivate		= gog_plot_type_service_deactivate;
}

GSF_CLASS (GogPlotTypeService, gog_plot_type_service,
           gog_plot_type_service_class_init, gog_plot_type_service_init,
           GO_TYPE_PLUGIN_SERVICE_SIMPLE)

/***************************************************************************/
/* Support regression curves engines in plugins */

/**
 * GogTrendLineType:
 * @engine: trend line engine.
 * @name: trend line type name.
 * @description: untranslated description.
 * @properties: trend line type properties.
 **/

#define GOG_TYPE_TREND_LINE_ENGINE_SERVICE  (gog_trend_line_engine_service_get_type ())
#define GOG_TREND_LINE_ENGINE_SERVICE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_TREND_LINE_ENGINE_SERVICE, GogTrendLineEngineService))
#define GOG_IS_TREND_LINE_ENGINE_SERVICE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_TREND_LINE_ENGINE_SERVICE))

static GType gog_trend_line_engine_service_get_type (void);

typedef GOPluginServiceGObjectLoader	GogTrendLineEngineService;
typedef GOPluginServiceGObjectLoaderClass GogTrendLineEngineServiceClass;

static GHashTable *pending_trend_lines_engines = NULL;

static char *
gog_trend_line_engine_service_get_description (GOPluginService *service)
{
	return g_strdup (_("Regression Curve Engine"));
}

static void
gog_trend_line_engine_service_class_init (GOPluginServiceGObjectLoaderClass *gobj_loader_class)
{
	GOPluginServiceClass *ps_class = GO_PLUGIN_SERVICE_CLASS (gobj_loader_class);

	ps_class->get_description = gog_trend_line_engine_service_get_description;

	gobj_loader_class->pending =
		pending_trend_lines_engines = g_hash_table_new (g_str_hash, g_str_equal);
}

GSF_CLASS (GogTrendLineEngineService, gog_trend_line_engine_service,
           gog_trend_line_engine_service_class_init, NULL,
           GO_TYPE_PLUGIN_SERVICE_GOBJECT_LOADER)

GogTrendLine *
gog_trend_line_new_by_name (char const *id)
{
	GType type = g_type_from_name (id);

	if (type == 0) {
		GOErrorInfo *err = NULL;
		GOPluginService *service =
			pending_trend_lines_engines
			? g_hash_table_lookup (pending_trend_lines_engines, id)
			: NULL;
		GOPlugin *plugin;

		if (!service || !service->is_active)
			return NULL;

		g_return_val_if_fail (!service->is_loaded, NULL);

		go_plugin_service_load (service, &err);
		type = g_type_from_name (id);

		if (err != NULL) {
			go_error_info_print (err);
			go_error_info_free	(err);
		}

		g_return_val_if_fail (type != 0, NULL);

		/*
		 * The plugin defined a gtype so it must not be unloaded.
		 */
		plugin = go_plugin_service_get_plugin (service);
		refd_plugins = g_slist_prepend (refd_plugins, plugin);
		g_object_ref (plugin);
		go_plugin_use_ref (plugin);
	}
	g_return_val_if_fail (g_type_is_a (type, GOG_TYPE_TREND_LINE), NULL);

	return g_object_new (type, NULL);
}

/***************************************************************************/
/* Use a plugin service to define regression curves types */

#define GOG_TYPE_TREND_LINE_SERVICE		(gog_trend_line_service_get_type ())
#define GOG_TREND_LINE_SERVICE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_TREND_LINE_SERVICE, GogTrendLineService))
#define GOG_IS_TREND_LINE_SERVICE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_TREND_LINE_SERVICE))

GType gog_trend_line_service_get_type (void);

typedef struct {
	GOPluginServiceSimple	base;

	GSList	*types, *paths;
} GogTrendLineService;
typedef GOPluginServiceSimpleClass GogTrendLineServiceClass;

static GHashTable *pending_trend_line_type_files = NULL;
static GHashTable *trend_line_types = NULL;

static void
cb_pending_trend_line_types_load (char const *path,
			    GogTrendLineService *service,
			    G_GNUC_UNUSED gpointer ignored)
{
	xmlNode *ptr, *prop;
	xmlDoc *doc;
	GogTrendLineType *type;

	if (debug)
		g_printerr ("Loading %s\n", path);
	doc = go_xml_parse_file (path);
	g_return_if_fail (doc != NULL && doc->xmlRootNode != NULL);

	for (ptr = doc->xmlRootNode->xmlChildrenNode; ptr ; ptr = ptr->next)
		if (!xmlIsBlankNode (ptr) && ptr->name && !strcmp (ptr->name, "Type")) {
			type = g_new0 (GogTrendLineType, 1);
			type->name = xmlGetProp (ptr, "_name");
			type->description = xmlGetProp (ptr, "_description");
			type->engine = xmlGetProp (ptr, "engine");
			service->types = g_slist_prepend (service->types, type);
			g_hash_table_insert (trend_line_types, type->name, type);
			for (prop = ptr->xmlChildrenNode ; prop != NULL ; prop = prop->next)
				if (!xmlIsBlankNode (prop) &&
					prop->name && !strcmp (prop->name, "property")) {
					xmlChar *prop_name = xmlGetProp (prop, "name");

					if (prop_name == NULL) {
						g_warning ("missing name for property entry");
						continue;
					}

					if (type->properties == NULL)
						type->properties = g_hash_table_new_full (g_str_hash, g_str_equal,
											  xmlFree, xmlFree);
					g_hash_table_replace (type->properties,
						prop_name, xmlNodeGetContent (prop));
				}
		}

	xmlFreeDoc (doc);
}

static void
pending_trend_line_types_load (void)
{
	if (pending_trend_line_type_files != NULL) {
		GHashTable *tmp = pending_trend_line_type_files;
		pending_trend_line_type_files = NULL;
		g_hash_table_foreach (tmp,
			(GHFunc) cb_pending_trend_line_types_load, NULL);
		g_hash_table_destroy (tmp);
	}
}

static void
gog_trend_line_service_read_xml (GOPluginService *service, xmlNode *tree, GOErrorInfo **ret_error)
{
	char    *path, *tmp;
	xmlNode *ptr;
	GSList *paths = NULL;

	for (ptr = tree->xmlChildrenNode; ptr != NULL; ptr = ptr->next)
		if (0 == xmlStrcmp (ptr->name, "file") &&
		    NULL != (path = xmlNodeGetContent (ptr))) {
			if (!g_path_is_absolute (path)) {
				char const *dir = go_plugin_get_dir_name (
					go_plugin_service_get_plugin (service));
				tmp = g_build_filename (dir, path, NULL);
			} else
				tmp = g_strdup (path);
			xmlFree (path);
			path = tmp;
			paths = g_slist_append (paths, path);
		}
	GOG_TREND_LINE_SERVICE (service)->paths = paths;
}

static void
gog_trend_line_service_activate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GSList *l = GOG_TREND_LINE_SERVICE (service)->paths;
	if (l && pending_trend_line_type_files == NULL)
		pending_trend_line_type_files = g_hash_table_new_full (
			g_str_hash, g_str_equal, g_free, g_object_unref);
	while (l) {
		g_object_ref (service);
		g_hash_table_replace (pending_trend_line_type_files, l->data, service);
		l = l->next;
	}
	service->is_active = TRUE;
}

static void
gog_trend_line_service_deactivate (GOPluginService *service, GOErrorInfo **ret_error)
{
	GogTrendLineService *line_service = GOG_TREND_LINE_SERVICE (service);
	GSList *l = line_service->types;
	while (l) {
		g_hash_table_remove (trend_line_types, ((GogTrendLineType *) l->data)->name);
		l = l->next;
	}
	g_slist_free (line_service->types);
	line_service->types = NULL;

	if (pending_trend_line_type_files) {
		GSList *l;
		for (l = line_service->paths; l; l = l->next) {
			const char *path = l->data;
			g_hash_table_remove (pending_trend_line_type_files, path);
		}
		if (g_hash_table_size (pending_trend_line_type_files) == 0) {
			g_hash_table_destroy (pending_trend_line_type_files);
			pending_trend_line_type_files = NULL;
		}
	}
	service->is_active = FALSE;
}

static char *
gog_trend_line_service_get_description (GOPluginService *service)
{
	return g_strdup (_("Regression Curve Type"));
}

static void gog_trend_line_type_free (GogTrendLineType *type);

static void
gog_trend_line_service_finalize (GObject *obj)
{
	GogTrendLineService *service = GOG_TREND_LINE_SERVICE (obj);
	GSList *ptr;

	for (ptr = service->types ; ptr != NULL ; ptr = ptr->next)
		gog_trend_line_type_free ((GogTrendLineType *) ptr->data);
	g_slist_free (service->types);
	service->types = NULL;

	g_slist_free (service->paths);
	service->paths = NULL;

	(plot_type_parent_klass->finalize) (obj);
}

static void
gog_trend_line_service_init (GObject *obj)
{
	GogTrendLineService *service = GOG_TREND_LINE_SERVICE (obj);

	service->types = NULL;
}

static void
gog_trend_line_service_class_init (GObjectClass *gobject_klass)
{
	GOPluginServiceClass *ps_class = GO_PLUGIN_SERVICE_CLASS (gobject_klass);

	gobject_klass->finalize	  = gog_trend_line_service_finalize;
	ps_class->read_xml	  = gog_trend_line_service_read_xml;
	ps_class->get_description = gog_trend_line_service_get_description;
	ps_class->activate	  = gog_trend_line_service_activate;
	ps_class->deactivate	  = gog_trend_line_service_deactivate;
}

GSF_CLASS (GogTrendLineService, gog_trend_line_service,
           gog_trend_line_service_class_init, gog_trend_line_service_init,
           GO_TYPE_PLUGIN_SERVICE_SIMPLE)

/***************************************************************************/

void
_gog_plugin_services_init (void)
{
	debug = go_debug_flag ("graph-plugin");
	if (debug)
		g_printerr ("graph-plugin init\n");

	go_plugin_service_define ("plot_engine", &gog_plot_engine_service_get_type);
	go_plugin_service_define ("plot_type",   &gog_plot_type_service_get_type);
	go_plugin_service_define ("trendline_engine", &gog_trend_line_engine_service_get_type);
	go_plugin_service_define ("trendline_type", &gog_trend_line_service_get_type);
}

void
_gog_plugin_services_shutdown (void)
{
	if (debug)
		g_printerr ("graph-plugin shutdown\n");

	g_slist_foreach (refd_plugins, (GFunc)go_plugin_use_unref, NULL);
	g_slist_foreach (refd_plugins, (GFunc)g_object_unref, NULL);
	g_slist_free (refd_plugins);
}

/***************************************************************************/

static GHashTable *plot_families = NULL;

static void
gog_plot_family_free (GogPlotFamily *family)
{
	g_free (family->name);			family->name = NULL;
	g_free (family->sample_image_file);	family->sample_image_file = NULL;
	if (family->types) {
		g_hash_table_destroy (family->types);
		family->types = NULL;
	}
	g_free (family);
}

static void
gog_plot_type_free (GogPlotType *type)
{
	g_free (type->name);
	g_free (type->sample_image_file);
	g_free (type->description);
	g_free (type->engine);
	if (type->properties) {
		g_hash_table_destroy (type->properties);
		type->properties = NULL;
	}
	g_free (type);
}

static void
create_plot_families (void)
{
	if (!plot_families)
		plot_families = g_hash_table_new_full
			(g_str_hash, g_str_equal,
			 NULL, (GDestroyNotify) gog_plot_family_free);
}

/**
 * gog_plot_families:
 *
 * Returns: (transfer none) (element-type utf8 GogPlotFamily): the
 * registered plot families.
 */
GHashTable const *
gog_plot_families (void)
{
	create_plot_families ();
	pending_plot_types_load ();
	return plot_families;
}

/**
 * gog_plot_family_by_name: (skip)
 * @name: family name
 *
 * Returns: the plot family if it exists.
 */
GogPlotFamily *
gog_plot_family_by_name (char const *name)
{
	create_plot_families ();
	pending_plot_types_load ();
	return g_hash_table_lookup (plot_families, name);
}

/**
 * gog_plot_family_register: (skip)
 * @name: family name
 * @sample_image_file: the sample image file name.
 * @priority:
 * @axis_set: the used axis set.
 *
 * Returns: the new #GogPlotFamily.
 */
GogPlotFamily *
gog_plot_family_register (char const *name, char const *sample_image_file,
			  int priority, GogAxisSet axis_set)
{
	GogPlotFamily *res;

	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (sample_image_file != NULL, NULL);

	create_plot_families ();
	g_return_val_if_fail (g_hash_table_lookup (plot_families, name) == NULL, NULL);

	res = g_new0 (GogPlotFamily, 1);
	res->name		= g_strdup (name);
	res->sample_image_file	= g_strdup (sample_image_file);
	res->priority		= priority;
	res->axis_set = axis_set;
	res->types = g_hash_table_new_full (g_str_hash, g_str_equal,
		NULL, (GDestroyNotify) gog_plot_type_free);

	g_hash_table_insert (plot_families, res->name, res);

	return res;
}

void
gog_plot_family_unregister (GogPlotFamily *family)
{
	g_hash_table_remove (plot_families, family->name);
}

/**
 * gog_plot_type_register: (skip)
 * @family: #GogPlotFamily
 * @col: the column where the sample should appear.
 * @row: the row where the sample should appear.
 * @name: the plot type name.
 * @sample_image_file: the sample image file name.
 * @description: the plot type description.
 * @engine: the plot engine name.
 *
 * Returns: the new #GogPlotType.
 */
GogPlotType *
gog_plot_type_register (GogPlotFamily *family, int col, int row,
		       char const *name, char const *sample_image_file,
		       char const *description, char const *engine)
{
	GogPlotType *res;

	g_return_val_if_fail (family != NULL, NULL);

	res = g_new0 (GogPlotType, 1);
	res->name = g_strdup (name);
	res->sample_image_file = g_strdup (sample_image_file);
	res->description = g_strdup (description);
	res->engine = g_strdup (engine);

	res->col = col;
	res->row = row;
	res->family = family;
	g_hash_table_replace (family->types, res->name, res);

	return res;
}

/***************************************************************************/

static void
gog_trend_line_type_free (GogTrendLineType *type)
{
	g_free (type->name);
	g_free (type->description);
	g_free (type->engine);
	g_free (type);
	if (type->properties) {
		g_hash_table_destroy (type->properties);
		type->properties = NULL;
	}
}

static void
create_trend_line_types (void)
{
	if (!trend_line_types)
		trend_line_types = g_hash_table_new_full
			(g_str_hash, g_str_equal,
			 NULL, (GDestroyNotify) gog_trend_line_type_free);
}

/**
 * gog_trend_line_types:
 *
 * Returns: (transfer none): the registered trend line types.
 **/
GHashTable const *
gog_trend_line_types (void)
{
	create_trend_line_types ();
	pending_trend_line_types_load ();
	return trend_line_types;
}
