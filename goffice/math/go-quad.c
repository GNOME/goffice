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

#define GO_QUAD_IMPL

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <math.h>

/* Normalize cpu id.  */
#if !defined(i386) && (defined(__i386__) || defined(__i386))
#define i386 1
#endif

#ifndef DOUBLE

#define DEFINE_COMMON

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
#define DOUBLE_EPSILON DBL_EPSILON

#ifdef GOFFICE_WITH_LONG_DOUBLE
#include "go-quad.c"
#undef DEFINE_COMMON
#undef DOUBLE
#undef SUFFIX
#undef DOUBLE_MANT_DIG
#undef DOUBLE_EPSILON
#define DOUBLE long double
#define SUFFIX(_n) _n ## l
#define DOUBLE_MANT_DIG LDBL_MANT_DIG
#define DOUBLE_EPSILON LDBL_EPSILON
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

#ifdef DEFINE_COMMON
/*
 * Store constants in a way doesn't depend on the layout of DOUBLE.  We use
 * ~400 bits of data in the tables below -- that's way more than needed even
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
#endif

/**
 * go_quad_start:
 *
 * Initializes #GOQuad arithmetic. Any use of #GOQuad must occur between calls
 * to go_quad_start() and go_quad_end().
 * Returns: (transfer full): a pointer to pass to go_quad_end() when done.
 **/
/**
 * go_quad_startl:
 *
 * Initializes #GOQuadl arithmetic. Any use of #GOQuadl must occur between calls
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
		res = g_memdup (&state, sizeof (state));

		newstate = (state & ~mask) | _FPU_DOUBLE;
		_FPU_SETCW (newstate);
#else
		/* Hope for the best.  */
#endif
	}
#endif

	if (first) {
		first = FALSE;
		SUFFIX(CST) = 1 + SUFFIX(ldexp) (1.0, (DOUBLE_MANT_DIG + 1) / 2);
		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_pi),
					   pi_hex_digits,
					   G_N_ELEMENTS (pi_hex_digits),
					   256.0,
					   256.0);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_2pi),
					   pi_hex_digits,
					   G_N_ELEMENTS (pi_hex_digits),
					   256.0,
					   512.0);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_e),
					   e_hex_digits,
					   G_N_ELEMENTS (e_hex_digits),
					   256.0,
					   256.0);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_ln2),
					   ln2_hex_digits,
					   G_N_ELEMENTS (ln2_hex_digits),
					   256.0,
					   1);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_sqrt2),
					   sqrt2_hex_digits,
					   G_N_ELEMENTS (sqrt2_hex_digits),
					   256.0,
					   256.0);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_euler),
					   euler_hex_digits,
					   G_N_ELEMENTS (euler_hex_digits),
					   256.0,
					   1);
	}

	return res;
}

/**
 * go_quad_end:
 * @state: state pointer from go_quad_start.
 *
 * This ends a section of quad precision arithmetic.
 **/
/**
 * go_quad_endl:
 * @state: state pointer from go_quad_startl.
 *
 * This ends a section of quad precision arithmetic.
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

const QUAD SUFFIX(go_quad_zero) = { 0, 0 };
const QUAD SUFFIX(go_quad_one) = { 1, 0 };
/*
 * The following are non-const so we can initialize them.  However,
 * from other compilation units there are const.  My reading of C99
 * Section 6.2.7 says that is allowed.
 */
QUAD SUFFIX(go_quad_pi);
QUAD SUFFIX(go_quad_2pi);
QUAD SUFFIX(go_quad_e);
QUAD SUFFIX(go_quad_ln2);
QUAD SUFFIX(go_quad_sqrt2);
QUAD SUFFIX(go_quad_euler);

/**
 * go_quad_init:
 * @res: (out): result location
 * @h: a double precision value
 *
 * This stores the value @h in @res.  As an exception, this may be called
 * outside go_quad_start and go_quad_end sections.
 **/
/**
 * go_quad_initl:
 * @res: (out): result location
 * @h: a double precision value
 *
 * This stores the value @h in @res.  As an exception, this may be called
 * outside go_quad_startl and go_quad_endl sections.
 **/
void
SUFFIX(go_quad_init) (QUAD *res, DOUBLE h)
{
	res->h = h;
	res->l = 0;
}

/**
 * go_quad_value:
 * @a: quad-precision value
 *
 * Returns: closest double precision value to @a.  As an exception,
 * this may be called outside go_quad_start and go_quad_end sections.
 **/
/**
 * go_quad_valuel:
 * @a: quad-precision value
 *
 * Returns: closest double precision value to @a.  As an exception,
 * this may be called outside go_quad_startl and go_quad_endl sections.
 **/
DOUBLE
SUFFIX(go_quad_value) (const QUAD *a)
{
	return a->h + a->l;
}

/**
 * go_quad_add:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function adds @a and @b, storing the result in @res.
 **/
/**
 * go_quad_addl:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function adds @a and @b, storing the result in @res.
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
 * go_quad_sub:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function subtracts @a and @b, storing the result in @res.
 **/
