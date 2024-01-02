/*
 * complex.c:  A quick library for complex math.
 *
 * Author:
 *  Morten Welinder <terra@gnome.org>
 *  Jukka-Pekka Iivonen <iivonen@iki.fi>
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
 */

#include <goffice-config.h>
#define GOFFICE_COMPLEX_IMPLEMENTATION
#include "go-complex.h"

#include <stdlib.h>
#include <errno.h>

// We need multiple versions of this code.  We're going to include ourself
// with different settings of various macros.  gdb will hate us.
#include <goffice/goffice-multipass.h>
#ifndef SKIP_THIS_PASS

#define COMPLEX SUFFIX(GOComplex)

/* ------------------------------------------------------------------------- */

static COMPLEX *
SUFFIX(go_complex_dup) (COMPLEX *src)
{
	return go_memdup (src, sizeof (*src));
}

#define MAKE_BOXED_TYPE(id,name)					\
GType									\
id (void)								\
{									\
	static GType t = 0;						\
									\
	if (t == 0)							\
		t = g_boxed_type_register_static			\
			(name,						\
			 (GBoxedCopyFunc)SUFFIX(go_complex_dup),	\
			 (GBoxedFreeFunc)g_free);			\
	return t;							\
}


#if INCLUDE_PASS == INCLUDE_PASS_LONG_DOUBLE
MAKE_BOXED_TYPE(go_complexl_get_type, "GOComplexl")
#elif INCLUDE_PASS == INCLUDE_PASS_DECIMAL64
MAKE_BOXED_TYPE(go_complexD_get_type, "GOComplexD")
#else
MAKE_BOXED_TYPE(go_complex_get_type, "GOComplex")
#ifndef GOFFICE_WITH_LONG_DOUBLE
// Hack for introspection
GType go_complexl_get_type (void);
GType go_complexl_get_type (void) { return 0; }
#endif
#ifndef GOFFICE_WITH_DECIMAL64
// Hack for introspection
GType go_complexD_get_type (void);
GType go_complexD_get_type (void) { return 0; }
#endif
#endif

/* ------------------------------------------------------------------------- */

char *
SUFFIX(go_complex_to_string) (COMPLEX const *src, char const *reformat,
			      char const *imformat, char imunit)
{
	char *re_buffer = NULL;
	char *im_buffer = NULL;
	char const *sign = "";
	char const *suffix = "";
	char *res;
	char suffix_buffer[2];

	if (src->re != 0 || src->im == 0) {
		/* We have a real part.  */
		re_buffer = g_strdup_printf (reformat, src->re);
	}

	if (src->im != 0) {
		/* We have an imaginary part.  */
		suffix = suffix_buffer;
		suffix_buffer[0] = imunit;
		suffix_buffer[1] = 0;
		if (src->im == 1) {
			if (re_buffer)
				sign = "+";
		} else if (src->im == -1) {
			sign = "-";
		} else {
			im_buffer = g_strdup_printf (imformat, src->im);
			if (re_buffer && *im_buffer != '-' && *im_buffer != '+')
				sign = (src->im >= 0) ? "+" : "-";
		}
	}

	res = g_strconcat (re_buffer ? re_buffer : "",
			   sign,
			   im_buffer ? im_buffer : "",
			   suffix,
			   NULL);

	g_free (re_buffer);
	g_free (im_buffer);

	return res;
}

/* ------------------------------------------------------------------------- */

static int
SUFFIX(is_unit_imaginary) (char const *src, DOUBLE *im, char *imunit)
{
	if (*src == '-') {
		*im = -1.0;
		src++;
	} else {
		*im = +1.0;
		if (*src == '+') src++;
	}

	if ((*src == 'i' || *src == 'j') && src[1] == 0) {
		*imunit = *src;
		return 1;
	} else
		return 0;
}

