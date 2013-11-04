/*
 * go-quad.c:  Extended precision routines.
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 * This follows "A Floating-Point Technique for Extending the Available
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
#include <goffice/goffice.h>
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

static guint SUFFIX(go_quad_depth) = 0;

static DOUBLE SUFFIX(CST);

/**
 * go_quad_start: (skip)
 *
 * Initializes #GOQuad arythmetics. Any use of #GOQuad must occur between calls
 * to go_quad_start() and go_quad_end().
 * Returns: (transfer full): a pointer to pass to go_quad_end() when done.
 **/
/**
 * go_quad_startl: (skip)
 *
 * Initializes #GOQuadl arythmetics. Any use of #GOQuadl must occur between calls
 * to go_quad_startl() and go_quad_endl().
 * Returns: (transfer full): a pointer to pass to go_quad_endl() when done.
 **/
void *
SUFFIX(go_quad_start) (void)
{
	void *res = NULL;
	static gboolean first = TRUE;

	if (SUFFIX(go_quad_depth)++ > 0)
		return NULL;

	if (!SUFFIX(go_quad_functional) () && first)
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

	if (first) {
		/*
		 * Calculate constants in a way doesn't depend on the
		 * layout of DOUBLE.  We use ~400 bits of data in the
		 * tables below -- that's way more than needed even
		 * for sunos' long double.
		 */

		static const guint8 pi_hex_digits[] = {
			0x03, 0x24, 0x3f, 0x6a, 0x88, 0x85, 0xa3, 0x08,
			0xd3, 0x13, 0x19, 0x8a, 0x2e, 0x03, 0x70, 0x73,
			0x44, 0xa4, 0x09, 0x38, 0x22, 0x29, 0x9f, 0x31,
			0xd0, 0x08, 0x2e, 0xfa, 0x98, 0xec, 0x4e, 0x6c,
			0x89, 0x45, 0x28, 0x21, 0xe6, 0x38, 0xd0, 0x13,
			0x77, 0xbe, 0x54, 0x66, 0xcf, 0x34, 0xe9, 0x0c,
			0x6c, 0xc0, 0xac
		};

		static const guint8 e_hex_digits[] = {
			0x02, 0xb7, 0xe1, 0x51, 0x62, 0x8a, 0xed, 0x2a,
			0x6a, 0xbf, 0x71, 0x58, 0x80, 0x9c, 0xf4, 0xf3,
			0xc7, 0x62, 0xe7, 0x16, 0x0f, 0x38, 0xb4, 0xda,
			0x56, 0xa7, 0x84, 0xd9, 0x04, 0x51, 0x90, 0xcf,
			0xef, 0x32, 0x4e, 0x77, 0x38, 0x92, 0x6c, 0xfb,
			0xe5, 0xf4, 0xbf, 0x8d, 0x8d, 0x8c, 0x31, 0xd7,
			0x63, 0xda, 0x06
		};

		static const guint8 ln2_hex_digits[] = {
			0xb1, 0x72, 0x17, 0xf7, 0xd1, 0xcf, 0x79, 0xab,
			0xc9, 0xe3, 0xb3, 0x98, 0x03, 0xf2, 0xf6, 0xaf,
			0x40, 0xf3, 0x43, 0x26, 0x72, 0x98, 0xb6, 0x2d,
			0x8a, 0x0d, 0x17, 0x5b, 0x8b, 0xaa, 0xfa, 0x2b,
			0xe7, 0xb8, 0x76, 0x20, 0x6d, 0xeb, 0xac, 0x98,
			0x55, 0x95, 0x52, 0xfb, 0x4a, 0xfa, 0x1b, 0x10,
			0xed, 0x2e
		};

		static const guint8 sqrt2_hex_digits[] = {
			0x01, 0x6a, 0x09, 0xe6, 0x67, 0xf3, 0xbc, 0xc9,
			0x08, 0xb2, 0xfb, 0x13, 0x66, 0xea, 0x95, 0x7d,
			0x3e, 0x3a, 0xde, 0xc1, 0x75, 0x12, 0x77, 0x50,
			0x99, 0xda, 0x2f, 0x59, 0x0b, 0x06, 0x67, 0x32,
			0x2a, 0x95, 0xf9, 0x06, 0x08, 0x75, 0x71, 0x45,
			0x87, 0x51, 0x63, 0xfc, 0xdf, 0xb9, 0x07, 0xb6,
			0x72, 0x1e, 0xe9
		};

		static const guint8 euler_hex_digits[] = {
			0x93, 0xc4, 0x67, 0xe3, 0x7d, 0xb0, 0xc7, 0xa4,
			0xd1, 0xbe, 0x3f, 0x81, 0x01, 0x52, 0xcb, 0x56,
			0xa1, 0xce, 0xcc, 0x3a, 0xf6, 0x5c, 0xc0, 0x19,
			0x0c, 0x03, 0xdf, 0x34, 0x70, 0x9a, 0xff, 0xbd,
			0x8e, 0x4b, 0x59, 0xfa, 0x03, 0xa9, 0xf0, 0xee,
			0xd0, 0x64, 0x9c, 0xcb, 0x62, 0x10, 0x57, 0xd1,
			0x10, 0x56
		};

		first = FALSE;
		SUFFIX(CST) = 1 + SUFFIX(ldexp) (1.0, (DOUBLE_MANT_DIG + 1) / 2);
		SUFFIX(go_quad_constant8) ((QUAD *)&SUFFIX(go_quad_pi),
					   pi_hex_digits,
					   G_N_ELEMENTS (pi_hex_digits),
					   256.0,
					   256.0);

		SUFFIX(go_quad_constant8) ((QUAD *)&SUFFIX(go_quad_e),
					   e_hex_digits,
					   G_N_ELEMENTS (e_hex_digits),
					   256.0,
					   256.0);

		SUFFIX(go_quad_constant8) ((QUAD *)&SUFFIX(go_quad_ln2),
					   ln2_hex_digits,
					   G_N_ELEMENTS (ln2_hex_digits),
					   256.0,
					   1);

		SUFFIX(go_quad_constant8) ((QUAD *)&SUFFIX(go_quad_sqrt2),
					   sqrt2_hex_digits,
					   G_N_ELEMENTS (sqrt2_hex_digits),
					   256.0,
					   256.0);

		SUFFIX(go_quad_constant8) ((QUAD *)&SUFFIX(go_quad_euler),
					   euler_hex_digits,
					   G_N_ELEMENTS (euler_hex_digits),
					   256.0,
					   1);
	}

	return res;
}