/**
 * go_quad_subl:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function subtracts @a and @b, storing the result in @res.
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


#define SPLIT1(x,h,t) do {				\
  DOUBLE p = x * SUFFIX(CST);				\
  if (!SUFFIX(go_finite) (p) && SUFFIX(go_finite)(x)) {	\
     x *= DOUBLE_EPSILON;				\
     p = x * SUFFIX(CST);				\
     h = x - p + p;					\
     t = x - h;						\
     h *= (1 / DOUBLE_EPSILON);				\
     t *= (1 / DOUBLE_EPSILON);				\
  } else {						\
     h = x - p + p;					\
     t = x - h;						\
  }							\
} while (0)

/**
 * go_quad_mul12:
 * @res: (out): result location
 * @x: double precision value
 * @y: double precision value
 *
 * This function multiplies @x and @y, storing the result in @res with full
 * quad precision.
 **/
/**
 * go_quad_mul12l:
 * @res: (out): result location
 * @x: double precision value
 * @y: double precision value
 *
 * This function multiplies @x and @y, storing the result in @res with full
 * quad precision.
 **/
void
SUFFIX(go_quad_mul12) (QUAD *res, DOUBLE x, DOUBLE y)
{
	DOUBLE hx, tx, hy, ty, p, q;

	SPLIT1 (x, hx, tx);
	SPLIT1 (y, hy, ty);

	p = hx * hy;
	q = hx * ty + tx * hy;
	res->h = p + q;
	res->l = p - res->h + q + tx * ty;
}

#undef SPLIT1


/**
 * go_quad_mul:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function multiplies @a and @b, storing the result in @res.
 **/
/**
 * go_quad_mull:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function multiplies @a and @b, storing the result in @res.
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
 * go_quad_div:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function divides @a and @b, storing the result in @res.
 **/
/**
 * go_quad_divl:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function divides @a and @b, storing the result in @res.
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
 * go_quad_sqrt:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function takes the square root of @a, storing the result in @res.
 **/
/**
 * go_quad_sqrtl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function takes the square root of @a, storing the result in @res.
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
 * go_quad_floor:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function takes the floor of @a, storing the result in @res.
 **/
/**
 * go_quad_floorl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function takes the floor of @a, storing the result in @res.
 **/
void
SUFFIX(go_quad_floor) (QUAD *res, const QUAD *a)
{
	QUAD qh, ql, q, r;

	SUFFIX(go_quad_init) (&qh, SUFFIX(floor)(a->h));
	SUFFIX(go_quad_init) (&ql, SUFFIX(floor)(a->l));
	SUFFIX(go_quad_add) (&q, &qh, &ql);

	/* Due to dual floors, we might be off by one.  */
	SUFFIX(go_quad_sub) (&r, a, &q);
	if (SUFFIX(go_quad_value) (&r) < 0)
		SUFFIX(go_quad_sub) (res, &q, &SUFFIX(go_quad_one));
	else {
		SUFFIX(go_quad_sub) (&r, &r, &SUFFIX(go_quad_one));
		if (SUFFIX(go_quad_value) (&r) < 0)
			*res = q;
		else
			SUFFIX(go_quad_add) (res, &q, &SUFFIX(go_quad_one));
	}
}

/**
 * go_quad_dot_product:
 * @res: (out): result location
 * @a: (array length=n): vector of quad-precision values
 * @b: (array length=n): vector of quad-precision values
 * @n: length of vectors.
 **/
/**
 * go_quad_dot_productl:
 * @res: (out): result location
 * @a: (array length=n): vector of quad-precision values
 * @b: (array length=n): vector of quad-precision values
 * @n: length of vectors.
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

/**
 * go_quad_constant8:
 * @res: (out): result location
 * @data: (array length=n): vector of digits
 * @base: base of vector's elements
 * @n: length of digit vector.
 * @scale: scaling value after interpreting digits
 *
 * This function interprets a vector of digits in a given base as a
 * quad-precision value.  It is mostly meant for internal use.
 **/
/**
 * go_quad_constant8l:
 * @res: (out): result location
 * @data: (array length=n): vector of digits
 * @base: base of vector's elements
 * @n: length of digit vector.
 * @scale: scaling value after interpreting digits
 *
 * This function interprets a vector of digits in a given base as a
 * quad-precision value.  It is mostly meant for internal use.
 **/
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

static void
SUFFIX(rescale2) (QUAD *x, DOUBLE *e)
{
	int xe;

	(void)SUFFIX(frexp) (SUFFIX(go_quad_value) (x), &xe);
	if (xe != 0) {
		QUAD qs;
		SUFFIX(go_quad_init) (&qs, SUFFIX(ldexp) (1.0, -xe));
		SUFFIX(go_quad_mul) (x, x, &qs);
		*e += xe;
	}
}


