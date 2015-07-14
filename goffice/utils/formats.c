/*
 * formats.c: The default formats supported in Gnumeric
 *
 * For information on how to translate these format strings properly,
 * refer to the doc/translating.sgml file in the Gnumeric distribution.
 *
 * Author:
 *    Miguel de Icaza (miguel@kernel.org)
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
#include "go-format.h"
#include "go-locale.h"
#include <goffice/utils/regutf8.h>
#include <glib/gi18n-lib.h>
#include <string.h>
#include <stdlib.h>

/* The various formats */
static char const * const
fmts_general [] = {
	"General",
	NULL
};

static char const * const
fmts_number [] = {
	"0",
	"0.00",
	"#,##0",
	"#,##0.00",
	"#,##0_);(#,##0)",
	"#,##0_);[Red](#,##0)",
	"#,##0.00_);(#,##0.00)",
	"#,##0.00_);[Red](#,##0.00)",
	"0.0",
	NULL
};

/* Some are generated */
static char const *
fmts_currency [] = {
	NULL, /* "$#,##0", */
	NULL, /* "$#,##0_);($#,##0)", */
	NULL, /* "$#,##0_);[Red]($#,##0)", */
	NULL, /* "$#,##0.00", */
	NULL, /* "$#,##0.00_);($#,##0.00)", */
	NULL, /* "$#,##0.00_);[Red]($#,##0.00)", */
	NULL,
};

/* Some are generated */
static char const *
fmts_accounting [] = {
	NULL, /* "_($* #,##0_);_($* (#,##0);_($* \"-\"_);_(@_)", */
	NULL, /* "_(* #,##0_);_(* (#,##0);_(* \"-\"_);_(@_)", */
	NULL, /* "_($* #,##0.00_);_($* (#,##0.00);_($* \"-\"??_);_(@_)", */
	NULL, /* "_(* #,##0.00_);_(* (#,##0.00);_(* \"-\"??_);_(@_)", */
	NULL
};

/* All are generated.  */
static char const *fmts_date[50];
static gunichar date_sep;
static char const *fmts_time[50];

/*****************************************************/

static char const * const
fmts_percentage [] = {
	"0%",
	"0.00%",
	NULL,
};

static char const * const
fmts_fraction [] = {
	"# ?/?",
	"# ?" "?/?" "?",  /* Don't accidentally use trigraph.  */
	"# ?" "?" "?/?" "?" "?",
	"# ?/2",
	"# ?/4",
	"# ?/8",
	"# ?/16",
	"# ?/10",
	"# ?/100",
	NULL
};

static char const * const
fmts_science [] = {
	"0.00E+00",
	"##0.0E+0",
	NULL
};

static char const *
fmts_text [] = {
	"@",
	NULL,
};

char const * const *
_go_format_builtins(GOFormatFamily fam)
{
	switch (fam) {
	case GO_FORMAT_GENERAL: return fmts_general;
	case GO_FORMAT_NUMBER: return fmts_number;
	case GO_FORMAT_CURRENCY: return fmts_currency;
	case GO_FORMAT_ACCOUNTING: return fmts_accounting;
	case GO_FORMAT_DATE: return fmts_date;
	case GO_FORMAT_TIME: return fmts_time;
	case GO_FORMAT_PERCENTAGE: return fmts_percentage;
	case GO_FORMAT_FRACTION: return fmts_fraction;
	case GO_FORMAT_SCIENTIFIC: return fmts_science;
	case GO_FORMAT_TEXT: return fmts_text;
	case GO_FORMAT_SPECIAL:
	case GO_FORMAT_MARKUP:
	default:
		return NULL;
	}
};

static void
add_dt_format (GHashTable *dt_hash, gboolean timep, guint *N, const char *fmt)
{
	char *fmt2;

	if (g_hash_table_lookup (dt_hash, fmt))
		return;

	if (timep)
		g_assert (*N + 1 < G_N_ELEMENTS (fmts_time));
	else
		g_assert (*N + 1 < G_N_ELEMENTS (fmts_date));
	fmt2 = g_strdup (fmt);
	(timep ? fmts_time : fmts_date)[*N] = fmt2;
	g_hash_table_insert (dt_hash, fmt2, fmt2);
	(*N)++;
	(timep ? fmts_time : fmts_date)[*N] = NULL;
}

