/*
 * regutf8.c:  UTF-8 regexp routines.
 *
 * Author:
 *   Morten Welinder (terra@gnome.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 */

#include <goffice/goffice-config.h>
#include "regutf8.h"
#include "go-glib-extras.h"
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <string.h>

/* ------------------------------------------------------------------------- */
/**
 * GORegexp:
 * @re_nsub: number of capturing subpatterns.
 **/

/**
 * GORegmatch:
 * @rm_so: start offset.
 * @rm_eo: end offset.
 **/

void
go_regfree (GORegexp *gor)
{
	if (gor->ppcre) {
		g_regex_unref (gor->ppcre);
		gor->ppcre = NULL;
	}
}

size_t
go_regerror (int errcode, const GORegexp *gor, char *dst, size_t dstsize)
{
	const char *err;
	size_t errlen;

	switch (errcode) {
	case GO_REG_NOERROR: err = "?"; break;
	case GO_REG_NOMATCH: err = _("Pattern not found."); break;
	default:
	case GO_REG_BADPAT: err = _("Invalid pattern."); break;
	case GO_REG_ECOLLATE: err = _("Invalid collating element."); break;
	case GO_REG_ECTYPE: err = _("Invalid character class name."); break;
	case GO_REG_EESCAPE: err = _("Trailing backslash."); break;
	case GO_REG_ESUBREG: err = _("Invalid back reference."); break;
	case GO_REG_EBRACK: err = _("Unmatched left bracket."); break;
	case GO_REG_EPAREN: err = _("Parenthesis imbalance."); break;
	case GO_REG_EBRACE: err = _("Unmatched \\{."); break;
	case GO_REG_BADBR: err = _("Invalid contents of \\{\\}."); break;
	case GO_REG_ERANGE: err = _("Invalid range end."); break;
	case GO_REG_ESPACE: err = _("Out of memory."); break;
	case GO_REG_BADRPT: err = _("Invalid repetition operator."); break;
	case GO_REG_EEND: err = _("Premature end of pattern."); break;
	case GO_REG_ESIZE: err = _("Pattern is too big."); break;
	case GO_REG_ERPAREN: err = _("Unmatched ) or \\)"); break;
	};

	errlen = strlen (err);
	if (dstsize > 0) {
		size_t copylen = MIN (errlen, dstsize - 1);
		memcpy (dst, err, copylen);
		dst[copylen] = 0;
	}

	return errlen;
}

int
go_regcomp (GORegexp *gor, const char *pat, int cflags)
{
	GError *error = NULL;
	GRegex *r;
	int coptions =
		((cflags & GO_REG_ICASE) ? G_REGEX_CASELESS : 0) |
		((cflags & GO_REG_NEWLINE) ? G_REGEX_MULTILINE : 0);

	gor->ppcre = r = g_regex_new (pat, coptions, 0, &error);

	if (r == NULL) {
		/* 10, 19, 22 and 37 are not handled by GRegex. */
		switch (error->code) {
		case G_REGEX_ERROR_STRAY_BACKSLASH:
		case G_REGEX_ERROR_MISSING_CONTROL_CHAR:
		case G_REGEX_ERROR_UNRECOGNIZED_ESCAPE:
		/* case 37: */
			return GO_REG_EESCAPE;
		case G_REGEX_ERROR_QUANTIFIERS_OUT_OF_ORDER:
		case G_REGEX_ERROR_QUANTIFIER_TOO_BIG:
			return GO_REG_EBRACE;
		case G_REGEX_ERROR_UNTERMINATED_CHARACTER_CLASS:
			return GO_REG_EBRACK;
		case G_REGEX_ERROR_INVALID_ESCAPE_IN_CHARACTER_CLASS:
		case G_REGEX_ERROR_UNKNOWN_POSIX_CLASS_NAME:
			return GO_REG_ECTYPE;
		case G_REGEX_ERROR_RANGE_OUT_OF_ORDER:
			return GO_REG_ERANGE;
		case G_REGEX_ERROR_NOTHING_TO_REPEAT:
		/* case 10: */
			return GO_REG_BADRPT;
		case G_REGEX_ERROR_UNMATCHED_PARENTHESIS:
		case G_REGEX_ERROR_UNTERMINATED_COMMENT:
		/* case 22: */
			return GO_REG_EPAREN;
		case G_REGEX_ERROR_INEXISTENT_SUBPATTERN_REFERENCE:
			return GO_REG_ESUBREG;
		case G_REGEX_ERROR_EXPRESSION_TOO_LARGE:
		/* case 19: */
			return GO_REG_ESIZE;
		case G_REGEX_ERROR_MEMORY_ERROR:
			return GO_REG_ESPACE;
		default:
			return GO_REG_BADPAT;
		}
	} else {
		gor->re_nsub = g_regex_get_capture_count (r);
		gor->nosub = (cflags & GO_REG_NOSUB) != 0;
		return 0;
	}
	return 0;
}

