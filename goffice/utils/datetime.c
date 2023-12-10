/*
 * datetime.c: Date and time routines grabbed from elsewhere.
 *
 * Authors:
 *   Miguel de Icaza (miguel@gnu.org)
 *   Morten Welinder <terra@gnome.org>
 *   Jukka-Pekka Iivonen <iivonen@iki.fi>
 *   Andreas J. Guelzow <aguelzow@taliesin.ca>
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
#include "datetime.h"
#include <goffice/math/go-math.h>

#include <math.h>
#include <string.h>

#define SECS_PER_DAY (24 * 60 * 60)
#define HALF_SEC (0.5 / SECS_PER_DAY)

/* ------------------------------------------------------------------------- */
static GODateConventions *
go_date_conventions_copy (GODateConventions const *dc)
{
	GODateConventions *res = g_new (GODateConventions, 1);
	memcpy (res, dc, sizeof(GODateConventions));
	return res;
}

GType
go_date_conventions_get_type (void)
{
	static GType t = 0;

	if (t == 0)
		t = g_boxed_type_register_static ("GODateConventions",
			 (GBoxedCopyFunc)go_date_conventions_copy,
			 (GBoxedFreeFunc)g_free);
	return t;
}

/* ------------------------------------------------------------------------- */

/* One less that the Julian day number of 19000101.  */
static int date_origin_1900 = 0;
/* Julian day number of 19040101.  */
static int date_origin_1904 = 0;

/*
 * The serial number of 19000228.  Excel allocates a serial number for
 * the non-existing date 19000229.
 */
static const int date_serial_19000228 = 59;

/* 31-Dec-9999 */
static const int date_serial_max_1900 = 2958465;
static const int date_serial_max_1904 = 2957003;


static void
date_init (void)
{
	GDate date;

	g_date_clear (&date, 1);

	/* Day 1 means 1st of January of 1900 */
	g_date_set_dmy (&date, 1, 1, 1900);
	date_origin_1900 = g_date_get_julian (&date) - 1;

	/* Day 0 means 1st of January of 1904 */
	g_date_set_dmy (&date, 1, 1, 1904);
	date_origin_1904 = g_date_get_julian (&date);
}

/* ------------------------------------------------------------------------- */

int
go_date_g_to_serial (GDate const *date, GODateConventions const *conv)
{
	int day;

	if (!date_origin_1900)
		date_init ();

	if (conv && conv->use_1904)
		return g_date_get_julian (date) - date_origin_1904;
	day = g_date_get_julian (date) - date_origin_1900;
	return day + (day > date_serial_19000228);
}

/* ------------------------------------------------------------------------- */

void
go_date_serial_to_g (GDate *res, int serial, GODateConventions const *conv)
{
	if (!date_origin_1900)
		date_init ();

	g_date_clear (res, 1);
	if (conv && conv->use_1904) {
		if (serial > date_serial_max_1904)
			return;
		g_date_set_julian (res, serial + date_origin_1904);
	} else if (serial > date_serial_19000228) {
		if (serial > date_serial_max_1900)
			return;
		if (serial == date_serial_19000228 + 1)
			return;
		g_date_set_julian (res, serial + date_origin_1900 - 1);
	} else
		g_date_set_julian (res, serial + date_origin_1900);
}

/* ------------------------------------------------------------------------- */

double
go_date_timet_to_serial_raw (time_t t, GODateConventions const *conv)
{
	struct tm *tm = localtime (&t);
	int secs;
	GDate date;

        g_date_clear (&date, 1);
	g_date_set_time_t (&date, t);
	secs = tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec;
	return go_date_g_to_serial (&date, conv) +
		secs / (double)SECS_PER_DAY;
}

/* ------------------------------------------------------------------------- */

int
go_date_serial_raw_to_serial (double raw)
{
	return (int) floor (raw + HALF_SEC);
}

/* ------------------------------------------------------------------------- */

int
go_date_timet_to_serial (time_t t, GODateConventions const *conv)
{
	return go_date_serial_raw_to_serial (go_date_timet_to_serial_raw (t, conv));
}

/* ------------------------------------------------------------------------- */

time_t
go_date_serial_to_timet (int serial, GODateConventions const *conv)
{
	GDate gd;

	go_date_serial_to_g (&gd, serial, conv);
	if (g_date_valid (&gd)) {
		struct tm tm;
		g_date_to_struct_tm (&gd, &tm);
		return mktime (&tm);
	} else
		return -1;
}

/* ------------------------------------------------------------------------- */
/* This is time-only assuming a 24h day.  It probably loses completely on */
/* days with summer time ("daylight savings") changes.  */

int
go_date_serial_raw_to_seconds (double raw)
{
	raw += HALF_SEC;
	return (raw - floor (raw)) * SECS_PER_DAY;
}

