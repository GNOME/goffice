#ifndef GO_RANGEFUNC_H
#define GO_RANGEFUNC_H
#include <glib.h>

G_BEGIN_DECLS

int go_range_sum (double const *xs, int n, double *res);
int go_range_sumsq (double const *xs, int n, double *res);
int go_range_average (double const *xs, int n, double *res);
int go_range_min (double const *xs, int n, double *res);
int go_range_max (double const *xs, int n, double *res);
int go_range_maxabs (double const *xs, int n, double *res);
int go_range_devsq (double const *xs, int n, double *res);
int go_range_fractile_inter (double const *xs, int n, double *res, double f);
int go_range_fractile_inter_nonconst (double *xs, int n, double *res, double f);
int go_range_fractile_inter_sorted (double const *xs, int n, double *res, double f);
int go_range_median_inter (double const *xs, int n, double *res);
int go_range_median_inter_nonconst (double *xs, int n, double *res);
int go_range_median_inter_sorted (double const *xs, int n, double *res);
int go_range_increasing (double const *xs, int n);
int go_range_decreasing (double const *xs, int n);
int go_range_vary_uniformly (double const *xs, int n);
double *go_range_sort (double const *xs, int n);

#ifdef GOFFICE_WITH_LONG_DOUBLE
int go_range_suml (long double const *xs, int n, long double *res);
int go_range_sumsql (long double const *xs, int n, long double *res);
int go_range_averagel (long double const *xs, int n, long double *res);
int go_range_minl (long double const *xs, int n, long double *res);
int go_range_maxl (long double const *xs, int n, long double *res);
int go_range_maxabsl (long double const *xs, int n, long double *res);
int go_range_devsql (long double const *xs, int n, long double *res);
int go_range_fractile_interl (long double const *xs, int n, long double *res, long double f);
int go_range_fractile_inter_nonconstl (long double *xs, int n, long double *res, long double f);
int go_range_fractile_inter_sortedl (long double const *xs, int n, long double *res, long double f);
int go_range_median_interl (long double const *xs, int n, long double *res);
int go_range_median_inter_nonconstl (long double *xs, int n, long double *res);
int go_range_median_inter_sortedl (long double const *xs, int n, long double *res);
int go_range_increasingl (long double const *xs, int n);
int go_range_decreasingl (long double const *xs, int n);
int go_range_vary_uniformlyl (long double const *xs, int n);
long double *go_range_sortl (long double const *xs, int n);
#endif

G_END_DECLS

#endif	/* GO_RANGEFUNC_H */
