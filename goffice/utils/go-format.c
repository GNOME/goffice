/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-format.c :
 *
 * Copyright (C) 1998 Chris Lahey, Miguel de Icaza
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2005-2007 Morten Welinder (terra@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

/*
 * NOTE - NOTE - NOTE
 *
 * This file includes itself in order to provide both "double" and "long
 * double" versions of most functions.
 *
 * Most source lines thus correspond to two functions, gdb is having
 * a hard time sorting things out.  Feel with it.
 */

#include <goffice/goffice-config.h>
#include <gsf/gsf-msole-utils.h>
#include <gsf/gsf-opendoc-utils.h>
#include "go-format.h"
#include "go-locale.h"
#include "go-font.h"
#include "go-color.h"
#include "datetime.h"
#include "go-glib-extras.h"
#include <goffice/math/go-math.h>
#include <glib/gi18n-lib.h>

#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define OBSERVE_XL_CONDITION_LIMITS
#define OBSERVE_XL_EXPONENT_1
#define ALLOW_EE_MARKUP
#define ALLOW_PI_SLASH
#define ALLOW_NEGATIVE_TIMES
#define MAX_DECIMALS 100

/* ------------------------------------------------------------------------- */

#ifndef DOUBLE

#define DEFINE_COMMON
#define DOUBLE double
#define SUFFIX(_n) _n
#define PREFIX(_n) DBL_ ## _n
#define FORMAT_e "e"
#define FORMAT_f "f"
#define FORMAT_E "E"
#define FORMAT_G "G"
#define STRTO go_strtod

#ifdef GOFFICE_WITH_LONG_DOUBLE
/*
 * We need two versions.  Include ourself in order to get regular
 * definition first.
 */
#include "go-format.c"

/* Now change definitions of macros for the long double version.  */
#undef DEFINE_COMMON
#undef DOUBLE
#undef SUFFIX
#undef PREFIX
#undef FORMAT_e
#undef FORMAT_f
#undef FORMAT_E
#undef FORMAT_G
#undef STRTO

#ifdef HAVE_SUNMATH_H
#include <sunmath.h>
#endif
#define DOUBLE long double
#define SUFFIX(_n) _n ## l
#define PREFIX(_n) LDBL_ ## _n
#define FORMAT_e "Le"
#define FORMAT_f "Lf"
#define FORMAT_E "LE"
#define FORMAT_G "LG"
#define STRTO go_strtold
#endif

#endif

/* ------------------------------------------------------------------------- */

#undef DEBUG_PROGRAMS
#undef DEBUG_REF_COUNT

/***************************************************************************/

#ifdef DEFINE_COMMON

typedef enum {
	OP_END = 0,
	OP_CHAR,		/* unichar */
	OP_CHAR_INVISIBLE,	/* unichar */
	OP_CHAR_REPEAT,
	OP_STRING,		/* 0-termined */
	OP_FILL,		/* unichar */
	OP_LOCALE,		/* locale langstr */
	/* ------------------------------- */
	OP_DATE_ROUND,		/* decimals seen_elapsed */
	OP_DATE_SPLIT,
	OP_DATE_YEAR,
	OP_DATE_YEAR_2,
	OP_DATE_YEAR_THAI,
	OP_DATE_YEAR_THAI_2,
	OP_DATE_MONTH,
	OP_DATE_MONTH_2,
	OP_DATE_MONTH_NAME,
	OP_DATE_MONTH_NAME_1,
	OP_DATE_MONTH_NAME_3,
	OP_DATE_DAY,
	OP_DATE_DAY_2,
	OP_DATE_WEEKDAY,
	OP_DATE_WEEKDAY_3,
	/* ------------------------------- */
	OP_TIME_SPLIT_24,
	OP_TIME_SPLIT_12,
	OP_TIME_SPLIT_ELAPSED_HOUR,
	OP_TIME_SPLIT_ELAPSED_MINUTE,
	OP_TIME_SPLIT_ELAPSED_SECOND,
	OP_TIME_HOUR,
	OP_TIME_HOUR_2,
	OP_TIME_HOUR_N,		/* n */
	OP_TIME_AMPM,
	OP_TIME_AP,		/* [aA][pP] */
	OP_TIME_MINUTE,
	OP_TIME_MINUTE_2,
	OP_TIME_MINUTE_N,	/* n */
	OP_TIME_SECOND,
	OP_TIME_SECOND_2,
	OP_TIME_SECOND_N,	/* n */
	OP_TIME_SECOND_DECIMAL_START,
	OP_TIME_SECOND_DECIMAL_DIGIT,
	/* ------------------------------- */
	OP_NUM_SCALE,		/* orders */
	OP_NUM_ENABLE_THOUSANDS,
	OP_NUM_DISABLE_THOUSANDS,
	OP_NUM_PRINTF_E,	/* prec wd */
	OP_NUM_PRINTF_F,	/* prec */
	OP_NUM_SIGN,
	OP_NUM_VAL_SIGN,
	OP_NUM_MOVETO_ONES,
	OP_NUM_MOVETO_DECIMALS,
	OP_NUM_REST_WHOLE,
	OP_NUM_APPEND_MODE,
	OP_NUM_DECIMAL_POINT,
	OP_NUM_DIGIT_1,		/* [0?#] */
	OP_NUM_DECIMAL_1,	/* [0?#] */
	OP_NUM_DIGIT_1_0,	/* [0?#] */
	OP_NUM_DENUM_DIGIT_Q,
	OP_NUM_EXPONENT_SIGN,	/* forced-p */
	OP_NUM_EXPONENT_1,
	OP_NUM_VAL_EXPONENT,
#ifdef ALLOW_EE_MARKUP
	OP_NUM_MARK_MANTISSA,
	OP_NUM_SIMPLIFY_MANTISSA,
	OP_NUM_SIMPLIFY_EXPONENT,
#endif
	OP_NUM_FRACTION,	/* wholep explicitp (digits|denominator) */
	OP_NUM_FRACTION_WHOLE,
	OP_NUM_FRACTION_NOMINATOR,
	OP_NUM_FRACTION_DENOMINATOR,
	OP_NUM_FRACTION_BLANK,
#ifdef ALLOW_PI_SLASH
	OP_NUM_FRACTION_SCALE_PI,
	OP_NUM_FRACTION_SIMPLIFY_PI,
#endif
	OP_NUM_GENERAL_MARK,
	OP_NUM_GENERAL_DO,
	/* ------------------------------- */
	OP_STR_APPEND_SVAL
} GOFormatOp;

typedef enum {
	TOK_NULL = 0,
	TOK_GENERAL = 256,	/* General	*/

	TOK_STRING,		/* "xyz"	*/
	TOK_CHAR,		/* non-ascii */
	TOK_ESCAPED_CHAR,	/* \x		*/
	TOK_INVISIBLE_CHAR,	/* _x		*/
	TOK_REPEATED_CHAR,	/* *x		*/

	TOK_AMPM5,		/* am/pm	*/
	TOK_AMPM3,		/* a/p		*/

	TOK_ELAPSED_H,		/* [hhh...]	*/
	TOK_ELAPSED_M,		/* [mmm...]	*/
	TOK_ELAPSED_S,		/* [sss...]	*/

	TOK_COLOR,		/* [red]	*/
	TOK_CONDITION,		/* [>0]		*/
	TOK_LOCALE,		/* [$txt-F800]	*/

	TOK_DECIMAL,            /* Decimal sep  */
	TOK_THOUSAND,           /* Thousand sep */

	TOK_ERROR
} GOFormatToken;

typedef enum {
	TT_TERMINATES_SINGLE = 1,
	TT_ERROR = 2,

	TT_ALLOWED_IN_DATE = 0x10,
	TT_STARTS_DATE = 0x20,

	TT_ALLOWED_IN_NUMBER = 0x100,
	TT_STARTS_NUMBER = 0x200,

	TT_ALLOWED_IN_TEXT = 0x1000,
	TT_STARTS_TEXT = 0x2000
} GOFormatTokenType;

typedef enum {
	GO_FMT_INVALID,
	GO_FMT_COND,
	GO_FMT_NUMBER,
	GO_FMT_EMPTY,
	GO_FMT_TEXT,
	GO_FMT_MARKUP
} GOFormatClass;

typedef enum {
	GO_FMT_COND_NONE,
	GO_FMT_COND_EQ,
	GO_FMT_COND_NE,
	GO_FMT_COND_LT,
	GO_FMT_COND_LE,
	GO_FMT_COND_GT,
	GO_FMT_COND_GE,
	GO_FMT_COND_TEXT,
	GO_FMT_COND_NONTEXT
} GOFormatConditionOp;

typedef struct {
	GOFormatConditionOp op;
	unsigned implicit : 1;
	unsigned true_inhibits_minus : 1;
	unsigned false_inhibits_minus : 1;
	/* Plain "double".  It isn't worth it to have two types.  */
	double val;
	GOFormat *fmt;
} GOFormatCondition;

typedef struct {
	guint32 locale;
} GOFormatLocale;


struct _GOFormat {
	unsigned int typ : 8;
	unsigned int ref_count : 24;
	GOColor color;
	unsigned char has_fill;
	GOFormatMagic magic;
	char *format;
	union {
		struct {
			int n;
			GOFormatCondition *conditions;
		} cond;

		struct {
			guchar *program;
			unsigned int E_format    : 1;
			unsigned int use_markup  : 1;
			unsigned int has_date    : 1;
			unsigned int date_ybm    : 1;  /* year, then month.  */
			unsigned int date_mbd    : 1;  /* month, then day.  */
			unsigned int date_dbm    : 1;  /* day, then month.  */
			unsigned int has_time    : 1;
			unsigned int has_hour    : 1;
			unsigned int has_minute  : 1;
			unsigned int has_elapsed : 1;
			unsigned int fraction    : 1;
			unsigned int scale_is_2  : 1;
			unsigned int has_general : 1;
			unsigned int is_general  : 1;
		} number;

		struct {
			guchar *program;
		} text;

		PangoAttrList *markup;
	} u;
};

typedef struct {
	const char *tstr;
	int token;
	GOFormatTokenType tt;
} GOFormatParseItem;

typedef struct {
	/* Set by go_format_preparse: */
	GArray *tokens;

	GOFormatClass typ;
	gboolean is_date;
	gboolean is_number;   /* has TT_STARTS_NUMBER token */

	GOFormatCondition cond;
	gboolean have_cond;

	GOColor color;
	int color_n;
	gboolean color_named;
	gboolean have_color;

	GOFormatLocale locale;
	gboolean have_locale;

	gunichar fill_char;

	int tno_slash;
	int tno_E;

	gboolean has_general, is_general;

	/* Set by go_format_parse_number_new_1: */
	int scale;

	/* Set by go_format_parse_number_fraction: */
	int force_zero_pos;
	gboolean explicit_denom;
	gboolean forced_exponent_sign;
} GOFormatParseState;

#define REPEAT_CHAR_MARKER 0
#define UNICODE_PI 0x03c0
#define UNICODE_TIMES 0x00D7
#define UNICODE_MINUS 0x2212
#define UNICODE_EURO 0x20ac
#define UNICODE_POUNDS1 0x00a3
#define UNICODE_POUNDS2 0x20a4
#define UNICODE_YEN 0x00a5
#define UNICODE_YEN_WIDE 0xffe5

GOFormatFamily
go_format_get_family (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, GO_FORMAT_UNKNOWN);

	switch (fmt->typ) {
	case GO_FMT_MARKUP:
		return GO_FORMAT_MARKUP;
	case GO_FMT_INVALID:
	case GO_FMT_EMPTY:
		return GO_FORMAT_UNKNOWN;
	case GO_FMT_TEXT:
		return GO_FORMAT_TEXT;
	case GO_FMT_NUMBER:
		if (fmt->u.number.is_general)
			return GO_FORMAT_GENERAL;
		if (fmt->u.number.has_date)
			return GO_FORMAT_DATE;
		if (fmt->u.number.has_time)
			return GO_FORMAT_TIME;
		if (fmt->u.number.fraction)
			return GO_FORMAT_FRACTION;
		if (fmt->u.number.E_format)
			return GO_FORMAT_SCIENTIFIC;
		if (fmt->u.number.scale_is_2)
			return GO_FORMAT_PERCENTAGE;
		return GO_FORMAT_NUMBER;
	default:
	case GO_FMT_COND: {
		int i;
		GOFormatFamily t0 = GO_FORMAT_UNKNOWN;

		for (i = 0; i < fmt->u.cond.n; i++) {
			const GOFormatCondition *ci =
				fmt->u.cond.conditions + i;
			if (i == 0)
				t0 = go_format_get_family (ci->fmt);
			if (ci->op == GO_FMT_COND_TEXT &&
			    i == fmt->u.cond.n - 1)
				continue;
			if (t0 != go_format_get_family (ci->fmt))
				return GO_FORMAT_UNKNOWN;
		}

		return t0;
	}
	}
}

const PangoAttrList *
go_format_get_markup (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, NULL);
	g_return_val_if_fail (fmt->typ == GO_FMT_MARKUP, NULL);
	return fmt->u.markup;
}

gboolean
go_format_is_simple (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, TRUE);
	return (fmt->typ != GO_FMT_COND);
}

/*
 * This function returns the format string to be used for a magic format.
 */
static char *
go_format_magic_fmt_str (GOFormatMagic m)
{
	switch (m) {
	default:
		return NULL;

	case GO_FORMAT_MAGIC_LONG_DATE: {
		/* xgettext: See http://projects.gnome.org/gnumeric/date-time-formats.shtml */
		const char *fmt = _("*Long Date Format");
		if (fmt[0] && fmt[0] != '*')
			return g_strdup (fmt);
		return g_strdup ("dddd, mmmm dd, yyyy");
	}

	case GO_FORMAT_MAGIC_MEDIUM_DATE: {
		/* xgettext: See http://projects.gnome.org/gnumeric/date-time-formats.shtml */
		const char *fmt = _("*Medium Date Format");
		if (fmt[0] && fmt[0] != '*')
			return g_strdup (fmt);
		return g_strdup ("d-mmm-yyyy");  /* Excel has yy only.  */
	}

	case GO_FORMAT_MAGIC_SHORT_DATE: {
		/* xgettext: See http://projects.gnome.org/gnumeric/date-time-formats.shtml */
		const char *fmt = _("*Short Date Format");
		if (fmt[0] && fmt[0] != '*')
			return g_strdup (fmt);
		switch (go_locale_month_before_day ()) {
		case 0: return g_strdup ("d/m/yy");
		default:
		case 1: return g_strdup ("m/d/yy");
		case 2: return g_strdup ("yy/m/d");
		}
	}

	case GO_FORMAT_MAGIC_SHORT_DATETIME: {
		/* xgettext: See http://projects.gnome.org/gnumeric/date-time-formats.shtml */
		const char *fmt = _("*Short Date/Time Format");
		if (fmt[0] && fmt[0] != '*')
			return g_strdup (fmt);
		else {
			char *d = go_format_magic_fmt_str (GO_FORMAT_MAGIC_SHORT_DATE);
			char *t = go_format_magic_fmt_str (GO_FORMAT_MAGIC_SHORT_TIME);
			char *res = g_strconcat (d, " ", t, NULL);
			g_free (d);
			g_free (t);
			return res;
		}
	}

	case GO_FORMAT_MAGIC_LONG_TIME: {
		/* xgettext: See http://projects.gnome.org/gnumeric/date-time-formats.shtml */
		const char *fmt = _("*Long Time Format");
		if (fmt[0] && fmt[0] != '*')
			return g_strdup (fmt);
		if (go_locale_24h ())
			return g_strdup ("hh:mm:ss");
		else
			return g_strdup ("h:mm:ss AM/PM");
		break;
	}

	case GO_FORMAT_MAGIC_MEDIUM_TIME: {
		/* xgettext: See http://projects.gnome.org/gnumeric/date-time-formats.shtml */
		const char *fmt = _("*Medium Time Format");
		if (fmt[0] && fmt[0] != '*')
			return g_strdup (fmt);
		if (go_locale_24h ())
			return g_strdup ("hh:mm");
		else
			return g_strdup ("h:mm AM/PM");
		break;
	}

	case GO_FORMAT_MAGIC_SHORT_TIME: {
		/* xgettext: See http://projects.gnome.org/gnumeric/date-time-formats.shtml */
		const char *fmt = _("*Short Time Format");
		if (fmt[0] && fmt[0] != '*')
			return g_strdup (fmt);
		return g_strdup ("hh:mm");
	}
	}
}


static GOFormat *default_percentage_fmt;
static GOFormat *default_money_fmt;
static GOFormat *default_accounting_fmt;
static GOFormat *default_date_fmt;
static GOFormat *default_time_fmt;
static GOFormat *default_date_time_fmt;
static GOFormat *default_general_fmt;
static GOFormat *default_empty_fmt;


static double beyond_precision;
#ifdef GOFFICE_WITH_LONG_DOUBLE
static long double beyond_precisionl;
#endif

/***************************************************************************/

/* WARNING : Global */
static GHashTable *style_format_hash = NULL;

/* used to generate formats when delocalizing so keep the leadings caps */
static struct {
	char const *name;
	GOColor	go_color;
} const format_colors[] = {
	{ N_("Black"),	 GO_COLOR_BLACK },
	{ N_("Blue"),	 GO_COLOR_BLUE },
	{ N_("Cyan"),	 GO_COLOR_CYAN },
	{ N_("Green"),	 GO_COLOR_GREEN },
	{ N_("Magenta"), GO_COLOR_VIOLET },
	{ N_("Red"),	 GO_COLOR_RED },
	{ N_("White"),	 GO_COLOR_WHITE },
	{ N_("Yellow"),	 GO_COLOR_YELLOW }
};

/*
 * go_format_parse_color :
 * @str :
 * @color :
 * @n :
 * @named :
 *
 * Return: TRUE, if ok.  Then @color will be filled in, and @n will be
 * 	a number 0-7 for standard colors.
 *	Returns FALSE otherwise and @color will be zeroed.
 **/
static gboolean
go_format_parse_color (char const *str, GOColor *color,
		       int *n, gboolean *named, gboolean is_localized)
{
	char const *close;
	unsigned int ui;
	const char *color_txt = N_("color");
	gsize len;

	*color = 0;

	if (*str++ != '[')
		return FALSE;

	close = strchr (str, ']');
	if (!close)
		return FALSE;

	for (ui = 0; ui < G_N_ELEMENTS (format_colors); ui++) {
		const char *name = format_colors[ui].name;
		if (is_localized)
			name = _(name);
		if (g_ascii_strncasecmp (str, name, strlen (name)) == 0) {
			*color = format_colors[ui].go_color;
			if (n)
				*n = ui;
			if (named)
				*named = TRUE;
			return TRUE;
		}
	}

	if (is_localized)
		color_txt = _(color_txt);
	len = strlen (color_txt);
	if (g_ascii_strncasecmp (str, color_txt, len) == 0) {
		char *end;
		guint64 ull = g_ascii_strtoull (str + len, &end, 10);
		if (end == str || errno == ERANGE || ull > 56)
			return FALSE;
		if (n)
			*n = ull;
		if (named)
			*named = FALSE;

		/* FIXME: What do we do about *color?  */
		return TRUE;
	}

	return FALSE;
}

static gboolean
go_format_parse_locale (const char *str, GOFormatLocale *locale, gsize *nchars)
{
	guint64 ull;
	const char *close;
	char *end;
	gsize n;

	if (*str++ != '[' ||
	    *str++ != '$')
		return FALSE;

	close = strchr (str, ']');
	if (!close)
		return FALSE;

	n = 0;
	while (*str != '-' && *str != ']') {
		str = g_utf8_next_char (str);
		n++;
	}
	if (nchars)
		*nchars = n;

	if (*str == '-') {
		str++;
		ull = g_ascii_strtoull (str, &end, 16);
		if (str == end || errno == ERANGE || ull > G_MAXUINT)
			return FALSE;
	} else {
		ull = 0;
	}
	if (locale)
		locale->locale = ull;

	return TRUE;
}

static void
determine_inhibit_minus (GOFormatCondition *cond)
{
	cond->true_inhibits_minus = FALSE;
	cond->false_inhibits_minus = FALSE;

	switch (cond->op) {
	case GO_FMT_COND_GT:
		/* ">" and ">=" strangely follow the same rule. */
	case GO_FMT_COND_GE:
		cond->false_inhibits_minus = (cond->val <= 0);
		break;
	case GO_FMT_COND_LT:
		cond->true_inhibits_minus = (cond->val <= 0);
		break;
	case GO_FMT_COND_LE:
		cond->true_inhibits_minus = (cond->val < 0);
		break;
	case GO_FMT_COND_EQ:
		cond->true_inhibits_minus = (cond->val < 0);
		break;
	case GO_FMT_COND_NE:
		cond->false_inhibits_minus = (cond->val < 0);
		break;
	default:
		break;
	}
}


static gboolean
go_format_parse_condition (const char *str, GOFormatCondition *cond)
{
	char *end;

	cond->op = GO_FMT_COND_NONE;
	cond->val = 0;
	cond->fmt = NULL;
	cond->implicit = TRUE;

	if (*str++ != '[')
		return FALSE;

	if (str[0] == '>' && str[1] == '=')
		cond->op = GO_FMT_COND_GE, str += 2;
	else if (str[0] == '>')
		cond->op = GO_FMT_COND_GT, str++;
	else if (str[0] == '<' && str[1] == '=')
		cond->op = GO_FMT_COND_LE, str += 2;
	else if (str[0] == '<' && str[1] == '>')
		cond->op = GO_FMT_COND_NE, str += 2;
	else if (str[0] == '<')
		cond->op = GO_FMT_COND_LT, str++;
	else if (str[0] == '=')
		cond->op = GO_FMT_COND_EQ, str++;
	else
		return FALSE;
	cond->implicit = FALSE;

	cond->val = go_ascii_strtod (str, &end);
	if (end == str || errno == ERANGE)
		return FALSE;
	end = strchr (end, ']');
	if (!end)
		return FALSE;

	determine_inhibit_minus (cond);

	return TRUE;
}

static GOFormatToken
go_format_token2 (char const **pstr, GOFormatTokenType *ptt, gboolean localized)
{
	const GString *decimal = go_locale_get_decimal ();
	const GString *comma = go_locale_get_thousand ();
	const char *str = *pstr;
	GOFormatTokenType tt =
		TT_ALLOWED_IN_DATE | TT_ALLOWED_IN_NUMBER | TT_ALLOWED_IN_TEXT;
	int t;
	int len = 1;
	const char *general = N_("General");
	size_t general_len = 7;

	if (localized) {
		general = _(general);
		general_len = strlen (general);
	}

	if (str == NULL)
		goto error;

	t = *(guchar *)str;

	/* "Ascii" is probably wrong for localized, but it is not clear what to do.  */
	if (g_ascii_strncasecmp (str, general, general_len) == 0) {
		t = TOK_GENERAL;
		tt = TT_ALLOWED_IN_DATE;
		len = general_len;
		goto got_token;
	}

	if (localized
	    ? strncmp (str, decimal->str, decimal->len) == 0
	    : *str == '.') {
		t = TOK_DECIMAL;
		if (localized)
			len = decimal->len;
		tt = TT_ALLOWED_IN_DATE | TT_ALLOWED_IN_NUMBER | TT_STARTS_NUMBER;
		goto got_token;
	}

	if (localized
	    ? strncmp (str, comma->str, comma->len) == 0
	    : *str == ',') {
		t = TOK_THOUSAND;
		if (localized)
			len = comma->len;
		goto got_token;
	}

	switch (t) {
	case 0:
		len = 0; /* Note: str not advanced.  */
	case ';':
		tt = TT_TERMINATES_SINGLE;
		break;

	case 'g': case 'G':
	case 'd': case 'D':
	case 'y': case 'Y':
	case 'b': case 'B':
	case 'e':
	case 'h': case 'H':
	case 'm': case 'M':
	case 's': case 'S':
		tt = TT_ALLOWED_IN_DATE | TT_STARTS_DATE;
		break;

	case 'n': case 'N':
		goto error;

	case 'a': case 'A':
		if (str[1] == '/' && (str[2] == 'p' || str[2] == 'P')) {
			tt = TT_ALLOWED_IN_DATE | TT_STARTS_DATE;
			t = TOK_AMPM3;
			len = 3;
		} else if ((str[1] == 'm' || str[1] == 'M') &&
			   str[2] == '/' &&
			   (str[3] == 'p' || str[3] == 'P') &&
			   (str[4] == 'm' || str[4] == 'M')) {
			tt = TT_ALLOWED_IN_DATE | TT_STARTS_DATE;
			t = TOK_AMPM5;
			len = 5;
		}
		break;

	case '[':
		switch (str[1]) {
		case 's': case 'S':
		case 'm': case 'M':
		case 'h': case 'H': {
			char c = g_ascii_toupper (str[1]);
			len++;
			while (g_ascii_toupper (str[len]) == c)
				len++;
			if (str[len] == ']') {
				t = (c == 'S'
				     ? TOK_ELAPSED_S
				     : (c == 'M'
					? TOK_ELAPSED_M
					: TOK_ELAPSED_H));
				tt = TT_ALLOWED_IN_DATE | TT_STARTS_DATE;
			} else
				t = TOK_COLOR;
			break;
		}
		case '=':
		case '>':
		case '<':
			t = TOK_CONDITION;
			break;

		case '$':
			t = TOK_LOCALE;
			break;

		default:
			t = TOK_COLOR;
			break;
		}

		while (str[len] != ']') {
			if (str[len] == 0)
				goto error;
			len++;
		}
		len++;
		break;

	case '0':
	case '/':
		tt = TT_ALLOWED_IN_DATE | TT_ALLOWED_IN_NUMBER | TT_STARTS_NUMBER;
		break;

	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	case '#':
	case '?':
	case '%':
	case 'E':
		tt = TT_ALLOWED_IN_NUMBER | TT_STARTS_NUMBER;
		break;

	case '@':
		tt = TT_ALLOWED_IN_TEXT | TT_STARTS_TEXT;
		break;

	case '\\':
		t = TOK_ESCAPED_CHAR;
		if (str[1] == 0)
			goto error;
		len += g_utf8_skip[((guchar *)str)[1]];
		break;

	case '_':
		t = TOK_INVISIBLE_CHAR;
		if (str[1] == 0)
			goto error;
		len += g_utf8_skip[((guchar *)str)[1]];
		break;

	case '*':
		t = TOK_REPEATED_CHAR;
		if (str[1] == 0)
			goto error;
		len += g_utf8_skip[((guchar *)str)[1]];
		break;

	case '"':
		t = TOK_STRING;
		while (str[len] != '"') {
			if (str[len] == 0)
				goto error;
			len++;
		}
		len++;
		break;

	default:
		if (t >= 0x80) {
			t = TOK_CHAR;
			len = g_utf8_skip[((guchar *)str)[0]];
		}
		break;
	}

 got_token:
	*ptt = tt;
	*pstr = str + len;
	return t;

 error:
	*ptt = TT_ERROR;
	return TOK_ERROR;
}

