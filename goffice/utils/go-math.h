#ifndef __GO_MATH_H
#define __GO_MATH_H

/* ------------------------------------------------------------------------- */

extern double go_nan;
extern double go_pinf;
extern double go_ninf;

/* ------------------------------------------------------------------------- */

double go_add_epsilon (double x);
double go_sub_epsilon (double x);

double go_fake_floor (double x);
double go_fake_ceil (double x);
double go_fake_trunc (double x);

int go_finite (double x);

/* ------------------------------------------------------------------------- */

void go_math_init (void);

/* ------------------------------------------------------------------------- */

#include <goffice/utils/numbers.h>

gnm_float gnm_add_epsilon (gnm_float x);
gnm_float gnm_sub_epsilon (gnm_float x);

gnm_float gnm_fake_floor  (gnm_float x);
gnm_float gnm_fake_ceil   (gnm_float x);
gnm_float gnm_fake_round  (gnm_float x);
gnm_float gnm_fake_trunc  (gnm_float x);

gnm_float gnm_pow10 (int n);
gnm_float gnm_pow2  (int n);

/* ------------------------------------------------------------------------- */

void gnm_continued_fraction (gnm_float val, int max_denom, int *res_num, int *res_denom);
void go_stern_brocot (double val, int max_denom, int *res_num, int *res_denom);

/* ------------------------------------------------------------------------- */

#endif	/* __GO_MATH_H */
