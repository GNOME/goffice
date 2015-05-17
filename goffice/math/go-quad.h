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
void go_quad_sqrt (GOQuad *res, const GOQuad *a);
void go_quad_floor (GOQuad *res, const GOQuad *a);
void go_quad_pow (GOQuad *res, double *exp2, const GOQuad *x, const GOQuad *y);
void go_quad_exp (GOQuad *res, double *exp2, const GOQuad *a);
void go_quad_expm1 (GOQuad *res, const GOQuad *a);
void go_quad_log (GOQuad *res, const GOQuad *a);
void go_quad_hypot (GOQuad *res, const GOQuad *a, const GOQuad *b);

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
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_pi;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_2pi;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_e;
GO_VAR_DECL GO_QUAD_IMPL GOQuad go_quad_ln2;
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
void go_quad_sqrtl (GOQuadl *res, const GOQuadl *a);
void go_quad_floorl (GOQuadl *res, const GOQuadl *a);
void go_quad_powl (GOQuadl *res, long double *exp2, const GOQuadl *x, const GOQuadl *y);
void go_quad_expl (GOQuadl *res, long double *exp2, const GOQuadl *a);
void go_quad_expm1l (GOQuadl *res, const GOQuadl *a);
void go_quad_logl (GOQuadl *res, const GOQuadl *a);
void go_quad_hypotl (GOQuadl *res, const GOQuadl *a, const GOQuadl *b);

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
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_pil;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_2pil;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_el;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_ln2l;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_sqrt2l;
GO_VAR_DECL GO_QUAD_IMPL GOQuadl go_quad_eulerl;

#endif

G_END_DECLS

#endif