static GOFormatToken
go_format_token (char const **pstr, GOFormatTokenType *ptt)
{
	return go_format_token2 (pstr, ptt, FALSE);
}

#define GET_TOKEN(_i) g_array_index (pstate->tokens, GOFormatParseItem, _i)

static const char *
go_format_preparse (const char *str, GOFormatParseState *pstate,
		    gboolean all_tokens, gboolean is_localized)
{
	gboolean ntokens = 0;  /* Excluding cond,color,locale unless all_tokens.  */
	gboolean is_date = FALSE;
	gboolean is_number = FALSE;
	gboolean is_text = FALSE;
	gboolean has_general = FALSE;

	pstate->tokens =
		g_array_new (FALSE, FALSE, sizeof (GOFormatParseItem));
	pstate->tno_slash = -1;
	pstate->tno_E = -1;

	while (1) {
		GOFormatTokenType tt;
		const char *tstr = str;
		int t = go_format_token2 (&str, &tt, is_localized);
		int tno = pstate->tokens->len;

		if (tt & TT_TERMINATES_SINGLE) {
			if (pstate->tno_E >= 0 && pstate->tno_slash >= 0)
				goto error;
			if (has_general && (is_number || is_text))
				goto error;

			if (ntokens == 0 &&
			    (pstate->have_color || pstate->have_cond) &&
			    !pstate->have_locale) {
				/* Pretend to have seen "General" */
				has_general = TRUE;
				g_array_set_size (pstate->tokens, tno + 1);
				GET_TOKEN(tno).tstr = tstr;
				GET_TOKEN(tno).token = TOK_GENERAL;
				GET_TOKEN(tno).tt = TT_ALLOWED_IN_DATE;
				ntokens++;
			}

			pstate->is_date = is_date;
			pstate->is_number = is_number;
			pstate->has_general = has_general;
			pstate->is_general = has_general && ntokens == 1;

			if (ntokens == 0)
				pstate->typ = GO_FMT_EMPTY;
			else if (is_text)
				pstate->typ = GO_FMT_TEXT;
			else
				pstate->typ = GO_FMT_NUMBER;
			return tstr;
		}

		if (is_date) {
			if (!(tt & TT_ALLOWED_IN_DATE))
				goto error;
		} else if (is_number) {
			if (!(tt & TT_ALLOWED_IN_NUMBER))
				goto error;
		} else if (is_text) {
			if (!(tt & TT_ALLOWED_IN_TEXT))
				goto error;
		} else {
			if (tt & TT_STARTS_DATE)
				is_date = TRUE;
			else if (tt & TT_STARTS_NUMBER)
				is_number = TRUE;
			else if (tt & TT_STARTS_TEXT)
				is_text = TRUE;
		}

		g_array_set_size (pstate->tokens, tno + 1);
		GET_TOKEN(tno).tstr = tstr;
		GET_TOKEN(tno).token = t;
		GET_TOKEN(tno).tt = tt;

		switch (t) {
		case TOK_ERROR:
			goto error;

		case TOK_GENERAL:
			if (has_general)
				goto error;
			has_general = TRUE;
			ntokens++;
			break;

		case TOK_CONDITION:
			if (pstate->have_cond ||
			    !go_format_parse_condition (tstr, &pstate->cond))
				goto error;
			pstate->have_cond = TRUE;
			if (all_tokens)
				ntokens++;
			break;

		case TOK_COLOR:
			if (pstate->have_color ||
			    !go_format_parse_color (tstr, &pstate->color,
						    &pstate->color_n,
						    &pstate->color_named,
						    is_localized))
				goto error;
			pstate->have_color = TRUE;
			if (all_tokens)
				ntokens++;
			break;

		case TOK_LOCALE:
			if (pstate->have_locale ||
			    !go_format_parse_locale (tstr, &pstate->locale, NULL))
				goto error;
			pstate->have_locale = TRUE;
			if (all_tokens)
				ntokens++;
			break;

		case TOK_REPEATED_CHAR:
			/* Last one counts.  */
			pstate->fill_char = g_utf8_get_char (tstr + 1);
			ntokens++;
			break;

		case 'E':
			ntokens++;
			if (pstate->tno_E >= 0) {
#ifdef ALLOW_EE_MARKUP
				if (pstate->tno_E == tno - 1)
					break;
#endif
				goto error;
			}
			pstate->tno_E = tno;
			break;

		case '/':
			ntokens++;
			if (is_number) {
				if (pstate->tno_slash >= 0)
					goto error;
				pstate->tno_slash = tno;
				break;
			}
			break;

		default:
			ntokens++;
			break;
		}
	}

 error:
	pstate->typ = GO_FMT_INVALID;
	return NULL;
}

static gboolean
tail_forces_minutes (const char *str)
{
	while (1) {
		GOFormatTokenType tt;
		int t = go_format_token (&str, &tt);

		switch (t) {
		case 0:
		case ';':
		case 'b': case 'B':
		case 'd': case 'D':
		case 'm': case 'M':
		case 'h': case 'H':
		case 'y': case 'Y':
			return FALSE;
		case 's': case 'S':
			return TRUE;
		}
	}
}


static GOFormat *
go_format_create (GOFormatClass cl, const char *format)
{
	GOFormat *fmt = g_new0 (GOFormat, 1);
	fmt->typ = cl;
	fmt->ref_count = 1;
	fmt->format = g_strdup (format);
	return fmt;
}

#define ADD_OP(op) g_string_append_c (prg,(op))
#define ADD_OPuc(op,uc) do { g_string_append_c (prg,(op)); g_string_append_unichar (prg, (uc)); } while (0)
#define ADD_OP2(op1,op2) do { ADD_OP(op1); ADD_OP(op2); } while (0)
#define ADD_OP3(op1,op2,op3) do { ADD_OP2(op1,op2); ADD_OP(op3); } while (0)

/*
 * Handle literal characters, including quoted segments and invisible
 * characters.
 */
static void
handle_common_token (const char *tstr, GOFormatToken t, GString *prg)
{
	switch (t) {
	case TOK_STRING: {
		size_t len = strchr (tstr + 1, '"') - (tstr + 1);
		if (len > 0) {
			ADD_OP (OP_STRING);
			g_string_append_len (prg, tstr + 1, len);
			g_string_append_c (prg, '\0');
		}
		tstr += len + 2;
		break;
	}

	case TOK_DECIMAL:
	case TOK_THOUSAND:
		ADD_OP2 (OP_CHAR, *tstr);
		break;

	case TOK_CHAR:
		ADD_OPuc (OP_CHAR, g_utf8_get_char (tstr));
		break;

	case TOK_ESCAPED_CHAR:
		ADD_OPuc (OP_CHAR, g_utf8_get_char (tstr + 1));
		break;

	case TOK_INVISIBLE_CHAR:
		ADD_OPuc (OP_CHAR_INVISIBLE, g_utf8_get_char (tstr + 1));
		break;

	case TOK_REPEATED_CHAR:
		ADD_OP (OP_CHAR_REPEAT);
		break;

	case TOK_LOCALE: {
		char *oldlocale;
		GOFormatLocale locale;
		const char *lang;
		gsize nchars;
		gboolean ok = go_format_parse_locale (tstr, &locale, &nchars);
		/* Already parsed elsewhere */
		g_return_if_fail (ok);

		tstr += 2;
		if (nchars > 0) {
			const char *next =
				g_utf8_offset_to_pointer (tstr, nchars);
			ADD_OP (OP_STRING);
			g_string_append_len (prg, tstr, next - tstr);
			g_string_append_c (prg, '\0');
			tstr = next;
		}

		lang = gsf_msole_language_for_lid (locale.locale & 0xffff);

		oldlocale = g_strdup (setlocale (LC_ALL, NULL));
		ok = setlocale (LC_ALL, lang) != NULL;
		setlocale (LC_ALL, oldlocale);
		g_free (oldlocale);

		if (!ok)
			break;
		ADD_OP (OP_LOCALE);
		g_string_append_len (prg, (void *)&locale, sizeof (locale));
		/* Include the terminating zero: */
		g_string_append_len (prg, lang, strlen (lang) + 1);
		break;
	}

	case TOK_NULL:
		break;

	default:
		if (t < 0x80) {
			ADD_OP2 (OP_CHAR, t);
		}
		break;
	}
}

static void
handle_fill (GString *prg, const GOFormatParseState *pstate)
{
	if (pstate->fill_char) {
		ADD_OP (OP_FILL);
		g_string_append_unichar (prg, pstate->fill_char);
	}
}


static GOFormat *
go_format_parse_sequential (const char *str, GString *prg,
			    const GOFormatParseState *pstate)
{
	int date_decimals = 0;
	gboolean seen_date = FALSE;
	gboolean seen_year = FALSE;
	gboolean seen_month = FALSE;
	gboolean seen_day = FALSE;
	gboolean seen_time = FALSE;
	gboolean seen_hour = FALSE;
	gboolean seen_ampm = FALSE;
	gboolean seen_minute = FALSE;
	gboolean seen_elapsed = FALSE;
	gboolean m_is_minutes = FALSE;
	gboolean seconds_trigger_minutes = TRUE;
	gboolean date_ybm = FALSE;
	gboolean date_mbd = FALSE;
	gboolean date_dbm = FALSE;
	gboolean prg_was_null = (prg == NULL);

	if (!prg)
		prg = g_string_new (NULL);

	while (1) {
		const char *token = str;
		GOFormatTokenType tt;
		int t = go_format_token (&str, &tt);

		switch (t) {
		case 0: case ';':
			goto done;

		case 'd': case 'D': {
			int n = 1;
			while (*str == 'd' || *str == 'D')
				str++, n++;
			seen_date = TRUE;
			switch (n) {
			case 1: ADD_OP (OP_DATE_DAY); break;
			case 2: ADD_OP (OP_DATE_DAY_2); break;
			case 3: ADD_OP (OP_DATE_WEEKDAY_3); break;
			default: ADD_OP (OP_DATE_WEEKDAY); break;
			}
			if (!seen_day) {
				seen_day = TRUE;
				if (seen_month)
					date_mbd = TRUE;
			}
			break;
		}

		case 'y': case 'Y': {
			int n = 1;
			while (*str == 'y' || *str == 'Y')
				str++, n++;
			seen_date = TRUE;
			seen_year = TRUE;
			ADD_OP (n <= 2 ? OP_DATE_YEAR_2 : OP_DATE_YEAR);
			break;
		}

		case 'b': case 'B': {
			int n = 1;
			while (*str == 'b' || *str == 'B')
				str++, n++;
			seen_date = TRUE;
			seen_year = TRUE;
			ADD_OP (n <= 2 ? OP_DATE_YEAR_THAI_2 : OP_DATE_YEAR_THAI);
			break;
		}

		case 'e':
			while (*str == 'e') str++;
			seen_date = TRUE;
			seen_year = TRUE;
			ADD_OP (OP_DATE_YEAR);
			break;

		case 'g': case 'G':
			/* Something with Japanese eras.  Blank for me. */
			seen_date = TRUE;
			break;

		case 'h': case 'H': {
			int n = 1;
			while (*str == 'h' || *str == 'H')
				str++, n++;
			seen_time = TRUE;
			ADD_OP (n == 1 ? OP_TIME_HOUR : OP_TIME_HOUR_2);
			seen_hour = TRUE;
			m_is_minutes = TRUE;
			break;
		}

		case 'm': case 'M': {
			int n = 1;
			while (*str == 'm' || *str == 'M')
				str++, n++;
			m_is_minutes = (n <= 2) && (m_is_minutes || tail_forces_minutes (str));

			if (m_is_minutes) {
				seen_time = TRUE;
				seconds_trigger_minutes = FALSE;
				ADD_OP (n == 1 ? OP_TIME_MINUTE : OP_TIME_MINUTE_2);
				m_is_minutes = FALSE;
				seen_minute = TRUE;
			} else {
				seen_date = TRUE;
				switch (n) {
				case 1: ADD_OP (OP_DATE_MONTH); break;
				case 2: ADD_OP (OP_DATE_MONTH_2); break;
				case 3: ADD_OP (OP_DATE_MONTH_NAME_3); break;
				case 5: ADD_OP (OP_DATE_MONTH_NAME_1); break;
				default: ADD_OP (OP_DATE_MONTH_NAME); break;
				}
				if (!seen_month) {
					seen_month = TRUE;
					if (seen_day)
						date_dbm = TRUE;
					if (seen_year && !seen_day)
						date_ybm = TRUE;
				}
			}
			break;
		}

		case 's': case 'S': {
			int n = 1;
			while (*str == 's' || *str == 'S')
				str++, n++;

			if (seconds_trigger_minutes) {
				seconds_trigger_minutes = FALSE;
				m_is_minutes = TRUE;
			}

			seen_time = TRUE;
			ADD_OP (n == 1 ? OP_TIME_SECOND : OP_TIME_SECOND_2);

			break;
		}

		case TOK_AMPM3:
			if (seen_elapsed)
				goto error;
			seen_time = TRUE;
			seen_ampm = TRUE;
			ADD_OP3 (OP_TIME_AP, token[0], token[2]);
			break;

		case TOK_AMPM5:
			if (seen_elapsed)
				goto error;
			seen_time = TRUE;
			seen_ampm = TRUE;
			ADD_OP (OP_TIME_AMPM);
			break;

		case TOK_DECIMAL:
			if (*str == '0') {
				int n = 0;
				seen_time = TRUE;
				ADD_OP (OP_TIME_SECOND_DECIMAL_START);
				while (*str == '0') {
					str++, n++;
					ADD_OP (OP_TIME_SECOND_DECIMAL_DIGIT);
				}
				/* The actual limit is debatable.  This is what XL does.  */
				if (n > 3)
					goto error;
				date_decimals = MAX (date_decimals, n);
			} else {
				ADD_OP2 (OP_CHAR, '.');
			}
			break;

		case '0':
			goto error;

		case TOK_ELAPSED_H:
			if (seen_elapsed || seen_ampm)
				goto error;
			seen_time = TRUE;
			seen_elapsed = TRUE;
			seen_hour = TRUE;
			m_is_minutes = TRUE;
			ADD_OP2 (OP_TIME_HOUR_N, str - token - 2);
			break;

		case TOK_ELAPSED_M:
			if (seen_elapsed || seen_ampm)
				goto error;
			seen_time = TRUE;
			seen_elapsed = TRUE;
			seen_minute = TRUE;
			m_is_minutes = FALSE;
			ADD_OP2 (OP_TIME_MINUTE_N, str - token - 2);
			break;

		case TOK_ELAPSED_S:
			if (seen_elapsed || seen_ampm)
				goto error;
			seen_time = TRUE;
			seen_elapsed = TRUE;
			if (seconds_trigger_minutes) {
				m_is_minutes = TRUE;
				seconds_trigger_minutes = FALSE;
			}
			ADD_OP2 (OP_TIME_SECOND_N, str - token - 2);
			break;

		case TOK_GENERAL:
			ADD_OP (OP_NUM_GENERAL_MARK);
			break;

		case '@':
			ADD_OP (OP_STR_APPEND_SVAL);
			break;

		default:
			handle_common_token (token, t, prg);
			break;
		}
	}

 done:
	if (pstate->typ == GO_FMT_TEXT) {
		GOFormat *fmt = go_format_create (GO_FMT_TEXT, NULL);
		handle_fill (prg, pstate);
		fmt->u.text.program = g_string_free (prg, FALSE);
		return fmt;
	} else {
		GOFormat *fmt = go_format_create (GO_FMT_NUMBER, NULL);
		guchar splits[5] = { OP_DATE_ROUND, date_decimals, seen_elapsed };
		guchar *p = splits + 3;
		if (seen_date) {
			*p++ = OP_DATE_SPLIT;
			fmt->u.number.has_date = TRUE;
			fmt->u.number.date_ybm = date_ybm;
			fmt->u.number.date_mbd = date_mbd;
			fmt->u.number.date_dbm = date_dbm;
		}
		if (seen_time) {
			guchar op;
			if (seen_elapsed) {
				if (seen_hour)
					op = OP_TIME_SPLIT_ELAPSED_HOUR;
				else if (seen_minute)
					op = OP_TIME_SPLIT_ELAPSED_MINUTE;
				else
					op = OP_TIME_SPLIT_ELAPSED_SECOND;
			} else {
				op = seen_ampm
					? OP_TIME_SPLIT_12
					: OP_TIME_SPLIT_24;
			}
			*p++ = op;
			fmt->u.number.has_time = TRUE;
			fmt->u.number.has_hour = seen_hour;
			fmt->u.number.has_minute = seen_minute;
			fmt->u.number.has_elapsed = seen_elapsed;
		}
		if (pstate->has_general) {
			ADD_OP (OP_NUM_GENERAL_DO);
			fmt->u.number.has_general = pstate->has_general;
			fmt->u.number.is_general = pstate->is_general;
		}
		handle_fill (prg, pstate);
		g_string_insert_len (prg, 0, splits, p - splits);
		fmt->u.number.program = g_string_free (prg, FALSE);
		return fmt;
	}

 error:
	if (prg_was_null)
		g_string_free (prg, TRUE);

	return NULL;
}

static gboolean
comma_is_thousands (const char *str)
{
	while (1) {
		GOFormatTokenType tt;
		int t = go_format_token (&str, &tt);

		switch (t) {
		case '0': case '?': case '#':
			return TRUE;
		case 0:
		case ';':
		case TOK_DECIMAL:
			return FALSE;
		}
	}
}

static gboolean
go_format_parse_number_new_1 (GString *prg, GOFormatParseState *pstate,
			      int tno_start, int tno_end,
			      int E_part, int frac_part)
{
	int decimals = 0;
	int whole_digits = 0;
	gboolean inhibit_thousands = (E_part == 2) || (frac_part >= 2);
	gboolean thousands = FALSE;
	gboolean whole_part = TRUE;
	int scale = 0;
	int first_digit_pos = -1;
	int dot_pos = -1;
	int one_pos;
	int i;
	int tno_numstart = -1;
	int force_zero_pos = frac_part == 3 ? pstate->force_zero_pos : INT_MAX;

	for (i = tno_start; i < tno_end; i++) {
		const GOFormatParseItem *ti = &GET_TOKEN(i);

		if (tno_numstart == -1 && (ti->tt & TT_STARTS_NUMBER))
			tno_numstart = i;

		switch (ti->token) {
		case TOK_DECIMAL:
			if (!whole_part)
				break;
			dot_pos = i;
			if (first_digit_pos == -1)
				first_digit_pos = i;
			whole_part = FALSE;
			break;

		case '0': case '?': case '#':
			if (first_digit_pos == -1)
				first_digit_pos = i;
			if (whole_part)
				whole_digits++;
			else
				decimals++;
			break;

		case '%':
			if (E_part != 2)
				scale += 2;
			break;

		case TOK_THOUSAND:
			if (tno_numstart != -1 && E_part != 2) {
				if (comma_is_thousands (ti->tstr)) {
					if (whole_part)
						thousands = TRUE;
					if (ti->tstr[1] == ' ')
						inhibit_thousands = TRUE;
				} else {
					if (frac_part == 0)
						scale -= 3;
				}
			}
			break;

		default:
			break;
		}
	}

	if (E_part == 1) {
		if (tno_numstart == -1)
			goto error;

		ADD_OP3 (OP_NUM_PRINTF_E, decimals, whole_digits);
#ifdef OBSERVE_XL_EXPONENT_1
		if (whole_digits + decimals == 0) {
			/*
			 * If no digits precede "E", pretend that the
			 * exponent is 1.  Don't ask!
			 */
			ADD_OP (OP_NUM_EXPONENT_1);
		}
#endif
	} else {
		if (scale && !frac_part)
			ADD_OP2 (OP_NUM_SCALE, scale);
		ADD_OP2 (OP_NUM_PRINTF_F, decimals);
		if (thousands && !inhibit_thousands)
			ADD_OP (OP_NUM_ENABLE_THOUSANDS);
	}
	ADD_OP (OP_NUM_SIGN);

	for (i = tno_start; i < tno_numstart; i++) {
		const GOFormatParseItem *ti = &GET_TOKEN(i);
		handle_common_token (ti->tstr, ti->token, prg);
	}

	if (E_part == 1) {
#ifdef ALLOW_EE_MARKUP
		ADD_OP (OP_NUM_MARK_MANTISSA);
#endif
	} else if (E_part == 2) {
		ADD_OP2 (OP_NUM_EXPONENT_SIGN, pstate->forced_exponent_sign);
		if (first_digit_pos == -1)
			goto error;
	}

	one_pos = (dot_pos == -1) ? tno_end - 1 : dot_pos - 1;
	ADD_OP (OP_NUM_MOVETO_ONES);
	for (i = one_pos; i >= tno_numstart; i--) {
		const GOFormatParseItem *ti = &GET_TOKEN(i);

		if (pstate->explicit_denom && g_ascii_isdigit (ti->token)) {
			ADD_OP2 (OP_CHAR, ti->token);
			continue;
		}

		switch (ti->token) {
		case '?':
			if (frac_part == 3 && i < force_zero_pos) {
				ADD_OP (OP_NUM_DENUM_DIGIT_Q);
				break;
			}
			/* Fall through  */
		case '0':
		case '#':
			if (i >= force_zero_pos)
				ADD_OP2 (OP_NUM_DIGIT_1_0, ti->token);
			else
				ADD_OP2 (OP_NUM_DIGIT_1, ti->token);
			break;
		case TOK_THOUSAND:
			if (frac_part == 3)
				ADD_OP2 (OP_CHAR, ',');
			break;
		default:
			handle_common_token (ti->tstr, ti->token, prg);
		}
		if (i == first_digit_pos)
			ADD_OP (OP_NUM_REST_WHOLE);
	}
	ADD_OP (OP_NUM_APPEND_MODE);

	if (dot_pos >= 0) {
		ADD_OP (OP_NUM_DECIMAL_POINT);
		ADD_OP (OP_NUM_MOVETO_DECIMALS);
		for (i = dot_pos + 1; i < tno_end; i++) {
			const GOFormatParseItem *ti = &GET_TOKEN(i);

			switch (ti->token) {
			case '0':
			case '?':
			case '#':
				ADD_OP2 (OP_NUM_DECIMAL_1, ti->token);
				break;
			case TOK_THOUSAND:
				break;
			default:
				handle_common_token (ti->tstr, ti->token, prg);
			}
		}
	}

	pstate->scale = scale;
	return TRUE;

 error:
	g_string_free (prg, TRUE);
	return FALSE;
}


static GOFormat *
go_format_parse_number_plain (GOFormatParseState *pstate)
{
	GOFormat *fmt;
	GString *prg = g_string_new (NULL);

	if (!go_format_parse_number_new_1 (prg, pstate,
					   0, pstate->tokens->len,
					   0, 0))
		return NULL;

	handle_fill (prg, pstate);

	fmt = go_format_create (GO_FMT_NUMBER, NULL);
	fmt->u.number.program = g_string_free (prg, FALSE);
	fmt->u.number.scale_is_2 = (pstate->scale == 2);
	return fmt;
}

