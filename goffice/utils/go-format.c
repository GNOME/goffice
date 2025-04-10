/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-format.c :
 *
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2005-2023 Morten Welinder (terra@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
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

#include <goffice/goffice-config.h>
#include <gsf/gsf-msole-utils.h>
#include <gsf/gsf-opendoc-utils.h>
#include "go-format.h"
#include "go-locale.h"
#include "go-font.h"
#include "go-color.h"
#include "datetime.h"
#include "go-glib-extras.h"
#include "go-pango-extras.h"
#include <goffice/math/go-math.h>
#include <glib/gi18n-lib.h>

#include <time.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

// We need multiple versions of this code.  We're going to include ourself
// with different settings of various macros.  gdb will hate us.
#include <goffice/goffice-multipass.h>
#ifndef SKIP_THIS_PASS

#undef DEBUG_GENERAL

/**
 * GOFormatFamily:
 * @GO_FORMAT_UNKNOWN: unknown, should not occur.
 * @GO_FORMAT_GENERAL: general.
 * @GO_FORMAT_NUMBER: number.
 * @GO_FORMAT_CURRENCY: currency.
 * @GO_FORMAT_ACCOUNTING: accounting.
 * @GO_FORMAT_DATE: date.
 * @GO_FORMAT_TIME: tipe.
 * @GO_FORMAT_PERCENTAGE: percentage.
 * @GO_FORMAT_FRACTION: fraction.
 * @GO_FORMAT_SCIENTIFIC: scientific.
 * @GO_FORMAT_TEXT: text.
 * @GO_FORMAT_SPECIAL: custom.
 **/

/**
 * GOFormatMagic:
 * @GO_FORMAT_MAGIC_NONE: none.
 * @GO_FORMAT_MAGIC_LONG_DATE: long date (Official).
 * @GO_FORMAT_MAGIC_MEDIUM_DATE: medium date.
 * @GO_FORMAT_MAGIC_SHORT_DATE: short date.
 * @GO_FORMAT_MAGIC_SHORT_DATETIME: short date with time.
 * @GO_FORMAT_MAGIC_LONG_TIME: long time (Official).
 * @GO_FORMAT_MAGIC_MEDIUM_TIME: medium time.
 * @GO_FORMAT_MAGIC_SHORT_TIME: short time.
 **/

/**
 * GOFormatNumberError:
 * @GO_FORMAT_NUMBER_OK: no error.
 * @GO_FORMAT_NUMBER_INVALID_FORMAT: invalid format.
 * @GO_FORMAT_NUMBER_DATE_ERROR: date error.
 **/

/**
 * GOFormatCurrency:
 * @symbol: currency symbol.
 * @description: description.
 * @precedes: whether the symbol precedes the amount.
 * @has_space: whether to add a space between amount and symbol.
 **/

/**
 * GOFormatDetails:
 * @family: #GOFormatFamily.
 * @magic: #GOFormatMagic.
 * @min_digits: minimum digits number.
 * @num_decimals: decimals number.
 * @thousands_sep: thousands separator.
 * @negative_red: display negative number using red ink.
 * @negative_paren: uses parenthersis around negative numbers.
 * @currency: #GOFormatCurrency.
 * @force_quoted: force quotes use.
 * @exponent_step: steps between allowed exponents in scientific notation.
 * @exponent_digits: digits number in exponent.
 * @exponent_sign_forced: whether the sign in the exponent is always shown.
 * @use_markup: whether to use a markup.
 * @simplify_mantissa: simplify the mantissa.
 * @append_SI: append an SI unit.
 * @appended_SI_unit: the SI unit to append.
 * @scale: scale.
 * @automatic_denominator: use an automatic denominator for fractions.
 * @split_fraction: split the fraction.
 * @pi_scale: use multiples of pi for fractions, e.g. 1/2*pi.
 * @numerator_min_digits: minimum digits number for the numerator.
 * @denominator_min_digits: minimum digits number for the denominator.
 * @denominator_max_digits: minimum digits number for the denominator.
 * @denominator: fixed denominator.
 **/

#define OBSERVE_XL_CONDITION_LIMITS
#define OBSERVE_XL_EXPONENT_1
#define ALLOW_NEGATIVE_TIMES
#define MAX_DECIMALS 100

// Define ALLOW_DENOM_REMOVAL to remove /1s. This is not XL compatible.
#undef ALLOW_DENOM_REMOVAL

// Define ALLOW_NO_SIGN_AFTER_E to permit formats such as '00E00' and '00E +00'
#define ALLOW_NO_SIGN_AFTER_E

#define ALLOW_EE_MARKUP
#define ALLOW_SI_APPEND
#define ALLOW_PI_SLASH

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
	OP_NUMERAL_SHAPE,	/* flags shape */
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
	OP_NUM_STORE_POS,
#ifdef ALLOW_EE_MARKUP
	OP_NUM_MARK_MANTISSA,
	OP_NUM_SIMPLIFY_MANTISSA,
	OP_NUM_SIMPLIFY_MARKUP_MANTISSA,
	OP_MARKUP_SUPERSCRIPT_START,
	OP_MARKUP_SUPERSCRIPT_END,
#endif
#ifdef ALLOW_SI_APPEND
	OP_NUM_SIMPLIFY_MANTISSA_SI,
	OP_NUM_REDUCE_EXPONENT_SI,
	OP_NUM_SIMPLIFY_EXPONENT_SI,
	OP_NUM_SI_EXPONENT,
#endif

	OP_NUM_FRACTION,	/* wholep explicitp (digits|denominator) */
	OP_NUM_FRACTION_WHOLE,
	OP_NUM_FRACTION_NOMINATOR,
	OP_NUM_FRACTION_DENOMINATOR,
	OP_NUM_FRACTION_BLANK,
	OP_NUM_FRACTION_BLANK_WHOLE,
	OP_NUM_FRACTION_ALIGN,
	OP_NUM_FRACTION_SLASH,
	OP_NUM_FRACTION_SIGN,
#ifdef ALLOW_DENOM_REMOVAL
	OP_NUM_FRACTION_SIMPLIFY,
	OP_NUM_FRACTION_SIMPLIFY_NUMERATOR,
#endif
#ifdef ALLOW_PI_SLASH
	OP_NUM_FRACTION_BLANK_PI,
	OP_NUM_FRACTION_SCALE_PI,
	OP_NUM_FRACTION_SIMPLIFY_PI,
	OP_NUM_FRACTION_SIMPLIFY_NUMERATOR_PI,
	OP_NUM_FRACTION_PI_SUM_START,
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

	TOK_EXP,                /* E */
#ifdef ALLOW_EE_MARKUP
	TOK_EXP_MU,             /* EE */
#endif
#ifdef ALLOW_SI_APPEND
	TOK_EXP_SI,             /* ESI */
#endif
#if defined(ALLOW_EE_MARKUP) && defined(ALLOW_SI_APPEND)
	TOK_EXP_MU_SI,          /* EESI */
#endif

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

typedef enum {
	GO_FMT_SHAPE_SIGNS = 1,
	/* GENERAL implies GO_FMT_POSITION_MARKERS whether this is set or not */
	/* This is only useful for Chinese/Japanese/Korean numerals           */
	GO_FMT_POSITION_MARKERS = 2
} GOFormatShapeFlags;

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
	guint64 locale;
} GOFormatLocale;


struct _GOFormat {
	unsigned int typ : 8;
	unsigned int ref_count : 24;
	GOColor color;
	unsigned int has_fill : 7;
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
			unsigned int use_SI      : 1;
			unsigned int has_date    : 1;
			unsigned int date_ybm    : 1;  /* year, then month.  */
			unsigned int date_mbd    : 1;  /* month, then day.  */
			unsigned int date_dbm    : 1;  /* day, then month.  */
			unsigned int has_year    : 1;
			unsigned int has_month   : 1;
			unsigned int has_day     : 1;
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
#define UNICODE_PI 0x1d70b  /* mathematical small italic pi */
#define UNICODE_PI_number_of_bytes 4
#define UNICODE_THINSPACE 0x2009
#define UNICODE_TIMES 0x00D7
#define UNICODE_MINUS 0x2212
#define UNICODE_EURO 0x20ac
#define UNICODE_POUNDS1 0x00a3
#define UNICODE_POUNDS2 0x20a4
#define UNICODE_YEN 0x00a5
#define UNICODE_YEN_WIDE 0xffe5

#define UTF8_MINUS "\xe2\x88\x92"
#define UTF8_FULLWIDTH_MINUS "\357\274\215"
#define UTF8_FULLWIDTH_PLUS  "\357\274\213"

/* Number of digits required for roundtrip of DOUBLE.  */
#ifdef DEFINE_COMMON
static int go_format_roundtrip_digits;
#ifdef GOFFICE_WITH_LONG_DOUBLE
static int go_format_roundtrip_digitsl;
#endif
#ifdef GOFFICE_WITH_DECIMAL64
static const int go_format_roundtrip_digitsD = 16;
#endif
#endif

gboolean
go_format_allow_ee_markup (void)
{
#ifdef ALLOW_EE_MARKUP
	return TRUE;
#else
	return FALSE;
#endif
}

gboolean
go_format_allow_si (void)
{
#ifdef ALLOW_SI_APPEND
	return TRUE;
#else
	return FALSE;
#endif
}

gboolean
go_format_allow_pi_slash (void)
{
#ifdef ALLOW_PI_SLASH
	return TRUE;
#else
	return FALSE;
#endif
}


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
		/* xgettext: See http://www.gnumeric.org/date-time-formats.shtml */
		const char *fmt = _("*Long Date Format");
		if (fmt[0] && fmt[0] != '*')
			return g_strdup (fmt);
		return g_strdup ("dddd, mmmm dd, yyyy");
	}

	case GO_FORMAT_MAGIC_MEDIUM_DATE: {
		/* xgettext: See http://www.gnumeric.org/date-time-formats.shtml */
		const char *fmt = _("*Medium Date Format");
		if (fmt[0] && fmt[0] != '*')
			return g_strdup (fmt);
		return g_strdup ("d-mmm-yyyy");  /* Excel has yy only.  */
	}

	case GO_FORMAT_MAGIC_SHORT_DATE: {
		/* xgettext: See http://www.gnumeric.org/date-time-formats.shtml */
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
		/* xgettext: See http://www.gnumeric.org/date-time-formats.shtml */
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
		/* xgettext: See http://www.gnumeric.org/date-time-formats.shtml */
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
		/* xgettext: See http://www.gnumeric.org/date-time-formats.shtml */
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
		/* xgettext: See http://www.gnumeric.org/date-time-formats.shtml */
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

/***************************************************************************/

/* WARNING : Global */
static GHashTable *style_format_hash = NULL;

/**
 * go_format_foreach:
 * @func: (scope call): function to execute for each known format
 * @user_data: user data for @func
 *
 * Executes @func for each registered #GOFormat
 **/
void
go_format_foreach (GHFunc func, gpointer user_data)
{
	if (style_format_hash != NULL)
		g_hash_table_foreach  (style_format_hash,
				       func, user_data);
}

/* used to generate formats when delocalizing so keep the leadings caps */
static struct {
	char const *name;
	GOColor	go_color;
} const format_colors[8] = {
	{ N_("Black"),	 GO_COLOR_BLACK },   /* Color 1 */
	{ N_("White"),	 GO_COLOR_WHITE },   /* Color 2 */
	{ N_("Red"),	 GO_COLOR_RED },     /* Color 3 */
	{ N_("Green"),	 GO_COLOR_GREEN },   /* Color 4 */
	{ N_("Blue"),	 GO_COLOR_BLUE },    /* Color 5 */
	{ N_("Yellow"),	 GO_COLOR_YELLOW },  /* Color 6 */
	{ N_("Magenta"), GO_COLOR_VIOLET },  /* Color 7 */
	{ N_("Cyan"),	 GO_COLOR_CYAN }    /* Color 8 */
};

static const GOColor
format_numbered_colors[56 + 1] = {
	GO_COLOR_BLACK, /* Invalid */
	GO_COLOR_BLACK,
	GO_COLOR_WHITE,
	GO_COLOR_RED,
	GO_COLOR_GREEN,
	GO_COLOR_BLUE,
	GO_COLOR_YELLOW,
	GO_COLOR_VIOLET,
	GO_COLOR_CYAN,
	GO_COLOR_FROM_RGB (0x80, 0x00, 0x00),
	GO_COLOR_FROM_RGB (0x00, 0x80, 0x00),
	GO_COLOR_FROM_RGB (0x00, 0x00, 0x80),
	GO_COLOR_FROM_RGB (0x80, 0x80, 0x00),
	GO_COLOR_FROM_RGB (0x80, 0x00, 0x80),
	GO_COLOR_FROM_RGB (0x00, 0x80, 0x80),
	GO_COLOR_FROM_RGB (0xC0, 0xC0, 0xC0),
	GO_COLOR_FROM_RGB (0x80, 0x80, 0x80),
	GO_COLOR_FROM_RGB (0x99, 0x99, 0xFF),
	GO_COLOR_FROM_RGB (0x99, 0x33, 0x66),
	GO_COLOR_FROM_RGB (0xFF, 0xFF, 0xCC),
	GO_COLOR_FROM_RGB (0xCC, 0xFF, 0xFF),
	GO_COLOR_FROM_RGB (0x66, 0x00, 0x66),
	GO_COLOR_FROM_RGB (0xFF, 0x80, 0x80),
	GO_COLOR_FROM_RGB (0x00, 0x66, 0xCC),
	GO_COLOR_FROM_RGB (0xCC, 0xCC, 0xFF),
	GO_COLOR_FROM_RGB (0x00, 0x00, 0x80),
	GO_COLOR_FROM_RGB (0xFF, 0x00, 0xFF),
	GO_COLOR_FROM_RGB (0xFF, 0xFF, 0x00),
	GO_COLOR_FROM_RGB (0x00, 0xFF, 0xFF),
	GO_COLOR_FROM_RGB (0x80, 0x00, 0x80),
	GO_COLOR_FROM_RGB (0x80, 0x00, 0x00),
	GO_COLOR_FROM_RGB (0x00, 0x80, 0x80),
	GO_COLOR_FROM_RGB (0x00, 0x00, 0xFF),
	GO_COLOR_FROM_RGB (0x00, 0xCC, 0xFF),
	GO_COLOR_FROM_RGB (0xCC, 0xFF, 0xFF),
	GO_COLOR_FROM_RGB (0xCC, 0xFF, 0xCC),
	GO_COLOR_FROM_RGB (0xFF, 0xFF, 0x99),
	GO_COLOR_FROM_RGB (0x99, 0xCC, 0xFF),
	GO_COLOR_FROM_RGB (0xFF, 0x99, 0xCC),
	GO_COLOR_FROM_RGB (0xCC, 0x99, 0xFF),
	GO_COLOR_FROM_RGB (0xFF, 0xCC, 0x99),
	GO_COLOR_FROM_RGB (0x33, 0x66, 0xFF),
	GO_COLOR_FROM_RGB (0x33, 0xCC, 0xCC),
	GO_COLOR_FROM_RGB (0x99, 0xCC, 0x00),
	GO_COLOR_FROM_RGB (0xFF, 0xCC, 0x00),
	GO_COLOR_FROM_RGB (0xFF, 0x99, 0x00),
	GO_COLOR_FROM_RGB (0xFF, 0x66, 0x00),
	GO_COLOR_FROM_RGB (0x66, 0x66, 0x99),
	GO_COLOR_FROM_RGB (0x96, 0x96, 0x96),
	GO_COLOR_FROM_RGB (0x00, 0x33, 0x66),
	GO_COLOR_FROM_RGB (0x33, 0x99, 0x66),
	GO_COLOR_FROM_RGB (0x00, 0x33, 0x00),
	GO_COLOR_FROM_RGB (0x33, 0x33, 0x00),
	GO_COLOR_FROM_RGB (0x99, 0x33, 0x00),
	GO_COLOR_FROM_RGB (0x99, 0x33, 0x66),
	GO_COLOR_FROM_RGB (0x33, 0x33, 0x99),
	GO_COLOR_FROM_RGB (0x33, 0x33, 0x33)
};

GOColor
go_format_palette_color_of_index (int i)
{
	g_return_val_if_fail (i >= 1, 0);

	return (i < (int)G_N_ELEMENTS (format_numbered_colors))
		? format_numbered_colors[i]
		: 0;
}

char *
go_format_palette_name_of_index (int i)
{
	g_return_val_if_fail (i >= 1, NULL);
	g_return_val_if_fail (i < (int)G_N_ELEMENTS (format_numbered_colors), NULL);

	if (i <= 8)
		return g_strdup (format_colors[i - 1].name);
	else
		return g_strdup_printf ("Color%d", i);
}


static int
color_dist (GOColor c1, GOColor c2)
{
	/* Simple 2-norm in rgb space. */
	int dr = (int)GO_COLOR_UINT_R(c1) - (int)GO_COLOR_UINT_R(c2);
	int dg = (int)GO_COLOR_UINT_G(c1) - (int)GO_COLOR_UINT_G(c2);
	int db = (int)GO_COLOR_UINT_B(c1) - (int)GO_COLOR_UINT_B(c2);

	return dr * dr + dg * dg + db * db;
}

/**
 * go_format_palette_index_from_color:
 * @c: color
 *
 * Returns: the index of the color closest to the argument color in some
 * sense.
 */
int
go_format_palette_index_from_color (GOColor c)
{
	int mindist = G_MAXINT;
	unsigned ui;
	int res = -1;

	for (ui = 1; ui < G_N_ELEMENTS (format_numbered_colors); ui++) {
		GOColor c2 = format_numbered_colors[ui];
		int d = color_dist (c, c2);
		if (d < mindist) {
			mindist = d;
			res = ui;
			if (d == 0)
				break;
		}
	}

	return res;
}

/*
 * go_format_parse_color :
 * @str:
 * @color:
 * @n:
 * @named:
 *
 * Return: TRUE, if ok.  Then @color will be filled in, and @n will be
 * 	a number 0-7 for standard colors.
 *	Returns %FALSE otherwise and @color will be zeroed.
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
		if (end == str || errno == ERANGE || ull > 56 || ull <= 0)
			return FALSE;
		if (n)
			*n = ull;
		if (named)
			*named = FALSE;

		*color = format_numbered_colors[ull];
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
		if (str == end || errno == ERANGE ||
		    ull > G_GINT64_CONSTANT(0x0001ffffffffffffU))
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
		tt = TT_ALLOWED_IN_NUMBER | TT_STARTS_NUMBER;
		break;

	case 'E':
		tt = TT_ALLOWED_IN_NUMBER | TT_STARTS_NUMBER;
#ifdef ALLOW_SI_APPEND
		if (str[1] == 'S' && str[2] == 'I') {
			t = TOK_EXP_SI,
			len = 3;
		} else
#endif
#if defined(ALLOW_EE_MARKUP) && defined(ALLOW_SI_APPEND)
		if (str[1] == 'E' && str[2] == 'S' && str[3] == 'I') {
			t = TOK_EXP_MU_SI;
			len = 4;
		} else
#endif
#ifdef ALLOW_EE_MARKUP
	        if (str[1] == 'E') {
			t = TOK_EXP_MU;
			len = 2;
		} else
#endif
			t = TOK_EXP;
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

		case TOK_EXP:
#ifdef ALLOW_EE_MARKUP
		case TOK_EXP_MU:
#endif
#ifdef ALLOW_SI_APPEND
		case TOK_EXP_SI:
#endif
#if defined(ALLOW_EE_MARKUP) && defined(ALLOW_SI_APPEND)
		case TOK_EXP_MU_SI:
#endif
			ntokens++;
			if (pstate->tno_E >= 0)
				goto error;
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
		guint shape, flags;
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

		if (0 != strcmp (lang, "-none-")) {
			char *lname = NULL;
			oldlocale = g_strdup (setlocale (LC_ALL, NULL));
			ok = setlocale (LC_ALL, lang) != NULL;

			if (!ok) {
				lname = g_strdup_printf ("%s.utf-8",lang);
				lang = lname;
				ok = setlocale (LC_ALL, lang) != NULL;
			}

			setlocale (LC_ALL, oldlocale);
			g_free (oldlocale);

			if (ok) {
				ADD_OP (OP_LOCALE);
				g_string_append_len (prg, (void *)&locale,
						     sizeof (locale));
				/* Include the terminating zero: */
				g_string_append_len (prg, lang, strlen (lang) + 1);
			}
			g_free (lname);
		}
		shape = (locale.locale & G_GINT64_CONSTANT(0x0ff000000U)) >> 24;
		flags = (locale.locale & G_GINT64_CONSTANT(0xf00000000U)) >> 32;
		if (shape != 0 || flags != 0)
			ADD_OP3 (OP_NUMERAL_SHAPE, flags, shape);
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
			fmt->u.number.has_year = seen_year;
			fmt->u.number.has_month = seen_month;
			fmt->u.number.has_day = seen_day;
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
			      int E_part, int frac_part,
			      gboolean *inhibit_blank)
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
	gboolean inhibit_b = FALSE;

	for (i = tno_start; i < tno_end; i++) {
		const GOFormatParseItem *ti = &GET_TOKEN(i);
		if (ti->tt & TT_STARTS_NUMBER)
			break;
		handle_common_token (ti->tstr, ti->token, prg);
	}

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

		case '0':
			inhibit_b = TRUE;
			/* no break */
		case '?': case '#':
			if (first_digit_pos == -1)
				first_digit_pos = i;
			if (whole_part)
				whole_digits++;
			else
				decimals++;
			break;

		case '%':
			if (E_part != 1 && E_part != 2)
 				scale += 2;
 			break;

		case '\'':
			if (E_part == 1)
				scale += 3;
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
		if (scale)
			ADD_OP2 (OP_NUM_SCALE, scale);
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
		/*
		 * It's unclear whether this is the correct action, but it
		 * happens for
		 *   "_($* /,##0.00_);_($* (#,##0.00);_($* \"-\"??_);_(@_)"
		 * in bug 750047.
		 */
		if (tno_numstart == -1)
			goto error;

		if (scale && !frac_part && E_part != 2)
			ADD_OP2 (OP_NUM_SCALE, scale);
		ADD_OP2 (OP_NUM_PRINTF_F, decimals);
		if (thousands && !inhibit_thousands)
			ADD_OP (OP_NUM_ENABLE_THOUSANDS);
	}

	ADD_OP (OP_NUM_SIGN);

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
	if (E_part == 2)
		ADD_OP (OP_NUM_STORE_POS);
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
		case '\'':
			if (E_part == 1)
				break;
			/* Fall through */
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
			case '\'':
				if (E_part == 1)
					break;
				/* Fall through */
			default:
				handle_common_token (ti->tstr, ti->token, prg);
			}
		}
	}

	pstate->scale = scale;

	if (inhibit_blank)
		*inhibit_blank = inhibit_b;
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
					   0, 0, NULL))
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
	gboolean use_markup = FALSE;
	gboolean append_SI = FALSE;
	gboolean simplify_mantissa = TRUE;
	int tno_end = pstate->tokens->len;
	int tno_exp_start = pstate->tno_E + 1;

	if (tno_exp_start >= tno_end)
		return NULL;

	switch (GET_TOKEN (pstate->tno_E).token) {
#ifdef ALLOW_SI_APPEND
	case TOK_EXP_SI:
		append_SI = TRUE;
		break;
#endif
#if defined(ALLOW_EE_MARKUP) && defined(ALLOW_SI_APPEND)
	case TOK_EXP_MU_SI:
		append_SI = TRUE;
		use_markup = TRUE;
		break;
#endif
#ifdef ALLOW_EE_MARKUP
	case TOK_EXP_MU:
		use_markup = TRUE;
		break;
#endif
	case TOK_EXP:
	default:
		break;
	}

	if (use_markup) {
		int i;
		simplify_mantissa = TRUE;
		for (i = 0; i < pstate->tno_E; i++) {
			if (GET_TOKEN(i).token == TOK_DECIMAL)
				break;
			if (GET_TOKEN(i).token == '0') {
				simplify_mantissa = FALSE;
				break;
			}
		}
	} else
		simplify_mantissa = FALSE;

	switch (GET_TOKEN (tno_exp_start).token) {
	case '-':
		pstate->forced_exponent_sign = FALSE;
		tno_exp_start++;
		break;
	case '+':
		pstate->forced_exponent_sign = TRUE;
		tno_exp_start++;
		break;
	default:
#ifndef ALLOW_NO_SIGN_AFTER_E
		if (!use_markup && !append_SI)
			return NULL;
#endif
		break;
	}

	prg = g_string_new (NULL);

	if (!go_format_parse_number_new_1 (prg, pstate,
					   0, pstate->tno_E,
					   1, 0, NULL))
		return NULL;

