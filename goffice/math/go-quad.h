#ifndef __GO_QUAD_H
#define __GO_QUAD_H

#include <glib.h>

G_BEGIN_DECLS

struct GOQuad_ {
	double h;
	double l;
};

gboolean go_quad_functional (void);
void *go_quad_start (void);
void go_quad_end (void *state);

void go_quad_init (GOQuad *res, double h);

double go_quad_value (const GOQuad *a);
void go_quad_add (GOQuad *res, const GOQuad *a, const GOQuad *b);
void go_quad_sub (GOQuad *res, const GOQuad *a, const GOQuad *b);
void go_quad_mul (GOQuad *res, const GOQuad *a, const GOQuad *b);
void go_quad_div (GOQuad *res, const GOQuad *a, const GOQuad *b);
void go_quad_scalbn (GOQuad *res, const GOQuad *a, int n);
void go_quad_sqrt (GOQuad *res, const GOQuad *a);
void go_quad_floor (GOQuad *res, const GOQuad *a);
void go_quad_pow (GOQuad *res, double *expb, const GOQuad *x, const GOQuad *y);
void go_quad_exp (GOQuad *res, double *expb, const GOQuad *a);
void go_quad_expm1 (GOQuad *res, const GOQuad *a);
void go_quad_log (GOQuad *res, const GOQuad *a);
void go_quad_hypot (GOQuad *res, const GOQuad *a, const GOQuad *b);
void go_quad_abs (GOQuad *res, const GOQuad *a);
void go_quad_negate (GOQuad *res, const GOQuad *a);

void go_quad_sin (GOQuad *res, const GOQuad *a);
void go_quad_sinpi (GOQuad *res, const GOQuad *a);
void go_quad_asin (GOQuad *res, const GOQuad *a);
void go_quad_cos (GOQuad *res, const GOQuad *a);
void go_quad_cospi (GOQuad *res, const GOQuad *a);
void go_quad_acos (GOQuad *res, const GOQuad *a);
void go_quad_atan2 (GOQuad *res, const GOQuad *y, const GOQuad *x);
void go_quad_atan2pi (GOQuad *res, const GOQuad *y, const GOQuad *x);

void go_quad_mul12 (GOQuad *res, double x, double y);

void go_quad_dot_product (GOQuad *res, const GOQuad *a, const GOQuad *b, int n);

void go_quad_constant8 (GOQuad *res, const guint8 *data, gsize n, double base, double scale);

#ifndef GO_QUAD_IMPL
#define GO_QUAD_IMPL const
#endif

GO_VAR_DECL const GOQuad go_quad_zero;
GO_VAR_DECL const GOQuad go_quad_one;
GO_VAR_DECL const GOQuad go_quad_half;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_pi;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_2pi;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_pihalf;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_e;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_ln2;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_ln10;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_sqrt2;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_euler;

#ifdef GOFFICE_WITH_LONG_DOUBLE
struct GOQuadl_ {
	long double h;
	long double l;
};

gboolean go_quad_functionall (void);
void *go_quad_startl (void);
void go_quad_endl (void *state);

void go_quad_initl (GOQuadl *res, long double h);

long double go_quad_valuel (const GOQuadl *a);
void go_quad_addl (GOQuadl *res, const GOQuadl *a, const GOQuadl *b);
void go_quad_subl (GOQuadl *res, const GOQuadl *a, const GOQuadl *b);
void go_quad_mull (GOQuadl *res, const GOQuadl *a, const GOQuadl *b);
void go_quad_divl (GOQuadl *res, const GOQuadl *a, const GOQuadl *b);
void go_quad_scalbnl (GOQuadl *res, const GOQuadl *a, int n);
void go_quad_sqrtl (GOQuadl *res, const GOQuadl *a);
void go_quad_floorl (GOQuadl *res, const GOQuadl *a);
void go_quad_powl (GOQuadl *res, long double *expb, const GOQuadl *x, const GOQuadl *y);
void go_quad_expl (GOQuadl *res, long double *expb, const GOQuadl *a);
void go_quad_expm1l (GOQuadl *res, const GOQuadl *a);
void go_quad_logl (GOQuadl *res, const GOQuadl *a);
void go_quad_hypotl (GOQuadl *res, const GOQuadl *a, const GOQuadl *b);
void go_quad_absl (GOQuadl *res, const GOQuadl *a);
void go_quad_negatel (GOQuadl *res, const GOQuadl *a);