static void
SUFFIX(go_quad_pow_int) (QUAD *res, DOUBLE *exp2, const QUAD *x, const QUAD *y)
{
	QUAD xn;
	DOUBLE dy;
	DOUBLE xe = 0;

	xn = *x;
	*exp2 = 0;

	dy = SUFFIX(go_quad_value) (y);
	g_return_if_fail (dy >= 0);

	*res = SUFFIX(go_quad_one);
	SUFFIX(rescale2) (&xn, &xe);

	while (dy > 0) {
		if (SUFFIX(fmod) (dy, 2) > 0) {
			SUFFIX(go_quad_mul) (res, res, &xn);
			*exp2 += xe;
			SUFFIX(rescale2) (res, exp2);
			dy--;
			if (dy == 0)
				break;
		}
		dy /= 2;
		SUFFIX(go_quad_mul) (&xn, &xn, &xn);
		xe *= 2;
		SUFFIX(rescale2) (&xn, &xe);
	}
}

static void
SUFFIX(go_quad_sqrt1pm1) (QUAD *res, const QUAD *a)
{
	QUAD x0, x1, d;

	SUFFIX(go_quad_add) (&x0, a, &SUFFIX(go_quad_one));
	SUFFIX(go_quad_sqrt) (&x0, &x0);
	SUFFIX(go_quad_sub) (&x0, &x0, &SUFFIX(go_quad_one));

	/* Newton step.  */
	SUFFIX(go_quad_mul) (&x1, &x0, &x0);
	SUFFIX(go_quad_add) (&x1, &x1, a);
	SUFFIX(go_quad_add) (&d, &x0, &SUFFIX(go_quad_one));
	SUFFIX(go_quad_add) (&d, &d, &d);
	SUFFIX(go_quad_div) (&x1, &x1, &d);

	*res = x1;
}

/*
 * Compute pow(x,y) assuming y is in [0;1[.  If r1m is true,
 * compute 1-pow(x,y) instead.
 */

static void
SUFFIX(go_quad_pow_frac) (QUAD *res, const QUAD *x, const QUAD *y,
			  gboolean r1m)
{
	QUAD qx, qr, qy = *y;
	DOUBLE dy;
	gboolean x1m;

	g_return_if_fail (SUFFIX(go_quad_value) (y) >= 0);
	g_return_if_fail (SUFFIX(go_quad_value) (y) <= 1); /* =1 might happen */

	/*
	 * "1m" mode refers to keeping 1-v instead of just v.
	 */
	x1m = SUFFIX(fabs) (SUFFIX(go_quad_value) (x)) >= 0.5;
	if (x1m) {
		SUFFIX(go_quad_sub) (&qx, x, &SUFFIX(go_quad_one));
	} else {
		qx = *x;
	}

	qr = r1m ? SUFFIX(go_quad_zero) : SUFFIX(go_quad_one);

	while ((dy = SUFFIX(go_quad_value) (&qy)) > 0) {
		SUFFIX(go_quad_add) (&qy, &qy, &qy);
		if (x1m)
			SUFFIX(go_quad_sqrt1pm1) (&qx, &qx);
		else {
			SUFFIX(go_quad_sqrt) (&qx, &qx);
			if (SUFFIX(go_quad_value) (&qx) >= 0.5) {
				x1m = TRUE;
				SUFFIX(go_quad_sub) (&qx, &qx, &SUFFIX(go_quad_one));
			}
		}
		if (dy >= 0.5) {
			QUAD qp;
			SUFFIX(go_quad_sub) (&qy, &qy, &SUFFIX(go_quad_one));
			SUFFIX(go_quad_mul) (&qp, &qx, &qr);
			if (x1m && r1m) {
				SUFFIX(go_quad_add) (&qr, &qr, &qp);
				SUFFIX(go_quad_add) (&qr, &qr, &qx);
			} else if (x1m) {
				SUFFIX(go_quad_add) (&qr, &qr, &qp);
			} else if (r1m) {
				QUAD qy;
				SUFFIX(go_quad_sub) (&qy, &qx, &SUFFIX(go_quad_one));
				SUFFIX(go_quad_add) (&qr, &qp, &qy);
			} else {
				qr = qp;
			}
		}
	}

	*res = qr;
}

/*
 * This computes pow(x,y) in the following way:
 *
 * 1. y is considered the sum of a number of one-bit values.
 * 2. For each y bit in the integer part of y, the corresponding x^y
 *    is computed by repeated squaring.
 * 3. For each y bit in the fractional part of y, the corresponding x^y
 *    is computed by repeating square rooting.
 *
 * Except that it's not quite as simple.  Repeating square rooting of x
 * will bring the value closer and closer to 1.  That's no good, so we
 * sometimes keep those values a 1-v instead of v.  A square root in
 * that representation is sqrt1pm1(v)=sqrt(1+v)-1.
 *
 * Finally we don't simply return one value.  If the optional exp2 location
 * is non-null, it is treated as a place to put a base 2 exponent with which
 * to scale the returned result.  This isn't much different from the exponent
 * inside the floating point number, except that the range is *huge*.  This
 * function will happily compute exp(1e6):
 *   exp(1e+06) = 0.5143737638002868*2^1442696  [1.028747527600574*2^1442695]
 * The bracked result is a reference value from Mathematica.
 */

/**
 * go_quad_pow:
 * @res: (out): result location
 * @exp2: (out): (allow-none): power-of-2 result scaling location
 * @x: quad-precision value
 * @y: quad-precision value
 *
 * This function computes @x to the power of @y, storing the result in @res.
 * If the optional @exp2 is supplied, it is used to return a power of 2 by
 * which the result should be scaled.  This is useful to represent results
 * much, much bigger than double precision can handle.
 **/