#ifdef ALLOW_SI_APPEND
	if (append_SI)
		ADD_OP (OP_NUM_REDUCE_EXPONENT_SI);
#endif
	ADD_OP (OP_NUM_VAL_EXPONENT);
#ifdef ALLOW_EE_MARKUP
	if (use_markup) {
		ADD_OPuc (OP_CHAR, UNICODE_TIMES);
		if (simplify_mantissa) {
			if (append_SI)
				ADD_OP (OP_NUM_SIMPLIFY_MANTISSA_SI);
			else
				ADD_OP (OP_NUM_SIMPLIFY_MANTISSA);
		}
		ADD_OP2 (OP_CHAR, '1');
		ADD_OP2 (OP_CHAR, '0');
		ADD_OP  (OP_MARKUP_SUPERSCRIPT_START);
	} else
#endif
		ADD_OP2 (OP_CHAR, 'E');

	if (!go_format_parse_number_new_1 (prg, pstate,
					   tno_exp_start, tno_end,
					   2, 0, NULL))
		return NULL;

#ifdef ALLOW_EE_MARKUP
	if (use_markup) {
		if (simplify_mantissa)
			ADD_OP (OP_NUM_SIMPLIFY_MARKUP_MANTISSA);
		ADD_OP  (OP_MARKUP_SUPERSCRIPT_END);
	}
#endif
#ifdef ALLOW_SI_APPEND
	if (append_SI) {
		ADD_OP  (OP_NUM_SIMPLIFY_EXPONENT_SI);
		ADD_OP  (OP_NUM_SI_EXPONENT);
	}
#endif

	handle_fill (prg, pstate);

	fmt = go_format_create (GO_FMT_NUMBER, NULL);
	fmt->u.number.program = g_string_free (prg, FALSE);
	fmt->u.number.E_format = TRUE;
	fmt->u.number.use_markup = use_markup;
	fmt->u.number.use_SI = append_SI;
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
	gboolean inhibit_blank_denom = FALSE;
	gboolean inhibit_blank_numerator = FALSE;
	gboolean inhibit_blank_whole = TRUE;
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

	ADD_OP (OP_NUM_VAL_SIGN);

	if (tno_endwhole != -1) {
		ADD_OP (OP_NUM_FRACTION_WHOLE);
		if (!go_format_parse_number_new_1 (prg, pstate,
						   0, tno_endwhole + 1,
						   0, 1, &inhibit_blank_whole))
			return NULL;
		scale += pstate->scale;

#ifdef ALLOW_PI_SLASH
		if (pi_scale) {
			/* ADD_OPuc (OP_CHAR, UNICODE_THINSPACE); */
			ADD_OPuc (OP_CHAR, UNICODE_PI); /* "pi" */
			ADD_OP (OP_NUM_FRACTION_PI_SUM_START);
			ADD_OPuc (OP_CHAR, UNICODE_THINSPACE);
			ADD_OP (OP_NUM_FRACTION_SIGN);
			ADD_OPuc (OP_CHAR, UNICODE_THINSPACE);
		}
#endif
	}


	ADD_OP (OP_NUM_DISABLE_THOUSANDS);

	ADD_OP (OP_NUM_FRACTION_NOMINATOR);
	if (!go_format_parse_number_new_1 (prg, pstate,
					   tno_endwhole + 1,
					   pi_scale ? tno_slash - 2 :tno_slash,
					   0, 2, &inhibit_blank_numerator))
		return NULL;
	scale += pstate->scale;

	ADD_OP (OP_NUM_FRACTION_SLASH);
	if (pi_scale)
		ADD_OPuc (OP_CHAR, UNICODE_PI); /* "pi" */
	ADD_OP2 (OP_CHAR, '/');

	pstate->explicit_denom = explicit_denom;
	ADD_OP (OP_NUM_FRACTION_DENOMINATOR);
	if (!go_format_parse_number_new_1 (prg, pstate,
					   tno_slash + 1, tno_suffix,
					   0, 3, &inhibit_blank_denom))
		return NULL;
	scale += pstate->scale;
	ADD_OP (OP_NUM_FRACTION_ALIGN);
#ifdef ALLOW_PI_SLASH
	if (pi_scale) {
		if (!inhibit_blank)
			ADD_OP (OP_NUM_FRACTION_BLANK_PI);
		if (!inhibit_blank_denom && !inhibit_blank_whole)
			ADD_OP (OP_NUM_FRACTION_SIMPLIFY_PI);
		if (!inhibit_blank_numerator)
			ADD_OP (OP_NUM_FRACTION_SIMPLIFY_NUMERATOR_PI);

	} else
#endif
		{
			if (!inhibit_blank)
				ADD_OP (OP_NUM_FRACTION_BLANK);
#ifdef ALLOW_DENOM_REMOVAL
			if (!inhibit_blank_denom && !inhibit_blank_whole)
				ADD_OP (OP_NUM_FRACTION_SIMPLIFY);
			if (!inhibit_blank_numerator)
				ADD_OP (OP_NUM_FRACTION_SIMPLIFY_NUMERATOR);
#endif
		}
	if (!inhibit_blank_whole)
		ADD_OP (OP_NUM_FRACTION_BLANK_WHOLE);

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

		magic_fmt_str = go_format_magic_fmt_str (state.locale.locale  & 0xffffffff);
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
			if (i < 4 && cond->op == GO_FMT_COND_NONE) {
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
			g_printerr ("OP_LOCALE -- \"%s\" -- numeral shape: %#x -- calendar: %#x\n",
				    lang,
				    (guint)((locale.locale & 0xFFF000000) >> 24),
				    (guint)((locale.locale & 0x000FF0000) >> 16));
			break;
		}
		case OP_NUMERAL_SHAPE:
			g_printerr ("OP_NUMERAL_SHAPE flags:%#x shape:%#x\n", prg[0], prg[1]);
			prg += 2;
			break;
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
		REGULAR(OP_NUM_FRACTION_SIGN);
		REGULAR(OP_NUM_MOVETO_ONES);
		REGULAR(OP_NUM_MOVETO_DECIMALS);
		REGULAR(OP_NUM_REST_WHOLE);
		REGULAR(OP_NUM_APPEND_MODE);
		REGULAR(OP_NUM_DECIMAL_POINT);
		REGULAR(OP_NUM_DENUM_DIGIT_Q);
		REGULAR(OP_NUM_EXPONENT_1);
		REGULAR(OP_NUM_VAL_EXPONENT);
		REGULAR(OP_NUM_STORE_POS);
#ifdef ALLOW_EE_MARKUP
		REGULAR(OP_NUM_MARK_MANTISSA);
		REGULAR(OP_NUM_SIMPLIFY_MANTISSA);
		REGULAR(OP_NUM_SIMPLIFY_MARKUP_MANTISSA);
		REGULAR(OP_MARKUP_SUPERSCRIPT_START);
		REGULAR(OP_MARKUP_SUPERSCRIPT_END);
#endif
#ifdef ALLOW_SI_APPEND
		REGULAR(OP_NUM_SIMPLIFY_MANTISSA_SI);
		REGULAR(OP_NUM_REDUCE_EXPONENT_SI);
		REGULAR(OP_NUM_SIMPLIFY_EXPONENT_SI);
		REGULAR(OP_NUM_SI_EXPONENT);
#endif
		REGULAR(OP_NUM_FRACTION_WHOLE);
		REGULAR(OP_NUM_FRACTION_NOMINATOR);
		REGULAR(OP_NUM_FRACTION_DENOMINATOR);
		REGULAR(OP_NUM_FRACTION_BLANK);
		REGULAR(OP_NUM_FRACTION_BLANK_WHOLE);
		REGULAR(OP_NUM_FRACTION_ALIGN);
		REGULAR(OP_NUM_FRACTION_SLASH);
#ifdef ALLOW_DENOM_REMOVAL
		REGULAR(OP_NUM_FRACTION_SIMPLIFY);
		REGULAR(OP_NUM_FRACTION_SIMPLIFY_NUMERATOR);
#endif
#ifdef ALLOW_PI_SLASH
		REGULAR(OP_NUM_FRACTION_BLANK_PI);
		REGULAR(OP_NUM_FRACTION_SCALE_PI);
		REGULAR(OP_NUM_FRACTION_SIMPLIFY_PI);
		REGULAR(OP_NUM_FRACTION_SIMPLIFY_NUMERATOR_PI);
		REGULAR(OP_NUM_FRACTION_PI_SUM_START);
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
	memmove (str->str + fill_pos + gap,
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
		go_dtoa (dst, "=^.*" FORMAT_E, n, val);
		return;
	}

	exponent_guess = (int)floor (SUFFIX(log10) (SUFFIX(fabs) (val)));
	/* Extra digits we need assuming guess correct */
	nde = (exponent_guess >= 0)
		? exponent_guess % wd
		: (wd - ((-exponent_guess) % wd)) % wd;

	go_dtoa (dst, "=^.*" FORMAT_E, n + nde, val);
	epos = (char *)strchr (dst->str, 'E');
	if (!epos)
		return;

	exponent = atoi (epos + 1);
	g_string_truncate (dst, epos - dst->str);
	dot = (char *)strstr (dst->str, decimal->str);
	if (exponent != exponent_guess) {
		/*
		 * We rounded from 9.99Exx to
		 *                 1.00Eyy
		 * with yy=xx+1.
		 */
		nde = (nde + 1) % wd;
		if (nde == 0)
			g_string_truncate (dst, dst->len - (wd - 1));
		else if (dot) /* only add a 0 when a decimal separator is present,
					   * see #785669 */
			g_string_append_c (dst, '0');
	}

	/* we need to adjust exponent before any modification to nde, see #785669 */
	exponent -= nde;
	if (dot) {
		memmove (dot, dot + decimal->len, nde);
		memcpy (dot + nde, decimal->str, decimal->len);
	} else {
		while (nde > 0) {
			g_string_append_c (dst, '0');
			nde--;
		}
	}

	g_string_append_printf (dst, "E%+d", exponent);
}

#ifdef DEFINE_COMMON
static int
go_format_get_width (GString *dst, PangoAttrList *attrs, int start,
		     int length, PangoLayout *layout)
{
	GList *plist, *l;
	PangoContext *context;
	int width = 0;

	if (layout == NULL ||
	    (context = pango_layout_get_context (layout)) == NULL
	    || pango_context_get_font_map (context) == NULL)
		return 0;

	plist = pango_itemize (context, dst->str, start, length, attrs, NULL);
	for (l = plist; l != NULL; l = l->next) {
		PangoItem *pi = l->data;
		PangoGlyphString *glyphs = pango_glyph_string_new ();

		pango_shape (dst->str + pi->offset, pi->length, &pi->analysis, glyphs);
		width += pango_glyph_string_get_width (glyphs);
		pango_glyph_string_free (glyphs);
	}
	g_list_free_full (plist, (GDestroyNotify) pango_item_free);
	return width;
}
#endif

#ifdef DEFINE_COMMON
static int
go_format_desired_width (PangoLayout *layout, PangoAttrList *attrs, int digits)
{
	char str[2] = {'0',0};
	const gchar *strp = &(str[0]);
	GList *plist, *l;
	int width = 0;
	PangoContext *context;

	if (layout == NULL ||
	    (context = pango_layout_get_context (layout)) == NULL
	    || pango_context_get_font_map (context) == NULL)
		return 0;

	plist = pango_itemize (context, strp, 0, 1, attrs, NULL);
	for (l = plist; l != NULL; l = l->next) {
		PangoItem *pi = l->data;
		PangoGlyphString *glyphs = pango_glyph_string_new ();
		PangoRectangle ink_rect;
		PangoRectangle logical_rect;

		pango_shape (strp + pi->offset, pi->length, &pi->analysis, glyphs);
		pango_glyph_string_extents (glyphs,
					    pi->analysis.font,
					    &ink_rect,
					    &logical_rect);
		pango_glyph_string_free (glyphs);
		width += logical_rect.width;
	}
	g_list_free_full (plist, (GDestroyNotify) pango_item_free);

	return (int)(1.1 * width *digits);
}
#endif


#ifdef DEFINE_COMMON
static void
blank_characters (GString *dst, PangoAttrList *attrs, int start, int length,
		  PangoLayout *layout)
{
	/* We have layouts that have no fontmap set, we need to avoid them */
	if (layout && pango_context_get_font_map (pango_layout_get_context (layout))) {
		int full_width, short_width;
		PangoAttribute *attr;
		PangoAttrList *new_attrs = pango_attr_list_new ();
		PangoRectangle logical_rect = {0, 0, 0, 2 * PANGO_SCALE};

		pango_layout_set_text (layout, dst->str, -1);
		pango_layout_set_attributes (layout, attrs);
		full_width = go_format_measure_pango (NULL, layout);
		g_string_erase (dst, start, length);
		go_pango_attr_list_erase (attrs, start, length);
		pango_layout_set_text (layout, dst->str, -1);
		pango_layout_set_attributes (layout, attrs);
		short_width = go_format_measure_pango (NULL, layout);
		logical_rect.width = full_width - short_width;
		g_string_insert_c (dst, start, ' ');
		attr = pango_attr_shape_new (&logical_rect, &logical_rect);
		attr->start_index = 0;
		attr->end_index = 1;
		pango_attr_list_insert (new_attrs, attr);
		pango_attr_list_splice (attrs, new_attrs, start, 1);
		pango_attr_list_unref (new_attrs);
	} else
		memset (dst->str + start, ' ', length);
}
#endif

#ifdef DEFINE_COMMON
static int
cnt_digits (int d)
{
	int cnt = 0;

	while (d > 0) {
		cnt++;
		d /= 10;
	}

	return cnt;
}
#endif

#if defined(ALLOW_SI_APPEND) && defined (DEFINE_COMMON)
static int
si_reduction (int exponent, char const **si)
{
	static struct {
		char const *prefix;
		int power;
	} si_prefixes[] = {
		{"Y" , 24},
		{"Z" , 21},
		{"E" , 18},
		{"P" , 15},
		{"T" , 12},
		{"G" ,  9},
		{"M" ,  6},
		{"k" ,  3},
		{"h" ,  2},
		{"da" , 1},
		{"" ,  0},
		{"d" , -1},
		{"c" , -2},
		{"m" , -3},
		{"\302\265" , -6},
		{"n" , -9},
		{"p" ,-12},
		{"f" ,-15},
		{"a" ,-18},
		{"z" ,-21},
		{"y" ,-24}
	};
	guint i;

	for (i = 0; i < G_N_ELEMENTS (si_prefixes) - 1; i++)
		if (si_prefixes[i].power <= exponent)
			break;

	*si = si_prefixes[i].prefix;
	return si_prefixes[i].power;
}