/**
 * go_quad_end: (skip)
 **/
/**
 * go_quad_endl: (skip)
 **/
void
SUFFIX(go_quad_end) (void *state)
{
	SUFFIX(go_quad_depth)--;
	if (!state)
		return;

#ifdef i386
#ifdef USE_FPU_CONTROL
	_FPU_SETCW (*(fpu_control_t*)state);
#endif
#endif

	g_free (state);
}

const QUAD SUFFIX(go_quad_zero);
const QUAD SUFFIX(go_quad_one) = { 1, 0 };
const QUAD SUFFIX(go_quad_pi);
const QUAD SUFFIX(go_quad_e);
const QUAD SUFFIX(go_quad_ln2);
const QUAD SUFFIX(go_quad_sqrt2);
const QUAD SUFFIX(go_quad_euler);

/**
 * go_quad_init: (skip)
 **/
/**
 * go_quad_initl: (skip)
 **/
void
SUFFIX(go_quad_init) (QUAD *res, DOUBLE h)
{
	res->h = h;
	res->l = 0;
}

/**
 * go_quad_value: (skip)
 **/
/**
 * go_quad_valuel: (skip)
 **/
DOUBLE
SUFFIX(go_quad_value) (const QUAD *a)
{
	return a->h + a->l;
}

/**
 * go_quad_add: (skip)
 **/
/**
 * go_quad_addl: (skip)
 **/
void
SUFFIX(go_quad_add) (QUAD *res, const QUAD *a, const QUAD *b)
{
	DOUBLE r = a->h + b->h;
	DOUBLE s = SUFFIX(fabs) (a->h) > SUFFIX(fabs) (b->h)
		? a->h - r + b->h + b->l + a->l
		: b->h - r + a->h + a->l + b->l;
	res->h = r + s;
	res->l = r - res->h + s;

	g_return_if_fail (SUFFIX(go_quad_depth) > 0);
}

/**
 * go_quad_sub: (skip)
 **/
/**
 * go_quad_subl: (skip)
 **/
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

/**
 * go_quad_mul12: (skip)
 **/
/**
 * go_quad_mul12l: (skip)
 **/
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

/**
 * go_quad_mul: (skip)
 **/
/**
 * go_quad_mull: (skip)
 **/
void
SUFFIX(go_quad_mul) (QUAD *res, const QUAD *a, const QUAD *b)
{
	QUAD c;
	SUFFIX(go_quad_mul12) (&c, a->h, b->h);
	c.l = a->h * b->l + a->l * b->h + c.l;
	res->h = c.h + c.l;
	res->l = c.h - res->h + c.l;
}

/**
 * go_quad_div: (skip)
 **/
/**
 * go_quad_divl: (skip)
 **/
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

/**
 * go_quad_sqrt: (skip)
 **/
/**
 * go_quad_sqrtl: (skip)
 **/
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

/**
 * go_quad_dot_product: (skip)
 **/
/**
 * go_quad_dot_productl: (skip)
 **/
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

void
SUFFIX(go_quad_constant8) (QUAD *res, const guint8 *data, gsize n,
			   DOUBLE base, DOUBLE scale)
{
	QUAD qbase, q;

	*res = SUFFIX(go_quad_zero);
	SUFFIX(go_quad_init) (&qbase, base);

	while (n-- > 0) {
		SUFFIX(go_quad_init) (&q, data[n]);
		SUFFIX(go_quad_add) (res, res, &q);
		SUFFIX(go_quad_div) (res, res, &qbase);
	}

	SUFFIX(go_quad_init) (&q, scale);
	SUFFIX(go_quad_mul) (res, res, &q);
}
