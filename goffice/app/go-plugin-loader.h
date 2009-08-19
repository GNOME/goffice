#ifndef GO_PLUGIN_LOADER_H
#define GO_PLUGIN_LOADER_H

#include <glib.h>
#include <glib-object.h>
#include <libxml/tree.h>
#include <goffice/app/go-plugin.h>
#include <goffice/app/goffice-app.h>

G_BEGIN_DECLS

#define GO_TYPE_PLUGIN_LOADER		(go_plugin_loader_get_type ())
#define GO_PLUGIN_LOADER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_PLUGIN_LOADER, GOPluginLoader))
#define GO_IS_PLUGIN_LOADER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_PLUGIN_LOADER))
#define GO_PLUGIN_LOADER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST ((k), GO_TYPE_PLUGIN_LOADER, GOPluginLoaderClass))
#define GO_IS_PLUGIN_LOADER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GO_TYPE_PLUGIN_LOADER))

typedef struct {
	GTypeInterface base;

	void (*load_base)		(GOPluginLoader *l, GOErrorInfo **err);
	void (*unload_base)		(GOPluginLoader *l, GOErrorInfo **err);
	void (*set_attributes)		(GOPluginLoader *l, GHashTable *attrs, GOErrorInfo **err);
	gboolean (*service_load)	(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);
	gboolean (*service_unload)	(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);

	void (*load_service_file_opener)	(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);
	void (*unload_service_file_opener)	(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);

	void (*load_service_file_saver)		(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);
	void (*unload_service_file_saver)	(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);

	void (*load_service_plugin_loader)	(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);
	void (*unload_service_plugin_loader)	(GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);
} GOPluginLoaderClass;

GType	   go_plugin_loader_get_type (void);
void	   go_plugin_loader_set_attributes (GOPluginLoader *l, GHashTable *attrs,
					    GOErrorInfo **err);
GOPlugin *go_plugin_loader_get_plugin	   (GOPluginLoader *l);
void	   go_plugin_loader_set_plugin	   (GOPluginLoader *l, GOPlugin *p);
void	   go_plugin_loader_load_base	   (GOPluginLoader *l, GOErrorInfo **err);
void	   go_plugin_loader_unload_base	   (GOPluginLoader *l, GOErrorInfo **err);
void	   go_plugin_loader_load_service   (GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);
void	   go_plugin_loader_unload_service (GOPluginLoader *l, GOPluginService *s, GOErrorInfo **err);
gboolean   go_plugin_loader_is_base_loaded (GOPluginLoader *l);

G_END_DECLS

#endif /* GO_PLUGIN_LOADER_H */