#endif

#ifdef DEFINE_COMMON
static char const *minus_shapes[] =
	{
		UTF8_MINUS,          /* 00 Unused */
		UTF8_MINUS,          /* 01 Unused */
		UTF8_MINUS,          /* 02 Arabic Indic */
		UTF8_MINUS,          /* 03 Extended Arabic Indic */
		UTF8_MINUS,          /* 04 Devanagari */
		UTF8_MINUS,          /* 05 Bengali */
		UTF8_MINUS,          /* 06 Gurmukhi */
		UTF8_MINUS,          /* 07 Gujarati */
		UTF8_MINUS,          /* 08 Orija */
		UTF8_MINUS,          /* 09 Tamil */
		UTF8_MINUS,          /* 0A Telugu */
		UTF8_MINUS,          /* 0B Kannada*/
		UTF8_MINUS,          /* 0C Malayalam*/
		UTF8_MINUS,          /* 0D Thai */
		UTF8_MINUS,          /* 0E Lao */
		UTF8_MINUS,          /* 0F Tibetan */
		UTF8_MINUS,          /* 10 Myanmar */
		UTF8_MINUS,          /* 11 Ethiopic ? Not really a decimal system */
		UTF8_MINUS,          /* 12 Khmer */
		UTF8_MINUS,          /* 13 Mongolian */
		UTF8_MINUS,          /* 14 Unused */
		UTF8_MINUS,          /* 15 Unused */
		UTF8_MINUS,          /* 16 Unused */
		UTF8_MINUS,          /* 17 Unused */
		UTF8_MINUS,          /* 18 Unused */
		UTF8_MINUS,          /* 19 Unused */
		UTF8_MINUS,          /* 1A Unused */
		UTF8_FULLWIDTH_MINUS,    /* 1B Japanese 1 ? */
		UTF8_FULLWIDTH_MINUS,    /* 1C Japanese 2 ? */
		UTF8_FULLWIDTH_MINUS,    /* 1D Japanese 3 ? */
		"\350\264\237",          /* 1E Simplified Chinese 1 */
		"\350\264\237",          /* 1F Simplified Chinese 2 */
		UTF8_FULLWIDTH_MINUS,    /* 20 Simplified Chinese 3 */
		"\350\262\240",          /* 21 Traditional Chinese 1 */
		"\350\262\240",          /* 22 Traditional Chinese 2 */
		UTF8_FULLWIDTH_MINUS,    /* 23 Traditional Chinese 3 */
		UTF8_FULLWIDTH_MINUS,    /* 24 Korean 1 ? */
		UTF8_FULLWIDTH_MINUS,    /* 25 Korean 2 ? */
		UTF8_FULLWIDTH_MINUS,    /* 26 Korean 3 ? */
		UTF8_FULLWIDTH_MINUS,    /* 27 Korean 4 ? */
	};

static char const *plus_shapes[] =
	{
		"+",          /* 00 Unused */
		"+",          /* 01 Unused */
		"+",          /* 02 Arabic Indic */
		"+",          /* 03 Extended Arabic Indic */
		"+",          /* 04 Devanagari */
		"+",          /* 05 Bengali */
		"+",          /* 06 Gurmukhi */
		"+",          /* 07 Gujarati */
		"+",          /* 08 Orija */
		"+",          /* 09 Tamil */
		"+",          /* 0A Telugu */
		"+",          /* 0B Kannada*/
		"+",          /* 0C Malayalam*/
		"+",          /* 0D Thai */
		"+",          /* 0E Lao */
		"+",          /* 0F Tibetan */
		"+",          /* 10 Myanmar */
		"+",          /* 11 Ethiopic ? Not really a decimal system */
		"+",          /* 12 Khmer */
		"+",          /* 13 Mongolian */
		"+",          /* 14 Unused */
		"+",          /* 15 Unused */
		"+",          /* 16 Unused */
		"+",          /* 17 Unused */
		"+",          /* 18 Unused */
		"+",          /* 19 Unused */
		"+",          /* 1A Unused */
		UTF8_FULLWIDTH_PLUS,    /* 1B Japanese 1 ? */
		UTF8_FULLWIDTH_PLUS,    /* 1C Japanese 2 ? */
		UTF8_FULLWIDTH_PLUS,    /* 1D Japanese 3 ? */
		UTF8_FULLWIDTH_PLUS,    /* 1E Simplified Chinese 1 */
		UTF8_FULLWIDTH_PLUS,    /* 1F Simplified Chinese 2 */
		UTF8_FULLWIDTH_PLUS,    /* 20 Simplified Chinese 3 */
		UTF8_FULLWIDTH_PLUS,    /* 21 Traditional Chinese 1 */
		UTF8_FULLWIDTH_PLUS,    /* 22 Traditional Chinese 2 */
		UTF8_FULLWIDTH_PLUS,    /* 23 Traditional Chinese 3 */
		UTF8_FULLWIDTH_PLUS,    /* 24 Korean 1 ? */
		UTF8_FULLWIDTH_PLUS,    /* 25 Korean 2 ? */
		UTF8_FULLWIDTH_PLUS,    /* 26 Korean 3 ? */
		UTF8_FULLWIDTH_PLUS,    /* 27 Korean 4 ? */
	};
#endif


#ifdef DEFINE_COMMON

static char const *numeral_shapes[][10]
= {{NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},                           /* 00 Unused */
   {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},                           /* 01 Unused */
   {"\331\240","\331\241","\331\242","\331\243","\331\244","\331\245","\331\246",
    "\331\247","\331\250","\331\251"},                                            /* 02 Arabic Indic */
   {"\333\260","\333\261","\333\262","\333\263","\333\264","\333\265","\333\266",
    "\333\267","\333\270","\333\271"},                                            /* 03 Extended Arabic Indic */
   {"\340\245\246","\340\245\247","\340\245\250","\340\245\251","\340\245\252",
    "\340\245\253","\340\245\254","\340\245\255","\340\245\256","\340\245\257"},  /* 04 Devanagari */
   {"\340\247\246","\340\247\247","\340\247\250","\340\247\251","\340\247\252",
    "\340\247\253","\340\247\254","\340\247\255","\340\247\256","\340\247\257",}, /* 05 Bengali */
   {"\340\251\246","\340\251\247","\340\251\250","\340\251\251","\340\251\252",
    "\340\251\253","\340\251\254","\340\251\255","\340\251\256","\340\251\257"},  /* 06 Gurmukhi */
   {"\340\253\246","\340\253\247","\340\253\250","\340\253\251","\340\253\252",
    "\340\253\253","\340\253\254","\340\253\255","\340\253\256","\340\253\257"},  /* 07 Gujarati */
   {"\340\255\246","\340\255\247","\340\255\250","\340\255\251","\340\255\252",
    "\340\255\253","\340\255\254","\340\255\255","\340\255\256","\340\255\257"},  /* 08 Orija */
   {"\340\257\246","\340\257\247","\340\257\250","\340\257\251","\340\257\252",
    "\340\257\253","\340\257\254","\340\257\255","\340\257\256","\340\257\257"},  /* 09 Tamil */
   {"\340\261\246","\340\261\247","\340\261\250","\340\261\251","\340\261\252",
    "\340\261\253","\340\261\254","\340\261\255","\340\261\256","\340\261\257"},  /* 0A Telugu */
   {"\340\263\246","\340\263\247","\340\263\250","\340\263\251","\340\263\252",
    "\340\263\253","\340\263\254","\340\263\255","\340\263\256","\340\263\257"},  /* 0B Kannada*/
   {"\340\265\246","\340\265\247","\340\265\250","\340\265\251","\340\265\252",
    "\340\265\253","\340\265\254","\340\265\255","\340\265\256","\340\265\257"},  /* 0C Malayalam*/
   {"\340\271\220","\340\271\221","\340\271\222","\340\271\223","\340\271\224",
    "\340\271\225","\340\271\226","\340\271\227","\340\271\230","\340\271\231"},  /* 0D Thai */
   {"\340\273\220","\340\273\221","\340\273\222","\340\273\223","\340\273\224",
    "\340\273\225","\340\273\226","\340\273\227","\340\273\230","\340\273\231"},  /* 0E Lao */
   {"\340\274\240","\340\274\241","\340\274\242","\340\274\243","\340\274\244",
    "\340\274\245","\340\274\246","\340\274\247","\340\274\250","\340\274\251",}, /* 0F Tibetan */
   {"\341\201\200","\341\201\201","\341\201\202","\341\201\203","\341\201\204",
    "\341\201\205","\341\201\206","\341\201\207","\341\201\210","\341\201\211"},  /* 10 Myanmar */
   {"0","\341\215\251","\341\215\252","\341\215\253","\341\215\254",
    "\341\215\255","\341\215\256","\341\215\257","\341\215\260","\341\215\261"},  /* 11 Ethiopic Not really a decimal system */
   {"\341\237\240","\341\237\241","\341\237\242","\341\237\243","\341\237\244",
    "\341\237\245","\341\237\246","\341\237\247","\341\237\250","\341\237\251"},  /* 12 Khmer */
   {"\341\240\220","\341\240\221","\341\240\222","\341\240\223","\341\240\224",
    "\341\240\225","\341\240\226","\341\240\227","\341\240\230","\341\240\231"},  /* 13 Mongolian */
   {"0","1","2","3","4","5","6","7","8","9"},                                     /* 14 Unused */
   {"0","1","2","3","4","5","6","7","8","9"},                                     /* 15 Unused */
   {"0","1","2","3","4","5","6","7","8","9"},                                     /* 16 Unused */
   {"0","1","2","3","4","5","6","7","8","9"},                                     /* 17 Unused */
   {"0","1","2","3","4","5","6","7","8","9"},                                     /* 18 Unused */
   {"0","1","2","3","4","5","6","7","8","9"},                                     /* 19 Unused */
   {"0","1","2","3","4","5","6","7","8","9"},                                     /* 1A Unused */
   {"\343\200\207","\344\270\200","\344\272\214","\344\270\211","\345\233\233",
    "\344\272\224","\345\205\255","\344\270\203","\345\205\253","\344\271\235"},  /* 1B Japanese 1 (same as Traditional Chinese 1) */
   {"\343\200\207","\345\243\261","\345\274\220","\345\217\202","\345\233\233",
    "\344\274\215","\345\205\255","\344\270\203","\345\205\253","\344\271\235"},  /* 1C Japanese 2 */
   {"\357\274\220","\357\274\221","\357\274\222","\357\274\223","\357\274\224",
    "\357\274\225","\357\274\226","\357\274\227","\357\274\230","\357\274\231"},  /* 1D Japanese 3 (same as Traditional Chinese 3) */
   /* see http://www.w3.org/TR/css3-lists/ */
   {"\351\233\266","\344\270\200","\344\272\214","\344\270\211","\345\233\233",
    "\344\272\224","\345\205\255","\344\270\203","\345\205\253","\344\271\235"},  /* 1E Simplified Chinese 1 */
   {"\351\233\266","\345\243\271","\350\264\260","\345\217\201","\350\202\206",
    "\344\274\215","\351\231\206","\346\237\222","\346\215\214","\347\216\226"},  /* 1F Simplified Chinese 2 */
   {"\357\274\220","\357\274\221","\357\274\222","\357\274\223","\357\274\224",
    "\357\274\225","\357\274\226","\357\274\227","\357\274\230","\357\274\231"},  /* 20 Simplified Chinese 3 */
   {"\351\233\266","\344\270\200","\344\272\214","\344\270\211","\345\233\233",
    "\344\272\224","\345\205\255","\344\270\203","\345\205\253","\344\271\235"},  /* 21 Traditional Chinese 1 */
   {"\351\233\266","\345\243\271","\350\262\263","\345\217\203","\350\202\206",
    "\344\274\215","\351\231\270","\346\237\222","\346\215\214","\347\216\226"},  /* 22 Traditional Chinese 2 */
   {"\357\274\220","\357\274\221","\357\274\222","\357\274\223","\357\274\224",
    "\357\274\225","\357\274\226","\357\274\227","\357\274\230","\357\274\231"},  /* 23 Traditional Chinese 3 */
   {"\343\200\207","\344\270\200","\344\272\214","\344\270\211","\345\233\233",
    "\344\272\224","\345\205\255","\344\270\203","\345\205\253","\344\271\235"},  /* 24 Korean 1 (same as Traditional Chinese 1) */
   {"\351\233\266","\345\243\271","\350\262\263","\345\217\203","\345\233\233",
    "\344\274\215","\345\205\255","\344\270\203","\345\205\253","\344\271\235"},  /* 25 Korean 2 */
   {"\357\274\220","\357\274\221","\357\274\222","\357\274\223","\357\274\224",
    "\357\274\225","\357\274\226","\357\274\227","\357\274\230","\357\274\231"},  /* 26 Korean 3 (same as Traditional Chinese 3) */
   {"\354\230\201","\354\235\274","\354\235\264","\354\202\274","\354\202\254",
    "\354\230\244","\354\234\241","\354\271\240","\355\214\224","\352\265\254"}   /* 27 Korean 4 */
};

static char const *ethiopic_additional_digits[] =
	{
		"\341\215\262", /* 10 U+1372 */
		"\341\215\263", /* 20 U+1373 */
		"\341\215\264", /* 30 U+1374 */
		"\341\215\265", /* 40 U+1375 */
		"\341\215\266", /* 50 U+1376 */
		"\341\215\267", /* 60 U+1377 */
		"\341\215\270", /* 70 U+1378 */
		"\341\215\271", /* 80 U+1379 */
		"\341\215\272", /* 90 U+137A */
		"\341\215\273", /* 100 U+137B */
		"\341\215\274"  /* 10000 U+137C */
	};

