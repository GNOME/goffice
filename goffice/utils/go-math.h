#ifndef __GO_MATH_H
#define __GO_MATH_H

#include <math.h>

/* ------------------------------------------------------------------------- */

void gnm_continued_fraction (double val, int max_denom, int *res_num, int *res_denom);
void go_stern_brocot (double val, int max_denom, int *res_num, int *res_denom);

/* ------------------------------------------------------------------------- */

extern double go_nan;
extern double go_pinf;
extern double go_ninf;

double go_add_epsilon (double x);
double go_sub_epsilon (double x);
double go_fake_floor (double x);
double go_fake_ceil (double x);
double go_fake_round (double x);
double go_fake_trunc (double x);

int go_finite (double x);
double go_pow2 (int n);
double go_pow10 (int n);

/* ------------------------------------------------------------------------- */

#ifdef HAVE_LONG_DOUBLE

extern long double go_nanl;
extern long double go_pinfl;
extern long double go_ninfl;

long double go_add_epsilonl (long double x);
long double go_sub_epsilonl (long double x);
long double go_fake_floorl (long double x);
long double go_fake_ceill (long double x);
long double go_fake_roundl (long double x);
long double go_fake_truncl (long double x);

#define go_finitel finitel
long double go_pow2l (int n);
long double go_pow10l (int n);

/*
 * We provide working versions of these functions for long doubles.
 */ 
#ifndef HAVE_LDEXPL
long double ldexpl (long double x, int exp);
#endif
#ifndef HAVE_FREXPL
long double frexpl (long double x, int *exp);
#endif
#if defined(MUST_PROTOTYPE_STRTOLD) || !defined(HAVE_STRTOLD)
long double strtold (const char *, char **);
#endif
#ifndef HAVE_EXPM1L
long double expm1l (long double x);
#endif
#ifndef HAVE_ASINHL
long double asinhl (long double x);
#endif
#ifndef HAVE_ACOSHL
long double acoshl (long double x);
#endif
#ifndef HAVE_ATANHL
long double atanhl (long double x);
#endif
#ifndef HAVE_MODFL
long double modfl (long double x, long double *iptr);
#endif

#endif

/* ------------------------------------------------------------------------- */

void go_math_init (void);

/* ------------------------------------------------------------------------- */

#endif	/* __GO_MATH_H */
