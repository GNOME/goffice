#ifndef GO_PLUGIN_LOADER_MODULE_H
#define GO_PLUGIN_LOADER_MODULE_H

#include <goffice/app/goffice-app.h>
#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

#define GO_TYPE_PLUGIN_LOADER_MODULE		(go_plugin_loader_module_get_type ())
#define GO_PLUGIN_LOADER_MODULE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_LOADER_MODULE, GOPluginLoaderModule))
#define GO_IS_PLUGIN_LOADER_MODULE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_LOADER_MODULE))
#define GO_PLUGIN_LOADER_MODULE_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GO_TYPE_PLUGIN_LOADER_MODULE, GOPluginLoaderModuleClass))
#define GO_IS_PLUGIN_LOADER_MODULE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GO_TYPE_PLUGIN_LOADER_MODULE))

typedef void (*GOPluginMethod) (GOPlugin *plugin, GOCmdContext *cc);
typedef struct {
	GObject	base;

	gchar *module_file_name;
	GModule *handle;

	GOPluginMethod plugin_init;
	GOPluginMethod plugin_shutdown;
} GOPluginLoaderModule;
typedef GObjectClass GOPluginLoaderModuleClass;

GType go_plugin_loader_module_get_type (void);

void go_plugin_loader_module_register_version (char const *id, char const *ver);

G_END_DECLS

#endif /* GO_PLUGIN_LOADER_MODULE_H */
