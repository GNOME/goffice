#ifndef GO_RSM_H
#define GO_RSM_H

#include <glib.h>

G_BEGIN_DECLS

void _go_rsm_init (void);
void _go_rsm_shutdown (void);

void go_rsm_register_file (const char *id, gconstpointer data, size_t len);
void go_rsm_unregister_file (const char *id);

gconstpointer go_rsm_lookup (const char *id, size_t *len);

G_END_DECLS

#endif /* GO_RSM_H */
