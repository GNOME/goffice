#ifndef GO_RYU_H__
#define GO_RYU_H__

#include <goffice/goffice-features.h>
#include <glib.h>

G_BEGIN_DECLS

/* ------------------------------------------------------------------------- */

int go_ryu_d2s_buffered_n (double d, char *dst);
int go_ryu_ld2s_buffered_n (long double d, char *dst);

/* ------------------------------------------------------------------------- */

G_END_DECLS

#endif