int
go_regexec (const GORegexp *gor, const char *txt,
	    size_t nmatch, GORegmatch *pmatch, int eflags)
{
	int eoptions =
		((eflags & GO_REG_NOTBOL) ? G_REGEX_MATCH_NOTBOL : 0) |
		((eflags & GO_REG_NOTEOL) ? G_REGEX_MATCH_NOTEOL : 0);
	size_t i = 0;
	gboolean matched;
	GMatchInfo *match_info = NULL;

	/* We need to totally ignore nmatch and pmatch in this case.  */
	if (gor->nosub)
		nmatch = 0;

	/* Paranoia.  */
	if (nmatch >= G_MAXINT / (2 * sizeof (GORegmatch)))
		return GO_REG_ESPACE;

	matched = g_regex_match (gor->ppcre, txt, eoptions,
				 nmatch ? &match_info : NULL);

	for (i = 0; matched && i < nmatch; i++) {
		gint start_pos = -1, end_pos = -1;
		g_match_info_fetch_pos (match_info, i, &start_pos, &end_pos);
		pmatch[i].rm_so = start_pos;
		pmatch[i].rm_eo = end_pos;
	}
	if (match_info)
		g_match_info_free (match_info);

	return matched ? GO_REG_NOERROR : GO_REG_NOMATCH;
}

/* ------------------------------------------------------------------------- */

static GObjectClass *parent_class;

enum {
	PROP_0,
	PROP_SEARCH_TEXT,
	PROP_REPLACE_TEXT,
	PROP_IS_REGEXP,
	PROP_IGNORE_CASE,
	PROP_PRESERVE_CASE,
	PROP_MATCH_WORDS
};

/* ------------------------------------------------------------------------- */

GQuark
go_search_replace_error_quark (void)
{
  static GQuark q = 0;
  if (q == 0)
    q = g_quark_from_static_string ("go-search-replace-error-quark");

  return q;
}

/* ------------------------------------------------------------------------- */

static void
kill_compiled (GOSearchReplace *sr)
{
	if (sr->comp_search) {
		go_regfree (sr->comp_search);
		g_free (sr->comp_search);
		sr->comp_search = NULL;
	}
}

/* ------------------------------------------------------------------------- */

static int
go_search_replace_compile (GOSearchReplace *sr)
{
	const char *pattern;
	char *tmp;
	int flags = 0;
	int res;

	g_return_val_if_fail (sr && sr->search_text, GO_REG_EEND);

	kill_compiled (sr);

	if (sr->is_regexp) {
		pattern = sr->search_text;
		tmp = NULL;
		sr->plain_replace =
			(sr->replace_text &&
			 g_utf8_strchr (sr->replace_text, -1, '$') == NULL &&
			 g_utf8_strchr (sr->replace_text, -1, '\\') == NULL);
	} else {
		/*
		 * Create a regular expression equivalent to the search
		 * string.  (Thus hoping the regular expression search
		 * routines are pretty good.)
		 */
		GString *regexp = g_string_new (NULL);
		go_regexp_quote (regexp, sr->search_text);
		pattern = tmp = g_string_free (regexp, FALSE);

		sr->plain_replace = TRUE;
	}

	if (sr->ignore_case) flags |= GO_REG_ICASE;

	sr->comp_search = g_new0 (GORegexp, 1);
	res = go_regcomp (sr->comp_search, pattern, flags);

	g_free (tmp);

	return res;
}

