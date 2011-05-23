/*
 * complex.c:  A quick library for complex math.
 *
 * Author:
 *  Morten Welinder <terra@gnome.org>
 *  Jukka-Pekka Iivonen <iivonen@iki.fi>
 */

#include <goffice-config.h>
#define GOFFICE_COMPLEX_IMPLEMENTATION
#include "go-complex.h"

#include <stdlib.h>
#include <errno.h>


#ifndef DOUBLE

#define DOUBLE double
#define SUFFIX(_n) _n
#define M_PIgo    3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117
#define go_strto go_strtod
#define GO_const(_n) _n

#ifdef GOFFICE_WITH_LONG_DOUBLE
#include "go-complex.c"
#undef DOUBLE
#undef SUFFIX
#undef M_PIgo
#undef go_strto
#undef GO_const

#ifdef HAVE_SUNMATH_H
#include <sunmath.h>
#endif
#define DOUBLE long double
#define SUFFIX(_n) _n ## l
#define M_PIgo    3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117L
#define go_strto go_strtold
#define GO_const(_n) _n ## L
#endif

#endif

/* ------------------------------------------------------------------------- */

char *
SUFFIX(go_complex_to_string) (SUFFIX(go_complex) const *src, char const *reformat,
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
SUFFIX(go_complex_from_string) (SUFFIX(go_complex) *dst, char const *src, char *imunit)
{
	DOUBLE x, y;
	char *end;

	/* Case: "i", "+i", "-i", ...  */
	if (SUFFIX(is_unit_imaginary) (src, &dst->im, imunit)) {
		dst->re = 0;
		return 0;
	}

	x = go_strto (src, &end);
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

	y = go_strto (src, &end);
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
SUFFIX(go_complex_to_polar) (DOUBLE *mod, DOUBLE *angle, SUFFIX(go_complex) const *src)
{
	*mod = SUFFIX(go_complex_mod) (src);
	*angle = SUFFIX(go_complex_angle) (src);
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_from_polar) (SUFFIX(go_complex) *dst, DOUBLE mod, DOUBLE angle)
{
	SUFFIX(go_complex_init) (dst, mod * SUFFIX(cos) (angle), mod * SUFFIX(sin) (angle));
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_mul) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *a, SUFFIX(go_complex) const *b)
{
	SUFFIX(go_complex_init) (dst,
		      a->re * b->re - a->im * b->im,
		      a->re * b->im + a->im * b->re);
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_div) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *a, SUFFIX(go_complex) const *b)
{
	DOUBLE bmod = SUFFIX(go_complex_mod) (b);

	if (bmod >= GO_const(1e10)) {
		/* Ok, it's big.  */
		DOUBLE a_re = a->re / bmod;
		DOUBLE a_im = a->im / bmod;
		DOUBLE b_re = b->re / bmod;
		DOUBLE b_im = b->im / bmod;
		SUFFIX(go_complex_init) (dst,
			      a_re * b_re + a_im * b_im,
			      a_im * b_re - a_re * b_im);
	} else {
		DOUBLE bmodsqr = bmod * bmod;
		SUFFIX(go_complex_init) (dst,
			      (a->re * b->re + a->im * b->im) / bmodsqr,
			      (a->im * b->re - a->re * b->im) / bmodsqr);
	}
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_sqrt) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *src)
{
	if (SUFFIX(go_complex_real_p) (src)) {
		if (src->re >= 0)
			SUFFIX(go_complex_init) (dst, SUFFIX(sqrt) (src->re), 0);
		else
			SUFFIX(go_complex_init) (dst, 0, SUFFIX(sqrt) (-src->re));
	} else
		SUFFIX(go_complex_from_polar) (dst,
				    SUFFIX(sqrt) (SUFFIX(go_complex_mod) (src)),
				    SUFFIX(go_complex_angle) (src) / 2);
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_complex_pow) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *a, SUFFIX(go_complex) const *b)
{
	if (SUFFIX(go_complex_zero_p) (a) && SUFFIX(go_complex_real_p) (b)) {
		if (b->re <= 0)
			SUFFIX(go_complex_invalid) (dst);
		else
			SUFFIX(go_complex_real) (dst, 0);
	} else {
		DOUBLE res_r, res_a1, res_a2, res_a2_pi, r, arg;
		SUFFIX(go_complex) F;

		SUFFIX(go_complex_to_polar) (&r, &arg, a);
		res_r = SUFFIX(pow) (r, b->re) * SUFFIX(exp) (-b->im * arg);
		res_a1 = b->im * SUFFIX(log) (r);
		res_a2 = b->re * arg;
		res_a2_pi = b->re * SUFFIX(go_complex_angle_pi) (a);

		res_a2_pi = SUFFIX(fmod) (res_a2_pi, 2);
		if (res_a2_pi < 0) res_a2_pi += 2;

		/*
		 * Problem: sometimes res_a2 is a nice fraction of pi.
		 * Actually adding it will introduce pointless rounding
		 * errors.
		 */
		if (res_a2_pi == 0.5) {
			res_a2 = 0;
			SUFFIX(go_complex_init) (&F, 0, 1);
		} else if (res_a2_pi == 1) {
			res_a2 = 0;
			SUFFIX(go_complex_real) (&F, -1);
		} else if (res_a2_pi == 1.5) {
			res_a2 = 0;
			SUFFIX(go_complex_init) (&F, 0, -1);
		} else
			SUFFIX(go_complex_real) (&F, 1);

		SUFFIX(go_complex_from_polar) (dst, res_r, res_a1 + res_a2);
		SUFFIX(go_complex_mul) (dst, dst, &F);
	}
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_init) (SUFFIX(go_complex) *dst, DOUBLE re, DOUBLE im)
{
	dst->re = re;
	dst->im = im;
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_invalid) (SUFFIX(go_complex) *dst)
{
	dst->re = SUFFIX(go_nan);
	dst->im = SUFFIX(go_nan);
}

