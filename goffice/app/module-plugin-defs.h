#ifndef GOFFICE_MODULE_PLUGIN_DEFS_H
#define GOFFICE_MODULE_PLUGIN_DEFS_H

#include <goffice/app/go-plugin.h>
#include <goffice/app/goffice-app.h>
#include <gmodule.h>

void go_plugin_init	(GOPlugin *p, GOCmdContext *cc); /* optional, called after dlopen */
void go_plugin_shutdown	(GOPlugin *p, GOCmdContext *cc); /* optional, called before close */

#ifdef PLUGIN_ID

static GOPlugin *go_get_current_plugin (void)
{
	static GOPlugin *plugin = NULL;
	if (plugin == NULL) plugin = go_plugins_get_plugin_by_id (PLUGIN_ID);
	return plugin;
}
#define PLUGIN (go_get_current_plugin ())

/* Use this macro for defining types inside plugins */
#define	PLUGIN_CLASS(name, prefix, class_init, instance_init, parent_type) \
GType \
prefix ## _get_type (void) \
{ \
	GType type = 0; \
	if (type == 0) { \
		static GTypeInfo const object_info = { \
			sizeof (name ## Class), \
			(GBaseInitFunc) NULL, \
			(GBaseFinalizeFunc) NULL, \
			(GClassInitFunc) class_init, \
			(GClassFinalizeFunc) NULL, \
			NULL,	/* class_data */ \
			sizeof (name), \
			0,	/* n_preallocs */ \
			(GInstanceInitFunc) instance_init, \
			NULL \
		}; \
		type = g_type_module_register_type ( \
			G_TYPE_MODULE (go_get_current_plugin ()), parent_type, #name, \
			&object_info, 0); \
	} \
	return type; \
}

#endif


typedef struct {
	char const * const key;		/* object being versioned */
	char const * const version;	/* version id (strict equality is required) */
} GOPluginModuleDepend;
typedef struct {
	guint32 const magic_number;
	guint32 const num_depends;
} GOPluginModuleHeader;

/* CHeesy api versioning
 * bump this when external api changes.  eventually we will just push this out
 * into the module's link dependencies */
#define GOFFICE_API_VERSION		"0.0"

#define GOFFICE_MODULE_PLUGIN_MAGIC_NUMBER             0x476e756d
#define GOFFICE_MODULE_PLUGIN_INFO_DECL(ver)				\
G_MODULE_EXPORT GOPluginModuleDepend const go_plugin_depends [] = ver;	\
G_MODULE_EXPORT GOPluginModuleHeader const go_plugin_header =  		\
	{ GOFFICE_MODULE_PLUGIN_MAGIC_NUMBER, G_N_ELEMENTS (go_plugin_depends) };

/* convenience header for goffice plugins */
#define GOFFICE_PLUGIN_MODULE_HEADER 					\
G_MODULE_EXPORT GOPluginModuleDepend const go_plugin_depends [] = {	\
    { "goffice", GOFFICE_API_VERSION }					\
};	\
G_MODULE_EXPORT GOPluginModuleHeader const go_plugin_header =  		\
	{ GOFFICE_MODULE_PLUGIN_MAGIC_NUMBER, G_N_ELEMENTS (go_plugin_depends) }

#endif /* GOFFICE_MODULE_PLUGIN_DEFS_H */
