/*
 * go-quad.c:  Extended precision routines.
 *
 * Authors
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
 * See also "On various ways to split a floating-point number" by
 * Claude-Pierre Jeannerod, Jean-Michel Muller, and Paul Zimmermann.
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

#ifdef HAVE_FPU_CONTROL_H
#include <fpu_control.h>
#define USE_FPU_CONTROL
#endif

// We need multiple versions of this code.  We're going to include ourself
// with different settings of various macros.  gdb will hate us.
#include <goffice/goffice-multipass.h>
#ifndef SKIP_THIS_PASS

#define DOUBLE_IS_double (INCLUDE_PASS == INCLUDE_PASS_DOUBLE)
#define QUAD SUFFIX(GOQuad)
#define HALF (DOUBLE)0.5

#if defined(i386) && DOUBLE_IS_double
#define MIGHT_NEED_FPU_SETUP
#else
#undef MIGHT_NEED_FPU_SETUP
#endif

gboolean
SUFFIX(go_quad_functional) (void)
{
#ifdef MIGHT_NEED_FPU_SETUP
  #ifdef USE_FPU_CONTROL
	return TRUE;
  #else
	return FALSE;
  #endif
#else
	return TRUE;
#endif
}

#ifdef MIGHT_NEED_FPU_SETUP
static guint SUFFIX(go_quad_depth) = 0;
#endif

static DOUBLE SUFFIX(CST);

/*
 * Store constants in a way doesn't depend on the layout of DOUBLE.  We use
 * ~400 bits of data in the tables below -- that's way more than needed even
 * for sparc's long double.
 */

static const guint8 SUFFIX(pi_digits)[] = {
#if DOUBLE_RADIX == 2
	0x03, 0x24, 0x3f, 0x6a, 0x88, 0x85, 0xa3, 0x08,
	0xd3, 0x13, 0x19, 0x8a, 0x2e, 0x03, 0x70, 0x73,
	0x44, 0xa4, 0x09, 0x38, 0x22, 0x29, 0x9f, 0x31,
	0xd0, 0x08, 0x2e, 0xfa, 0x98, 0xec, 0x4e, 0x6c,
	0x89, 0x45, 0x28, 0x21, 0xe6, 0x38, 0xd0, 0x13,
	0x77, 0xbe, 0x54, 0x66, 0xcf, 0x34, 0xe9, 0x0c,
	0x6c, 0xc0, 0xac
#else
	 3, 14, 15, 92, 65, 35, 89, 79, 32, 38,
	46, 26, 43, 38, 32, 79, 50, 28, 84, 19,
	71, 69, 39, 93, 75, 10, 58, 20, 97, 49,
	44, 59, 23,  7, 81, 64,  6, 28, 62,  9
#endif
};

static const guint8 SUFFIX(e_digits)[] = {
#if DOUBLE_RADIX == 2
	0x02, 0xb7, 0xe1, 0x51, 0x62, 0x8a, 0xed, 0x2a,
	0x6a, 0xbf, 0x71, 0x58, 0x80, 0x9c, 0xf4, 0xf3,
	0xc7, 0x62, 0xe7, 0x16, 0x0f, 0x38, 0xb4, 0xda,
	0x56, 0xa7, 0x84, 0xd9, 0x04, 0x51, 0x90, 0xcf,
	0xef, 0x32, 0x4e, 0x77, 0x38, 0x92, 0x6c, 0xfb,
	0xe5, 0xf4, 0xbf, 0x8d, 0x8d, 0x8c, 0x31, 0xd7,
	0x63, 0xda, 0x06
#else
	 2, 71, 82, 81, 82, 84, 59,  4, 52, 35,
	36,  2, 87, 47, 13, 52, 66, 24, 97, 75,
	72, 47,  9, 36, 99, 95, 95, 74, 96, 69,
	67, 62, 77, 24,  7, 66, 30, 35, 35, 48
#endif
};

static const guint8 SUFFIX(ln2_digits)[] = {
#if DOUBLE_RADIX == 2
	0xb1, 0x72, 0x17, 0xf7, 0xd1, 0xcf, 0x79, 0xab,
	0xc9, 0xe3, 0xb3, 0x98, 0x03, 0xf2, 0xf6, 0xaf,
	0x40, 0xf3, 0x43, 0x26, 0x72, 0x98, 0xb6, 0x2d,
	0x8a, 0x0d, 0x17, 0x5b, 0x8b, 0xaa, 0xfa, 0x2b,
	0xe7, 0xb8, 0x76, 0x20, 0x6d, 0xeb, 0xac, 0x98,
	0x55, 0x95, 0x52, 0xfb, 0x4a, 0xfa, 0x1b, 0x10,
	0xed, 0x2e
#else
	69, 31, 47, 18,  5, 59, 94, 53,  9, 41,
	72, 32, 12, 14, 58, 17, 65, 68,  7, 55,
	 0, 13, 43, 60, 25, 52, 54, 12,  6, 80,
	 0, 94, 93, 39, 36, 21, 96, 96, 94, 72
#endif
};

