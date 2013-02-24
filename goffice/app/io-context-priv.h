#ifndef GO_IO_CONTEXT_PRIV_H
#define GO_IO_CONTEXT_PRIV_H

#include <goffice/app/io-context.h>
#include <goffice/app/error-info.h>
#include <goffice/app/go-cmd-context-impl.h>

G_BEGIN_DECLS

#define GO_IO_CONTEXT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), GO_TYPE_IO_CONTEXT, GOIOContextClass))
#define GO_IS_IO_CONTEXT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), GO_TYPE_IO_CONTEXT))

typedef enum {
	GO_PROGRESS_HELPER_NONE,
	GO_PROGRESS_HELPER_COUNT,
	GO_PROGRESS_HELPER_VALUE,
	GO_PROGRESS_HELPER_LAST
} GOProgressHelperType;

typedef struct {
	GOProgressHelperType helper_type;
	union {
		struct {
			gchar *start;
			gint size;
		} mem;
		struct {
			gint total, last, current;
			gint step;
		} count;
		struct {
			gint total, last;
			gint step;
		} value;
		struct {
			gint n_elements, last, current;
			gint step;
		} workbook;
	} v;
} GOProgressHelper;

typedef struct {
	double min, max;
} GOProgressRange;

struct _GOIOContext {
	GObject base;

	GOCmdContext	*impl;
	GSList          *info; /* GSList of GOErrorInfo in reverse order */
	gboolean	 error_occurred;
	gboolean	 warning_occurred;

	GList	*progress_ranges;
	double	 progress_min, progress_max;
	gdouble  last_progress;
	gdouble  last_time;
	GOProgressHelper helper;
	gboolean exec_main_loop;
};

struct _GOIOContextClass {
	GObjectClass base;
	void  (*set_num_files)   (GOIOContext *ioc, guint count);
	void  (*processing_file) (GOIOContext *ioc, char const *uri);
};

G_END_DECLS

#endif /* GO_IO_CONTEXT_PRIV_H */
