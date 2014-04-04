#ifndef GO_DTOA_H__
#define GO_DTOA_H__

#include <goffice/goffice-features.h>
#include <glib.h>

G_BEGIN_DECLS

/* ------------------------------------------------------------------------- */

void go_dtoa (GString *dst, const char *fmt, ...);

/* ------------------------------------------------------------------------- */

G_END_DECLS

#endif	/* GO_DTOA_H__ */