/* ------------------------------------------------------------------------- */
/**
 * go_search_replace_verify:
 * @sr: Search-and-Replace info to be checked
 * @repl: Check replacement part too.
 * @err: Location to store error message.
 *
 * Returns: %TRUE if  search-and-replace data is valid.
 **/
gboolean
go_search_replace_verify (GOSearchReplace *sr, gboolean repl, GError **err)
{
	int comp_err;
	g_return_val_if_fail (sr != NULL, err ? ((*err = NULL), FALSE) : FALSE);

	if (!sr->search_text || sr->search_text[0] == 0) {
		if (err)
			g_set_error (err,
				     go_search_replace_error_quark (),
				     0,
				     _("Search string must not be empty."));
		return FALSE;
	}

	if (repl && !sr->replace_text) {
		/* Probably a programmer error.  */
		if (err)
			g_set_error (err,
				     go_search_replace_error_quark (),
				     0,
				     _("Replacement string must be set."));
		return FALSE;
	}

	comp_err = go_search_replace_compile (sr);
	if (comp_err) {
		if (err) {
			char buf[500];
			go_regerror (comp_err, sr->comp_search, buf, sizeof (buf));
			g_set_error (err,
				     go_search_replace_error_quark (),
				     0,
				     _("Invalid search pattern (%s)"),
				     buf);
		}
		return FALSE;
	}

	if (repl && !sr->plain_replace) {
		const char *s;

		for (s = sr->replace_text; *s; s = g_utf8_next_char (s)) {
			switch (*s) {
			case '$':
				s++;
				switch (*s) {
				case '1': case '2': case '3': case '4': case '5':
				case '6': case '7': case '8': case '9':
					if ((*s - '0') <= (int)sr->comp_search->re_nsub)
						break;
					/* Fall through */
				default:
					if (err)
						g_set_error (err,
							     go_search_replace_error_quark (),
							     0,
							     _("Invalid $-specification in replacement."));
					return FALSE;
				}
				break;
			case '\\':
				if (s[1] == 0) {
					if (err)
						g_set_error (err,
							     go_search_replace_error_quark (),
							     0,
							     _("Invalid trailing backslash in replacement."));
					return FALSE;
				}
				s++;
				break;
			}
		}
	}

	return TRUE;
}

/* ------------------------------------------------------------------------- */
/*
 * Quote a single UTF-8 encoded character from s into target and return the
 * location of the next character in s.
 */
const char *
go_regexp_quote1 (GString *target, const char *s)
{
	g_return_val_if_fail (target != NULL, NULL);
	g_return_val_if_fail (s != NULL, NULL);

	switch (*s) {
	case '.': case '[': case '\\':
	case '*': case '+': case '{': case '?':
	case '^': case '$':
	case '(': case '|': case ')':
		g_string_append_c (target, '\\');
		g_string_append_c (target, *s);
		return s + 1;

	case 0:
		return s;

	default:
		do {
			g_string_append_c (target, *s);
			s++;
		} while ((*s & 0xc0) == 0x80);

		return s;
	}
}

/* ------------------------------------------------------------------------- */

/*
 * Regexp quote a UTF-8 string.
 */
void
go_regexp_quote (GString *target, const char *s)
{
	g_return_if_fail (target != NULL);
	g_return_if_fail (s != NULL);

	while (*s)
		s = go_regexp_quote1 (target, s);
}

/* ------------------------------------------------------------------------- */

static gboolean
match_is_word (const char *src, const GORegmatch *pm, gboolean bolp)
{
	/* The empty string is not a word.  */
	if (pm->rm_so == pm->rm_eo)
		return FALSE;

	if (pm->rm_so > 0 || !bolp) {
		/* We get here when something actually preceded the match.  */
		gunichar c_pre = g_utf8_get_char (g_utf8_prev_char (src + pm->rm_so));
		if (g_unichar_isalnum (c_pre))
			return FALSE;
	}

	{
		gunichar c_post = g_utf8_get_char (src + pm->rm_eo);
		if (c_post != 0 && g_unichar_isalnum (c_post))
			return FALSE;
	}

	return TRUE;
}

/* ------------------------------------------------------------------------- */

