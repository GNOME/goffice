#ifndef GO_REGUTF8_H
#define GO_REGUTF8_H

#include <glib.h>
#include <goffice/cut-n-paste/pcre/pcreposix.h>

/* -------------------------------------------------------------------------- */

G_BEGIN_DECLS

int gnumeric_regcomp_XL (go_regex_t *preg, char const *pattern, int cflags);

const char *go_regexp_quote1 (GString *target, const char *s);
void go_regexp_quote (GString *target, const char *s);

G_END_DECLS

#endif /* GNUMERIC_REGUTF8_H */