int
SUFFIX(go_complex_from_string) (COMPLEX *dst, char const *src, char *imunit)
{
	DOUBLE x, y;
	char *end;

	/* Case: "i", "+i", "-i", ...  */
	if (SUFFIX(is_unit_imaginary) (src, &dst->im, imunit)) {
		dst->re = 0;
		return 0;
	}

	x = STRTO (src, &end);
	if (src == end || errno == ERANGE)
		return -1;
	src = end;

	/* Case: "42", "+42", "-42", ...  */
	if (*src == 0) {
		SUFFIX(go_complex_real) (dst, x);
		*imunit = 'i';
		return 0;
	}

	/* Case: "42i", "+42i", "-42i", ...  */
	if ((*src == 'i' || *src == 'j') && src[1] == 0) {
		SUFFIX(go_complex_init) (dst, 0, x);
		*imunit = *src;
		return 0;
	}

	/* Case: "42+i", "+42-i", "-42-i", ...  */
	if (SUFFIX(is_unit_imaginary) (src, &dst->im, imunit)) {
		dst->re = x;
		return 0;
	}

	y = STRTO (src, &end);
	if (src == end || errno == ERANGE)
		return -1;
	src = end;

	/* Case: "42+12i", "+42-12i", "-42-12i", ...  */
	if ((*src == 'i' || *src == 'j') && src[1] == 0) {
		SUFFIX(go_complex_init) (dst, x, y);
		*imunit = *src;
		return 0;
	}

	return -1;
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_to_polar) (DOUBLE *mod, DOUBLE *angle, COMPLEX const *src)
{
	*mod = SUFFIX(go_complex_mod) (src);
	*angle = SUFFIX(go_complex_angle) (src);
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_from_polar) (COMPLEX *dst, DOUBLE mod, DOUBLE angle)
{
	SUFFIX(go_complex_init) (dst,
				 mod * SUFFIX(cos) (angle),
				 mod * SUFFIX(sin) (angle));
}

void
SUFFIX(go_complex_from_polar_pi) (COMPLEX *dst, DOUBLE mod, DOUBLE angle)
{
	SUFFIX(go_complex_init) (dst,
				 mod * SUFFIX(go_cospi) (angle),
				 mod * SUFFIX(go_sinpi) (angle));
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_mul) (COMPLEX *dst, COMPLEX const *a, COMPLEX const *b)
{
	SUFFIX(go_complex_init) (dst,
		      a->re * b->re - a->im * b->im,
		      a->re * b->im + a->im * b->re);
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_div) (COMPLEX *dst, COMPLEX const *a, COMPLEX const *b)
{
	DOUBLE a_re = a->re, a_im = a->im;
	DOUBLE b_re = b->re, b_im = b->im;
	DOUBLE asize = MAX (SUFFIX(fabs)(a_re), SUFFIX(fabs)(a_im));
	DOUBLE bsize = MAX (SUFFIX(fabs)(b_re), SUFFIX(fabs)(b_im));
	DOUBLE bmodsqr, q_re, q_im;
	int e = 0;

	if (!SUFFIX(go_finite) (asize) || !SUFFIX(go_finite) (bsize) ||
	    bsize == 0) {
		// Inf or Nan occurs somewhere.  Or divide by zero.
		SUFFIX(go_complex_invalid) (dst);
		return;
	}

	if (b_im == 0) {
		// Special case for correctly-rounded result
		SUFFIX(go_complex_init) (dst, a_re / b_re, a_im / b_re);
		return;
	}

	if (b_re == 0) {
		// Special case for correctly-rounded result
		SUFFIX(go_complex_init) (dst, a_im / b_im, -a_re / b_im);
		return;
	}

	if (asize + bsize > CONST(1e100) ||
	    asize < CONST(1e-100) ||
	    bsize < CONST(1e-100)) {
		// Avoid overflows and underflows by scaling both arguments
		// to sane sizes without any rounding errors.
		int ea, eb;

		(void)UNSCALBN (asize, &ea);
		a_re = SUFFIX(scalbn) (a_re, -ea);
		a_im = SUFFIX(scalbn) (a_im, -ea);

		(void)UNSCALBN (bsize, &eb);
		b_re = SUFFIX(scalbn) (b_re, -eb);
		b_im = SUFFIX(scalbn) (b_im, -eb);

		e = ea - eb;
	}

	bmodsqr = b_re * b_re + b_im * b_im;
	q_re = (a_re * b_re + a_im * b_im) / bmodsqr;
	q_im = (a_im * b_re - a_re * b_im) / bmodsqr;

	if (e) {
		q_re = SUFFIX(scalbn) (q_re, e);
		q_im = SUFFIX(scalbn) (q_im, e);
	}

	SUFFIX(go_complex_init) (dst, q_re, q_im);
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_sqrt) (COMPLEX *dst, COMPLEX const *src)
{
	DOUBLE m, sm;

	if (src->re < 0 && -src->re > SUFFIX(fabs)(src->im)) {
		// Near the negative axis we do not want to squeeze
		// angle/pi against +-1.
		COMPLEX msrc, r;
		msrc.re = - src->re;
		msrc.im = - src->im;
		SUFFIX(go_complex_sqrt) (&r, &msrc);
		if (src->im >= 0)
			SUFFIX(go_complex_init) (dst, 0 - r.im, r.re);
		else
			SUFFIX(go_complex_init) (dst, r.im, 0 - r.re);
		return;
	}

	m = SUFFIX(go_complex_mod) (src);
	if (SUFFIX(go_finite) (m))
		sm = SUFFIX(sqrt) (m);
	else {
		// Overflow avoidance
		sm = 2 * SUFFIX(sqrt) (SUFFIX(hypot) (src->re / 4, src->im / 4));
	}

	SUFFIX(go_complex_from_polar_pi)
		(dst,
		 sm,
		 SUFFIX(go_complex_angle_pi) (src) / 2);
}