/**
 * go_quad_powl:
 * @res: (out): result location
 * @exp2: (out): (allow-none): power-of-2 result scaling location
 * @x: quad-precision value
 * @y: quad-precision value
 *
 * This function computes @x to the power of @y, storing the result in @res.
 * If the optional @exp2 is supplied, it is used to return a power of 2 by
 * which the result should be scaled.  This is useful to represent results
 * much, much bigger than double precision can handle.
 **/
void
SUFFIX(go_quad_pow) (QUAD *res, DOUBLE *exp2,
		     const QUAD *x, const QUAD *y)
{
	DOUBLE dy, exp2ew;
	QUAD qw, qf, qew, qef, qxm1;

	dy = SUFFIX(go_quad_value) (y);

	SUFFIX(go_quad_sub) (&qxm1, x, &SUFFIX(go_quad_one));
	if (SUFFIX(go_quad_value) (&qxm1) == 0 || dy == 0) {
		*res = SUFFIX(go_quad_one);
		if (exp2) *exp2 = 0;
		return;
	}

	SUFFIX(go_quad_floor) (&qw, y);
	SUFFIX(go_quad_sub) (&qf, y, &qw);
	if (SUFFIX(go_quad_value) (&qxm1) == 0 && dy > 0) {
		gboolean wodd =
			(SUFFIX(fmod)(SUFFIX(fabs)(qw.h),2) +
			 SUFFIX(fmod)(SUFFIX(fabs)(qw.l),2)) == 1;
		if (SUFFIX(go_quad_value) (&qf) == 0 && wodd) {
			/* 0 ^ (odd positive integer) */
			*res = *x;
		} else {
			/* 0 ^ y, y positive, but not odd integer */
			*res = SUFFIX(go_quad_zero);
		}
		if (exp2) *exp2 = 0;
		return;
	}

	/* Lots of infinity/nan cases ignored */

	if (dy < 0) {
		QUAD my;
		SUFFIX(go_quad_sub) (&my, &SUFFIX(go_quad_zero), y);
		SUFFIX(go_quad_pow) (res, exp2, x, &my);
		SUFFIX(go_quad_div) (res, &SUFFIX(go_quad_one), res);
		if (exp2) *exp2 = 0 - *exp2;
		return;
	}

	SUFFIX(go_quad_pow_int) (&qew, &exp2ew, x, &qw);
	SUFFIX(go_quad_pow_frac) (&qef, x, &qf, FALSE);

	SUFFIX(go_quad_mul) (res, &qew, &qef);
	if (exp2)
		*exp2 = exp2ew;
	else {
		QUAD qs;
		int e = CLAMP (exp2ew, G_MININT, G_MAXINT);
		SUFFIX(go_quad_init) (&qs, SUFFIX(ldexp)(1.0, e));
		SUFFIX(go_quad_mul) (res, res, &qs);
	}
}


/**
 * go_quad_exp:
 * @res: (out): result location
 * @exp2: (out): (allow-none): power-of-2 result scaling location
 * @a: quad-precision value
 *
 * This function computes the exponential function at @a, storing the result
 * in @res.  If the optional @exp2 is supplied, it is used to return a
 * power of 2 by which the result should be scaled.  This is useful to
 * represent results much, much bigger than double precision can handle.
 **/
/**
 * go_quad_expl:
 * @res: (out): result location
 * @exp2: (out): (allow-none): power-of-2 result scaling location
 * @a: quad-precision value
 *
 * This function computes the exponential function at @a, storing the result
 * in @res.  If the optional @exp2 is supplied, it is used to return a
 * power of 2 by which the result should be scaled.  This is useful to
 * represent results much, much bigger than double precision can handle.
 **/
void
SUFFIX(go_quad_exp) (QUAD *res, DOUBLE *exp2, const QUAD *a)
{
	SUFFIX(go_quad_pow) (res, exp2, &SUFFIX(go_quad_e), a);
}

/**
 * go_quad_expm1:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the exponential function at @a with 1 subtracted,
 * storing the difference in @res.
 **/
/**
 * go_quad_expm1l:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the exponential function at @a with 1 subtracted,
 * storing the difference in @res.
 **/
void
SUFFIX(go_quad_expm1) (QUAD *res, const QUAD *a)
{
	DOUBLE da = SUFFIX(go_quad_value) (a);

	if (!SUFFIX(go_finite) (da))
		*res = *a;
	else if (SUFFIX (fabs) (da) > 0.5) {
		SUFFIX(go_quad_exp) (res, NULL, a);
		SUFFIX(go_quad_sub) (res, res, &SUFFIX(go_quad_one));
	} else if (da >= 0) {
		SUFFIX(go_quad_pow_frac) (res, &SUFFIX(go_quad_e), a, TRUE);
	} else {
		QUAD ma, z, zp1;
		SUFFIX(go_quad_sub) (&ma, &SUFFIX(go_quad_zero), a);
		SUFFIX(go_quad_pow_frac) (&z, &SUFFIX(go_quad_e), &ma, TRUE);
		SUFFIX(go_quad_add) (&zp1, &z, &SUFFIX(go_quad_one));
		SUFFIX(go_quad_div) (res, &z, &zp1);
	}
}

