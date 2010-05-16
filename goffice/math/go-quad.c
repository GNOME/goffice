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

#if defined(i386) && defined(HAVE_FPU_CONTROL_H)
#include <fpu_control.h>
#endif

#ifndef DOUBLE

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

#ifdef HAVE_FPU_CONTROL_H
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

	if (!go_quad_functional ())
		g_warning ("quad precision math may not be completely accurate.");

#ifdef i386
	if (sizeof (DOUBLE) == sizeof (double)) {
#ifdef HAVE_FPU_CONTROL_H
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
#ifdef HAVE_FPU_CONTROL_H
	_FPU_SETCW (*(fpu_control_t*)state);
#endif
#endif

	g_free (state);
}

void
SUFFIX(go_quad_init) (QUAD *res, DOUBLE h, DOUBLE l)
{
	res->h = h;
	res->l = l;
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

int
SUFFIX(go_quad_sgn) (const QUAD *a)
{
	DOUBLE d = (a->h == 0 ? a->l : a->h);

	if (d == 0)
		return 0;
	else if (d > 0)
		return +1;
	else
		return -1;
}