/* ------------------------------------------------------------------------- */

/*
 * Subtract a whole number leaving a fractional part in [-0.5,+0.5]
 * We only look at z->h, so this probably won't do the full reduction
 * for huge z.
 */
static void
SUFFIX(reduce1) (SUFFIX(GOQuad) *z)
{
	SUFFIX(GOQuad) d;

	if (SUFFIX (fabs) (z->h) <= CONST(0.5))
		return;

	SUFFIX(go_quad_init) (&d, SUFFIX(round) (z->h));
	SUFFIX(go_quad_sub) (z, z, &d);
}

static void
SUFFIX(mulmod1) (SUFFIX(GOQuad) *dst, SUFFIX(GOQuad) const *qa_, DOUBLE b)
{
	SUFFIX(GOQuad) qa = *qa_, qfb, qfa, qp, res;
	DOUBLE wb, wa;
	int ea, eb, de;

	(void)UNSCALBN (SUFFIX(go_quad_value) (&qa), &ea);
	(void)UNSCALBN (b, &eb);
	if (ea + eb <= 0) {
		/* |ab| <= 2 */
		SUFFIX(go_quad_init) (&qfb, b);
		SUFFIX(go_quad_mul) (dst, &qfb, &qa);
		return;
	}

	de = (ea - eb) / 2;
	if (de) {
		DOUBLE f = SUFFIX(scalbn) (1, de);
		b *= f;
		qa.h /= f;
		qa.l /= f;
	}

	wb = SUFFIX(round) (b);
	b -= wb;
	SUFFIX(go_quad_init) (&qfb, b);

	wa = SUFFIX (round) (SUFFIX(go_quad_value) (&qa));
	SUFFIX(go_quad_init) (&qfa, wa);
	SUFFIX(go_quad_sub) (&qfa, &qa, &qfa);

	/*
	 * Compute (wb+qfb)*(wa+qfa) mod 1.
	 *
	 * We can ignore wa*wb.  (And qfb==b.)
	 */

	SUFFIX(go_quad_mul) (&res, &qfa, &qfb);

	SUFFIX(go_quad_mul12) (&qp, wa, b);
	SUFFIX(reduce1) (&qp);
	SUFFIX(go_quad_add) (&res, &res, &qp);

	SUFFIX(go_quad_init) (&qp, wb);
	SUFFIX(go_quad_mul) (&qp, &qp, &qfa);
	SUFFIX(reduce1) (&qp);
	SUFFIX(go_quad_add) (&res, &res, &qp);

	SUFFIX(reduce1) (&res);

	*dst = res;
}