void go_quad_sinl (GOQuadl *res, const GOQuadl *a);
void go_quad_sinpil (GOQuadl *res, const GOQuadl *a);
void go_quad_asinl (GOQuadl *res, const GOQuadl *a);
void go_quad_cosl (GOQuadl *res, const GOQuadl *a);
void go_quad_cospil (GOQuadl *res, const GOQuadl *a);
void go_quad_acosl (GOQuadl *res, const GOQuadl *a);
void go_quad_atan2l (GOQuadl *res, const GOQuadl *y, const GOQuadl *x);
void go_quad_atan2pil (GOQuadl *res, const GOQuadl *y, const GOQuadl *x);

void go_quad_mul12l (GOQuadl *res, long double x, long double y);

void go_quad_dot_productl (GOQuadl *res,
			   const GOQuadl *a, const GOQuadl *b, int n);

void go_quad_constant8l (GOQuadl *res, const guint8 *data, gsize n, long double base, long double scale);

GO_VAR_DECL const GOQuadl go_quad_zerol;
GO_VAR_DECL const GOQuadl go_quad_onel;
GO_VAR_DECL const GOQuadl go_quad_halfl;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_pil;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_2pil;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_pihalfl;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_el;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_ln2l;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_ln10l;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_sqrt2l;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_eulerl;

#endif

#ifdef GOFFICE_WITH_DECIMAL64
struct GOQuadD_ {
	_Decimal64 h;
	_Decimal64 l;
};

gboolean go_quad_functionalD (void);
void *go_quad_startD (void);
void go_quad_endD (void *state);

void go_quad_initD (GOQuadD *res, _Decimal64 h);

_Decimal64 go_quad_valueD (const GOQuadD *a);
void go_quad_addD (GOQuadD *res, const GOQuadD *a, const GOQuadD *b);
void go_quad_subD (GOQuadD *res, const GOQuadD *a, const GOQuadD *b);
void go_quad_mulD (GOQuadD *res, const GOQuadD *a, const GOQuadD *b);
void go_quad_divD (GOQuadD *res, const GOQuadD *a, const GOQuadD *b);
void go_quad_scalbnD (GOQuadD *res, const GOQuadD *a, int n);
void go_quad_sqrtD (GOQuadD *res, const GOQuadD *a);
void go_quad_floorD (GOQuadD *res, const GOQuadD *a);
void go_quad_powD (GOQuadD *res, _Decimal64 *expb, const GOQuadD *x, const GOQuadD *y);
void go_quad_expD (GOQuadD *res, _Decimal64 *expb, const GOQuadD *a);
void go_quad_expm1D (GOQuadD *res, const GOQuadD *a);
void go_quad_logD (GOQuadD *res, const GOQuadD *a);
void go_quad_hypotD (GOQuadD *res, const GOQuadD *a, const GOQuadD *b);
void go_quad_absD (GOQuadD *res, const GOQuadD *a);
void go_quad_negateD (GOQuadD *res, const GOQuadD *a);

void go_quad_sinD (GOQuadD *res, const GOQuadD *a);
void go_quad_sinpiD (GOQuadD *res, const GOQuadD *a);
void go_quad_asinD (GOQuadD *res, const GOQuadD *a);
void go_quad_cosD (GOQuadD *res, const GOQuadD *a);
void go_quad_cospiD (GOQuadD *res, const GOQuadD *a);
void go_quad_acosD (GOQuadD *res, const GOQuadD *a);
void go_quad_atan2D (GOQuadD *res, const GOQuadD *y, const GOQuadD *x);
void go_quad_atan2piD (GOQuadD *res, const GOQuadD *y, const GOQuadD *x);

void go_quad_mul12D (GOQuadD *res, _Decimal64 x, _Decimal64 y);

void go_quad_dot_productD (GOQuadD *res,
			   const GOQuadD *a, const GOQuadD *b, int n);

void go_quad_constant8D (GOQuadD *res, const guint8 *data, gsize n, _Decimal64 base, _Decimal64 scale);

GO_VAR_DECL const GOQuadD go_quad_zeroD;
GO_VAR_DECL const GOQuadD go_quad_oneD;
GO_VAR_DECL const GOQuadD go_quad_halfD;
GO_VAR_DECL GO_QUAD_IMPL GOQuadD go_quad_piD;
GO_VAR_DECL GO_QUAD_IMPL GOQuadD go_quad_2piD;
GO_VAR_DECL GO_QUAD_IMPL GOQuadD go_quad_pihalfD;
GO_VAR_DECL GO_QUAD_IMPL GOQuadD go_quad_eD;
GO_VAR_DECL GO_QUAD_IMPL GOQuadD go_quad_ln2D;
GO_VAR_DECL GO_QUAD_IMPL GOQuadD go_quad_ln10D;
GO_VAR_DECL GO_QUAD_IMPL GOQuadD go_quad_sqrt2D;
GO_VAR_DECL GO_QUAD_IMPL GOQuadD go_quad_eulerD;
#endif

G_END_DECLS

#endif