/**
 * go_quad_log:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the natural logarithm at @a, storing the result
 * in @res.
 **/
/**
 * go_quad_logl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the natural logarithm at @a, storing the result
 * in @res.
 **/
void
SUFFIX(go_quad_log) (QUAD *res, const QUAD *a)
{
	DOUBLE da = SUFFIX(go_quad_value) (a);

	if (da == 0)
		SUFFIX(go_quad_init) (res, SUFFIX(go_ninf));
	else if (da < 0)
		SUFFIX(go_quad_init) (res, SUFFIX(go_nan));
	else if (!SUFFIX(go_finite) (da))
		*res = *a;
	else {
		QUAD xi, yi, dx;
		SUFFIX(go_quad_init) (&xi, SUFFIX(log) (da));

		/* Newton step. */
		SUFFIX(go_quad_exp) (&yi, NULL, &xi);
		SUFFIX(go_quad_sub) (&dx, a, &yi);
		SUFFIX(go_quad_div) (&dx, &dx, &yi);
		SUFFIX(go_quad_add) (&xi, &xi, &dx);

		*res = xi;
	}
}

/**
 * go_quad_hypot:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function computes the square root of @a^2 plugs @b^2, storing the
 * result in @res.
 **/
/**
 * go_quad_hypotl:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function computes the square root of @a^2 plugs @b^2, storing the
 * result in @res.
 **/
void
SUFFIX(go_quad_hypot) (QUAD *res, const QUAD *a, const QUAD *b)
{
	int e;
	QUAD qa2, qb2, qn;

	if (a->h == 0) {
		res->h = SUFFIX(fabs)(b->h);
		res->l = SUFFIX(fabs)(b->l);
		return;
	}
	if (b->h == 0) {
		res->h = SUFFIX(fabs)(a->h);
		res->l = SUFFIX(fabs)(a->l);
		return;
	}

	/* Scale by power of 2 to protect against over- and underflow */
	(void)SUFFIX(frexp) (MAX (SUFFIX(fabs) (a->h), SUFFIX(fabs) (b->h)), &e);

	qa2.h = SUFFIX(ldexp) (a->h, -e);
	qa2.l = SUFFIX(ldexp) (a->l, -e);
	SUFFIX(go_quad_mul) (&qa2, &qa2, &qa2);

	qb2.h = SUFFIX(ldexp) (b->h, -e);
	qb2.l = SUFFIX(ldexp) (b->l, -e);
	SUFFIX(go_quad_mul) (&qb2, &qb2, &qb2);

	SUFFIX(go_quad_add) (&qn, &qa2, &qb2);
	SUFFIX(go_quad_sqrt) (&qn, &qn);
	res->h = SUFFIX(ldexp) (qn.h, e);
	res->l = SUFFIX(ldexp) (qn.l, e);
}

/* sqrt(1-a*a) helper */
static void
SUFFIX(go_quad_ihypot) (QUAD *res, const QUAD *a)
{
	QUAD qp;

	SUFFIX(go_quad_mul) (&qp, a, a);
	SUFFIX(go_quad_sub) (&qp, &SUFFIX(go_quad_one), &qp);
	SUFFIX(go_quad_sqrt) (res, &qp);
}


#ifdef DEFINE_COMMON
typedef enum {
	AGM_ARCSIN,
	AGM_ARCCOS,
	AGM_ARCTAN
} AGM_Method;
#endif

