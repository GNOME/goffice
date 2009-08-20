#ifndef GO_IO_CONTEXT_H
#define GO_IO_CONTEXT_H

#include <goffice/app/goffice-app.h>
#include <glib-object.h>
#include <stdarg.h>

G_BEGIN_DECLS

typedef struct _GOIOContextClass GOIOContextClass;

#define GO_TYPE_IO_CONTEXT    (go_io_context_get_type ())
#define GO_IO_CONTEXT(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GO_TYPE_IO_CONTEXT, GOIOContext))
#define GO_IS_IO_CONTEXT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GO_TYPE_IO_CONTEXT))

GType      go_io_context_get_type (void);
GOIOContext *go_io_context_new        (GOCmdContext *cc);

void       go_io_error_unknown      (GOIOContext *ioc);

void       go_io_error_info_set     (GOIOContext *ioc, GOErrorInfo *error);
void       go_io_error_string       (GOIOContext *ioc, const gchar *str);
void       go_io_error_push         (GOIOContext *ioc, GOErrorInfo *error);
void       go_io_error_clear        (GOIOContext *ioc);
void       go_io_error_display      (GOIOContext *ioc);

gboolean   go_io_error_occurred     (GOIOContext *ioc);
gboolean   go_io_warning_occurred   (GOIOContext *ioc);

void       go_io_progress_message      (GOIOContext *io_context, const gchar *msg);
void       go_io_progress_update       (GOIOContext *io_context, gdouble f);
void       go_io_progress_range_push   (GOIOContext *io_context, gdouble min, gdouble max);
void       go_io_progress_range_pop    (GOIOContext *io_context);

void       go_io_count_progress_set    (GOIOContext *io_context, gint total, gint step);
void       go_io_count_progress_update (GOIOContext *io_context, gint inc);

void       go_io_value_progress_set    (GOIOContext *io_context, gint total, gint step);
void       go_io_value_progress_update (GOIOContext *io_context, gint value);

void       go_io_progress_unset      (GOIOContext *io_context);

void go_io_context_set_num_files	(GOIOContext *ioc, guint count);
void go_io_context_processing_file	(GOIOContext *ioc, char const *uri);
void go_io_warning			(GOIOContext *ioc, char const *fmt, ...) G_GNUC_PRINTF (2, 3);
void go_io_warning_varargs		(GOIOContext *ioc, char const *fmt, va_list args);
void go_io_warning_unknown_font	(GOIOContext *ioc, char const *font_name);
void go_io_warning_unknown_function	(GOIOContext *ioc, char const *funct_name);
void go_io_warning_unsupported_feature	(GOIOContext *ioc, char const *feature);

G_END_DECLS

#endif /* GO_IO_CONTEXT_H */