static char const *chinese_marker_shapes[][20] =
	{
		{"\345\215\201", /* 10 U+5341 */
		 "\347\231\276", /* 100 U+767E */
		 "\345\215\203", /* 1000 U+5343 */
		 "\344\270\207", /* 10^4 U+4E07 */
		 "\345\204\204", /* 10^8 U+5104 */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\344\270\244", /* 10^16 U+4E24 */
		 "\345\236\223", /* 10^20 垓 */
		 "\360\245\235\261", /* 10^24 𥝱 or 秭 */
		 "\347\251\243", /* 10^28 穣 */
		 "\346\272\235", /* 10^32 溝 */
		 "\346\276\227", /* 10^36 澗 */
		 "\346\255\243", /* 10^40 正 */
		 "\350\274\211", /* 10^44 載 */
		 "\346\245\265", /* 10^48 極 */
		 "\346\201\222\346\262\263\346\262\231", /* 10^52 恒河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56 阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60 那由他 or 那由多 */
		 "\344\270\215\345\217\257\346\200\235\350\255\260", /* 10^64 不可思議 */
		 "\347\204\241\351\207\217\345\244\247\346\225\260"  /* 10^68 無量大数 */
		},  /* 1B Japanese 1 */
		{"\346\213\276", /* 10 U+62FE */
		 "\347\231\276", /* 100 U+767E */
		 "\351\230\241", /* 1000 U+9621 */
		 "\350\220\254", /* 10^4 U+842C */
		 "\345\204\204", /* 10^8 U+5104 */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\344\270\244", /* 10^16 U+4E24 */
		 "\345\236\223", /* 10^20 垓 */
		 "\360\245\235\261", /* 10^24 𥝱 or 秭 */
		 "\347\251\243", /* 10^28 穣 */
		 "\346\272\235", /* 10^32 溝 */
		 "\346\276\227", /* 10^36 澗 */
		 "\346\255\243", /* 10^40 正 */
		 "\350\274\211", /* 10^44 載 */
		 "\346\245\265", /* 10^48 極 */
		 "\346\201\222\346\262\263\346\262\231", /* 10^52 恒河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56 阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60 那由他 or 那由多 */
		 "\344\270\215\345\217\257\346\200\235\350\255\260", /* 10^64 不可思議 */
		 "\347\204\241\351\207\217\345\244\247\346\225\260"  /* 10^68 無量大数 */
		},  /* 1C Japanese 2 */
		{"\345\215\201", /* 10 U+5341 */
		 "\347\231\276", /* 100 U+767E */
		 "\345\215\203", /* 1000 U+5343 */
		 "\344\270\207", /* 10^4 U+4E07 */
		 "\345\204\204", /* 10^8 U+5104 */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\344\270\244", /* 10^16 U+4E24 */
		 "\345\236\223", /* 10^20 垓 */
		 "\360\245\235\261", /* 10^24 𥝱 or 秭 */
		 "\347\251\243", /* 10^28 穣 */
		 "\346\272\235", /* 10^32 溝 */
		 "\346\276\227", /* 10^36 澗 */
		 "\346\255\243", /* 10^40 正 */
		 "\350\274\211", /* 10^44 載 */
		 "\346\245\265", /* 10^48 極 */
		 "\346\201\222\346\262\263\346\262\231", /* 10^52 恒河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56 阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60 那由他 or 那由多 */
		 "\344\270\215\345\217\257\346\200\235\350\255\260", /* 10^64 不可思議 */
		 "\347\204\241\351\207\217\345\244\247\346\225\260"  /* 10^68 無量大数 */
		},  /* 1D Japanese 3 */
		{"\345\215\201", /* 10 U+5341 */
		 "\347\231\276", /* 100 U+767E */
		 "\345\215\203", /* 1000 U+5343 */
		 "\344\270\207", /* 10^4 U+4E07 */
		 "\344\272\277", /* 10^8 U+4EBF 亿 */
		 "\345\205\206", /* 10^12 U+5146 兆 */
		 "\345\205\251", /* 10^16 U+5169 京 */
		 "\345\236\223", /* 10^20 垓 */
		 "\347\247\255", /* 10^24 秭 */
		 "\347\251\260", /* 10^28 穰 */
		 "\346\262\237", /* 10^32 沟 */
		 "\346\266\247", /* 10^36 涧 */
		 "\346\255\243", /* 10^40 正 */
		 "\350\275\275", /* 10^44 载 */
		 "\346\236\201", /* 10^48 极 */
		 "\346\201\222\346\262\263\346\262\231", /* 10^52 恒河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56 阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60 那由他 */
		 "\344\270\215\345\217\257\346\200\235\350\256\256", /* 10^64 不可思议 */
		 "\346\227\240\351\207\217"  /* 10^68 无量 */
		},  /* 1E Simplified Chinese 1 */
		{"\346\213\276", /* 10 U+62FE */
		 "\344\275\260", /* 100 U+4F70 */
		 "\344\273\237", /* 1000 U+4EDF */
		 "\344\270\207", /* 10^4 U+4E07 */
		 "\344\272\277", /* 10^8 U+4EBF */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\345\205\251", /* 10^16 U+5169 */
		 "\345\236\223", /* 10^20 垓 */
		 "\347\247\255", /* 10^24 秭 */
		 "\347\251\260", /* 10^28 穰 */
		 "\346\262\237", /* 10^32 沟 */
		 "\346\266\247", /* 10^36 涧 */
		 "\346\255\243", /* 10^40 正 */
		 "\350\275\275", /* 10^44 载 */
		 "\346\236\201", /* 10^48 极 */
		 "\346\201\222\346\262\263\346\262\231", /* 10^52 恒河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56 阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60 那由他 */
		 "\344\270\215\345\217\257\346\200\235\350\256\256", /* 10^64 不可思议 */
		 "\346\227\240\351\207\217"  /* 10^68 无量 */
		},  /* 1F Simplified Chinese 2 */
		{"\345\215\201", /* 10 U+5341 */
		 "\347\231\276", /* 100 U+767E */
		 "\345\215\203", /* 1000 U+5343 */
		 "\344\270\207", /* 10^4 U+4E07 */
		 "\344\272\277", /* 10^8 U+4EBF */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\345\205\251", /* 10^16 U+5169 */
		 "\345\236\223", /* 10^20 垓 */
		 "\347\247\255", /* 10^24 秭 */
		 "\347\251\260", /* 10^28 穰 */
		 "\346\262\237", /* 10^32 沟 */
		 "\346\266\247", /* 10^36 涧 */
		 "\346\255\243", /* 10^40 正 */
		 "\350\275\275", /* 10^44 载 */
		 "\346\236\201", /* 10^48 极 */
		 "\346\201\222\346\262\263\346\262\231", /* 10^52 恒河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56 阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60 那由他 */
		 "\344\270\215\345\217\257\346\200\235\350\256\256", /* 10^64 不可思议 */
		 "\346\227\240\351\207\217"  /* 10^68 无量 */
		},  /* 20 Simplified Chinese 3 */
		{"\345\215\201", /* 10 U+5341 */
		 "\347\231\276", /* 100 U+767E */
		 "\345\215\203", /* 1000 U+5343 */
		 "\350\220\254", /* 10^4 U+842C */
		 "\345\204\204", /* 10^8 U+5104 */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\344\270\244", /* 10^16 U+4E24 */
		 "\345\236\223", /* 10^20 垓 */
		 "\347\247\255", /* 10^24 秭 */
		 "\347\251\260", /* 10^28 穰 */
		 "\346\272\235", /* 10^32 溝 */
		 "\346\276\227", /* 10^36 澗 */
		 "\346\255\243", /* 10^40 正 */
		 "\350\274\211", /* 10^44 載 */
		 "\346\245\265", /* 10^48 極 */
		 "\346\201\206\346\262\263\346\262\231", /* 10^52 恆河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56 阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60 那由他 */
		 "\344\270\215\345\217\257 \346\200\235\350\255\260", /* 10^64 不可思議 */
		 "\347\204\241\351\207\217"  /* 10^68 無量 */
		},  /* 21 Traditional Chinese 1 */
		{"\346\213\276", /* 10 U+62FE */
		 "\344\275\260", /* 100 U+4F70 */
		 "\344\273\237", /* 1000 U+4EDF */
		 "\350\220\254", /* 10^4 U+842C */
		 "\345\204\204", /* 10^8 U+5104 億 */
		 "\345\205\206", /* 10^12 U+5146 兆 */
		 "\344\270\244", /* 10^16 U+4E24 京 */
		 "\345\236\223", /* 10^20 垓 */
		 "\347\247\255", /* 10^24 秭 */
		 "\347\251\260", /* 10^28 穰 */
		 "\346\272\235", /* 10^32 溝 */
		 "\346\276\227", /* 10^36 澗 */
		 "\346\255\243", /* 10^40 正 */
		 "\350\274\211", /* 10^44 載 */
		 "\346\245\265", /* 10^48 極 */
		 "\346\201\206\346\262\263\346\262\231", /* 10^52 恆河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56 阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60 那由他 */
		 "\344\270\215\345\217\257 \346\200\235\350\255\260", /* 10^64 不可思議 */
		 "\347\204\241\351\207\217"  /* 10^68 無量 */
		},  /* 22 Traditional Chinese 2 */
		{"\345\215\201", /* 10 U+5341 */
		 "\347\231\276", /* 100 U+767E */
		 "\345\215\203", /* 1000 U+5343 */
		 "\350\220\254", /* 10^4 U+842C */
		 "\345\204\204", /* 10^8 U+5104 */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\344\270\244", /* 10^16 U+4E24 */
		 "\345\236\223", /* 10^20 垓 */
		 "\347\247\255", /* 10^24 秭 */
		 "\347\251\260", /* 10^28 穰 */
		 "\346\272\235", /* 10^32 溝 */
		 "\346\276\227", /* 10^36 澗 */
		 "\346\255\243", /* 10^40 正 */
		 "\350\274\211", /* 10^44 載 */
		 "\346\245\265", /* 10^48 極 */
		 "\346\201\206\346\262\263\346\262\231", /* 10^52 恆河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56 阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60 那由他 */
		 "\344\270\215\345\217\257 \346\200\235\350\255\260", /* 10^64 不可思議 */
		 "\347\204\241\351\207\217"  /* 10^68 無量 */
		},  /* 23 Traditional Chinese 3 */
		{"\345\215\201", /* 10 U+5341 */
		 "\347\231\276", /* 100 U+767E */
		 "\345\215\203", /* 1000 U+5343 */
		 "\344\270\207", /* 10^4 U+4E07 */
		 "\345\204\204", /* 10^8 U+5104 */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\344\270\244", /* 10^16 U+4E24 */
		 "\345\236\223", /* 10^20  垓 */
		 "\347\247\255", /* 10^24  秭 */
		 "\347\251\260", /* 10^28  穰 */
		 "\346\272\235", /* 10^32  溝 */
		 "\346\276\227", /* 10^36  澗 */
		 "\346\255\243", /* 10^40  正 */
		 "\350\274\211", /* 10^44  載 */
		 "\346\245\265", /* 10^48  極 */
		 "\346\201\222\346\262\263\346\262\231", /* 10^52  恒河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56  阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60  那由他 */
		 "\344\270\215\345\217\257\346\200\235\350\255\260", /* 10^64  不可思議 */
		 "\347\204\241\351\207\217\345\244\247\346\225\270"  /* 10^68  無量大數 */
		},  /* 24 Korean 1 */
		{"\346\213\276", /* 10 U+62FE */
		 "\347\231\276", /* 100 U+767E */
		 "\344\273\237", /* 1000 U+4EDF */
		 "\350\220\254", /* 10^4 U+842C */
		 "\345\204\204", /* 10^8 U+5104 */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\344\270\244", /* 10^16 U+4E24 */
		 "\345\236\223", /* 10^20  垓 */
		 "\347\247\255", /* 10^24  秭 */
		 "\347\251\260", /* 10^28  穰 */
		 "\346\272\235", /* 10^32  溝 */
		 "\346\276\227", /* 10^36  澗 */
		 "\346\255\243", /* 10^40  正 */
		 "\350\274\211", /* 10^44  載 */
		 "\346\245\265", /* 10^48  極 */
		 "\346\201\222\346\262\263\346\262\231", /* 10^52  恒河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56  阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60  那由他 */
		 "\344\270\215\345\217\257\346\200\235\350\255\260", /* 10^64  不可思議 */
		 "\347\204\241\351\207\217\345\244\247\346\225\270"  /* 10^68  無量大數 */
		},  /* 25 Korean 2 */
		{"\345\215\201", /* 10 U+5341 */
		 "\347\231\276", /* 100 U+767E */
		 "\345\215\203", /* 1000 U+5343 */
		 "\350\220\254", /* 10^4 U+842C */
		 "\345\204\204", /* 10^8 U+5104 */
		 "\345\205\206", /* 10^12 U+5146 */
		 "\344\270\244", /* 10^16 U+4E24 */
		 "\345\236\223", /* 10^20  垓 */
		 "\347\247\255", /* 10^24  秭 */
		 "\347\251\260", /* 10^28  穰 */
		 "\346\272\235", /* 10^32  溝 */
		 "\346\276\227", /* 10^36  澗 */
		 "\346\255\243", /* 10^40  正 */
		 "\350\274\211", /* 10^44  載 */
		 "\346\245\265", /* 10^48  極 */
		 "\346\201\222\346\262\263\346\262\231", /* 10^52  恒河沙 */
		 "\351\230\277\345\203\247\347\245\207", /* 10^56  阿僧祇 */
		 "\351\202\243\347\224\261\344\273\226", /* 10^60  那由他 */
		 "\344\270\215\345\217\257\346\200\235\350\255\260", /* 10^64  不可思議 */
		 "\347\204\241\351\207\217\345\244\247\346\225\270"  /* 10^68  無量大數 */
		},  /* 26 Korean 3 */
		{"\354\213\255", /* 10 십 */
		 "\353\260\261", /* 100 백 */
		 "\354\262\234", /* 1000 천 */
		 "\353\247\214", /* 10^4 U+B9CC 만 */
		 "\354\226\265", /* 10^8 U+C5B5 억 */
		 "\354\241\260", /* 10^12 U+C870 조 */
		 "\352\262\275",  /* 10^16 U+ACBD 경 */
		 "\355\225\264", /* 10^20 해 */
		 "\354\236\220", /* 10^24 자 */
		 "\354\226\221", /* 10^28 양 */
		 "\352\265\254", /* 10^32 구 */
		 "\352\260\204", /* 10^36 간 */
		 "\354\240\225", /* 10^40 정 */
		 "\354\236\254", /* 10^44 재 */
		 "\352\267\271", /* 10^48 극 */
		 "\355\225\255\355\225\230\354\202\254", /* 10^52 항하사 */
		 "\354\225\204\354\212\271\352\270\260", /* 10^56 아승기 */
		 "\353\202\230\354\234\240\355\203\200", /* 10^60 나유타 */
		 "\353\266\210352\260\200\354\202\254\354\235\230", /* 10^64 불가사의 */
		 "\353\254\264\353\237\211\353\214\200\354\210\230"  /* 10^68 무량대수 */
		}   /* 27 Korean 4 */
	};



G_STATIC_ASSERT (G_N_ELEMENTS (minus_shapes) == G_N_ELEMENTS (numeral_shapes));
G_STATIC_ASSERT (G_N_ELEMENTS (plus_shapes) == G_N_ELEMENTS (numeral_shapes));

static gboolean
convert_numerals (GString *str, gsize from, gsize to, guint shape)
{
	int i;
	gboolean val = FALSE;
	g_return_val_if_fail (shape > 1, FALSE);
	if (shape >= G_N_ELEMENTS (numeral_shapes))
		return FALSE;
	for (i = to; i >= (int)from; i--) {
		if (str->str[i] >= '0' &&
		    str->str[i] <= '9') {
			gint num = str->str[i] - '0';
			char const *num_str = numeral_shapes[shape][num];
			if (*num_str != 0) {
				go_string_replace (str, i, 1, num_str, -1);
				val = TRUE;
			}
		} else if (shape >= 0x1b && shape <= 0x27 &&
			   str->str[i] >= 'a' &&
			   str->str[i] <= 't') {
			gint num = str->str[i] - 'a';
			char const *num_str = chinese_marker_shapes[shape - 0x1B][num];
			if (*num_str != 0) {
				go_string_replace (str, i, 1, num_str, -1);
				val = TRUE;
			}
		} else if (shape == 0x11 &&
			   str->str[i] >= 'a' &&
			   str->str[i] <= 'k') {
			gint num = str->str[i] - 'a';
			char const *num_str = ethiopic_additional_digits[num];
			if (*num_str != 0) {
				go_string_replace (str, i, 1, num_str, -1);
				val = TRUE;
			}
		}
	}
	return val;
}

static gboolean
convert_sign (GString *str, size_t i, guint shape, guint shape_flags)
{
	gchar const *shaped_sign;
	gchar const **shapes;
	gchar const *sign;

	switch (str->str[i]) {
	case '-':
		shapes = minus_shapes;
		sign = UTF8_MINUS;
		break;
	case '+':
		shapes = plus_shapes;
		sign = "+";
		break;
	default:
		return FALSE;
	}

	if (((shape_flags & GO_FMT_SHAPE_SIGNS) == 0) ||
	    (shape <= 1) ||
	    (shape > G_N_ELEMENTS (minus_shapes)))
		shaped_sign = sign;
	else
		shaped_sign = shapes [shape];

	if (*shaped_sign != '+')
		go_string_replace (str, i, 1, shaped_sign, -1);
	return TRUE;
}

static void
handle_ethiopic (GString *numtxt, const char **dot, guint numeral_shape,
		guint shape_flags)
{
	gint last;
	gboolean hundred = FALSE;
	gboolean cnt = 0;
	gint tail = 0;

	if ((shape_flags & GO_FMT_POSITION_MARKERS) == 0 ||
	    numeral_shape != 0x11)
		return;
	if (dot && *dot) {
		last = *dot - numtxt->str - 1;
		tail = numtxt->len - last - 1;
	} else {
		last = (int)numtxt->len - 1;
	}
	if (last == 0 && numtxt->str[0] == '0')
		g_string_erase (numtxt, 0, 1);
	else if (last > 0) {
		for (; last >= 0; last--) {
			char digit = numtxt->str[last];
			if (digit >= '0' && digit <= '9') {
				if (cnt == 2)
					g_string_insert_c (numtxt, last + 1, 'j');
				else if (cnt == 4)
					g_string_insert_c (numtxt, last + 1, 'k');
				if (hundred) {
					if (digit == '0') {
						if (numtxt->str[last + 1] == '0') {
							if (numtxt->str[last + 2] == 'j')
								g_string_erase (numtxt, last, 3);
							else
								g_string_erase (numtxt, last, 2);
						} else {
							g_string_erase (numtxt, last, 1);
							if (numtxt->str[last] == '1' &&
							    numtxt->str[last + 1] == 'j')
								g_string_erase (numtxt, last, 1);
						}
					} else {
						numtxt->str[last] += ('a'-'1');
						if (numtxt->str[last + 1] == '0')
							g_string_erase (numtxt, last + 1, 1);
					}
				}
				hundred = !hundred;
				cnt++;
				if (cnt > 5)
					cnt -= 4;
			}
		}
		if (hundred) {
			if (numtxt->str[0] == '0')
				g_string_erase (numtxt, 0, 2);
			else
				if (numtxt->str[0] == '1' &&
				    numtxt->str[1] == 'j')
					g_string_erase (numtxt, 0, 1);
		}
	}

	if (dot && *dot)
		*dot = numtxt->str + (numtxt->len - tail);
}


static void
handle_chinese (GString *numtxt, const char **dot, guint numeral_shape,
		guint shape_flags)
{
	GString *ntxt;
	char const *last;
	gint i, wan;
	gboolean wan_written = TRUE;
	gboolean digit_written = FALSE;
	gboolean suppress_ten, suppress_ten_always;
	if ((shape_flags & GO_FMT_POSITION_MARKERS) == 0 ||
	    numeral_shape < 0x1B || numeral_shape > 0x27)
		return;
	last = ((dot && *dot) ? *dot - 1 : numtxt->str + (numtxt->len - 1));
	if (last <= numtxt->str + 1)
		return;

	ntxt = g_string_sized_new (100);
	suppress_ten = (numeral_shape == 0x1b || numeral_shape == 0x1d
			|| numeral_shape == 0x26);
	suppress_ten_always = (numeral_shape == 0x26);
	i = 0;
	wan = 0;
	while (last >= numtxt->str) {
		if (*last >= '0' && *last <= '9') {
			if (*last > '0' || digit_written) {
				if (!wan_written) {
					if (wan + 'c' > 't')
						g_string_prepend_c (ntxt, '?');
					else
						g_string_prepend_c (ntxt, 'c' + wan);
					wan_written = TRUE;
				}
				if (i > 0)
					g_string_prepend_c (ntxt, 'a' + i - 1);
				if (!suppress_ten_always ||
				    !(suppress_ten && wan == 0) ||
				    *last != '1')
					g_string_prepend_c (ntxt, *last);
				digit_written = TRUE;
			}
		} else g_string_prepend_c (ntxt, *last);
		if (++i > 3) {
			i = i % 4;
			wan++;
			wan_written = FALSE;
			digit_written = FALSE;
		}
		last --;
	}

	if (dot && *dot) {
		gint len = ntxt->len;
		g_string_append (ntxt, *dot);
		*dot = ntxt->str + len;
	}
	g_string_assign (numtxt, ntxt->str);
	g_string_free (ntxt, TRUE);
}
#endif

#define INSERT_MINUS(pos) do {						\
		if (unicode_minus ||					\
		    ((shape_flags & GO_FMT_SHAPE_SIGNS) &&		\
		     numeral_shape)) {					\
			if ((shape_flags & GO_FMT_SHAPE_SIGNS) &&	\
			    numeral_shape > 1 &&			\
			    numeral_shape < G_N_ELEMENTS (minus_shapes)) \
				g_string_insert				\
					(dst, (pos),			\
					 minus_shapes[numeral_shape]);	\
			else						\
				g_string_insert_len			\
					(dst, (pos), UTF8_MINUS, 3);	\
		} else							\
			g_string_insert_c (dst, (pos), '-');		\
	} while (0)

#define INSERT_PLUS(pos) do {						\
		if ((shape_flags & GO_FMT_SHAPE_SIGNS) &&		\
		    numeral_shape > 1 &&				\
		    numeral_shape < G_N_ELEMENTS (plus_shapes))		\
			g_string_insert					\
				(dst, (pos),				\
				 plus_shapes[numeral_shape]);		\
		else							\
			g_string_insert_c (dst, (pos), '+');		\
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
	char fsecond[DOUBLE_DIG + 10];
	const char *date_dec_ptr = NULL;
	GString *numtxt = NULL;
	size_t dotpos = 0;
	size_t numi = 0;
	int numpos = -1;
	int numpos_end = -1;
	int generalpos = -1;
	const GString *decimal = go_locale_get_decimal ();
	const GString *comma = go_locale_get_thousand ();
	gboolean thousands = FALSE;
	gboolean digit_count = 0;
	int exponent = 0;
#ifdef ALLOW_SI_APPEND
	char const *si_str = NULL;
	int si_pos = -1;
#endif
	struct {
		DOUBLE w, n, d, val;
		int digits;
		gsize whole_start, nominator_start, denominator_start, pi_sum_start;
		gboolean blanked, use_whole, denom_blanked;
	} fraction = {0., 0., 0., 0., 0, 0, 0, 0, 0, FALSE, FALSE, FALSE};
	char *oldlocale = NULL;
	guint numeral_shape = 0;
	guint shape_flags = 0;
	PangoAttrList *attrs = NULL;

#ifdef ALLOW_EE_MARKUP
	int mantissa_start = -1;
	int special_mantissa = INT_MAX;
	GSList *markup_stack = NULL;