static GOFormat *
go_format_parse_number_E (GOFormatParseState *pstate)
{
	GOFormat *fmt;
	GString *prg;
	gboolean use_markup;
	int tno_end = pstate->tokens->len;
	int tno_exp_start = pstate->tno_E + 2;
	gboolean simplify_mantissa;

	if (tno_exp_start >= tno_end)
		return NULL;
	switch (GET_TOKEN (pstate->tno_E + 1).token) {
	case '-':
		use_markup = FALSE;
		simplify_mantissa = FALSE;
		pstate->forced_exponent_sign = FALSE;
		break;
	case '+':
		use_markup = FALSE;
		simplify_mantissa = FALSE;
		pstate->forced_exponent_sign = TRUE;
		break;
#ifdef ALLOW_EE_MARKUP
	case 'E': {
		int i;

		use_markup = TRUE;
		pstate->forced_exponent_sign =
			(GET_TOKEN (tno_exp_start).token == '+');
		if (pstate->forced_exponent_sign)
			tno_exp_start++;

		simplify_mantissa = TRUE;
		for (i = 0; i < pstate->tno_E; i++) {
			if (GET_TOKEN(i).token == TOK_DECIMAL)
				break;
			if (GET_TOKEN(i).token == '0') {
				simplify_mantissa = FALSE;
				break;
			}
		}

		break;
	}
#endif
	default:
		return NULL;
	}

	prg = g_string_new (NULL);

	if (!go_format_parse_number_new_1 (prg, pstate,
					   0, pstate->tno_E,
					   1, 0))
		return NULL;

	ADD_OP (OP_NUM_VAL_EXPONENT);
#ifdef ALLOW_EE_MARKUP
	if (use_markup) {
		ADD_OPuc (OP_CHAR, UNICODE_TIMES);
		if (simplify_mantissa)
			ADD_OP (OP_NUM_SIMPLIFY_MANTISSA);
		ADD_OP2 (OP_CHAR, '1');
		ADD_OP2 (OP_CHAR, '0');
		ADD_OP2 (OP_CHAR, '<');
		ADD_OP2 (OP_CHAR, 's');
		ADD_OP2 (OP_CHAR, 'u');
		ADD_OP2 (OP_CHAR, 'p');
		ADD_OP2 (OP_CHAR, '>');
	} else
#endif
		ADD_OP2 (OP_CHAR, 'E');

	if (!go_format_parse_number_new_1 (prg, pstate,
					   tno_exp_start, tno_end,
					   2, 0))
		return NULL;

#ifdef ALLOW_EE_MARKUP
	if (use_markup) {
		ADD_OP2 (OP_CHAR, '<');
		ADD_OP2 (OP_CHAR, '/');
		ADD_OP2 (OP_CHAR, 's');
		ADD_OP2 (OP_CHAR, 'u');
		ADD_OP2 (OP_CHAR, 'p');
		ADD_OP2 (OP_CHAR, '>');
		if (simplify_mantissa)
			ADD_OP (OP_NUM_SIMPLIFY_EXPONENT);
	}
#endif

	handle_fill (prg, pstate);

	fmt = go_format_create (GO_FMT_NUMBER, NULL);
	fmt->u.number.program = g_string_free (prg, FALSE);
	fmt->u.number.E_format = TRUE;
	fmt->u.number.use_markup = use_markup;
	fmt->u.number.scale_is_2 = (pstate->scale == 2);
	return fmt;
}

static GOFormat *
go_format_parse_number_fraction (GOFormatParseState *pstate)
{
	GOFormat *fmt;
	GString *prg;
	int tno_slash = pstate->tno_slash;
	int tno_end = pstate->tokens->len;
	int tno_suffix = tno_end;
	int tno_endwhole, tno_denom;
	int i;
	double denominator = 0;
	gboolean explicit_denom = FALSE;
	int denominator_digits = 0;
	gboolean inhibit_blank = FALSE;
	int scale = 0;
#ifdef ALLOW_PI_SLASH
	gboolean pi_scale = (tno_slash >= 2 &&
			     GET_TOKEN(tno_slash - 2).token == 'p' &&
			     GET_TOKEN(tno_slash - 1).token == 'i');
#else
	gboolean pi_scale = FALSE;
#endif


	/*
	 * First determine where the whole part, if any, ends.
	 *
	 * ???? ???/??? xxx
	 *     ^   ^    ^
	 *     |   |    +--- tno_suffix
	 *     |   +-------- tno_slash
	 *     +------------ tno_endwhole
	 */

	i = tno_slash - 1;
	/* Back up to digit. */
	while (i >= 0) {
		int t = GET_TOKEN (i).token;
		if (t == '0' || t == '#' || t == '?')
			break;
		i--;
	}

	/* Back up to space. */
	while (i >= 0) {
		int t = GET_TOKEN (i).token;
		if (t == ' ')
			break;
		i--;
	}
	tno_endwhole = i;

	/* Back up to digit. */
	while (i >= 0) {
		int t = GET_TOKEN (i).token;
		if (t == '0' || t == '#' || t == '?')
			break;
		i--;
	}

	if (i < 0) {
		tno_endwhole = -1;
		inhibit_blank = TRUE;
	} else {
		for (i = tno_endwhole; i < tno_slash; i++) {
			int t = GET_TOKEN (i).token;
			if (t == '0')
				inhibit_blank = TRUE;
		}
	}

	/* ---------------------------------------- */

	i = tno_slash + 1;
	while (i < tno_end) {
		int t = GET_TOKEN (i).token;
		if (g_ascii_isdigit (t) || t == '#' || t == '?')
			break;
		i++;
	}
	if (i == tno_end)
		return NULL;
	tno_denom = i;

	for (i = tno_denom; i < tno_end; i++) {
		int t = GET_TOKEN (i).token;
		if (g_ascii_isdigit (t)) {
			denominator = denominator * 10 + (t - '0');
			if (t != '0')
				explicit_denom = TRUE;
		}
	}

	for (i = tno_denom; i < tno_end; i++) {
		const GOFormatParseItem *ti = &GET_TOKEN(i);
		if (ti->token == TOK_THOUSAND)
			return NULL;
		if (ti->token == TOK_CONDITION ||
		    ti->token == TOK_LOCALE ||
		    ti->token == TOK_COLOR)
			continue;
		if (!(ti->tt & TT_STARTS_NUMBER))
			break;
		if (explicit_denom && (ti->token == '?' || ti->token == '#'))
			return NULL;
		denominator_digits++;
	}
	pstate->force_zero_pos = i;

	while (tno_suffix >= i) {
		int token = GET_TOKEN(tno_suffix - 1).token;
		if (token == '#' || token == '?' || g_ascii_isdigit (token))
			break;
		tno_suffix--;
	}

	/* ---------------------------------------- */

	prg = g_string_new (NULL);

#ifdef ALLOW_PI_SLASH
	if (pi_scale)
		ADD_OP (OP_NUM_FRACTION_SCALE_PI);
#endif

	ADD_OP3 (OP_NUM_FRACTION, tno_endwhole != -1, explicit_denom);
	if (explicit_denom)
		g_string_append_len (prg, (void*)&denominator, sizeof (denominator));
	else
		ADD_OP (MIN (10, denominator_digits));

	if (tno_endwhole != -1) {
		ADD_OP (OP_NUM_FRACTION_WHOLE);
		if (!go_format_parse_number_new_1 (prg, pstate,
						   0, tno_endwhole + 1,
						   0, 1))
			return NULL;
		scale += pstate->scale;
	}
	ADD_OP (OP_NUM_DISABLE_THOUSANDS);

	ADD_OP (OP_NUM_FRACTION_NOMINATOR);
	if (!go_format_parse_number_new_1 (prg, pstate,
					   tno_endwhole + 1,
					   pi_scale ? tno_slash - 2 :tno_slash,
					   0, 2))
		return NULL;
	scale += pstate->scale;

	if (pi_scale)
		ADD_OPuc (OP_CHAR, UNICODE_PI); /* "pi" */
	ADD_OP2 (OP_CHAR, '/');

	pstate->explicit_denom = explicit_denom;
	ADD_OP (OP_NUM_FRACTION_DENOMINATOR);
	if (!go_format_parse_number_new_1 (prg, pstate,
					   tno_slash + 1, tno_suffix,
					   0, 3))
		return NULL;
	scale += pstate->scale;
	if (!inhibit_blank)
		ADD_OP (OP_NUM_FRACTION_BLANK);
#ifdef ALLOW_PI_SLASH
	else if (pi_scale)
		ADD_OP (OP_NUM_FRACTION_SIMPLIFY_PI);
#endif

	for (i = tno_suffix; i < tno_end; i++) {
		const GOFormatParseItem *ti = &GET_TOKEN(i);
		if (ti->token == '%')
			scale += 2;
		handle_common_token (ti->tstr, ti->token, prg);
	}

	if (scale) {
		guchar scaling[2] = { OP_NUM_SCALE, scale };
		g_string_insert_len (prg, 0, scaling, 2);
	}

	handle_fill (prg, pstate);

	fmt = go_format_create (GO_FMT_NUMBER, NULL);
	fmt->u.number.program = g_string_free (prg, FALSE);
	fmt->u.number.fraction = TRUE;
	fmt->u.number.scale_is_2 = (pstate->scale == 2);
	return fmt;
}

static GOFormat *
go_format_parse (const char *str)
{
	GOFormat *fmt;
	const char *str0 = str;
	GOFormatCondition *conditions = NULL;
	int i, nparts = 0;
	gboolean has_text_format = FALSE;

#if 0
	g_printerr ("Parse: [%s]\n", str0);
#endif
	while (1) {
		GOFormatCondition *condition;
		const char *tail;
		GOFormatParseState state;
		GOFormat *fmt = NULL;
		gboolean is_magic = FALSE;
		char *magic_fmt_str;

		memset (&state, 0, sizeof (state));
		tail = go_format_preparse (str, &state, FALSE, FALSE);
		if (!tail) {
			g_array_free (state.tokens, TRUE);
			goto bail;
		}

		nparts++;
		conditions = g_renew (GOFormatCondition, conditions, nparts);
		condition = conditions + (nparts - 1);
		*condition = state.cond;
		if (!state.have_cond)
			condition->implicit = TRUE;

		magic_fmt_str = go_format_magic_fmt_str (state.locale.locale);
		if (magic_fmt_str) {
			is_magic = TRUE;
			/* Make the upcoming switch do nothing.  */
			state.typ = GO_FMT_INVALID;
			fmt = go_format_parse_sequential (magic_fmt_str, NULL, &state);
			g_free (magic_fmt_str);
		}

		switch (state.typ) {
		case GO_FMT_EMPTY:
			fmt = go_format_create (state.typ, NULL);
			break;

		case GO_FMT_TEXT:
			fmt = go_format_parse_sequential (str, NULL, &state);
			break;

		case GO_FMT_NUMBER:
			if (state.is_date || state.has_general)
				fmt = go_format_parse_sequential (str, NULL, &state);
			else if (state.tno_E >= 0)
				fmt = go_format_parse_number_E (&state);
			else if (state.tno_slash >= 0)
				fmt = go_format_parse_number_fraction (&state);
			else if (state.is_number) {
				fmt = go_format_parse_number_plain (&state);
			} else {
				GString *prg = g_string_new (NULL);
				/* Crazy number.  Sign only.  */
				ADD_OP (OP_NUM_VAL_SIGN);
				fmt = go_format_parse_sequential (str, prg, &state);
			}
			break;

		default:
			; /* Nothing */
		}
		g_array_free (state.tokens, TRUE);
		if (!fmt)
			goto bail;

		condition->fmt = fmt;
		fmt->format = g_strndup (str, tail - str);
		fmt->has_fill = state.fill_char != 0;
		fmt->color = state.color;
		if (is_magic) fmt->magic = state.locale.locale;

		if (go_format_is_text (fmt)) {
			/* Only one text format.  */
			if (has_text_format)
				goto bail;
			has_text_format = TRUE;
			if (condition->implicit)
				condition->op = GO_FMT_COND_TEXT;
		}

		str = tail;
		if (*str == 0)
			break;
		str++;
	}

	if (nparts == 1 && conditions[0].implicit) {
		/* Simple. */
		fmt = conditions[0].fmt;
		g_free (conditions);
	} else {
		int i;

		fmt = go_format_create (GO_FMT_COND, str0);
		fmt->u.cond.n = nparts;
		fmt->u.cond.conditions = conditions;

		for (i = 0; i < nparts; i++) {
			gboolean no_zero_format =
				(nparts <= 2 ||
				 conditions[2].op != GO_FMT_COND_NONE);
			gboolean negative_explicit =
				(nparts >= 2 &&
				 conditions[1].op != GO_FMT_COND_NONE);
			static const GOFormatConditionOp ops[4] = {
				GO_FMT_COND_GT,
				GO_FMT_COND_LT,
				GO_FMT_COND_EQ,
				GO_FMT_COND_TEXT
			};
			GOFormatCondition *cond = conditions + i;
			if (i <= 4 && cond->op == GO_FMT_COND_NONE) {
				cond->implicit = TRUE;
				cond->val = 0;
				if (i == 0 && no_zero_format && !negative_explicit)
					cond->op = GO_FMT_COND_GE;
				else if (i == 1 && no_zero_format) {
					if (!conditions[0].implicit &&
					    conditions[0].true_inhibits_minus)
						conditions[0].false_inhibits_minus = TRUE;
					cond->op = GO_FMT_COND_NONTEXT;
				} else
					cond->op = ops[i];
				determine_inhibit_minus (cond);
			}
#ifdef OBSERVE_XL_CONDITION_LIMITS
			if (i >= 2 && !cond->implicit) {
				go_format_unref (fmt);
				nparts = 0;
				conditions = NULL;
				goto bail;
			}
#endif
		}
	}

	return fmt;

 bail:
	for (i = 0; i < nparts; i++)
		go_format_unref (conditions[i].fmt);
	g_free (conditions);
	return go_format_create (GO_FMT_INVALID, str0);
}

#undef ADD_OP
#undef ADD_OP2
#undef ADD_OP3

#ifdef DEBUG_PROGRAMS
#define REGULAR(op) case op: g_printerr ("%s\n", #op); break
static void
go_format_dump_program (const guchar *prg)
{
	const guchar *next;
	size_t len;

	while (1) {
		GOFormatOp op = *prg++;

		switch (op) {
		case OP_END:
			g_printerr ("OP_END\n");
			return;
		case OP_CHAR:
			next = g_utf8_next_char (prg);
			len = next - prg;
			g_printerr ("OP_CHAR '%-.*s'\n",
				    (int)len, prg);
			prg = next;
			break;
		case OP_CHAR_INVISIBLE:
			next = g_utf8_next_char (prg);
			len = next - prg;
			g_printerr ("OP_CHAR_INVISIBLE '%-.*s'\n",
				    (int)len, prg);
			prg = next;
			break;
		case OP_STRING:
			len = strlen (prg);
			g_printerr ("OP_STRING \"%s\"\n", prg);
			prg += len + 1;
			break;
		case OP_FILL:
			next = g_utf8_next_char (prg);
			len = next - prg;
			g_printerr ("OP_FILL '%-.*s'\n", (int)len, prg);
			prg = next;
			break;
		case OP_LOCALE: {
			GOFormatLocale locale;
			const char *lang;
			memcpy (&locale, prg, sizeof (locale));
			prg += sizeof (locale);
			lang = (const char *)prg;
			prg += strlen (lang) + 1;
			g_printerr ("OP_LOCALE -- \"%s\"\n", lang);
			break;
		}
		case OP_DATE_ROUND:
			g_printerr ("OP_DATE_ROUND %d %d\n", prg[0], prg[1]);
			prg += 2;
			break;
		case OP_TIME_HOUR_N:
			g_printerr ("OP_TIME_HOUR_N %d\n", prg[0]);
			prg += 1;
			break;
		case OP_TIME_AP:
			g_printerr ("OP_TIME_AP '%c' '%c'\n", prg[0], prg[1]);
			prg += 2;
			break;
		case OP_TIME_MINUTE_N:
			g_printerr ("OP_TIME_MINUTE_N %d\n", prg[0]);
			prg += 1;
			break;
		case OP_TIME_SECOND_N:
			g_printerr ("OP_TIME_SECOND_N %d\n", prg[0]);
			prg += 1;
			break;
		case OP_NUM_SCALE:
			g_printerr ("OP_NUM_SCALE %d\n", (signed char)(prg[0]));
			prg += 1;
			break;
		case OP_NUM_PRINTF_E:
			g_printerr ("OP_NUM_PRINTF_E %d %d\n", prg[0], prg[1]);
			prg += 2;
			break;
		case OP_NUM_PRINTF_F:
			g_printerr ("OP_NUM_PRINTF_F %d\n", prg[0]);
			prg += 1;
			break;
		case OP_NUM_DIGIT_1:
			g_printerr ("OP_NUM_DIGIT_1 '%c'\n", prg[0]);
			prg += 1;
			break;
		case OP_NUM_DECIMAL_1:
			g_printerr ("OP_NUM_DECIMAL_1 '%c'\n", prg[0]);
			prg += 1;
			break;
		case OP_NUM_DIGIT_1_0:
			g_printerr ("OP_NUM_DIGIT_1_0 '%c'\n", prg[0]);
			prg += 1;
			break;
		case OP_NUM_EXPONENT_SIGN:	/* forced-p */
			g_printerr ("OP_NUM_EXPONENT_SIGN %d\n", prg[0]);
			prg += 1;
			break;
		case OP_NUM_FRACTION: {
			gboolean wp = *prg++;
			gboolean explicit_denom = *prg++;

			if (explicit_denom) {
				double plaind; /* Plain double */
				memcpy (&plaind, prg, sizeof (plaind));
				prg += sizeof (plaind);

				g_printerr ("OP_NUM_FRACTION %d %d %g\n", wp, explicit_denom, plaind);
			} else {
				int digits = *prg++;

				g_printerr ("OP_NUM_FRACTION %d %d %d\n", wp, explicit_denom, digits);
			}
			break;
		}

		REGULAR(OP_CHAR_REPEAT);
		REGULAR(OP_DATE_SPLIT);
		REGULAR(OP_DATE_YEAR);
		REGULAR(OP_DATE_YEAR_2);
		REGULAR(OP_DATE_YEAR_THAI);
		REGULAR(OP_DATE_YEAR_THAI_2);
		REGULAR(OP_DATE_MONTH);
		REGULAR(OP_DATE_MONTH_2);
		REGULAR(OP_DATE_MONTH_NAME);
		REGULAR(OP_DATE_MONTH_NAME_1);
		REGULAR(OP_DATE_MONTH_NAME_3);
		REGULAR(OP_DATE_DAY);
		REGULAR(OP_DATE_DAY_2);
		REGULAR(OP_DATE_WEEKDAY);
		REGULAR(OP_DATE_WEEKDAY_3);
		REGULAR(OP_TIME_SPLIT_24);
		REGULAR(OP_TIME_SPLIT_12);
		REGULAR(OP_TIME_SPLIT_ELAPSED_HOUR);
		REGULAR(OP_TIME_SPLIT_ELAPSED_MINUTE);
		REGULAR(OP_TIME_SPLIT_ELAPSED_SECOND);
		REGULAR(OP_TIME_HOUR);
		REGULAR(OP_TIME_HOUR_2);
		REGULAR(OP_TIME_AMPM);
		REGULAR(OP_TIME_MINUTE);
		REGULAR(OP_TIME_MINUTE_2);
		REGULAR(OP_TIME_SECOND);
		REGULAR(OP_TIME_SECOND_2);
		REGULAR(OP_TIME_SECOND_DECIMAL_START);
		REGULAR(OP_TIME_SECOND_DECIMAL_DIGIT);
		REGULAR(OP_NUM_ENABLE_THOUSANDS);
		REGULAR(OP_NUM_DISABLE_THOUSANDS);
		REGULAR(OP_NUM_SIGN);
		REGULAR(OP_NUM_VAL_SIGN);
		REGULAR(OP_NUM_MOVETO_ONES);
		REGULAR(OP_NUM_MOVETO_DECIMALS);
		REGULAR(OP_NUM_REST_WHOLE);
		REGULAR(OP_NUM_APPEND_MODE);
		REGULAR(OP_NUM_DECIMAL_POINT);
		REGULAR(OP_NUM_DENUM_DIGIT_Q);
		REGULAR(OP_NUM_EXPONENT_1);
		REGULAR(OP_NUM_VAL_EXPONENT);
#ifdef ALLOW_EE_MARKUP
		REGULAR(OP_NUM_MARK_MANTISSA);
		REGULAR(OP_NUM_SIMPLIFY_MANTISSA);
		REGULAR(OP_NUM_SIMPLIFY_EXPONENT);
#endif
		REGULAR(OP_NUM_FRACTION_WHOLE);
		REGULAR(OP_NUM_FRACTION_NOMINATOR);
		REGULAR(OP_NUM_FRACTION_DENOMINATOR);
		REGULAR(OP_NUM_FRACTION_BLANK);
#ifdef ALLOW_PI_SLASH
		REGULAR(OP_NUM_FRACTION_SCALE_PI);
		REGULAR(OP_NUM_FRACTION_SIMPLIFY_PI);
#endif
		REGULAR(OP_NUM_GENERAL_MARK);
		REGULAR(OP_NUM_GENERAL_DO);
		REGULAR(OP_STR_APPEND_SVAL);

		default:
			g_printerr ("???\n");
			return;
		}
	}
}
#undef REGULAR
#endif

static void
append_i2 (GString *dst, int i)
{
	g_string_append_printf (dst, "%02d", i);
}

static void
append_i (GString *dst, int i)
{
	g_string_append_printf (dst, "%d", i);
}

static const char unicode_minus_utf8[3] = "\xe2\x88\x92";

#define SETUP_LAYOUT do { if (layout) pango_layout_set_text (layout, str->str, -1); } while (0)

static void
fill_with_char (GString *str, PangoLayout *layout, gsize fill_pos,
		gunichar fill_char,
		GOFormatMeasure measure, int col_width)
{
	int w, w1, wbase;
	gsize n, gap;
	char fill_utf8[7];
	gsize fill_utf8_len;

	SETUP_LAYOUT;
	wbase = measure (str, layout);
	if (wbase >= col_width)
		return;

	fill_utf8_len = g_unichar_to_utf8 (fill_char, fill_utf8);

	g_string_insert_len (str, fill_pos, fill_utf8, fill_utf8_len);
	SETUP_LAYOUT;
	w = measure (str, layout);
	w1 = w - wbase;
	if (w > col_width || w1 <= 0) {
		g_string_erase (str, fill_pos, fill_utf8_len);
		return;
	}

	n = (col_width - w) / w1;
	if (n == 0)
		return;

	gap = n * fill_utf8_len;
	g_string_set_size (str, str->len + gap);
	g_memmove (str->str + fill_pos + gap,
		   str->str + fill_pos,
		   str->len - (fill_pos + gap));
	while (n > 0) {
		memcpy (str->str + fill_pos, fill_utf8, fill_utf8_len);
		fill_pos += fill_utf8_len;
		n--;
	}
}

#endif

static void
SUFFIX(printf_engineering) (GString *dst, DOUBLE val, int n, int wd)
{
	int exponent_guess = 0;
	int exponent;
	int nde = 0;
	char *epos;
	char *dot;
	const GString *decimal = go_locale_get_decimal ();

	if (wd <= 1 || val == 0 || !SUFFIX(go_finite) (val)) {
		g_string_printf (dst, "%.*" FORMAT_E, n, val);
		return;
	}

	exponent_guess = (int)floor (SUFFIX(log10) (SUFFIX(fabs) (val)));
	/* Extra digits we need assuming guess correct */
	nde = (exponent_guess >= 0)
		? exponent_guess % wd
		: (wd - ((-exponent_guess) % wd)) % wd;

	g_string_printf (dst, "%.*" FORMAT_E, n + nde, val);
	epos = (char *)strchr (dst->str, 'E');
	if (!epos)
		return;

	exponent = atoi (epos + 1);
	g_string_truncate (dst, epos - dst->str);
	if (exponent != exponent_guess) {
		/*
		 * We rounded from 9.99Exx to
		 *                 1.00Eyy
		 * with yy=xx+1.
		 */
		nde = (nde + 1) % wd;
		if (nde == 0)
			g_string_truncate (dst, dst->len - (wd - 1));
		else
			g_string_append_c (dst, '0');
	}

	dot = (char *)strstr (dst->str, decimal->str);
	if (dot) {
		memmove (dot, dot + decimal->len, nde);
		memcpy (dot + nde, decimal->str, decimal->len);
	} else {
		while (nde > 0) {
			g_string_append_c (dst, '0');
			nde--;
		}
	}
	exponent -= nde;

	g_string_append_printf (dst, "E%+d", exponent);
}



#define INSERT_MINUS(pos) do {							\
	if (unicode_minus)							\
		g_string_insert_len (dst, (pos), unicode_minus_utf8, 3);	\
	else									\
		g_string_insert_c (dst, (pos), '-');				\
} while (0)