static void
SUFFIX(go_quad_agm_internal) (QUAD *res, AGM_Method method, const QUAD *x)
{
	QUAD g, gp, dk[20], dpk[20], qr, qrp, qnum;
	int n, k;
	gboolean converged = FALSE;

	/*
	 * This follows "An Algorithm for Computing Logarithms
	 * and Arctangents" by B. C. Carlson in *Mathematics of
	 * Computation*, Volume 26, Number 118, April 1972.
	 *
	 * If need be we can do log, arctanh, arcsinh, and
	 * arccosh we the same code.
	 */

	qrp = SUFFIX(go_quad_zero);

	switch (method) {
	case AGM_ARCSIN:
		g_return_if_fail (SUFFIX(fabs) (x->h) <= 1);
		SUFFIX(go_quad_ihypot) (&dpk[0], x);
		gp = SUFFIX(go_quad_one);
		qnum = *x;
		break;
	case AGM_ARCCOS:
		g_return_if_fail (SUFFIX(fabs) (x->h) <= 1);
		dpk[0] = *x;
		gp = SUFFIX(go_quad_one);
		SUFFIX(go_quad_ihypot) (&qnum, x);
		break;
	case AGM_ARCTAN:
		g_return_if_fail (SUFFIX(fabs) (x->h) <= 1);
		dpk[0] = SUFFIX(go_quad_one);
		SUFFIX(go_quad_hypot) (&gp, x, &SUFFIX(go_quad_one));
		qnum = *x;
		break;
	default:
		g_assert_not_reached ();
	}

	for (n = 1; n < (int)G_N_ELEMENTS(dk); n++) {
		SUFFIX(go_quad_add) (&dk[0], &dpk[0], &gp);
		dk[0].h *= 0.5; dk[0].l *= 0.5;

		SUFFIX(go_quad_mul) (&g, &dk[0], &gp);
		SUFFIX(go_quad_sqrt) (&g, &g);

		for (k = 1; k <= n; k++) {
			QUAD f;

			SUFFIX(go_quad_init) (&f, SUFFIX(ldexp) (1, -(2 * k)));
			SUFFIX(go_quad_mul) (&dk[k], &f, &dpk[k-1]);
			SUFFIX(go_quad_sub) (&dk[k], &dk[k-1], &dk[k]);
			SUFFIX(go_quad_sub) (&f, &SUFFIX(go_quad_one), &f);
			SUFFIX(go_quad_div) (&dk[k], &dk[k], &f);
		}

		SUFFIX(go_quad_div) (&qr, &qnum, &dk[n]);
		SUFFIX(go_quad_sub) (&qrp, &qrp, &qr);
		if (SUFFIX(fabs)(qrp.h) <= SUFFIX(ldexp) (SUFFIX(fabs)(qr.h), -2 * (DOUBLE_MANT_DIG - 1))) {
			converged = TRUE;
			break;
		}

		qrp = qr;
		gp = g;
		for (k = 0; k <= n; k++) dpk[k] = dk[k];
	}

	if (!converged)
		g_warning ("go_quad_agm_internal(%.20g) failed to converge\n",
			   (double)SUFFIX(go_quad_value) (x));

	*res = qr;
}