static void
add_frobbed_date_format (GHashTable *dt_hash, guint *N, const char *fmt)
{
	GString *s = g_string_new (NULL);

	while (*fmt) {
		if (*fmt == '/') {
			g_string_append_unichar (s, date_sep);
		} else
			g_string_append_c (s, *fmt);
		fmt++;
	}

	add_dt_format (dt_hash, FALSE, N, s->str);
	g_string_free (s, TRUE);
}

static void
add_dmy_formats (GHashTable *dt_hash, guint *N)
{
	add_frobbed_date_format (dt_hash, N, "dd/mm/yyyy");
	add_frobbed_date_format (dt_hash, N, "dd/mm");
}

static void
add_mdy_formats (GHashTable *dt_hash, guint *N)
{
	add_frobbed_date_format (dt_hash, N, "m/d/yyyy");
	add_frobbed_date_format (dt_hash, N, "m/d");
}

/* Very hacky.  Sorry.  */
static gunichar
guess_date_sep (void)
{
	const GString *df = go_locale_get_date_format();
	const char *s;

	for (s = df->str; *s; s++) {
		switch (*s) {
		case 'd': case 'm': case 'y':
			while (g_ascii_isalpha (*s))
				s++;
			while (g_unichar_isspace (g_utf8_get_char (s)))
				s = g_utf8_next_char (s);
			if (*s != ',' &&
			    g_unichar_ispunct (g_utf8_get_char (s)))
				return g_utf8_get_char (s);
			break;
		default:
			; /* Nothing */
		}
	}

	return '/';
}

void
_go_currency_date_format_init (void)
{
	GOFormatCurrency const *currency = go_format_locale_currency ();
	GHashTable *dt_hash;
	GOFormat *fmt;
	guint N;
	int i;
	GOFormatDetails *details;

	details = go_format_details_new (GO_FORMAT_CURRENCY);
	details->currency = currency;
	for (i = 0; i < 6; i++) {
		GString *str = g_string_new (NULL);
		details->num_decimals = (i >= 3) ? 2 : 0;
		details->negative_red = (i % 3 == 2);
		details->negative_paren = (i % 3 >= 1);
		go_format_generate_str (str, details);
		fmts_currency[i] = g_string_free (str, FALSE);
	}
	go_format_details_free (details);

	details = go_format_details_new (GO_FORMAT_ACCOUNTING);
	for (i = 0; i < 4; i++) {
		GString *str = g_string_new (NULL);
		details->num_decimals = (i >= 2) ? 2 : 0;
		details->currency = (i & 1) ? NULL : currency;
		go_format_generate_str (str, details);
		fmts_accounting[i] = g_string_free (str, FALSE);
	}
	go_format_details_free (details);

	/* ---------------------------------------- */

	date_sep = guess_date_sep ();
	dt_hash = g_hash_table_new (g_str_hash, g_str_equal);
	N = 0;

	fmt = go_format_new_magic (GO_FORMAT_MAGIC_LONG_DATE);
	if (fmt) {
		add_dt_format (dt_hash, FALSE, &N, go_format_as_XL (fmt));
		go_format_unref (fmt);
	}
	fmt = go_format_new_magic (GO_FORMAT_MAGIC_MEDIUM_DATE);
	if (fmt) {
		add_dt_format (dt_hash, FALSE, &N, go_format_as_XL (fmt));
		go_format_unref (fmt);
	}
	fmt = go_format_new_magic (GO_FORMAT_MAGIC_SHORT_DATE);
	if (fmt) {
		add_dt_format (dt_hash, FALSE, &N, go_format_as_XL (fmt));
		go_format_unref (fmt);
	}

	switch (go_locale_month_before_day ()) {
	case 0:
		add_dmy_formats (dt_hash, &N);
		break;
	default:
	case 1:
		add_mdy_formats (dt_hash, &N);
		break;
	case 2:
		add_dt_format (dt_hash, FALSE, &N, "yyyy/mm/dd");
		add_mdy_formats (dt_hash, &N);
		break;
	}

	/* Make sure these exist no matter what.  */
	add_dt_format (dt_hash, FALSE, &N, "m/d/yyyy");
	add_dt_format (dt_hash, FALSE, &N, "m/d/yy");
	add_dt_format (dt_hash, FALSE, &N, "dd/mm/yyyy");
	add_dt_format (dt_hash, FALSE, &N, "dd/mm/yy");
	add_dt_format (dt_hash, FALSE, &N, "d-mmm-yyyy");
	add_dt_format (dt_hash, FALSE, &N, "yyyy-mm-dd");

	fmt = go_format_new_magic (GO_FORMAT_MAGIC_SHORT_DATETIME);
	if (fmt) {
		add_dt_format (dt_hash, FALSE, &N, go_format_as_XL (fmt));
		go_format_unref (fmt);
	}
	add_dt_format (dt_hash, FALSE, &N, "yyyy-mm-dd hh:mm:ss");
	/* ISO 8601 (zulu time only for now) */
	add_dt_format (dt_hash, FALSE, &N, "yyyy-mm-dd\"T\"hh:mm:ss\"Z\"");


	g_hash_table_destroy (dt_hash);

	/* ---------------------------------------- */

	dt_hash = g_hash_table_new (g_str_hash, g_str_equal);
	N = 0;

	fmt = go_format_new_magic (GO_FORMAT_MAGIC_LONG_TIME);
	if (fmt) {
		add_dt_format (dt_hash, TRUE, &N, go_format_as_XL (fmt));
		go_format_unref (fmt);
	}
	fmt = go_format_new_magic (GO_FORMAT_MAGIC_MEDIUM_TIME);
	if (fmt) {
		add_dt_format (dt_hash, TRUE, &N, go_format_as_XL (fmt));
		go_format_unref (fmt);
	}
	fmt = go_format_new_magic (GO_FORMAT_MAGIC_SHORT_TIME);
	if (fmt) {
		add_dt_format (dt_hash, TRUE, &N, go_format_as_XL (fmt));
		go_format_unref (fmt);
	}

	/* Must-have formats.  */
	add_dt_format (dt_hash, TRUE, &N, "h:mm AM/PM");
	add_dt_format (dt_hash, TRUE, &N, "h:mm:ss AM/PM");
	add_dt_format (dt_hash, TRUE, &N, "hh:mm");
	add_dt_format (dt_hash, TRUE, &N, "hh:mm:ss");
	add_dt_format (dt_hash, TRUE, &N, "h:mm:ss");

	/* Elapsed time.  */
	add_dt_format (dt_hash, TRUE, &N, "[h]:mm:ss");
	add_dt_format (dt_hash, TRUE, &N, "[mm]:ss");

	g_hash_table_destroy (dt_hash);
}

