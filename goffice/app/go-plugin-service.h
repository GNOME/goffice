#ifndef GO_PLUGIN_SERVICE_H
#define GO_PLUGIN_SERVICE_H

#include <goffice/app/goffice-app.h>
#include <goffice/app/go-plugin.h>
#include <glib.h>
#include <gmodule.h>
#include <libxml/tree.h>

G_BEGIN_DECLS

#define GO_TYPE_PLUGIN_SERVICE         (go_plugin_service_get_type ())
#define GO_PLUGIN_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_SERVICE, GOPluginService))
#define GO_IS_PLUGIN_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_SERVICE))

GType go_plugin_service_get_type (void);

#define GO_TYPE_PLUGIN_SERVICE_GENERAL  (go_plugin_service_general_get_type ())
#define GO_PLUGIN_SERVICE_GENERAL(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_SERVICE_GENERAL, GOPluginServiceGeneral))
#define GO_IS_PLUGIN_SERVICE_GENERAL(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_SERVICE_GENERAL))

GType go_plugin_service_general_get_type (void);
typedef struct _GOPluginServiceGeneral GOPluginServiceGeneral;
typedef struct {
	void (*plugin_func_init) (GOPluginService *service, GOErrorInfo **ret_error);
	void (*plugin_func_cleanup) (GOPluginService *service, GOErrorInfo **ret_error);
} GOPluginServiceGeneralCallbacks;

#define GO_TYPE_PLUGIN_SERVICE_RESOURCE  (go_plugin_service_resource_get_type ())
#define GO_PLUGIN_SERVICE_RESOURCE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_SERVICE_RESOURCE, GOPluginServiceResource))
#define GO_IS_PLUGIN_SERVICE_RESOURCE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_SERVICE_RESOURCE))

GType go_plugin_service_resource_get_type (void);
typedef struct _GOPluginServiceResource GOPluginServiceResource;

#define GO_TYPE_PLUGIN_SERVICE_FILE_OPENER  (go_plugin_service_file_opener_get_type ())
#define GO_PLUGIN_SERVICE_FILE_OPENER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_SERVICE_FILE_OPENER, GOPluginServiceFileOpener))
#define GO_IS_PLUGIN_SERVICE_FILE_OPENER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_SERVICE_FILE_OPENER))

GType go_plugin_service_file_opener_get_type (void);
typedef struct _GOPluginServiceFileOpener GOPluginServiceFileOpener;
typedef struct {
	/* plugin_func_file_probe may be NULL */
	gboolean (*plugin_func_file_probe) (
	         GOFileOpener const *fo, GOPluginService *service,
	         GsfInput *input, GOFileProbeLevel pl);
	void     (*plugin_func_file_open) (
	         GOFileOpener const *fo, GOPluginService *service,
	         GOIOContext *io_context, GoView *view,
		 GsfInput *input, char const *enc);
} GOPluginServiceFileOpenerCallbacks;


#define GO_TYPE_PLUGIN_SERVICE_FILE_SAVER  (go_plugin_service_file_saver_get_type ())
#define GO_PLUGIN_SERVICE_FILE_SAVER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_SERVICE_FILE_SAVER, GOPluginServiceFileSaver))
#define GO_IS_PLUGIN_SERVICE_FILE_SAVER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_SERVICE_FILE_SAVER))

GType go_plugin_service_file_saver_get_type (void);
typedef struct _GOPluginServiceFileSaver GOPluginServiceFileSaver;
typedef struct {
	void  (*plugin_func_file_save) (
	      GOFileSaver const *fs, GOPluginService *service,
	      GOIOContext *io_context, GoView const *view,
	      GsfOutput *output);
} GOPluginServiceFileSaverCallbacks;

#define GO_TYPE_PLUGIN_SERVICE_PLUGIN_LOADER  (go_plugin_service_plugin_loader_get_type ())
#define GO_PLUGIN_SERVICE_PLUGIN_LOADER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_SERVICE_PLUGIN_LOADER, GOPluginServicePluginLoader))
#define GO_IS_PLUGIN_SERVICE_PLUGIN_LOADER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_SERVICE_PLUGIN_LOADER))

GType go_plugin_service_plugin_loader_get_type (void);
typedef struct _GOPluginServicePluginLoader GOPluginServicePluginLoader;
typedef struct {
	GType (*plugin_func_get_loader_type) (
	      GOPluginService *service, GOErrorInfo **ret_error);
} GOPluginServicePluginLoaderCallbacks;

GType go_plugin_service_plugin_loader_generate_type (GOPluginService *service,
						     GOErrorInfo **ret_error);

/****************************************************************************/

#define GO_TYPE_PLUGIN_SERVICE_GOBJECT_LOADER  (go_plugin_service_gobject_loader_get_type ())
#define GO_PLUGIN_SERVICE_GOBJECT_LOADER(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_SERVICE_GOBJECT_LOADER, GOPluginServiceGObjectLoader))
#define GO_IS_PLUGIN_SERVICE_GOBJECT_LOADER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_SERVICE_GOBJECT_LOADER))

GType go_plugin_service_gobject_loader_get_type (void);
typedef struct _GOPluginServiceGObjectLoader GOPluginServiceGObjectLoader;

/****************************************************************************/
#define GO_TYPE_PLUGIN_SERVICE_SIMPLE  (go_plugin_service_simple_get_type ())
#define GO_PLUGIN_SERVICE_SIMPLE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_SERVICE_SIMPLE, GOPluginServiceSimple))
#define GO_IS_PLUGIN_SERVICE_SIMPLE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_SERVICE_SIMPLE))

GType go_plugin_service_simple_get_type (void);
typedef struct _GOPluginServiceSimple GOPluginServiceSimple;

/****************************************************************************/

GOPluginService  *go_plugin_service_new (GOPlugin *plugin, xmlNode *tree, GOErrorInfo **ret_error);
char const     *go_plugin_service_get_id (const GOPluginService *service);
char const     *go_plugin_service_get_description (GOPluginService *service);
GOPlugin      *go_plugin_service_get_plugin (GOPluginService *service);
gpointer	go_plugin_service_get_cbs (GOPluginService *service);
void		go_plugin_service_activate (GOPluginService *service, GOErrorInfo **ret_error);
void		go_plugin_service_deactivate (GOPluginService *service, GOErrorInfo **ret_error);
void		go_plugin_service_load   (GOPluginService *service, GOErrorInfo **ret_error);
void		go_plugin_service_unload (GOPluginService *service, GOErrorInfo **ret_error);

typedef GType (*GOPluginServiceCreate) (void);
void _go_plugin_services_init     (void);
void go_plugin_services_shutdown (void);
void go_plugin_service_define    (char const *type_str,
				  GOPluginServiceCreate ctor);

G_END_DECLS

#endif /* GO_PLUGIN_SERVICE_H */