typedef enum {
	SC_Upper,    /* At least one letter.  No lower case.  */
	SC_Capital,  /* Something Like: This */
	SC_Other
} SearchCase;

static SearchCase
inspect_case (const char *p, const char *pend)
{
	gboolean is_upper = TRUE;
	gboolean is_capital = TRUE;
	gboolean has_letter = FALSE;
	gboolean expect_upper = TRUE;

	for (; p < pend; p = g_utf8_next_char (p)) {
		gunichar c = g_utf8_get_char (p);
		if (g_unichar_isalpha (c)) {
			has_letter = TRUE;
			if (!g_unichar_isupper (c)) {
				is_upper = FALSE;
			}

			if (expect_upper ? !g_unichar_isupper (c) : !g_unichar_islower (c)) {
				is_capital = FALSE;
			}
			expect_upper = FALSE;
		} else
			expect_upper = TRUE;
	}

	if (has_letter) {
		if (is_upper)
			return SC_Upper;
		if (is_capital)
			return SC_Capital;
	}

	return SC_Other;
}


static char *
calculate_replacement (GOSearchReplace *sr, const char *src, const GORegmatch *pm)
{
	char *res;

	if (sr->plain_replace) {
		res = g_strdup (sr->replace_text);
	} else {
		const char *s;
		GString *gres = g_string_sized_new (strlen (sr->replace_text));

		for (s = sr->replace_text; *s; s = g_utf8_next_char (s)) {
			switch (*s) {
			case '$':
			{
				int n = s[1] - '0';
				s++;

				g_assert (n > 0 && n <= (int)sr->comp_search->re_nsub);
				g_string_append_len (gres,
						     src + pm[n].rm_so,
						     pm[n].rm_eo - pm[n].rm_so);
				break;
			}
			case '\\':
				s++;
				g_assert (*s != 0);
				g_string_append_unichar (gres, g_utf8_get_char (s));
				break;
			default:
				g_string_append_unichar (gres, g_utf8_get_char (s));
				break;
			}
		}

		res = g_string_free (gres, FALSE);
	}

	/*
	 * Try to preserve the case during replacement, i.e., do the
	 * following substitutions:
	 *
	 * search -> replace
	 * Search -> Replace
	 * SEARCH -> REPLACE
	 * TheSearch -> TheReplace
	 */
	if (sr->preserve_case) {
		SearchCase sc =
			inspect_case (src + pm->rm_so, src + pm->rm_eo);

		switch (sc) {
		case SC_Upper:
		{
			char *newres = g_utf8_strup (res, -1);
			g_free (res);
			res = newres;
			break;
		}

		case SC_Capital:
		{
			char *newres = go_utf8_strcapital (res, -1);
			g_free (res);
			res = newres;
			break;
		}

		case SC_Other:
			break;

#ifndef DEBUG_SWITCH_ENUM
		default:
			g_assert_not_reached ();
#endif
		}
	}

	return res;
}

/* ------------------------------------------------------------------------- */

gboolean
go_search_match_string (GOSearchReplace *sr, const char *src)
{
	int flags = 0;

	g_return_val_if_fail (sr, FALSE);

	if (!sr->comp_search) {
		go_search_replace_compile (sr);
		g_return_val_if_fail (sr->comp_search, FALSE);
	}

	while (1) {
		GORegmatch match;
		int ret = go_regexec (sr->comp_search, src, 1, &match, flags);

		switch (ret) {
		case 0:
			if (!sr->match_words)
				return TRUE;

			if (match_is_word (src, &match, (flags & GO_REG_NOTBOL) != 0))
				return TRUE;

			/*
			 * We had a match, but it's not a word.  Pretend we saw
			 * a one-character match and continue after that.
			 */
			flags |= GO_REG_NOTBOL;
			src = g_utf8_next_char (src + match.rm_so);
			break;

		case GO_REG_NOMATCH:
			return FALSE;

		default:
			g_error ("Unexpected error code from regexec: %d.", ret);
			return FALSE;
		}
	}
}

/* ------------------------------------------------------------------------- */

/**
 * go_search_replace_string:
 * @sr: A #GOSearchReplace search-and-replace specification
 * @src: the source string
 *
 * Returns: (transfer full) (nullable): the string after search-and-replace.
 * However, if nothing changed, %NULL is returned.
 */
