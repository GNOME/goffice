/*
 * regutf8.c:  UTF-8 regexp routines.
 *
 * Author:
 *   Morten Welinder (terra@gnome.org)
 */

#include <goffice/goffice-config.h>
#include "regutf8.h"

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