/* ------------------------------------------------------------------------- */

int
go_date_timet_to_seconds (time_t t)
{
	/* we just want the seconds, actual date does not matter. So we can ignore
	 * the date convention (1900 vs 1904) */
	return go_date_serial_raw_to_seconds (go_date_timet_to_serial_raw (t, NULL));
}

/* ------------------------------------------------------------------------- */

int
go_date_g_months_between (GDate const *date1, GDate const *date2)
{
	g_return_val_if_fail (g_date_valid (date1), 0);
	g_return_val_if_fail (g_date_valid (date2), 0);

	/* find the difference according to the month and year ordinals,
	   but discount the last month if there are not enough days. */
	return 12 * (g_date_get_year (date2) - g_date_get_year (date1))
		+ g_date_get_month (date2) - g_date_get_month (date1)
		- (g_date_get_day (date2) >= g_date_get_day (date1) ? 0 : 1);
}

/* ------------------------------------------------------------------------- */

int
go_date_g_years_between (GDate const *date1, GDate const *date2)
{
	int months;

	g_return_val_if_fail (g_date_valid (date1), 0);
	g_return_val_if_fail (g_date_valid (date2), 0);

	months = go_date_g_months_between (date1, date2);
	return months > 0 ? months / 12 : -(-months / 12);
}

/* ------------------------------------------------------------------------- */

/**
 * go_date_weeknum:
 * @date:      date
 * @method:    week numbering method
 *
 * Returns: week number according to the given method.
 * 1:   Week starts on Sunday.  January 1 is in week 1.
 * 2:   Week starts on Monday.  January 1 is in week 1.
 * 150: ISO 8601 week number.
 */
int
go_date_weeknum (GDate const *date, int method)
{
	g_return_val_if_fail (g_date_valid (date), -1);
	g_return_val_if_fail (method == GO_WEEKNUM_METHOD_SUNDAY ||
			      method == GO_WEEKNUM_METHOD_MONDAY ||
			      method == GO_WEEKNUM_METHOD_ISO,
			      -1);

	switch (method) {
	case GO_WEEKNUM_METHOD_SUNDAY: {
		GDate jan1;
		GDateWeekday wd;
		int doy;

		g_date_clear (&jan1, 1);
		g_date_set_dmy (&jan1, 1, 1, g_date_get_year (date));
		wd = g_date_get_weekday (&jan1);
		doy = g_date_get_day_of_year (date);

		return (doy + (int)wd + 6 - (wd == G_DATE_SUNDAY ? 7 : 0)) / 7;
	}
	case GO_WEEKNUM_METHOD_MONDAY: {
		GDate jan1;
		GDateWeekday wd;
		int doy;

		g_date_clear (&jan1, 1);
		g_date_set_dmy (&jan1, 1, 1, g_date_get_year (date));
		wd = g_date_get_weekday (&jan1);
		doy = g_date_get_day_of_year (date);

		return (doy + (int)wd + 5) / 7;
	}
	case GO_WEEKNUM_METHOD_ISO:
		return g_date_get_iso8601_week_of_year (date);
	default:
		return -1;
	}
}

/* ------------------------------------------------------------------------- */

static gint32
days_between_GO_BASIS_MSRB_30_360 (GDate const *from, GDate const *to)
{
	int y1, m1, d1, y2, m2, d2;

	y1 = g_date_get_year (from);
	m1 = g_date_get_month (from);
	d1 = g_date_get_day (from);
	y2 = g_date_get_year (to);
	m2 = g_date_get_month (to);
	d2 = g_date_get_day (to);

	if (d1 == d2 && m1 == m2 && y1 == y2)
		return 0;
	if (d1 == 31)
		d1 = 30;
	if (d2 == 31 && d1 == 30)
		d2 = 30;

	if (m1 == 2 && g_date_is_last_of_month (from)) {
		if (m2 == 2 && g_date_is_last_of_month (to))
			d2 = 30;
		d1 = 30;
	}

	return (y2 - y1) * 360 + (m2 - m1) * 30 + (d2 - d1);
}

static gint32
days_between_GO_BASIS_MSRB_30_360_SYM (GDate const *from, GDate const *to)
{
	int y1, m1, d1, y2, m2, d2;

	y1 = g_date_get_year (from);
	m1 = g_date_get_month (from);
	d1 = g_date_get_day (from);
	y2 = g_date_get_year (to);
	m2 = g_date_get_month (to);
	d2 = g_date_get_day (to);

	if (m1 == 2 && g_date_is_last_of_month (from))
		d1 = 30;
	if (m2 == 2 && g_date_is_last_of_month (to))
		d2 = 30;
	if (d2 == 31 && d1 >= 30)
		d2 = 30;
	if (d1 == 31)
		d1 = 30;

	return (y2 - y1) * 360 + (m2 - m1) * 30 + (d2 - d1);
}