#endif

	if (layout) {
		attrs = pango_attr_list_copy (pango_layout_get_attributes (layout));
		if (attrs == NULL)
			attrs = pango_attr_list_new ();
	}

	while (1) {
		GOFormatOp op = *prg++;

		switch (op) {
		case OP_END:
			if (layout) {
				pango_layout_set_text (layout, dst->str, -1);
				if (attrs) {
					pango_layout_set_attributes (layout, attrs);
					pango_attr_list_unref (attrs);
					attrs = NULL;
				}
#ifdef ALLOW_EE_MARKUP
				g_slist_free (markup_stack);
#endif
			}
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
			const char *lang;
			/* GOFormatLocale locale; */
			/* memcpy (&locale, prg, sizeof (locale)); */
			prg += sizeof (GOFormatLocale);
			lang = (const char *)prg;
			prg += strlen (lang) + 1;

			if (oldlocale == NULL)
				oldlocale = g_strdup (go_setlocale (LC_ALL, NULL));
			/* Setting LC_TIME should be enough, but glib gets
			   confused over character sets.  */
			go_setlocale (LC_TIME, lang);
			go_setlocale (LC_CTYPE, lang);
			break;
		}

		case OP_NUMERAL_SHAPE:
			shape_flags = (*prg++ & 0x000f);
			numeral_shape = (*prg++ & 0x00ff);
			break;

		case OP_DATE_ROUND: {
			int date_decimals = *prg++;
			gboolean seen_elapsed = *prg++;
			DOUBLE unit = SUFFIX(go_pow10)(date_decimals);
#ifdef ALLOW_NEGATIVE_TIMES
			gboolean isneg = (val < 0);
#else
			gboolean isneg = FALSE;
#endif

			valsecs = SUFFIX(round)(SUFFIX(go_add_epsilon)(SUFFIX(fabs)(val)) * (unit * 86400));
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
			g_string_append_unichar (dst, g_utf8_get_char (s));
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
			go_dtoa (numtxt, "=^.*" FORMAT_f, n, val);
			dot = strstr (numtxt->str, decimal->str);
			handle_chinese (numtxt, &dot,
					numeral_shape, shape_flags);
			handle_ethiopic (numtxt, &dot,
					 numeral_shape, shape_flags);
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
				INSERT_MINUS(0);
			}
			break;

		case OP_NUM_VAL_SIGN:
			if (val < 0)
				INSERT_MINUS(numpos);
			break;

		case OP_NUM_FRACTION_SIGN:
			if (fraction.val < 0)
				INSERT_MINUS(numpos);
			else
				g_string_insert_c (dst, numpos, '+');
			break;

		case OP_NUM_MOVETO_ONES: {
			numi = dotpos;
			/* Ignore the zero in "0.xxx" or "0 xx/xx" */
			if (numi == 1 && numtxt->str[0] == '0' &&
			    numtxt->str[dotpos] != 0)
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
				    digit_count % 3 == 1) {
					g_string_insert_len (dst, numpos,
							     comma->str,
							     comma->len);
					if (numpos_end >= 0)
						numpos_end += comma->len;
				}
				g_string_insert_c (dst, numpos, c);
				if ((numeral_shape) > 1)  /* 0: not set; 1: Western */
					convert_numerals (dst, numpos, numpos, numeral_shape);
				if (numpos_end >= 0)
					numpos_end++;

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
				if (numpos_end >= 0)
					numpos_end += comma->len;
			}
			g_string_insert_c (dst, numpos, c);
			if ((numeral_shape) > 1)  /* 0: not set; 1: Western */
				convert_numerals (dst, numpos, numpos, numeral_shape);
			if (numpos_end >= 0)
				numpos_end++;
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
			if ((numeral_shape) > 1)  /* 0: not set; 1: Western */
				convert_numerals (dst, dst->len - 1, dst->len - 1,
						  numeral_shape);
			break;
		}

		case OP_NUM_DIGIT_1_0: {
			char fc = *prg++;
			if (fc == '0') {
				g_string_insert_c (dst, numpos, '0');
				if ((numeral_shape) > 1)  /* 0: not set; 1: Western */
					convert_numerals (dst, numpos, numpos, numeral_shape);
			} else if (fc == '?')
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
				if ((numeral_shape) > 1)  /* 0: not set; 1: Western */
					convert_numerals (dst, numpos, numpos, numeral_shape);
			}
			break;

		case OP_NUM_EXPONENT_SIGN: {
			gboolean forced = (*prg++ != 0);
			if (exponent >= 0) {
				if (forced)
					INSERT_PLUS(numpos);
			} else
				INSERT_MINUS(numpos);
			break;
		}

		case OP_NUM_VAL_EXPONENT:
			val = SUFFIX (fabs) (exponent + 0.0);
			break;

		case OP_NUM_STORE_POS:
			numpos_end = numpos;
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

		case OP_NUM_SIMPLIFY_MARKUP_MANTISSA:
			if (special_mantissa == 0) {
				g_string_erase (dst, mantissa_start,
						numpos_end - mantissa_start);
				g_string_insert_c (dst, mantissa_start, '0');
				numpos_end = mantissa_start + 1;
			}
			break;

		case OP_MARKUP_SUPERSCRIPT_START:
			if (layout) {
				/* FIXME: we need to fix the exponent handling */
				/* until that time use only latin numerals */
				numeral_shape = 0;

				markup_stack = g_slist_prepend
					(markup_stack, GSIZE_TO_POINTER (dst->len));
			}
			break;

		case OP_MARKUP_SUPERSCRIPT_END:
			if (layout) {
				guint start = 0,
					end = (guint)numpos_end;
				PangoAttribute *attr;

				if (markup_stack) {
					start = (guint)GPOINTER_TO_SIZE (markup_stack->data);
					markup_stack = g_slist_delete_link (markup_stack, markup_stack);
				}

				attr = go_pango_attr_superscript_new (TRUE);
				attr->start_index = start;
				attr->end_index = end;
				pango_attr_list_insert (attrs, attr);
			}
			break;
#endif
#ifdef ALLOW_SI_APPEND
		case OP_NUM_SIMPLIFY_MANTISSA_SI:
			if (exponent != 0 && special_mantissa != INT_MAX) {
				g_string_truncate (dst, mantissa_start);
				si_pos = mantissa_start;
			}

			break;

		case OP_NUM_REDUCE_EXPONENT_SI:
			exponent -= si_reduction (exponent, &si_str);
			si_pos = dst->len;
			break;

		case OP_NUM_SIMPLIFY_EXPONENT_SI:
			if (exponent == 0 && si_pos > 0 && numpos_end >= 0) {
				int len = numpos_end - si_pos;
				if (attrs)
					go_pango_attr_list_erase (attrs, si_pos, len);
				g_string_erase (dst, si_pos, len);
				numpos_end = si_pos;
			}
			break;

		case OP_NUM_SI_EXPONENT:
			g_string_insert_c (dst, numpos_end, ' ');
			numpos_end++;
			if (si_str != NULL) {
				g_string_insert (dst, numpos_end, si_str);
				numpos_end += strlen (si_str);
			}
			si_pos = -1;
			break;
#endif

		case OP_NUM_FRACTION: {
			gboolean wp = *prg++;
			gboolean explicit_denom = *prg++;
			DOUBLE aval = SUFFIX(fabs)(val);
			DOUBLE aval0 = aval;
			DOUBLE delta;

			// Parse the program argument here, not after "retry:"
			if (explicit_denom) {
				double plaind = 1; /* Plain double */
				memcpy (&plaind, prg, sizeof (plaind));
				prg += sizeof (plaind);
				fraction.d = plaind;
				fraction.digits = cnt_digits (fraction.d);
			} else {
				fraction.digits = *prg++;
			}

			fraction.val = val;

			if (aval != SUFFIX(floor)(aval))
				aval = SUFFIX(go_add_epsilon) (aval);

			retry:
			delta = aval - aval0;
			fraction.w = SUFFIX(floor) (aval);
			aval -= fraction.w;

			if (explicit_denom) {
				fraction.n = SUFFIX(round) (aval * fraction.d);
			} else {
				int ni, di;
				DOUBLE p10 = SUFFIX(go_pow10) (fraction.digits);
				int max_denom = MIN (INT_MAX, p10 - 1);

				go_continued_fraction (aval, max_denom, &ni, &di);
				fraction.n = ni;
				fraction.d = di;
			}

			if (delta >= 1 / fraction.d) {
				// Don't allow the add-epsilon to exceed 1/d
				aval = aval0;
				goto retry;
			}

			if (wp && fraction.n == fraction.d) {
				fraction.w += 1;
				fraction.n = 0;
			}
			if (!wp)
				fraction.n += fraction.d * fraction.w;

			break;
		}

#ifdef ALLOW_PI_SLASH
		case OP_NUM_FRACTION_SCALE_PI:
			/* FIXME: not long-double safe.  */
			val /= DOUBLE_PI;
			break;
#endif

		case OP_NUM_FRACTION_WHOLE:
			fraction.use_whole = TRUE;
			fraction.whole_start = dst->len;
			val = fraction.w;
			break;

		case OP_NUM_FRACTION_NOMINATOR:
			fraction.nominator_start = dst->len;
			val = fraction.n;
			break;

		case OP_NUM_FRACTION_SLASH:
			{
				int desired_width = go_format_desired_width (layout, attrs, fraction.digits);
				int length;
				PangoRectangle logical_rect = {0, 0, 0, 2 * PANGO_SCALE};
				char *end = dst->str + dst->len;
				char *nom = dst->str + fraction.nominator_start;
				end = g_utf8_prev_char (end);
				while (!g_unichar_isdigit (g_utf8_get_char (end)) && end > nom)
					end = g_utf8_prev_char (end);
				end = g_utf8_find_next_char (end, NULL);
				length = end - nom;

				logical_rect.width = go_format_get_width
					(dst, attrs, fraction.nominator_start,
					 length, layout);

				if (logical_rect.width < desired_width) {
					PangoAttribute *attr;
					PangoAttrList *new_attrs = pango_attr_list_new ();
					logical_rect.width = desired_width - logical_rect.width;
					attr = pango_attr_shape_new (&logical_rect, &logical_rect);
					attr->start_index = 0;
					attr->end_index = 1;
					pango_attr_list_insert (new_attrs, attr);
					g_string_insert_c (dst, fraction.nominator_start, ' ');
					pango_attr_list_splice (attrs, new_attrs, fraction.nominator_start, 1);
					pango_attr_list_unref (new_attrs);
					fraction.nominator_start++;
				}
			}
			break;

		case OP_NUM_FRACTION_DENOMINATOR:
			fraction.denominator_start = dst->len;
			val = fraction.d;
			break;

		case OP_NUM_FRACTION_BLANK:
			if (fraction.n == 0) {
				/* Replace all added characters by spaces of the right length.  */
				if (dst->len > fraction.nominator_start) {
					blank_characters (dst, attrs, fraction.nominator_start,
							  dst->len - fraction.nominator_start, layout);
					fraction.blanked = TRUE;
				}
			}
			break;

#ifdef ALLOW_PI_SLASH
		case OP_NUM_FRACTION_PI_SUM_START:
			fraction.pi_sum_start = dst->len;
			break;

		case OP_NUM_FRACTION_BLANK_PI:
			if (fraction.n == 0) {
				/* Replace all added characters by spaces of the right length.  */
				if (dst->len > fraction.pi_sum_start + 1) {
					blank_characters (dst, attrs, fraction.pi_sum_start,
							  dst->len - fraction.pi_sum_start, layout);
					fraction.blanked = TRUE;
				}
			}
			break;
#endif

		case OP_NUM_FRACTION_BLANK_WHOLE:
			if (!fraction.blanked && fraction.w == 0) {
				gsize start = fraction.whole_start;
				gsize length = fraction.nominator_start - start;
				g_string_erase (dst, start, length);
				go_pango_attr_list_erase (attrs, start, length);
				fraction.nominator_start -= length;
				fraction.denominator_start -= length;
				fraction.pi_sum_start = 0;
			} else if (fraction.pi_sum_start > 0) {
				if (fraction.w == 1) {
					gsize start = fraction.whole_start;
					gsize length = fraction.pi_sum_start
						- UNICODE_PI_number_of_bytes - start;
					g_string_erase (dst, start, length);
					go_pango_attr_list_erase (attrs, start, length);
					fraction.nominator_start -= length;
					fraction.denominator_start -= length;
					fraction.pi_sum_start = 0;
				} else if (fraction.w == 0) {
					blank_characters
						(dst, attrs,
						 fraction.pi_sum_start - UNICODE_PI_number_of_bytes,
						 UNICODE_PI_number_of_bytes, layout);
				}
			}
			break;

		case OP_NUM_FRACTION_ALIGN:
			if (layout && pango_context_get_font_map (pango_layout_get_context (layout))) {
				int desired_width = go_format_desired_width (layout, attrs, fraction.digits);
				PangoRectangle logical_rect = {0, 0, 0, 2 * PANGO_SCALE};
				int existing_width;

				existing_width = go_format_get_width
					(dst, attrs, fraction.denominator_start,
					 dst->len - fraction.denominator_start, layout);
				if (existing_width < desired_width) {
					PangoAttribute *attr;
					int start = dst->len;
					logical_rect.width = desired_width - existing_width;
					attr = pango_attr_shape_new (&logical_rect, &logical_rect);
					attr->start_index = start;
					attr->end_index = start + 1;
					g_string_append_c (dst, ' ');
					pango_attr_list_insert (attrs, attr);
				}
			}
			break;


#ifdef ALLOW_PI_SLASH
		case OP_NUM_FRACTION_SIMPLIFY_PI:
			if (!fraction.blanked && fraction.d == 1 &&
			    fraction.w == 0) {
				blank_characters (dst, attrs, fraction.denominator_start - 1,
						  2, layout);
				fraction.denom_blanked = TRUE;
			}
			break;
#endif
#ifdef ALLOW_DENOM_REMOVAL
		case OP_NUM_FRACTION_SIMPLIFY:
			if (!fraction.blanked && fraction.d == 1 &&  fraction.w == 0) {
				blank_characters (dst, attrs, fraction.denominator_start - 1,
						  2, layout);
				fraction.denom_blanked = TRUE;
			}
			break;
#endif

#ifdef ALLOW_DENOM_REMOVAL
		case OP_NUM_FRACTION_SIMPLIFY_NUMERATOR:
			if (fraction.denom_blanked && fraction.n == 0){
				gsize p = fraction.nominator_start;
				gsize length = fraction.denominator_start - p;
				blank_characters (dst, attrs, p, length, layout);
				fraction.denominator_start -= length - 1;
			}
			break;
#endif

#ifdef ALLOW_PI_SLASH
		case OP_NUM_FRACTION_SIMPLIFY_NUMERATOR_PI:
			if (!fraction.blanked &&
			    (fraction.n == 1 || fraction.n == -1)) {
				/* Remove "1".  */
				gsize p = fraction.nominator_start;
				gsize length = fraction.denominator_start - p - 1 -
					UNICODE_PI_number_of_bytes;
				blank_characters (dst, attrs, p, length, layout);
				fraction.denominator_start -= length - 1;
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
				 val, w, unicode_minus, numeral_shape,
				 shape_flags);
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
	g_printerr ("[%s] --> %d\n", str->str, w);
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

#define HANDLE_SIGN(i) do {						\
		if (unicode_minus ||					\
		    ((shape_flags & GO_FMT_SHAPE_SIGNS)			\
		     && (num_shape > 1)))				\
			convert_sign (str, (i), num_shape, shape_flags); \
	} while (0)

#define HANDLE_ESIGN(i) do {			\
	const char *e = strchr (str->str, 'E');	\
	if (e)					\
		HANDLE_SIGN (e - str->str + 1);	\
} while (0)


#define HANDLE_NUMERAL_SHAPE		                                \
	do {								\
	if (num_shape > 1) {						\
		/* 0: not set; 1: Western */				\
		handle_chinese (str, NULL, num_shape,			\
				shape_flags | GO_FMT_POSITION_MARKERS);	\
		handle_ethiopic (str, NULL, num_shape,			\
				 shape_flags | GO_FMT_POSITION_MARKERS); \
		convert_numerals (str, 0, str->len - 1, num_shape);	\
	}								\
	} while (0)

#ifdef DEFINE_COMMON
static void
drop_zeroes (GString *str, int *prec)
{
	if (*prec == 0)
		return;

	while (str->str[str->len - 1] == '0') {
		g_string_truncate (str, str->len - 1);
		(*prec)--;
	}
	if (*prec == 0) {
		/* We got "xxxxxx.000" and dropped the zeroes.  */
		const char *dot = g_utf8_prev_char (str->str + str->len);
		g_string_truncate (str, dot - str->str);
	}
}

static gboolean
cheap_drop_decimal (GString *str, size_t pos)
{
	char l = str->str[pos];
	int carry;

	if (!g_ascii_isdigit (l) || l == '5')
		return FALSE;

	g_string_erase (str, pos, 1);
	pos--;

	carry = (l > '5');
	if (!carry && !g_ascii_isdigit (str->str[pos])) {
		const char *dot = g_utf8_prev_char (str->str + pos + 1);
		g_string_erase (str, dot - str->str, str->str + pos + 1 - dot);
#ifdef DEBUG_GENERAL
		g_printerr ("Removing decimal cheaply\n");
#endif
	}

	while (carry) {
		l = str->str[pos];
		if (!g_ascii_isdigit (l))
			return FALSE;

		if (l < '9') {
			str->str[pos] = l + 1;
			carry = 0;
		} else
			str->str[pos] = '0';
		pos--;
	}

#ifdef DEBUG_GENERAL
	g_printerr ("Cheaply dropped decimal\n");
#endif
	return TRUE;
}
#endif

static int
SUFFIX(ilog10) (DOUBLE x)
{
	if (x >= 1000)
		return (int)log10 (x);
	if (x >= 100)
		return 2;
	if (x >= 10)
		return 1;
	return 0;
}


/**
 * go_render_general:
 * @layout: Optional #PangoLayout, probably preseeded with font attribute.
 * @str: a GString to store (not append!) the resulting string in.
 * @measure: (scope call): Function to measure width of string/layout.
 * @metrics: Font metrics corresponding to @measure.
 * @val: floating-point value.  Must be finite.
 * @col_width: intended max width of layout in the units that @measure uses.
 * A width of -1 means no restriction.
 * @unicode_minus: Use unicode minuses, not hyphens.
 * @numeral_shape: numeral shape identifier.
 * @custom_shape_flags: flags for using @numeral_shape.
 *
 * Render a floating-point value into @layout in such a way that the
 * layouting width does not needlessly exceed @col_width.  Optionally
 * use unicode minus instead of hyphen.
 **/