void
SUFFIX(go_complex_powx) (COMPLEX *dst, DOUBLE *e,
			 COMPLEX const *a, COMPLEX const *b)
{
	if (e)
		*e = 0;

	if (b->im == 0) {
		if (!e && SUFFIX(go_complex_real_p) (a) && a->re >= 0) {
			SUFFIX(go_complex_init) (dst, SUFFIX(pow) (a->re, b->re), 0);
			return;
		}

		if (b->re == 0) {
			SUFFIX(go_complex_init) (dst, 1, 0);
			return;
		}

		if (b->re == 1) {
			*dst = *a;
			return;
		}

		if (b->re == 2) {
			SUFFIX(go_complex_mul) (dst, a, a);
			return;
		}
	}

	{
		DOUBLE e1, e2, er;
		SUFFIX(GOQuad) qr, qa, qb, qarg, qrr, qra;
		void *state = SUFFIX(go_quad_start) ();

		/* Convert a to polar coordinates.  */
		SUFFIX(go_quad_init) (&qa, a->re);
		SUFFIX(go_quad_init) (&qb, a->im);
		SUFFIX(go_quad_atan2pi) (&qarg, &qb, &qa);
		SUFFIX(go_quad_hypot) (&qr, &qa, &qb);

		/* Compute result modulus.  */
		SUFFIX(go_quad_init) (&qa, b->re);
		SUFFIX(go_quad_pow) (&qa, &e1, &qr, &qa);
		SUFFIX(go_quad_init) (&qb, -b->im);
		SUFFIX(go_quad_mul) (&qb, &qb, &qarg);
		SUFFIX(go_quad_mul) (&qb, &qb, &SUFFIX(go_quad_pi));
		SUFFIX(go_quad_exp) (&qb, &e2, &qb);
		SUFFIX(go_quad_mul) (&qrr, &qa, &qb);
		er = e1 + e2;
		if (e)
			*e = er;
		else {
			er = CLAMP (er, G_MININT, G_MAXINT);
			SUFFIX(go_quad_scalbn) (&qrr, &qrr, er);
		}

		/* Compute result angle.  */
		SUFFIX(go_quad_log) (&qa, &qr);
		SUFFIX(go_quad_div) (&qa, &qa, &SUFFIX(go_quad_2pi));
		SUFFIX(mulmod1) (&qa, &qa, b->im);
		SUFFIX(mulmod1) (&qb, &qarg, b->re / 2);
		SUFFIX(go_quad_add) (&qa, &qa, &qb);
		SUFFIX(go_quad_add) (&qra, &qa, &qa);

		/* Convert to rectangular coordinates.  */
		SUFFIX(go_quad_sinpi) (&qa, &qra);
		SUFFIX(go_quad_mul) (&qa, &qa, &qrr);
		SUFFIX(go_quad_cospi) (&qb, &qra);
		SUFFIX(go_quad_mul) (&qb, &qb, &qrr);

		SUFFIX(go_complex_init) (dst,
					 SUFFIX(go_quad_value) (&qb),
					 SUFFIX(go_quad_value) (&qa));

		SUFFIX(go_quad_end) (state);
	}
}

void
SUFFIX(go_complex_pow) (COMPLEX *dst, COMPLEX const *a, COMPLEX const *b)
{
	SUFFIX(go_complex_powx) (dst, NULL, a, b);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_init) (COMPLEX *dst, DOUBLE re, DOUBLE im)
{
	dst->re = re;
	dst->im = im;
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_invalid) (COMPLEX *dst)
{
	dst->re = SUFFIX(go_nan);
	dst->im = SUFFIX(go_nan);
}

void SUFFIX(go_complex_real) (COMPLEX *dst, DOUBLE re)
{
	dst->re = re;
	dst->im = 0;
}

/* ------------------------------------------------------------------------- */

int SUFFIX(go_complex_real_p) (COMPLEX const *src)
{
	return src->im == 0;
}

/* ------------------------------------------------------------------------- */

int SUFFIX(go_complex_zero_p) (COMPLEX const *src)
{
	return src->re == 0 && src->im == 0;
}

/* ------------------------------------------------------------------------- */

int SUFFIX(go_complex_invalid_p) (COMPLEX const *src)
{
	return !(SUFFIX(go_finite) (src->re) && SUFFIX(go_finite) (src->im));
}