static GOFormatNumberError
SUFFIX(go_format_execute) (PangoLayout *layout, GString *dst,
			   const GOFormatMeasure measure,
			   const GOFontMetrics *metrics,
			   const guchar *prg,
			   int col_width,
			   DOUBLE val, const char *sval,
			   GODateConventions const *date_conv,
			   gboolean unicode_minus)
{
	GOFormatNumberError res = GO_FORMAT_NUMBER_OK;
	DOUBLE valsecs = 0;
	GDateYear year = 0;
	GDateMonth month = 0;
	GDateDay day = 0;
	GDateWeekday weekday = 0;
	DOUBLE hour = 0, minute = 0, second = 0;
	gboolean ispm = FALSE;
	char fsecond[PREFIX(DIG) + 10];
	const char *date_dec_ptr = NULL;
	GString *numtxt = NULL;
	size_t dotpos = 0;
	size_t numi = 0;
	int numpos = -1;
	int generalpos = -1;
	const GString *decimal = go_locale_get_decimal ();
	const GString *comma = go_locale_get_thousand ();
	gboolean thousands = FALSE;
	gboolean digit_count = 0;
	int exponent = 0;
	struct {
		DOUBLE w, n, d;
		gsize nominator_start, denominator_start;
	} fraction;
#ifdef ALLOW_EE_MARKUP
	int mantissa_start = -1;
	int special_mantissa = INT_MAX;
#endif
	char *oldlocale = NULL;

	memset (&fraction, 0, sizeof (fraction));

	while (1) {
		GOFormatOp op = *prg++;

		switch (op) {
		case OP_END:
			if (layout)
				pango_layout_set_text (layout, dst->str, -1);
			if (numtxt)
				g_string_free (numtxt, TRUE);
			if (oldlocale) {
				go_setlocale (LC_ALL, oldlocale);
				g_free (oldlocale);
			}
			return res;

		case OP_CHAR: {
			const guchar *next = g_utf8_next_char (prg);
			g_string_insert_len (dst, numpos, prg, next - prg);
			prg = next;
			break;
		}

		case OP_CHAR_INVISIBLE: {
			const guchar *next = g_utf8_next_char (prg);
			/* This ignores actual width for now.  */
			g_string_insert_c (dst, numpos, ' ');
			prg = next;
			break;
		}

		case OP_CHAR_REPEAT: {
			g_string_insert_c (dst, numpos, REPEAT_CHAR_MARKER);
			break;
		}

		case OP_STRING: {
			size_t len = strlen (prg);
			g_string_insert_len (dst, numpos, prg, len);
			prg += len + 1;
			break;
		}

		case OP_FILL: {
			gssize fill_pos = -1;
			gsize i = 0;
			gunichar fill_char = g_utf8_get_char (prg);

			prg = g_utf8_next_char (prg);

			while (i < dst->len) {
				if (dst->str[i] == REPEAT_CHAR_MARKER) {
					fill_pos = i;
					g_string_erase (dst, i, 1);
				} else
					i++;
			}

			if (fill_pos >= 0 && col_width >= 0) {
				fill_with_char (dst, layout, fill_pos,
						fill_char,
						measure, col_width);
				if (fill_char == ' ' && metrics->thin_space)
					fill_with_char (dst, layout, fill_pos,
							metrics->thin_space,
							measure, col_width);
			}
			break;
		}

		case OP_LOCALE: {
			GOFormatLocale locale;
			const char *lang;
			memcpy (&locale, prg, sizeof (locale));
			prg += sizeof (locale);
			lang = (const char *)prg;
			prg += strlen (lang) + 1;

			oldlocale = g_strdup (go_setlocale (LC_ALL, NULL));
			/* Setting LC_TIME should be enough, but glib gets
			   confused over character sets.  */
			go_setlocale (LC_TIME, lang);
			go_setlocale (LC_CTYPE, lang);
			break;
		}

		case OP_DATE_ROUND: {
			int date_decimals = *prg++;
			gboolean seen_elapsed = *prg++;
			DOUBLE unit = SUFFIX(go_pow10)(date_decimals);
#ifdef ALLOW_NEGATIVE_TIMES
			gboolean isneg = (val < 0);
#else
			gboolean isneg = FALSE;
#endif

			valsecs = SUFFIX(floor)(SUFFIX(go_add_epsilon)(SUFFIX(fabs)(val)) * (unit * 86400) + 0.5);
			if (date_decimals) {
				DOUBLE vs = (seen_elapsed || !isneg) ? valsecs : 0 - valsecs;
				DOUBLE f = SUFFIX(fmod) (vs, unit);
#ifdef ALLOW_NEGATIVE_TIMES
				if (f < 0)
					f += unit;
#endif
				sprintf (fsecond, "%0*.0" FORMAT_f,
					 date_decimals, f);
				valsecs = SUFFIX(floor)(valsecs / unit);
			}
			if (isneg)
				valsecs = 0 - valsecs;
			break;
		}

		case OP_DATE_SPLIT: {
			GDate date;
			go_date_serial_to_g (&date,
					      (int)SUFFIX(floor)(valsecs / 86400),
					      date_conv);
			if (!g_date_valid (&date)) {
				res = GO_FORMAT_NUMBER_DATE_ERROR;
				g_date_set_dmy (&date, 1, 1, 1900);
			}
			year = g_date_get_year (&date);
			month = g_date_get_month (&date);
			day = g_date_get_day (&date);
			weekday = g_date_get_weekday (&date);
			if (year > 9999)
				res = GO_FORMAT_NUMBER_DATE_ERROR;
			break;
		}

		case OP_DATE_YEAR:
			append_i (dst, year);
			break;

		case OP_DATE_YEAR_2:
			append_i2 (dst, year % 100);
			break;

		case OP_DATE_YEAR_THAI:
			append_i (dst, year + 543);
			break;

		case OP_DATE_YEAR_THAI_2:
			append_i2 (dst, (year + 543) % 100);
			break;

		case OP_DATE_MONTH:
			append_i (dst, month);
			break;

		case OP_DATE_MONTH_2:
			append_i2 (dst, month);
			break;

		case OP_DATE_MONTH_NAME: {
			char *s = go_date_month_name (month, FALSE);
			g_string_append (dst, s);
			g_free (s);
			break;
		}

		case OP_DATE_MONTH_NAME_1: {
			char *s = go_date_month_name (month, TRUE);
			g_string_append_c (dst, *s);
			g_free (s);
			break;
		}

		case OP_DATE_MONTH_NAME_3: {
			char *s = go_date_month_name (month, TRUE);
			g_string_append (dst, s);
			g_free (s);
			break;
		}

		case OP_DATE_DAY:
			append_i (dst, day);
			break;

		case OP_DATE_DAY_2:
			append_i2 (dst, day);
			break;

		case OP_DATE_WEEKDAY: {
			char *s = go_date_weekday_name (weekday, FALSE);
			g_string_append (dst, s);
			g_free (s);
			break;
		}

		case OP_DATE_WEEKDAY_3: {
			char *s = go_date_weekday_name (weekday, TRUE);
			g_string_append (dst, s);
			g_free (s);
			break;
		}

		case OP_TIME_SPLIT_12:
		case OP_TIME_SPLIT_24: {
			int secs = (int)SUFFIX(fmod)(valsecs, 86400);
#ifdef ALLOW_NEGATIVE_TIMES
			if (secs < 0)
				secs += 86400;
#endif
			hour = secs / 3600;
			minute = (secs / 60) % 60;
			second = secs % 60;
			if (op == OP_TIME_SPLIT_12) {
				ispm = (hour >= 12);
				if (ispm) hour -= 12;
				if (hour == 0) hour = 12;
			}
			break;
		}

		case OP_TIME_SPLIT_ELAPSED_HOUR:
		case OP_TIME_SPLIT_ELAPSED_MINUTE:
		case OP_TIME_SPLIT_ELAPSED_SECOND: {
			DOUBLE s = SUFFIX(fabs)(valsecs);

			if (op == OP_TIME_SPLIT_ELAPSED_SECOND)
				second = s;
			else {
				second = SUFFIX(fmod)(s, 60);
				s = SUFFIX(floor)(s / 60);
				if (op == OP_TIME_SPLIT_ELAPSED_MINUTE)
					minute = s;
				else {
					minute = SUFFIX(fmod)(s, 60);
					s = SUFFIX(floor)(s / 60);
					hour = s;
				}
			}
			break;
		}

		case OP_TIME_HOUR_2:
			if (hour < 100) {
				append_i2 (dst, (int)hour);
				break;
			}
			/* Fall through */
		case OP_TIME_HOUR:
			g_string_append_printf (dst, "%.0" FORMAT_f, hour);
			break;

		case OP_TIME_HOUR_N: {
			int n = *prg++;
#ifdef ALLOW_NEGATIVE_TIMES
			if (valsecs < 0)
				INSERT_MINUS(-1);
#endif
			g_string_append_printf (dst, "%0*.0" FORMAT_f, n, hour);
			break;
		}

		case OP_TIME_AMPM:
			g_string_append (dst, ispm ? "PM" : "AM");
			break;

		case OP_TIME_AP: {
			char ca = *prg++;
			char cp = *prg++;
			g_string_append_c (dst, ispm ? cp : ca);
			break;
		}

		case OP_TIME_MINUTE_2:
			if (minute < 100) {
				append_i2 (dst, (int)minute);
				break;
			}
			/* Fall through */
		case OP_TIME_MINUTE:
			g_string_append_printf (dst, "%.0" FORMAT_f, minute);
			break;

		case OP_TIME_MINUTE_N: {
			int n = *prg++;
#ifdef ALLOW_NEGATIVE_TIMES
			if (valsecs < 0)
				INSERT_MINUS(-1);
#endif
			g_string_append_printf (dst, "%0*.0" FORMAT_f, n, minute);
			break;
		}

		case OP_TIME_SECOND_2:
			if (second < 100) {
				append_i2 (dst, (int)second);
				break;
			}
			/* Fall through */
		case OP_TIME_SECOND:
			g_string_append_printf (dst, "%.0" FORMAT_f, second);
			break;

		case OP_TIME_SECOND_N: {
			int n = *prg++;
#ifdef ALLOW_NEGATIVE_TIMES
			if (valsecs < 0)
				INSERT_MINUS(-1);
#endif
			g_string_append_printf (dst, "%0*.0" FORMAT_f, n, second);
			break;
		}

		case OP_TIME_SECOND_DECIMAL_START:
			/* Reset to start of decimal string.  */
			date_dec_ptr = fsecond;
			go_string_append_gstring (dst, decimal);
			break;

		case OP_TIME_SECOND_DECIMAL_DIGIT:
			g_string_append_c (dst, *date_dec_ptr++);
			break;

		case OP_NUM_SCALE: {
			int n = *(const signed char *)prg;
			prg++;
			if (n >= 0)
				val *= SUFFIX(go_pow10) (n);
			else
				val /= SUFFIX(go_pow10) (-n);
			break;
		}

		case OP_NUM_PRINTF_E: {
			int n = *prg++;
			int wd = *prg++;
			char *dot;
			const char *epos;

			if (!numtxt)
				numtxt = g_string_sized_new (100);

			SUFFIX(printf_engineering) (numtxt, val, n, wd);

			epos = strchr (numtxt->str, 'E');
			if (epos) {
				exponent = atoi (epos + 1);
				g_string_truncate (numtxt, epos - numtxt->str);
			}

			dot = strstr (numtxt->str, decimal->str);
			if (dot) {
				size_t i = numtxt->len;
				dotpos = dot - numtxt->str;
				while (numtxt->str[i - 1] == '0')
					i--;
				/* Kill zeroes in "xxx.xxx000"  */
				g_string_truncate (numtxt, i);
			} else {
				dotpos = numtxt->len;
			}

#ifdef ALLOW_EE_MARKUP
			if (!dot || numtxt->str[dotpos + decimal->len] == 0) {
				if (dotpos == 2 &&
				    numtxt->str[0] == '-' &&
				    numtxt->str[1] == '1')
					special_mantissa = -1;
				else if (dotpos == 1 && numtxt->str[0] == '0')
					special_mantissa = 0;
				else if (dotpos == 1 && numtxt->str[0] == '1')
					special_mantissa = +1;
			}
#endif

			break;
		}

		case OP_NUM_PRINTF_F: {
			int n = *prg++;
			const char *dot;
			if (!numtxt)
				numtxt = g_string_sized_new (100);
			g_string_printf (numtxt, "%.*" FORMAT_f, n, val);
			dot = strstr (numtxt->str, decimal->str);
			if (dot) {
				size_t i = numtxt->len;
				dotpos = dot - numtxt->str;
				while (numtxt->str[i - 1] == '0')
					i--;
				/* Kill zeroes in "xxx.xxx000"  */
				g_string_truncate (numtxt, i);

				if (numtxt->str[0] == '-' &&
				    numtxt->str[1] == '0' &&
				    dotpos == 2 &&
				    numtxt->len == dotpos + decimal->len) {
					g_string_erase (numtxt, 0, 1);
					dotpos--;
				}
			} else {
				if (numtxt->str[0] == '-' &&
				    numtxt->str[1] == '0' &&
				    numtxt->len == 2)
					g_string_erase (numtxt, 0, 1);
				dotpos = numtxt->len;
			}
			break;
		}

		case OP_NUM_ENABLE_THOUSANDS:
			thousands = TRUE;
			break;

		case OP_NUM_DISABLE_THOUSANDS:
			thousands = FALSE;
			break;

		case OP_NUM_SIGN:
			if (numtxt->str[0] == '-') {
				g_string_erase (numtxt, 0, 1);
				dotpos--;
				INSERT_MINUS(numpos);
			}
			break;

		case OP_NUM_VAL_SIGN:
			if (val < 0)
				INSERT_MINUS(numpos);
			break;

		case OP_NUM_MOVETO_ONES: {
			numi = dotpos;
			/* Ignore the zero in "0.xxx" */
			if (numi == 1 && numtxt->str[0] == '0' && numtxt->str[dotpos] != 0)
				numi--;
			numpos = dst->len;
			break;
		}

		case OP_NUM_MOVETO_DECIMALS:
			if (dotpos == numtxt->len)
				numi = dotpos;
			else
				numi = dotpos + decimal->len;
			break;

		case OP_NUM_REST_WHOLE:
			while (numi > 0) {
				char c = numtxt->str[--numi];
				digit_count++;
				if (thousands && digit_count > 3 &&
				    digit_count % 3 == 1)
					g_string_insert_len (dst, numpos,
							     comma->str,
							     comma->len);
				g_string_insert_c (dst, numpos, c);
			}
			break;

		case OP_NUM_APPEND_MODE:
			numpos = -1;
			break;

		case OP_NUM_DECIMAL_POINT:
			go_string_append_gstring (dst, decimal);
			break;

		case OP_NUM_DIGIT_1: {
			char fc = *prg++;
			char c;
			if (numi == 0) {
				if (fc == '0')
					c = '0';
				else if (fc == '?')
					c = ' ';
				else
					break;
			} else {
				c = numtxt->str[numi - 1];
				numi--;
			}
			digit_count++;
			if (thousands && digit_count > 3 &&
			    digit_count % 3 == 1) {
				g_string_insert_len (dst, numpos,
						     comma->str, comma->len);
			}
			g_string_insert_c (dst, numpos, c);
			break;
		}

		case OP_NUM_DECIMAL_1: {
			char fc = *prg++;
			char c;
			if (numi == numtxt->len) {
				if (fc == '0')
					c = '0';
				else if (fc == '?')
					c = ' ';
				else
					break;
			} else {
				c = numtxt->str[numi];
				numi++;
			}
			g_string_append_c (dst, c);
			break;
		}

		case OP_NUM_DIGIT_1_0: {
			char fc = *prg++;
			if (fc == '0')
				g_string_insert_c (dst, numpos, '0');
			else if (fc == '?')
				g_string_insert_c (dst, numpos, ' ');
			break;
		}

		case OP_NUM_DENUM_DIGIT_Q:
			if (numi == 0)
				g_string_append_c (dst, ' ');
			else {
				char c = numtxt->str[numi - 1];
				numi--;
				g_string_insert_c (dst, numpos, c);
			}
			break;

		case OP_NUM_EXPONENT_SIGN: {
			gboolean forced = (*prg++ != 0);
			if (exponent >= 0) {
				if (forced)
					g_string_insert_c (dst, numpos, '+');
			} else
				INSERT_MINUS(numpos);
			break;
		}

		case OP_NUM_VAL_EXPONENT:
			val = SUFFIX (fabs) (exponent);
			break;

		case OP_NUM_EXPONENT_1:
			exponent = 1;
			break;

#ifdef ALLOW_EE_MARKUP
		case OP_NUM_MARK_MANTISSA:
			mantissa_start = dst->len;
			break;

		case OP_NUM_SIMPLIFY_MANTISSA:
			if (special_mantissa != INT_MAX)
				g_string_truncate (dst, mantissa_start);
			break;

		case OP_NUM_SIMPLIFY_EXPONENT:
			if (special_mantissa == 0) {
				g_string_truncate (dst, mantissa_start);
				g_string_append_c (dst, '0');
			}
			break;
#endif

		case OP_NUM_FRACTION: {
			gboolean wp = *prg++;
			gboolean explicit_denom = *prg++;
			DOUBLE aval = SUFFIX(go_add_epsilon) (SUFFIX(fabs)(val));

			fraction.w = SUFFIX(floor) (aval);
			aval -= fraction.w;

			if (explicit_denom) {
				double plaind; /* Plain double */
				memcpy (&plaind, prg, sizeof (plaind));
				prg += sizeof (plaind);

				fraction.d = plaind;
				fraction.n = SUFFIX(floor) (0.5 + aval * fraction.d);
			} else {
				int digits = *prg++;
				int ni, di;
				go_continued_fraction (aval, SUFFIX(go_pow10) (digits), &ni, &di);
				fraction.n = ni;
				fraction.d = di;
			}

			if (wp && fraction.n == fraction.d) {
				fraction.w += 1;
				fraction.n = 0;
			}
			if (!wp)
				fraction.n += fraction.d * fraction.w;

			if (val < 0) {
				if (wp)
					fraction.w = 0 - fraction.w;
				else
					fraction.n = 0 - fraction.n;
			}

			break;
		}

#ifdef ALLOW_PI_SLASH
		case OP_NUM_FRACTION_SCALE_PI:
			/* FIXME: not long-double safe.  */
			val /= G_PI;
			break;
#endif

		case OP_NUM_FRACTION_WHOLE:
			val = fraction.w;
			break;

		case OP_NUM_FRACTION_NOMINATOR:
			fraction.nominator_start = dst->len;
			val = fraction.n;
			break;

		case OP_NUM_FRACTION_DENOMINATOR:
			fraction.denominator_start = dst->len;
			val = fraction.d;
			break;

		case OP_NUM_FRACTION_BLANK:
			if (fraction.n == 0) {
				/* Replace all added characters by spaces.  */
				gsize chars = g_utf8_strlen (dst->str + fraction.nominator_start, -1);
				memset (dst->str + fraction.nominator_start, ' ', chars);
				g_string_truncate (dst, fraction.nominator_start + chars);
			}
			break;

#ifdef ALLOW_PI_SLASH
		case OP_NUM_FRACTION_SIMPLIFY_PI:
			if (fraction.n != 0 && fraction.d == 1) {
				/* Remove "/1".  */
				g_string_truncate (dst,
						   fraction.denominator_start - 1);
			}

			if (fraction.n == 0) {
				/* Replace the whole thing by "0".  */
				g_string_truncate (dst,
						   fraction.nominator_start);
				g_string_append_c (dst, '0');
			} else if (fraction.n == 1 || fraction.n == -1) {
				/* Remove "1".  */
				gsize p = fraction.nominator_start;
				while (dst->str[p] && dst->str[p] != '1')
					p++;
				if (dst->str[p])
					g_string_erase (dst, p, 1);
			}
			break;
#endif

		case OP_NUM_GENERAL_MARK:
			generalpos = dst->len;
			break;

		case OP_NUM_GENERAL_DO: {
			gboolean is_empty = (dst->len == 0);
			GString *gen;
			int w = col_width;
			if (is_empty) {
				gen = dst;
			} else {
				if (w >= 0) {
					w -= measure (dst, layout);
					if (w < 0) w = 0;
				}
				gen = g_string_new (NULL);
			}
			SUFFIX(go_render_general)
				(layout, gen, measure, metrics,
				 val, w, unicode_minus);
			if (!is_empty) {
				g_string_insert_len (dst, generalpos,
						     gen->str, gen->len);
				g_string_free (gen, TRUE);
			}
			break;
		}

		case OP_STR_APPEND_SVAL:
			g_string_append (dst, sval);
			break;
		}
	}
}

/*********************************************************************/

#ifdef DEFINE_COMMON
int
go_format_measure_zero (G_GNUC_UNUSED const GString *str,
			G_GNUC_UNUSED PangoLayout *layout)
{
	return 0;
}
#endif

#ifdef DEFINE_COMMON
int
go_format_measure_pango (G_GNUC_UNUSED const GString *str,
			 PangoLayout *layout)
{
	int w;
	pango_layout_get_size (layout, &w, NULL);
#ifdef DEBUG_GENERAL
	g_print ("[%s] --> %d\n", str->str, w);
#endif
	return w;
}
#endif

#ifdef DEFINE_COMMON
int
go_format_measure_strlen (const GString *str,
			  G_GNUC_UNUSED PangoLayout *layout)
{
	return g_utf8_strlen (str->str, -1);
}
#endif

/*********************************************************************/

#ifdef DEFINE_COMMON
static gboolean
convert_minus (GString *str, size_t i)
{
	if (str->str[i] != '-')
		return FALSE;

	str->str[i] = 0xe2;
	g_string_insert_len (str, i + 1, "\x88\x92", 2);
	return TRUE;
}
#endif

#define HANDLE_MINUS(i) do { if (unicode_minus) convert_minus (str, (i)); } while (0)

/*
 * go_format_general:
 * @layout: Optional PangoLayout, probably preseeded with font attribute.
 * @str: a GString to store (not append!) the resulting string in.
 * @measure: Function to measure width of string/layout.
 * @metrics: Font metrics corresponding to @mesaure.
 * @val: floating-point value.  Must be finite.
 * @col_width: intended max width of layout in pango units.  -1 means
 *             no restriction.
 * @unicode_minus: Use unicode minuses, not hyphens.
 *
 * Render a floating-point value into @layout in such a way that the
 * layouting width does not needlessly exceed @col_width.  Optionally
 * use unicode minus instead of hyphen.
 */
void
SUFFIX(go_render_general) (PangoLayout *layout, GString *str,
			   GOFormatMeasure measure,
			   const GOFontMetrics *metrics,
			   DOUBLE val,
			   int col_width,
			   gboolean unicode_minus)
{
	DOUBLE aval, l10;
	int prec, safety, digs, maxdigits;
	size_t epos;
	gboolean rounds_to_0;
	int sign_width;

	if (col_width == -1) {
		measure = go_format_measure_zero;
		maxdigits = PREFIX(DIG);
		col_width = INT_MAX;
		sign_width = 0;
	} else {
		maxdigits = MIN (PREFIX(DIG), col_width / metrics->min_digit_width);
		sign_width = unicode_minus
			? metrics->minus_width
			: metrics->hyphen_width;
	}

#ifdef DEBUG_GENERAL
	g_print ("Rendering %" FORMAT_G " to width %d (<=%d digits)\n",
		 val, col_width, maxdigits);
#endif
	if (val == 0)
		goto zero;

	aval = SUFFIX(fabs) (val);
	if (aval >= SUFFIX(1e15) || aval < SUFFIX(1e-4))
		goto e_notation;
	l10 = SUFFIX(log10) (aval);

	/* Number of digits in [aval].  */
	digs = (aval >= 1 ? 1 + (int)l10 : 1);

	/* Check if there is room for the whole part, including sign.  */
	safety = metrics->avg_digit_width / 2;

	if (digs * metrics->min_digit_width > col_width) {
#ifdef DEBUG_GENERAL
		g_print ("No room for whole part.\n");
#endif
		goto e_notation;
	} else if (digs * metrics->max_digit_width + safety <
		   col_width - (val > 0 ? 0 : sign_width)) {
#ifdef DEBUG_GENERAL
		g_print ("Room for whole part.\n");
#endif
		if (val == SUFFIX(floor) (val) || digs == maxdigits) {
			g_string_printf (str, "%.0" FORMAT_f, val);
			HANDLE_MINUS (0);
			SETUP_LAYOUT;
			return;
		}
	} else {
		int w;
#ifdef DEBUG_GENERAL
		g_print ("Maybe room for whole part.\n");
#endif

		g_string_printf (str, "%.0" FORMAT_f, val);
		HANDLE_MINUS (0);
		SETUP_LAYOUT;
		w = measure (str, layout);
		if (w > col_width)
			goto e_notation;

		if (val == SUFFIX(floor) (val) || digs == maxdigits)
			return;
	}

	prec = maxdigits - digs;
	g_string_printf (str, "%.*" FORMAT_f, prec, val);
	HANDLE_MINUS (0);
	while (str->str[str->len - 1] == '0') {
		g_string_truncate (str, str->len - 1);
		prec--;
	}
	if (prec == 0) {
		/* We got "xxxxxx.000" and dropped the zeroes.  */
		const char *dot = g_utf8_prev_char (str->str + str->len);
		g_string_truncate (str, dot - str->str);
		SETUP_LAYOUT;
		return;
	}

	while (prec > 0) {
		int w;

		SETUP_LAYOUT;
		w = measure (str, layout);
		if (w <= col_width)
			return;

		prec--;
		g_string_printf (str, "%.*" FORMAT_f, prec, val);
		HANDLE_MINUS (0);
	}

	SETUP_LAYOUT;
	return;

 e_notation:
	rounds_to_0 = (aval < 0.5);
	prec = (col_width -
		(val > 0 ? 0 : sign_width) -
		(aval < 1 ? sign_width : metrics->plus_width) -
		metrics->E_width) / metrics->min_digit_width - 3;
	if (prec <= 0) {
#ifdef DEBUG_GENERAL
		if (prec == 0)
			g_print ("Maybe room for E notation with no decimals.\n");
		else
			g_print ("No room for E notation.\n");
#endif
		/* Certainly too narrow for precision.  */
		if (prec == 0 || !rounds_to_0) {
			int w;

			g_string_printf (str, "%.0" FORMAT_E, val);
			HANDLE_MINUS (0);
			epos = strchr (str->str, 'E') - str->str;
			HANDLE_MINUS (epos + 1);
			SETUP_LAYOUT;
			if (!rounds_to_0)
				return;

			w = measure (str, layout);
			if (w <= col_width)
				return;
		}

		goto zero;
	}
	prec = MIN (prec, PREFIX(DIG) - 1);
	g_string_printf (str, "%.*" FORMAT_E, prec, val);
	epos = strchr (str->str, 'E') - str->str;
	digs = 0;
	while (str->str[epos - 1 - digs] == '0')
		digs++;
	if (digs) {
		epos -= digs;
		g_string_erase (str, epos, digs);
		prec -= digs;
		if (prec == 0) {
			int dot = 1 + (str->str[0] == '-');
			g_string_erase (str, dot, epos - dot);
		}
	}

	while (1) {
		int w;

		HANDLE_MINUS (0);
		epos = strchr (str->str + prec + 1, 'E') - str->str;
		HANDLE_MINUS (epos + 1);
		SETUP_LAYOUT;
		w = measure (str, layout);
		if (w <= col_width)
			return;

		if (prec > 2 && w - metrics->max_digit_width > col_width)
			prec -= 2;
		else {
			prec--;
			if (prec < 0)
				break;
		}
		g_string_printf (str, "%.*" FORMAT_E, prec, val);
	}

	if (rounds_to_0)
		goto zero;

	SETUP_LAYOUT;
	return;

 zero:
#ifdef DEBUG_GENERAL
	g_print ("Zero.\n");
#endif
	g_string_assign (str, "0");
	SETUP_LAYOUT;
	return;
}