void SUFFIX(go_complex_real) (SUFFIX(go_complex) *dst, DOUBLE re)
{
	dst->re = re;
	dst->im = 0;
}

/* ------------------------------------------------------------------------- */

int SUFFIX(go_complex_real_p) (SUFFIX(go_complex) const *src)
{
	return src->im == 0;
}

/* ------------------------------------------------------------------------- */

int SUFFIX(go_complex_zero_p) (SUFFIX(go_complex) const *src)
{
	return src->re == 0 && src->im == 0;
}

/* ------------------------------------------------------------------------- */

int SUFFIX(go_complex_invalid_p) (SUFFIX(go_complex) const *src)
{
	return !(SUFFIX(go_finite) (src->re) && SUFFIX(go_finite) (src->im));
}

/* ------------------------------------------------------------------------- */

DOUBLE SUFFIX(go_complex_mod) (SUFFIX(go_complex) const *src)
{
	return SUFFIX(hypot) (src->re, src->im);
}

/* ------------------------------------------------------------------------- */

DOUBLE SUFFIX(go_complex_angle) (SUFFIX(go_complex) const *src)
{
	return SUFFIX(atan2) (src->im, src->re);
}

/* ------------------------------------------------------------------------- */
/*
 * Same as go_complex_angle, but divided by pi (which occasionally produces
 * nice round numbers not suffering from rounding errors).
 */

DOUBLE SUFFIX(go_complex_angle_pi) (SUFFIX(go_complex) const *src)
{
	if (src->im == 0)
		return (src->re >= 0 ? 0 : -1);

	if (src->re == 0)
		return (src->im >= 0 ? 0.5 : -0.5);

	/* We could do quarters too */

	/* Fallback.  */
	return SUFFIX(go_complex_angle) (src) / M_PIgo;
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_conj) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *src)
{
	SUFFIX(go_complex_init) (dst, src->re, -src->im);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_scale_real) (SUFFIX(go_complex) *dst, DOUBLE f)
{
	dst->re *= f;
	dst->im *= f;
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_add) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *a, SUFFIX(go_complex) const *b)
{
	SUFFIX(go_complex_init) (dst, a->re + b->re, a->im + b->im);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_sub) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *a, SUFFIX(go_complex) const *b)
{
	SUFFIX(go_complex_init) (dst, a->re - b->re, a->im - b->im);
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_exp) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *src)
{
	SUFFIX(go_complex_init) (dst,
		SUFFIX(exp) (src->re) * SUFFIX(cos) (src->im),
		SUFFIX(exp) (src->re) * SUFFIX(sin) (src->im));
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_ln) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *src)
{
	SUFFIX(go_complex_init) (dst,
		SUFFIX(log) (SUFFIX(go_complex_mod) (src)),
		SUFFIX(go_complex_angle) (src));
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_sin) ( SUFFIX(go_complex) *dst,  SUFFIX(go_complex) const *src)
{
	SUFFIX(go_complex_init) (dst,
		SUFFIX(sin) (src->re) * SUFFIX(cosh) (src->im),
		SUFFIX(cos) (src->re) * SUFFIX(sinh) (src->im));
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_cos) (SUFFIX(go_complex) *dst,  SUFFIX(go_complex) const *src)
{
	SUFFIX(go_complex_init) (dst,
		SUFFIX(cos) (src->re) * SUFFIX(cosh) (src->im),
		-SUFFIX(sin) (src->re) * SUFFIX(sinh) (src->im));
}

/* ------------------------------------------------------------------------- */

void SUFFIX(go_complex_tan) (SUFFIX(go_complex) *dst, SUFFIX(go_complex) const *src)
{
	SUFFIX(go_complex) s, c;

	SUFFIX(go_complex_sin) (&s, src);
	SUFFIX(go_complex_cos) (&c, src);
	SUFFIX(go_complex_div) (dst, &s, &c);
}