void
SUFFIX(go_render_general) (PangoLayout *layout, GString *str,
			   GOFormatMeasure measure,
			   const GOFontMetrics *metrics,
			   DOUBLE val,
			   int col_width,
			   gboolean unicode_minus,
			   guint num_shape,
			   guint shape_flags)
{
	DOUBLE aval;
	int prec, safety, digs, digs_as_int;
	int maxdigits = SUFFIX(go_format_roundtrip_digits) - 1;
	size_t epos;
	gboolean rounds_to_0;
	int sign_width;
	int min_digit_width = metrics->min_digit_width;
	gboolean check_val = TRUE;
	gboolean try_needed = FALSE;

	if (num_shape > 0) {
		/* FIXME: We should adjust min_digit_width if num_shape != 0 */
	}

	sign_width = unicode_minus
		? metrics->minus_width
		: metrics->hyphen_width;

	if (col_width == -1) {
		try_needed = TRUE;
	} else {
		int w;

		w = (col_width - (val <= CONST(-0.5) ? sign_width : 0)) / min_digit_width;
		if (w <= maxdigits) {
			/* We're limited by width.  */
			maxdigits = w;
			check_val = FALSE;
		}

		try_needed = (w >= maxdigits);
	}

	if (try_needed) {
#ifdef DEBUG_GENERAL
		g_printerr ("Rendering %.20" FORMAT_G " to needed width\n", val);
#endif
		go_dtoa (str, "=!^" FORMAT_G, val);
		HANDLE_NUMERAL_SHAPE;
		HANDLE_SIGN (0);
		HANDLE_ESIGN ();
		SETUP_LAYOUT;
		if (col_width == -1 || measure (str, layout) <= col_width)
			return;
	}

#ifdef DEBUG_GENERAL
	g_printerr ("Rendering %.20" FORMAT_G " to width %d (<=%d digits)\n",
		 val, col_width, maxdigits);
#endif
	if (val == 0)
		goto zero;

	aval = SUFFIX(fabs) (val);
	if (aval >= CONST(1e15) || aval < CONST(1e-4))
		goto e_notation;

	/* Number of digits in round(aval).  */
	digs_as_int = (aval >= (DOUBLE)9.5 ? 1 + SUFFIX(ilog10) (aval + CONST(0.5)) : 1);

	/* Check if there is room for the whole part, including sign.  */
	safety = metrics->avg_digit_width / 2;

	if (digs_as_int * min_digit_width > col_width) {
#ifdef DEBUG_GENERAL
		g_printerr ("No room for whole part.\n");
#endif
		goto e_notation;
	} else if (digs_as_int * metrics->max_digit_width + safety <
		   col_width - (val > 0 ? 0 : sign_width)) {
#ifdef DEBUG_GENERAL
		g_printerr ("Room for whole part.\n");
#endif
		if (val == SUFFIX(floor) (val) || digs_as_int == maxdigits) {
#ifdef DEBUG_GENERAL
			g_printerr ("Integer or room for nothing else.\n");
#endif
			go_dtoa (str, "=^.0" FORMAT_f, val);
			HANDLE_NUMERAL_SHAPE;
			HANDLE_SIGN (0);
			SETUP_LAYOUT;
			return;
		}
	} else {
		int w;
#ifdef DEBUG_GENERAL
		g_printerr ("Maybe room for whole part.\n");
#endif

		go_dtoa (str, "=^.0" FORMAT_f, val);
		HANDLE_NUMERAL_SHAPE;
		HANDLE_SIGN (0);
		SETUP_LAYOUT;
		w = measure (str, layout);
		if (w > col_width)
			goto e_notation;

		if (val == SUFFIX(floor) (val) || digs_as_int == maxdigits) {
#ifdef DEBUG_GENERAL
			g_printerr ("Integer or room for nothing else.\n");
#endif
			return;
		}
	}

	/* Number of digits in [aval].  */
	digs = (aval >= 10 ? 1 + SUFFIX(ilog10) (aval) : 1);

	prec = maxdigits - digs;
#ifdef DEBUG_GENERAL
	g_printerr ("Starting with %d decimals\n", prec);
#endif
	go_dtoa (str, "=^.*" FORMAT_f, prec, val);
	if (check_val) {
		/*
		 * We're not width-limited; we may have to increase maxdigits
		 * by one.  This is a terribly wasteful way of doing this.
		 */
		if (val != STRTO(str->str, NULL)) {
			maxdigits++, prec++;
			go_dtoa (str, "=^.*" FORMAT_f, prec, val);
		}
	}
	HANDLE_NUMERAL_SHAPE;
	HANDLE_SIGN (0);
	drop_zeroes (str, &prec);

	while (prec > 0) {
		int w;

#ifdef DEBUG_GENERAL
		g_printerr ("Trying with %d decimals\n", prec);
#endif
		SETUP_LAYOUT;
		w = measure (str, layout);
		if (w <= col_width) {
#ifdef DEBUG_GENERAL
			g_printerr ("Success\n");
#endif
			return;
		}

		prec--;
		if (num_shape > 1 || !cheap_drop_decimal (str, str->len - 1)) {
			go_dtoa (str, "=^.*" FORMAT_f, prec, val);
			drop_zeroes (str, &prec);
			HANDLE_NUMERAL_SHAPE;
			HANDLE_SIGN (0);
		} else
			drop_zeroes (str, &prec);
	}

	SETUP_LAYOUT;
	return;

 e_notation:
#ifdef DEBUG_GENERAL
	g_printerr ("Trying E-notation\n");
#endif
	rounds_to_0 = (aval < CONST(0.5));
	prec = (col_width -
		(val >= 0 ? 0 : sign_width) -
		(aval < 1 ? sign_width : metrics->plus_width) -
		metrics->E_width) / min_digit_width - 3;
	if (prec <= 0) {
#ifdef DEBUG_GENERAL
		if (prec == 0)
			g_printerr ("Maybe room for E notation with no decimals.\n");
		else
			g_printerr ("No room for E notation.\n");
#endif
		/* Certainly too narrow for precision.  */
		if (prec == 0 || !rounds_to_0) {
			int w;

			go_dtoa (str, "=^.0" FORMAT_E, val);
			HANDLE_NUMERAL_SHAPE;
			HANDLE_SIGN (0);
			HANDLE_ESIGN ();
			SETUP_LAYOUT;
			if (!rounds_to_0)
				return;

			w = measure (str, layout);
			if (w <= col_width)
				return;
		}

		goto zero;
	}
	prec = MIN (prec, DOUBLE_DIG - 1);
	go_dtoa (str, "=^.*" FORMAT_E, prec, val);
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

		HANDLE_NUMERAL_SHAPE;
		HANDLE_SIGN (0);
		epos = strchr (str->str + prec + 1, 'E') - str->str;
		HANDLE_SIGN (epos + 1);
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
		go_dtoa (str, "=^.*" FORMAT_E, prec, val);
	}

	if (rounds_to_0)
		goto zero;

	SETUP_LAYOUT;
	return;

 zero:
#ifdef DEBUG_GENERAL
	g_printerr ("Zero.\n");
#endif
	g_string_assign (str, "0");
	SETUP_LAYOUT;
	return;
}

#define FREE_NEW_STR do { if (new_str) (void)g_string_free (new_str, TRUE); } while (0)

/**
 * go_format_value_gstring:
 * @layout: Optional PangoLayout, probably preseeded with font attribute.
 * @str: a GString to store (not append!) the resulting string in.
 * @measure: (scope call): Function to measure width of string/layout.
 * @metrics: Font metrics corresponding to @measure.
 * @fmt: #GOFormat
 * @val: floating-point value.  Must be finite.
 * @type: a format character
 * @sval: a string to append to @str after @val
 * @go_color: a color to render
 * @col_width: intended max width of layout in pango units.  -1 means
 *             no restriction.
 * @date_conv: #GODateConventions
 * @unicode_minus: Use unicode minuses, not hyphens.
 *
 * Render a floating-point value into @layout in such a way that the
 * layouting width does not needlessly exceed @col_width.  Optionally
 * use unicode minus instead of hyphen.
 * Returns: a #GOFormatNumberError
 **/
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
	GString *new_str =  NULL;
	GOFormatNumberError err;

	g_return_val_if_fail (type == 'F' || sval != NULL,
			      (GOFormatNumberError)-1);

	if (str == NULL)
		new_str = str = g_string_new (NULL);
	else
		g_string_truncate (str, 0);

	if (fmt)
		fmt = SUFFIX(go_format_specialize) (fmt, val, type, &inhibit);
	if (!fmt)
		fmt = go_format_general ();
	if (go_color)
		*go_color = fmt->color;

	if (layout && fmt->color != 0) {
		/*
		 * We ignore fully-transparent black, no-one should be able to
		 * specify that as a color anyway.  And it is invisible.
		 */
		PangoAttrList *attrs = pango_layout_get_attributes (layout);
		PangoAttribute *attr;
		attrs = attrs ? pango_attr_list_copy (attrs) : pango_attr_list_new ();
		attr = go_color_to_pango (fmt->color, TRUE);
		attr->start_index = 0;
		attr->end_index = G_MAXUINT;
		pango_attr_list_insert (attrs, attr);
		pango_layout_set_attributes (layout, attrs);
		pango_attr_list_unref (attrs);
	}

	if (type == 'F') {
		switch (fmt->typ) {
		case GO_FMT_TEXT:
			if (inhibit)
				val = SUFFIX(fabs)(val);
			SUFFIX(go_render_general)
				(layout, str, measure, metrics,
				 val,
				 col_width, unicode_minus, 0, 0);
			FREE_NEW_STR;
			return GO_FORMAT_NUMBER_OK;

		case GO_FMT_NUMBER:
			if (val < 0) {
#ifndef ALLOW_NEGATIVE_TIMES
				if (fmt->u.number.has_date ||
				    fmt->u.number.has_time) {
					FREE_NEW_STR;
					return GO_FORMAT_NUMBER_DATE_ERROR;
				}
#endif
				if (inhibit)
					val = SUFFIX(fabs)(val);
			}
#ifdef DEBUG_PROGRAMS
			g_printerr ("Executing %s\n", fmt->format);
			go_format_dump_program (fmt->u.number.program);
#endif

			err = SUFFIX(go_format_execute)
				(layout, str,
				 measure, metrics,
				 fmt->u.number.program,
				 col_width,
				 val, sval, date_conv,
				 unicode_minus);
			FREE_NEW_STR;
			return err;

		case GO_FMT_EMPTY:
			SETUP_LAYOUT;
			FREE_NEW_STR;
			return GO_FORMAT_NUMBER_OK;

		default:
		case GO_FMT_INVALID:
		case GO_FMT_MARKUP:
		case GO_FMT_COND:
			SETUP_LAYOUT;
			FREE_NEW_STR;
			return GO_FORMAT_NUMBER_INVALID_FORMAT;
		}
	} else {
		switch (fmt->typ) {
		case GO_FMT_TEXT:
			err = SUFFIX(go_format_execute)
				(layout, str,
				 measure, metrics,
				 fmt->u.text.program,
				 col_width,
				 val, sval, date_conv,
				 unicode_minus);
			FREE_NEW_STR;
			return err;
		case GO_FMT_NUMBER:
			g_string_assign (str, sval);
			SETUP_LAYOUT;
			FREE_NEW_STR;
			return GO_FORMAT_NUMBER_OK;

		case GO_FMT_EMPTY:
			SETUP_LAYOUT;
			FREE_NEW_STR;
			return GO_FORMAT_NUMBER_OK;

		default:
		case GO_FMT_INVALID:
		case GO_FMT_MARKUP:
		case GO_FMT_COND:
			SETUP_LAYOUT;
			FREE_NEW_STR;
			return GO_FORMAT_NUMBER_INVALID_FORMAT;
		}
	}
}

#undef FREE_NEW_STR

/**
 * go_format_value:
 * @fmt: a #GOFormat
 * @val: value to format
 *
 * Converts @val into a string using format specified by @fmt.
 *
 * Returns: (transfer full): formatted value.
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
		 val, 'F', NULL, NULL,
		 -1, NULL, FALSE);
	if (err) {
		/* Invalid number for format.  */
		g_string_assign (res, "#####");
	}
	return g_string_free (res, FALSE);
}



#ifdef DEFINE_COMMON
/**
 * _go_number_format_init: (skip)
 */
void
_go_number_format_init (void)
{
	double l10 = log10 (FLT_RADIX);
	go_format_roundtrip_digits =
		(int)ceil (DBL_MANT_DIG * l10) + (l10 != (int)l10);
#ifdef GOFFICE_WITH_LONG_DOUBLE
	go_format_roundtrip_digitsl =
		(int)ceil (LDBL_MANT_DIG * l10) + (l10 != (int)l10);
#endif

	style_format_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
		NULL, (GDestroyNotify) go_format_unref);
}
#endif

#ifdef DEFINE_COMMON
static void
cb_format_leak (G_GNUC_UNUSED gpointer key, gpointer value, G_GNUC_UNUSED gpointer user_data)
{
	GOFormat const *gf = value;
	if (gf->ref_count != 1)
		g_printerr ("Leaking GOFormat at %p [%s].\n", gf, gf->format);
}
#endif

#ifdef DEFINE_COMMON
/**
 * _go_number_format_shutdown: (skip)
 */
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
 * @str: A *valid* format string
 *
 * Localizes the given format string, i.e., changes decimal dots to the
 * locale's notion of that and performs other such transformations.
 *
 * Returns: (transfer full) (nullable): a localized format string, or
 * %NULL if the format was not valid.
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
 * @str: A *valid* localized format string
 *
 * De-localizes the given format string, i.e., changes locale's decimal
 * separators to dots and performs other such transformations.
 *
 * Returns: (transfer full) (nullable): a non-local format string, or
 * %NULL if the format was not valid.
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

			case TOK_ESCAPED_CHAR:
				if (ti->tstr[1] == ' ') {
					g_string_append_c (res, ' ');
					break;
				}
				/* no break */
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
 * go_format_inc_precision:
 * @fmt: #GOFormat
 *
 * Increases the displayed precision for @fmt by one digit.
 *
 * Returns: (transfer full) (nullable): New format, or %NULL if the format
 * would not change.
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
 * go_format_dec_precision:
 * @fmt: #GOFormat
 *
 * Decreases the displayed precision for @fmt by one digit.
 *
 * Returns: (transfer full) (nullable): New format, or %NULL if the format
 * would not change.
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
	case PANGO_ATTR_FAMILY:
		g_string_append_printf (accum, "[family=%s",
			((PangoAttrString *)a)->value);
		break;
	case PANGO_ATTR_SIZE:
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
	case PANGO_ATTR_STYLE:
		g_string_append_printf (accum, "[italic=%d",
			(((PangoAttrInt *)a)->value == PANGO_STYLE_ITALIC) ? 1 : 0);
		break;
	case PANGO_ATTR_WEIGHT: {
		int w = ((PangoAttrInt *)a)->value;
		/* We are scaling the weight so that earlier versions that used only 0/1 */
		/* can still read the new formats and we can read the old ones. */
		const int Z = PANGO_WEIGHT_NORMAL;
		const double R = PANGO_WEIGHT_BOLD - Z;
		double d = (w - Z) / R;
		/* We cap to prevent older versions from seeing -1 or less.  */
		if (d <= -1) d = -0.999;
		g_string_append (accum, "[bold=");
		g_ascii_formatd (buf, sizeof (buf), "%.3g", d);
		g_string_append (accum, buf);
		break;
	}
	case PANGO_ATTR_STRIKETHROUGH:
		g_string_append_printf (accum, "[strikethrough=%d",
			((PangoAttrInt *)a)->value ? 1 : 0);
		break;
	case PANGO_ATTR_UNDERLINE:
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

	case PANGO_ATTR_FOREGROUND:
		c = &((PangoAttrColor *)a)->color;
		g_string_append_printf (accum, "[color=%02xx%02xx%02x",
			((c->red & 0xff00) >> 8),
			((c->green & 0xff00) >> 8),
			((c->blue & 0xff00) >> 8));
		break;
	default:
		if (a->klass->type == go_pango_attr_subscript_get_attr_type ()) {
			g_string_append_printf (accum, "[subscript=%d",
						((GOPangoAttrSubscript *)a)->val ?
						1:0);
			break;
		} else if (a->klass->type == go_pango_attr_superscript_get_attr_type ()) {
			g_string_append_printf (accum, "[superscript=%d",
						((GOPangoAttrSuperscript *)a)->val ?
						1:0);
			break;
		} else return FALSE; /* ignored */
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
			else if (0 == strncmp (str, "bold", 4)) {
				double d = g_ascii_strtod (val, NULL);
				const int Z = PANGO_WEIGHT_NORMAL;
				const double R = PANGO_WEIGHT_BOLD - Z;
				int w = (int)(d * R + Z);
				a = pango_attr_weight_new (w);
			} else if (0 == strncmp (str, "rise", 4))
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
			} else if (0 == strncmp (str, "subscript", 9))
				a = go_pango_attr_subscript_new (atoi (val));
			break;

		case 11:
			if (0 == strncmp (str, "superscript", 11))
				a = go_pango_attr_superscript_new (atoi (val));
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
 * go_format_new_from_XL:
 * @str: XL descriptor in UTF-8 encoding.
 *
 * Returns: (transfer full): A GOFormat matching @str.
 **/
