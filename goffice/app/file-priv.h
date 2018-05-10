#ifndef GO_FILE_PRIV_H
#define GO_FILE_PRIV_H

#include <goffice/app/goffice-app.h>

G_BEGIN_DECLS

/*
 * GOFileOpener
 */

#define GO_FILE_OPENER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GO_TYPE_FILE_OPENER, GOFileOpenerClass))
#define GO_FILE_OPENER_METHOD(obj,name) \
        ((GO_FILE_OPENER_CLASS (G_OBJECT_GET_CLASS (obj)))->name)

/**
 * GOFileOpenerClass:
 * @parent_class: parent class.
 * @can_probe: returns %TRUE if the files can be probed.
 * @probe: probes the file.
 * @open: opens and reads the file.
 *
 * File opener base class.
 **/
struct _GOFileOpenerClass {
	GObjectClass parent_class;

	/* private */
	gboolean  (*can_probe) (GOFileOpener const *fo,
				GOFileProbeLevel pl);
	gboolean  (*probe) (GOFileOpener const *fo,
	                    GsfInput *input,
	                    GOFileProbeLevel pl);
	void      (*open)  (GOFileOpener const *fo,
			    gchar const *opt_enc,
	                    GOIOContext *io_context,
	                    GoView *view,
	                    GsfInput *input);
};

struct _GOFileOpener {
	GObject parent;

	/* private */
	gchar	*id;
	gchar	*description;
	GSList	*suffixes;
	GSList	*mimes;
	gboolean encoding_dependent;
	gboolean interactive_only;

	GOFileOpenerProbeFunc probe_func;
	GOFileOpenerOpenFunc  open_func;
};

void go_file_opener_setup (GOFileOpener *fo, const gchar *id,
			   const gchar *description,
			   GSList *suffixes,
			   GSList *mimes,
			   gboolean encoding_dependent,
			   GOFileOpenerProbeFunc probe_func,
			   GOFileOpenerOpenFunc open_func);

/*
 * GOFileSaver
 */

#define GO_FILE_SAVER_METHOD(obj,name) \
        ((GO_FILE_SAVER_CLASS (G_OBJECT_GET_CLASS (obj)))->name)
#define GO_FILE_SAVER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GO_TYPE_FILE_SAVER, GOFileSaverClass))

struct _GOFileSaverClass {
	GObjectClass parent_class;

	/* private */
	void (*save) (GOFileSaver const *fs,
	              GOIOContext *io_context,
	              GoView const *view,
	              GsfOutput *output);

	gboolean (*set_export_options) (GOFileSaver *fs,
					const char *options,
					GError **err);
};


struct _GOFileSaver {
	GObject parent;

	/* private */
	gchar                *id;
	gchar                *mime_type;
	gchar                *extension;
	gchar                *description;
	guint                 overwrite_files : 1;
	guint                 interactive_only : 1;
	guint                 sheet_selection : 1;
	GOFileFormatLevel     format_level;
	GOFileSaveScope       save_scope;
	GOFileSaverSaveFunc   save_func;
};


G_END_DECLS

#endif /* GO_FILE_PRIV_H */
