#ifndef GO_FILE_PRIV_H
#define GO_FILE_PRIV_H

#include <goffice/app/goffice-app.h>

G_BEGIN_DECLS

/*
 * GOFileOpener
 */

#define GO_FILE_OPENER_METHOD(obj,name) \
        ((GO_FILE_OPENER_CLASS (G_OBJECT_GET_CLASS (obj)))->name)


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

G_END_DECLS

#endif /* GO_FILE_PRIV_H */