/* ------------------------------------------------------------------------- */

DOUBLE SUFFIX(go_complex_mod) (COMPLEX const *src)
{
	return SUFFIX(hypot) (src->re, src->im);
}

/* ------------------------------------------------------------------------- */

DOUBLE SUFFIX(go_complex_angle) (COMPLEX const *src)
{
	return SUFFIX(atan2) (src->im, src->re);
}

/* ------------------------------------------------------------------------- */
/*
 * Same as go_complex_angle, but divided by pi (which occasionally produces
 * nice round numbers not suffering from rounding errors).
 */

DOUBLE SUFFIX(go_complex_angle_pi) (COMPLEX const *src)
{
	return SUFFIX(go_atan2pi) (src->im, src->re);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_conj) (COMPLEX *dst, COMPLEX const *src)
{
	SUFFIX(go_complex_init) (dst, src->re, -src->im);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_scale_real) (COMPLEX *dst, DOUBLE f)
{
	dst->re *= f;
	dst->im *= f;
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_add) (COMPLEX *dst, COMPLEX const *a, COMPLEX const *b)
{
	SUFFIX(go_complex_init) (dst, a->re + b->re, a->im + b->im);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_sub) (COMPLEX *dst, COMPLEX const *a, COMPLEX const *b)
{
	SUFFIX(go_complex_init) (dst, a->re - b->re, a->im - b->im);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_exp) (COMPLEX *dst, COMPLEX const *src)
{
	SUFFIX(go_complex_from_polar) (dst, SUFFIX(exp) (src->re), src->im);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_ln) (COMPLEX *dst, COMPLEX const *src)
{
	DOUBLE x = SUFFIX(fabs) (src->re);
	DOUBLE y = SUFFIX(fabs) (src->im);
	DOUBLE lm = x > y
		? SUFFIX(log) (x) + CONST(0.5) * SUFFIX(log1p) ((y / x) * (y / x))
		: SUFFIX(log) (y) + CONST(0.5) * SUFFIX(log1p) ((x / y) * (x / y));
	DOUBLE a = SUFFIX(go_complex_angle) (src);

	SUFFIX(go_complex_init) (dst, lm, a);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_sin) (COMPLEX *dst, COMPLEX const *src)
{
	SUFFIX(go_complex_init) (dst,
		SUFFIX(sin) (src->re) * SUFFIX(cosh) (src->im),
		SUFFIX(cos) (src->re) * SUFFIX(sinh) (src->im));
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_cos) (COMPLEX *dst, COMPLEX const *src)
{
	SUFFIX(go_complex_init) (dst,
		SUFFIX(cos) (src->re) * SUFFIX(cosh) (src->im),
		-SUFFIX(sin) (src->re) * SUFFIX(sinh) (src->im));
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_tan) (COMPLEX *dst, COMPLEX const *src)
{
	DOUBLE R = src->re, I = src->im;
	DOUBLE sr = SUFFIX(sin) (R);
	DOUBLE cr = SUFFIX(cos) (R);

	// Code here inspired by gsl, but doesn't follow it.

	if (SUFFIX(fabs) (I) < 1) {
		DOUBLE shi = SUFFIX(sinh) (I);
		DOUBLE D = cr * cr + shi * shi;

		SUFFIX(go_complex_init) (dst,
					 sr * cr / D,
					 CONST(0.5) * SUFFIX(sinh) (2 * I) / D);
	} else {
		DOUBLE u = SUFFIX(exp) (-SUFFIX(fabs)(I));
		DOUBLE C = 2 * u / (1 - u * u);
		DOUBLE S = C * C;
		DOUBLE D = 1 + cr * cr * S;
		DOUBLE T = 1 / SUFFIX(tanh) (SUFFIX(fabs) (I));
		SUFFIX(go_complex_init) (dst,
					 sr * cr * S / D,
					 (I < 0 ? -T : T) / D);
	}
}

/* ------------------------------------------------------------------------- */

// See comments at top
#endif // SKIP_THIS_PASS
#if INCLUDE_PASS < INCLUDE_PASS_LAST
#include __FILE__
#endif
