/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-format.h : 
 *
 * Copyright (C) 2003-2005 Jody Goldberg (jody@gnome.org)
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
#ifndef GO_FORMAT_H
#define GO_FORMAT_H

#include <goffice/goffice.h>
#include <goffice/goffice-features.h>
#include <goffice/utils/goffice-utils.h>
#include <pango/pango-attributes.h>
#include <pango/pango-layout.h>
#include <glib.h>

#include <glib-object.h>

G_BEGIN_DECLS

/* Keep these sequential, they are used as the index for go_format_builtins */
typedef enum {
	GO_FORMAT_UNKNOWN	= -1,

	GO_FORMAT_GENERAL	= 0,
	GO_FORMAT_NUMBER	= 1,
	GO_FORMAT_CURRENCY	= 2,
	GO_FORMAT_ACCOUNTING	= 3,
	GO_FORMAT_DATE		= 4,
	GO_FORMAT_TIME		= 5,
	GO_FORMAT_PERCENTAGE	= 6,
	GO_FORMAT_FRACTION	= 7,
	GO_FORMAT_SCIENTIFIC	= 8,
	GO_FORMAT_TEXT		= 9,
	GO_FORMAT_SPECIAL	= 10,

	GO_FORMAT_MARKUP	= 11	/* Internal use only */
} GOFormatFamily;

typedef enum {
	GO_FORMAT_NUMBER_OK = 0,
	GO_FORMAT_NUMBER_INVALID_FORMAT,
	GO_FORMAT_NUMBER_DATE_ERROR
} GOFormatNumberError;


typedef struct {
	gboolean thousands_sep;
	int	 num_decimals;	/* 0 - 30 */
	int	 negative_fmt;	/* 0 - 3 */
	int	 currency_symbol_index;
	int	 list_element;
	int      fraction_denominator;
	gboolean simplify_mantissa;
	int	 exponent_step;
	gboolean use_markup;
} GOFormatDetails;

struct _GOFormat {
	int		 ref_count;
	char		*format;
        GSList		*entries;	/* Of type GOFormatElement. */
	GOFormatFamily	 family;
	GOFormatDetails	 family_info;
	gboolean	 is_var_width;
	PangoAttrList	*markup;	/* only for GO_FORMAT_MARKUP */
};

/*************************************************************************/

typedef int (*GOFormatMeasure) (const GString *str, PangoLayout *layout);
int go_format_measure_zero (const GString *str, PangoLayout *layout);
int go_format_measure_pango (const GString *str, PangoLayout *layout);
int go_format_measure_strlen (const GString *str, PangoLayout *layout);

void go_render_general  (PangoLayout *layout, GString *str,
			 GOFormatMeasure measure,
			 const GOFontMetrics *metrics,
			 double val,
			 int col_width,
			 gboolean unicode_minus);
#ifdef GOFFICE_WITH_LONG_DOUBLE
void go_render_generall (PangoLayout *layout, GString *str,
			 GOFormatMeasure measure,
			 const GOFontMetrics *metrics,
			 long double val,
			 int col_width,
			 gboolean unicode_minus);
#endif

/*************************************************************************/

GOFormat *go_format_new_from_XL		(char const *str, gboolean delocalize);
GOFormat *go_format_new			(GOFormatFamily family,
					 GOFormatDetails const *details);
GOFormat *go_format_new_markup		(PangoAttrList *markup, gboolean add_ref);

/* these do not add a reference to the result */
GOFormat *go_format_general	   	(void);
GOFormat *go_format_default_date	(void);
GOFormat *go_format_default_time	(void);
GOFormat *go_format_default_date_time	(void);
GOFormat *go_format_default_percentage	(void);
GOFormat *go_format_default_money	(void);

char	 *go_format_str_delocalize	(char const *str);
char   	 *go_format_str_as_XL	   	(char const *str, gboolean localized);
char   	 *go_format_as_XL	   	(GOFormat const *fmt, gboolean localized);