char *
go_search_replace_string (GOSearchReplace *sr, const char *src)
{
	int nmatch;
	GORegmatch *pmatch;
	GString *res = NULL;
	int ret;
	int flags = 0;

	g_return_val_if_fail (sr, NULL);
	g_return_val_if_fail (sr->replace_text, NULL);

	if (!sr->comp_search) {
		go_search_replace_compile (sr);
		g_return_val_if_fail (sr->comp_search, NULL);
	}

	nmatch = 1 + sr->comp_search->re_nsub;
	pmatch = g_new (GORegmatch, nmatch);

	while ((ret = go_regexec (sr->comp_search, src, nmatch, pmatch, flags)) == 0) {
		if (!res) {
			/* The size here is a bit arbitrary.  */
			int size = strlen (src) +
				10 * strlen (sr->replace_text);
			res = g_string_sized_new (size);
		}

		if (pmatch[0].rm_so > 0) {
			g_string_append_len (res, src, pmatch[0].rm_so);
		}

		if (sr->match_words && !match_is_word (src, pmatch,
						       (flags & GO_REG_NOTBOL) != 0)) {
			/*  We saw a fake match.  */
			if (pmatch[0].rm_so < pmatch[0].rm_eo) {
				const char *p = src + pmatch[0].rm_so;
				gunichar c = g_utf8_get_char (p);

				g_string_append_unichar (res, c);

				/* Pretend we saw a one-character match.  */
				pmatch[0].rm_eo = pmatch[0].rm_so +
					(g_utf8_next_char (p) - p);
			}
		} else {
			char *replacement =
				calculate_replacement (sr, src, pmatch);
			g_string_append (res, replacement);
			g_free (replacement);

			if (src[pmatch[0].rm_eo] == 0) {
				/*
				 * We matched and replaced the last character
				 * of the string.  Do not continue as we might
				 * then match the empty string at the end and
				 * re-add the replacement.  This would happen,
				 * for example, if you searched for ".*".
				 */
				src = "";
				break;
			}
		}

		if (pmatch[0].rm_eo > 0) {
			src += pmatch[0].rm_eo;
			flags |= GO_REG_NOTBOL;
		}

		if (pmatch[0].rm_so == pmatch[0].rm_eo) {
			/*
			 * We have matched a null string at the current point.
			 * This might happen searching for just an anchor, for
			 * example.  Don't loop forever...
			 */
			break;
		}
	}

	g_free (pmatch);

	if (res) {
		if (*src)
			g_string_append (res, src);
		return g_string_free (res, FALSE);
	} else {
		return NULL;
	}
}

/* ------------------------------------------------------------------------- */

static void
go_search_replace_init (GObject *obj)
{
}

/* ------------------------------------------------------------------------- */