static gint32
days_between_GO_BASIS_30E_360 (GDate const *from, GDate const *to)
{
	int y1, m1, d1, y2, m2, d2;

	y1 = g_date_get_year (from);
	m1 = g_date_get_month (from);
	d1 = g_date_get_day (from);
	y2 = g_date_get_year (to);
	m2 = g_date_get_month (to);
	d2 = g_date_get_day (to);

	if (d1 == 31)
		d1 = 30;
	if (d2 == 31)
		d2 = 30;

	return (y2 - y1) * 360 + (m2 - m1) * 30 + (d2 - d1);
}

static gint32
days_between_GO_BASIS_30Ep_360 (GDate const *from, GDate const *to)
{
	int y1, m1, d1, y2, m2, d2;

	y1 = g_date_get_year (from);
	m1 = g_date_get_month (from);
	d1 = g_date_get_day (from);
	y2 = g_date_get_year (to);
	m2 = g_date_get_month (to);
	d2 = g_date_get_day (to);

	if (d1 == 31)
		d1 = 30;
	if (d2 == 31) {
		d2 = 1;
		m2++;
		/* No need to check for m2 == 13 since 12*30 == 360 */
	}

	return (y2 - y1) * 360 + (m2 - m1) * 30 + (d2 - d1);
}

/*
 * go_date_days_between_basis
 *
 * @from: GDate *
 * @to: GDate *
 * @basis: GOBasisType
 * see datetime.h and doc/fn-financial-basis.txt for details
 *
 * @in_order: dates are considered in order
 *
 * Returns: Number of days after the earlier and not after the later date
 *
 */

gint32
go_date_days_between_basis (GDate const *from, GDate const *to, GOBasisType basis)
{
	int sign = 1;

	if (g_date_compare (from, to) == 1) {
		GDate const *tmp = from;
		from = to;
		to = tmp;
		sign = -1;
	}

	switch (basis) {
	case GO_BASIS_ACT_ACT:
	case GO_BASIS_ACT_360:
	case GO_BASIS_ACT_365:
		return sign * (g_date_get_julian (to) - g_date_get_julian (from));
	case GO_BASIS_30E_360:
		return sign * days_between_GO_BASIS_30E_360 (from, to);
	case GO_BASIS_30Ep_360:
		return sign * days_between_GO_BASIS_30Ep_360 (from, to);
	case GO_BASIS_MSRB_30_360_SYM:
		return sign * days_between_GO_BASIS_MSRB_30_360_SYM (from, to);
	case GO_BASIS_MSRB_30_360:
	default:
		return sign * days_between_GO_BASIS_MSRB_30_360 (from, to);
	}
}

/* ------------------------------------------------------------------------- */

/*
 * go_coup_cd
 *
 * @res:
 * @settlement: GDate *
 * @maturity: GDate *  must follow settlement strictly
 * @freq: int      divides 12 evenly
 * @eom: gboolean whether to do special end of month
 *                       handling
 * @next: gboolean whether next or previous date
 *
 * Returns: GDate *  next  or previous coupon date
 *
 * this function does not depend on the basis of counting!
 */
void
go_coup_cd (GDate *result, GDate const *settlement, GDate const *maturity,
	 int freq, gboolean eom, gboolean next)
{
        int        months, periods;
	gboolean   is_eom_special;

	is_eom_special = eom && g_date_is_last_of_month (maturity);

	g_date_clear (result, 1);

	months = 12 / freq;
	periods = (g_date_get_year(maturity) - g_date_get_year (settlement));
	if (periods > 0)
		periods = (periods - 1) * freq;

	do {
		g_date_set_julian (result, g_date_get_julian (maturity));
		periods++;
		g_date_subtract_months (result, periods * months);
		if (is_eom_special) {
			int ndays = g_date_get_days_in_month
				(g_date_get_month (result),
				 g_date_get_year (result));
			g_date_set_day (result, ndays);
		}
	} while (g_date_compare (settlement, result) < 0 );

	if (next) {
		g_date_set_julian (result, g_date_get_julian (maturity));
		periods--;
		g_date_subtract_months (result, periods * months);
		if (is_eom_special) {
			int ndays = g_date_get_days_in_month
				(g_date_get_month (result),
				 g_date_get_year (result));
			g_date_set_day (result, ndays);
		}
	}
}

/* ------------------------------------------------------------------------- */


/**
 * go_coupdays:
 * @settlement: #GDate
 * @maturity: #GDate
 * @conv: #GoCouponConvention
 *
 * Returns: the number of days in the coupon period of the settlement date.
 * Currently, returns negative numbers if the branch is not implemented.
 **/
