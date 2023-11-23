#ifndef GOFFICE_COMPLEX_H
#define GOFFICE_COMPLEX_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	double re, im;
} GOComplex;
#define go_complex GOComplex

#ifdef GOFFICE_WITH_LONG_DOUBLE
typedef struct {
	long double re, im;
} GOComplexl;
#define go_complexl GOComplexl
#endif

#ifdef GOFFICE_WITH_DECIMAL64
typedef struct {
	_Decimal64 re, im;
} GOComplexD;
#define go_complexD GOComplexD
#endif

#include "go-math.h"

/* ------------------------------------------------------------------------- */

GType go_complex_get_type (void);

char *go_complex_to_string (GOComplex const *src, char const *reformat,
			    char const *imformat, char imunit);

int go_complex_from_string (GOComplex *dst, char const *src, char *imunit);

void go_complex_to_polar (double *mod, double *angle, GOComplex const *src);
void go_complex_from_polar (GOComplex *dst, double mod, double angle);
void go_complex_from_polar_pi (GOComplex *dst, double mod, double angle);
void go_complex_mul  (GOComplex *dst, GOComplex const *a, GOComplex const *b);
void go_complex_div  (GOComplex *dst, GOComplex const *a, GOComplex const *b);
void go_complex_pow  (GOComplex *dst, GOComplex const *a, GOComplex const *b);
void go_complex_powx (GOComplex *dst, double *e,
		      GOComplex const *a, GOComplex const *b);
void go_complex_sqrt (GOComplex *dst, GOComplex const *src);
void go_complex_init (GOComplex *dst, double re, double im);
void go_complex_invalid (GOComplex *dst);
void go_complex_real (GOComplex *dst, double re);
int go_complex_real_p (GOComplex const *src);
int go_complex_zero_p (GOComplex const *src);
int go_complex_invalid_p (GOComplex const *src);
double go_complex_mod (GOComplex const *src);
double go_complex_angle (GOComplex const *src);
double go_complex_angle_pi (GOComplex const *src);
void go_complex_conj (GOComplex *dst, GOComplex const *src);
void go_complex_scale_real (GOComplex *dst, double f);
void go_complex_add (GOComplex *dst, GOComplex const *a, GOComplex const *b);
void go_complex_sub (GOComplex *dst, GOComplex const *a, GOComplex const *b);
void go_complex_exp (GOComplex *dst, GOComplex const *src);
void go_complex_ln (GOComplex *dst, GOComplex const *src);
void go_complex_sin (GOComplex *dst, GOComplex const *src);
void go_complex_cos (GOComplex *dst, GOComplex const *src);
void go_complex_tan (GOComplex *dst, GOComplex const *src);

/* ------------------------------------------------------------------------- */
/* long double version                                                       */

#ifdef GOFFICE_WITH_LONG_DOUBLE

GType go_complexl_get_type (void);

char *go_complex_to_stringl (GOComplexl const *src, char const *reformat,
			     char const *imformat, char imunit);

int go_complex_from_stringl (GOComplexl *dst, char const *src, char *imunit);

void go_complex_to_polarl (long double *mod, long double *angle, GOComplexl const *src);
void go_complex_from_polarl (GOComplexl *dst, long double mod, long double angle);
void go_complex_from_polar_pil (GOComplexl *dst, long double mod, long double angle);
void go_complex_mull  (GOComplexl *dst, GOComplexl const *a, GOComplexl const *b);
void go_complex_divl  (GOComplexl *dst, GOComplexl const *a, GOComplexl const *b);
void go_complex_powl  (GOComplexl *dst, GOComplexl const *a, GOComplexl const *b);
void go_complex_powxl  (GOComplexl *dst, long double *e,
			GOComplexl const *a, GOComplexl const *b);