static const guint8 SUFFIX(ln10_digits)[] = {
#if DOUBLE_RADIX == 2
	0x02, 0x4d, 0x76, 0x37, 0x76, 0xaa, 0xa2, 0xb0,
	0x5b, 0xa9, 0x5b, 0x58, 0xae, 0x0b, 0x4c, 0x28,
	0xa3, 0x8a, 0x3f, 0xb3, 0xe7, 0x69, 0x77, 0xe4,
	0x3a, 0x0f, 0x18, 0x7a, 0x08, 0x07, 0xc0, 0xb6
#else
	 2, 30, 25, 85,  9, 29, 94,  4, 56, 84,
	 1, 79, 91, 45, 46, 84, 36, 42,  7, 60,
	11,  1, 48, 86, 28, 77, 29, 76,  3, 33,
	27, 90,  9, 67, 57, 26,  9, 67, 73, 52
#endif
};


static const guint8 SUFFIX(sqrt2_digits)[] = {
#if DOUBLE_RADIX == 2
	0x01, 0x6a, 0x09, 0xe6, 0x67, 0xf3, 0xbc, 0xc9,
	0x08, 0xb2, 0xfb, 0x13, 0x66, 0xea, 0x95, 0x7d,
	0x3e, 0x3a, 0xde, 0xc1, 0x75, 0x12, 0x77, 0x50,
	0x99, 0xda, 0x2f, 0x59, 0x0b, 0x06, 0x67, 0x32,
	0x2a, 0x95, 0xf9, 0x06, 0x08, 0x75, 0x71, 0x45,
	0x87, 0x51, 0x63, 0xfc, 0xdf, 0xb9, 0x07, 0xb6,
	0x72, 0x1e, 0xe9
#else
	 1, 41, 42, 13, 56, 23, 73,  9, 50, 48,
	80, 16, 88, 72, 42,  9, 69, 80, 78, 56,
	96, 71, 87, 53, 76, 94, 80, 73, 17, 66,
	79, 73, 79, 90, 73, 24, 78, 46, 21,  7
#endif
};

static const guint8 SUFFIX(euler_digits)[] = {
#if DOUBLE_RADIX == 2
	0x93, 0xc4, 0x67, 0xe3, 0x7d, 0xb0, 0xc7, 0xa4,
	0xd1, 0xbe, 0x3f, 0x81, 0x01, 0x52, 0xcb, 0x56,
	0xa1, 0xce, 0xcc, 0x3a, 0xf6, 0x5c, 0xc0, 0x19,
	0x0c, 0x03, 0xdf, 0x34, 0x70, 0x9a, 0xff, 0xbd,
	0x8e, 0x4b, 0x59, 0xfa, 0x03, 0xa9, 0xf0, 0xee,
	0xd0, 0x64, 0x9c, 0xcb, 0x62, 0x10, 0x57, 0xd1,
	0x10, 0x56
#else
	57, 72, 15, 66, 49,  1, 53, 28, 60, 60,
	65, 12,  9,  0, 82, 40, 24, 31,  4, 21,
	59, 33, 59, 39, 92, 35, 98, 80, 57, 67,
	23, 48, 84, 86, 77, 26, 77, 76, 64, 67
#endif
};

/**
 * go_quad_start:
 *
 * Initializes #GOQuad arithmetic. Any use of #GOQuad must occur between calls
 * to go_quad_start() and go_quad_end().
 * Returns: (transfer full): a pointer to pass to go_quad_end() when done.
 **/
