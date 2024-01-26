/*
 * go-decimal.c:  Support for Decimal64 numbers
 *
 * Authors
 *   Morten Welinder <terra@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Note: this file is deliberately "lgpl 2 or later", not just "2 or 3".

// This file contains
// * libm functions for the _Decimal64 type using a "D" suffix
//   - some have proper implementation
//   - some have stubs that just defer to "double" versions
// * printf hooks for _Decimal64 and _Decimal128
//   - no intl support yet
//   - no 'a' and 'A' formats
//   - no grouping flag (single quote)
//   - THIS IS INCOMPATIBLE WITH THE "quadmath" LIBRARY due to glibc
//     deficiency.

// Implementation status
//
// FUNCTION        RANGE     ACCURACY  TESTING
// -------------------------------------------
// acosD           A         *         *
// acoshD          A         *         *
// asinD           A         *         *
// asinhD          A         *         *
// atanD           A         *         *
// atan2D          A         *         B
// atanhD          A         A-        *
// cbrtD           A         A-        *
// ceilD           A         A         A
// copysignD       A         A         A
// cosD            B         C*        *
// coshD           A         A-        *
// erfD            A         *         *
// erfcD           A         *         *
// expD            A         *         *
// expm1D          A         *         *
// fabsD           A         A         *
// floorD          A         A         A
// frexpD          A         B         -
// fmodD           A         A         *
// hypotD          A         A         -
// jnD             B         C*        -
// ldexpD          B         B         -
// lgammaD         A         A-        *
// lgammaD_r       A         A-        -
// log10D          A         A-        *
// log2D           A         A-        *
// log1pD          A         A-        *
// logD            A         A-        *
// modfD           A         A         A
// nextafterD      A         A         A
// powD            B         *         B
// roundD          A         A         A
// sinD            B         C*        *
// sinhD           A         A-        *
// scalbln         A         A         A
// scalbn          A         A         A
// sqrtD           A         A-        *
// tanD            B         C*        *
// tanhD           A         A-        *
// truncD          A         A         A
// ynD             *         C*        -
// finiteD         A         A         A
// isnanD          A         A         A
// signbitD        A         A         A
// strtoDd         A         A         B
// (printf)        A         A         -
//
// * Stub via double.  Range:B, Accuracy:B

#include <math/go-decimal.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <printf.h>
#include <assert.h>
#include <ctype.h>
#include <locale.h>

// ---------------------------------------------------------------------------

#define DECIMAL64_BIAS (-398)
#define DECIMAL64_MAX_BIASED_EXP 369
#define DECIMAL64_MIN_DEN 1e-398dd
#define DECIMAL64_MAX_MANT 9999999999999999ull

#define DECIMAL128_BIAS (-6176)

#define M_LN10D  2.3025850929940456840179914546843642076dd // log(10)
#define M_LG10D  3.32192809488736235dd                     // log_2(10)
#define M_LN2D   0.6931471805599453094dd                   // log(2)
#define M_SQRT2D 1.414213562373095dd                       // sqrt(2)

// We assume bis format (and check for it during init)
#define decode64 decode64_bis
#define make64 make64_bis
#define decode128 decode128_bis

enum {
	CLS_NORMAL,
	CLS_INVALID,  // Treat as zero
	CLS_NAN,
	CLS_INF
};

// ---------------------------------------------------------------------------

static char *decimal_point_str;

static const char *
decimal_point (void)
{
	struct lconv *lc = localeconv ();

	if (lc->decimal_point == NULL || lc->decimal_point[0] == 0)
		return ".";

	g_free (decimal_point_str);
	decimal_point_str = g_locale_to_utf8 (lc->decimal_point, -1,
					      NULL, NULL, NULL);
	if (decimal_point_str == NULL || decimal_point_str[0] == 0)
		return ".";

	return decimal_point_str;
}


// ---------------------------------------------------------------------------

// Decode a _Decimal64 assuming binary integer significant encoding
static inline int
decode64_bis (_Decimal64 const *args0, uint64_t *pmant, int *pp10, int *sign)
{
	uint64_t d64, mant;
	int special = CLS_NORMAL, p10 = 0;

	memcpy (&d64, args0, sizeof(d64));

	if (sign) *sign = (d64 >> 63);

	if (((d64 >> 59) & 15) == 15) {
		special = ((d64 >> 58) & 1) ? CLS_NAN : CLS_INF;
	} else {
		if (((d64 >> 61) & 3) == 3) {
			p10 = DECIMAL64_BIAS + ((d64 >> 51) & 0x3ff);
			mant = (d64 & ((1ul << 51) - 1)) | (4ul << 51);
		} else {
			p10 = DECIMAL64_BIAS + ((d64 >> 53) & 0x3ff);
			mant = d64 & ((1ul << 53) - 1);
		}
		if (mant > DECIMAL64_MAX_MANT)
			special = CLS_INVALID; // Invalid (>= 10^16)
	}

	if (pp10) *pp10 = (special ? 0 : p10);
	if (pmant) *pmant = (special ? 0 : mant);

	return special;
}

// Encode a (finite) _Decimal64 assuming binary integer significant encoding
static _Decimal64
make64_bis (uint64_t mant, int e, int sign)
{
	uint64_t ue, u64;
	_Decimal64 res;

	assert (mant <= DECIMAL64_MAX_MANT);
	assert (e >= DECIMAL64_BIAS && e <= DECIMAL64_MAX_BIASED_EXP);

	ue = e - DECIMAL64_BIAS;
	if (mant & ((uint64_t)1 << 53)) {
		u64 = (mant ^ ((uint64_t)1 << 53)) |
			(ue << 51) |
			((uint64_t)3 << 61) |
			((uint64_t)sign << 63);
	} else {
		u64 = mant | (ue << 53) | ((uint64_t)sign << 63);
	}

	memcpy (&res, &u64, sizeof (res));
	return res;
}

// Decode a _Decimal128 assuming binary integer significant encoding
static int
decode128_bis (_Decimal128 const *args0, uint64_t *pmantu, uint64_t *pmantl,
	       int *pp10, int *sign)
{
	uint64_t l64, u64, mantl, mantu;
	int special = CLS_NORMAL, p10 = 0;

	// Hmm...  Little endian, I hope
	memcpy (&l64, (uint64_t const *)args0, sizeof(l64));
	memcpy (&u64, (uint64_t const *)args0 + 1, sizeof(u64));

	if (sign) *sign = (u64 >> 63);

	if (((u64 >> 59) & 15) == 15) {
		special = ((u64 >> 58) & 1) ? CLS_NAN : CLS_INF;
	} else {
		if (((u64 >> 61) & 3) == 3) {
			p10 = DECIMAL128_BIAS + ((u64 >> 47) & 0x3fff);
			mantu = (u64 & ((1ul << 47) - 1)) | (4ul << 47);
		} else {
			p10 = DECIMAL128_BIAS + ((u64 >> 49) & 0x3fff);
			mantu = u64 & ((1ul << 49) - 1);
		}
		mantl = l64;
		if (mantu > 0x1ed09bead87c0ull ||
		    (mantu == 0x1ed09bead87c0ull &&
		     mantl > 0x378d8e63ffffffffull))
			special = CLS_INVALID; // Invalid (>= 10^34)
	}

	if (pp10) *pp10 = special ? 0 : p10;
	if (pmantu) *pmantu = special ? 0 : mantu;
	if (pmantl) *pmantl = special ? 0 : mantl;

	return special;
}

static void
render128 (char *buffer, uint64_t u, uint64_t l)
{
	char tmp[8 * sizeof (u)];
	char *p = tmp + sizeof (tmp);
	int len;
	const uint64_t Md10 = (uint64_t)-1 / 10; // 1844674407370955161
	const unsigned Mr10 = (uint64_t)-1 % 10 + 1; // 6

	while (u) {
		unsigned u2 = u % 10;
		unsigned l2 = l % 10;
		unsigned d = (Mr10 * u2 + l2) % 10;
		uint64_t v1 = (u2 * Md10) + (Mr10 * u2 + l2) / 10;

		u /= 10;
		l /= 10;
		l += v1; if (l < v1) u++;

		*--p = '0' + d;
	}

	while (l >= 10) {
		int d = l % 10;
		*--p = '0' + d;
		l /= 10;
	}

	*--p = '0' + l;

	len = tmp + sizeof (tmp) - p;
	memcpy (buffer, p, len);
	buffer[len] = 0;
}

static const uint64_t
u64_pow10_table[20] = {
	1ull,
	10ull,
	100ull,
	1000ull,
	10000ull,
	100000ull,
	1000000ull,
	10000000ull,
	100000000ull,
	1000000000ull,
	10000000000ull,
	100000000000ull,
	1000000000000ull,
	10000000000000ull,
	100000000000000ull,
	1000000000000000ull,
	10000000000000000ull,
	100000000000000000ull,
	1000000000000000000ull,
	10000000000000000000ull,
};


static int
u64_digits (uint64_t x)
{
	int l2, l10;

	if (x == 0)
		return 1;

	assert (sizeof (long) == sizeof (uint64_t));
	l2 = 63 - __builtin_clzl (x);
	// log_10(2) is a hair bigger than 77/256
	l10 = l2 * 77 / 256;

	if (x >= u64_pow10_table[l10 + 1])
		l10++;

	return l10 + 1;
}


static int decimal64_modifier, decimal128_modifier;
static int decimal64_type, decimal128_type;

static void
decimal64_va_arg (void *mem, va_list *ap)
{
	_Decimal64 d = va_arg(*ap, _Decimal64);
	memcpy (mem, &d, sizeof(d));
}

static void
decimal128_va_arg (void *mem, va_list *ap)
{
	_Decimal128 d = va_arg(*ap, _Decimal128);
	memcpy (mem, &d, sizeof(d));
}

static int
decimal_arginfo (const struct printf_info *info, size_t n,
		 int *argtypes, int *size)
{
	if (n > 0) {
		if (info->user & decimal64_modifier) {
			argtypes[0] = decimal64_type;
			size[0] = sizeof (_Decimal64);
			return 1;
		}
		if (info->user & decimal128_modifier) {
			argtypes[0] = decimal128_type;
			size[0] = sizeof (_Decimal128);
			return 1;
		}
	}

	return -1;
}

// Round decimal number in buf based on character in position ix.
// '0'..'4' round down, '5'..'9' round up.
//
// Returns new length which is normally ix, except
// * When ix is <= 0: 1 is returned, buffer is "0"
// * when rounding 9999|5 up getting 10000.
static int
do_round (char *buf, int ix, int *qoverflow)
{
	*qoverflow = 0;
	if (ix < 0) {
		ix = 0;
	} else {
		char rc = buf[ix];

		if (rc >= '5') {
			int i = ix - 1;
			while (i >= 0 && (buf[i])++ == '9') {
				buf[i] = '0';
				i--;
			}
			if (i == -1) {
				buf[ix++] = '0';
				buf[0] = '1';
				*qoverflow = 1;
			}
		}
	}

	if (ix == 0)
		buf[ix++] = '0';
	return ix;
}

static int
decimal_format (FILE *stream, const struct printf_info *info,
		const void *const *args)
{
	char buffer[1024];
	int special, p10, sign;
	int len = 0;
	int qupper = (info->spec <= 'Z');
	char signchar;
	const char *dot = decimal_point ();
	int dotlen = strlen (dot);

	if (info->user & decimal64_modifier) {
		_Decimal64 const *args0 = *(_Decimal64 **)(args[0]);
		uint64_t mant;
		special = decode64 (args0, &mant, &p10, &sign);
		if (special == CLS_NORMAL) render128 (buffer, 0, mant);
	} else if (info->user & decimal128_modifier) {
		_Decimal128 const *args0 = *(_Decimal128 **)(args[0]);
		uint64_t mantu, mantl;
		special = decode128 (args0, &mantu, &mantl, &p10, &sign);
		if (special == CLS_NORMAL) render128 (buffer, mantu, mantl);
	} else {
		// Not sure why this gets called on a regular "%f".  The special
		// return value of -2 to use default handler is not documented.
		return -2;
	}

	if (sign)
		signchar = '-';
	else if (info->showsign)
		signchar = '+';
	else if (info->space)
		signchar = ' ';
	else
		signchar = 0;

	switch (special) {
	case CLS_NORMAL: {
		int estyle, fstyle, gstyle, prec;

		len = strlen (buffer);

		// We don't want a zero with scaling
		if (len == 1 && buffer[0] == '0')
			p10 = 0;

		// Default is 6.  Avoid buffer overflow.
		prec = info->prec;
		if (prec < 0) prec = 6;
		if (prec > 100) prec = 100;

		estyle = ((info->spec | 32) == 'e');
		fstyle = ((info->spec | 32) == 'f');
		gstyle = ((info->spec | 32) == 'g');

		if (gstyle) {
			int effp10 = p10 + (len - 1);
			if (prec == 0) prec = 1;
			if (effp10 < -4 || effp10 >= prec) {
				estyle = 1;
				prec--;
			} else {
				fstyle = 1;
				prec -= effp10 + 1;
			}
		}

		overflow_to_estyle:
		if (estyle) {
			int decimals = len - 1, ap10;
			p10 += decimals;

			if (decimals > prec) {
				int cut = decimals - prec;
				int qoverflow;
				len = do_round (buffer, len - cut, &qoverflow);
				if (qoverflow) {
					// We overflowed 9999 into 10000
					len--;
					p10++;
				}
			} else if (decimals < prec && !gstyle) {
				int diff = prec - decimals;
				memset (buffer + len, '0', diff);
				len += diff;
			}

			// For gstyle we should not have 0 at the end of the fractional part
			while (gstyle && len > 1 && buffer[len - 1] == '0')
				len--;

			if (prec && (len > 1 || info->alt)) {
				memmove (buffer + 1 + dotlen,
					 buffer + 1, len - 1);
				memcpy (buffer + 1, dot, dotlen);
				len += dotlen;
			}

			buffer[len++] = (qupper ? 'E' : 'e');
			buffer[len++] = (p10 >= 0 ? '+' : '-');
			ap10 = (p10 >= 0 ? p10 : -p10);
			if (ap10 <= 9) buffer[len++] = '0';
			render128 (buffer + len, 0, ap10);
			len += strlen (buffer + len);
		} else if (fstyle) {
			int decimals = 0;

			while (p10 < 0 && len > 1 && buffer[len - 1] == '0') {
				buffer[--len] = 0;
				p10++;
			}
			if (p10 < 0) decimals = -p10;
			if (decimals > prec) {
				int cut = decimals - prec;
				int qoverflow;
				len = do_round (buffer, len - cut, &qoverflow);
				decimals = prec;
				p10 += cut;
				if (qoverflow) {
					//p10++;
					if (gstyle && decimals == 0) {
						estyle = 1;
						fstyle = 0;
						goto overflow_to_estyle;
					}
				}
			}
			// For gstyle we should not have 0 at the end of the fractional part
			while (gstyle && len > 1 && p10 < 0 && buffer[len - 1] == '0')
				len--, p10++, decimals--;

			// Add 0 for large integers
			while (p10 > 0) {
				buffer[len++] = '0';
				p10--;
			}
			// Add 0 for extra decimals
			while (decimals < prec && !gstyle) {
				buffer[len++] = '0';
				decimals++;
			}

			if (decimals > len) {
				int diff = decimals - len;
				memmove (buffer + diff + 1 + dotlen,
					 buffer, len);
				memset (buffer, '0', diff + 1 + dotlen);
				memcpy (buffer + 1, dot, dotlen);
				len += diff + 1 + dotlen;
			} else if (decimals > 0 || info->alt) {
				int need0 = (decimals == len);
				memmove (buffer + len - decimals + dotlen + need0,
					 buffer + len - decimals,
					 decimals);
				if (need0) buffer[0] = '0';
				memcpy (buffer + len + need0 - decimals,
					dot, dotlen);
				len += dotlen + need0;
			}
			buffer[len] = 0;
		} else {
			// Shouldn't happen
			buffer[0] = '?';
			len = 1;
		}
		break;
	}
	case CLS_NAN:
		strcpy (buffer, (qupper ? "NAN" : "nan"));
		len = 3;
		break;
	case CLS_INF:
		strcpy (buffer, (qupper ? "INF" : "inf"));
		len = 3;
		break;
	case CLS_INVALID:
		buffer[0] = '0';
		len = 1;
		p10 = 0;
		break;
	}

	if (signchar) len++;

	while (!info->left && len < info->width) {
		len++;
		putc (info->pad, stream);
	}
	if (signchar) putc (signchar, stream);
	fputs (buffer, stream);
	while (len < info->width) {
		len++;
		putc (' ', stream);
	}
	return len;
}

static void
init_decimal_printf_support (void)
{
	decimal64_type = register_printf_type (decimal64_va_arg);
	decimal128_type = register_printf_type (decimal128_va_arg);

#define CAT(x,y) x ## y
#define WSTR(x) CAT(L,x)
	decimal64_modifier = register_printf_modifier (WSTR (GO_DECIMAL64_MODIFIER));
	decimal128_modifier = register_printf_modifier (WSTR (GO_DECIMAL128_MODIFIER));
#undef WSTR
#undef CAT

	register_printf_specifier ('f', decimal_format, decimal_arginfo);
	register_printf_specifier ('F', decimal_format, decimal_arginfo);
	register_printf_specifier ('e', decimal_format, decimal_arginfo);
	register_printf_specifier ('E', decimal_format, decimal_arginfo);
	register_printf_specifier ('g', decimal_format, decimal_arginfo);
	register_printf_specifier ('G', decimal_format, decimal_arginfo);
	// No 'a' and 'A'
}

// ---------------------------------------------------------------------------

inline int
isnanD (_Decimal64 x)
{
	return decode64 (&x, NULL, NULL, NULL) == CLS_NAN;
}

inline int
finiteD (_Decimal64 x)
{
	return decode64 (&x, NULL, NULL, NULL) < CLS_NAN;
}

inline int
signbitD (_Decimal64 x)
{
	int sign;
	(void)decode64 (&x, NULL, NULL, &sign);
	return sign;
}

_Decimal64
copysignD (_Decimal64 x, _Decimal64 y)
{
	if (signbitD (x) == signbitD (y))
		return x;
	else
		return -x;
}

_Decimal64
fabsD (_Decimal64 x)
{
	return signbitD (x) ? -x : x;
}

static _Decimal64
pow10D (int e)
{
	if (e < DECIMAL64_BIAS)
		return 0;
	if (e <= DECIMAL64_MAX_BIASED_EXP)
		return make64 (1, e, 0);
	if (e <= DECIMAL64_MAX_EXP)
		return make64 (u64_pow10_table[e - DECIMAL64_MAX_BIASED_EXP],
			       DECIMAL64_MAX_BIASED_EXP, 0);
	return (_Decimal64)INFINITY;
}

_Decimal64
nextafterD (_Decimal64 x, _Decimal64 y)
{
	int qadd, qeffadd, e, lm64, sign, special, sn;
	uint64_t m64;

	special = decode64 (&x, &m64, &e, &sign);
	if (special == CLS_NAN)
		return x;

	if (x < y)
		qadd = 1;
	else if (x > y)
		qadd = 0;
	else
		return y; // Either equal or y is NAN

	if (special == CLS_INF)
		return copysignD (DECIMAL64_MAX, x);

	if (m64 == 0) {
		_Decimal64 eps = DECIMAL64_MIN_DEN;
		return qadd ? eps : -eps;
	}

	// Scale mantissa as far up as we can, ie., to 16 digits unless that
	// would cause the exponent to become too small.
	lm64 = u64_digits (m64);
	sn = MIN (DECIMAL64_DIG - lm64, e - DECIMAL64_BIAS);
	m64 *= u64_pow10_table[sn];
	e -= sn;

	qeffadd = sign != qadd;
	if (qeffadd) {
		m64++;
		if (m64 == DECIMAL64_MAX_MANT + 1) {
			_Decimal64 r = pow10D (e + DECIMAL64_DIG);
			return sign ? -r : r;
		}
	} else {
		m64--;
		if (m64 == DECIMAL64_MAX_MANT / 10 && e != DECIMAL64_BIAS) {
			m64 = DECIMAL64_MAX_MANT;
			e--;
		}
	}

	return make64 (m64, e, sign);
}

// NOTE: THIS IS NOT A LOSSLESS OPERATION
_Decimal64
ldexpD (_Decimal64 x, int e)
{
	if (x == 0 || !finiteD (x))
		return x;

	if (e > 1023) {
		// Note: log2(DECIMAL64_MAX / DBL_MAX) =~ 252
		return x * (_Decimal64)ldexp(1, 300) *
			(_Decimal64)ldexp(1, e - 300);
	} else if (e < -1023) {
		return x * (_Decimal64)ldexp(1, -300) *
			(_Decimal64)ldexp(1, e + 300);
	} else
		return x * (_Decimal64)(ldexp(1, e));
}

// NOTE: THIS IS NOT A LOSSLESS OPERATION
_Decimal64
frexpD (_Decimal64 x, int *e)
{
	_Decimal64 m, ax;

	if (x == 0 || !finiteD (x)) {
		*e = 0;
		return x;
	}

	ax = fabsD (x);

	if (ax >= (_Decimal64)DBL_MAX) {
		_Decimal64 p_2_300 = ldexp (1, 300);
		m = frexpD (ax / p_2_300, e);
		*e += 300;
	} else if (ax <= (_Decimal64)DBL_MIN) {
		_Decimal64 p_2_300 = ldexp (1, 300);
		m = frexpD (ax * p_2_300, e);
		*e -= 300;
	} else
		m = frexp (ax, e);

	return copysignD (m, x);
}

// This is lossless (expect when going denormal or underflowing).
_Decimal64
scalblnD (_Decimal64 x, long e)
{
	uint64_t mant;
	int p10, sign;
	int too_far = (DECIMAL64_MAX_EXP - DECIMAL64_MIN_EXP) + DECIMAL64_DIG;

	if (decode64 (&x, &mant, &p10, &sign) || mant == 0)
		return x;

	p10 += CLAMP (e, -too_far, +too_far);
	if (p10 > DECIMAL64_MAX_BIASED_EXP) {
		int excess = p10 - DECIMAL64_MAX_BIASED_EXP;
		int lmant = excess >= DECIMAL64_DIG ? DECIMAL64_DIG : u64_digits (mant);
		if (lmant + excess > DECIMAL64_DIG)
			return sign ? -(_Decimal64)INFINITY : (_Decimal64)INFINITY;
		p10 = DECIMAL64_MAX_BIASED_EXP;
		mant *= u64_pow10_table[excess];
	} else if (p10 < DECIMAL64_BIAS) {
		int deficit = DECIMAL64_BIAS - p10;
		if (deficit > DECIMAL64_DIG) // Strict ">" since to rounding can bring back 1
			return sign ? -0.dd : 0.dd;
		else {
			uint64_t f = u64_pow10_table[deficit];
			// Note: rounding ties away from zero
			mant = (mant + f / 2) / f;
			p10 = mant ? DECIMAL64_BIAS : 0;
		}
	}

	return make64 (mant, p10, sign);
}

_Decimal64
scalbnD (_Decimal64 x, int e)
{
	return scalblnD (x, e);
}

_Decimal64
unscalbnD (_Decimal64 x, int *e)
{
	int special, sign, p10, m10;
	uint64_t mant;

	special = decode64 (&x, &mant, &p10, &sign);
	switch (special) {
	case CLS_INF:
	case CLS_NAN:
		*e = 0;
		return x;
	default:
		break;
	}

	if (mant == 0) {
		*e = 0;
		return sign ? -0.dd : 0.dd;
	}

	m10 = u64_digits (mant);
	p10 += m10;
	*e = p10;
	return make64 (mant, -m10, sign);
}

static int
caseprefix (const unsigned char *us, const char *p)
{
	while (*p) {
		if (*p != toupper (*us))
			return 0;
		p++;
		us++;
	}
	return 1;
}

_Decimal64
strtoDd (const char *s, char **end)
{
	uint64_t m = 0;
	int sign = 0;
	const unsigned char *us = (const unsigned char *)s;
	int digits = 0;
	int period = 0;
	int scale = 0;
	_Decimal64 res;
	const char *dot = decimal_point ();

	while (isspace (*us))
		us++;

	if (*us == '-')
		sign++, us++;
	else if (*us == '+')
		us++;

	if (!isdigit (*us) && !(g_str_has_prefix (us, dot) && isdigit (us[1]))) {
		if (caseprefix (us, "INFINITY"))
			res = INFINITY, us += 8;
		else if (caseprefix (us, "INF"))
			res = INFINITY, us += 3;
		else if (caseprefix (us, "NAN"))
			res = NAN, us += 3;
		else {
			if (end) *end = (char *)s;
			return 0;
		}

		if (end) *end = (char *)us;
		return sign ? -res : res;
	}

	while (isdigit (*us) || g_str_has_prefix (us, dot)) {
		if (g_str_has_prefix (us, dot)) {
			if (period)
				break;
			period = 1;
		} else {
			if (digits < DECIMAL64_DIG) {
				m = 10 * m + (*us - '0');
				if (m) digits++;
				if (period) scale--;
			} else if (digits == DECIMAL64_DIG) {
				if (*us >= '5')
					m++; // Always round away from 0
				if (!period) scale++;
				digits++;
			} else {
				if (!period) scale++;
			}
		}
		us++;
	}

	if (*us == 'e' || *us == 'E') {
		int esign = 0;
		int p10 = 0;

		if (us[1] == '-' && isdigit(us[2]))
			us += 2, esign = 1;
		else if (us[1] == '+' && isdigit(us[2]))
			us += 2;
		else if (isdigit (us[1]))
			us++;

		while (isdigit (*us)) {
			if (p10 < INT_MAX / 10 - 10)
				p10 = p10 * 10 + (*us - '0');
			us++;
		}

		scale += esign ? -p10 : p10;
	}

	if (end) *end = (char *)us;
	res = scalbnD (m, scale);
	if (sign) res = -res;
	return res;
}

// ---------------------------------------------------------------------------

_Decimal64
floorD (_Decimal64 x)
{
	if (x < 0)
		return -ceilD (-x);
	if (x > 0) {
		if (x < 1e15dd) {
			_Decimal64 y = x - 0.5dd;
			_Decimal64 C = 1e16dd - x;
			_Decimal64 s = C + y;
			return s - C;
		} else
			return x;
	} else
		return x;
}

_Decimal64
ceilD (_Decimal64 x)
{
	if (x < 0)
		return -floorD (-x);
	if (x > 0) {
		_Decimal64 f = floorD (x);
		return x == f ? f : f + 1;
	} else
		return x;
}

_Decimal64
roundD (_Decimal64 x)
{
	_Decimal64 const C = 1e15dd;
	if (x < 0)
		return -roundD (-x);
	if (x > 0 && x < C) {
		_Decimal64 s = C + x;
		_Decimal64 r = s - C;
		if (r - x == -0.5dd)
			r += 1; // We don't want round-ties-to-even
		return r;
	} else
		return x;
}

_Decimal64
truncD (_Decimal64 x)
{
	return x < 0 ? ceilD (x) : floorD (x);
}

// ---------------------------------------------------------------------------

_Decimal64
lgammaD (_Decimal64 x)
{
	int sign;
	return lgammaD_r (x, &sign);
}

_Decimal64
lgammaD_r (_Decimal64 x, int *signp)
{
	if (fabsD (x) <= (_Decimal64)DBL_MIN) {
		*signp = x >= 0 ? +1 : -1;
		return -logD (x);
	} else if (finiteD (x) && x >= (_Decimal64)DBL_MAX) {
		*signp = +1;
		return x * logD (x);
	}
	// No need to handle overflow on the left as all large numbers
	// are integers.

	return lgamma_r (x, signp);
}


_Decimal64
erfD (_Decimal64 x)
{
	// No need to handle overflow because of |y|=1 horizontal tangents
	if (fabsD (x) <= (_Decimal64)DBL_MIN) {
		_Decimal64 f = 1.1283791670955125738961589dd; // 2/sqrt(Pi)
		return x * f;
	} else
		return erf (x);
}

_Decimal64
erfcD (_Decimal64 x)
{
	// No need to handle overflow on the left because of y=2 horizontal tangent
	// No need to handle overflow to the right because underflow already happened
	// No need to handle underflow because erfc(0)=1
	return erfc (x);
}

// ---------------------------------------------------------------------------

_Decimal64
sinhD (_Decimal64 x)
{
	// No need to handle overflow because result will overflow anyway
	if (fabsD (x) <= (_Decimal64)DBL_MIN)
		return x;
	else
		return sinh (x);
}

_Decimal64
asinhD (_Decimal64 x)
{
	_Decimal64 ax = fabsD (x);
	if (ax <= (_Decimal64)DBL_MIN)
		return x;
	if (ax >= (_Decimal64)DBL_MAX)
		return copysignD (logD (ax) + M_LN2D, x);
	return asinh (x);
}

_Decimal64
coshD (_Decimal64 x)
{
	// No need to handle overflow because result will overflow anyway
	// No need to handle underflow because cosh(0)=1
	return cosh (x);
}

_Decimal64
acoshD (_Decimal64 x)
{
	// No need to handle underflow because the domain is [1,inf[
	if (x >= (_Decimal64)DBL_MAX)
		return logD (x) + M_LN2D;
	return acosh (x);
}

_Decimal64
tanhD (_Decimal64 x)
{
	// No need to handle overflow because of horizontal tangents
	if (fabsD (x) <= (_Decimal64)DBL_MIN)
		return x;
	else
		return tanh (x);
}

_Decimal64
atanhD (_Decimal64 x)
{
	_Decimal64 ax = fabsD (x);
	// No need to handle overflow because the domain is ]-1;+1[
	if (ax <= DECIMAL64_EPSILON) {
		// x - x^3/3 + ...
		return x;
	} else if (ax > 0.9dd && ax < 1) {
		_Decimal64 y = log1pD (2 * ax / (1 - ax)) / 2;
		return copysignD (y, x);
	} else
		return atanh (x);
}

// ---------------------------------------------------------------------------

_Decimal64
sinD (_Decimal64 x)
{
	if (fabsD (x) <= (_Decimal64)DBL_MIN)
		return x;
	// FIXME: need to handle large values.  Going via "double" is no good.
	return sin (x);
}

_Decimal64
cosD (_Decimal64 x)
{
	// No need to handle underflow as cos(0)=1.
	// FIXME: need to handle large values.  Going via "double" is no good.
	return cos (x);
}

_Decimal64
tanD (_Decimal64 x)
{
	if (fabsD (x) <= (_Decimal64)DBL_MIN)
		return x;
	// FIXME: need to handle large values.  Going via "double" is no good.
	return tan (x);
}

_Decimal64
asinD (_Decimal64 x)
{
	// No need to handle overflow because domain is [1,1]
	if (fabsD (x) <= (_Decimal64)DBL_MIN)
		return x;
	else
		return asin (x);
}

_Decimal64
acosD (_Decimal64 x)
{
	// No need to handle overflow because domain is [1,1]
	// No need to handle underflow because acos(0)=Pi/2
	return acos (x);
}

_Decimal64
atanD (_Decimal64 x)
{
	// No need to handle overflow because of horizontal tangents
	if (fabsD (x) <= (_Decimal64)DBL_MIN)
		return x;
	else
		return atan (x);
}

_Decimal64
atan2D (_Decimal64 y, _Decimal64 x)
{
	const _Decimal64 PI_1_4 = 0.7853981633974483dd;
	const _Decimal64 PI_1_2 = 1.570796326794897dd;
	const _Decimal64 PI_3_4 = 2.356194490192345dd;
	int signx, signy, specialx, specialy;
	uint64_t mantx, manty;

	specialy = decode64 (&y, &manty, NULL, &signy);
	specialx = decode64 (&x, &mantx, NULL, &signx);

	if (specialy == CLS_NAN) return y;
	if (specialx == CLS_NAN) return x;

	if (specialy == CLS_INF && specialx == CLS_INF)
		return copysignD (signx ? PI_3_4 : PI_1_4, y);
	else if (specialy == CLS_INF)
		return copysignD (PI_1_2, y);
	else if (manty == 0 || specialx == CLS_INF)
		return copysignD (signx ? M_PID : 0.dd, y);
	else if (mantx == 0)
		return copysignD (PI_1_2, y);

	if (fabsD (y) >= 1e100dd || fabsD (x) >= 1e100dd) {
		y = scalbnD (y, -100);
		x = scalbnD (x, -100);
	} else if (fabsD (y) <= 1e-100dd || fabsD (x) <= 1e-100dd) {
		y = scalbnD (y, +100);
		x = scalbnD (x, +100);
	}

	return atan2 (y, x);
}

// ---------------------------------------------------------------------------

// negneg: return -NAN on negatives.
static _Decimal64
log_helper (_Decimal64 x, int base)
{
	int special, sign, p2, p10, bits;
	uint64_t mant;
	_Decimal64 xm1;
	double dx;
	static const _Decimal64 res[64] = {
		+0.dd,
		+0.3010299956639812dd,
		-0.3979400086720376dd,
		-0.09691001300805641dd,
		+0.2041199826559248dd,
		-0.4948500216800940dd,
		-0.1938200260161128dd,
		+0.1072099696478684dd,
		+0.4082399653118496dd,
		-0.2907300390241692dd,
		+0.01029995663981195dd,
		+0.3113299523037931dd,
		-0.3876400520322257dd,
		-0.08661005636824446dd,
		+0.2144199392957367dd,
		-0.4845500650402821dd,
		-0.1835200693763009dd,
		+0.1175099262876803dd,
		+0.4185399219516615dd,
		-0.2804300823843573dd,
		+0.02059991327962390dd,
		+0.3216299089436051dd,
		-0.3773400953924137dd,
		-0.07631009972843251dd,
		+0.2247198959355487dd,
		-0.4742501084004701dd,
		-0.1732201127364889dd,
		+0.1278098829274923dd,
		+0.4288398785914735dd,
		-0.2701301257445453dd,
		+0.03089986991943586dd,
		+0.3319298655834171dd,
		-0.3670401387526018dd,
		-0.06601014308862056dd,
		+0.2350198525753606dd,
		-0.4639501517606582dd,
		-0.1629201560966770dd,
		+0.1381098395673042dd,
		+0.4391398352312854dd,
		-0.2598301691047334dd,
		+0.04119982655924781dd,
		+0.3422298222232290dd,
		-0.3567401821127898dd,
		-0.05571018644880861dd,
		+0.2453198092151726dd,
		-0.4536501951208462dd,
		-0.1526201994568650dd,
		+0.1484097962071162dd,
		+0.4494397918710974dd,
		-0.2495302124649214dd,
		+0.05149978319905976dd,
		+0.3525297788630410dd,
		-0.3464402254729778dd,
		-0.04541022980899665dd,
		+0.2556197658549845dd,
		-0.4433502384810343dd,
		-0.1423202428170531dd,
		+0.1587097528469281dd,
		+0.4597397485109093dd,
		-0.2392302558251095dd,
		+0.06179973983887171dd,
		+0.3628297355028529dd,
		-0.3361402688331659dd,
		-0.03511027316918470dd,
	};

	special = decode64 (&x, &mant, &p10, &sign);
	switch (special) {
	case CLS_NAN:
		return x;
	case CLS_INF:
		if (!sign)
			return x;
		break;
	default:
		if (mant == 0)
			return (_Decimal64)-INFINITY;
		break;
	}

	if (sign)
		return base == 10 ? (_Decimal64)NAN : -(_Decimal64)NAN;

	xm1 = x - 1;
	if (fabsD (xm1) < 0.25dd) {
		// x - 1 was exact and has smaller magnitude than x, so use log1p
		// This reduces _Decimal64-to-double rounding error greatly which
		// is significant for x very near 1.
		_Decimal64 lxm1 = log1p (xm1);
		switch (base) {
		default:
		case  2: return lxm1 * 1.4426950408889634073599dd;  // 1/log(2)
		case  3: return lxm1;
		case 10: return lxm1 * 0.434294481903251827651dd;   // 1/log(10)
		}
	}

	while (mant % 10 == 0) {
		mant /= 10;
		p10++;
	}

	p2 = 0;
	while ((mant & 1) == 0) {
		mant >>= 1;
		p2++;
	}

	while (base == 2 && p10 != 0 && mant % 5 == 0) {
		mant /= 5;
		p10++;
		p2--;
	}

	bits = 63 - __builtin_clzl (mant);
	dx = ldexp (mant, -bits);
	p2 += bits;
	if (dx > 1.41) {
		dx /= 2;
		p2++;
	}

	if (base == 2) {
		return p2 + M_LG10D * p10 + (_Decimal64)(log2 (dx));
	} else {
		p10 += (p2 * 77 + 128) / 256;
		_Decimal64 residual = ((_Decimal64)(log10 (dx)) + res[p2]);
		_Decimal64 l10 = p10 + residual;
		return base == 10 ? l10 : l10 * M_LN10D;
	}
}

// Note: log10D(-42) = +NaN            <-- inconsistent
_Decimal64
log10D (_Decimal64 x)
{
	return log_helper (x, 10);
}

// Note: log2D(-42) = -NaN
_Decimal64
log2D (_Decimal64 x)
{
	return log_helper (x, 2);
}


// Note: logD(-42) = -NaN
_Decimal64
logD (_Decimal64 x)
{
	return log_helper (x, 3);
}

// Note: log1pD(-43) = -NaN
_Decimal64
log1pD (_Decimal64 x)
{
	_Decimal64 ax = fabsD (x);

	if (ax < 1) {
		// x - x^2/2 + ... so this is fine:
		if (ax <= 0.01dd * (DECIMAL64_EPSILON * DECIMAL64_EPSILON))
			return x;
		return log1p (x);
	} else
		return logD (x + 1);
}

_Decimal64
expD (_Decimal64 x)
{
	// No need to handle overflow because horizontal tangent (left) and overflow (right)
	// No need to handle underflow because exp(0)=1
	return exp (x);
}

_Decimal64
expm1D (_Decimal64 x)
{
	// No need to handle overflow because horizontal tangent (left) and overflow (right)
	if (fabsD (x) <= (_Decimal64)DBL_MIN)
		return x;
	else
		return expm1 (x);
}

// 1: even integer, 0: non-integer (including inf, nan), -1 odd integer
static int
isint (_Decimal64 x)
{
	int special, p10;
	uint64_t mant;

	special = decode64 (&x, &mant, &p10, NULL);
	switch (special) {
	case CLS_NAN:
	case CLS_INF:
		return 0;
	case CLS_INVALID:
		return +1;
	default:
		break;
	}

	if (mant == 0 || p10 > 0)
		return 1;
	if (p10) {
		if (p10 <= -DECIMAL64_DIG || mant % u64_pow10_table[-p10])
			return 0;
		mant /= u64_pow10_table[-p10];
	}
	return 1 - ((mant & 1) << 1);
}



_Decimal64
powD (_Decimal64 x, _Decimal64 y)
{
	int qneg = 0, ysign;
	_Decimal64 z;

	if (x == 1 || y == 0)
		return 1;

	if (isnanD (x))
		return x;
	if (isnanD (y))
		return y;

	ysign = signbitD (y);
	if (x == 0) {
		int yoddint = isint (y) < 0;
		if (ysign)
			return yoddint ? copysignD (INFINITY, x) : (_Decimal64)INFINITY;
		else
			return yoddint ? x : 0;
	}

	if (!finiteD (y)) {
		if (x == -1)
			return 1;
		if (fabsD (x) < 1)
			return ysign ? (_Decimal64)INFINITY : 0.dd;
		else
			return ysign ? 0.dd : (_Decimal64)INFINITY;
	}

	if (x == -(_Decimal64)INFINITY) {
		int yoddint = isint (y) < 0;
		if (ysign) {
			return yoddint ? -0.dd : +0.dd;
		} else {
			return yoddint ? -(_Decimal64)INFINITY : +(_Decimal64)INFINITY;
		}
	}

	if (x == (_Decimal64)INFINITY) {
		return (ysign ? 0.dd : (_Decimal64)INFINITY);
	}

	if (x == 10 && isint (y) && fabsD (y) < INT_MAX)
		return pow10D ((int)y);

	if (x < 0) {
		int qint = isint (y);
		if (!qint)
			return NAN;
		qneg = qint < 0;
		x = -x;
	}

	z = pow (x, y);
	return qneg ? -z : z;
}

_Decimal64
modfD (_Decimal64 x, _Decimal64 *y)
{
	if (!finiteD (x)) {
		*y = x;
		return isnanD (x) ? x : copysignD (0, x);
	}

	*y = truncD (x);
	return copysignD (x - *y, x);
}

_Decimal64
fmodD (_Decimal64 x, _Decimal64 y)
{
	int specialx, specialy, signx, signy, p10x, p10y;
	uint64_t mantx, manty;
	const uint64_t min_normal_mant = (DECIMAL64_MAX_MANT + 1) / 10;

	specialx = decode64 (&x, &mantx, &p10x, &signx);
	specialy = decode64 (&y, &manty, &p10y, &signy);

	if (specialx >= CLS_NAN || specialy == CLS_NAN || (manty == 0 && specialy != CLS_INF))
		return copysignD (NAN, x);
	if (mantx == 0 || specialy == CLS_INF)
		return x;

	// At this point both x and y are finite and non-zero

	while (mantx < min_normal_mant) {
		mantx *= 10;
		p10x--;
	}
	while (manty < min_normal_mant) {
		manty *= 10;
		p10y--;
	}

	while (p10x >= p10y) {
		uint8_t q;
		uint64_t qy;

		if (manty > mantx) {
			if (p10x == p10y)
				break;
			mantx *= 10;
			p10x--;
		}

		q = mantx / manty;
		qy = q * manty;
		mantx -= qy;
		if (mantx == 0)
			return signx ? -0.dd : 0.dd;
		while (mantx < min_normal_mant) {
			mantx *= 10;
			p10x--;
		}
	}

	if (p10x < DECIMAL64_BIAS)
		return copysignD (scalbnD (mantx, p10x), x);
	return make64 (mantx, p10x, signx);
}

_Decimal64
sqrtD (_Decimal64 x)
{
	int s = 0;

	if (x < 0)
		return -(_Decimal64)NAN;
	if (x == 0 || !finiteD (x))
		return x;

	if (x <= (_Decimal64)DBL_MIN) {
		x = scalbnD (x, 300);
		s = -150;
	} else if (x >= (_Decimal64)DBL_MAX) {
		x = scalbnD (x, -300);
		s = +150;
	}

	x = sqrt (x);
	// We could do a newton step here.

	x = scalbnD (x, s);
	return x;
}

_Decimal64
cbrtD (_Decimal64 x)
{
	_Decimal64 ax;
	int s = 0;

	if (x == 0 || !finiteD (x))
		return x;

	ax = fabsD (x);
	if (ax <= (_Decimal64)DBL_MIN) {
		x = scalbnD (x, 300);
		s = -100;
	} else if (ax >= (_Decimal64)DBL_MAX) {
		x = scalbnD (x, -300);
		s = 100;
	}

	x = cbrt (x);
	// We could do a newton step here.

	return scalbnD (x, s);
}

_Decimal64
hypotD (_Decimal64 x, _Decimal64 y)
{
	int specialx, specialy;
	uint64_t mantx, manty;
	_Decimal64 r, s;
	const _Decimal64 SQRT2P1_HI = 2.414213562373095dd;
	const _Decimal64 SQRT2P1_LO = 4.880168872420970e-17dd;  // Extra "0" between the two

	specialx = decode64 (&x, &mantx, NULL, NULL);
	specialy = decode64 (&y, &manty, NULL, NULL);

	if (specialx == CLS_INF || specialy == CLS_INF)
		return (_Decimal64)INFINITY;
	if (specialx == CLS_NAN || specialy == CLS_NAN)
		return (_Decimal64)NAN; // Always +nan

	x = fabsD (x);
	y = fabsD (y);
	if (mantx == 0)
		return y;
	if (manty == 0)
		return x;

	if (y > x) {
		_Decimal64 z = x;
		x = y;
		y = z;
	}

	r = x - y;
	if (r <= y) {
		r = r / y;
		s = r * (r + 2);
		r = ((s / (M_SQRT2D + sqrtD (2 + s)) + r) + SQRT2P1_LO) + SQRT2P1_HI;
	} else {
		r = x / y;
		r = r + sqrtD (r * r + 1);
	}

	return (y / r) + x;
}

// ---------------------------------------------------------------------------

_Decimal64
jnD (int n, _Decimal64 x)
{
	if ((n == 1 || n == -1) && fabsD (x) <= (_Decimal64)DBL_MIN)
		return x / (2 * n);
	// No need to handle other underflows

	// FIXME: need to handle large values.  Going via "double" is no good.

	return jn (n, x);
}

_Decimal64
ynD (int n, _Decimal64 x)
{
	// FIXME: need to handle small and large numbers.
	return yn (n, x);
}

// ---------------------------------------------------------------------------

void
_go_decimal_init (void)
{
	// Test number big enough to have only one representation (but still
	// subject to two different encodings)
	_Decimal64 const x = 1234567890123456.dd;
	uint64_t const expected = 0x31c462d53c8abac0ull;
	uint64_t u64;

	memcpy (&u64, &x, sizeof (u64));
	if (sizeof (u64) != 8 || u64 != expected) {
		// Is this fails, Decimal64 is probably dpd encoded.
		// (or we have really weird endianess going on)
		g_printerr ("Decimal64 numbers are not bis encoded.\n");
		g_printerr ("(Got x%lx, expected 0x%lx)\n", u64, expected);
		abort ();
	}

	init_decimal_printf_support ();
}

void
_go_decimal_shutdown (void)
{
	g_free (decimal_point_str);
	decimal_point_str = NULL;
}

// ---------------------------------------------------------------------------
