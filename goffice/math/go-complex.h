#ifndef GOFFICE_COMPLEX_H
#define GOFFICE_COMPLEX_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	double re, im;
} go_complex;

#ifdef GOFFICE_WITH_LONG_DOUBLE
typedef struct {
	long double re, im;
} go_complexl;
#endif

#include "go-math.h"

/* ------------------------------------------------------------------------- */

char *go_complex_to_string (go_complex const *src, char const *reformat,
			 char const *imformat, char imunit);

int go_complex_from_string (go_complex *dst, char const *src, char *imunit);

void go_complex_to_polar (double *mod, double *angle, go_complex const *src);
void go_complex_from_polar (go_complex *dst, double mod, double angle);
void go_complex_mul  (go_complex *dst, go_complex const *a, go_complex const *b);
void go_complex_div  (go_complex *dst, go_complex const *a, go_complex const *b);
void go_complex_pow  (go_complex *dst, go_complex const *a, go_complex const *b);
void go_complex_sqrt (go_complex *dst, go_complex const *src);
void go_complex_init (go_complex *dst, double re, double im);
void go_complex_invalid (go_complex *dst);
void go_complex_real (go_complex *dst, double re);
int go_complex_real_p (go_complex const *src);
int go_complex_zero_p (go_complex const *src);
int go_complex_invalid_p (go_complex const *src);
double go_complex_mod (go_complex const *src);
double go_complex_angle (go_complex const *src);
double go_complex_angle_pi (go_complex const *src);
void go_complex_conj (go_complex *dst, go_complex const *src);
void go_complex_scale_real (go_complex *dst, double f);
void go_complex_add (go_complex *dst, go_complex const *a, go_complex const *b);
void go_complex_sub (go_complex *dst, go_complex const *a, go_complex const *b);
void go_complex_exp (go_complex *dst, go_complex const *src);
void go_complex_ln (go_complex *dst, go_complex const *src);
void go_complex_sin (go_complex *dst, go_complex const *src);
void go_complex_cos (go_complex *dst, go_complex const *src);
void go_complex_tan (go_complex *dst, go_complex const *src);

/* ------------------------------------------------------------------------- */
/* long double version                                                       */

#ifdef GOFFICE_WITH_LONG_DOUBLE

char *go_complex_to_stringl (go_complexl const *src, char const *reformat,
			 char const *imformat, char imunit);

int go_complex_from_stringl (go_complexl *dst, char const *src, char *imunit);

void go_complex_to_polarl (long double *mod, long double *angle, go_complexl const *src);
void go_complex_from_polarl (go_complexl *dst, long double mod, long double angle);
void go_complex_mull  (go_complexl *dst, go_complexl const *a, go_complexl const *b);
void go_complex_divl  (go_complexl *dst, go_complexl const *a, go_complexl const *b);
void go_complex_powl  (go_complexl *dst, go_complexl const *a, go_complexl const *b);
void go_complex_sqrtl (go_complexl *dst, go_complexl const *src);
void go_complex_initl (go_complexl *dst, long double re, long double im);
void go_complex_invalidl (go_complexl *dst);
void go_complex_reall (go_complexl *dst, long double re);
int go_complex_real_pl (go_complexl const *src);
int go_complex_zero_pl (go_complexl const *src);
int go_complex_invalid_pl (go_complexl const *src);
long double go_complex_modl (go_complexl const *src);
long double go_complex_anglel (go_complexl const *src);
long double go_complex_angle_pil (go_complexl const *src);
void go_complex_conjl (go_complexl *dst, go_complexl const *src);
void go_complex_scale_reall (go_complexl *dst, long double f);
void go_complex_addl (go_complexl *dst, go_complexl const *a, go_complexl const *b);
void go_complex_subl (go_complexl *dst, go_complexl const *a, go_complexl const *b);
void go_complex_expl (go_complexl *dst, go_complexl const *src);
void go_complex_lnl (go_complexl *dst, go_complexl const *src);
void go_complex_sinl (go_complexl *dst, go_complexl const *src);
void go_complex_cosl (go_complexl *dst, go_complexl const *src);
void go_complex_tanl (go_complexl *dst, go_complexl const *src);


#endif	/* GOFFICE_WITH_LONG_DOUBLE */

/* ------------------------------------------------------------------------- */

G_END_DECLS

#endif