void *
SUFFIX(go_quad_start) (void)
{
	void *res = NULL;
	static gboolean first = TRUE;

#ifdef MIGHT_NEED_FPU_SETUP
	if (SUFFIX(go_quad_depth)++ > 0)
		return NULL;

	if (!SUFFIX(go_quad_functional) () && first)
		g_warning ("quad precision math may not be completely accurate.");

  #ifdef USE_FPU_CONTROL
	{
		fpu_control_t state, newstate;
		fpu_control_t mask =
			_FPU_EXTENDED | _FPU_DOUBLE | _FPU_SINGLE;

		_FPU_GETCW (state);
		res = go_memdup (&state, sizeof (state));

		newstate = (state & ~mask) | _FPU_DOUBLE;
		_FPU_SETCW (newstate);
	}
  #else
	/* Hope for the best.  */
  #endif
#endif

	if (first) {
		DOUBLE base = (DOUBLE_RADIX == 2 ? 256 : 100);
		first = FALSE;
		SUFFIX(CST) = 1 + SUFFIX(scalbn) (1, (DOUBLE_MANT_DIG + 1) / 2);
		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_pi),
					   SUFFIX(pi_digits),
					   G_N_ELEMENTS (SUFFIX(pi_digits)),
					   base,
					   base);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_2pi),
					   SUFFIX(pi_digits),
					   G_N_ELEMENTS (SUFFIX(pi_digits)),
					   base,
					   2 * base);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_pihalf),
					   SUFFIX(pi_digits),
					   G_N_ELEMENTS (SUFFIX(pi_digits)),
					   base,
					   base / 2);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_e),
					   SUFFIX(e_digits),
					   G_N_ELEMENTS (SUFFIX(e_digits)),
					   base,
					   base);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_ln2),
					   SUFFIX(ln2_digits),
					   G_N_ELEMENTS (SUFFIX(ln2_digits)),
					   base,
					   1);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_ln10),
					   SUFFIX(ln10_digits),
					   G_N_ELEMENTS (SUFFIX(ln10_digits)),
					   base,
					   base);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_sqrt2),
					   SUFFIX(sqrt2_digits),
					   G_N_ELEMENTS (SUFFIX(sqrt2_digits)),
					   base,
					   base);

		SUFFIX(go_quad_constant8) (&SUFFIX(go_quad_euler),
					   SUFFIX(euler_digits),
					   G_N_ELEMENTS (SUFFIX(euler_digits)),
					   base,
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
void
SUFFIX(go_quad_end) (void *state)
{
#ifdef MIGHT_NEED_FPU_SETUP
	SUFFIX(go_quad_depth)--;

	if (!state)
		return;

#ifdef USE_FPU_CONTROL
	_FPU_SETCW (*(fpu_control_t*)state);
#endif

	g_free (state);
#else
	(void)state;
#endif
}

const QUAD SUFFIX(go_quad_zero) = { 0, 0 };
const QUAD SUFFIX(go_quad_one) = { 1, 0 };
const QUAD SUFFIX(go_quad_half) = { 0.5, 0 };
/*
 * The following are non-const so we can initialize them.  However,
 * from other compilation units they are const.  My reading of C99
 * Section 6.2.7 says that is allowed.
 */
QUAD SUFFIX(go_quad_pi);
QUAD SUFFIX(go_quad_2pi);
QUAD SUFFIX(go_quad_pihalf);
QUAD SUFFIX(go_quad_e);
QUAD SUFFIX(go_quad_ln2);
QUAD SUFFIX(go_quad_ln10);
QUAD SUFFIX(go_quad_sqrt2);
QUAD SUFFIX(go_quad_euler);

#undef LNBASE
#if DOUBLE_RADIX == 2
#define LNBASE SUFFIX(go_quad_ln2)
#elif DOUBLE_RADIX == 10
#define LNBASE SUFFIX(go_quad_ln10)
#else
#error "Code needs fixing"
#endif

/**
 * go_quad_init:
 * @res: (out): result location
 * @h: a double precision value
 *
 * This stores the value @h in @res.  As an exception, this may be called
 * outside go_quad_start and go_quad_end sections.
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
void
SUFFIX(go_quad_add) (QUAD *res, const QUAD *a, const QUAD *b)
{
	DOUBLE r = a->h + b->h;
	DOUBLE s = SUFFIX(fabs) (a->h) > SUFFIX(fabs) (b->h)
		? a->h - r + b->h + b->l + a->l
		: b->h - r + a->h + a->l + b->l;
	res->h = r + s;
	res->l = r - res->h + s;

#ifdef MIGHT_NEED_FPU_SETUP
	g_return_if_fail (SUFFIX(go_quad_depth) > 0);
#endif
}

/**
 * go_quad_sub:
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

static int
SUFFIX(go_quad_compare) (const QUAD *a, const QUAD *b)
{
	QUAD d;
	int sa = a->h < 0;
	int sb = b->h < 0;

	if (sa != sb)
		return sa ? -1 : +1;

	// Same sign, so no overflow on subtraction.
	SUFFIX(go_quad_sub) (&d, a, b);
	if (d.h > 0)
		return +1;
	else if (d.h == 0)
		return 0;
	else
		return -1;
}

// 1: even integer, 0: non-integer (including inf, nan), -1 odd integer
static int
SUFFIX(go_quad_isint) (QUAD const *x)
{
	QUAD fx, rx;

	SUFFIX(go_quad_floor) (&fx, x);
	SUFFIX(go_quad_sub) (&rx, x, &fx);

	if (!(rx.h == 0))
		return 0;

	// If we have a lower part at this point then the upper is even.
	return SUFFIX(fmod) (fx.l ? fx.l : fx.h, 2) == 0 ? 1 : -1;
}

void
SUFFIX(go_quad_scalbn) (QUAD *res, const QUAD *a, int n)
{
	res->h = SUFFIX(scalbn) (a->h, n);
	res->l = SUFFIX(scalbn) (a->l, n);
}


/**
 * go_quad_sqrt:
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
		c.l = (a->h - u.h - u.l + a->l) * HALF / c.h;
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
void
SUFFIX(go_quad_constant8) (QUAD *res, const guint8 *data, gsize n,
			   DOUBLE base, DOUBLE scale)
{
	QUAD qbaseinv, q;

	*res = SUFFIX(go_quad_zero);
	SUFFIX(go_quad_init) (&qbaseinv, 1 / base);

	while (n-- > 0) {
		SUFFIX(go_quad_init) (&q, data[n]);
		SUFFIX(go_quad_add) (res, res, &q);
		SUFFIX(go_quad_mul) (res, res, &qbaseinv);
	}

	SUFFIX(go_quad_init) (&q, scale);
	SUFFIX(go_quad_mul) (res, res, &q);
}

#if DOUBLE_RADIX == 2
static void
SUFFIX(rescale_base) (QUAD *x, DOUBLE *e)
{
	int xe;

	(void)UNSCALBN (SUFFIX(go_quad_value) (x), &xe);
	if (xe != 0) {
		QUAD qs;
		SUFFIX(go_quad_init) (&qs, SUFFIX(scalbn) (1.0, -xe));
		SUFFIX(go_quad_mul) (x, x, &qs);
		*e += xe;
	}
}
#endif

#if DOUBLE_RADIX == 2
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
	SUFFIX(rescale_base) (&xn, &xe);

	while (dy > 0) {
		if (SUFFIX(fmod) (dy, 2) > 0) {
			SUFFIX(go_quad_mul) (res, res, &xn);
			*exp2 += xe;
			SUFFIX(rescale_base) (res, exp2);
			dy--;
			if (dy == 0)
				break;
		}
		dy /= 2;
		SUFFIX(go_quad_mul) (&xn, &xn, &xn);
		xe *= 2;
		SUFFIX(rescale_base) (&xn, &xe);
	}
}
#endif

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
	x1m = SUFFIX(fabs) (SUFFIX(go_quad_value) (x)) >= HALF;
	if (x1m) {
		SUFFIX(go_quad_sub) (&qx, x, &SUFFIX(go_quad_one));
	} else {
		qx = *x;
	}

	qr = r1m ? SUFFIX(go_quad_zero) : SUFFIX(go_quad_one);

	while ((dy = SUFFIX(go_quad_value) (&qy)) > 0) {
		SUFFIX(go_quad_add) (&qy, &qy, &qy);
		if (x1m) {
			SUFFIX(go_quad_sqrt1pm1) (&qx, &qx);
			if (SUFFIX(go_quad_value) (&qx) == 0)
				break;
		} else {
			SUFFIX(go_quad_sqrt) (&qx, &qx);
			if (SUFFIX(go_quad_value) (&qx) >= HALF) {
				x1m = TRUE;
				SUFFIX(go_quad_sub) (&qx, &qx, &SUFFIX(go_quad_one));
			}
		}
		if (dy >= HALF) {
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
 * @expb: (out): (allow-none): power-of-base result scaling location
 * @x: quad-precision value
 * @y: quad-precision value
 *
 * This function computes @x to the power of @y, storing the result in @res.
 * If the optional @expb is supplied, it is used to return a power of radix
 * by which the result should be scaled.  Such scaling can be done with the
 * scalbn function, typically after combining multiple such terms.  This is
 * useful to represent results much, much bigger than double precision can
 * handle.
 **/
void
SUFFIX(go_quad_pow) (QUAD *res, DOUBLE *expb,
		     const QUAD *x, const QUAD *y)
{
	if (expb) *expb = 0;

	if (y->h == 0 || SUFFIX(go_quad_compare) (x, &SUFFIX(go_quad_one)) == 0)
		return (void)(*res = SUFFIX(go_quad_one));
	if (x->h == 0 && y->h > 0)
		return (void)(*res = SUFFIX(go_quad_zero));
	if (SUFFIX(isnan) (x->h))
		return (void)(*res = *x);
	if (SUFFIX(isnan) (y->h))
		return (void)(*res = *y);
	if (SUFFIX(go_quad_compare) (y, &SUFFIX(go_quad_one)) == 0)
		return (void)(*res = *x);
	if (x->h > 0 && SUFFIX(go_quad_compare) (y, &SUFFIX(go_quad_half)) == 0)
		return SUFFIX(go_quad_sqrt) (res, x);

#if DOUBLE_RADIX == 2
	// "this is a base-2 algorithm"
	DOUBLE dy, exp2ew;
	QUAD qw, qf, qew, qef, qxm1;

	dy = SUFFIX(go_quad_value) (y);

	SUFFIX(go_quad_sub) (&qxm1, x, &SUFFIX(go_quad_one));

	SUFFIX(go_quad_floor) (&qw, y);
	SUFFIX(go_quad_sub) (&qf, y, &qw);
	if (SUFFIX(go_quad_value) (&qxm1) == 0 && dy > 0) {
		int wint = SUFFIX(go_quad_isint) (&qw);
		if (SUFFIX(go_quad_value) (&qf) == 0 && wint < 0) {
			/* 0 ^ (odd positive integer) */
			*res = *x;
		} else {
			/* 0 ^ y, y positive, but not odd integer */
			*res = SUFFIX(go_quad_zero);
		}
		return;
	}

	/* Lots of infinity/nan cases ignored */

	if (dy < 0) {
		QUAD my;
		SUFFIX(go_quad_sub) (&my, &SUFFIX(go_quad_zero), y);
		SUFFIX(go_quad_pow) (res, expb, x, &my);
		SUFFIX(go_quad_div) (res, &SUFFIX(go_quad_one), res);
		if (expb) *expb = 0 - *expb;
		return;
	}

	SUFFIX(go_quad_pow_int) (&qew, &exp2ew, x, &qw);
	SUFFIX(go_quad_pow_frac) (&qef, x, &qf, FALSE);

	SUFFIX(go_quad_mul) (res, &qew, &qef);
	if (expb)
		*expb = exp2ew;
	else {
		int e = CLAMP (exp2ew, G_MININT, G_MAXINT);
		SUFFIX(go_quad_scalbn) (res, res, e);
	}
#else
	QUAD lx, ax, f10;
	gboolean qneg = FALSE;
	int e;
	DOUBLE er;

	SUFFIX(go_quad_abs) (&ax, x);

	if (x->h < 0) {
		int yint = SUFFIX(go_quad_isint) (y);
		if (!yint)
			return SUFFIX(go_quad_init) (res, go_nan);
		qneg = yint < 0;
	}

	// x = z * 10^k
	// x^y = z^y * 10^(ky)

	ax.h = UNSCALBN (ax.h, &e);
	if (ax.h < 1 / SUFFIX(sqrt) (DOUBLE_RADIX)) {
		ax.h *= DOUBLE_RADIX;
		e--;
	}
	ax.l = SUFFIX(scalbn) (ax.l, -e);

	if (e == 0) {
		er = 0;
		f10 = SUFFIX(go_quad_one);
	} else {
		QUAD fy, qe, qer1, qer2, qr;

		SUFFIX(go_quad_floor) (&fy, y);
		SUFFIX(go_quad_init) (&qe, e);
		SUFFIX(go_quad_mul) (&qer1, &fy, &qe);
		SUFFIX(go_quad_sub) (&fy, y, &fy);
		SUFFIX(go_quad_mul) (&qr, &fy, &qe);
		SUFFIX(go_quad_floor) (&qer2, &qr);
		SUFFIX(go_quad_sub) (&qr, &qr, &qer2);

		SUFFIX(go_quad_add) (&qer1, &qer1, &qer2);
		er = SUFFIX(go_quad_value) (&qer1);

		SUFFIX(go_quad_mul) (&f10, &qr, &LNBASE);
		SUFFIX(go_quad_exp) (&f10, NULL, &f10);
	}

	SUFFIX(go_quad_log) (&lx, &ax);
	SUFFIX(go_quad_mul) (&lx, &lx, y);
	SUFFIX(go_quad_exp) (res, expb, &lx);
	if (expb) {
		*expb += er;
	} else {
		er = CLAMP (er, G_MININT, G_MAXINT);
		SUFFIX(go_quad_scalbn) (res, res, er);
	}
	SUFFIX(go_quad_mul) (res, res, &f10);
	if (qneg)
		SUFFIX(go_quad_negate) (res, res);
#endif
}

#if DOUBLE_RADIX == 10
static void
SUFFIX(go_quad_exp_taylor) (QUAD *res, QUAD const *x)
{
	QUAD qxn[DOUBLE_DIG * 2 + 10], term[DOUBLE_DIG * 2 + 10];
	QUAD sum = SUFFIX(go_quad_zero);
	QUAD qf = SUFFIX(go_quad_one);
	unsigned i;

	// We have |x| <= 0.1 except when creating e_parts

	qxn[0] = term[0] = SUFFIX(go_quad_one);
	qxn[1] = term[1] = *x;
	for (i = 2; i < G_N_ELEMENTS(qxn); i++) {
		QUAD qi;
		SUFFIX(go_quad_init) (&qi, i);
		SUFFIX(go_quad_mul) (&qf, &qf, &qi);
		SUFFIX(go_quad_mul) (qxn + i, qxn + (i / 2), qxn + ((i + 1) / 2));
		SUFFIX(go_quad_div) (term + i, qxn + i, &qf);
		if (SUFFIX(fabs) (term[i].h) < (DOUBLE_EPSILON * DOUBLE_EPSILON / 100))
			break;
	}

	while (i-- > 0) {
		// g_printerr ("%d: x^n %.16Wg\n", i, qxn[i].h);
		// g_printerr ("%d: term %.16Wg\n", i, term[i].h);
		SUFFIX(go_quad_add) (&sum, &sum, term + i);
	}

	*res = sum;
}
#endif

/**
 * go_quad_exp:
 * @res: (out): result location
 * @expb: (out): (allow-none): power-of-base result scaling location
 * @a: quad-precision value
 *
 * This function computes the exponential function at @a, storing the result
 * in @res.  If the optional @expb is supplied, it is used to return a
 * power of radix by which the result should be scaled.  This is useful to
 * represent results much, much bigger than double precision can handle.
 **/
void
SUFFIX(go_quad_exp) (QUAD *res, DOUBLE *expb, const QUAD *a)
{
#if DOUBLE_RADIX == 2
	SUFFIX(go_quad_pow) (res, expb, &SUFFIX(go_quad_e), a);
#else
	DOUBLE pbase, da;
	int parts;
	QUAD qpbase, qparts, qares, qres;
#if DOUBLE_RADIX == 10
	static const DOUBLE lnbaseparts[] = {
		CONST(2.302585092994045),
		CONST(.6840179914546843e-15),
		CONST(.6420760110148862e-31),
		CONST(.8772976033327901e-47)
	};
	static QUAD e_parts[24];  // exp(i/10)
	if (e_parts[0].h == 0) {
		QUAD qtenth = { CONST(1.) / DOUBLE_RADIX, 0 };
		e_parts[0] = SUFFIX(go_quad_one);
		SUFFIX(go_quad_exp_taylor) (e_parts + 1, &qtenth);
		for (parts = 2; parts < (int)G_N_ELEMENTS(e_parts); parts++) {
			if (parts < DOUBLE_RADIX) {
				QUAD qtenth = { (_Decimal64)parts / DOUBLE_RADIX, 0 };
				SUFFIX(go_quad_exp_taylor) (e_parts + parts, &qtenth);
			} else if (parts == DOUBLE_RADIX)
				e_parts[parts] = SUFFIX(go_quad_e);
			else
				SUFFIX(go_quad_mul) (e_parts + parts,
						     e_parts + (parts / 2),
						     e_parts + ((parts + 1) / 2));
		}
	}
#endif

	da = SUFFIX(go_quad_value) (a);
	if (!SUFFIX(go_finite) (da)) {
		if (da < 0)
			*res = SUFFIX(go_quad_zero);
		else
			*res = *a;
		if (expb) *expb = 0;
		return;
	}

	// Extract powers of base
	SUFFIX(go_quad_div) (&qpbase, a, &LNBASE);
	SUFFIX(go_quad_add) (&qpbase, &qpbase, &SUFFIX(go_quad_half));
	SUFFIX(go_quad_floor) (&qpbase, &qpbase);
	pbase = SUFFIX(go_quad_value) (&qpbase);
	qares = *a;
	for (unsigned i = 0; i < G_N_ELEMENTS(lnbaseparts); i++) {
		QUAD qp;
		SUFFIX(go_quad_mul12) (&qp, pbase, lnbaseparts[i]);
		SUFFIX(go_quad_sub) (&qares, &qares, &qp);
	}

	SUFFIX(go_quad_scalbn) (&qparts, &qares, 1);
	SUFFIX(go_quad_add) (&qparts, &qparts, &SUFFIX(go_quad_half));
	SUFFIX(go_quad_floor) (&qparts, &qparts);
	parts = (int)SUFFIX(go_quad_value) (&qparts);
	SUFFIX(go_quad_scalbn) (&qparts, &qparts, -1);
	SUFFIX(go_quad_sub) (&qares, &qares, &qparts);

	SUFFIX(go_quad_exp_taylor) (&qres, &qares);

	if (parts >= (int)G_N_ELEMENTS(e_parts) || parts <= -(int)G_N_ELEMENTS(e_parts))
		g_printerr("Something is funky in quad exp.\n");
	else if (parts > 0) {
		// g_printerr ("%.16Wg + %.16Wg\n", e_parts[parts].h, e_parts[parts].l);
		SUFFIX(go_quad_mul) (&qres, &qres, &e_parts[parts]);
	} else if (parts < 0) {
		// g_printerr ("%.16Wg + %.16Wg\n", e_parts[-parts].h, e_parts[-parts].l);
		SUFFIX(go_quad_div) (&qres, &qres, &e_parts[-parts]);
	}

	if (expb) {
		DOUBLE m = SUFFIX(go_quad_value) (&qres);
		if (0 < m && m < 1) {
			pbase--;
			SUFFIX(go_quad_scalbn) (&qres, &qres, 1);
		}
		*expb = pbase;
	} else {
		pbase = CLAMP (pbase, G_MININT, G_MAXINT);
		SUFFIX(go_quad_scalbn) (&qres, &qres, (int)pbase);
	}
	*res = qres;

#endif
}

/**
 * go_quad_expm1:
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
	else if (SUFFIX (fabs) (da) > HALF) {
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
		QUAD as, xi, yi, dx, dl;
		int e;

		// Scale down to near 1.
		da = UNSCALBN (da, &e);
		if (da < 1 / SUFFIX(sqrt) (DOUBLE_RADIX)) e--;
		SUFFIX(go_quad_scalbn) (&as, a, -e);

		// Initial approximation
		SUFFIX(go_quad_init) (&xi, SUFFIX(log) (as.h));

		// Newton step.
		SUFFIX(go_quad_exp) (&yi, NULL, &xi);
		SUFFIX(go_quad_sub) (&dx, &as, &yi);
		SUFFIX(go_quad_div) (&dx, &dx, &yi);
		SUFFIX(go_quad_add) (&xi, &xi, &dx);

		// Adjust for scaling
		SUFFIX(go_quad_init) (&dl, e);
		SUFFIX(go_quad_mul) (&dl, &LNBASE, &dl);
		SUFFIX(go_quad_add) (&xi, &xi, &dl);

		*res = xi;
	}
}

/**
 * go_quad_hypot:
 * @res: (out): result location
 * @a: quad-precision value
 * @b: quad-precision value
 *
 * This function computes the square root of @a^2 plus @b^2, storing the
 * result in @res.
 **/
void
SUFFIX(go_quad_hypot) (QUAD *res, const QUAD *a, const QUAD *b)
{
	int e;
	QUAD qa, qb, qn;
	DOUBLE maxh;

	SUFFIX(go_quad_abs)(&qa, a);
	SUFFIX(go_quad_abs)(&qb, b);

	if (qa.h == 0)
		return (void)(*res = qb);
	if (qb.h == 0)
		return (void)(*res = qa);
	if (qa.h == (DOUBLE)INFINITY || qb.h == (DOUBLE)INFINITY)
		return SUFFIX(go_quad_init) (res, INFINITY);
	if (SUFFIX(isnan) (qa.h) || SUFFIX(isnan) (qb.h))
		return SUFFIX(go_quad_init) (res, NAN);

	/* Scale by power of radix to protect against over- and underflow */
	maxh = MAX (qa.h, qb.h);
	(void)UNSCALBN (maxh, &e);

	SUFFIX(go_quad_scalbn) (&qa, &qa, -e);
	SUFFIX(go_quad_mul) (&qa, &qa, &qa);

	SUFFIX(go_quad_scalbn) (&qb, &qb, -e);
	SUFFIX(go_quad_mul) (&qb, &qb, &qb);

	SUFFIX(go_quad_add) (&qn, &qa, &qb);
	SUFFIX(go_quad_sqrt) (&qn, &qn);
	SUFFIX(go_quad_scalbn) (res, &qn, e);
}

/**
 * go_quad_abs:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function computes the absolute value of @a, storing the result in @res.
 **/
void
SUFFIX(go_quad_abs) (QUAD *res, const QUAD *a)
{
	if (a->h < 0)
		SUFFIX(go_quad_negate) (res, a);
	else
		*res = *a;
}


/**
 * go_quad_negate:
 * @res: (out): result location
 * @a: quad-precision value
 *
 * This function negates @a and stores the result in @res.
 **/
void
SUFFIX(go_quad_negate) (QUAD *res, const QUAD *a)
{
	res->h = -a->h;
	res->l = -a->l;
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
		SUFFIX(go_quad_mul) (&dk[0], &dk[0], &SUFFIX(go_quad_half));

		SUFFIX(go_quad_mul) (&g, &dk[0], &gp);
		SUFFIX(go_quad_sqrt) (&g, &g);

		for (k = 1; k <= n; k++) {
			QUAD f;

			SUFFIX(go_quad_init) (&f, go_pow2 (-2 * k));
			SUFFIX(go_quad_mul) (&dk[k], &f, &dpk[k-1]);
			SUFFIX(go_quad_sub) (&dk[k], &dk[k-1], &dk[k]);
			SUFFIX(go_quad_sub) (&f, &SUFFIX(go_quad_one), &f);
			SUFFIX(go_quad_div) (&dk[k], &dk[k], &f);
		}

		SUFFIX(go_quad_div) (&qr, &qnum, &dk[n]);
		SUFFIX(go_quad_sub) (&qrp, &qrp, &qr);
		if (SUFFIX(fabs)(qrp.h) <= SUFFIX(scalbn) (SUFFIX(fabs)(qr.h), -2 * (DOUBLE_MANT_DIG - 1))) {
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
		*f = (dy >= 0 ? HALF : -HALF);
		return TRUE;
	}

	if (SUFFIX(fabs) (SUFFIX(fabs)(dx) - SUFFIX(fabs)(dy)) < (DOUBLE)1e-10) {
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
void
SUFFIX(go_quad_atan2) (QUAD *res, const QUAD *y, const QUAD *x)
{
	DOUBLE f;
	DOUBLE dy = SUFFIX(go_quad_value) (y);
	DOUBLE dx = SUFFIX(go_quad_value) (x);
	QUAD qr;

	if (SUFFIX(go_quad_atan2_special) (y, x, &f)) {
		QUAD qf;
		SUFFIX(go_quad_init) (&qf, f);
		SUFFIX(go_quad_mul) (res, &qf, &SUFFIX(go_quad_pi));
		return;
	}

	if (SUFFIX(fabs) (dx) >= SUFFIX(fabs) (dy)) {
		SUFFIX(go_quad_div) (&qr, y, x);
		SUFFIX(go_quad_agm_internal) (res, AGM_ARCTAN, &qr);
	} else {
		QUAD qa;

		SUFFIX(go_quad_div) (&qr, x, y);
		SUFFIX(go_quad_agm_internal) (res, AGM_ARCTAN, &qr);

		qa = SUFFIX(go_quad_pihalf);
		if (qr.h < 0)
			SUFFIX(go_quad_negate) (&qa, &qa);
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
	QUAD qa, qk, qb;
	DOUBLE k;
	unsigned ui;
	static const DOUBLE pi_half_parts[] = {
#if DOUBLE_RADIX == 2
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
#else
		CONST(1.570796326794896),
		CONST(6192313216916397e-31),
		CONST(5144209858469968e-47),
		CONST(7552910487472296e-63),
		CONST(1539082031431044e-79),
		CONST(9931401741267105e-95),
		CONST(8533991074043256e-111),
		CONST(6411533235469223e-127),
		CONST(0477529111586267e-143),
		CONST(9704064240558725e-169)
#endif
	};

	if (!SUFFIX(go_finite) (a->h))
		return TRUE;

	if (SUFFIX(fabs) (a->h) > 1 / DOUBLE_EPSILON) {
		g_warning ("Reduced accuracy for very large trigonometric arguments");
		return TRUE;
	}

	SUFFIX(go_quad_div) (&qk, a, &SUFFIX(go_quad_pihalf));
	SUFFIX(go_quad_add) (&qk, &qk, &SUFFIX(go_quad_half));
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
	int k = 0;
	QUAD qxr = *a;

	if (a->h < 0) {
		QUAD aa;
		SUFFIX(go_quad_negate) (&aa, a);
		SUFFIX(reduce_half) (&qxr, &aa, &k);
		SUFFIX(go_quad_negate) (&qxr, &qxr);
		k = 4 - k;
		if (qxr.h <= (DOUBLE)-0.25 && qxr.l == 0) {
			SUFFIX(go_quad_add) (&qxr, &qxr, &SUFFIX(go_quad_half));
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
		if (qxr.h >= HALF) {
			SUFFIX(go_quad_sub) (&qxr, &qxr, &SUFFIX(go_quad_half));
			k++;
		}
		if (qxr.h > (DOUBLE)0.25) {
			SUFFIX(go_quad_sub) (&qxr, &qxr, &SUFFIX(go_quad_half));
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

		SUFFIX(go_quad_abs) (&aa, a);
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
	else if (a->h == (DOUBLE)0.25 && a->l == 0)
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
void
SUFFIX(go_quad_asin) (QUAD *res, const QUAD *a)
{
	QUAD aa, aam1;

	SUFFIX(go_quad_abs) (&aa, a);
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
void
SUFFIX(go_quad_acos) (QUAD *res, const QUAD *a)
{
	QUAD aa, aam1;

	SUFFIX(go_quad_abs) (&aa, a);
	SUFFIX(go_quad_sub) (&aam1, &aa, &SUFFIX(go_quad_one));
	if (aam1.h > 0) {
		SUFFIX(go_quad_init) (res, SUFFIX(go_nan));
		return;
	}

	SUFFIX(go_quad_agm_internal) (res, AGM_ARCCOS, &aa);
	if (a->h < 0)
		SUFFIX(go_quad_sub) (res, &SUFFIX(go_quad_pi), res);
}

/* ------------------------------------------------------------------------- */

// See comments at top
#endif // SKIP_THIS_PASS
#if INCLUDE_PASS < INCLUDE_PASS_LAST
#include __FILE__
#endif
