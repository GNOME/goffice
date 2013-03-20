#ifndef _GO_DATETIME_H_
#define _GO_DATETIME_H_

#include <goffice/goffice.h>
#include <time.h>

G_BEGIN_DECLS

/**
 * GODateConventions:
 * @use_1904: Base on 1904 as opposed to 1900.
 */
struct _GODateConventions {
	gboolean use_1904;	/* Use MacOffice 1904 based date convention,
				 * Rather than the Win32 style 1900 */
};

GType go_date_conventions_get_type (void);

/*
 * Naming conventions:
 *
 * "g": a GDate *.
 * "timet": Unix' time_t.
 * "serial": Excel serial day number.
 * "serial_raw": serial plus time as fractional day.
 */

/* Week numbering methods */
/* 1:   Week starts on Sunday. Days before first Sunday are in week 0. */
/* 2:   Week starts on Monday. Days before first Monday are in week 0. */
/* 150: ISO 8601 week number. */
enum {
	GO_WEEKNUM_METHOD_SUNDAY = 1,
	GO_WEEKNUM_METHOD_MONDAY = 2,
	GO_WEEKNUM_METHOD_ISO = 150
};

/* These do not round and produces fractional values, i.e., includes time.  */
double	go_date_timet_to_serial_raw  (time_t t, GODateConventions const *conv);

/* These are date-only, no time.  */
int	go_date_timet_to_serial      (time_t t, GODateConventions const *conv);
int	go_date_g_to_serial	      (GDate const *date, GODateConventions const *conv);
void	go_date_serial_to_g	      (GDate *res, int serial, GODateConventions const *conv);
time_t	go_date_serial_to_timet      (int serial, GODateConventions const *conv);
int	go_date_serial_raw_to_serial (double raw);

/* These are time-only assuming a 24h day.  It probably loses completely on */
/* days with summer time ("daylight savings") changes.  */
int go_date_timet_to_seconds (time_t t);
int go_date_serial_raw_to_seconds (double raw);

/* Number of full months between date1 and date2. */
/* largest value s.t. g_date_add_months (date1, result) <= date2 */
/* except that if the day is decreased in g_date_add_monts, treat
   that as > the date it is decreased to. */
/* ( go_date_g_months_between ( March 31, April 30 ) == 0
     even though g_date_add_months ( Mar 31, 1 ) <= Apr 30.... */
int go_date_g_months_between (GDate const *date1, GDate const *date2);
/* Number of full years between date1 and date2. */
/* (g_date_add_years (date1, result) <= date2; largest such value. */
/*  treat add_years (29-feb, x) > 28-feb ) */
int go_date_g_years_between (GDate const *date1, GDate const *date2);
/* week number according to the given method. */
int go_date_weeknum (GDate const *date, int method);

/**
 * GOBasisType:
 * @GO_BASIS_MSRB_30_360: US 30/360 (days in a month/days in a year)
 * @GO_BASIS_ACT_ACT: actual days/actual days
 * @GO_BASIS_ACT_360: actual days/360
 * @GO_BASIS_ACT_365: actual days/365
 * @GO_BASIS_30E_360: European 30/360
 * @GO_BASIS_30Ep_360: ?
 * @GO_BASIS_MSRB_30_360_SYM: ?
 **/
typedef enum { /* see doc/fn-financial-basis.txt for details */
	GO_BASIS_MSRB_30_360     = 0,
	GO_BASIS_ACT_ACT         = 1,
	GO_BASIS_ACT_360         = 2,
	GO_BASIS_ACT_365         = 3,
	GO_BASIS_30E_360         = 4,
	GO_BASIS_30Ep_360        = 5,
	GO_BASIS_MSRB_30_360_SYM = 6         /* Gnumeric extension.  */
} GOBasisType;
#define go_basis_t GOBasisType /* for compatibility */

gint32  go_date_days_between_basis (GDate const *from, GDate const *to, GOBasisType basis);

/**
 * GoCouponConvention:
 * @freq: frequency.
 * @basis: #GOBasisType
 * @eom: end of month.
 * @date_conv: #GODateConventions
 **/
typedef struct {
	int	 freq;
	GOBasisType  basis;
	gboolean eom;
	GODateConventions const *date_conv;
} GoCouponConvention;

void   go_coup_cd    (GDate *res, GDate const *settle, GDate const *mat,
		      int freq, gboolean eom, gboolean next);
double go_coupdays   (GDate const *settlement, GDate const *maturity,
		      GoCouponConvention const *conv);
double go_coupdaybs  (GDate const *settlement, GDate const *maturity,
		      GoCouponConvention const *conv);
double go_coupdaysnc (GDate const *settlement, GDate const *maturity,
		      GoCouponConvention const *conv);

int go_date_convention_base (GODateConventions const *conv);

const GODateConventions *go_date_conv_from_str (const char *s);
gboolean go_date_conv_equal (const GODateConventions *a, const GODateConventions *b);
double go_date_conv_translate (double f,
			       const GODateConventions *src,
			       const GODateConventions *dst);

char *go_date_weekday_name (GDateWeekday wd, gboolean abbrev);
char *go_date_month_name (GDateMonth m, gboolean abbrev);


G_END_DECLS

#endif /* _GO_DATETIME_H_ */
