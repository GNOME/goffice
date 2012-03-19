#ifndef GO_REGUTF8_H
#define GO_REGUTF8_H

#include <glib.h>
#include <glib-object.h>

/* -------------------------------------------------------------------------- */

G_BEGIN_DECLS

/*
 * This enum snarfed from glibc.  Insofar it is copyrightable, it is
 * Copyright (C) 1985,1989-93,1995-98,2000,2001,2002,2003
 * Free Software Foundation, Inc.
 */
enum {
	GO_REG_NOERROR = 0,	/* Success.  */
	GO_REG_NOMATCH,		/* Didn't find a match (for regexec).  */

	/* POSIX regcomp return error codes.  (In the order listed in the
	   standard.)  */
	GO_REG_BADPAT,		/* Invalid pattern.  */
	GO_REG_ECOLLATE,		/* Inalid collating element.  */
	GO_REG_ECTYPE,		/* Invalid character class name.  */
	GO_REG_EESCAPE,		/* Trailing backslash.  */
	GO_REG_ESUBREG,		/* Invalid back reference.  */
	GO_REG_EBRACK,		/* Unmatched left bracket.  */
	GO_REG_EPAREN,		/* Parenthesis imbalance.  */
	GO_REG_EBRACE,		/* Unmatched \{.  */
	GO_REG_BADBR,		/* Invalid contents of \{\}.  */
	GO_REG_ERANGE,		/* Invalid range end.  */
	GO_REG_ESPACE,		/* Ran out of memory.  */
	GO_REG_BADRPT,		/* No preceding re for repetition op.  */

	/* Error codes we've added.  */
	GO_REG_EEND,		/* Premature end.  */
	GO_REG_ESIZE,		/* Compiled pattern bigger than 2^16 bytes.  */
	GO_REG_ERPAREN		/* Unmatched ) or \); not returned from regcomp.  */
};
#define GO_REG_OK GO_REG_NOERROR

/* eflags bits.  */
enum {
	GO_REG_NOTBOL = 1,
	GO_REG_NOTEOL = 2
};

/* cflags bits.  */
enum {
	GO_REG_EXTENDED = 1,
	GO_REG_ICASE = 2,
	GO_REG_NEWLINE = 4,
	GO_REG_NOSUB = 8
};

/* Like POSIX' regex_t.  */
typedef struct {
	size_t re_nsub;
	/*< private >*/
	gboolean nosub;
	void *ppcre;
} GORegexp;

/* Like POSIX' regoff_t.  */
typedef int GORegoff;

/* Like POSIX' regmatch_t.  */
typedef struct {
	GORegoff rm_so, rm_eo;
} GORegmatch;

int go_regcomp (GORegexp * preg, const char *pattern, int cflags);
int go_regexec (const GORegexp * preg, const char *string, size_t nmatch,
		GORegmatch pmatch[], int eflags);
size_t go_regerror (int errcode, const GORegexp * preg, char *errbuf,
		    size_t errbuf_size);
void go_regfree (GORegexp * preg);

/* -------------------------------------------------------------------------- */

#define GO_TYPE_SEARCH_REPLACE        (go_search_replace_get_type ())
#define GO_SEARCH_REPLACE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GO_TYPE_SEARCH_REPLACE, GOSearchReplace))
#define GO_IS_SEARCH_REPLACE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GO_TYPE_SEARCH_REPLACE))

typedef struct _GoSearchReplace {
	GObject base;

	/*< public >*/
	char *search_text;
	char *replace_text;

	GORegexp *comp_search;
	gboolean is_regexp;	/* Search text is a regular expression.  */
	gboolean ignore_case;	/* Consider "a" and "A" the same.  */
	gboolean preserve_case;	/* Like Emacs' case-replace.  */
	gboolean match_words;	/* Like grep -w.  */

	/*< private >*/
	gboolean plain_replace;
} GOSearchReplace;

typedef struct {
	GObjectClass g_object_class;
} GOSearchReplaceClass;


GQuark           go_search_replace_error_quark (void);
GType            go_search_replace_get_type (void);

gboolean         go_search_replace_verify (GOSearchReplace *sr, gboolean repl, GError **err);

gboolean         go_search_match_string (GOSearchReplace *sr, const char *src);
char *           go_search_replace_string (GOSearchReplace *sr, const char *src);

const char *go_regexp_quote1 (GString *target, const char *s);
void go_regexp_quote (GString *target, const char *s);

G_END_DECLS

#endif /* GO_REGUTF8_H */
