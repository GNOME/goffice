#ifndef __GO_QUAD_H
#define __GO_QUAD_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
	double h;
	double l;
} GOQuad;

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

void go_quad_mul12 (GOQuad *res, double x, double y);

void go_quad_dot_product (GOQuad *res, const GOQuad *a, const GOQuad *b, int n);

#ifdef GOFFICE_WITH_LONG_DOUBLE
typedef struct {
	long double h;
	long double l;
} GOQuadl;

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

void go_quad_mul12l (GOQuadl *res, long double x, long double y);

void go_quad_dot_productl (GOQuadl *res,
			   const GOQuadl *a, const GOQuadl *b, int n);
#endif

G_END_DECLS

#endif