GOFormat *
go_format_new_from_XL (char const *str)
{
	GOFormat *format;

	g_return_val_if_fail (str != NULL, go_format_general ());

	format = g_hash_table_lookup (style_format_hash, str);

	if (format == NULL) {
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
		} else
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
 * go_format_new_markup:
 * @markup: #PangoAttrList
 * @add_ref: boolean
 *
 * If @add_ref is %FALSE absorb the reference to @markup, otherwise add a
 * reference.
 *
 * Returns: (transfer full): A new format.
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
 * Returns: (transfer none): the XL style format strint.
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
 * go_format_ref:
 * @fmt: a #GOFormat
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
 * go_format_unref:
 * @fmt: (transfer full) (nullable): a #GOFormat
 *
 * Removes a reference to @fmt, freeing when it goes to zero.
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
	if (gf->ref_count > 1)
		return;

	if (gf->ref_count == 1) {
		if (NULL != style_format_hash &&
		    gf_ == g_hash_table_lookup (style_format_hash, gf_->format))
			g_hash_table_remove (style_format_hash, gf_->format);
		return;
	}

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
GType
go_format_get_type (void)
{
	static GType t = 0;

	if (t == 0)
		t = g_boxed_type_register_static ("GOFormat",
			 (GBoxedCopyFunc)go_format_ref,
			 (GBoxedFreeFunc)go_format_unref);
	return t;
}
#endif


#ifdef DEFINE_COMMON
/**
 * go_format_is_invalid:
 * @fmt: Format to query
 *
 * Returns: %TRUE if, and if only, the format is invalid
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
 * go_format_is_general:
 * @fmt: Format to query
 *
 * Returns: %TRUE if the format is "General", possibly with condition,
 * 	color, and/or locale.  ("xGeneral" is thus not considered to be General
 * 	for the purpose of this function.)
 *	Returns %FALSE otherwise.
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
 * go_format_is_markup:
 * @fmt: Format to query
 *
 * Returns: %TRUE if the format is a markup format and %FALSE otherwise.
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
 * go_format_is_text:
 * @fmt: Format to query
 *
 * Returns: %TRUE if the format is a text format and %FALSE otherwise.
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
 * go_format_is_var_width:
 * @fmt: Format to query
 *
 * Returns: %TRUE if the format is variable width, i.e., can stretch.
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
 * go_format_has_year:
 * @fmt: Format to query
 *
 * Returns: %TRUE if format is a number format with a year specifier
 * 	    %FALSE otherwise.
 **/
gboolean
go_format_has_year (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, FALSE);

	return (fmt->typ == GO_FMT_NUMBER &&
		fmt->u.number.has_year);
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_has_month:
 * @fmt: Format to query
 *
 * Returns: %TRUE if format is a number format with a year specifier
 * 	    %FALSE otherwise.
 **/
gboolean
go_format_has_month (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, FALSE);

	return (fmt->typ == GO_FMT_NUMBER &&
		fmt->u.number.has_month);
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_has_day:
 * @fmt: Format to query
 *
 * Returns: %TRUE if format is a number format with a day-of-month specifier
 * 	    %FALSE otherwise.
 **/
gboolean
go_format_has_day (GOFormat const *fmt)
{
	g_return_val_if_fail (fmt != NULL, FALSE);

	return (fmt->typ == GO_FMT_NUMBER &&
		fmt->u.number.has_day);
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_has_hour:
 * @fmt: Format to query
 *
 * Returns: %TRUE if format is a number format with an hour specifier
 * 	    %FALSE otherwise.
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
 * Returns: %TRUE if format is a number format with a minute specifier
 * 	    %FALSE otherwise.
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


/**
 * go_format_specialize:
 * @fmt: the format to specialize
 * @val: the value to use
 * @type: the type of value; 'F' for numeric, 'B' for boolean, 'S' for string.
 * @inhibit_minus: (out): set to %TRUE if the format dictates that a minus
 * should be inhibited when rendering negative values.
 *
 * Returns: (transfer none): @fmt format, presumably a conditional format,
 * specialized to @value of @type.
 */
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
			cond = (is_number && val == (DOUBLE)c->val);
			break;
		case GO_FMT_COND_NE:
			cond = (is_number && val != (DOUBLE)c->val);
			break;
		case GO_FMT_COND_LT:
			cond = (is_number && val <  (DOUBLE)c->val);
			break;
		case GO_FMT_COND_LE:
			cond = (is_number && val <= (DOUBLE)c->val);
			break;
		case GO_FMT_COND_GT:
			cond = (is_number && val >  (DOUBLE)c->val);
			break;
		case GO_FMT_COND_GE:
			cond = (is_number && val >= (DOUBLE)c->val);
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
/**
 * go_format_general:
 *
 * Returns: (transfer none): the general format
 */
GOFormat *
go_format_general (void)
{
	if (!default_general_fmt)
		default_general_fmt = go_format_new_from_XL (
			_go_format_builtins (GO_FORMAT_GENERAL)[0]);
	return default_general_fmt;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_empty:
 *
 * Returns: (transfer none): the empty format
 */
GOFormat *
go_format_empty (void)
{
	if (!default_empty_fmt)
		default_empty_fmt = go_format_new_from_XL ("");
	return default_empty_fmt;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_default_date:
 *
 * Returns: (transfer none): the default date format
 */
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
/**
 * go_format_default_time:
 *
 * Returns: (transfer none): the default time format
 */
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
/**
 * go_format_default_date_time:
 *
 * Returns: (transfer none): the default date-and-time format
 */
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
/**
 * go_format_default_percentage:
 *
 * Returns: (transfer none): the default percentage format
 */
GOFormat *
go_format_default_percentage (void)
{
	if (!default_percentage_fmt)
		default_percentage_fmt = go_format_new_from_XL (
			_go_format_builtins (GO_FORMAT_PERCENTAGE) [1]);
	return default_percentage_fmt;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_default_money:
 *
 * Returns: (transfer none): the default monetary format
 */
GOFormat *
go_format_default_money (void)
{
	if (!default_money_fmt)
		default_money_fmt = go_format_new_from_XL (
			_go_format_builtins (GO_FORMAT_CURRENCY)[2]);
	return default_money_fmt;
}
#endif

#ifdef DEFINE_COMMON
/**
 * go_format_default_accounting:
 *
 * Returns: (transfer none): the default accounting format
 */
GOFormat *
go_format_default_accounting (void)
{
	if (!default_accounting_fmt)
		default_accounting_fmt = go_format_new_from_XL (
			_go_format_builtins (GO_FORMAT_ACCOUNTING)[2]);
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
	int digits;

	go_string_append_c_n (dst, '#', step - 1);
	if (details->simplify_mantissa)
		g_string_append_c (dst, '#');
	else
		g_string_append_c (dst, '0');

	if (num_decimals > 0) {
		g_string_append_c (dst, '.');
		go_string_append_c_n (dst, '0', num_decimals);
	}
	if (details->scale == 3)
		g_string_append_c (dst, '\'');
	if (details->use_markup)
		g_string_append_c (dst, 'E');
	g_string_append_c (dst, 'E');
	if (details->append_SI)
		g_string_append_len (dst, "SI", 2);

	g_string_append_c (dst, details->exponent_sign_forced ? '+' : '-');
	/* Maximum not terribly important. */
	digits = CLAMP (details->exponent_digits, 1, 10);
	go_string_append_c_n (dst, '0', digits);

	if (details->append_SI && details->appended_SI_unit != NULL) {
		g_string_append_c (dst, '\"');
		g_string_append (dst, details->appended_SI_unit);
		g_string_append_c (dst, '\"');
	}
}
#endif

#ifdef DEFINE_COMMON
static void
go_format_generate_fraction_str (GString *dst, GOFormatDetails const *details)
{
	/* Maximum not terribly important. */
	int numerator_min_digits = CLAMP (details->numerator_min_digits, 0, 30);
	int denominator_max_digits = CLAMP (details->denominator_max_digits, 1, 30);
	int denominator_min_digits = CLAMP (details->denominator_min_digits, 0, denominator_max_digits);
	int denominator = CLAMP (details->denominator, 2, G_MAXINT);
	int num_digits;

	if (details->split_fraction) {
		/* Maximum not terribly important. */
		int min_digits = CLAMP (details->min_digits, 0, 30);
		if (min_digits > 0)
			go_string_append_c_n (dst, '0', min_digits);
		else
			g_string_append_c (dst, '#');
		g_string_append_c (dst, ' ');
	}

	if (details->automatic_denominator)
		num_digits = denominator_max_digits - numerator_min_digits;
	else
		num_digits = cnt_digits (denominator) - numerator_min_digits;

	if (num_digits > 0)
		go_string_append_c_n (dst, '?', num_digits);
	if  (numerator_min_digits > 0)
		go_string_append_c_n (dst, '0', numerator_min_digits);

	if (details->pi_scale)
		g_string_append (dst, " pi/");
	else
		g_string_append_c (dst, '/');

	if (details->automatic_denominator) {
		go_string_append_c_n (dst, '?',
				      denominator_max_digits - denominator_min_digits);
		go_string_append_c_n (dst, '0',
				      denominator_min_digits);
	} else
		g_string_append_printf (dst, "%d", denominator);
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
		currency = _go_format_currencies ();

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
		currency = _go_format_currencies ();

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
		return NULL;

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

	for (i = 1; _go_format_currencies ()[i].symbol; i++) {
		const GOFormatCurrency *ci = _go_format_currencies() + i;

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
	case GO_FORMAT_FRACTION:
		go_format_generate_fraction_str (dst, details);
		break;
	default:
		break;
	}
}
#endif

#ifdef DEFINE_COMMON
static GOFormatDetails *
go_format_details_ref (GOFormatDetails * details)
{
	details->ref_count++;
	return details;
}

GType
go_format_details_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOFormatDetails",
			 (GBoxedCopyFunc)go_format_details_ref,
			 (GBoxedFreeFunc)go_format_details_free);
	}
	return t;
}

GOFormatDetails *
go_format_details_new (GOFormatFamily family)
{
	GOFormatDetails *res = g_new (GOFormatDetails, 1);
	go_format_details_init (res, family);
	res->ref_count = 1;
	return res;
}
#endif


#ifdef DEFINE_COMMON
void
go_format_details_finalize (GOFormatDetails *details)
{
	g_free (details->appended_SI_unit);
	details->appended_SI_unit = NULL;
	/* We do not own ->currency.  */
}
#endif

#ifdef DEFINE_COMMON
void
go_format_details_free (GOFormatDetails *details)
{
	if (!details || details->ref_count-- > 1)
		return;
	go_format_details_finalize (details);
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
	details->exponent_sign_forced = FALSE;
	details->exponent_step = 1;
	details->exponent_digits = 2;
	details->min_digits = (family == GO_FORMAT_FRACTION) ? 0 : 1;
	details->split_fraction = TRUE;
	details->denominator_max_digits = 1;
	details->denominator = 8;
	details->automatic_denominator = TRUE;
}
#endif


#ifdef DEFINE_COMMON
/**
 * go_format_get_details:
 * @fmt: Format to quert
 * @dst: (out): #GOFormatDetails
 * @exact: (out) (optional): whether @dst describes @fmt exactly.
 */
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
			const char *epos = strchr (str, 'E');
			const char *scale_pos = strstr (str, "'E");
			const char *mend = dot ? dot : (scale_pos ? scale_pos : epos);
			dst->append_SI = (strstr (str, "ESI") != NULL);
			dst->use_markup = (strstr (str, "EE") != NULL);
			dst->scale = ((scale_pos != NULL) ? 3 : 0);
			dst->exponent_step = mend - str;
			dst->simplify_mantissa = mend != str && mend[-1] == '#';
			if (dst->simplify_mantissa)
				dst->min_digits = 0;

			dst->exponent_digits = 0;
			if (dst->use_markup) epos++;
			epos++;
			if (dst->use_markup)
				epos++;
			if (dst->append_SI)
				epos += 2;
			if (epos[0] == '+') {
				epos++;
				dst->exponent_sign_forced = TRUE;
			} else if (epos[0] == '-')
				epos++;
			while (epos[0] == '0' || epos[0] == '#' || epos[0] == '?') {
				epos++;
				dst->exponent_digits++;
			}
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

	case GO_FORMAT_FRACTION: {
		char *c_str = g_strdup (str), *p;
		gchar **tokens;
		int numerator_base;
		char const *integer;
		int d;
		gboolean pi_token;

		// Trim spaces
		for (p = c_str + strlen (c_str); p > c_str && p[-1] == ' '; p--)
			p[-1] = 0;
		p = c_str;
		while (p[0] == ' ')
			p++;

		// This is really a hack that doesn't take quoted string
		// into account.
		tokens = g_strsplit_set (c_str, " /", 3);

		/* Since it is a fraction we get at least 2 tokens */
		g_return_if_fail (tokens[1] != NULL);

		dst->pi_scale = (NULL != strstr (str, "pi/"));
		pi_token = (0 == strcmp (tokens[1], "pi"));

		dst->split_fraction = (tokens[2] != NULL) && !pi_token;

		if (dst->split_fraction) {
			integer = tokens[0];
			dst->min_digits = 0;
			while (*integer != 0)
				if (*integer++ == '0')
					dst->min_digits++;
			numerator_base = 1;
		} else
			numerator_base = 0;

		integer = tokens[numerator_base];
		dst->numerator_min_digits = 0;
		while (*integer != 0)
			if (*integer++ == '0')
				dst->numerator_min_digits++;

		integer = tokens[numerator_base + (pi_token ? 2 : 1)];
		d = atoi (integer);
		if (d > 1) {
			dst->denominator = d;
			dst->automatic_denominator = FALSE;
		} else {
			dst->automatic_denominator = TRUE;
			dst->denominator_min_digits = 0;
			dst->denominator_max_digits = 0;
			while (*integer != 0) {
				if (*integer == '#' || *integer == '?'
				    || g_ascii_isdigit (*integer))
					dst->denominator_max_digits++;
				if (*integer == '0') {
					dst->denominator_min_digits++;
				}
				integer++;
			}
		}

		g_strfreev (tokens);
		g_free (c_str);

		if (exact != NULL) {
			newstr = g_string_new (NULL);
			go_format_generate_str (newstr, dst);
		}

		break;
	}

	default:
		break;
	}

	if (newstr) {
		*exact = (strcmp (str, newstr->str) == 0);
		if (!*exact && dst->family == GO_FORMAT_SCIENTIFIC &&
		    dst->append_SI && g_str_has_prefix (str, newstr->str) &&
		    str [newstr->len] == '\"') {
			int len = strlen (str);
			if (str [len - 1] == '\"') {
				dst->appended_SI_unit = g_strndup (str + newstr->len + 1,
								   len - newstr->len - 2);
				g_string_truncate (newstr, 0);
				go_format_generate_str (newstr, dst);
				*exact = (strcmp (str, newstr->str) == 0);
			}
		}
		g_string_free (newstr, TRUE);
	}
}
#endif


#ifdef DEFINE_COMMON

/* making GOFormatCurrency a boxed type for introspection. ref and unref don't
 * do anything because we always return a static object */
static GOFormatCurrency *
go_format_currency_ref (GOFormatCurrency * currency)
{
	return currency;
}

static void
go_format_currency_unref (G_GNUC_UNUSED GOFormatCurrency * currency)
{
}

GType
go_format_currency_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOFormatCurrency",
			 (GBoxedCopyFunc)go_format_currency_ref,
			 (GBoxedFreeFunc)go_format_currency_unref);
	}
	return t;
}

/**
 * go_format_locale_currency:
 *
 * Returns: (transfer none): The #GOFormatCurrency matches the current locale.
 */
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
#define OFFICE   "office:"


#ifdef DEFINE_COMMON
char *
go_format_odf_style_map (GOFormat const *fmt, int cond_part)
{
	char const *format_string;
	GString *valstr;

	g_return_val_if_fail (fmt != NULL, NULL);
	g_return_val_if_fail (fmt->typ == GO_FMT_COND, NULL);

	if (cond_part > fmt->u.cond.n)
		return NULL;

	switch (fmt->u.cond.conditions[cond_part].op) {
	case GO_FMT_COND_EQ:
		format_string = "value()=";
		break;
	case GO_FMT_COND_NE:
		format_string = "value()!=";
		break;
	case GO_FMT_COND_NONTEXT: /* Under certain circumstances this */
                                  /*appears for second of two conditions */
	case GO_FMT_COND_LT:
		format_string = "value()<";
		break;
	case GO_FMT_COND_LE:
		format_string = "value()<=";
		break;
	case GO_FMT_COND_GT:
		format_string = "value()>";
		break;
	case GO_FMT_COND_GE:
		format_string = "value()>=";
		break;
	default:
		return NULL;
	}

	valstr = g_string_new (format_string);
	go_dtoa (valstr, "!g", fmt->u.cond.conditions[cond_part].val);
	return g_string_free (valstr, FALSE);
}
#endif

#ifdef DEFINE_COMMON

static void
odf_add_bool (GsfXMLOut *xout, char const *id, gboolean val)
{
	gsf_xml_out_add_cstr_unchecked (xout, id, val ? "true" : "false");
}

static void
odf_output_color (GsfXMLOut *xout, GOFormat const *fmt)
{
	/* Ignore transparent black. */
	if (fmt->color) {
		char *str = g_strdup_printf ("#%.2X%.2X%.2X",
					     GO_COLOR_UINT_R (fmt->color),
					     GO_COLOR_UINT_G (fmt->color),
					     GO_COLOR_UINT_B (fmt->color));

		gsf_xml_out_start_element (xout, STYLE "text-properties");
		gsf_xml_out_add_cstr_unchecked (xout, FOSTYLE "color", str);
		gsf_xml_out_end_element (xout); /*<style:text-properties>*/

		g_free (str);
	}
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
	odf_output_color (xout, fmt);

	while (1) {
		const char *token = xl;
		GOFormatTokenType tt;
		int t = go_format_token (&xl, &tt);

		switch (t) {
		case 0: case ';':
			ODF_CLOSE_STRING;
			if (!element_written)
				gsf_xml_out_simple_element (xout, NUMBER "am-pm", NULL);
			/* keep element open */
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
			if (seen_elapsed || seen_ampm) break;
			seen_ampm = TRUE;
			ODF_CLOSE_STRING;
			element_written = TRUE;
			gsf_xml_out_start_element (xout, NUMBER "am-pm");
			if (with_extension) {
				gchar *suffix = g_strndup (token, 1);
				gsf_xml_out_add_cstr (xout, GNMSTYLE "am-suffix", suffix);
				g_free (suffix);
				suffix = g_strndup (token + 2, 1);
				gsf_xml_out_add_cstr (xout, GNMSTYLE "pm-suffix", suffix);
				g_free (suffix);
			}		
			gsf_xml_out_end_element (xout); /* </number:am-pm> */
			break;

		case TOK_AMPM5:
			if (seen_elapsed || seen_ampm) break;
			seen_ampm = TRUE;
			ODF_CLOSE_STRING;
			element_written = TRUE;
			gsf_xml_out_start_element (xout, NUMBER "am-pm");
			if (with_extension) {
				gsf_xml_out_add_cstr (xout, GNMSTYLE "am-suffix", "AM");
				gsf_xml_out_add_cstr (xout, GNMSTYLE "pm-suffix", "PM");
			}			
			gsf_xml_out_end_element (xout); /* </number:am-pm> */
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

		case TOK_COLOR:
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
			if (t <= 0x7f) {
				ODF_OPEN_STRING;
				g_string_append_c (accum, t);
			}
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
	int odf_version = gsf_odf_out_get_version (GSF_ODF_OUT (xout));

	int int_digits = -1; /* -1 means no integer part */
	int min_numerator_digits = 0;
	int zeroes = 0;

	gboolean fraction_in_progress = FALSE;
	gboolean fraction_completed = FALSE;
	gboolean string_is_open = FALSE;
	gboolean pi_scale = FALSE;


	gsf_xml_out_start_element (xout, NUMBER "number-style");
	gsf_xml_out_add_cstr (xout, STYLE "name", name);
	odf_output_color (xout, fmt);

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
				else if (odf_version < 102) {
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
			/* keep element open */
			g_string_free (accum, TRUE);
			return;

		case '0':
			zeroes = 1;
			/* fall through */
		case '#': case '?': {
			int i = 1;
			char const *slash;
			char const *look;
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

			slash = strchr (xl, '/');
			if (slash) {
				for (look = xl; look < slash; look++)
					if (*look == '0' || *look == '?' || *look == '#')
						break;
				if (look < slash)
					int_digits = zeroes;
				else
					min_numerator_digits = zeroes;
				zeroes = 0;
				break;
			}
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
			else if (odf_version < 102) {
				gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", 0);
				if (with_extension)
					gsf_xml_out_add_cstr_unchecked
						(xout, GNMSTYLE "no-integer-part", "true");
			}
			/* In ODF1.2, absence of NUMBER "min-integer-digits" means not to show an       */
			/* integer part. In ODF 1.1 we used a foreign element: gnm:no-integer-part=true */
			gsf_xml_out_add_int (xout, NUMBER "min-numerator-digits",
					     min_numerator_digits);
			if (pi_scale && with_extension) {
				gsf_xml_out_add_cstr_unchecked (xout, GNMSTYLE "display-factor", "pi");
			}
			gsf_xml_out_end_element (xout); /* </number:fraction> */
			fraction_completed = TRUE;
			fraction_in_progress = FALSE;
			break;
		}

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
		case TOK_COLOR:
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

		case 'p':
			if (fraction_in_progress && g_str_has_prefix (token, "pi/")) {
				pi_scale = TRUE;
				xl++;
				break;
			}
			/* no break */
		default:
			if (t <= 0x7f) {
				ODF_OPEN_STRING;
				g_string_append_c (accum, t);
			}
			break;
		}
	}
}

#undef ODF_CLOSE_STRING
#undef ODF_OPEN_STRING

static void
go_format_output_number_element_to_odf (GsfXMLOut *xout,
					int min_integer_digits,
					int min_integer_chars,
					int min_decimal_digits,
					gboolean comma_seen,
					GSList *embedded,
					gboolean with_extension)
{
	GSList *l;

	gsf_xml_out_start_element (xout, NUMBER "number");
	gsf_xml_out_add_int (xout, NUMBER "decimal-places", min_decimal_digits);
	odf_add_bool (xout, NUMBER "grouping", comma_seen);
	gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", min_integer_digits);
	if (with_extension && (min_integer_chars > min_integer_digits))
		gsf_xml_out_add_int (xout, GNMSTYLE "min-integer-chars", min_integer_chars);

	embedded = g_slist_reverse (embedded);
	for (l = embedded; l; l = l->next->next) {
		int pos = GPOINTER_TO_INT (l->data);
		char *str = l->next->data;
		gsf_xml_out_start_element (xout, NUMBER "embedded-text");
		gsf_xml_out_add_int (xout, NUMBER "position", pos);
		gsf_xml_out_add_cstr (xout, NULL, str);
		gsf_xml_out_end_element (xout); /* </number:embedded-text> */
		g_free (str);
	}
	g_slist_free (embedded);

	gsf_xml_out_end_element (xout); /* </number:number> */
}

#define ODF_OPEN_STRING do {						\
	if (!string_is_open) {						\
		string_is_open = TRUE;					\
		gsf_xml_out_start_element (xout, NUMBER "text");	\
	}								\
} while (0)

#define ODF_CLOSE_STRING do {						\
	if (string_is_open) {						\
		string_is_open = FALSE;					\
		gsf_xml_out_end_element (xout); /* NUMBER "text" */	\
	}								\
} while (0)


#define ODF_FLUSH_STRING do {						\
	if (accum->len == 0) {						\
		/* Nothing */						\
	} else if (phase == 1) {					\
		if (allow_embedded) {					\
			embedded = g_slist_prepend			\
				(embedded,				\
				 GINT_TO_POINTER (digits));		\
			embedded = g_slist_prepend			\
				(embedded,				\
				 g_strdup (accum->str));		\
		}							\
	} else {							\
		ODF_OPEN_STRING;					\
		gsf_xml_out_add_cstr (xout, NULL, accum->str);		\
	}								\
	g_string_truncate (accum, 0);					\
	ODF_CLOSE_STRING;						\
} while (0)

/*
 * This is to be called when the decimal point (or its location) is reached.
 * The digit counts in embedded will be patched up to become the position
 * before the decimal point.
 */