void
_go_currency_date_format_shutdown (void)
{
	/* We need to free allocated strings since */
	/* currency_date_format_init/shutdown may  */
	/* be called repeatedly by the format selector */
	/* when switching locales */

	int i;

	for (i = 0; i < 6; i++) {
		g_free ((char *)(fmts_currency[i]));
		fmts_currency[i] = NULL;
	}

	for (i = 0; i < 4; i++) {
		g_free ((char *)(fmts_accounting[i]));
		fmts_accounting[i] = NULL;
	}

	for (i = 0; fmts_date[i]; i++) {
		g_free ((char*)(fmts_date[i]));
		fmts_date[i] = NULL;
	}

	for (i = 0; fmts_time[i]; i++) {
		g_free ((char*)(fmts_time[i]));
		fmts_time[i] = NULL;
	}
}

static GOFormatCurrency const _go_format_currencies_table[] =
{
 	{ "", N_("None"), TRUE, FALSE },	/* These first six elements */
 	{ "$", "$", TRUE, FALSE },		/* Must stay in this order */
 	{ "£", "£", TRUE, FALSE },		/* GBP */
 	{ "¥", "¥", TRUE, FALSE },		/* JPY */

 	/* Add yours to this list ! */
 	{ "[$€-1]", "€ Euro (100 €)", FALSE, TRUE},
	{ "[$€-2]", "€ Euro (€ 100)", TRUE, TRUE},

	/* The first column has three letter acronyms
	 * for each currency.  They MUST start with '[$'
	 * The second column has the long names of the currencies.
	 */

	/* 2010/08/12 Updated to match iso 4217 */

	{ "[$AED]",	N_("United Arab Emirates, Dirhams"), TRUE, TRUE},
	{ "[$AFN]",	N_("Afghanistan, Afghanis"), TRUE, TRUE},
	{ "[$ALL]",	N_("Albania, Leke"), TRUE, TRUE},
	{ "[$AMD]",	N_("Armenia, Drams"), TRUE, TRUE},
	{ "[$ANG]",	N_("Netherlands antilles, Guilders"), TRUE, TRUE},
	{ "[$AOA]",	N_("Angola, Kwanza"), TRUE, TRUE},
	{ "[$ARS]",	N_("Argentina, Pesos"), TRUE, TRUE},
	{ "[$AUD]",	N_("Australia, Dollars"), TRUE, TRUE},
	{ "[$AWG]",	N_("Aruba, Guilders"), TRUE, TRUE},
	{ "[$AZN]",	N_("Azerbaijan, Manats"), TRUE, TRUE},
	{ "[$BAM]",	N_("Bosnia and herzegovina, Convertible Marka"), TRUE, TRUE},
	{ "[$BBD]",	N_("Barbados, Dollars"), TRUE, TRUE},
	{ "[$BDT]",	N_("Bangladesh, Taka"), TRUE, TRUE},
	{ "[$BGN]",	N_("Bulgaria, Leva"), TRUE, TRUE},
	{ "[$BHD]",	N_("Bahrain, Dinars"), TRUE, TRUE},
	{ "[$BIF]",	N_("Burundi, Francs"), TRUE, TRUE},
	{ "[$BMD]",	N_("Bermuda, Dollars"), TRUE, TRUE},
	{ "[$BND]",	N_("Brunei Darussalam, Dollars"), TRUE, TRUE},
	{ "[$BOB]",	N_("Bolivia, Bolivianos"), TRUE, TRUE},
	{ "[$BOV]",	N_("Bolivia, Mvdol"), TRUE, TRUE},
	{ "[$BRL]",	N_("Brazil, Brazilian Real"), TRUE, TRUE},
	{ "[$BSD]",	N_("Bahamas, Dollars"), TRUE, TRUE},
	{ "[$BTN]",	N_("Bhutan, Ngultrum"), TRUE, TRUE},
	{ "[$BWP]",	N_("Botswana, Pulas"), TRUE, TRUE},
	{ "[$BYR]",	N_("Belarus, Rubles"), TRUE, TRUE},
	{ "[$BZD]",	N_("Belize, Dollars"), TRUE, TRUE},
	{ "[$CAD]",	N_("Canada, Dollars"), TRUE, TRUE},
	{ "[$CDF]",	N_("Congo, the democratic republic of, Congolese Francs"), TRUE, TRUE},
	{ "[$CHE]",	N_("Switzerland, WIR Euros"), TRUE, TRUE},
	{ "[$CHF]",	N_("Switzerland, Francs"), TRUE, TRUE},
	{ "[$CHW]",	N_("Switzerland, WIR Francs"), TRUE, TRUE},
	{ "[$CLF]",	N_("Chile, Unidades de fomento"), TRUE, TRUE},
	{ "[$CLP]",	N_("Chile, Pesos"), TRUE, TRUE},
	{ "[$CNY]",	N_("China, Yuan Renminbi"), TRUE, TRUE},
	{ "[$COP]",	N_("Colombia, Pesos"), TRUE, TRUE},
	{ "[$COU]",	N_("Colombia, Unidades de Valor Real"), TRUE, TRUE},
	{ "[$CRC]",	N_("Costa rica, Colones"), TRUE, TRUE},
	{ "[$CUC]",	N_("Cuba, Pesos Convertibles"), TRUE, TRUE},
	{ "[$CUP]",	N_("Cuba, Pesos"), TRUE, TRUE},
	{ "[$CVE]",	N_("Cape verde, Escudos"), TRUE, TRUE},
	{ "[$CZK]",	N_("Czech republic, Koruny"), TRUE, TRUE},
	{ "[$DJF]",	N_("Djibouti, Francs"), TRUE, TRUE},
	{ "[$DKK]",	N_("Denmark, Kroner"), TRUE, TRUE},
	{ "[$DOP]",	N_("Dominican republic, Pesos"), TRUE, TRUE},
	{ "[$DZD]",	N_("Algeria, Algerian Dinars"), TRUE, TRUE},
	{ "[$EGP]",	N_("Egypt, Pounds"), TRUE, TRUE},
	{ "[$ERN]",	N_("Eritrea, Nakfa"), TRUE, TRUE},
	{ "[$ETB]",	N_("Ethiopia, Birr"), TRUE, TRUE},
	{ "[$EUR]",	N_("Euro Members Countries, Euros"), TRUE, TRUE},
	{ "[$FJD]",	N_("Fiji, Dollars"), TRUE, TRUE},
	{ "[$FKP]",	N_("Falkland Islands (Malvinas), Pounds"), TRUE, TRUE},
	{ "[$GBP]",	N_("United kingdom, Pounds"), TRUE, TRUE},
	{ "[$GEL]",	N_("Georgia, Lari"), TRUE, TRUE},
	{ "[$GHS]",	N_("Ghana, Cedis"), TRUE, TRUE},
	{ "[$GIP]",	N_("Gibraltar, Pounds"), TRUE, TRUE},
	{ "[$GMD]",	N_("Gambia, Dalasi"), TRUE, TRUE},
	{ "[$GNF]",	N_("Guinea, Francs"), TRUE, TRUE},
	{ "[$GTQ]",	N_("Guatemala, Quetzales"), TRUE, TRUE},
	{ "[$GYD]",	N_("Guyana, Dollars"), TRUE, TRUE},
	{ "[$HKD]",	N_("Hong Kong, Dollars"), TRUE, TRUE},
	{ "[$HNL]",	N_("Honduras, Lempiras"), TRUE, TRUE},
	{ "[$HRK]",	N_("Croatia, Kuna"), TRUE, TRUE},
	{ "[$HTG]",	N_("Haiti, Gourdes"), TRUE, TRUE},
	{ "[$HUF]",	N_("Hungary, Forint"), TRUE, TRUE},
	{ "[$IDR]",	N_("Indonesia, Rupiahs"), TRUE, TRUE},
	{ "[$ILS]",	N_("Israel, New Sheqels"), TRUE, TRUE},
	{ "[$INR]",	N_("India, Rupees"), TRUE, TRUE},
	{ "[$IQD]",	N_("Iraq, Dinars"), TRUE, TRUE},
	{ "[$IRR]",	N_("Iran, Rials"), TRUE, TRUE},
	{ "[$ISK]",	N_("Iceland, Kronur"), TRUE, TRUE},
	{ "[$JMD]",	N_("Jamaica, Dollars"), TRUE, TRUE},
	{ "[$JOD]",	N_("Jordan, Dinars"), TRUE, TRUE},
	{ "[$JPY]",	N_("Japan, Yen"), TRUE, TRUE},
	{ "[$KES]",	N_("Kenya, Shillings"), TRUE, TRUE},
	{ "[$KGS]",	N_("Kyrgyzstan, Soms"), TRUE, TRUE},
	{ "[$KHR]",	N_("Cambodia, Riels"), TRUE, TRUE},
	{ "[$KMF]",	N_("Comoros, Francs"), TRUE, TRUE},
	{ "[$KPW]",	N_("Korea (North), Won"), TRUE, TRUE},
	{ "[$KRW]",	N_("Korea (South) Wons"), TRUE, TRUE},
	{ "[$KWD]",	N_("Kuwait, Dinars"), TRUE, TRUE},
	{ "[$KYD]",	N_("Cayman islands, Dollars"), TRUE, TRUE},
	{ "[$KZT]",	N_("Kazakhstan, Tenge"), TRUE, TRUE},
	{ "[$LAK]",	N_("Laos, Kips"), TRUE, TRUE},
	{ "[$LBP]",	N_("Lebanon, Pounds"), TRUE, TRUE},
	{ "[$LKR]",	N_("Sri Lanka, Rupees"), TRUE, TRUE},
	{ "[$LRD]",	N_("Liberia, Dollars"), TRUE, TRUE},
	{ "[$LTL]",	N_("Lithuania, Litai"), TRUE, TRUE},
	{ "[$LVL]",	N_("Latvia, Lati"), TRUE, TRUE},
	{ "[$LYD]",	N_("Libya, Dinars"), TRUE, TRUE},
	{ "[$MAD]",	N_("Morocco, Dirhams"), TRUE, TRUE},
	{ "[$MDL]",	N_("Moldova, Lei"), TRUE, TRUE},
	{ "[$MGA]",	N_("Madagascar, Malagasy Ariary"), TRUE, TRUE},
	{ "[$MKD]",	N_("Macedonia, Denars"), TRUE, TRUE},
	{ "[$MMK]",	N_("Myanmar (Burma), Kyats"), TRUE, TRUE},
	{ "[$MNT]",	N_("Mongolia, Tugriks"), TRUE, TRUE},
	{ "[$MOP]",	N_("Macao, Patacas"), TRUE, TRUE},
	{ "[$MRO]",	N_("Mauritania, Ouguiyas"), TRUE, TRUE},
	{ "[$MUR]",	N_("Mauritius, Rupees"), TRUE, TRUE},
	{ "[$MVR]",	N_("Maldives (Maldive Islands), Rufiyaas"), TRUE, TRUE},
	{ "[$MWK]",	N_("Malawi, Kwachas"), TRUE, TRUE},
	{ "[$MXN]",	N_("Mexico, Pesos"), TRUE, TRUE},
	{ "[$MXV]",	N_("Mexico, Unidades de Inversion"), TRUE, TRUE},
	{ "[$MYR]",	N_("Malaysia, Ringgits"), TRUE, TRUE},
	{ "[$MZN]",	N_("Mozambique, Meticais"), TRUE, TRUE},
	{ "[$NGN]",	N_("Nigeria, Nairas"), TRUE, TRUE},
	{ "[$NIO]",	N_("Nicaragua, Gold Cordobas"), TRUE, TRUE},
	{ "[$NOK]",	N_("Norway, Norwegian Krone"), TRUE, TRUE},
	{ "[$NPR]",	N_("Nepal, Nepalese Rupees"), TRUE, TRUE},
	{ "[$NZD]",	N_("New zealand, Dollars"), TRUE, TRUE},
	{ "[$OMR]",	N_("Oman, Rials"), TRUE, TRUE},
	{ "[$PAB]",	N_("Panama, Balboa"), TRUE, TRUE},
	{ "[$PEN]",	N_("Peru, Nuevo Soles"), TRUE, TRUE},
	{ "[$PGK]",	N_("Papua New Guinea, Kina"), TRUE, TRUE},
	{ "[$PHP]",	N_("Philippines, Pesos"), TRUE, TRUE},
	{ "[$PKR]",	N_("Pakistan, Rupees"), TRUE, TRUE},
	{ "[$PLN]",	N_("Poland, Zlotys"), TRUE, TRUE},
	{ "[$PYG]",	N_("Paraguay, Guarani"), TRUE, TRUE},
	{ "[$QAR]",	N_("Qatar, Rials"), TRUE, TRUE},
	{ "[$RON]",	N_("Romania, New Lei"), TRUE, TRUE},
	{ "[$RSD]",	N_("Serbia, Dinars"), TRUE, TRUE},
	{ "[$RUB]",	N_("Russia, Rubles"), TRUE, TRUE},
	{ "[$RWF]",	N_("Rwanda, Rwanda Francs"), TRUE, TRUE},
	{ "[$SAR]",	N_("Saudi arabia, Riyals"), TRUE, TRUE},
	{ "[$SBD]",	N_("Solomon Islands, Dollars"), TRUE, TRUE},
	{ "[$SCR]",	N_("Seychelles, Rupees"), TRUE, TRUE},
	{ "[$SDG]",	N_("Sudan, Pounds"), TRUE, TRUE},
	{ "[$SDR]",	N_("International Monetary Fund (I.M.F), SDR"), TRUE, TRUE},
	{ "[$SEK]",	N_("Sweden, Kronor"), TRUE, TRUE},
	{ "[$SGD]",	N_("Singapore, Dollars"), TRUE, TRUE},
	{ "[$SHP]",	N_("Saint Helena, Ascension and Tristan da Cunha, Pounds"), TRUE, TRUE},
	{ "[$SLL]",	N_("Sierra leone, Leones"), TRUE, TRUE},
	{ "[$SOS]",	N_("Somalia, Shillings"), TRUE, TRUE},
	{ "[$SRD]",	N_("Suriname, Dollars"), TRUE, TRUE},
	{ "[$STD]",	N_("São Tome and Principe, Dobras"), TRUE, TRUE},
	{ "[$SVC]",	N_("El Salvador, Colones"), TRUE, TRUE},
	{ "[$SYP]",	N_("Syria, Pounds"), TRUE, TRUE},
	{ "[$SZL]",	N_("Swaziland, Emalangeni"), TRUE, TRUE},
	{ "[$THB]",	N_("Thailand, Baht"), TRUE, TRUE},
	{ "[$TJS]",	N_("Tajikistan, Somoni"), TRUE, TRUE},
	{ "[$TMT]",	N_("Turkmenistan, Manats"), TRUE, TRUE},
	{ "[$TND]",	N_("Tunisia, Dinars"), TRUE, TRUE},
	{ "[$TOP]",	N_("Tonga, Pa'angas"), TRUE, TRUE},
	{ "[$TRY]",	N_("Turkey, Liras"), TRUE, TRUE},
	{ "[$TTD]",	N_("Trinidad and Tobago, Dollars"), TRUE, TRUE},
	{ "[$TWD]",	N_("Taiwan, New Dollars"), TRUE, TRUE},
	{ "[$TZS]",	N_("Tanzania, Shillings"), TRUE, TRUE},
	{ "[$UAH]",	N_("Ukraine, Hryvnia"), TRUE, TRUE},
	{ "[$UGX]",	N_("Uganda, Shillings"), TRUE, TRUE},
	{ "[$USD]",	N_("United States of America, Dollars"), TRUE, TRUE},
	{ "[$USN]",	N_("United States of America, Dollars (Next day)"), TRUE, TRUE},
	{ "[$USS]",	N_("United States of America, Dollars (Same day)"), TRUE, TRUE},
	{ "[$UYI]",	N_("Uruguay, Pesos en Unidades Indexadas"), TRUE, TRUE},
	{ "[$UYU]",	N_("Uruguay, Pesos"), TRUE, TRUE},
	{ "[$UZS]",	N_("Uzbekistan, Sums"), TRUE, TRUE},
	{ "[$VEF]",	N_("Venezuela, Bolivares Fuertes"), TRUE, TRUE},
	{ "[$VND]",	N_("Viet Nam, Dong"), TRUE, TRUE},
	{ "[$VUV]",	N_("Vanuatu, Vatu"), TRUE, TRUE},
	{ "[$WST]",	N_("Samoa, Tala"), TRUE, TRUE},
	{ "[$XAF]",	N_("Communaute Financiere Africaine BEAC, Francs"), TRUE, TRUE},
	{ "[$XAG]",	N_("Silver, Ounces"), TRUE, TRUE },
	{ "[$XAU]",	N_("Gold, Ounces"), TRUE, TRUE },
	{ "[$XBA]",	N_("Bond Markets Units, European Composite Units (EURCO)"), TRUE, TRUE },
	{ "[$XBB]",	N_("European Monetary Units (E.M.U.-6)"), TRUE, TRUE },
	{ "[$XBC]",	N_("European Units of Account 9 (E.U.A.-9)"), TRUE, TRUE },
	{ "[$XBD]",	N_("European Units of Account 17 (E.U.A.-17)"), TRUE, TRUE },
	{ "[$XCD]",	N_("East Caribbean Dollars"), TRUE, TRUE},
	{ "[$XFU]",	N_("UIC-Francs"), TRUE, TRUE },
	{ "[$XOF]",	N_("Communaute Financiere Africaine BCEAO, Francs"), TRUE, TRUE},
	{ "[$XPF]",	N_("Comptoirs Francais du Pacifique, Francs"), TRUE, TRUE},
	{ "[$XPT]",	N_("Palladium, Ounces"), TRUE, TRUE },
	{ "[$YER]",	N_("Yemen, Rials"), TRUE, TRUE},
	{ "[$ZAR]",	N_("South Africa, Rands"), TRUE, TRUE},
	{ "[$LSL]",	N_("Lesotho, Maloti"), TRUE, TRUE},
	{ "[$NAD]",	N_("Namibia, Dollars"), TRUE, TRUE},
	{ "[$ZMK]",	N_("Zambia, Zambian Kwacha"), TRUE, TRUE},
	{ "[$ZWL]",	N_("Zimbabwe, Zimbabwe Dollars"), TRUE, TRUE},

	{ NULL, NULL, FALSE, FALSE }
};


GOFormatCurrency const *_go_format_currencies (void)
{
	return &_go_format_currencies_table[0];
}