static void
go_search_replace_finalize (GObject *obj)
{
	GOSearchReplace *sr = (GOSearchReplace *)obj;

	kill_compiled (sr);
	g_free (sr->search_text);
	g_free (sr->replace_text);

	G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* ------------------------------------------------------------------------- */

static void
go_search_replace_get_property (GObject     *object,
				guint        property_id,
				GValue      *value,
				GParamSpec  *pspec)
{
	GOSearchReplace *sr = (GOSearchReplace *)object;

	switch (property_id) {
	case PROP_SEARCH_TEXT:
		g_value_set_string (value, sr->search_text);
		break;
	case PROP_REPLACE_TEXT:
		g_value_set_string (value, sr->replace_text);
		break;
	case PROP_IS_REGEXP:
		g_value_set_boolean (value, sr->is_regexp);
		break;
	case PROP_IGNORE_CASE:
		g_value_set_boolean (value, sr->ignore_case);
		break;
	case PROP_PRESERVE_CASE:
		g_value_set_boolean (value, sr->preserve_case);
		break;
	case PROP_MATCH_WORDS:
		g_value_set_boolean (value, sr->match_words);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

/* ------------------------------------------------------------------------- */
/**
 * GOSearchReplace:
 * @search_text: string to replace.
 * @replace_text: string to use as replacement/
 * @comp_search: #GORegexp
 * @is_regexp: search text is a regular expression.
 * @ignore_case: consider "a" and "A" the same.
 * @preserve_case: like Emacs' case-replace.
 * @match_words: like grep -w.
 **/

static void
go_search_replace_set_search_text (GOSearchReplace *sr, const char *text)
{
	char *text_copy = g_strdup (text);
	g_free (sr->search_text);
	sr->search_text = text_copy;
	kill_compiled (sr);
}

static void
go_search_replace_set_replace_text (GOSearchReplace *sr, const char *text)
{
	char *text_copy = g_strdup (text);
	g_free (sr->replace_text);
	sr->replace_text = text_copy;
	kill_compiled (sr);
}

static void
go_search_replace_set_property (GObject      *object,
				guint         property_id,
				GValue const *value,
				GParamSpec   *pspec)
{
	GOSearchReplace *sr = (GOSearchReplace *)object;

	switch (property_id) {
	case PROP_SEARCH_TEXT:
		go_search_replace_set_search_text (sr, g_value_get_string (value));
		break;
	case PROP_REPLACE_TEXT:
		go_search_replace_set_replace_text (sr, g_value_get_string (value));
		break;
	case PROP_IS_REGEXP:
		sr->is_regexp = g_value_get_boolean (value);
		kill_compiled (sr);
		break;
	case PROP_IGNORE_CASE:
		sr->ignore_case = g_value_get_boolean (value);
		kill_compiled (sr);
		break;
	case PROP_PRESERVE_CASE:
		sr->preserve_case = g_value_get_boolean (value);
		kill_compiled (sr);
		break;
	case PROP_MATCH_WORDS:
		sr->match_words = g_value_get_boolean (value);
		kill_compiled (sr);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

/* ------------------------------------------------------------------------- */

static void
go_search_replace_class_init (GObjectClass *gobject_class)
{
	parent_class = g_type_class_peek_parent (gobject_class);

	gobject_class->finalize = go_search_replace_finalize;
	gobject_class->get_property = go_search_replace_get_property;
	gobject_class->set_property = go_search_replace_set_property;

	g_object_class_install_property
		(gobject_class,
		 PROP_SEARCH_TEXT,
		 g_param_spec_string ("search-text",
				      _("Search Text"),
				      _("The text to search for"),
				      NULL,
				      GSF_PARAM_STATIC |
				      G_PARAM_READWRITE));

	g_object_class_install_property
		(gobject_class,
		 PROP_REPLACE_TEXT,
		 g_param_spec_string ("replace-text",
				      _("Replacement Text"),
				      _("The text to replace with"),
				      NULL,
				      GSF_PARAM_STATIC |
				      G_PARAM_READWRITE));
	g_object_class_install_property
		(gobject_class,
		 PROP_IS_REGEXP,
		 g_param_spec_boolean ("is-regexp",
				       _("Is Regular Expression"),
				       _("Is the search text a regular expression."),
				       FALSE,
				       GSF_PARAM_STATIC |
				       G_PARAM_READWRITE));
	g_object_class_install_property
		(gobject_class,
		 PROP_IGNORE_CASE,
		 g_param_spec_boolean ("ignore-case",
				       _("Ignore Case"),
				       _("Ignore the case of letters."),
				       FALSE,
				       GSF_PARAM_STATIC |
				       G_PARAM_READWRITE));
	g_object_class_install_property
		(gobject_class,
		 PROP_PRESERVE_CASE,
		 g_param_spec_boolean ("preserve-case",
				       _("Preserve Case"),
				       _("Preserve the case of letters."),
				       FALSE,
				       GSF_PARAM_STATIC |
				       G_PARAM_READWRITE));
	g_object_class_install_property
		(gobject_class,
		 PROP_MATCH_WORDS,
		 g_param_spec_boolean ("match-words",
				       _("Match Words"),
				       _("Match whole words only."),
				       FALSE,
				       GSF_PARAM_STATIC |
				       G_PARAM_READWRITE));
}

/* ------------------------------------------------------------------------- */

GSF_CLASS (GOSearchReplace, go_search_replace,
	   go_search_replace_class_init, go_search_replace_init, G_TYPE_OBJECT)