static gboolean
SUFFIX(go_quad_atan2_special) (const QUAD *y, const QUAD *x, DOUBLE *f)
{
	DOUBLE dy = SUFFIX(go_quad_value) (y);
	DOUBLE dx = SUFFIX(go_quad_value) (x);

	if (dy == 0) {
		/* This assumes all zeros are +0. */
		*f = (dx >= 0 ? 0 : +1);
		return TRUE;
	}

	if (dx == 0) {
		*f = (dy >= 0 ? 0.5 : -0.5);
		return TRUE;
	}

	if (SUFFIX(fabs) (SUFFIX(fabs)(dx) - SUFFIX(fabs)(dy)) < 1e-10) {
		QUAD d;
		SUFFIX(go_quad_sub) (&d, x, y);
		if (d.h == 0) {
			*f = (dy >= 0 ? 0.25 : -0.75);
			return TRUE;
		}
		SUFFIX(go_quad_add) (&d, x, y);
		if (d.h == 0) {
			*f = (dy >= 0 ? +0.75 : -0.25);
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * go_quad_atan2:
 * @res: (out): result location
 * @y: quad-precision value
 * @x: quad-precision value
 *
 * This function computes polar angle coordinate of the point (@x,@y), storing
 * the result in @res.
 **/
/**
 * go_quad_atan2l:
 * @res: (out): result location
 * @y: quad-precision value
 * @x: quad-precision value
 *
 * This function computes polar angle coordinate of the point (@x,@y), storing
 * the result in @res.
 **/
void
SUFFIX(go_quad_atan2) (QUAD *res, const QUAD *y, const QUAD *x)
{
	DOUBLE f;
	DOUBLE dy = SUFFIX(go_quad_value) (y);
	DOUBLE dx = SUFFIX(go_quad_value) (x);
	QUAD qr;

	if (SUFFIX(go_quad_atan2_special) (y, x, &f)) {
		res->h = f * SUFFIX(go_quad_pi).h;
		res->l = f * SUFFIX(go_quad_pi).l;
		return;
	}

	if (SUFFIX(fabs) (dx) >= SUFFIX(fabs) (dy)) {
		SUFFIX(go_quad_div) (&qr, y, x);
		SUFFIX(go_quad_agm_internal) (res, AGM_ARCTAN, &qr);
	} else {
		DOUBLE f;
		QUAD qa;

		SUFFIX(go_quad_div) (&qr, x, y);
		SUFFIX(go_quad_agm_internal) (res, AGM_ARCTAN, &qr);

		f = (qr.h >= 0) ? 0.5 : -0.5;
		qa.h = f * SUFFIX(go_quad_pi).h;
		qa.l = f * SUFFIX(go_quad_pi).l;
		SUFFIX(go_quad_sub) (res, &qa, res);
	}

	if (dx < 0) {
		/* Correct for quadrants 2 and 3. */
		if (dy > 0)
			SUFFIX(go_quad_add) (res, res, &SUFFIX(go_quad_pi));
		else
			SUFFIX(go_quad_sub) (res, res, &SUFFIX(go_quad_pi));
	}
}

/**
 * go_quad_atan2pi:
 * @res: (out): result location
 * @y: quad-precision value
 * @x: quad-precision value
 *
 * This function computes polar angle coordinate of the point (@x,@y) divided
 * by pi, storing the result in @res.
 **/
/**
 * go_quad_atan2pil:
 * @res: (out): result location
 * @y: quad-precision value
 * @x: quad-precision value
 *
 * This function computes polar angle coordinate of the point (@x,@y) divided
 * by pi, storing the result in @res.
 **/
void
SUFFIX(go_quad_atan2pi) (QUAD *res, const QUAD *y, const QUAD *x)
{
	DOUBLE f;

	if (SUFFIX(go_quad_atan2_special) (y, x, &f)) {
		SUFFIX(go_quad_init) (res, f);
		return;
	}

	/* Fallback.  */
	SUFFIX(go_quad_atan2) (res, y, x);
	SUFFIX(go_quad_div) (res, res, &SUFFIX(go_quad_pi));
}

static gboolean
SUFFIX(reduce_pi_half) (QUAD *res, const QUAD *a, int *pk)
{
	static QUAD pi_half;
	QUAD qa, qk, qh, qb;
	DOUBLE k;
	unsigned ui;
	static const DOUBLE pi_half_parts[] = {
		+0x1.921fb54442d18p+0,
		+0x1.1a62633145c04p-54,
		+0x1.707344a40938p-105,
		+0x1.114cf98e80414p-156,
		+0x1.bea63b139b224p-207,
		+0x1.14a08798e3404p-259,
		+0x1.bbdf2a33679a4p-312,
		+0x1.a431b302b0a6cp-363,
		+0x1.f25f14374fe1p-415,
		+0x1.ab6b6a8e122fp-466
	};

	if (!SUFFIX(go_finite) (a->h))
		return TRUE;

	if (SUFFIX(fabs) (a->h) > SUFFIX(ldexp) (1.0, DOUBLE_MANT_DIG)) {
		g_warning ("Reduced accuracy for very large trigonometric arguments");
		return TRUE;
	}

	if (pi_half.h == 0) {
		pi_half.h = SUFFIX(go_quad_pi).h * 0.5;
		pi_half.l = SUFFIX(go_quad_pi).l * 0.5;
	}

	SUFFIX(go_quad_div) (&qk, a, &pi_half);
	qh.h = 0.5; qh.l = 0;
	SUFFIX(go_quad_add) (&qk, &qk, &qh);
	SUFFIX(go_quad_floor) (&qk, &qk);
	k = SUFFIX(go_quad_value) (&qk);
	*pk = (int)(SUFFIX(fmod) (k, 4));

	qa = *a;
	for (ui = 0; ui < G_N_ELEMENTS(pi_half_parts); ui++) {
		SUFFIX(go_quad_mul12) (&qb, pi_half_parts[ui], k);
		SUFFIX(go_quad_sub) (&qa, &qa, &qb);
	}

	*res = qa;

	return FALSE;
}

static void
SUFFIX(reduce_half) (QUAD *res, const QUAD *a, int *pk)
{
	static const QUAD half = { 0.5, 0 };
	int k = 0;
	QUAD qxr = *a;

	if (a->h < 0) {
		QUAD aa;
		aa.h = -a->h; aa.l = -a->l;
		SUFFIX(reduce_half) (&qxr, &aa, &k);
		qxr.h = -qxr.h; qxr.l = -qxr.l;
		k = 4 - k;
		if (qxr.h <= -0.25 && qxr.l == 0) {
			SUFFIX(go_quad_add) (&qxr, &qxr, &half);
			k += 3;
		}
	} else {
		QUAD qdx;

		/* Remove integer multiples of 2.  */
		SUFFIX(go_quad_init) (&qdx, qxr.h - SUFFIX(fmod) (qxr.h, 2));
		SUFFIX(go_quad_sub) (&qxr, &qxr, &qdx);

		/* Again, just in case it was really big.  */
		SUFFIX(go_quad_init) (&qdx, qxr.h - SUFFIX(fmod) (qxr.h, 2));
		SUFFIX(go_quad_sub) (&qxr, &qxr, &qdx);

		if (qxr.h >= 1) {
			SUFFIX(go_quad_sub) (&qxr, &qxr, &SUFFIX(go_quad_one));
			k += 2;
		}
		if (qxr.h >= 0.5) {
			SUFFIX(go_quad_sub) (&qxr, &qxr, &half);
			k++;
		}
		if (qxr.h > 0.25) {
			SUFFIX(go_quad_sub) (&qxr, &qxr, &half);
			k++;
		}
	}

	*pk = (k & 3);
	*res = qxr;
}

static void
SUFFIX(do_sin) (QUAD *res, const QUAD *a, int k)
{
	QUAD qr;

	if (k & 1) {
		QUAD qn, qd, qq, aa;

		aa.h = SUFFIX(fabs)(a->h);
		aa.l = SUFFIX(fabs)(a->l);
		SUFFIX(go_quad_init) (&qr, SUFFIX(cos) (aa.h));

		/* Newton step */
		SUFFIX(go_quad_acos) (&qn, &qr);
		SUFFIX(go_quad_sub) (&qn, &qn, &aa);
		SUFFIX(go_quad_ihypot) (&qd, &qr);
		SUFFIX(go_quad_mul) (&qq, &qn, &qd);
		SUFFIX(go_quad_add) (&qr, &qr, &qq);
	} else {
		QUAD qn, qd, qq;
		SUFFIX(go_quad_init) (&qr, SUFFIX(sin) (a->h));

		/* Newton step */
		SUFFIX(go_quad_asin) (&qn, &qr);
		SUFFIX(go_quad_sub) (&qn, &qn, a);
		SUFFIX(go_quad_ihypot) (&qd, &qr);
		SUFFIX(go_quad_mul) (&qq, &qn, &qd);
		SUFFIX(go_quad_sub) (&qr, &qr, &qq);
	}

	if (k & 2) {
		qr.h = 0 - qr.h;
		qr.l = 0 - qr.l;
	}

	*res = qr;
}

static void
SUFFIX(do_sinpi) (QUAD *res, const QUAD *a, int k)
{
	QUAD qr;

	if (a->h == 0)
		SUFFIX(go_quad_init) (&qr, k & 1);
	else if (a->h == 0.25 && a->l == 0)
		SUFFIX(go_quad_div) (&qr,
				     &SUFFIX(go_quad_one),
				     &SUFFIX(go_quad_sqrt2));
	else {
		QUAD api;
		SUFFIX(go_quad_mul) (&api, a, &SUFFIX(go_quad_pi));
		SUFFIX(do_sin) (&qr, &api, k & 1);
	}

	if (k & 2) {
		qr.h = 0 - qr.h;
		qr.l = 0 - qr.l;
	}

	*res = qr;
}

/**
 * go_quad_sin:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the sine of @a, storing the result in @res.
 **/
/**
 * go_quad_sinl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the sine of @a, storing the result in @res.
 **/
void
SUFFIX(go_quad_sin) (QUAD *res, const QUAD *a)
{
	int k;
	QUAD a0;

	if (SUFFIX(reduce_pi_half) (&a0, a, &k))
		SUFFIX(go_quad_init) (res, SUFFIX(sin) (a->h));
	else
		SUFFIX(do_sin) (res, &a0, k);
}

/**
 * go_quad_sinpi:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the sine of @a times pi, storing the result in @res.
 * This is more accurate than actually doing the multiplication.
 **/
/**
 * go_quad_sinpil:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the sine of @a times pi, storing the result in @res.
 * This is more accurate than actually doing the multiplication.
 **/
void
SUFFIX(go_quad_sinpi) (QUAD *res, const QUAD *a)
{
	int k;
	QUAD a0;

	SUFFIX(reduce_half) (&a0, a, &k);
	SUFFIX(do_sinpi) (res, &a0, k);
}

/**
 * go_quad_asin:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the arc sine of @a, storing the result in @res.
 **/
/**
 * go_quad_asinl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the arc sine of @a, storing the result in @res.
 **/
void
SUFFIX(go_quad_asin) (QUAD *res, const QUAD *a)
{
	QUAD aa, aam1;

	aa.h = SUFFIX(fabs) (a->h);
	aa.l = SUFFIX(fabs) (a->l);
	SUFFIX(go_quad_sub) (&aam1, &aa, &SUFFIX(go_quad_one));
	if (aam1.h > 0) {
		SUFFIX(go_quad_init) (res, SUFFIX(go_nan));
		return;
	}

	SUFFIX(go_quad_agm_internal) (res, AGM_ARCSIN, a);
}

/**
 * go_quad_cos:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the cosine of @a, storing the result in @res.
 **/
/**
 * go_quad_cosl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the cosine of @a, storing the result in @res.
 **/
void
SUFFIX(go_quad_cos) (QUAD *res, const QUAD *a)
{
	int k;
	QUAD a0;

	if (SUFFIX(reduce_pi_half) (&a0, a, &k))
		SUFFIX(go_quad_init) (res, SUFFIX(cos) (a->h));
	else
		SUFFIX(do_sin) (res, &a0, k + 1);
}

/**
 * go_quad_cospi:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the cosine of @a times pi, storing the result in @res.
 * This is more accurate than actually doing the multiplication.
 **/
/**
 * go_quad_cospil:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the cosine of @a times pi, storing the result in @res.
 * This is more accurate than actually doing the multiplication.
 **/
void
SUFFIX(go_quad_cospi) (QUAD *res, const QUAD *a)
{
	int k;
	QUAD a0;

	SUFFIX(reduce_half) (&a0, a, &k);
	SUFFIX(do_sinpi) (res, &a0, k + 1);
}

/**
 * go_quad_acos:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the arc cosine of @a, storing the result in @res.
 **/
/**
 * go_quad_acosl:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the arc cosine of @a, storing the result in @res.
 **/
void
SUFFIX(go_quad_acos) (QUAD *res, const QUAD *a)
{
	QUAD aa, aam1;

	aa.h = SUFFIX(fabs) (a->h);
	aa.l = SUFFIX(fabs) (a->l);
	SUFFIX(go_quad_sub) (&aam1, &aa, &SUFFIX(go_quad_one));
	if (aam1.h > 0) {
		SUFFIX(go_quad_init) (res, SUFFIX(go_nan));
		return;
	}

	SUFFIX(go_quad_agm_internal) (res, AGM_ARCCOS, &aa);
	if (a->h < 0)
		SUFFIX(go_quad_sub) (res, &SUFFIX(go_quad_pi), res);
}