GOFormatNumberError
SUFFIX(go_format_value_gstring) (PangoLayout *layout, GString *str,
				 const GOFormatMeasure measure,
				 const GOFontMetrics *metrics,
				 GOFormat const *fmt,
				 DOUBLE val, char type, const char *sval,
				 GOColor *go_color,
				 int col_width,
				 GODateConventions const *date_conv,
				 gboolean unicode_minus)
{
	gboolean inhibit = FALSE;

	g_return_val_if_fail (type == 'F' || sval != NULL,
			      (GOFormatNumberError)-1);

	g_string_truncate (str, 0);

	if (fmt)
		fmt = SUFFIX(go_format_specialize) (fmt, val, type, &inhibit);
	if (!fmt)
		fmt = go_format_general ();

	if (go_color)
		*go_color = fmt->color;

	if (type == 'F') {
		switch (fmt->typ) {
		case GO_FMT_TEXT:
			if (inhibit)
				val = SUFFIX(fabs)(val);
			SUFFIX(go_render_general)
				(layout, str, measure, metrics,
				 val,
				 col_width, unicode_minus);
			return GO_FORMAT_NUMBER_OK;

		case GO_FMT_NUMBER:
			if (val < 0) {
#ifndef ALLOW_NEGATIVE_TIMES
				if (fmt->u.number.has_date ||
				    fmt->u.number.has_time)
					return GO_FORMAT_NUMBER_DATE_ERROR;
#endif
				if (inhibit)
					val = SUFFIX(fabs)(val);
			}
#ifdef DEBUG_PROGRAMS
			g_printerr ("Executing %s\n", fmt->format);
			go_format_dump_program (fmt->u.number.program);
#endif
			return SUFFIX(go_format_execute)
				(layout, str,
				 measure, metrics,
				 fmt->u.number.program,
				 col_width,
				 val, sval, date_conv,
				 unicode_minus);

		case GO_FMT_EMPTY:
			SETUP_LAYOUT;
			return GO_FORMAT_NUMBER_OK;

		default:
		case GO_FMT_INVALID:
		case GO_FMT_MARKUP:
		case GO_FMT_COND:
			SETUP_LAYOUT;
			return GO_FORMAT_NUMBER_INVALID_FORMAT;
		}
	} else {
		switch (fmt->typ) {
		case GO_FMT_TEXT:
			return SUFFIX(go_format_execute)
				(layout, str,
				 measure, metrics,
				 fmt->u.text.program,
				 col_width,
				 val, sval, date_conv,
				 unicode_minus);

		case GO_FMT_NUMBER:
			g_string_assign (str, sval);
			SETUP_LAYOUT;
			return GO_FORMAT_NUMBER_OK;

		case GO_FMT_EMPTY:
			SETUP_LAYOUT;
			return GO_FORMAT_NUMBER_OK;

		default:
		case GO_FMT_INVALID:
		case GO_FMT_MARKUP:
		case GO_FMT_COND:
			SETUP_LAYOUT;
			return GO_FORMAT_NUMBER_INVALID_FORMAT;
		}
	}
}

/**
 * go_format_value:
 * @fmt: a #GOFormat
 * @val: value to format
 *
 * Converts @val into a string using format specified by @fmt.
 *
 * returns: a newly allocated string containing formated value.
 **/

char *
SUFFIX(go_format_value) (GOFormat const *fmt, DOUBLE val)
{
	GString *res = g_string_sized_new (20);
	GOFormatNumberError err = SUFFIX(go_format_value_gstring)
		(NULL, res,
		 go_format_measure_strlen,
		 go_font_metrics_unit,
		 fmt,
		 val, 'F', NULL,
		 NULL,
		 -1, NULL, FALSE);
	if (err) {
		/* Invalid number for format.  */
		g_string_assign (res, "#####");
	}
	return g_string_free (res, FALSE);
}



#ifdef DEFINE_COMMON
void
_go_number_format_init (void)
{
	style_format_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
		NULL, (GDestroyNotify) go_format_unref);

	beyond_precision = go_pow10 (DBL_DIG) + 1;
#ifdef GOFFICE_WITH_LONG_DOUBLE
	beyond_precisionl = go_pow10l (LDBL_DIG) + 1;
#endif
}
#endif

#ifdef DEFINE_COMMON
static void
cb_format_leak (gpointer key, gpointer value, gpointer user_data)
{
	GOFormat const *gf = value;
	if (gf->ref_count != 1)
		g_printerr ("Leaking GOFormat at %p [%s].\n", gf, gf->format);
}
#endif

#ifdef DEFINE_COMMON
void
_go_number_format_shutdown (void)
{
	GHashTable *tmp;

	go_format_unref (default_percentage_fmt);
	default_percentage_fmt = NULL;

	go_format_unref (default_money_fmt);
	default_money_fmt = NULL;

	go_format_unref (default_accounting_fmt);
	default_accounting_fmt = NULL;

	go_format_unref (default_date_fmt);
	default_date_fmt = NULL;

	go_format_unref (default_time_fmt);
	default_time_fmt = NULL;

	go_format_unref (default_date_time_fmt);
	default_date_time_fmt = NULL;

	go_format_unref (default_general_fmt);
	default_general_fmt = NULL;

	go_format_unref (default_empty_fmt);
	default_empty_fmt = NULL;

	tmp = style_format_hash;
	style_format_hash = NULL;
	g_hash_table_foreach (tmp, cb_format_leak, NULL);
	g_hash_table_destroy (tmp);
}
#endif

/****************************************************************************/

#ifdef DEFINE_COMMON

#undef DEBUG_LOCALIZATION

/**
 * go_format_str_localize:
 * @str : A *valid* format string
 *
 * Localizes the given format string, i.e., changes decimal dots to the
 * locale's notion of that and performs other such transformations.
 *
 * Returns: a localized format string, or NULL if the format was not valid.
 **/
char *
go_format_str_localize (char const *str)
{
	GString *res;
	GString const *comma = go_locale_get_thousand ();
	GString const *decimal = go_locale_get_decimal ();

	g_return_val_if_fail (str != NULL, NULL);

#ifdef DEBUG_LOCALIZATION
	g_printerr ("Localize in : [%s]\n", str);
#endif

	res = g_string_new (NULL);
	while (1) {
		GOFormatParseState state;
		const GOFormatParseState *pstate = &state;
		const char *tail;
		unsigned tno;

		memset (&state, 0, sizeof (state));
		tail = go_format_preparse (str, &state, TRUE, FALSE);
		if (!tail) {
			g_array_free (state.tokens, TRUE);
			g_string_free (res, TRUE);
			return NULL;
		}

		for (tno = 0; tno < state.tokens->len; tno++) {
			const GOFormatParseItem *ti = &GET_TOKEN(tno);
			const char *tstr = ti->tstr;
			const char *end = (tno + 1 == state.tokens->len)
				? tail
				: GET_TOKEN(tno + 1).tstr;

			switch (ti->token) {
			case TOK_ERROR:
				g_array_free (state.tokens, TRUE);
				g_string_free (res, TRUE);
				return NULL;
			case TOK_GENERAL:
				g_string_append (res, _("General"));
				break;
			case TOK_CONDITION:
				while (tstr != end) {
					if (*tstr == '.') {
						go_string_append_gstring (res, decimal);
						tstr++;
					} else if (strncmp (tstr, decimal->str, decimal->len) == 0) {
						/* 1000,00 becomes 1000\,00 */
						g_string_append_c (res, '\\');
						g_string_append_c (res, *tstr++);
					} else
						g_string_append_c (res, *tstr++);
				}
				break;

			case TOK_COLOR:
				g_string_append_c (res, '[');
				if (state.color_named)
					g_string_append (res, _(format_colors[state.color_n].name));
				else
					g_string_append_printf (res, "Color%d", state.color_n);
				g_string_append_c (res, ']');
				break;

			case TOK_DECIMAL:
				if (state.is_number ||
				    (state.is_date && *end == '0'))
					go_string_append_gstring (res, decimal);
				else
					goto regular;
				break;

			case TOK_THOUSAND:
				if (!state.is_number)
					goto regular;
				go_string_append_gstring (res, comma);
				break;

			default:
				if (strncmp (tstr, decimal->str, decimal->len) == 0 ||
				    (state.is_number &&
				     strncmp (tstr, comma->str, comma->len) == 0)) {
					/* In particular, neither "." nor ","  */
					g_string_append_c (res, '\\');
				}

			regular:
				g_string_append_len (res, tstr, end - tstr);
			}
		}

		g_array_free (state.tokens, TRUE);

		str = tail;
		if (*str == 0)
			break;
		g_string_append_c (res, *str++);
	}

#ifdef DEBUG_LOCALIZATION
	g_printerr ("Localize out: [%s]\n", res->str);
#endif
	return g_string_free (res, FALSE);
}

/**
 * go_format_str_delocalize:
 * @str : A *valid* localized format string
 *
 * De-localizes the given format string, i.e., changes locale's decimal
 * separators to dots and performs other such transformations.
 *
 * Returns: a non-local format string, or NULL if the format was not valid.
 **/
char *
go_format_str_delocalize (char const *str)
{
	GString *res;
	GString const *comma = go_locale_get_thousand ();
	GString const *decimal = go_locale_get_decimal ();
	gboolean decimal_needs_quoting =
		strcmp (decimal->str, ".") == 0 ||
		strcmp (decimal->str, ",") == 0;
	gboolean comma_needs_quoting =
		strcmp (comma->str, ".") == 0 ||
		strcmp (comma->str, ",") == 0;

	g_return_val_if_fail (str != NULL, NULL);

#ifdef DEBUG_LOCALIZATION
	g_printerr ("Delocalize in : [%s]\n", str);
#endif

	res = g_string_new (NULL);
	while (1) {
		GOFormatParseState state;
		const GOFormatParseState *pstate = &state;
		const char *tail;
		unsigned tno;

		memset (&state, 0, sizeof (state));
		tail = go_format_preparse (str, &state, TRUE, TRUE);
		if (!tail) {
			g_array_free (state.tokens, TRUE);
			g_string_free (res, TRUE);
			return NULL;
		}

		for (tno = 0; tno < state.tokens->len; tno++) {
			const GOFormatParseItem *ti = &GET_TOKEN(tno);
			const char *tstr = ti->tstr;
			const char *end = (tno + 1 == state.tokens->len)
				? tail
				: GET_TOKEN(tno + 1).tstr;

			switch (ti->token) {
			case TOK_ERROR:
				g_array_free (state.tokens, TRUE);
				g_string_free (res, TRUE);
				return NULL;
			case TOK_GENERAL:
				g_string_append (res, "General");
				break;
			case TOK_CONDITION:
				while (tstr != end) {
					if (strncmp (tstr, decimal->str, decimal->len) == 0) {
						g_string_append_c (res, '.');
						tstr += decimal->len;
					} else if (*tstr == '.') {
						/* 1000.00 becomes 1000\.00 */
						g_string_append_c (res, '\\');
						g_string_append_c (res, *tstr++);
					} else
						g_string_append_c (res, *tstr++);
				}
				break;

			case TOK_COLOR:
				g_string_append_c (res, '[');
				if (state.color_named)
					g_string_append (res, format_colors[state.color_n].name);
				else
					g_string_append_printf (res, "Color%d", state.color_n);
				g_string_append_c (res, ']');
				break;

			case '\\':
				if ((strncmp (tstr + 1, decimal->str, decimal->len) == 0 && !decimal_needs_quoting) ||
				    (strncmp (str + 1, comma->str, comma->len) == 0 && !comma_needs_quoting))
					tstr++;
				g_string_append_len (res, tstr, str - tstr);
				break;

			case TOK_DECIMAL:
				if (state.is_number ||
				    (state.is_date && *end == '0'))
					g_string_append_c (res, '.');
				else
					goto regular;
				break;

			case TOK_THOUSAND:
				if (!state.is_number)
					goto regular;
				g_string_append_c (res, ',');
				break;

			default:
				if (*tstr == '.' &&
				    (state.is_number || (state.is_date && *str == '0')))
					g_string_append_c (res, '\\');
				else if (*tstr == ',' && state.is_number)
					g_string_append_c (res, '\\');
				/* Fall through.  */

			regular:
				g_string_append_len (res, tstr, end - tstr);
			}
		}

		g_array_free (state.tokens, TRUE);

		str = tail;
		if (*str == 0)
			break;
		g_string_append_c (res, *str++);
	}

#ifdef DEBUG_LOCALIZATION
	g_printerr ("Delocalize out: [%s]\n", res->str);
#endif
	return g_string_free (res, FALSE);
}

static GOFormat *
make_frobbed_format (char *str, const GOFormat *fmt)
{
	GOFormat *res;

	if (strcmp (str, fmt->format) == 0)
		res = NULL;
	else {
#if 0
		g_printerr ("Frob: [%s] -> [%s]\n", fmt->format, str);
#endif
		res = go_format_new_from_XL (str);
		if (res->typ == GO_FMT_INVALID) {
			go_format_unref (res);
			res = NULL;
		}
	}

	g_free (str);
	return res;
}

/**
 * go_format_inc_precision :
 * @fmt : #GOFormat
 *
 * Increases the displayed precision for @fmt by one digit.
 *
 * Returns: NULL if the new format would not change things
 **/
GOFormat *
go_format_inc_precision (GOFormat const *fmt)
{
	GString *res = g_string_new (NULL);
	const char *str = fmt->format;
	gssize last_zero = -1;
	GOFormatDetails details;
	gboolean exact;

	go_format_get_details (fmt, &details, &exact);
	if (exact) {
		switch (details.family) {
		case GO_FORMAT_NUMBER:
		case GO_FORMAT_SCIENTIFIC:
		case GO_FORMAT_CURRENCY:
		case GO_FORMAT_ACCOUNTING:
		case GO_FORMAT_PERCENTAGE:
			if (details.num_decimals >= MAX_DECIMALS)
				return NULL;
			details.num_decimals++;
			go_format_generate_str (res, &details);
			return make_frobbed_format (g_string_free (res, FALSE),
						    fmt);
		case GO_FORMAT_GENERAL:
		case GO_FORMAT_TEXT:
			return NULL;
		default:
			break;
		}
	}

	/* Fall-back.  */
	while (1) {
		const char *tstr = str;
		GOFormatTokenType tt;
		int t = go_format_token (&str, &tt);

		switch (t) {
		case TOK_ERROR:
			g_string_free (res, TRUE);
			return NULL;

		case 0:
		case ';':
			g_string_append_len (res, tstr, str - tstr);
			if (last_zero >= 0)
				g_string_insert_len (res, last_zero + 1,
						     ".0", 2);
			last_zero = -1;
			if (t == ';')
				break;
			return make_frobbed_format (g_string_free (res, FALSE), fmt);

		case 's': case 'S':
			g_string_append_c (res, t);
			while (*str == 's' || *str == 'S')
				g_string_append_c (res, *str++);
			if (str[0] != '.')
				g_string_append_c (res, '.');
			else
				g_string_append_c (res, *str++);
			g_string_append_c (res, '0');
			last_zero = -2;
			break;

		case TOK_DECIMAL: {
			int n = 0;
			g_string_append_c (res, *tstr);
			while (*str == '0') {
				g_string_append_c (res, *str++);
				n++;
			}
			if (n < DBL_DIG)
				g_string_append_c (res, '0');
			last_zero = -2;
			break;
		}

		case 'E':
			if (last_zero != -2) {
				if (last_zero>=0)
					g_string_insert_len (res, last_zero + 1, ".0",2);
				else
					g_string_append (res, ".0");
				last_zero = -2;
			}
			g_string_append_c (res, t);
			break;

		case '0':
			if (last_zero != -2)
				last_zero = res->len;
			/* Fall through.  */

		default:
			g_string_append_len (res, tstr, str - tstr);
		}
	}
}

/**
 * go_format_dec_precision :
 * @fmt : #GOFormat
 *
 * Decreases the displayed precision for @fmt by one digit.
 *
 * Returns: NULL if the new format would not change things
 **/
GOFormat *
go_format_dec_precision (GOFormat const *fmt)
{
	GString *res = g_string_new (NULL);
	const char *str = fmt->format;
	GOFormatDetails details;
	gboolean exact;

	go_format_get_details (fmt, &details, &exact);
	if (exact) {
		switch (details.family) {
		case GO_FORMAT_NUMBER:
		case GO_FORMAT_SCIENTIFIC:
		case GO_FORMAT_CURRENCY:
		case GO_FORMAT_ACCOUNTING:
		case GO_FORMAT_PERCENTAGE:
			if (details.num_decimals <= 0)
				return NULL;
			details.num_decimals--;
			go_format_generate_str (res, &details);
			return make_frobbed_format (g_string_free (res, FALSE),
						    fmt);
		case GO_FORMAT_GENERAL:
		case GO_FORMAT_TEXT:
			return NULL;
		default:
			break;
		}
	}

	/* Fall-back.  */
	while (1) {
		const char *tstr = str;
		GOFormatTokenType tt;
		int t = go_format_token (&str, &tt);

		switch (t) {
		case TOK_ERROR:
			g_string_free (res, TRUE);
			return NULL;

		case 0:
			return make_frobbed_format (g_string_free (res, FALSE), fmt);

		case TOK_DECIMAL:
			if (str[0] == '0') {
				if (str[1] == '0')
					g_string_append_c (res, '.');
				str++;
				break;
			}
			/* Fall through */

		default:
			g_string_append_len (res, tstr, str - tstr);
		}
	}
}

GOFormat *
go_format_toggle_1000sep (GOFormat const *fmt)
{
	GString *res;
	const char *str;
	int commapos = -1;
	int numstart = -1;

	g_return_val_if_fail (fmt != NULL, NULL);

	res = g_string_new (NULL);
	str = fmt->format;

	/* No need to go via go_format_get_details since we can handle
	   all the standard formats with the code below.  */

	while (1) {
		const char *tstr = str;
		GOFormatTokenType tt;
		int t = go_format_token (&str, &tt);

		if (numstart == -1 && (tt & TT_STARTS_NUMBER))
			numstart = res->len;

		switch (t) {
		case TOK_ERROR:
			g_string_free (res, TRUE);
			return NULL;

		case 0:
		case ';':
			if (numstart == -1) {
				/* Nothing. */
			} else if (commapos == -1) {
				g_string_insert (res, numstart, "#,##");
			} else {
				int n = 0;

				g_string_erase (res, commapos, 1);
				commapos = -1;

				while (res->str[numstart + n] == '#')
					n++;

				if (n && res->str[numstart + n] == '0')
					g_string_erase (res, numstart, n);
			}

			if (t == 0)
				return make_frobbed_format
					(g_string_free (res, FALSE), fmt);

			numstart = -1;
			break;

		case TOK_THOUSAND:
			if (numstart != -1 && comma_is_thousands (tstr)) {
				if (commapos != -1)
					g_string_erase (res, commapos, 1);
				commapos = res->len;
			}
			break;

		case TOK_GENERAL:
			if (go_format_is_general (fmt)) {
				/* Format is just "General" and color etc. */
				g_string_append (res, "#,##0");
				continue;
			}
			break;

		default:
			break;
		}

		g_string_append_len (res, tstr, str - tstr);
	}
}
#endif

#ifdef DEFINE_COMMON
static gboolean
cb_attrs_as_string (PangoAttribute *a, GString *accum)
{
	PangoColor const *c;
	char buf[16];

	if (a->start_index >= a->end_index)
		return FALSE;

	switch (a->klass->type) {
	case PANGO_ATTR_FAMILY :
		g_string_append_printf (accum, "[family=%s",
			((PangoAttrString *)a)->value);
		break;
	case PANGO_ATTR_SIZE :
		g_string_append_printf (accum, "[size=%d",
			((PangoAttrInt *)a)->value);
		break;
	case PANGO_ATTR_RISE:
		g_string_append_printf (accum, "[rise=%d",
			((PangoAttrInt *)a)->value);
		break;
	case PANGO_ATTR_SCALE:
		g_string_append (accum, "[scale=");
		g_ascii_formatd (buf, sizeof (buf), "%.2f",
				((PangoAttrFloat *)a)->value);
		g_string_append (accum, buf);
		break;
	case PANGO_ATTR_STYLE :
		g_string_append_printf (accum, "[italic=%d",
			(((PangoAttrInt *)a)->value == PANGO_STYLE_ITALIC) ? 1 : 0);
		break;
	case PANGO_ATTR_WEIGHT :
		/* We are scaling the weight so that earlier versions that used only 0/1 */
		/* can still read the new formats and we can read the old ones. */
		g_string_append (accum, "[bold=");
		g_ascii_formatd (buf, sizeof (buf), "%.3f",
				(((PangoAttrInt *)a)->value - 399.)/300.);
		g_string_append (accum, buf);
		break;
	case PANGO_ATTR_STRIKETHROUGH :
		g_string_append_printf (accum, "[strikethrough=%d",
			((PangoAttrInt *)a)->value ? 1 : 0);
		break;
	case PANGO_ATTR_UNDERLINE :
		switch (((PangoAttrInt *)a)->value) {
		case PANGO_UNDERLINE_NONE :
			g_string_append (accum, "[underline=none");
			break;
		case PANGO_UNDERLINE_SINGLE :
			g_string_append (accum, "[underline=single");
			break;
		case PANGO_UNDERLINE_DOUBLE :
			g_string_append (accum, "[underline=double");
			break;
		case PANGO_UNDERLINE_LOW :
			g_string_append (accum, "[underline=low");
			break;
		case PANGO_UNDERLINE_ERROR :
			g_string_append (accum, "[underline=error");
			break;
		}
		break;

	case PANGO_ATTR_FOREGROUND :
		c = &((PangoAttrColor *)a)->color;
		g_string_append_printf (accum, "[color=%02xx%02xx%02x",
			((c->red & 0xff00) >> 8),
			((c->green & 0xff00) >> 8),
			((c->blue & 0xff00) >> 8));
		break;
	default :
		return FALSE; /* ignored */
	}
	g_string_append_printf (accum, ":%u:%u]", a->start_index, a->end_index);
	return FALSE;
}
#endif

