
#ifndef GO_CMD_CONTEXT_IMPL_H
#define GO_CMD_CONTEXT_IMPL_H

#include <goffice/app/go-cmd-context.h>

G_BEGIN_DECLS

typedef struct {
	GTypeInterface base;

	char *  (*get_password)		(GOCmdContext *gcc,
					 char const *filename);
	void    (*set_sensitive)	(GOCmdContext *gcc,
					 gboolean sensitive);
	struct _GOCmdContextIface {
		void (*error)		(GOCmdContext *gcc, GError *err);
		void (*error_info)  	(GOCmdContext *gcc, GOErrorInfo *err);
		void (*error_info_list) (GOCmdContext *gcc, GSList *errs);
	} error;

	void    (*progress_set)		(GOCmdContext *gcc, double val);
	void    (*progress_message_set)	(GOCmdContext *gcc, gchar const *msg);
} GOCmdContextClass;

#define GO_CMD_CONTEXT_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GO_TYPE_CMD_CONTEXT, GOCmdContextClass))
#define GO_IS_CMD_CONTEXT_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), GO_TYPE_CMD_CONTEXT))

G_END_DECLS

#endif /* GO_CMD_CONTEXT_IMPL_H */