static void
fixup_embedded (GSList *embedded, int digits)
{
	while (embedded) {
		int pos = digits - GPOINTER_TO_INT (embedded->next->data);
		embedded->next->data = GINT_TO_POINTER (pos);
		embedded = embedded->next->next;
	}
}

static void
go_format_output_number_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
				GOFormatFamily family,
				char const *name, gboolean with_extension)
{
	char const *xl = go_format_as_XL (fmt);
	int digits = 0;
	int min_integer_digits = 0;
	int min_integer_chars = 0;
	int min_decimal_places = 0;
	gboolean comma_seen = FALSE;
	GSList *embedded = NULL;
	GString *accum = g_string_new (NULL);
	/*
	 * 0: before number
	 * 1: in number, before dot
	 * 2: in number, after dot
	 * 3: after number
	 */
	int phase = 0;
	gboolean string_is_open = FALSE;
	gboolean allow_embedded;
	gboolean has_currency = FALSE;
	gboolean has_number = TRUE;
	GOFormatParseState state;
	const GOFormatParseState *pstate = &state;
	unsigned tno, tno_end;
	unsigned tno_phase1 = 0, tno_phase2 = 0, tno_phase3 = 0;

	switch (family) {
	case GO_FORMAT_TEXT:
		allow_embedded = FALSE;
		has_number = FALSE;
		gsf_xml_out_start_element (xout, NUMBER "text-style");
		break;
	case GO_FORMAT_PERCENTAGE:
		allow_embedded = TRUE;
		gsf_xml_out_start_element (xout, NUMBER "percentage-style");
		break;
	case GO_FORMAT_ACCOUNTING:
	case GO_FORMAT_CURRENCY:
		allow_embedded = FALSE;
		has_currency = TRUE;
		gsf_xml_out_start_element (xout, NUMBER "currency-style");
		break;
	case GO_FORMAT_NUMBER:
		allow_embedded = TRUE;
		gsf_xml_out_start_element (xout, NUMBER "number-style");
		break;
	default:
		g_assert_not_reached ();
	}
	gsf_xml_out_add_cstr (xout, STYLE "name", name);

	odf_output_color (xout, fmt);

	memset (&state, 0, sizeof (state));
	(void)go_format_preparse (xl, &state, TRUE, FALSE);

	/* Find the start of each phase.  */
	for (tno = 0; tno < state.tokens->len; tno++) {
		const GOFormatParseItem *ti = &GET_TOKEN(tno);

		switch (ti->token) {
		case TOK_DECIMAL:
			tno_phase2 = tno + 1;
			/* Fall through */
		case '0': case '?': case '#':
		case '@':
			if (!tno_phase3)
				tno_phase1 = tno;
			tno_phase3 = tno + 1;
			break;
		default:
			break;
		}
	}
	if (!tno_phase2)
		tno_phase2 = tno_phase3;

	/* Add terminator token */
	tno_end = pstate->tokens->len;
	g_array_set_size (pstate->tokens, tno_end + 1);
	GET_TOKEN(tno_end).tstr = "";
	GET_TOKEN(tno_end).token = 0;
	GET_TOKEN(tno_end).tt = 0;

	for (tno = 0; tno < state.tokens->len; tno++) {
		const GOFormatParseItem *ti = &GET_TOKEN(tno);
		const char *tstr = ti->tstr;

		if (phase == 0 && tno >= tno_phase1) {
			ODF_FLUSH_STRING;
			phase = 1;
		}
		if (phase == 1 && tno >= tno_phase2) {
			ODF_FLUSH_STRING;
			fixup_embedded (embedded, digits);
			phase = 2;
		}
		if (phase == 2 && tno >= tno_phase3) {
			ODF_FLUSH_STRING;
			if (has_number) {
				go_format_output_number_element_to_odf
					(xout,
					 min_integer_digits, min_integer_chars,
					 min_decimal_places,
					 comma_seen, embedded,
					 with_extension);
				embedded = NULL;
			}
			phase = 3;
		}

		switch (ti->token) {
		case 0:
			ODF_FLUSH_STRING;
			g_array_free (state.tokens, TRUE);
			/* keep element open */
			g_string_free (accum, TRUE);
			return;

		case '@':
			gsf_xml_out_simple_element (xout, NUMBER "text-content", NULL);
			break;

		case TOK_DECIMAL:
			/* Nothing special */
			break;

		case TOK_THOUSAND:
			comma_seen = TRUE;
			break;

		case '?':
			ODF_FLUSH_STRING;
			if (phase != 2)
				min_integer_chars++;
			break;

		case '0':
			ODF_FLUSH_STRING;
			if (phase == 2)
				min_decimal_places++;
			else {
				min_integer_digits++;
				min_integer_chars++;
			}
			digits++;
			break;

		case '#':
			ODF_FLUSH_STRING;
			digits++;
			break;

		case TOK_COLOR:
		case TOK_CONDITION:
			/* Handled elsewhere */
			break;

		default:
			if (ti->token <= 0x7f) {
				g_string_append_c (accum, *tstr);
				break;
			}
			/* Fall through */

		case 'd': case 'D':
		case 'y': case 'Y':
		case 'b': case 'B':
		case 'e': case 'E':
		case 'g': case 'G':
		case 'h': case 'H':
		case 'm': case 'M':
		case 's': case 'S':
		case ';':
		case '/':
		case TOK_AMPM3:
		case TOK_AMPM5:
		case TOK_ELAPSED_H:
		case TOK_ELAPSED_M:
		case TOK_ELAPSED_S:
		case TOK_GENERAL:
		case TOK_ERROR:
			g_printerr ("Unexpected token: %d\n", ti->token);
			break;

		case TOK_REPEATED_CHAR: {
			size_t len = g_utf8_next_char (tstr + 1) - (tstr + 1);
			char *text;

			if (!with_extension)
				break;

			if (phase == 1 || phase == 2)
				break; /* Don't know what to do */

			/* Flush visible prior contents */
			ODF_OPEN_STRING;
			gsf_xml_out_add_cstr (xout, NULL, accum->str);
			g_string_truncate (accum, 0);

			text = g_strndup (tstr + 1, len);
			gsf_xml_out_start_element (xout, GNMSTYLE "repeated");
			gsf_xml_out_add_cstr (xout, NULL, text);
			gsf_xml_out_end_element (xout); /* </gnm:repeated> */
			g_free (text);

			break;
		}

		case TOK_INVISIBLE_CHAR:
			if ((phase == 1 || phase == 2) || !with_extension) {
				g_string_append_c (accum, ' ');
			} else {
				size_t len = g_utf8_next_char (tstr + 1) - (tstr + 1);
				gchar *text = g_strndup (tstr + 1, len);

				/* Flush visible prior contents */
				ODF_OPEN_STRING;
				gsf_xml_out_add_cstr (xout, NULL, accum->str);
				g_string_truncate (accum, 0);

				/*
				 * Readers that do not understand gnm:invisible will
				 * see this space.  Readers that do understand the tag
				 * will reach back and replace this space.
				 */
				gsf_xml_out_add_cstr (xout, NULL, " ");

				gsf_xml_out_start_element (xout, GNMSTYLE "invisible");
				gsf_xml_out_add_cstr (xout, GNMSTYLE  "char", text);
				odf_add_bool (xout, OFFICE "process-content", TRUE);
				gsf_xml_out_end_element (xout); /* </gnm:invisible> */
				g_free (text);
			}
			break;

		case TOK_STRING: {
			size_t len = strchr (tstr + 1, '"') - (tstr + 1);
			g_string_append_len (accum, tstr + 1, len);
			break;
		}

		case '$':
		case TOK_CHAR: {
			size_t len = g_utf8_next_char (tstr) - tstr;
			gunichar uc = has_currency
				? g_utf8_get_char (tstr)
				: 0;
			switch (uc) {
			case '$':
			case UNICODE_POUNDS1:
			case UNICODE_POUNDS2:
			case UNICODE_YEN:
			case UNICODE_YEN_WIDE:
			case UNICODE_EURO: {
				char *str = g_strndup (tstr, len);
				ODF_FLUSH_STRING;
				gsf_xml_out_simple_element (xout, NUMBER "currency-symbol", str);
				g_free (str);
				break;
			}

			default:
				g_string_append_len (accum, tstr, len);
			}
			break;
		}

#if 0
		case TOK_LOCALE:
			ODF_WRITE_NUMBER;
			if (has_currency && (*str == '[') && (*(tstr + 1) == '$')) {
				int len;
				tstr += 2;
				len = strcspn (tstr, "-]");
				if (len > 0) {
					char *str;
					ODF_CLOSE_STRING;
					str = g_strndup (tstr, len);
					gsf_xml_out_simple_element (xout, NUMBER "currency-symbol", str);
					g_free (str);
				}
			}
			break;
#endif

		case TOK_ESCAPED_CHAR: {
			size_t len = g_utf8_next_char (tstr + 1) - (tstr + 1);
			g_string_append_len (accum, tstr + 1, len);
			break;
		}
		}
	}

	g_assert_not_reached ();
}

#undef ODF_FLUSH_STRING
#undef ODF_OPEN_STRING
#undef ODF_CLOSE_STRING



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
						   gboolean forced_exponent_sign,
						   gboolean engineering,
						   gboolean use_literal_E,
						   gboolean with_extension)
{
	gsf_xml_out_start_element (xout, NUMBER "scientific-number");
	gsf_xml_out_add_int (xout, NUMBER "decimal-places", min_decimal_digits);
	odf_add_bool (xout, NUMBER "grouping", comma_seen);
	gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", min_integer_digits);
	gsf_xml_out_add_int (xout, NUMBER "min-exponent-digits", min_exponent_digits);
	if (with_extension) {
		odf_add_bool (xout, GNMSTYLE "forced-exponent-sign", forced_exponent_sign);
		odf_add_bool (xout, GNMSTYLE "engineering", engineering);
		odf_add_bool (xout, GNMSTYLE "literal-E", use_literal_E);
	}
	gsf_xml_out_end_element (xout); /* </number:scientific-number> */
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

	gboolean forced_exponent_sign = FALSE;
	gboolean number_completed = FALSE;
	gboolean string_is_open = FALSE;

	gsf_xml_out_start_element (xout, NUMBER "number-style");
	gsf_xml_out_add_cstr (xout, STYLE "name", name);
	odf_output_color (xout, fmt);

	while (1) {
		const char *token = xl;
		GOFormatTokenType tt;
		int t = go_format_token (&xl, &tt);

		switch (t) {
		case 0: case ';':
			ODF_CLOSE_STRING;
			/* keep element open */
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

#ifdef ALLOW_SI_APPEND
		case TOK_EXP_SI:
#endif
#ifdef ALLOW_EE_MARKUP
		case TOK_EXP_MU:
#ifdef ALLOW_SI_APPEND
		case TOK_EXP_MU_SI:
#endif
#endif
		case TOK_EXP: {
			gboolean use_literal_E = TRUE;

			if (number_completed)
				break;

#ifdef ALLOW_EE_MARKUP
			if (t == TOK_EXP_MU)
				use_literal_E = FALSE;
#ifdef ALLOW_SI_APPEND
			if (t == TOK_EXP_MU_SI)
				use_literal_E = FALSE;
#endif
#endif

			if (*xl == '+') {
				forced_exponent_sign = TRUE;
				xl++;
			}
			if (*xl == '-')
				xl++;
			while (*xl == '0') {
				xl++;
				min_exponent_digits++;
			}
			go_format_output_scientific_number_element_to_odf
				(xout, min_integer_digits, min_decimal_digits,
				 min_exponent_digits, comma_seen,
				 forced_exponent_sign,
				 hashes > 0 && (hashes + min_integer_digits == 3),
				 use_literal_E, with_extension);
			number_completed = TRUE;
			break;
		}

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
		case TOK_COLOR:
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
			if (t <= 0x7f) {
				ODF_OPEN_STRING;
				g_string_append_c (accum, t);
			}
			break;
		}
	}
}

#undef ODF_CLOSE_STRING
#undef ODF_OPEN_STRING

static void
go_format_output_general_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
				 char const *name)
{
	gsf_xml_out_start_element (xout, NUMBER "number-style");
	gsf_xml_out_add_cstr (xout, STYLE "name", name);
	odf_output_color (xout, fmt);
	gsf_xml_out_start_element (xout, NUMBER "number");
	gsf_xml_out_add_int (xout, NUMBER "min-integer-digits", 1);
	gsf_xml_out_end_element (xout); /* </number:number> */
	gsf_xml_out_simple_element (xout, NUMBER "text", NULL);
	/* keep element open */
}

static gboolean
go_format_output_simple_to_odf (GsfXMLOut *xout, gboolean with_extension,
				GOFormat const *fmt, GOFormat const *parent_fmt,
				char const *name, gboolean keep_open)
{
	gboolean result = TRUE;
	GOFormatFamily family;
	GOFormatDetails details;
	gboolean exact;

	family = go_format_get_family (fmt);
	if (family == GO_FORMAT_UNKNOWN) {
		go_format_get_details (parent_fmt, &details, &exact);
	} else {
		go_format_get_details (fmt, &details, &exact);
	}
	family = details.family;

	switch (family) {
	case GO_FORMAT_GENERAL:
		go_format_output_general_to_odf (xout, fmt, name);
		result = FALSE;
		break;
	case GO_FORMAT_DATE:
		go_format_output_date_to_odf (xout, fmt, name, family, with_extension);
		break;
	case GO_FORMAT_TIME:
		go_format_output_date_to_odf (xout, fmt, name, family, with_extension);
		break;
	case GO_FORMAT_FRACTION:
		go_format_output_fraction_to_odf (xout, fmt, name, with_extension);
		break;
	case GO_FORMAT_SCIENTIFIC:
		go_format_output_scientific_number_to_odf (xout, fmt, name, with_extension);
		break;
	case GO_FORMAT_ACCOUNTING:
	case GO_FORMAT_CURRENCY:
	case GO_FORMAT_PERCENTAGE:
	case GO_FORMAT_NUMBER:
	case GO_FORMAT_TEXT:
		go_format_output_number_to_odf (xout, fmt, family, name, with_extension);
		break;
	default: {
		/* We need to output something and we don't need any details for this */
		int date = 0, digit = 0;
		char const *fstr, *str = go_format_as_XL (fmt);
		fstr = str;
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
			go_format_output_date_to_odf (xout, fmt, name,
						      GO_FORMAT_DATE, with_extension);
		else {
			/* We have a format that we can't identify */
			/* The following is really only appropriate for "" or so */
			gsf_xml_out_start_element (xout, NUMBER "number-style");
			gsf_xml_out_add_cstr (xout, STYLE "name", name);
			gsf_xml_out_start_element (xout, NUMBER "text");
			gsf_xml_out_add_cstr (xout, NULL, fstr);
			gsf_xml_out_end_element (xout);
		}
		result = FALSE;
		break;
	}
	}

	if (!keep_open)
		gsf_xml_out_end_element (xout);

	return result;
}

static void
go_format_output_conditional_to_odf (GsfXMLOut *xout, gboolean with_extension,
				     GOFormat const *fmt, char const *name)
{
	int i, N, defi = -1;
	GOFormatCondition const *cond0;
	int last_implicit_num = -1;

	g_return_if_fail (fmt->typ == GO_FMT_COND);

	N = fmt->u.cond.n;
	g_return_if_fail (N > 0);
	cond0 = &fmt->u.cond.conditions[0];
	g_return_if_fail (cond0->op >= GO_FMT_COND_EQ && cond0->op <= GO_FMT_COND_GE);

	/*
	 * Output all sub-formats, except perhaps one deemed the default.
	 */
	for (i = 0; i < fmt->u.cond.n; i++) {
		GOFormatCondition const *cond = &fmt->u.cond.conditions[i];

		if (cond->implicit && cond->op != GO_FMT_COND_TEXT)
			last_implicit_num = i;

		switch (cond->op) {
		case GO_FMT_COND_TEXT:
		case GO_FMT_COND_NONTEXT:
			if (defi != -1 || i >= 4)
				g_warning ("This shouldn't happen.");
			defi = i;
			break;
		case GO_FMT_COND_NONE:
			if (i < 4)
				g_warning ("This shouldn't happen.");
			if (defi == -1)
				defi = i;
			break;
		default: {
			char *partname = g_strdup_printf ("%s-%d", name, i);
			go_format_output_simple_to_odf (xout, with_extension,
							cond->fmt, fmt,
							partname, FALSE);
			g_free (partname);
		}
		}
	}

	if (defi == -1 && last_implicit_num != -1) {
		/*
		 * The default could be this one, but we cannot reorder
		 * conditions unless we can prove it doesn't matter.  Just
		 * check if it is last.
		 */
		if (last_implicit_num == fmt->u.cond.n - 1)
			defi = last_implicit_num;
	}

	if (defi == -1) {
		/* We don't have a default; make one.  */
		go_format_output_general_to_odf (xout, go_format_general (),
						 name);
	} else {
		go_format_output_simple_to_odf (xout, with_extension,
						fmt->u.cond.conditions[defi].fmt, fmt,
						name, TRUE);
	}

	for (i = 0; i < fmt->u.cond.n; i++) {
		GOFormatCondition const *cond = &fmt->u.cond.conditions[i];
		const char *oper = NULL;
		double val = cond->val;
		char *partname;
		GString *condition;

		if (i == defi)
			continue;

		switch (cond->op) {
		case GO_FMT_COND_TEXT:
		case GO_FMT_COND_NONE:
		case GO_FMT_COND_NONTEXT:
			/* Already handled or to be ignored.  */
			continue;

		case GO_FMT_COND_EQ: oper = "="; break;
		case GO_FMT_COND_NE: oper = "!="; break;
		case GO_FMT_COND_LT: oper = "<"; break;
		case GO_FMT_COND_LE: oper = "<="; break;
		case GO_FMT_COND_GT: oper = ">"; break;
		case GO_FMT_COND_GE: oper = ">="; break;
		}

		partname = g_strdup_printf ("%s-%d", name, i);

		condition = g_string_new ("value()");
		g_string_append (condition, oper);
		go_dtoa (condition, "!g", val);

		gsf_xml_out_start_element (xout, STYLE "map");
		gsf_xml_out_add_cstr (xout, STYLE "condition", condition->str);
		gsf_xml_out_add_cstr (xout, STYLE "apply-style-name", partname);
		gsf_xml_out_end_element (xout); /* </style:map> */

		g_free (partname);
		g_string_free (condition, TRUE);
	}

	gsf_xml_out_end_element (xout); /* </number:text-style> or </number:number-style> */
}

gboolean
go_format_output_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
			 G_GNUC_UNUSED int cond_part, char const *name,
			 gboolean with_extension)
{
	gboolean pp, result;

	g_object_get (G_OBJECT (xout), "pretty-print", &pp, NULL);
	if (pp) {
		/* We need to switch off pretty printing since number:text preserves whitespace */
		g_object_set (G_OBJECT (xout), "pretty-print", FALSE, NULL);
	}

	if (fmt->typ == GO_FMT_COND) {
		go_format_output_conditional_to_odf (xout, with_extension, fmt, name);
		result = TRUE;
	} else {
		result = go_format_output_simple_to_odf (xout, with_extension,
							 fmt, NULL, name, FALSE);
	}

	if (pp)
		g_object_set (G_OBJECT (xout), "pretty-print", pp, NULL);

	return result;
}
#endif

/* ------------------------------------------------------------------------- */

// See comments at top
#endif // SKIP_THIS_PASS
#if INCLUDE_PASS < INCLUDE_PASS_LAST
#include __FILE__
#endif