#ifdef DEFINE_COMMON
static PangoAttrList *
go_format_parse_markup (char *str)
{
	PangoAttrList *attrs;
	PangoAttribute *a;
	char *closer, *val, *val_end;
	unsigned len;
	int r, g, b;

	g_return_val_if_fail (*str == '@', NULL);

	attrs = pango_attr_list_new ();
	for (str++ ; *str ; str = closer + 1) {
		if (*str != '[') goto bail;
		str++;

		val = strchr (str, '=');
		if (!val) goto bail;
		len = val - str;
		val++;

		val_end = strchr (val, ':');
		if (!val_end) goto bail;

		closer = strchr (val_end, ']');
		if (!closer) goto bail;
		*val_end = '\0';
		*closer = '\0';

		a = NULL;
		switch (len) {
		case 4:
			if (0 == strncmp (str, "size", 4))
				a = pango_attr_size_new (atoi (val));
			else if (0 == strncmp (str, "bold", 4))
				a = pango_attr_weight_new ((int)(g_ascii_strtod (val, NULL) * 300. + 399.));
			else if (0 == strncmp (str, "rise", 4))
				a = pango_attr_rise_new (atoi (val));
			break;

		case 5:
			if (0 == strncmp (str, "color", 5) &&
			    3 == sscanf (val, "%02xx%02xx%02x", &r, &g, &b))
				a = pango_attr_foreground_new ((r << 8) | r, (g << 8) | g, (b << 8) | b);
			else if (0 == strncmp (str, "scale", 5))
				a = pango_attr_scale_new (g_ascii_strtod (val, NULL));
			break;

		case 6:
			if (0 == strncmp (str, "family", 6))
				a = pango_attr_family_new (val);
			else if (0 == strncmp (str, "italic", 6))
				a = pango_attr_style_new (atoi (val) ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
			break;

		case 9:
			if (0 == strncmp (str, "underline", 9)) {
				if (0 == strcmp (val, "none"))
					a = pango_attr_underline_new (PANGO_UNDERLINE_NONE);
				else if (0 == strcmp (val, "single"))
					a = pango_attr_underline_new (PANGO_UNDERLINE_SINGLE);
				else if (0 == strcmp (val, "double"))
					a = pango_attr_underline_new (PANGO_UNDERLINE_DOUBLE);
				else if (0 == strcmp (val, "low"))
					a = pango_attr_underline_new (PANGO_UNDERLINE_LOW);
				else if (0 == strcmp (val, "error"))
					a = pango_attr_underline_new (PANGO_UNDERLINE_ERROR);
			}
			break;

		case 13:
			if (0 == strncmp (str, "strikethrough", 13))
				a = pango_attr_strikethrough_new (atoi (val) != 0);
			break;
		}

		if (a != NULL && val_end != NULL) {
			if (sscanf (val_end+1, "%u:%u]", &a->start_index, &a->end_index) == 2 &&
				a->start_index < a->end_index)
				pango_attr_list_insert (attrs, a);
			else
				pango_attribute_destroy (a);
		}

		*val_end = ':';
		*closer = ']';
	}

	return attrs;

 bail:
	pango_attr_list_unref (attrs);
	return NULL;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_new_from_XL :
 * @str: XL descriptor in UTF-8 encoding.
 *
 * Returns: Looks up and potentially creates a GOFormat from the supplied
 * 	string in XL format.
 **/
GOFormat *
go_format_new_from_XL (char const *str)
{
	GOFormat *format;

	g_return_val_if_fail (str != NULL, go_format_general ());

	if (str[0] == '@' && str[1] == '[') {
		PangoAttrList *attrs;
		char *desc_copy = g_strdup (str);
		attrs = go_format_parse_markup (desc_copy);
		if (attrs) {
			format = go_format_create (GO_FMT_MARKUP, str);
			format->u.markup = attrs;
		} else
			format = go_format_create (GO_FMT_INVALID, str);

		g_free (desc_copy);
		return format;
	}

	format = g_hash_table_lookup (style_format_hash, str);
	if (format == NULL) {
		format = go_format_parse (str);
		g_hash_table_insert (style_format_hash,
				     format->format,
				     format);
	}

#ifdef DEBUG_REF_COUNT
	g_message ("%s: format=%p '%s' ref_count=%d",
		   G_GNUC_FUNCTION,
		   format, format->format, format->ref_count);
#endif

	return go_format_ref (format);
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_new_markup :
 * @markup : #PangoAttrList
 * @add_ref : boolean
 *
 * If @add_ref is FALSE absorb the reference to @markup, otherwise add a
 * reference.
 *
 * Returns: A new format.
 **/
GOFormat *
go_format_new_markup (PangoAttrList *markup, gboolean add_ref)
{
	GOFormat *fmt;

	GString *accum = g_string_new ("@");
	pango_attr_list_filter (markup,
		(PangoAttrFilterFunc) cb_attrs_as_string, accum);
	fmt = go_format_new_from_XL (accum->str);
	g_string_free (accum, TRUE);

	if (!add_ref)
		pango_attr_list_unref (markup);

	return fmt;
}
#endif


#ifdef DEFINE_COMMON
/**
 * go_format_as_XL:
 * @fmt: a #GOFormat
 *
 * Returns: the XL style format strint.
 */
const char *
go_format_as_XL (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, "General");

	return fmt->format;
}
#endif

#ifdef DEFINE_COMMON
gboolean
go_format_eq (GOFormat const *a, GOFormat const *b)
{
	/*
	 * The way we create GOFormat *s ensures that we don't need
	 * to compare anything but pointers.
	 */
	return (a == b);
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_ref :
 * @fmt : a #GOFormat
 *
 * Adds a reference to a GOFormat.
 *
 * Returns: @gf
 **/
GOFormat *
go_format_ref (GOFormat const *gf_)
{
	GOFormat *gf = (GOFormat *)gf_;

	g_return_val_if_fail (gf != NULL, NULL);

	gf->ref_count++;
#ifdef DEBUG_REF_COUNT
	g_message ("%s: format=%p '%s' ref_count=%d",
		   G_GNUC_FUNCTION,
		   gf, gf->format, gf->ref_count);
#endif

	return gf;
}
#endif


#ifdef DEFINE_COMMON
/**
 * go_format_unref :
 * @fmt : a #GOFormat
 *
 * Removes a reference to @fmt, freeing when it goes to zero.
 *
 **/
void
go_format_unref (GOFormat const *gf_)
{
	GOFormat *gf = (GOFormat *)gf_;

	if (gf == NULL)
		return;

	g_return_if_fail (gf->ref_count > 0);

	gf->ref_count--;
#ifdef DEBUG_REF_COUNT
	g_message ("%s: format=%p '%s' ref_count=%d",
		   G_GNUC_FUNCTION,
		   gf, gf->format, gf->ref_count);
#endif
	if (gf->ref_count != 0)
		return;

	switch (gf->typ) {
	case GO_FMT_COND: {
		int i;
		for (i = 0; i < gf->u.cond.n; i++)
			go_format_unref (gf->u.cond.conditions[i].fmt);
		g_free (gf->u.cond.conditions);
		break;
	}
	case GO_FMT_NUMBER:
		g_free (gf->u.number.program);
		break;
	case GO_FMT_TEXT:
		g_free (gf->u.text.program);
		break;
	case GO_FMT_EMPTY:
	case GO_FMT_INVALID:
		break;
	case GO_FMT_MARKUP:
		if (gf->u.markup)
			pango_attr_list_unref (gf->u.markup);
		break;
	}

	g_free (gf->format);
	g_free (gf);
}
#endif


#ifdef DEFINE_COMMON
/**
 * go_format_is_invalid
 * @fmt: Format to query
 *
 * Returns: TRUE if, and if only, the format is invalid
 **/
gboolean
go_format_is_invalid (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, TRUE);
	return fmt->typ == GO_FMT_INVALID;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_is_general
 * @fmt: Format to query
 *
 * Returns: TRUE if the format is "General", possibly with condition,
 * 	color, and/or locale.  ("xGeneral" is thus not considered to be General
 * 	for the purpose of this function.)
 *	Returns FALSE otherwise.
 **/
gboolean
go_format_is_general (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, FALSE);
	return fmt->typ == GO_FMT_NUMBER && fmt->u.number.is_general;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_is_markup
 * @fmt: Format to query
 *
 * Returns: TRUE if the format is a markup format
 * 	Returns FALSE otherwise.
 */
gboolean
go_format_is_markup (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, FALSE);
	return fmt->typ == GO_FMT_MARKUP;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_is_text
 * @fmt: Format to query
 *
 * Returns: TRUE if the format is a text format
 * 	Returns FALSE otherwise.
 **/
gboolean
go_format_is_text (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, FALSE);
	return fmt->typ == GO_FMT_TEXT;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_is_var_width
 * @fmt: Format to query
 *
 * Returns: TRUE if the format is variable width, i.e., can stretch.
 * 	Returns FALSE otherwise.
 **/
gboolean
go_format_is_var_width (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, FALSE);

	if (fmt->has_fill != 0)
		return TRUE;

	switch (fmt->typ) {
	case GO_FMT_COND: {
		int i;
		for (i = 0; i < fmt->u.cond.n; i++)
			if (go_format_is_var_width (fmt->u.cond.conditions[i].fmt))
				return TRUE;
		return FALSE;
	}
	case GO_FMT_NUMBER:
		return fmt->u.number.has_general;
	default:
		return FALSE;
	}
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_is_date:
 * @fmt: Format to query
 *
 * Returns:
 *      +2 if the format is a date format with time
 *      +1 if the format is any other date format.
 * 	 0 if the format is not a date format.
 * 	-1 if the format is inconsistent.
 **/
int
go_format_is_date (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, -1);
	if (go_format_get_family (fmt) != GO_FORMAT_DATE)
		return 0;
	return fmt->u.number.has_time ? +2 : +1;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_is_time:
 * @fmt: Format to query
 *
 * Returns:
 *   +2 if the format is a time format with elapsed hour/minute/second
 *   +1 if the format is any other time format
 *    0 if the format is not a time format
 *   -1	if the format is inconsistent.
 **/
int
go_format_is_time (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, -1);
	if (go_format_get_family (fmt) != GO_FORMAT_TIME)
		return 0;
	return fmt->u.number.has_elapsed ? +2 : +1;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_month_before_day:
 * @fmt: Format to query
 *
 * Returns:
 *  0, if format is a date format with day and month in that order
 *  1, if format is a date format with month and day in that order, unless
 *  2, if format is a date with year before month before day
 * -1, otherwise.
 **/
int
go_format_month_before_day (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, -1);

	if (go_format_is_date (fmt) < 1)
		return -1;
	if (fmt->u.number.date_ybm)
		return +2;
	if (fmt->u.number.date_mbd)
		return +1;
	if (fmt->u.number.date_dbm)
		return 0;
	return -1;
}
#endif


#ifdef DEFINE_COMMON
/**
 * go_format_has_hour:
 * @fmt: Format to query
 *
 * Returns: TRUE if format is a number format with an hour specifier
 * 	    FALSE otherwise.
 **/
gboolean
go_format_has_hour (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, FALSE);

	return (fmt->typ == GO_FMT_NUMBER &&
		fmt->u.number.has_time &&
		fmt->u.number.has_hour);
}
#endif


#ifdef DEFINE_COMMON
/**
 * go_format_has_minute:
 * @fmt: Format to query
 *
 * Returns: TRUE if format is a number format with a minute specifier
 * 	    FALSE otherwise.
 **/
gboolean
go_format_has_minute (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, FALSE);

	return (fmt->typ == GO_FMT_NUMBER &&
		fmt->u.number.has_time &&
		fmt->u.number.has_minute);
}
#endif


#ifdef DEFINE_COMMON
/**
 * go_format_get_magic:
 * @fmt: Format to query
 *
 * Returns: a non-zero magic code for certain formats, such as system date.
 **/
GOFormatMagic
go_format_get_magic (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, GO_FORMAT_MAGIC_NONE);

	return fmt->magic;
}
#endif


#ifdef DEFINE_COMMON
GOFormat *
go_format_new_magic (GOFormatMagic m)
{
	const char *suffix;
	char *s;
	GOFormat *res;

	/*
	 * Note: the format strings here are actually fixed and do not relate
	 * to how these formats are rendered.
	 */

	switch (m) {
	default:
		return NULL;

	case GO_FORMAT_MAGIC_LONG_DATE:
		suffix = "dddd, mmmm dd, yyyy";
		break;

	case GO_FORMAT_MAGIC_MEDIUM_DATE:
		suffix = "d-mmm-yy";
		break;

	case GO_FORMAT_MAGIC_SHORT_DATE:
		suffix = "m/d/yy";
		break;

	case GO_FORMAT_MAGIC_SHORT_DATETIME:
		suffix = "m/d/yy h:mm";
		break;

	case GO_FORMAT_MAGIC_LONG_TIME:
		suffix = "h:mm:ss AM/PM";
		break;

	case GO_FORMAT_MAGIC_MEDIUM_TIME:
		suffix = "h:mm AM/PM";
		break;

	case GO_FORMAT_MAGIC_SHORT_TIME:
		suffix = "hh:mm";
		break;
	}

	s = g_strdup_printf ("[$-%x]%s", (unsigned)m, suffix);
	res = go_format_new_from_XL (s);
	g_free (s);
	return res;
}
#endif


const GOFormat *
SUFFIX(go_format_specialize) (GOFormat const *fmt, DOUBLE val, char type,
			      gboolean *inhibit_minus)
{
	int i;
	gboolean is_number = (type == 'F');
	GOFormat *last_implicit_num = NULL;
	gboolean has_implicit = FALSE;
	gboolean dummy;

	g_return_val_if_fail (fmt != NULL, NULL);

	if (inhibit_minus == NULL)
		inhibit_minus = &dummy;

	*inhibit_minus = FALSE;

	if (fmt->typ != GO_FMT_COND) {
		if (fmt->typ == GO_FMT_EMPTY && !is_number)
			return go_format_general ();
		return fmt;
	}

	for (i = 0; i < fmt->u.cond.n; i++) {
		GOFormatCondition *c = fmt->u.cond.conditions + i;
		gboolean cond;

		if (c->implicit) {
			if (c->op != GO_FMT_COND_TEXT)
				last_implicit_num = c->fmt;
			has_implicit = TRUE;
		} else {
			if (has_implicit)
				*inhibit_minus = FALSE;
			last_implicit_num = NULL;
			has_implicit = FALSE;
		}

		switch (c->op) {
		case GO_FMT_COND_EQ:
			cond = (is_number && val == c->val);
			break;
		case GO_FMT_COND_NE:
			cond = (is_number && val != c->val);
			break;
		case GO_FMT_COND_LT:
			cond = (is_number && val <  c->val);
			break;
		case GO_FMT_COND_LE:
			cond = (is_number && val <= c->val);
			break;
		case GO_FMT_COND_GT:
			cond = (is_number && val >  c->val);
			break;
		case GO_FMT_COND_GE:
			cond = (is_number && val >= c->val);
			break;
		case GO_FMT_COND_TEXT:
			cond = (type == 'S' || type == 'B');
			break;
		case GO_FMT_COND_NONTEXT:
			cond = is_number;
			break;
		default:
			cond = TRUE;
			break;
		}

		if (cond) {
			if (c->true_inhibits_minus)
				*inhibit_minus = TRUE;
			return c->fmt;
		}

		if (c->false_inhibits_minus)
			*inhibit_minus = TRUE;
	}

	*inhibit_minus = FALSE;

	if (is_number) {
		if (last_implicit_num)
			return last_implicit_num;
		else if (has_implicit)
			return go_format_empty ();
	}

	return go_format_general ();
}

#ifdef DEFINE_COMMON
GOFormat *
go_format_general (void)
{
	if (!default_general_fmt)
		default_general_fmt = go_format_new_from_XL (
			_go_format_builtins[GO_FORMAT_GENERAL][0]);
	return default_general_fmt;
}
#endif

#ifdef DEFINE_COMMON
GOFormat *
go_format_empty (void)
{
	if (!default_empty_fmt)
		default_empty_fmt = go_format_new_from_XL ("");
	return default_empty_fmt;
}
#endif

#ifdef DEFINE_COMMON
GOFormat *
go_format_default_date (void)
{
	if (!default_date_fmt)
		default_date_fmt =
			go_format_new_magic (GO_FORMAT_MAGIC_SHORT_DATE);
	return default_date_fmt;
}
#endif

#ifdef DEFINE_COMMON
GOFormat *
go_format_default_time (void)
{
	if (!default_time_fmt)
		default_time_fmt =
			go_format_new_magic (GO_FORMAT_MAGIC_SHORT_TIME);
	return default_time_fmt;
}
#endif

#ifdef DEFINE_COMMON
GOFormat *
go_format_default_date_time (void)
{
	if (!default_date_time_fmt)
		default_date_time_fmt =
			go_format_new_magic (GO_FORMAT_MAGIC_SHORT_DATETIME);
	return default_date_time_fmt;
}
#endif

#ifdef DEFINE_COMMON
GOFormat *
go_format_default_percentage (void)
{
	if (!default_percentage_fmt)
		default_percentage_fmt = go_format_new_from_XL (
			_go_format_builtins[GO_FORMAT_PERCENTAGE][1]);
	return default_percentage_fmt;
}
#endif

#ifdef DEFINE_COMMON
GOFormat *
go_format_default_money (void)
{
	if (!default_money_fmt)
		default_money_fmt = go_format_new_from_XL (
			_go_format_builtins[GO_FORMAT_CURRENCY][2]);
	return default_money_fmt;
}
#endif