GOFormat *go_format_ref		 	(GOFormat *fmt);
void      go_format_unref		(GOFormat *fmt);

#define	  go_format_is_general(fmt)	((fmt)->family == GO_FORMAT_GENERAL)
#define   go_format_is_markup(fmt)	((fmt)->family == GO_FORMAT_MARKUP)
#define   go_format_is_text(fmt)	((fmt)->family == GO_FORMAT_TEXT)
#define   go_format_is_var_width(fmt)	((fmt)->is_var_width)
int       go_format_is_date             (GOFormat const *fmt);
int       go_format_is_date_for_value   (GOFormat const *fmt, double val, char type);
#ifdef GOFFICE_WITH_LONG_DOUBLE
int       go_format_is_date_for_valuel  (GOFormat const *fmt, long double val, char type);
#endif

GOFormatNumberError
go_format_value_gstring (PangoLayout *layout, GString *str,
			 const GOFormatMeasure measure,
			 const GOFontMetrics *metrics,
			 GOFormat const *format,
			 double val, char type, const char *sval,
			 GOColor *go_color,
			 int col_width,
			 GODateConventions const *date_conv,
			 gboolean unicode_minus);
char	*go_format_value (GOFormat const *fmt, double val);
#ifdef GOFFICE_WITH_LONG_DOUBLE
GOFormatNumberError
go_format_value_gstringl (PangoLayout *layout, GString *str,
			  const GOFormatMeasure measure,
			  const GOFontMetrics *metrics,
			  GOFormat const *format,
			  long double val, char type, const char *sval,
			  GOColor *go_color,
			  int col_width,
			  GODateConventions const *date_conv,
			  gboolean unicode_minus);
char	*go_format_valuel (GOFormat const *fmt, long double val);
#endif

gboolean go_format_eq			(GOFormat const *a, GOFormat const *b);
GOFormat *go_format_inc_precision	(GOFormat const *fmt);
GOFormat *go_format_dec_precision	(GOFormat const *fmt);
GOFormat *go_format_toggle_1000sep	(GOFormat const *fmt);
GOFormatFamily go_format_classify	(GOFormat const *fmt, GOFormatDetails *details);

typedef struct {
	gchar const *symbol;
	gchar const *description;
	gboolean precedes;
	gboolean has_space;
} GOFormatCurrency;

/* Indexed by GOFormatFamily */
GO_VAR_DECL char const * const * const go_format_builtins [];
GO_VAR_DECL GOFormatCurrency     const go_format_currencies [];

/*************************************************************************/

typedef struct {
	int  right_optional, right_spaces, right_req, right_allowed;
	int  left_optional, left_spaces, left_req;
	double scale;
	gboolean rendered;
	gboolean decimal_separator_seen;
	gboolean group_thousands;
	gboolean has_fraction;
	gboolean unicode_minus;

	gboolean exponent_seen;

	int exponent_digit_nbr;
	gboolean exponent_show_sign;
	gboolean exponent_lower_e;

	gboolean use_markup;
} GONumberFormat;

void go_render_number (GString *result, double number, GONumberFormat const *info);
#ifdef GOFFICE_WITH_LONG_DOUBLE
void go_render_numberl (GString *result, long double number, GONumberFormat const *info);
#endif

/* Locale support routines */
void	       go_set_untranslated_bools  (void);
char const *   go_setlocale               (int category, char const *val);
GString const *go_format_get_currency     (gboolean *precedes, gboolean *space_sep);
gboolean       go_format_month_before_day (void);
char           go_format_get_arg_sep      (void);
char           go_format_get_col_sep      (void);
char           go_format_get_row_sep      (void);
GString const *go_format_get_thousand     (void);
GString const *go_format_get_decimal      (void);
char const *   go_format_boolean          (gboolean b);

void go_number_format_init (void);
void go_number_format_shutdown (void);
void go_currency_date_format_init     (void);
void go_currency_date_format_shutdown (void);

G_END_DECLS

#endif /* GO_FORMAT_H */
