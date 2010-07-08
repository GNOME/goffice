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
#include <gsf/gsf-libxml.h>

G_BEGIN_DECLS

/* Keep these sequential, they are used as the index for _go_format_builtins */
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
	GO_FORMAT_MAGIC_NONE            = 0,
	GO_FORMAT_MAGIC_LONG_DATE       = 0xf800,    /* Official */
	GO_FORMAT_MAGIC_MEDIUM_DATE     = 0xf8f1,
	GO_FORMAT_MAGIC_SHORT_DATE      = 0xf8f2,
	GO_FORMAT_MAGIC_SHORT_DATETIME  = 0xf8fa,
	GO_FORMAT_MAGIC_LONG_TIME       = 0xf400,    /* Official */
	GO_FORMAT_MAGIC_MEDIUM_TIME     = 0xf4f1,
	GO_FORMAT_MAGIC_SHORT_TIME      = 0xf4f2
} GOFormatMagic;

typedef enum {
	GO_FORMAT_NUMBER_OK = 0,
	GO_FORMAT_NUMBER_INVALID_FORMAT,
	GO_FORMAT_NUMBER_DATE_ERROR
} GOFormatNumberError;

typedef struct {
	gchar const *symbol;
	gchar const *description;
	gboolean precedes;
	gboolean has_space;
} GOFormatCurrency;

typedef struct {
	GOFormatFamily family;
	GOFormatMagic magic;

	/* NUMBER, SCIENTIFIC, CURRENCY, ACCOUNTING, PERCENTAGE: */
	int min_digits;
	int num_decimals;

	/* NUMBER, CURRENCY, ACCOUNTING, PERCENTAGE: */
	gboolean thousands_sep;

	/* NUMBER, CURRENCY, ACCOUNTING, PERCENTAGE: */
	gboolean negative_red;
	gboolean negative_paren;

	/* CURRENCY, ACCOUNTING: */
	GOFormatCurrency const *currency;

	/* CURRENCY: */
	gboolean force_quoted;

	/* SCIENTIFIC: */
	int exponent_step;
	gboolean use_markup;
	gboolean simplify_mantissa;
} GOFormatDetails;

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

GOFormat *go_format_new_from_XL		(char const *str);
GOFormat *go_format_new_markup		(PangoAttrList *markup, gboolean add_ref);

/* these do not add a reference to the result */
GOFormat *go_format_general	   	(void);
GOFormat *go_format_empty               (void);
GOFormat *go_format_default_date	(void);
GOFormat *go_format_default_time	(void);
GOFormat *go_format_default_date_time	(void);
GOFormat *go_format_default_percentage	(void);
GOFormat *go_format_default_money	(void);
GOFormat *go_format_default_accounting  (void);

void      go_format_generate_number_str (GString *dst,
					 int min_digits,
					 int num_decimals,
					 gboolean thousands_sep,
					 gboolean negative_red,
					 gboolean negative_paren,
					 const char *prefix,
					 const char *postfix);
GOFormatDetails *go_format_details_new  (GOFormatFamily family);
void  go_format_details_free            (GOFormatDetails *details);
void  go_format_details_init            (GOFormatDetails *details,
					 GOFormatFamily family);
void  go_format_generate_str            (GString *dst,
					 GOFormatDetails const *details);

char     *go_format_str_localize        (char const *str);
char	 *go_format_str_delocalize	(char const *str);
const char* go_format_as_XL	   	(GOFormat const *fmt);

GOFormat *go_format_ref		 	(GOFormat const *fmt);
void      go_format_unref		(GOFormat const *fmt);

gboolean  go_format_is_invalid          (GOFormat const *fmt);
gboolean  go_format_is_general          (GOFormat const *fmt);
gboolean  go_format_is_markup           (GOFormat const *fmt);
gboolean  go_format_is_text             (GOFormat const *fmt);
gboolean  go_format_is_var_width        (GOFormat const *fmt);
int       go_format_is_date             (GOFormat const *fmt);
int       go_format_is_time             (GOFormat const *fmt);

int       go_format_month_before_day    (GOFormat const *fmt);
gboolean  go_format_has_hour            (GOFormat const *fmt);
gboolean  go_format_has_minute          (GOFormat const *fmt);

GOFormatMagic go_format_get_magic       (GOFormat const *fmt);
GOFormat *go_format_new_magic           (GOFormatMagic m);

const GOFormat *go_format_specialize          (GOFormat const *fmt,
					       double val, char type,
					       gboolean *inhibit_minus);
#ifdef GOFFICE_WITH_LONG_DOUBLE
const GOFormat *go_format_specializel         (GOFormat const *fmt,
					       long double val, char type,
					       gboolean *inhibit_minus);
#endif

GOFormatFamily         go_format_get_family (GOFormat const *fmt);

void      go_format_get_details         (GOFormat const *fmt,
					 GOFormatDetails *dst,
					 gboolean *exact);

const PangoAttrList *go_format_get_markup (GOFormat const *fmt);

gboolean go_format_is_simple (GOFormat const *fmt);

GOFormatNumberError
go_format_value_gstring (PangoLayout *layout, GString *str,
			 const GOFormatMeasure measure,
			 const GOFontMetrics *metrics,
			 GOFormat const *fmt,
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
			  GOFormat const *fmt,
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

/******************* GOFormat ODF Support ********************************/

char *go_format_odf_style_map (GOFormat const *fmt, int cond_part);
gboolean go_format_output_to_odf (GsfXMLOut *xout, GOFormat const *fmt,
				  int cond_part, char const *name,
				  gboolean with_extension);


/*************************************************************************/

/* Indexed by GOFormatFamily */
GO_VAR_DECL char const * const * const _go_format_builtins [];
GO_VAR_DECL GOFormatCurrency     const _go_format_currencies [];

GOFormatCurrency const *go_format_locale_currency (void);

/*************************************************************************/

void _go_number_format_init (void);
void _go_number_format_shutdown (void);
void _go_currency_date_format_init     (void);
void _go_currency_date_format_shutdown (void);

G_END_DECLS

#endif /* GO_FORMAT_H */