#ifdef DEFINE_COMMON
GOFormat *
go_format_default_accounting (void)
{
	if (!default_accounting_fmt)
		default_accounting_fmt = go_format_new_from_XL (
			_go_format_builtins[GO_FORMAT_ACCOUNTING][2]);
	return default_accounting_fmt;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_generate_number_str:
 * @dst: GString to append format string to.
 * @min_digits: minimum number of digits before decimal separator.
 * @num_decimals: number of decimals
 * @thousands_sep: if true, use a thousands separator.
 * @negative_red: if true, make negative values red.
 * @negative_paren: if true, enclose negative values in parentheses.
 * @prefix: optional string to place before number part of the format
 * @postfix: optional string to place after number part of the format
 *
 * Generates a format string for a number format with the given
 * parameters and appends it to @dst.
 **/
void
go_format_generate_number_str (GString *dst,
			       int min_digits,
			       int num_decimals,
			       gboolean thousands_sep,
			       gboolean negative_red,
			       gboolean negative_paren,
			       const char *prefix, const char *postfix)
{
	size_t init_len = dst->len;
	size_t plain_len;

	min_digits = CLAMP (min_digits, 0, MAX_DECIMALS);
	num_decimals = CLAMP (num_decimals, 0, MAX_DECIMALS);

	if (prefix)
		g_string_append (dst, prefix);

	if (thousands_sep) {
		switch (min_digits) {
		case 0: g_string_append (dst, "#,###"); break;
		case 1: g_string_append (dst, "#,##0"); break;
		case 2: g_string_append (dst, "#,#00"); break;
		case 3: g_string_append (dst, "#,000"); break;
		default: {
			int r = min_digits % 3;
			go_string_append_c_n (dst, '0', r ? r : 3);
			for (r = min_digits; r > 3; r -= 3)
				g_string_append (dst, ",000");
		}
		}
	} else {
		if (min_digits > 0)
			go_string_append_c_n (dst, '0', min_digits);
		else
			g_string_append_c (dst, '#');
	}

	if (num_decimals > 0) {
		g_string_append_c (dst, '.');
		go_string_append_c_n (dst, '0', num_decimals);
	}

	if (postfix)
		g_string_append (dst, postfix);

	plain_len = dst->len - init_len;

	if (negative_paren)
		g_string_append (dst, "_)");
	if (negative_paren || negative_red)
		g_string_append_c (dst, ';');
	if (negative_red)
		g_string_append (dst, "[Red]");
	if (negative_paren)
		g_string_append (dst, "(");
	if (negative_paren || negative_red)
		g_string_append_len (dst, dst->str + init_len, plain_len);
	if (negative_paren)
		g_string_append (dst, ")");
}
#endif

#ifdef DEFINE_COMMON
static void
go_format_generate_scientific_str (GString *dst, GOFormatDetails const *details)
{
	/* Maximum not terribly important. */
	int step = CLAMP (details->exponent_step, 1, 10);
	int num_decimals = CLAMP (details->num_decimals, 0, MAX_DECIMALS);

	go_string_append_c_n (dst, '#', step - 1);
	if (details->simplify_mantissa)
		g_string_append_c (dst, '#');
	else
		g_string_append_c (dst, '0');

	if (num_decimals > 0) {
		g_string_append_c (dst, '.');
		go_string_append_c_n (dst, '0', num_decimals);
	}

	if (details->use_markup)
		g_string_append (dst, "EE0");
	else
		g_string_append (dst, "E+00");
}
#endif

#ifdef DEFINE_COMMON
static void
go_format_generate_accounting_str (GString *dst,
				   GOFormatDetails const *details)
{
	int num_decimals = CLAMP (details->num_decimals, 0, MAX_DECIMALS);
	GString *num = g_string_new (NULL);
	GString *sym = g_string_new (NULL);
	GString *q = g_string_new (NULL);
	const char *symstr;
	const char *quote = "\"";
	GOFormatCurrency const *currency = details->currency;

	if (!currency)
		currency = &_go_format_currencies[0];

	symstr = currency->symbol;
	switch (g_utf8_get_char (symstr)) {
	case '$':
	case UNICODE_POUNDS1:
	case UNICODE_YEN:
	case UNICODE_EURO:
		if ((g_utf8_next_char (symstr))[0])
			break;  /* Something follows.  */
		/* Fall through.  */
	case 0:
	case '[':
		quote = "";
		break;
	default:
		break;
	}

	go_format_generate_number_str (num, details->min_digits,
				       num_decimals,
				       details->thousands_sep,
				       FALSE, FALSE, NULL, NULL);
	go_string_append_c_n (q, '?', num_decimals);

	if (currency->precedes) {
		g_string_append (sym, quote);
		g_string_append (sym, symstr);
		g_string_append (sym, quote);
		g_string_append (sym, "* ");
		if (currency->has_space) g_string_append_c (sym, ' ');

		g_string_append_printf
			(dst,
			 "_(%s%s_);_(%s(%s);_(%s\"-\"%s_);_(@_)",
			 sym->str, num->str,
			 sym->str, num->str,
			 sym->str, q->str);
	} else {
		g_string_append (sym, "* ");
		if (currency->has_space) g_string_append_c (sym, ' ');
		g_string_append (sym, quote);
		g_string_append (sym, symstr);
		g_string_append (sym, quote);

		g_string_append_printf
			(dst,
			 "_(%s%s_);_((%s)%s;_(\"-\"%s%s_);_(@_)",
			 num->str, sym->str,
			 num->str, sym->str,
			 q->str, sym->str);
	}

	g_string_free (num, TRUE);
	g_string_free (q, TRUE);
	g_string_free (sym, TRUE);
}
#endif

#ifdef DEFINE_COMMON
static void
go_format_generate_currency_str (GString *dst,
				 GOFormatDetails const *details)
{
	GString *prefix = NULL;
	GString *postfix = NULL;
	gboolean extra_quotes;
	GOFormatCurrency const *currency = details->currency;

	if (!currency)
		currency = &_go_format_currencies[0];

	extra_quotes = (details->force_quoted &&
			currency->symbol[0] != '"' &&
			currency->symbol[0] != 0);

	if (currency->precedes) {
		prefix = g_string_new (NULL);
		if (extra_quotes) g_string_append_c (prefix, '"');
		g_string_append (prefix, currency->symbol);
		if (extra_quotes) g_string_append_c (prefix, '"');
		if (currency->has_space) g_string_append_c (prefix, ' ');
	} else {
		postfix = g_string_new (NULL);
		if (currency->has_space)
			g_string_append_c (postfix, ' ');
		if (extra_quotes) g_string_append_c (postfix, '"');
		g_string_append (postfix, currency->symbol);
		if (extra_quotes) g_string_append_c (postfix, '"');
	}

	go_format_generate_number_str (dst,
				       details->min_digits,
				       details->num_decimals,
				       details->thousands_sep,
				       details->negative_red,
				       details->negative_paren,
				       prefix ? prefix->str : NULL,
				       postfix ? postfix->str : NULL);

	if (prefix) g_string_free (prefix, TRUE);
	if (postfix) g_string_free (postfix, TRUE);
}
#endif

#ifdef DEFINE_COMMON
static const GOFormatCurrency *
find_currency (char const *ptr, gsize len, gboolean precedes)
{
	int i;
	gboolean has_space;
	gboolean quoted;

	if (len <= 0)
		return 0;

	if (precedes) {
		has_space = ptr[len - 1] == ' ';
		if (has_space)
			len--;
	} else {
		has_space = ptr[0] == ' ';
		if (has_space)
			len--, ptr++;
	}

	quoted = len > 2 && ptr[0] == '\"' && ptr[len - 1] == '\"';

	for (i = 1; _go_format_currencies[i].symbol; i++) {
		const GOFormatCurrency *ci = _go_format_currencies + i;

		if (ci->precedes != precedes)
			continue;

		if (strncmp (ci->symbol, ptr, len) == 0) {
			return ci;
		}

		/* Allow quoting of things that aren't [$FOO] */
		if (quoted && ci->symbol[0] != '[' &&
		    strncmp (ci->symbol, ptr + 1, len - 2) == 0) {
			return ci;
		}
	}

	return NULL;
}
#endif

#ifdef DEFINE_COMMON
void
go_format_generate_str (GString *dst, GOFormatDetails const *details)
{
	switch (details->family) {
	case GO_FORMAT_TEXT:
		g_string_append (dst, "@");
		break;
	case GO_FORMAT_GENERAL:
		g_string_append (dst, "General");
		break;
	case GO_FORMAT_NUMBER:
		go_format_generate_number_str
			(dst,
			 details->min_digits,
			 details->num_decimals,
			 details->thousands_sep,
			 details->negative_red,
			 details->negative_paren,
			 NULL, NULL);
		break;
	case GO_FORMAT_CURRENCY:
		go_format_generate_currency_str (dst, details);
		break;
	case GO_FORMAT_ACCOUNTING:
		go_format_generate_accounting_str (dst, details);
		break;
	case GO_FORMAT_PERCENTAGE:
		go_format_generate_number_str
			(dst,
			 details->min_digits,
			 details->num_decimals,
			 details->thousands_sep,
			 details->negative_red,
			 details->negative_paren,
			 NULL, "%");
		break;
	case GO_FORMAT_SCIENTIFIC:
		go_format_generate_scientific_str (dst, details);
		break;
	default:
		break;
	}
}
#endif

#ifdef DEFINE_COMMON
GOFormatDetails *
go_format_details_new (GOFormatFamily family)
{
	GOFormatDetails *res = g_new (GOFormatDetails, 1);
	go_format_details_init (res, family);
	return res;
}
#endif


#ifdef DEFINE_COMMON
void
go_format_details_free (GOFormatDetails *details)
{
	/* We do not own ->currency.  */
	g_free (details);
}
#endif


#ifdef DEFINE_COMMON
void
go_format_details_init (GOFormatDetails *details, GOFormatFamily family)
{
	g_return_if_fail (details != NULL);

	memset (details, 0, sizeof (*details));
	/* Assign reasonable defaults.  For most, the memset is just fine. */
	details->family = family;
	details->thousands_sep = (family == GO_FORMAT_ACCOUNTING ||
				  family == GO_FORMAT_CURRENCY);
	details->magic = GO_FORMAT_MAGIC_NONE;
	details->exponent_step = 1;
	details->min_digits = 1;
}
#endif


#ifdef DEFINE_COMMON
void
go_format_get_details (GOFormat const *fmt,
		       GOFormatDetails *dst,
		       gboolean *exact)
{
	const char *str;
	GString *newstr = NULL;
	static GOFormatCurrency currency;

	g_return_if_fail (fmt != NULL);
	g_return_if_fail (dst != NULL);

	if (exact) *exact = FALSE;
	go_format_details_init (dst, go_format_get_family (fmt));
	dst->magic = go_format_get_magic (fmt);

	str = go_format_as_XL (fmt);

	switch (dst->family) {
	case GO_FORMAT_NUMBER:
	case GO_FORMAT_CURRENCY:
	case GO_FORMAT_ACCOUNTING:
	case GO_FORMAT_PERCENTAGE:
	case GO_FORMAT_SCIENTIFIC: {
		/*
		 * These guesses only have to be good enough to work on
		 * the formats we generate ourselves.
		 */

 		const char *dot = strchr (str, '.');
 		const char *zero = strchr (str, '0');
 		const char *comma = strchr (str, ',');

		if (dot) {
			while (dot[dst->num_decimals + 1] == '0')
				dst->num_decimals++;
		}

		dst->min_digits = 0;
		if (zero) {
			const char *p = zero;
			while (*p == ',' || *p == '0') {
				if (*p == '0')
					dst->min_digits++;
				p++;
			}
		}

		dst->negative_red = (strstr (str, ";[Red]") != NULL);
		dst->negative_paren = (strstr (str, "_);") != NULL);

		if (str[0] == '_' && str[1] == '(') {
			const char *start = str + 2;
			gboolean precedes = start[0] != '#';
			gsize len = 0;
			const GOFormatCurrency *pcurr;

			if (precedes) {
				while (start[len] && start[len] != '*')
					len++;
			} else {
				while (start[0] == '0' || start[0] == '.' ||
				       start[0] == '#' || start[0] == ',')
					start++;
				if (start[0] == '*' && start[1])
					start += 2;
				while (start[len] && start[len] != '_')
					len++;
			}

			pcurr = find_currency (start, len, precedes);
			if (pcurr) {
				dst->currency = &currency;
				currency = *pcurr;
			}
			dst->force_quoted = (start[0] == '"');
			dst->family = GO_FORMAT_ACCOUNTING;
		} else {
			gboolean precedes = str[0] != '0' && str[0] != '#';
			const char *start;
			gsize len = 0;
			const GOFormatCurrency *pcurr;

			if (precedes) {
				start = str;
				while (start[len] && start[len] != '0' && start[len] != '#')
					len++;
			} else {
				start = str + strlen (str);
				if (start > str && start[-1] == ')')
					start--;
				while (start > str && start[-1] != '0' && start[-1] != '#')
					start--, len++;
			}

			pcurr = find_currency (start, len, precedes);
			if (pcurr) {
				dst->currency = &currency;
				currency = *pcurr;
				dst->force_quoted = (start[0] == '"');
				dst->family = GO_FORMAT_CURRENCY;
			}
		}

		dst->thousands_sep = (comma > str &&
				      (comma[-1] == '0' || comma[-1] == '#') &&
				      (comma[1] == '0' || comma[1] == '#') &&
				      (comma[2] == '0' || comma[2] == '#') &&
				      (comma[3] == '0' || comma[3] == '#'));

		if (dst->family == GO_FORMAT_SCIENTIFIC) {
			const char *mend = dot ? dot : strchr (str, 'E');
			dst->use_markup = (strstr (str, "EE0") != NULL);
			dst->exponent_step = mend - str;
			dst->simplify_mantissa = mend != str && mend[-1] == '#';
			if (dst->simplify_mantissa)
				dst->min_digits = 0;
		}

		if (exact != NULL) {
			newstr = g_string_new (NULL);
			go_format_generate_str (newstr, dst);
		}

		break;
	}

	case GO_FORMAT_DATE:
	case GO_FORMAT_TIME: {
		const char *sdot = strstr (str, "s.");

		if (sdot) {
			while (sdot[dst->num_decimals + 2] == '0')
				dst->num_decimals++;
		}

		break;
	}

	default:
		break;
	}

	if (newstr) {
		*exact = (strcmp (str, newstr->str) == 0);
		g_string_free (newstr, TRUE);
	}
}
#endif


#ifdef DEFINE_COMMON
GOFormatCurrency const *
go_format_locale_currency (void)
{
	static GOFormatCurrency currency;
	const GOFormatCurrency *pcurr;
	gboolean precedes, has_space;
	const GString *lcurr = go_locale_get_currency (&precedes, &has_space);

#if 0
	g_printerr ("go_format_locale_currency: looking for [%s] %d %d\n", symbol, precedes, has_space);
#endif

	pcurr = find_currency (lcurr->str, lcurr->len, precedes);
	if (pcurr)
		return pcurr;

	currency.has_space = has_space;
	currency.precedes = precedes;
	currency.symbol = lcurr->str;
	currency.description = NULL;
	return &currency;
}
#endif


/********************* GOFormat ODF Support ***********************/

#define STYLE	 "style:"
#define FOSTYLE	 "fo:"
#define NUMBER   "number:"
#define GNMSTYLE "gnm:"


#ifdef DEFINE_COMMON
char *
go_format_odf_style_map (GOFormat const *fmt, int cond_part)
{
	char const *format_string = NULL;

	g_return_val_if_fail (fmt != NULL, NULL);
	g_return_val_if_fail (fmt->typ == GO_FMT_COND, NULL);

	if (cond_part >= fmt->u.cond.n)
		return NULL;

	switch (fmt->u.cond.conditions[cond_part].op) {
	case GO_FMT_COND_EQ:
		format_string = "value()=%g";
		break;
	case GO_FMT_COND_NE:
		format_string = "value()!=%g";
		break;
	case GO_FMT_COND_NONTEXT: /* Under certain circumstances this */
                                  /*appears for second of two conditions */
	case GO_FMT_COND_LT:
		format_string = "value()<%g";
		break;
	case GO_FMT_COND_LE:
		format_string = "value()<=%g";
		break;
	case GO_FMT_COND_GT:
		format_string = "value()>%g";
		break;
	case GO_FMT_COND_GE:
		format_string = "value()>=%g";
		break;
	default:
		return NULL;
	}
	return g_strdup_printf (format_string,
				fmt->u.cond.conditions[cond_part].val);

}
#endif

#ifdef DEFINE_COMMON

static void
odf_add_bool (GsfXMLOut *xout, char const *id, gboolean val)
{
	gsf_xml_out_add_cstr_unchecked (xout, id, val ? "true" : "false");
}

#define ODF_CLOSE_STRING  if (string_is_open) {  \
                                 gsf_xml_out_add_cstr (xout, NULL, accum->str); \
                                 gsf_xml_out_end_element (xout); /* </number:text> */  \
				 string_is_open = FALSE; \
                          }
#define ODF_OPEN_STRING   if (!string_is_open) {			\
		gsf_xml_out_start_element (xout, NUMBER "text");	\
		string_is_open = TRUE;					\
		text_written = TRUE;					\
		g_string_erase (accum, 0, -1);				\
	}

