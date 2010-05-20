/*
 * go-quad.c:  Extended precision routines.
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
 *
 * This follows "A Floating-Point Technoque for Extending the Available
 * Precision" by T. J. Dekker in _Numerische Mathematik_ 18.
 * Springer Verlag 1971.
 *
 * Note: for this to work, the processor must evaluate with the right
 * precision.  For ix86 that means trouble as the default is to evaluate
 * with long-double precision internally.  We solve this by setting the
 * relevant x87 control flag.
 *
 * Note: the compiler should not rearrange expressions.
 */

#include <goffice/goffice-config.h>
#include "go-quad.h"
#include <math.h>

/* Normalize cpu id.  */
#if !defined(i386) && (defined(__i386__) || defined(__i386))
#define i386 1
#endif

#ifndef DOUBLE

#ifdef i386
#ifdef HAVE_FPU_CONTROL_H
#include <fpu_control.h>
#define USE_FPU_CONTROL
#elif defined(__GNUC__)
/* The next few lines from glibc licensed under lpgl 2.1 */
/* FPU control word bits.  i387 version.
   Copyright (C) 1993,1995-1998,2000,2001,2003 Free Software Foundation, Inc. */
#define _FPU_EXTENDED 0x300	/* libm requires double extended precision.  */
#define _FPU_DOUBLE   0x200
#define _FPU_SINGLE   0x0
typedef unsigned int fpu_control_t __attribute__ ((__mode__ (__HI__)));
#define _FPU_GETCW(cw) __asm__ __volatile__ ("fnstcw %0" : "=m" (*&cw))
#define _FPU_SETCW(cw) __asm__ __volatile__ ("fldcw %0" : : "m" (*&cw))
#define USE_FPU_CONTROL
#endif
#endif

#define QUAD SUFFIX(GOQuad)

#define DOUBLE double
#define SUFFIX(_n) _n
#define DOUBLE_MANT_DIG DBL_MANT_DIG

#ifdef GOFFICE_WITH_LONG_DOUBLE
#include "go-quad.c"
#undef DOUBLE
#undef SUFFIX
#undef DOUBLE_MANT_DIG
#define DOUBLE long double
#define SUFFIX(_n) _n ## l
#define DOUBLE_MANT_DIG LDBL_MANT_DIG
#endif

#endif

gboolean
SUFFIX(go_quad_functional) (void)
{
	if (FLT_RADIX != 2)
		return FALSE;

#ifdef i386
	if (sizeof (DOUBLE) != sizeof (double))
		return TRUE;

#ifdef USE_FPU_CONTROL
	return TRUE;
#else
	return FALSE;
#endif
#else
	return TRUE;
#endif
}

static DOUBLE SUFFIX(CST);

void *
SUFFIX(go_quad_start) (void)
{
	void *res = NULL;

	if (!SUFFIX(go_quad_functional) ())
		g_warning ("quad precision math may not be completely accurate.");

#ifdef i386
	if (sizeof (DOUBLE) == sizeof (double)) {
#ifdef USE_FPU_CONTROL
		fpu_control_t state, newstate;
		fpu_control_t mask =
			_FPU_EXTENDED | _FPU_DOUBLE | _FPU_SINGLE;
		_FPU_GETCW (state);
		newstate = (state & ~mask) | _FPU_DOUBLE;
		_FPU_SETCW (newstate);

		res = g_memdup (&state, sizeof (state));
#else
		/* Hope for the best.  */
#endif
	}
#endif

	SUFFIX(CST) = 1 + SUFFIX(ldexp) (1.0, (DOUBLE_MANT_DIG + 1) / 2);

	return res;
}

void
SUFFIX(go_quad_end) (void *state)
{
	if (!state)
		return;

#ifdef i386
#ifdef USE_FPU_CONTROL
	_FPU_SETCW (*(fpu_control_t*)state);
#endif
#endif

	g_free (state);
}

void
SUFFIX(go_quad_init) (QUAD *res, DOUBLE h)
{
	res->h = h;
	res->l = 0;
}

DOUBLE
SUFFIX(go_quad_value) (const QUAD *a)
{
	return a->h + a->l;
}

void
SUFFIX(go_quad_add) (QUAD *res, const QUAD *a, const QUAD *b)
{
	DOUBLE r = a->h + b->h;
	DOUBLE s = SUFFIX(fabs) (a->h) > SUFFIX(fabs) (b->h)
		? a->h - r + b->h + b->l + a->l
		: b->h - r + a->h + a->l + b->l;
	res->h = r + s;
	res->l = r - res->h + s;
}

void
SUFFIX(go_quad_sub) (QUAD *res, const QUAD *a, const QUAD *b)
{
	DOUBLE r = a->h - b->h;
	DOUBLE s = SUFFIX(fabs) (a->h) > SUFFIX(fabs) (b->h)
		? +a->h - r - b->h - b->l + a->l
		: -b->h - r + a->h + a->l - b->l;
	res->h = r + s;
	res->l = r - res->h + s;
}

void
SUFFIX(go_quad_mul12) (QUAD *res, DOUBLE x, DOUBLE y)
{
	DOUBLE p1 = x * SUFFIX(CST);
	DOUBLE hx = x - p1 + p1;
	DOUBLE tx = x - hx;

	DOUBLE p2 = y * SUFFIX(CST);
	DOUBLE hy = y - p2 + p2;
	DOUBLE ty = y - hy;

	DOUBLE p = hx * hy;
	DOUBLE q = hx * ty + tx * hy;
	res->h = p + q;
	res->l = p - res->h + q + tx * ty;
}

void
SUFFIX(go_quad_mul) (QUAD *res, const QUAD *a, const QUAD *b)
{
	QUAD c;
	SUFFIX(go_quad_mul12) (&c, a->h, b->h);
	c.l = a->h * b->l + a->l * b->h + c.l;
	res->h = c.h + c.l;
	res->l = c.h - res->h + c.l;
}

void
SUFFIX(go_quad_div) (QUAD *res, const QUAD *a, const QUAD *b)
{
	QUAD c, u;
	c.h = a->h / b->h;
	SUFFIX(go_quad_mul12) (&u, c.h, b->h);
	c.l = (a->h - u.h - u.l + a->l - c.h * b->l) / b->h;
	res->h = c.h + c.l;
	res->l = c.h - res->h + c.l;
}

void
SUFFIX(go_quad_sqrt) (QUAD *res, const QUAD *a)
{
	if (a->h > 0) {
		QUAD c, u;
		c.h = SUFFIX(sqrt) (a->h);
		SUFFIX(go_quad_mul12) (&u, c.h, c.h);
		c.l = (a->h - u.h - u.l + a->l) * 0.5 / c.h;
		res->h = c.h + c.l;
		res->l = c.h - res->h + c.l;
	} else
		res->h = res->l = 0;
}

void
SUFFIX(go_quad_dot_product) (QUAD *res, const QUAD *a, const QUAD *b, int n)
{
	int i;
	SUFFIX(go_quad_init) (res, 0);
	for (i = 0; i < n; i++) {
		QUAD d;
		SUFFIX(go_quad_mul) (&d, &a[i], &b[i]);
		SUFFIX(go_quad_add) (res, res, &d);
	}
}