void go_complex_sqrtl (GOComplexl *dst, GOComplexl const *src);
void go_complex_initl (GOComplexl *dst, long double re, long double im);
void go_complex_invalidl (GOComplexl *dst);
void go_complex_reall (GOComplexl *dst, long double re);
int go_complex_real_pl (GOComplexl const *src);
int go_complex_zero_pl (GOComplexl const *src);
int go_complex_invalid_pl (GOComplexl const *src);
long double go_complex_modl (GOComplexl const *src);
long double go_complex_anglel (GOComplexl const *src);
long double go_complex_angle_pil (GOComplexl const *src);
void go_complex_conjl (GOComplexl *dst, GOComplexl const *src);
void go_complex_scale_reall (GOComplexl *dst, long double f);
void go_complex_addl (GOComplexl *dst, GOComplexl const *a, GOComplexl const *b);
void go_complex_subl (GOComplexl *dst, GOComplexl const *a, GOComplexl const *b);
void go_complex_expl (GOComplexl *dst, GOComplexl const *src);
void go_complex_lnl (GOComplexl *dst, GOComplexl const *src);
void go_complex_sinl (GOComplexl *dst, GOComplexl const *src);
void go_complex_cosl (GOComplexl *dst, GOComplexl const *src);
void go_complex_tanl (GOComplexl *dst, GOComplexl const *src);


#endif	/* GOFFICE_WITH_LONG_DOUBLE */

/* ------------------------------------------------------------------------- */
/* _Decimal64 version                                                        */

#ifdef GOFFICE_WITH_DECIMAL64

GType go_complexD_get_type (void);

char *go_complex_to_stringD (GOComplexD const *src, char const *reformat,
			     char const *imformat, char imunit);

int go_complex_from_stringD (GOComplexD *dst, char const *src, char *imunit);

void go_complex_to_polarD (_Decimal64 *mod, _Decimal64 *angle, GOComplexD const *src);
void go_complex_from_polarD (GOComplexD *dst, _Decimal64 mod, _Decimal64 angle);
void go_complex_from_polar_piD (GOComplexD *dst, _Decimal64 mod, _Decimal64 angle);
void go_complex_mulD (GOComplexD *dst, GOComplexD const *a, GOComplexD const *b);
void go_complex_divD (GOComplexD *dst, GOComplexD const *a, GOComplexD const *b);
void go_complex_powD (GOComplexD *dst, GOComplexD const *a, GOComplexD const *b);
void go_complex_powxD (GOComplexD *dst, _Decimal64 *e,
		       GOComplexD const *a, GOComplexD const *b);
void go_complex_sqrtD (GOComplexD *dst, GOComplexD const *src);
void go_complex_initD (GOComplexD *dst, _Decimal64 re, _Decimal64 im);
void go_complex_invalidD (GOComplexD *dst);
void go_complex_realD (GOComplexD *dst, _Decimal64 re);
int go_complex_real_pD (GOComplexD const *src);
int go_complex_zero_pD (GOComplexD const *src);
int go_complex_invalid_pD (GOComplexD const *src);
_Decimal64 go_complex_modD (GOComplexD const *src);
_Decimal64 go_complex_angleD (GOComplexD const *src);
_Decimal64 go_complex_angle_piD (GOComplexD const *src);
void go_complex_conjD (GOComplexD *dst, GOComplexD const *src);
void go_complex_scale_realD (GOComplexD *dst, _Decimal64 f);
void go_complex_addD (GOComplexD *dst, GOComplexD const *a, GOComplexD const *b);
void go_complex_subD (GOComplexD *dst, GOComplexD const *a, GOComplexD const *b);
void go_complex_expD (GOComplexD *dst, GOComplexD const *src);
void go_complex_lnD (GOComplexD *dst, GOComplexD const *src);
void go_complex_sinD (GOComplexD *dst, GOComplexD const *src);
void go_complex_cosD (GOComplexD *dst, GOComplexD const *src);
void go_complex_tanD (GOComplexD *dst, GOComplexD const *src);


#endif	/* GOFFICE_WITH_DECIMAL64 */

/* ------------------------------------------------------------------------- */

G_END_DECLS

#endif