static void
go_format_output_date_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
			      char const *name,
			      GOFormatFamily family,
			      gboolean with_extension)
{
	char const *xl = go_format_as_XL (fmt);
	GString *accum = g_string_new (NULL);
	gboolean time_only = (family == GO_FORMAT_TIME);
	gboolean seen_year = FALSE;
	gboolean seen_month = FALSE;
	gboolean seen_day = FALSE;
	gboolean seen_weekday = FALSE;
	gboolean seen_hour = FALSE;
	gboolean seen_ampm = FALSE;
	gboolean seen_minute = FALSE;
	gboolean seen_second = FALSE;
	gboolean seen_elapsed = FALSE;
	gboolean m_is_minutes = FALSE;
	gboolean string_is_open = FALSE;
	gboolean seconds_trigger_minutes = TRUE;
	gboolean element_written = FALSE;
	gboolean text_written = FALSE;
	GOFormatMagic magic = go_format_get_magic (fmt);
	gboolean color_completed = FALSE;

	gsf_xml_out_start_element (xout,  time_only ?
				   NUMBER "time-style" : NUMBER "date-style");
	gsf_xml_out_add_cstr (xout, STYLE "name", name);
	if (magic == GO_FORMAT_MAGIC_NONE)
		gsf_xml_out_add_cstr (xout, NUMBER "format-source", "fixed");
	else {
		xl = _(xl);
		gsf_xml_out_add_cstr (xout, NUMBER "format-source", "language");
		if (with_extension)
			gsf_xml_out_add_int (xout, GNMSTYLE "format-magic", magic);
	}

	while (1) {
		const char *token = xl;
		GOFormatTokenType tt;
		int t = go_format_token (&xl, &tt);

		switch (t) {
		case 0: case ';':
			ODF_CLOSE_STRING;
			if (!element_written)
				gsf_xml_out_simple_element (xout, NUMBER "am-pm", NULL);
			gsf_xml_out_end_element (xout); /* </number:date-style or time-style> */
			g_string_free (accum, TRUE);
			return;

		case 'd': case 'D': {
			int n = 1;
			while (*xl == 'd' || *xl == 'D')
				xl++, n++;
			if (time_only) break;
			switch (n) {
			case 1:
			case 2: if (seen_day) break;
				seen_day = TRUE;
				ODF_CLOSE_STRING;
				element_written = TRUE;
				gsf_xml_out_start_element (xout, NUMBER "day");
				gsf_xml_out_add_cstr (xout, NUMBER "style",
						      (n==1) ? "short" : "long");
				gsf_xml_out_end_element (xout); /* </number:day> */
				break;
			case 3:
			default: if (seen_weekday) break;
				seen_weekday = TRUE;
				ODF_CLOSE_STRING;
				element_written = TRUE;
				gsf_xml_out_start_element (xout, NUMBER "day-of-week");
				gsf_xml_out_add_cstr (xout, NUMBER "style",
						      (n==3) ? "short" : "long");
				gsf_xml_out_end_element (xout); /* </number:day-of-week> */
				break;
			}
			break;
		}

		case 'y': case 'Y': {
			int n = 1;
			while (*xl == 'y' || *xl == 'Y')
				xl++, n++;
			if (time_only || seen_year) break;
			seen_year = TRUE;
			ODF_CLOSE_STRING;
			element_written = TRUE;
			gsf_xml_out_start_element (xout, NUMBER "year");
			gsf_xml_out_add_cstr (xout, NUMBER "style",
					      (n <= 2) ? "short" : "long");
			gsf_xml_out_add_cstr (xout, NUMBER "calendar", "gregorian");
			gsf_xml_out_end_element (xout); /* </number:year> */
			break;
		}

		case 'b': case 'B': {
			int n = 1;
			while (*xl == 'b' || *xl == 'B')
				xl++, n++;
			if (time_only || seen_year) break;
			seen_year = TRUE;
			ODF_CLOSE_STRING;
			element_written = TRUE;
			gsf_xml_out_start_element (xout, NUMBER "year");
			gsf_xml_out_add_cstr (xout, NUMBER "style",
					      (n <= 2) ? "short" : "long");
			gsf_xml_out_add_cstr (xout, NUMBER "calendar", "buddhist");
			gsf_xml_out_end_element (xout); /* </number:year> */
			break;
		}

		case 'e': {  /* What is 'e' really? */
			while (*xl == 'e') xl++;
			if (time_only || seen_year) break;
			seen_year = TRUE;
			ODF_CLOSE_STRING;
			gsf_xml_out_start_element (xout, NUMBER "year");
			gsf_xml_out_add_cstr (xout, NUMBER "style", "long");
			gsf_xml_out_add_cstr (xout, NUMBER "calendar", "gregorian");
			gsf_xml_out_end_element (xout); /* </number:year> */
			break;
		}

		case 'g': case 'G':
			/* Something with Japanese eras.  Blank for me. */
			break;

		case 'h': case 'H': {
			int n = 1;
			while (*xl == 'h' || *xl == 'H')
				xl++, n++;
			if (seen_hour) break;
			seen_hour = TRUE;
			ODF_CLOSE_STRING;
			element_written = TRUE;
			gsf_xml_out_start_element (xout, NUMBER "hours");
			gsf_xml_out_add_cstr (xout, NUMBER "style",
					      (n == 1) ? "short" : "long");
			if (with_extension)
				gsf_xml_out_add_cstr (xout, GNMSTYLE "truncate-on-overflow", "true");
			gsf_xml_out_end_element (xout); /* </number:hours> */
			m_is_minutes = TRUE;
			break;
		}

		case 'm': case 'M': {
			int n = 1;
			while (*xl == 'm' || *xl == 'M')
				xl++, n++;
			m_is_minutes = (n <= 2) && (m_is_minutes || tail_forces_minutes (xl));

			element_written = TRUE;
			if (m_is_minutes) {
				if (seen_minute) break;
				seen_minute = TRUE;
				m_is_minutes = FALSE;
				seconds_trigger_minutes = FALSE;
				ODF_CLOSE_STRING;
				gsf_xml_out_start_element (xout, NUMBER "minutes");
				gsf_xml_out_add_cstr (xout, NUMBER "style",
						      (n == 1) ? "short" : "long");
				if (with_extension)
					gsf_xml_out_add_cstr (xout, GNMSTYLE "truncate-on-overflow", "true");
				gsf_xml_out_end_element (xout); /* </number:minutes> */
			} else {
				if (seen_month || time_only) break;
				seen_month = TRUE;
				ODF_CLOSE_STRING;
				gsf_xml_out_start_element (xout, NUMBER "month");
				gsf_xml_out_add_cstr (xout, NUMBER "possessive-form", "false");
				switch (n) {
				case 1:
					gsf_xml_out_add_cstr (xout, NUMBER "textual", "false");
					gsf_xml_out_add_cstr (xout, NUMBER "style", "short");
					break;
				case 2:
					gsf_xml_out_add_cstr (xout, NUMBER "textual", "false");
					gsf_xml_out_add_cstr (xout, NUMBER "style", "long");
					break;
				case 3: /* ODF does not support single letter abbreviation */
				case 5:
					gsf_xml_out_add_cstr (xout, NUMBER "textual", "true");
					gsf_xml_out_add_cstr (xout, NUMBER "style", "short");
					break;
				default:
					gsf_xml_out_add_cstr (xout, NUMBER "textual", "true");
					gsf_xml_out_add_cstr (xout, NUMBER "style", "long");
					break;
				}

				gsf_xml_out_end_element (xout); /* </number:month> */
			}
			break;
		}

		case 's': case 'S': {
			int n = 1, d = 0;
			while (*xl == 's' || *xl == 'S')
				xl++, n++;
			if (*xl == '.' && *(xl + 1) == '0') {
				xl++;
				while (*xl == '0')
					xl++, d++;
			}
			if (seconds_trigger_minutes) {
				seconds_trigger_minutes = FALSE;
				m_is_minutes = TRUE;
			}
			if (seen_second) break;
			seen_second = TRUE;
			ODF_CLOSE_STRING;
			element_written = TRUE;
			gsf_xml_out_start_element (xout, NUMBER "seconds");
			gsf_xml_out_add_cstr (xout, NUMBER "style",
					      (n == 1) ? "short" : "long");
			gsf_xml_out_add_int (xout, NUMBER "decimal-places", d);
			if (with_extension)
				gsf_xml_out_add_cstr (xout, GNMSTYLE "truncate-on-overflow", "true");
			gsf_xml_out_end_element (xout); /* </number:seconds> */
			break;
		}

		case TOK_AMPM3:
		case TOK_AMPM5:
			if (seen_elapsed || seen_ampm) break;
			seen_ampm = TRUE;
			ODF_CLOSE_STRING;
			element_written = TRUE;
			gsf_xml_out_simple_element (xout, NUMBER "am-pm", NULL);
			break;

		case TOK_ELAPSED_H:
			if (seen_elapsed || seen_ampm || seen_hour) break;
			seen_hour = TRUE;
			seen_elapsed  = TRUE;
			ODF_CLOSE_STRING;
			if ((!text_written) && !element_written && time_only)
				gsf_xml_out_add_cstr (xout, NUMBER "truncate-on-overflow", "false");
			gsf_xml_out_start_element (xout, NUMBER "hours");
			gsf_xml_out_add_cstr (xout, NUMBER "style", "short");
			/* ODF can mark elapsed time in the time-style only and then not clearly */
			if (with_extension)
				gsf_xml_out_add_cstr (xout, GNMSTYLE "truncate-on-overflow", "false");
			gsf_xml_out_end_element (xout); /* </number:hours> */
			m_is_minutes = TRUE;
			element_written = TRUE;
			break;

		case TOK_ELAPSED_M:
			if (seen_elapsed || seen_ampm || seen_minute) break;
			seen_minute = TRUE;
			seen_elapsed  = TRUE;
			m_is_minutes = FALSE;
			seconds_trigger_minutes = FALSE;
			ODF_CLOSE_STRING;
			if ((!text_written) && !element_written && time_only)
				gsf_xml_out_add_cstr (xout, NUMBER "truncate-on-overflow", "false");
			gsf_xml_out_start_element (xout, NUMBER "minutes");
			gsf_xml_out_add_cstr (xout, NUMBER "style", "long");
			if (with_extension)
				gsf_xml_out_add_cstr (xout, GNMSTYLE "truncate-on-overflow", "false");
			gsf_xml_out_end_element (xout); /* </number:minutes> */
			element_written = TRUE;

		case TOK_ELAPSED_S:
			if (seen_elapsed || seen_ampm || seen_second) break;
			seen_elapsed = TRUE;
			seen_second = TRUE;
			if (seconds_trigger_minutes) {
				m_is_minutes = TRUE;
				seconds_trigger_minutes = FALSE;
			}
			ODF_CLOSE_STRING;
			if ((!text_written) && !element_written && time_only)
				gsf_xml_out_add_cstr (xout, NUMBER "truncate-on-overflow", "false");
			gsf_xml_out_start_element (xout, NUMBER "seconds");
			gsf_xml_out_add_cstr (xout, NUMBER "style", "short");
			if (with_extension)
				gsf_xml_out_add_cstr (xout, GNMSTYLE "truncate-on-overflow", "false");
			gsf_xml_out_end_element (xout); /* </number:seconds> */
			element_written = TRUE;
			break;

		case TOK_STRING: {
			size_t len = strchr (token + 1, '"') - (token + 1);
			if (len > 0) {
				ODF_OPEN_STRING;
				g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		case TOK_CHAR: {
			size_t len = g_utf8_next_char(token) - (token);
			if (len > 0) {
				ODF_OPEN_STRING;
				g_string_append_len (accum, token, len);
			}
			break;
		}

		case TOK_ESCAPED_CHAR: {
			size_t len = g_utf8_next_char(token + 1) - (token + 1);
			if (len > 0) {
				ODF_OPEN_STRING;
				g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		case TOK_THOUSAND:
			ODF_OPEN_STRING;
			g_string_append_c (accum, ',');
			break;

		case TOK_DECIMAL:
			ODF_OPEN_STRING;
			g_string_append_c (accum, '.');
			break;

		case TOK_COLOR: {
			GOColor color;
			char *str;
			if (color_completed)
				break;
			if (go_format_parse_color (token, &color, NULL, NULL, FALSE)) {
				ODF_CLOSE_STRING;
				gsf_xml_out_start_element (xout, STYLE "text-properties");
				str = g_strdup_printf ("#%.2X%.2X%.2X",
						       GO_COLOR_UINT_R (color), GO_COLOR_UINT_G (color),
						       GO_COLOR_UINT_B (color));
				gsf_xml_out_add_cstr_unchecked (xout, FOSTYLE "color", str);
				g_free (str);
				gsf_xml_out_end_element (xout); /*<style:text-properties>*/
				color_completed = TRUE;
			}
		} break;

		case TOK_GENERAL:
		case TOK_INVISIBLE_CHAR:
		case TOK_REPEATED_CHAR:
		case TOK_CONDITION:
		case TOK_LOCALE:
			break;
		case TOK_ERROR:
			xl++;
			break;

		default:
			ODF_OPEN_STRING;
			g_string_append_c (accum, t);
			break;
		}
	}
}

#undef ODF_CLOSE_STRING
#undef ODF_OPEN_STRING

#define ODF_CLOSE_STRING  if (string_is_open) {  \
                                 gsf_xml_out_add_cstr (xout, NULL, accum->str); \
                                 gsf_xml_out_end_element (xout); /* </number:text> */  \
				 string_is_open = FALSE; \
                          }
#define ODF_OPEN_STRING   if (fraction_in_progress) break;\
	                  if (!string_is_open) { \
	                         gsf_xml_out_start_element (xout, NUMBER "text");\
                                 string_is_open = TRUE; \
				 g_string_erase (accum, 0, -1); \
				 }

static void
go_format_output_fraction_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
				  char const *name,
				  gboolean with_extension)
{
	char const *xl = go_format_as_XL (fmt);
	GString *accum = g_string_new (NULL);

	int int_digits = -1; /* -1 means no integer part */
	int min_numerator_digits = 0;
	int zeroes = 0;

	gboolean fraction_in_progress = FALSE;
	gboolean fraction_completed = FALSE;
	gboolean string_is_open = FALSE;
	gboolean color_completed = FALSE;


	gsf_xml_out_start_element (xout, NUMBER "number-style");
	gsf_xml_out_add_cstr (xout, STYLE "name", name);

	while (1) {
		const char *token = xl;
		GOFormatTokenType tt;
		int t = go_format_token (&xl, &tt);

		switch (t) {
		case 0: case ';':
			ODF_CLOSE_STRING;
			if (!fraction_completed) {
				/* We need a fraction element */
				gsf_xml_out_start_element (xout, NUMBER "fraction");
				odf_add_bool (xout, NUMBER "grouping", FALSE);
				gsf_xml_out_add_int (xout, NUMBER "min-denominator-digits", 3);
				if (int_digits >= 0)
					gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", int_digits);
				else
#ifdef HAVE_GET_GSF_ODF_VERSION
					if (get_gsf_odf_version () < 102)
#endif
						{
							gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", 0);
							if (with_extension)
								gsf_xml_out_add_cstr_unchecked
									(xout, GNMSTYLE "no-integer-part", "true");
							
						}
				/* In ODF1.2, absence of NUMBER "min-integer-digits" means not to show an       */
				/* integer part. In ODF 1.1 we used a foreign element: gnm:no-integer-part=true */
				gsf_xml_out_add_int (xout, NUMBER "min-numerator-digits", 1);
				gsf_xml_out_end_element (xout); /* </number:fraction> */
			}
			gsf_xml_out_end_element (xout); /* </number:number-style> */
			g_string_free (accum, TRUE);
			return;

		case '0':
			zeroes = 1;
			/* fall through */
		case '#': case '?': {
			int i = 1;
			if (fraction_completed) {
				zeroes = 0;
				break;
			}
			ODF_CLOSE_STRING;
                        /* ODF allows only for a single fraction specification */
			fraction_in_progress = TRUE;
			while (*xl == '0' || *xl == '?' ||*xl == '#') {
				if (*xl == '0') zeroes++;
				xl++; i++;
			}
			if (zeroes > 0 && *(xl - 1) != '0') zeroes++;
			if (*xl == '/')
				min_numerator_digits = zeroes;
			else
				int_digits = zeroes;
			zeroes = 0;
			break;
		}

		case '/': {
			int fixed_denominator;
			int digits = 0;
			int i = 0;

			if (fraction_completed) break;
                        /* ODF allows only for a single fraction specification */
			ODF_CLOSE_STRING;

			fixed_denominator = atoi (xl);
			while (g_ascii_isdigit (*xl) || *xl == '?' ||*xl == '#') {
				if (*xl == '0') zeroes++;
				if (g_ascii_isdigit (*xl)) digits ++;
				xl++; i++;
			}
			if (!g_ascii_isdigit (*(xl - 1)))
				zeroes++;

			gsf_xml_out_start_element (xout, NUMBER "fraction");
			odf_add_bool (xout, NUMBER "grouping", FALSE);
			if ((fixed_denominator > 0) && (digits == i)) {
				gsf_xml_out_add_int (xout, NUMBER "denominator-value",
						     fixed_denominator);
				gsf_xml_out_add_int (xout, NUMBER "min-denominator-digits", digits);
			} else
				gsf_xml_out_add_int (xout, NUMBER "min-denominator-digits", zeroes);
			if (with_extension)
				gsf_xml_out_add_int (xout, GNMSTYLE "max-denominator-digits", i);

			if (int_digits >= 0)
				gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", int_digits);
			else
#ifdef HAVE_GET_GSF_ODF_VERSION
				if (get_gsf_odf_version () < 102)
#endif
					{
						gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", 0);
						if (with_extension)
							gsf_xml_out_add_cstr_unchecked
								(xout, GNMSTYLE "no-integer-part", "true");

					}
			/* In ODF1.2, absence of NUMBER "min-integer-digits" means not to show an       */
			/* integer part. In ODF 1.1 we used a foreign element: gnm:no-integer-part=true */
			gsf_xml_out_add_int (xout, NUMBER "min-numerator-digits",
					     min_numerator_digits);
			gsf_xml_out_end_element (xout); /* </number:fraction> */
			fraction_completed = TRUE;
			fraction_in_progress = FALSE;
			break;
		}

		case TOK_COLOR: {
			GOColor color;
			char *str;
			if (color_completed)
				break;
			if (go_format_parse_color (token, &color, NULL, NULL, FALSE)) {
				ODF_CLOSE_STRING;
				gsf_xml_out_start_element (xout, STYLE "text-properties");
				str = g_strdup_printf ("#%.2X%.2X%.2X",
						       GO_COLOR_UINT_R (color), GO_COLOR_UINT_G (color),
						       GO_COLOR_UINT_B (color));
				gsf_xml_out_add_cstr_unchecked (xout, FOSTYLE "color", str);
				g_free (str);
				gsf_xml_out_end_element (xout); /*<style:text-properties>*/
				color_completed = TRUE;
			}
		} break;

		case 'd': case 'D':
		case 'y': case 'Y':
		case 'b': case 'B':
		case 'e':
		case 'g': case 'G':
		case 'h': case 'H':
		case 'm': case 'M':
		case 's': case 'S':
		case TOK_AMPM3:
		case TOK_AMPM5:
		case TOK_ELAPSED_H:
		case TOK_ELAPSED_M:
		case TOK_ELAPSED_S:
		case TOK_GENERAL:
		case TOK_INVISIBLE_CHAR:
		case TOK_REPEATED_CHAR:
		case TOK_CONDITION:
		case TOK_LOCALE:
		case TOK_ERROR:
			break;

		case TOK_STRING: {
			size_t len = strchr (token + 1, '"') - (token + 1);
			if (len > 0) {
				ODF_OPEN_STRING;
				g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		case TOK_CHAR: {
			size_t len = g_utf8_next_char(token) - (token);
			if (len > 0) {
				ODF_OPEN_STRING;
				g_string_append_len (accum, token, len);
			}
			break;
		}

		case TOK_ESCAPED_CHAR: {
			size_t len = g_utf8_next_char(token + 1) - (token + 1);
			if (len > 0) {
				ODF_OPEN_STRING;
				g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		case TOK_THOUSAND:
			ODF_OPEN_STRING;
			g_string_append_c (accum, ',');
			break;

		case TOK_DECIMAL:
			ODF_OPEN_STRING;
			g_string_append_c (accum, '.');
			break;

		default:
			ODF_OPEN_STRING;
			g_string_append_c (accum, t);
			break;
		}
	}
}

#undef ODF_CLOSE_STRING
#undef ODF_OPEN_STRING

#define ODF_CLOSE_STRING  if (string_is_open) {				\
		gsf_xml_out_add_cstr (xout, NULL, accum->str);		\
		gsf_xml_out_end_element (xout); /* </number:text> */	\
		string_is_open = FALSE;					\
	}
#define ODF_OPEN_STRING   if (!string_is_open) {			\
		gsf_xml_out_start_element (xout, NUMBER "text");	\
		string_is_open = TRUE;					\
		g_string_erase (accum, 0, -1);				\
	}
#define ODF_WRITE_NUMBER  if (number_seen && !number_completed) {	\
		go_format_output_number_element_to_odf			\
			(xout, min_integer_digits, min_decimal_digits,	\
			 comma_seen, cond_part);			\
		number_completed = TRUE;				\
	}

static void
go_format_output_number_element_to_odf (GsfXMLOut *xout,
					int min_integer_digits,
					int min_decimal_digits,
					gboolean comma_seen,
					int cond_part)
{
	gsf_xml_out_start_element (xout, NUMBER "number");
	gsf_xml_out_add_int (xout, NUMBER "decimal-places", min_decimal_digits);
	gsf_xml_out_add_int (xout, NUMBER "display-factor", (cond_part > 0) ? -1 : 1);
	odf_add_bool (xout, NUMBER "grouping", comma_seen);
	gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", min_integer_digits);
	gsf_xml_out_end_element (xout); /* </number:number> */
}

static void
go_format_output_number_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
				GOFormatFamily family,
				char const *name, int cond_part)
{
	char const *xl = go_format_as_XL (fmt);
	GString *accum = g_string_new (NULL);

	int min_integer_digits = 0;
	int min_decimal_digits = 0;
	gboolean comma_seen = FALSE;
	gboolean dot_seen = FALSE;

	gboolean number_completed = FALSE;
	gboolean number_seen = FALSE;
	gboolean color_completed = FALSE;
	gboolean string_is_open = FALSE;

	switch (family) {
	case GO_FORMAT_PERCENTAGE:
		gsf_xml_out_start_element (xout, NUMBER "percentage-style");
		break;
	case GO_FORMAT_CURRENCY:
		gsf_xml_out_start_element (xout, NUMBER "currency-style");
		break;
	case GO_FORMAT_NUMBER:
	default:
		gsf_xml_out_start_element (xout, NUMBER "number-style");
		break;
	}

	gsf_xml_out_add_cstr (xout, STYLE "name", name);

	while (1) {
		const char *token = xl;
		GOFormatTokenType tt;
		int t = go_format_token (&xl, &tt);

		switch (t) {
		case 0: case ';':
			ODF_CLOSE_STRING;
			number_seen = TRUE; /* since we need a number element */
			ODF_WRITE_NUMBER;
			gsf_xml_out_end_element (xout); /* </number:number-style> */
			g_string_free (accum, TRUE);
			return;

		case TOK_DECIMAL:
			if (number_completed) break;
                        /* ODF allows only for a single number specification */
			ODF_CLOSE_STRING;
			number_seen = TRUE;
			dot_seen = TRUE;
			break;

		case TOK_THOUSAND:
			if (number_completed) break;
                        /* ODF allows only for a single number specification */
			ODF_CLOSE_STRING;
			number_seen = TRUE;
			comma_seen = TRUE;
			break;

		case '0':
			if (number_completed) break;
                        /* ODF allows only for a single number specification */
			ODF_CLOSE_STRING;
			number_seen = TRUE;
			if (dot_seen)
				min_decimal_digits++;
			else
				min_integer_digits++;
			break;

		case '#':
		case '?':
			if (number_completed) break;
                        /* ODF allows only for a single number specification */
			ODF_CLOSE_STRING;
			number_seen = TRUE;
			break;

		case TOK_COLOR: {
			GOColor color;
			char *str;
			if (color_completed)
				break;
			if (go_format_parse_color (token, &color, NULL, NULL, FALSE)) {
				ODF_CLOSE_STRING;
				ODF_WRITE_NUMBER;
				gsf_xml_out_start_element (xout, STYLE "text-properties");
				str = g_strdup_printf ("#%.2X%.2X%.2X",
						       GO_COLOR_UINT_R (color), GO_COLOR_UINT_G (color),
						       GO_COLOR_UINT_B (color));
				gsf_xml_out_add_cstr_unchecked (xout, FOSTYLE "color", str);
				g_free (str);
				gsf_xml_out_end_element (xout); /*<style:text-properties>*/
				color_completed = TRUE;
			}
		} break;

		case '$':
			ODF_WRITE_NUMBER;
			if (family == GO_FORMAT_CURRENCY) {
				ODF_CLOSE_STRING;
				gsf_xml_out_simple_element(xout, NUMBER "currency-symbol", "$");
			}
			break;

		case '/':
		case 'd': case 'D':
		case 'y': case 'Y':
		case 'b': case 'B':
		case 'e': case 'E':
		case 'g': case 'G':
		case 'h': case 'H':
		case 'm': case 'M':
		case 's': case 'S':
		case TOK_AMPM3:
		case TOK_AMPM5:
		case TOK_ELAPSED_H:
		case TOK_ELAPSED_M:
		case TOK_ELAPSED_S:
		case TOK_GENERAL:
		case TOK_INVISIBLE_CHAR:
		case TOK_REPEATED_CHAR:
		case TOK_CONDITION:
		case TOK_ERROR:
			ODF_WRITE_NUMBER;
			break;

		case TOK_STRING: {
			size_t len = strchr (token + 1, '"') - (token + 1);
			if (len > 0) {
				ODF_WRITE_NUMBER;
				ODF_OPEN_STRING;
				g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		case TOK_CHAR: {
			size_t len = g_utf8_next_char(token) - (token);
			if (len > 0) {
				gunichar uc;
				ODF_WRITE_NUMBER;
				if ((family == GO_FORMAT_CURRENCY) &&
				    ((uc = g_utf8_get_char (token)) == UNICODE_POUNDS1
				     || uc == UNICODE_POUNDS2 || uc == UNICODE_YEN
				     || uc == UNICODE_YEN_WIDE || uc == UNICODE_EURO)) {
					char *str;
					ODF_CLOSE_STRING;
					str = g_strndup (token, len);
					gsf_xml_out_simple_element(xout, NUMBER "currency-symbol", str);
					g_free (str);
				} else {
					ODF_OPEN_STRING;
					if (*token == '-')
						g_string_append_unichar (accum, UNICODE_MINUS);
					else
						g_string_append_len (accum, token, len);
				}
			}
			break;
		}

		case TOK_LOCALE:
			ODF_WRITE_NUMBER;
			if ((family == GO_FORMAT_CURRENCY) && (*token == '[') && (*(token + 1) == '$')) {
				int len;
				token += 2;
				len = strcspn (token, "-]");
				if (len > 0) {
					char *str;
					ODF_CLOSE_STRING;
					str = g_strndup (token, len);
					gsf_xml_out_simple_element(xout, NUMBER "currency-symbol", str);
					g_free (str);
				}
			}
			break;

		case TOK_ESCAPED_CHAR: {
			size_t len = g_utf8_next_char(token + 1) - (token + 1);
			if (len > 0) {
				ODF_WRITE_NUMBER;
				ODF_OPEN_STRING;
				if (*(token+1) == '-')
					g_string_append_unichar (accum, UNICODE_MINUS);
				else
					g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		default:
			ODF_WRITE_NUMBER;
			ODF_OPEN_STRING;
			g_string_append_c (accum, t);
			break;
		}
	}
}

#undef ODF_CLOSE_STRING
#undef ODF_OPEN_STRING

#define ODF_CLOSE_STRING  if (string_is_open) {				\
		gsf_xml_out_add_cstr (xout, NULL, accum->str);		\
		gsf_xml_out_end_element (xout); /* </number:text> */	\
		string_is_open = FALSE;					\
	}
#define ODF_OPEN_STRING   if (!string_is_open) {			\
		gsf_xml_out_start_element (xout, NUMBER "text");	\
		string_is_open = TRUE;					\
		g_string_erase (accum, 0, -1);				\
	}

static void
go_format_output_text_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
				char const *name, int cond_part)
{
	char const *xl = go_format_as_XL (fmt);
	GString *accum = g_string_new (NULL);

	gboolean string_is_open = FALSE;

	gsf_xml_out_start_element (xout, NUMBER "text-style");

	gsf_xml_out_add_cstr (xout, STYLE "name", name);

	while (1) {
		const char *token = xl;
		GOFormatTokenType tt;
		int t = go_format_token (&xl, &tt);

		switch (t) {
		case 0: case ';':
			ODF_CLOSE_STRING;
			gsf_xml_out_end_element (xout); /* </number:text-style> */
			g_string_free (accum, TRUE);
			return;

		case TOK_COLOR: {
			GOColor color;
			char *str;
			if (go_format_parse_color (token, &color, NULL, NULL, FALSE)) {
				ODF_CLOSE_STRING;
				gsf_xml_out_start_element (xout, STYLE "text-properties");
				str = g_strdup_printf ("#%.2X%.2X%.2X",
						       GO_COLOR_UINT_R (color), GO_COLOR_UINT_G (color),
						       GO_COLOR_UINT_B (color));
				gsf_xml_out_add_cstr_unchecked (xout, FOSTYLE "color", str);
				g_free (str);
				gsf_xml_out_end_element (xout); /*<style:text-properties>*/
			}
		} break;

		case TOK_STRING: {
			size_t len = strchr (token + 1, '"') - (token + 1);
			if (len > 0) {
				ODF_OPEN_STRING;
				g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		case TOK_CHAR: {
			size_t len = g_utf8_next_char(token) - (token);
			if (len > 0) {
				ODF_OPEN_STRING;
				if (*token == '-')
					g_string_append_unichar (accum, UNICODE_MINUS);
				else
					g_string_append_len (accum, token, len);
			}
			break;
		}

		case TOK_ESCAPED_CHAR: {
			size_t len = g_utf8_next_char(token + 1) - (token + 1);
			if (len > 0) {
				ODF_OPEN_STRING;
				if (*(token+1) == '-')
					g_string_append_unichar (accum, UNICODE_MINUS);
				else
					g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		case '@':
			ODF_CLOSE_STRING;
			gsf_xml_out_simple_element (xout, NUMBER "text-content", NULL);
			break;

		default:
			ODF_OPEN_STRING;
			g_string_append_c (accum, t);
			break;
		}
	}
}

#undef ODF_CLOSE_STRING
#undef ODF_OPEN_STRING

#define ODF_CLOSE_STRING  if (string_is_open) {				\
		gsf_xml_out_add_cstr (xout, NULL, accum->str);		\
		gsf_xml_out_end_element (xout); /* </number:text> */	\
		string_is_open = FALSE;					\
	}
#define ODF_OPEN_STRING   if (!string_is_open) {			\
		gsf_xml_out_start_element (xout, NUMBER "text");	\
		string_is_open = TRUE;					\
		g_string_erase (accum, 0, -1);				\
	}

static void
go_format_output_scientific_number_element_to_odf (GsfXMLOut *xout,
						   int min_integer_digits,
						   int min_decimal_digits,
						   int min_exponent_digits,
						   gboolean comma_seen,
						   gboolean engineering)
{
	gsf_xml_out_start_element (xout, NUMBER "scientific-number");
	gsf_xml_out_add_int (xout, NUMBER "decimal-places", min_decimal_digits);
	odf_add_bool (xout, NUMBER "grouping", comma_seen);
	gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", min_integer_digits);
	gsf_xml_out_add_int (xout, NUMBER "min-exponent-digits", min_exponent_digits);
	if (engineering)
		odf_add_bool (xout, GNMSTYLE "engineering", TRUE);
	gsf_xml_out_end_element (xout); /* </number:number> */
}

static void
go_format_output_scientific_number_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
					   char const *name,
					   gboolean with_extension)
{
	char const *xl = go_format_as_XL (fmt);
	GString *accum = g_string_new (NULL);

	int min_integer_digits = 0;
	int min_decimal_digits = 0;
	int min_exponent_digits = 0;
	int hashes = 0;
	gboolean comma_seen = FALSE;
	gboolean dot_seen = FALSE;

	gboolean number_completed = FALSE;
	gboolean color_completed = FALSE;
	gboolean string_is_open = FALSE;

	gsf_xml_out_start_element (xout, NUMBER "number-style");
	gsf_xml_out_add_cstr (xout, STYLE "name", name);

	while (1) {
		const char *token = xl;
		GOFormatTokenType tt;
		int t = go_format_token (&xl, &tt);

		switch (t) {
		case 0: case ';':
			ODF_CLOSE_STRING;
			gsf_xml_out_end_element (xout); /* </number:number-style> */
			g_string_free (accum, TRUE);
			return;

		case TOK_DECIMAL:
			if (number_completed) break;
                        /* ODF allows only for a single number specification */
			ODF_CLOSE_STRING;
			dot_seen = TRUE;
			break;

		case TOK_THOUSAND:
			if (number_completed) break;
                        /* ODF allows only for a single number specification */
			ODF_CLOSE_STRING;
			comma_seen = TRUE;
			break;

		case '0':
			if (number_completed) break;
                        /* ODF allows only for a single number specification */
			ODF_CLOSE_STRING;
			if (dot_seen)
				min_decimal_digits++;
			else
				min_integer_digits++;
			break;

		case '?':
			if (number_completed) break;
                        /* ODF allows only for a single number specification */
			ODF_CLOSE_STRING;
			break;

		case '#':
			if (number_completed) break;
                        /* ODF allows only for a single number specification */
			ODF_CLOSE_STRING;
			if (!dot_seen)
				hashes++;
			break;

		case 'e':
		case 'E':
			if (number_completed)
				break;
			while (*xl == '0' || *xl == '+' || *xl == '-')
				if (*xl++ == '0')
					min_exponent_digits++;
			go_format_output_scientific_number_element_to_odf
				(xout, min_integer_digits, min_decimal_digits,
				 min_exponent_digits, comma_seen,
				 with_extension && hashes > 0 && (hashes + min_integer_digits == 3));
			number_completed = TRUE;
			break;

		case TOK_COLOR: {
			GOColor color;
			char *str;
			if (color_completed)
				break;
			if (go_format_parse_color (token, &color, NULL, NULL, FALSE)) {
				ODF_CLOSE_STRING;
				gsf_xml_out_start_element (xout, STYLE "text-properties");
				str = g_strdup_printf ("#%.2X%.2X%.2X",
						       GO_COLOR_UINT_R (color), GO_COLOR_UINT_G (color),
						       GO_COLOR_UINT_B (color));
				gsf_xml_out_add_cstr_unchecked (xout, FOSTYLE "color", str);
				g_free (str);
				gsf_xml_out_end_element (xout); /*<style:text-properties>*/
				color_completed = TRUE;
			}
		} break;

		case '/':
		case 'd': case 'D':
		case 'y': case 'Y':
		case 'b': case 'B':
		case 'g': case 'G':
		case 'h': case 'H':
		case 'm': case 'M':
		case 's': case 'S':
		case TOK_AMPM3:
		case TOK_AMPM5:
		case TOK_ELAPSED_H:
		case TOK_ELAPSED_M:
		case TOK_ELAPSED_S:
		case TOK_GENERAL:
		case TOK_INVISIBLE_CHAR:
		case TOK_REPEATED_CHAR:
		case TOK_CONDITION:
		case TOK_LOCALE:
		case TOK_ERROR:
			break;

		case TOK_STRING: {
			size_t len = strchr (token + 1, '"') - (token + 1);
			if (len > 0) {
				ODF_OPEN_STRING;
				g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		case TOK_CHAR: {
			size_t len = g_utf8_next_char(token) - (token);
			if (len > 0) {
				ODF_OPEN_STRING;
				if (*token == '-')
					g_string_append_unichar (accum, UNICODE_MINUS);
				else
					g_string_append_len (accum, token, len);
			}
			break;
		}

		case TOK_ESCAPED_CHAR: {
			size_t len = g_utf8_next_char(token + 1) - (token + 1);
			if (len > 0) {
				ODF_OPEN_STRING;
				if (*(token+1) == '-')
					g_string_append_unichar (accum, UNICODE_MINUS);
				else
					g_string_append_len (accum, token + 1, len);
			}
			break;
		}

		default:
			ODF_OPEN_STRING;
			g_string_append_c (accum, t);
			break;
		}
	}
}

#undef ODF_CLOSE_STRING
#undef ODF_OPEN_STRING

static void
go_format_output_general_to_odf (GsfXMLOut *xout, char const *name, int cond_part)
{
	gsf_xml_out_start_element (xout, NUMBER "number-style");
	gsf_xml_out_add_cstr (xout, STYLE "name", name);
	if (cond_part == 1)
		gsf_xml_out_simple_element(xout, NUMBER "text", "\xe2\x88\x92");
	gsf_xml_out_start_element (xout, NUMBER "number");
	gsf_xml_out_add_int (xout, NUMBER "decimal-places", 2);
	if (cond_part == 1)
		gsf_xml_out_add_int (xout, NUMBER "display-factor",  -1);
	gsf_xml_out_end_element (xout); /* </number:number> */
	gsf_xml_out_end_element (xout); /* </number:number-style> */
}
#endif

#ifdef DEFINE_COMMON
gboolean
go_format_output_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
			 int cond_part, char const *name,
			 gboolean with_extension)
{
	gboolean pp = TRUE, result = TRUE;
	GOFormatDetails details;
	gboolean exact;
	GOFormat const *act_fmt, *det_fmt = fmt;
	GOFormatCondition *condition = NULL;
	GOFormatFamily family;

	if (fmt == NULL)
			return FALSE;

	if (fmt->typ == GO_FMT_COND) {
		g_return_val_if_fail (cond_part <= fmt->u.cond.n, FALSE);
		condition = &(fmt->u.cond.conditions[cond_part]);
		act_fmt = condition->fmt;
	} else {
		g_return_val_if_fail (cond_part == 0, FALSE);
		act_fmt = fmt;
	}

	g_object_get (G_OBJECT (xout), "pretty-print", &pp, NULL);
	/* We need to switch off pretty printing since number:text preserves whitespace */
	g_object_set (G_OBJECT (xout), "pretty-print", FALSE, NULL);

	family = go_format_get_family (fmt);
	if (family == GO_FORMAT_UNKNOWN) {
		family = go_format_get_family (act_fmt);
		det_fmt = act_fmt;
	}
	go_format_get_details (det_fmt, &details, &exact);
	family = details.family;

	switch (family) {
	case GO_FORMAT_GENERAL:
		go_format_output_general_to_odf (xout, name, cond_part);
		result = FALSE;
		break;
	case GO_FORMAT_DATE:
		go_format_output_date_to_odf (xout, act_fmt, name, family, with_extension);
		break;
	case GO_FORMAT_TIME:
		go_format_output_date_to_odf (xout, act_fmt, name, family, with_extension);
		break;
	case GO_FORMAT_FRACTION:
		go_format_output_fraction_to_odf (xout, act_fmt, name, with_extension);
		break;
	case GO_FORMAT_TEXT:
		go_format_output_text_to_odf (xout, act_fmt, name, with_extension);
		break;
	case GO_FORMAT_SCIENTIFIC:
		go_format_output_scientific_number_to_odf (xout, act_fmt, name, with_extension);
		break;
	case GO_FORMAT_ACCOUNTING:
		family = GO_FORMAT_CURRENCY;
		/* no break;   fall through */
	case GO_FORMAT_CURRENCY:
	case GO_FORMAT_PERCENTAGE:
	case GO_FORMAT_NUMBER:
		go_format_output_number_to_odf (xout, act_fmt, family, name, cond_part);
		break;
	default: {
		/* We need to output something and we don't need any details for this */
		int date = 0, digit = 0;
		char const *str = go_format_as_XL (fmt);
		while (*str != '\0') {
			switch (*str) {
			case 'd': case 'm': case 'y': case 'h': case 's':
				date++;
				break;
			case '#': case '.': case '0': case 'e':
				digit++;
				break;
			default:
				break;
			}
			str++;
		}
		if (digit < date)
			go_format_output_date_to_odf (xout, act_fmt, name,
						      GO_FORMAT_DATE, with_extension);
		else
			go_format_output_general_to_odf (xout, name, cond_part);
		result = FALSE;
		break;
	}
	}

	g_object_set (G_OBJECT (xout), "pretty-print", pp, NULL);

	return result;
}
#endif