double
go_coupdays (GDate const *settlement, GDate const *maturity,
	     GoCouponConvention const *conv)
{
	GDate prev, next;

        switch (conv->basis) {
	case GO_BASIS_MSRB_30_360:
	case GO_BASIS_ACT_360:
        case GO_BASIS_30E_360:
        case GO_BASIS_30Ep_360:
		return 360 / conv->freq;
	case GO_BASIS_ACT_365:
		return 365.0 / conv->freq;
	case GO_BASIS_ACT_ACT:
	default:
		go_coup_cd (&next, settlement, maturity, conv->freq, conv->eom, TRUE);
		go_coup_cd (&prev, settlement, maturity, conv->freq, conv->eom, FALSE);
		return go_date_days_between_basis (&prev, &next, GO_BASIS_ACT_ACT);
        }
}

/* ------------------------------------------------------------------------- */


/**
 * go_coupdaybs:
 * @settlement: #GDate
 * @maturity: #GDate
 * @conv: #GoCouponConvention
 *
 * Returns: the number of days from the beginning of the coupon period to the
 * 	settlement date.
 **/
double
go_coupdaybs (GDate const *settlement, GDate const *maturity,
	      GoCouponConvention const *conv)
{
	GDate prev_coupon;
	go_coup_cd (&prev_coupon, settlement, maturity, conv->freq, conv->eom, FALSE);
	return go_date_days_between_basis (&prev_coupon, settlement, conv->basis);
}

/**
 * go_coupdaysnc:
 * @settlement:
 * @maturity:
 * @conv: #GoCouponConvention
 *
 * Returns: the number of days from the settlement date to the next
 * coupon date.
 **/
double
go_coupdaysnc (GDate const *settlement, GDate const *maturity,
	       GoCouponConvention const *conv)
{
	GDate next_coupon;
	go_coup_cd (&next_coupon, settlement, maturity, conv->freq, conv->eom, TRUE);
	return go_date_days_between_basis (settlement, &next_coupon, conv->basis);
}

int
go_date_convention_base (GODateConventions const *conv)
{
	g_return_val_if_fail (conv != NULL, 1900);
	return conv->use_1904 ? 1904 : 1900;
}

const GODateConventions *
go_date_conv_from_str (const char *s)
{
	static const GODateConventions apple1904 = { TRUE };
	static const GODateConventions lotus1900 = { FALSE };

	g_return_val_if_fail (s != NULL, NULL);

	if (strcmp (s, "Apple:1904") == 0)
		return &apple1904;

	if (strcmp (s, "Lotus:1900") == 0)
		return &lotus1900;

	return NULL;
}

gboolean
go_date_conv_equal (const GODateConventions *a, const GODateConventions *b)
{
	g_return_val_if_fail (a != NULL, FALSE);
	g_return_val_if_fail (b != NULL, FALSE);

	return a->use_1904 == b->use_1904;
}

double
go_date_conv_translate (double f,
			const GODateConventions *src,
			const GODateConventions *dst)
{
	g_return_val_if_fail (src != NULL, f);
	g_return_val_if_fail (dst != NULL, f);

	if (!go_finite (f) || go_date_conv_equal (src, dst))
		return f;

	if (dst->use_1904) {
		if (f < date_serial_19000228 + 1)
			f += 1;
		f -= 1462;
	} else {
		f += 1462;
		if (f < date_serial_19000228 + 2)
			f -= 1;
	}

	return f;
}

/* ------------------------------------------------------------------------- */

static char *
deal_with_spaces (char *buf)
{
	/* Abbreviated January in "fi_FI" has a terminating space.  */
	gsize len = strlen (buf);

	while (len > 0) {
		char *prev = g_utf8_prev_char (buf + len);
		if (!g_unichar_isspace (g_utf8_get_char (prev)))
			break;
		len = prev - buf;
	}
	buf[len] = 0;
	return g_strdup (buf);
}

char *
go_date_weekday_name (GDateWeekday wd, gboolean abbrev)
{
	char buf[100];
	GDate date;

	g_return_val_if_fail (g_date_valid_weekday (wd), NULL);

	g_date_clear (&date, 1);
	g_date_set_dmy (&date, 5 + (int)wd, 3, 2006);
	g_date_strftime (buf, sizeof (buf) - 1,
			 abbrev ? "%a" : "%A",
			 &date);

	return deal_with_spaces (buf);
}

char *
go_date_month_name (GDateMonth m, gboolean abbrev)
{
	char buf[100];
	GDate date;

	g_return_val_if_fail (g_date_valid_month (m), NULL);

	g_date_clear (&date, 1);
	g_date_set_dmy (&date, 15, m, 2006);
	g_date_strftime (buf, sizeof (buf) - 1,
			 abbrev ? "%b" : "%B",
			 &date);

	return deal_with_spaces (buf);
}

/* ------------------------------------------------------------------------- */
