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
// acoshD          A         A-        B
// asinD           A         *         *
// asinhD          A         A-        B
// atanD           A         *         *
// atan2D          A         *         A
// atanhD          A         A-        *
// cbrtD           A         A         B
// ceilD           A         A         A
// copysignD       A         A         A
// cosD            B         C*        *
// coshD           A         A         B
// erfD            A         *         *
// erfcD           A         A-        B
// expD            A         A         *
// expm1D          A         *         *
// fabsD           A         A         *
// floorD          A         A         A
// frexpD          A         B         -
// fmodD           A         A         *
// hypotD          A         A         -
// jnD             B         C*        B
// ldexpD          B         B         -
// lgammaD         A         A-        A
// lgammaD_r       A         A-        B
// log10D          A         A-        A
// log2D           A         A-        A
// log1pD          A         A-        A
// logD            A         A-        A
// modfD           A         A         A
// nextafterD      A         A         A
// powD            A         A-        A
// roundD          A         A         A
// sinD            B         C*        *
// sinhD           A         A-        B
// scalbln         A         A         A
// scalbn          A         A         A
// sqrtD           A         A         B
// tanD            B         C*        *
// tanhD           A         A-        B
// truncD          A         A         A
// ynD             B         C*        B
// isfiniteD       A         A         A
// isnanD          A         A         A
// signbitD        A         A         A
// strtoDd         A         A         B
// (printf)        A         A         -
//
// * Stub via double.  Range:B, Accuracy:B


// NOTE: Implementations have comments like
//
// * No need to handle overflow on the left because [...]
// * No need to handle overflow to the right because [...]
// * No need to handle underflow because [...]
//
// These refer to what happens when the argument is cast from _Decimal64
// to double, typically implicitly in a stub call.  These are not claims
// about whether the stub's result can overflow or underflow.



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
#define M_LN2D   0.6931471805599453094dd                   // log(2)
#define M_2_PID  0.6366197723675813431dd                   // 2/pi
#define M_1_PID  0.3183098861837906715dd                   // 1/pi
// Euler-Mascheroni minus log(2), combined ahead of time (rather than as
// two separate Decimal64 constants added/subtracted at run time) so that
// ynD's n==0 case only rounds its "logD(x) + ..." once instead of twice.
#define M_EULER_LN2D (-0.11593151565841244881dd)
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

#define CCHR(s) ((const char *)(s))

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

// Like decimal64, but normalize.
static int
decode64_norm (_Decimal64 const *args0, uint64_t *pmant, int *pp10, int *sign)
{
	uint64_t mant;
	int p10;
	int special = decode64 (args0, &mant, &p10, sign);

	switch (special) {
	case CLS_INF:
	case CLS_NAN:
		break;

	case CLS_INVALID:
		special = CLS_NORMAL;
		break;

	default:
	case CLS_NORMAL:
		if (mant)
			while (mant % 10 == 0) {
				mant /= 10;
				p10++;
			}
		else
			p10 = 0;
		break;
	}

	if (pp10) *pp10 = p10;
	if (pmant) *pmant = mant;
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

// 5^0 .. 5^22 -- all fit comfortably in 64 bits
static const uint64_t
u64_pow5_table[23] = {
	1ull, 5ull, 25ull, 125ull, 625ull, 3125ull, 15625ull,
	78125ull, 390625ull, 1953125ull, 9765625ull, 48828125ull,
	244140625ull, 1220703125ull, 6103515625ull, 30517578125ull,
	152587890625ull, 762939453125ull, 3814697265625ull,
	19073486328125ull, 95367431640625ull, 476837158203125ull,
	2384185791015625ull
};


static int
u64_digits (uint64_t x)
{
	int l2, l10;

	if (x == 0)
		return 1;

	assert (sizeof (long) == sizeof (uint64_t));
	l2 = 63 - __builtin_clzl (x);
	// log_10(2) is a hair smaller than 77/256
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
	char buffer[8192];
	int special, p10, sign;
	int len = 0;
	int qupper = (info->spec <= 'Z');

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

	// This isn't thread-safe and adds a measurable amount of overhead
	// in the plain "%f" case, so only do this once we have verified
	// that it's a format we want to handle.
	const char *dot = decimal_point ();
	int dotlen = strlen (dot);

	char signchar;
	if (sign)
		signchar = '-';
	else if (info->showsign)
		signchar = '+';
	else if (info->space)
		signchar = ' ';
	else
		signchar = 0;

	char padchar = info->pad;

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
			buffer[1] = 0;
			len = 1;
		}
		break;
	}
	case CLS_NAN:
		strcpy (buffer, (qupper ? "NAN" : "nan"));
		len = 3;
		padchar = ' ';
		break;
	case CLS_INF:
		strcpy (buffer, (qupper ? "INF" : "inf"));
		len = 3;
		padchar = ' ';
		break;
	case CLS_INVALID:
		buffer[0] = '0';
		buffer[1] = 0;
		len = 1;
		p10 = 0;
		break;
	}

	if (signchar) len++;

	while (!info->left && len < info->width) {
		len++;
		putc (padchar, stream);
	}
	if (signchar) putc (signchar, stream);
	fputs (buffer, stream);
	while (len < info->width) {
		len++;
		putc (' ', stream);
	}
	return len;
}

static gboolean
decimal_printf_already_supported (void)
{
	char const testfmt[] = "%" GO_DECIMAL64_MODIFIER "g";
	char *test = g_strdup_printf (testfmt, 123.45dd);
	gboolean good = g_str_equal (test, "123.45");
	g_free (test);

	return good;
}

static void
init_decimal_printf_support (void)
{
	if (decimal_printf_already_supported ())
		return;

	decimal64_type = register_printf_type (decimal64_va_arg);
	decimal128_type = register_printf_type (decimal128_va_arg);
	if (decimal64_type == -1 || decimal128_type == -1) {
		g_printerr ("Failed to install printf handlers for Decimal64 and Decimal128.\n");
		abort ();
	}

#define CAT(x,y) x ## y
#define WSTR(x) CAT(L,x)
	decimal64_modifier = register_printf_modifier (WSTR (GO_DECIMAL64_MODIFIER));
	decimal128_modifier = register_printf_modifier (WSTR (GO_DECIMAL128_MODIFIER));
	if (decimal64_modifier == -1 || decimal128_modifier == -1) {
		g_printerr ("Failed to install printf modifiers for Decimal64 and Decimal128.\n");
		abort ();
	}
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

// Classification.  Deliberately not the C99 type-generic macros: they have
// no decimal branch, so a _Decimal64 would be converted to long double,
// which is slow and lossy.  Decoding the encoding is exact and cheap.

/**
 * isnanD:
 * @x: value to test
 *
 * Tests for NaN.
 *
 * Decodes the encoding directly, so it is exact and never converts @x to a
 * wider type.  Both quiet and signalling NaNs of either sign are
 * recognised.
 *
 * Returns: Non-zero if @x is a NaN, otherwise 0.
 */
inline int
isnanD (_Decimal64 x)
{
	return decode64 (&x, NULL, NULL, NULL) == CLS_NAN;
}

/**
 * isfiniteD:
 * @x: value to test
 *
 * Tests for a finite value.
 *
 * Zero, subnormals and normal numbers are finite; infinities and NaNs are
 * not.  An encoding that does not represent a valid value is treated as
 * zero and so counts as finite.
 *
 * Returns: Non-zero if @x is neither infinite nor a NaN, otherwise 0.
 */
inline int
isfiniteD (_Decimal64 x)
{
	return decode64 (&x, NULL, NULL, NULL) < CLS_NAN;
}

/**
 * signbitD:
 * @x: value to test
 *
 * Extracts the sign bit.
 *
 * Unlike a comparison against zero this distinguishes -0 from +0, and it
 * also reports the sign of infinities and NaNs.
 *
 * Returns: Non-zero if the sign bit of @x is set, otherwise 0.  The value
 *     is not necessarily 1.
 */
inline int
signbitD (_Decimal64 x)
{
	int sign;
	(void)decode64 (&x, NULL, NULL, &sign);
	return sign;
}

/**
 * copysignD:
 * @x: magnitude source
 * @y: sign source
 *
 * Copies a sign.
 *
 * The magnitude comes from @x and the sign bit from @y.  Works for zeros
 * and infinities.  If @x is a NaN the result is a NaN.
 *
 * Returns: @x with the sign of @y.
 */
_Decimal64
copysignD (_Decimal64 x, _Decimal64 y)
{
	if (signbitD (x) == signbitD (y))
		return x;
	else
		return -x;
}

/**
 * fabsD:
 * @x: argument
 *
 * Absolute value.
 *
 * Exact.  fabsD(-0) is +0 and the absolute value of an infinity is
 * +infinity.  A NaN stays a NaN.
 *
 * Returns: |@x|.
 */
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

/**
 * nextafterD:
 * @x: starting value
 * @y: direction
 *
 * Steps to the adjacent representable value.
 *
 * Moves @x by one unit in the last place, on the 16 digit grid appropriate
 * to its magnitude, towards @y.  If the two are equal @y is returned.  From
 * zero the smallest subnormal (1e-398) of the appropriate sign is returned.
 * From an infinity the largest finite value with the same sign is returned.
 * Stepping past #DECIMAL64_MAX gives infinity.  If either argument is a
 * NaN, the result is a NaN.
 *
 * Returns: The next representable value after @x in the direction of @y.
 */
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

/**
 * ldexpD:
 * @x: significand
 * @e: binary exponent
 *
 * Multiplies by a power of two.
 *
 * Computes @x * 2^@e.  <emphasis>This operation is not lossless</emphasis>:
 * powers of two are not exact in decimal, so the result is subject to an
 * ordinary Decimal64 rounding, and repeated use can accumulate error.
 * Zero, infinities and NaNs are returned as-is.  Overflow and underflow
 * follow normal Decimal64 arithmetic.
 *
 * Returns: @x * 2^@e.
 */
// NOTE: THIS IS NOT A LOSSLESS OPERATION
_Decimal64
ldexpD (_Decimal64 x, int e)
{
	if (x == 0 || !isfiniteD (x))
		return x;

	if (e > 1023) {
		return x * (_Decimal64)ldexp(1, 1023) *
			(_Decimal64)ldexp(1, e - 1023);
	} else if (e < -1023) {
		return x * (_Decimal64)ldexp(1, -1023) *
			(_Decimal64)ldexp(1, e + 1023);
	} else
		return x * (_Decimal64)(ldexp(1, e));
}

/**
 * frexpD:
 * @x: value to split
 * @e: (out): binary exponent
 *
 * Splits into a binary significand and exponent.
 *
 * Returns @m and stores @e such that @x = @m * 2^@e with 0.5 <= |@m| < 1.
 * <emphasis>This operation is not lossless</emphasis>: it is computed via
 * double (with scaling to cope with values outside the range of double), so
 * @m is only accurate to roughly double precision.  For zero, infinities
 * and NaNs, @x is returned and *@e is set to 0.
 *
 * Returns: The significand @m, carrying the sign of @x.
 */
// NOTE: THIS IS NOT A LOSSLESS OPERATION
_Decimal64
frexpD (_Decimal64 x, int *e)
{
	_Decimal64 m, ax;

	if (x == 0 || !isfiniteD (x)) {
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

/**
 * scalblnD:
 * @x: value to scale
 * @e: decimal exponent adjustment
 *
 * Multiplies by a power of ten.
 *
 * Computes @x * 10^@e by adjusting the encoded exponent, so it is exact
 * except when the result becomes subnormal (digits are then rounded, ties
 * away from zero) or underflows to zero.  Overflow gives an infinity of the
 * same sign as @x.  Any @e of large magnitude is safe: it is clamped
 * internally to a range that already saturates.
 *
 * Zero, infinities and NaNs are returned as-is.
 *
 * This is a lossless operation (expect when going denormal or underflowing).
 *
 * Returns: @x * 10^@e.
 */
_Decimal64
scalblnD (_Decimal64 x, long e)
{
	uint64_t mant;
	int p10, sign;
	int too_far = (DECIMAL64_MAX_EXP - DECIMAL64_MIN_EXP) + DECIMAL64_DIG;

	int special = decode64 (&x, &mant, &p10, &sign);
	switch (special) {
	case CLS_NORMAL:
		if (mant == 0)
			return x;
		break;
	case CLS_INVALID:
		return sign ? -0.dd : 0.dd;
	case CLS_NAN:
	case CLS_INF:
		return x;
	}

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

/**
 * scalbnD:
 * @x: value to scale
 * @e: decimal exponent adjustment
 *
 * Multiplies by a power of ten.
 *
 * Same as scalblnD() but with an @e of type int.  Note that this scales by
 * a power of <emphasis>ten</emphasis>, which is the radix of Decimal64,
 * unlike the double-precision function of the same name whose radix is two.
 *
 * Returns: @x * 10^@e.
 */
_Decimal64
scalbnD (_Decimal64 x, int e)
{
	return scalblnD (x, e);
}

/**
 * unscalbnD:
 * @x: value to split
 * @e: (out): decimal exponent
 *
 * Splits into a decimal significand and exponent.
 *
 * Returns @m and stores @e such that @x = @m * 10^@e with 0.1 <= |@m| < 1.
 * This is exact, being the decimal counterpart of frexpD().  For zero, @m
 * is a zero with the sign of @x and *@e is 0.  For infinities and NaNs, @x
 * is returned and *@e is set to 0.
 *
 * Returns: The significand @m, carrying the sign of @x.
 */
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

static gboolean
caseprefix (const unsigned char *us, const char *p)
{
	while (*p) {
		unsigned char c = *p;
		if (c != toupper (*us))
			return FALSE;
		p++;
		us++;
	}
	return TRUE;
}

/**
 * strtoDd:
 * @s: string to parse
 * @end: (out) (optional): location to receive the end of the parsed text
 *
 * Converts a string to Decimal64.
 *
 * Accepts optional leading white space, an optional sign, then either
 * decimal digits with an optional decimal point (in the current locale) and
 * optional exponent, or INF, INFINITY or NAN in any case.  The result is
 * correctly rounded (ties rounding up in magnitude) for inputs of any
 * length: digits beyond the 16th only affect rounding.  Exponents of
 * arbitrary magnitude are handled without integer overflow; values too
 * large give an infinity and values too small give a zero, both with the
 * sign of the input.  If no number can be parsed, 0 is returned and *@end
 * (if given) is set to @s.
 *
 * Returns: The converted value.
 */
_Decimal64
strtoDd (const char *s, char **end)
{
	uint64_t m = 0;
	int sign = 0;
	const unsigned char *us = (const unsigned char *)s;
	int digits = 0;
	int period = 0;
	int64_t scale = 0;
	_Decimal64 res;
	const char *dot = decimal_point ();
	gboolean ru = FALSE;
	while (isspace (*us))
		us++;

	if (*us == '-')
		sign++, us++;
	else if (*us == '+')
		us++;

	if (!isdigit (*us) && !(g_str_has_prefix (CCHR (us), dot) && isdigit (us[strlen(dot)]))) {
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

	while (isdigit (*us) || g_str_has_prefix (CCHR (us), dot)) {
		if (g_str_has_prefix (CCHR (us), dot)) {
			if (period)
				break;
			period = 1;
		} else {
			if (digits < DECIMAL64_DIG) {
				m = 10 * m + (*us - '0');
				if (m) digits++;
				if (period) scale--;
			} else if (digits == DECIMAL64_DIG) {
				ru = (*us >= '5');  // Delayed round-up.  Apply only for normals
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
	if (ru && (scale >= DECIMAL64_BIAS))
		// Check if we need to round up.  Don't for subnormals.
		m++;
        res = scalbnD (m, CLAMP (scale, G_MININT, G_MAXINT));
	if (sign) res = -res;
	return res;
}

// ---------------------------------------------------------------------------

/**
 * floorD:
 * @x: argument
 *
 * Rounds down (towards minus infinity) to nearest integer.
 *
 * The sign of zero is preserved and infinities and NaNs are returned as-is.
 *
 * Returns: The largest integer not greater than @x.
 */
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

/**
 * ceilD:
 * @x: argument
 *
 * Rounds up (towards minus infinity) to nearest integer.
 *
 * The sign of zero is preserved and infinities and NaNs are returned as-is.
 *
 * Returns: The smallest integer not less than @x.
 */
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

/**
 * roundD:
 * @x: argument
 *
 * Rounds to nearest integer, halves away from zero.
 *
 * Unlike the default Decimal64 rounding mode this does not round
 * ties to even.  Zeros, infinities and NaNs are returned as-is.
 *
 * Returns: @x rounded to an integer.
 */
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

/**
 * truncD:
 * @x: argument
 *
 * Rounds towards zero to nearest integer.
 *
 * Zeros, infinities and NaNs are returned as-is.
 *
 * Returns: The integer part of @x.
 */
_Decimal64
truncD (_Decimal64 x)
{
	return x < 0 ? ceilD (x) : floorD (x);
}

// ---------------------------------------------------------------------------

/**
 * lgammaD:
 * @x: argument
 *
 * Logarithm of the absolute value of the gamma function.
 *
 * Poles at zero and the negative integers give +infinity.  Use lgammaD_r()
 * if the sign of gamma is needed.
 *
 * Returns: log(|gamma(@x)|).
 */
_Decimal64
lgammaD (_Decimal64 x)
{
	int sign;
	return lgammaD_r (x, &sign);
}

/**
 * lgammaD_r:
 * @x: argument
 * @signp: (out): sign of gamma(@x), +1 or -1
 *
 * Logarithm of the absolute value of the gamma function, and its sign.
 *
 * As lgammaD(), but also stores in *@signp the sign of gamma(@x).
 *
 * Returns: log(|gamma(@x)|).
 */
_Decimal64
lgammaD_r (_Decimal64 x, int *signp)
{
	if (fabsD (x) <= (_Decimal64)DBL_MIN) {
		*signp = (signbitD (x) ? -1 : +1);
		return -logD (fabsD (x));
	} else if (isfiniteD (x) && x >= 1e20dd) {
		// Stirling's asymptotic expansion, evaluated natively in
		// _Decimal64 so we never round-trip through "double" (whose
		// own lgamma() already overflows somewhere around x=1e306,
		// long before the true result overflows _Decimal64's own
		// range around x=1.14e382).  For x this large the 1/(12x)
		// correction term -- and everything past it -- is far below
		// 16-digit precision relative to the ~x*ln(x) sized result,
		// so the bare leading term is already correctly rounded; if
		// the true result is itself too big for _Decimal64, this
		// naturally overflows to infinity like any other _Decimal64
		// arithmetic would.

		*signp = +1;

		// const _Decimal64 half_ln_2pi = 0.9189385332046727dd;
		// return (x - 0.5dd) * logD (x) - x + half_ln_2pi;

		// Simplified, given how big x is:
		return x * (logD (x) - 1);
	}
	// No need to handle overflow on the left as all large numbers
	// are integers.

	return lgamma_r (x, signp);
}


/**
 * erfD:
 * @x: argument
 *
 * Error function.
 *
 * This produces a value in the range -1 to 1.
 *
 * Returns: erf(@x).
 */
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

// Compute exp(-x*x) with extra precision, even though a plain "x*x" would
// itself already lose precision at decimal64's 16 digits (that rounding
// error gets amplified almost 1:1 into a *relative* error of exp(-x*x),
// since exp is that sensitive to its argument once |x*x| is in the
// hundreds).  We recover the rounding error of "x*x" exactly (TwoProduct,
// via a Veltkamp/Dekker split of x) and fold that tiny correction into the
// exponential's own argument reduction, the same way exp_helper folds in
// the low part of ln(10).
static _Decimal64
neg_square_exp (_Decimal64 x)
{
	const _Decimal64 l10e = 0.434294481903251827651dd;
	const _Decimal64 l10_h = 2.302585092994e-00dd;
	const _Decimal64 l10_l = 4.568401799145e-14dd;
	const _Decimal64 split = 100000001.dd;  // 10^8 + 1

	_Decimal64 p = x * x;
	if (p >= 1000)
		return 0;  // To avoid Inf-Inf later

	_Decimal64 t = x * split;
	_Decimal64 x_hi = t - (t - x);
	_Decimal64 x_lo = x - x_hi;
	_Decimal64 err = ((x_hi * x_hi - p) + 2 * x_hi * x_lo) + x_lo * x_lo;
	// x*x == p + err, to about twice decimal64's working precision.

	// exp(-(p+err)): reduce based on -p as usual, but fold -err into the
	// (already small) reduced residual before handing it to "double".
	_Decimal64 y = -p;
	_Decimal64 k = roundD (l10e * y);
	_Decimal64 yr = ((y - (k * l10_h)) - (k * l10_l)) - err;
	return scalblnD ((_Decimal64)exp (yr), k);
}

/**
 * erfcD:
 * @x: argument
 *
 * Complementary error function.
 *
 * Computes 1 - erf(@x) without cancellation.  This produces a value in the
 * range -2 to 0.
 *
 * Returns: erfc(@x).
 */
_Decimal64
erfcD (_Decimal64 x)
{
	// No need to handle overflow on the left because of y=2 horizontal tangent

	if (x >= 20) {
		// For x this large, glibc's erfc() has already underflowed to
		// 0 in "double" (that starts around x=27.5, and precision is
		// already badly degraded from x=27 on, as the true result is
		// deep in double's subnormal range) even though the true
		// result is still comfortably representable in _Decimal64,
		// whose range reaches down to 1e-398.  Use the standard
		// asymptotic expansion instead, evaluated natively in
		// _Decimal64 so we never round-trip through a double that
		// might underflow:
		//
		//   erfc(x) ~ exp(-x^2)/(x*sqrt(pi)) *
		//             (1 - 1/(2x^2) + 3/(4x^4) - 15/(8x^6) + ...)
		//
		// This is an asymptotic (eventually divergent) series, but
		// it gets more accurate the larger x is, and for x >= 20
		// twelve terms already give around 1e-25 relative accuracy --
		// far more than _Decimal64's 16 digits need.
		const _Decimal64 sqrt_pi = 1.772453850905516dd;
		_Decimal64 inv2x2 = 0.5dd / (x * x);
		_Decimal64 term = 1.dd, sum = 1.dd;
		int n;

		for (n = 1; n <= 12; n++) {
			term *= -(_Decimal64)(2 * n - 1) * inv2x2;
			sum += term;
		}
		return neg_square_exp (x) / (sqrt_pi * x) * sum;
	}
	// No need to handle underflow because erfc(0)=1
	return erfc (x);
}

// ---------------------------------------------------------------------------

// Compute f * exp(x)
// Precondition: f should be 1.dd or 0.5dd, possibly negative, exact.
static _Decimal64
exp_helper (_Decimal64 x, _Decimal64 f)
{
	const _Decimal64 l10e = 0.434294481903251827651dd;
	const _Decimal64 l10_h = 2.302585092994e-00dd;
	const _Decimal64 l10_l = 4.568401799145e-14dd;  // free extra 0 in-between

	if (fabsD (x) < 1000) {
		// Get the power of 10.  We might be off by one once in a very
		// blue moon, but that's fine.
		_Decimal64 k = roundD (l10e * x);
		// (k * l10_h) and (k * l10_l) are exact since k has at most
		// three digits and the l10 parts have 13.
		_Decimal64 xr = (x - (k * l10_h)) - (k * l10_l);
		if (k > 300 && fabsD (f) < 1)
			// If we're just on the edge of overflow, move a 10
			// into f.
			k--, f *= 10;
		return f * scalblnD (exp (xr), k);
	} else {
		// Large or nan
		return f * (_Decimal64)(exp (x));
	}
}


/**
 * sinhD:
 * @x: argument
 *
 * Hyperbolic sine.
 *
 * Returns: sinh(@x).
 */
_Decimal64
sinhD (_Decimal64 x)
{
	_Decimal64 ax = fabsD (x);

	if (ax < 1e-10dd || !isfiniteD (x))
		return x;
	else if (ax < 1) {
		_Decimal64 u = expm1D (x);
		_Decimal64 r = 0.5dd * (u + u / (u + 1));
		return copysignD (r, x);
	} else if (ax < 30) {
		_Decimal64 u = expD (x);
		_Decimal64 r = 0.5dd * (u - 1 / u);
		return copysignD (r, x);
	} else {
		// ax > 30
		return copysignD (exp_helper (ax, 0.5dd), x);
	}
}

/**
 * asinhD:
 * @x: argument
 *
 * Inverse hyperbolic sine.
 *
 * Returns: asinh(@x).
 */
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

/**
 * coshD:
 * @x: argument
 *
 * Hyperbolic cosine.
 *
 * Returns: cosh(@x).
 */
_Decimal64
coshD (_Decimal64 x)
{
	if (isnanD (x)) {
		// Specifically there so -nan -> -nan to match libc's cosh
		return x;
	}

	x = fabsD (x);
	if (x > 30)
		return exp_helper (x, 0.5dd);
	else {
		_Decimal64 u = exp_helper (x, 1);
		return 0.5dd * (u + 1 / u);
	}
}

/**
 * acoshD:
 * @x: argument
 *
 * Inverse hyperbolic cosine.
 *
 * The domain is [1, +infinity].  An argument less than 1 gives a NaN.
 *
 * Returns: The non-negative value whose hyperbolic cosine is @x.
 */
_Decimal64
acoshD (_Decimal64 x)
{
	// No need to handle underflow because the domain is [1,inf[
	if (x >= 1e10dd)
		return logD (x) + M_LN2D;
	if (x < 1) {
		// Prevent rounding up to 1.  Call acosh for getting
		// the same nan as libc.
		return acosh (0);
	}
	return acosh (x);
}

/**
 * tanhD:
 * @x: argument
 *
 * Hyperbolic tangent.
 *
 * This produces a value in the range -1 to 1.
 *
 * Returns: tanh(@x).
 */
_Decimal64
tanhD (_Decimal64 x)
{
	// No need to handle overflow because of horizontal tangents
	if (fabsD (x) <= 1e-10dd)
		return x;
	else
		return tanh (x);
}

/**
 * atanhD:
 * @x: argument
 *
 * Inverse hyperbolic tangent.
 *
 * The domain is [-1, +1]: atanhD(+-1) is +-infinity and a larger magnitude
 * gives a NaN.
 *
 * Returns: atanh(@x).
 */
_Decimal64
atanhD (_Decimal64 x)
{
	_Decimal64 ax = fabsD (x);
	if (ax > 1)
		return -(_Decimal64)NAN;  // To match glibc's atanh
	else if (ax <= DECIMAL64_EPSILON) {
		// x - x^3/3 + ...
		return x;
	} else if (ax > 0.9dd && ax < 1) {
		_Decimal64 y = log1pD (2 * ax / (1 - ax)) / 2;
		return copysignD (y, x);
	} else
		return atanh (x);
}

// ---------------------------------------------------------------------------

/**
 * sinD:
 * @x: angle in radians
 *
 * Sine.
 *
 * NOTE: Accuracy is good for modest arguments but deteriorates as |@x| grows.
 * Beyond 10^10 the result starts approaching noise.
 *
 * Returns: sin(@x).
 */
_Decimal64
sinD (_Decimal64 x)
{
	// sin x = x - x^3/6 + ...
	if (fabsD (x) <= DECIMAL64_EPSILON)
		return x;

	int km4;
	_Decimal64 xr = go_reduce_piD (x, 1, &km4);
	if (!isfiniteD (xr)) xr = x;  // Unimplemented

	switch (km4) {
	default:
	case 0: return +sin (xr);
	case 1: return +cos (xr);
	case 2: return -sin (xr);
	case 3: return -cos (xr);
	}
}

/**
 * cosD:
 * @x: angle in radians
 *
 * Cosine.
 *
 * The accuracy limitations for large |@x| described for sinD() apply.
 *
 * Returns: cos(@x).
 */
_Decimal64
cosD (_Decimal64 x)
{
	// No need to handle underflow as cos(0)=1.
	int km4;
	_Decimal64 xr = go_reduce_piD (x, 1, &km4);
	if (!isfiniteD (xr)) xr = x;  // Unimplemented

	switch (km4) {
	default:
	case 0: return +cos (xr);
	case 1: return -sin (xr);
	case 2: return -cos (xr);
	case 3: return +sin (xr);
	}
}

/**
 * tanD:
 * @x: angle in radians
 *
 * Tangent.
 *
 * The accuracy limitations for large |@x| described for sinD() apply.
 *
 * Returns: tan(@x).
 */
_Decimal64
tanD (_Decimal64 x)
{
	// sin x = x - x^3/3 + ...
	if (fabsD (x) <= DECIMAL64_EPSILON)
		return x;

	int km4;
	_Decimal64 xr = go_reduce_piD (x, 1, &km4);
	if (!isfiniteD (xr)) xr = x;  // Unimplemented

	switch (km4) {
	default:
	case 0: case 2: return +tan (xr);
	case 1: case 3: return -1 / tan (xr);
	}
}

/**
 * asinD:
 * @x: argument
 *
 * Inverse sine.
 *
 * The domain is [-1, +1]; a larger magnitude gives a NaN.
 *
 * Returns: The value in [-pi/2, pi/2] whose sine is @x.
 */
_Decimal64
asinD (_Decimal64 x)
{
	if (fabsD (x) > 1)
		return (_Decimal64)NAN;
	else if (fabsD (x) <= (_Decimal64)DBL_MIN)
		return x;
	else
		return asin (x);
}

/**
 * acosD:
 * @x: argument
 *
 * Inverse cosine.
 *
 * The domain is [-1, +1]; a larger magnitude gives a NaN.
 *
 * Returns: The value in [0, pi] whose cosine is @x.
 */
_Decimal64
acosD (_Decimal64 x)
{
	if (fabsD (x) > 1)
		return (_Decimal64)NAN;

	// No need to handle underflow because acos(0)=Pi/2
	return acos (x);
}

/**
 * atanD:
 * @x: argument
 *
 * Inverse tangent.
 *
 * Returns: The value in [-pi/2, pi/2] whose tangent is @x.
 */
_Decimal64
atanD (_Decimal64 x)
{
	// No need to handle overflow because of horizontal tangents
	if (fabsD (x) <= (_Decimal64)DBL_MIN)
		return x;
	else
		return atan (x);
}

/**
 * atan2D:
 * @y: ordinate
 * @x: abscissa
 *
 * Two-argument inverse tangent.
 *
 * Computes the angle of the point (@x,@y) in the range [-pi, pi], using the
 * signs of both arguments to select the quadrant.  Follows the special
 * cases of C99 Annex F for zeros and infinities, including the sign of
 * zero.
 *
 * Returns: The angle in radians.
 */
_Decimal64
atan2D (_Decimal64 y, _Decimal64 x)
{
	const _Decimal64 PI_1_4 = 0.7853981633974483dd;
	const _Decimal64 PI_1_2 = 1.570796326794897dd;
	const _Decimal64 PI_3_4 = 2.356194490192345dd;
	int signx, signy, p10x, p10y, specialx, specialy;
	uint64_t mantx, manty;

	specialy = decode64 (&y, &manty, &p10y, &signy);
	specialx = decode64 (&x, &mantx, &p10x, &signx);

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

	int d10x = p10x + u64_digits (mantx);
	int d10y = p10y + u64_digits (manty);

	if (d10x - d10y > 20) {
		//  The stub answer could underflow.
		_Decimal64 q = y / x;
		return signx
			? (signy ? -M_PID - q : M_PID + q)
			: q;
	}

	if (d10y - d10x > 20) {
		return copysignD (PI_1_2, y);
	}

	if (d10x >= DBL_MAX_10_EXP - 20) {
		y = scalbnD (y, -DBL_MAX_10_EXP);
		x = scalbnD (x, -DBL_MAX_10_EXP);
	} else if (d10x <= DBL_MIN_10_EXP + 20) {
		y = scalbnD (y, -DBL_MIN_10_EXP);
		x = scalbnD (x, -DBL_MIN_10_EXP);
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
	static const _Decimal64 lg10_h = 3.32192809488dd;  // 12 digits
	static const _Decimal64 lg10_l = 7.362347870319429e-12dd;
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

	special = decode64_norm (&x, &mant, &p10, &sign);
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

	p2 = 0;
	while ((mant & 1) == 0) {
		mant >>= 1;
		p2++;
	}

	if (base == 2) {
		// Is x an exact power of two?  (Not strictly necessary,
		// but cheap.)

		if (mant == 1 && p10 == 0) {
			// A positive exact power
			return p2;
		} else if (p10 < 0 && p10 >= -DECIMAL64_MANT_DIG &&
			   mant == u64_pow5_table[-p10]) {
			// On the negative side we test mant==5^(-p10).
			return p10;
		}
	}

	bits = 63 - __builtin_clzl (mant);
	dx = ldexp (mant, -bits);
	p2 += bits;
	if (dx > 1.41) {
		dx /= 2;
		p2++;
	}

	// Always go via log10.  p10 can be large (several hundred), so
	// computing log2(x) as "p2 + log2(10) * p10 + log2(dx)" multiplies
	// that large p10 by the *imprecise* (16-digit) constant log2(10).
	// giving an absolute error that grows with |p10|; when x happens to
	// be close to a power of 2, that absolute error swamps the (small)
	// true result.
	p10 += (p2 * 77 + 128) / 256;
	_Decimal64 residual = ((_Decimal64)(log10 (dx)) + res[p2]);
	switch (base) {
	case 10: return p10 + residual;
	case  3: return (p10 + residual) * M_LN10D;
	default:
	case  2: {
		_Decimal64 t1 = lg10_h * p10;  // Exact due to |p10| < 1000
		_Decimal64 t2 = lg10_h * residual;
		_Decimal64 t34 = lg10_l * (p10 + residual);
		return t1 + (t2 + t34);
	}
	}
}

/**
 * log10D:
 * @x: argument
 *
 * Base-10 logarithm.
 *
 * Returns: The base-10 logarithm of @x.
 */
// Note: log10D(-42) = +NaN            <-- inconsistent
_Decimal64
log10D (_Decimal64 x)
{
	return log_helper (x, 10);
}

/**
 * log2D:
 * @x: argument
 *
 * Base-2 logarithm.
 *
 * Returns: The base-2 logarithm of @x.
 */
// Note: log2D(-42) = -NaN
_Decimal64
log2D (_Decimal64 x)
{
	return log_helper (x, 2);
}


/**
 * logD:
 * @x: argument
 *
 * Natural logarithm.
 *
 * Returns: The natural logarithm of @x.
 */
// Note: logD(-42) = -NaN
_Decimal64
logD (_Decimal64 x)
{
	return log_helper (x, 3);
}

/**
 * log1pD:
 * @x: argument
 *
 * log(1 + x).
 *
 * More accurate than logD(1+@x) for small @x.
 *
 * Returns: The natural logarithm of 1+@x.
 */
// Note: log1pD(-43) = -NaN
_Decimal64
log1pD (_Decimal64 x)
{
	if (-0.5dd < x && x < 1) {
		_Decimal64 ax = fabsD (x);
		// x - x^2/2 + ... so this is fine:
		if (ax <= 0.01dd * (DECIMAL64_EPSILON * DECIMAL64_EPSILON))
			return x;
		return log1p (x);
	} else
		return logD (x + 1);
}

/**
 * expD:
 * @x: argument
 *
 * Exponential function.
 *
 * Returns: e raised to @x.
 */
_Decimal64
expD (_Decimal64 x)
{
	return exp_helper (x, 1);
}

/**
 * expm1D:
 * @x: argument
 *
 * exp(x) - 1.
 *
 * More accurate than expD(@x)-1 for small @x.
 *
 * Returns: e^@x - 1.
 */
_Decimal64
expm1D (_Decimal64 x)
{
	// No need to handle negative overflow because of horizontal tangent
	if (fabsD (x) <= DECIMAL64_EPSILON)
		return x;
	else if (x > 50)
		return exp_helper (x, 1);  // -1 isn't going to make a difference in this range
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

// Is (mant,p10) exactly representable as a double?
// Prerequisite: (mant,p10) normalized
static gboolean
qrepdbl (uint64_t mant, int p10)
{
	g_return_val_if_fail (mant <= DECIMAL64_MAX_MANT, FALSE);

	if (mant == 0)
		return TRUE;

	if (p10 >= 0) {
		if (p10 > 22)
			return FALSE;

		while ((mant & 1) == 0)
			mant >>= 1;

		uint64_t pow5 = u64_pow5_table[p10];
		return mant <= ((1ull << 53) - 1) / pow5;
	} else {
		int k = -p10;
		// Cannot have too many digits after decimal point
		if (k > DECIMAL64_MANT_DIG)
			return FALSE;

		// Fractional part must multiple of 5^k
		uint64_t pow5 = u64_pow5_table[k];
		if (mant % pow5 != 0)
			return FALSE;

		// We must have room for the rest.
		mant /= pow5;
		return mant < (1ull << 53);
	}
}

static uint64_t
ipow_u64 (uint64_t x, unsigned e)
{
	uint64_t r = 1;
	uint64_t f = x;
	while (e > 1) {
		if (e & 1)
			r *= f;
		f *= f;
		e >>= 1;
	}
	if (e & 1)
		r *= f;
	return r;
}


/**
 * powD:
 * @x: base
 * @y: exponent
 *
 * Raises to a power.
 *
 * Follows the special cases of C99 Annex F: powD(x,0) and powD(1,y) are 1
 * even for NaN arguments, a negative finite @x with a non-integer @y gives
 * a NaN, and the signs and infinities for zero and infinite operands follow
 * the usual odd-integer rules.  Integer exponents of moderate size are
 * handled with extra care so that exact results, such as powers of ten,
 * come out exactly.  Overflow gives an infinity and underflow gives zero.
 *
 * Returns: @x raised to the power @y.
 */
_Decimal64
powD (_Decimal64 x, _Decimal64 y)
{
	int ysign;
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

	if (!isfiniteD (y)) {
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

	int qinty = isint (y);

	int p10x, signx;
	uint64_t mantx;
	(void)decode64_norm (&x, &mantx, &p10x, &signx);
	if (signx && !qinty)
		return NAN;

	// End of mandated special cases

	if (qinty) {
		// A few special cases where we can do a lot better than
		// going via plain pow.
		if (y == -1) return 1 / x;
		if (y == 1) return x;
		if (y == 2) return x * x;

		_Decimal64 ay = fabsD (y);
		int iy = (ay < 1000 ? (int)y : 1000);
		int iay = (iy < 0 ? -iy : iy);
		int digits_needed = iay * u64_digits (mantx);
		if (digits_needed <= 19 && iy > 0) {
			// We could do a 128-bit version of this but conversion
			// from uint128_t (or whatever it's called) requires
			// gcc 15 or higher so we'd have to test for it.
			z = scalbnD (ipow_u64 (mantx, iay), p10x * iy);
			goto do_sign;
		}
		if (!qrepdbl (mantx, p10x) &&
		    qrepdbl (mantx, 0) &&
		    digits_needed < DBL_MAX_10_EXP - 10) {
			// x is not representable as a double
			// y is a smallish integer
			// mantx is representable as a double
			// mantx^y will not overflow
			z = scalbnD (pow (mantx, y), p10x * iy);
			goto do_sign;
		}
	}

	if (signx) x = -x;

	if (x == 10 && fabsD (y) <= G_MAXINT) {
		// This could be extended to x being any integer power of 10
		int iy = (int)roundD (y);
		_Decimal64 dy = y - iy;
		z = scalbnD (pow (x, dy), iy);
	} else if (x == 2 && qinty && fabsD (y) <= G_MAXINT) {
		// This could be extended to x being any integer power of 2
		z = ldexpD (1, (int)y);
	} else {
		z = pow (x, y);
		if (z == (_Decimal64)INFINITY || z < (_Decimal64)DBL_MIN) {
			// Overflow or near-underflow.  Retry
			z = pow (x, y / 2);
			z *= z;
		}
	}

do_sign:
	return (signx && qinty < 0) ? -z : z;
}

/**
 * modfD:
 * @x: value to split
 * @y: (out): integer part
 *
 * Splits into integer and fractional parts.
 *
 * Stores the integer part (as truncD()) in *@y and returns the fractional
 * part, both carrying the sign of @x.  For an infinity, *@y is the infinity
 * and the return value is a zero of the same sign.  For a NaN, both are
 * that NaN.
 *
 * Returns: The fractional part of @x.
 */
_Decimal64
modfD (_Decimal64 x, _Decimal64 *y)
{
	if (!isfiniteD (x)) {
		*y = x;
		return isnanD (x) ? x : copysignD (0, x);
	}

	*y = truncD (x);
	return copysignD (x - *y, x);
}

/**
 * fmodD:
 * @x: dividend
 * @y: divisor
 *
 * Floating-point remainder.
 *
 * Computes @x - n*@y where n is the integer obtained by truncating @x/@y,
 * and where the result has the sign of @x and magnitude less than |@y|.
 * The computation is exact: no rounding error occurs (except that a
 * subnormal result is rounded to the subnormal grid).  If @x is infinite or
 * a NaN, or if @y is zero or a NaN, the result is a NaN.  If @y is infinite
 * the result is @x.  If @x is zero the result is @x.
 *
 * Returns: The remainder of @x divided by @y.
 */
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

/**
 * sqrtD:
 * @x: argument
 *
 * Square root.
 *
 * Compute the square root of @x.  Note that by convention, a nagative
 * zero is passed through unchanged.
 *
 * Returns: The non-negative square root of @x.
 */
_Decimal64
sqrtD (_Decimal64 x)
{
	int s = 0;

	if (x < 0)
		return -(_Decimal64)NAN;
	if (x == 0 || !isfiniteD (x))
		return x;

	if (x <= (_Decimal64)DBL_MIN) {
		x = scalbnD (x, 300);
		s = -150;
	} else if (x >= (_Decimal64)DBL_MAX) {
		x = scalbnD (x, -300);
		s = +150;
	}

	_Decimal64 r = sqrt (x);
	// Newton step
	r = (r + x / r) / 2;

	r = scalbnD (r, s);
	return r;
}

/**
 * cbrtD:
 * @x: argument
 *
 * Cube root.
 *
 * Returns: The real cube root of @x, with the sign of @x.
 */
_Decimal64
cbrtD (_Decimal64 x)
{
	_Decimal64 ax;
	int s = 0;

	if (x == 0 || !isfiniteD (x))
		return x;

	ax = fabsD (x);
	if (ax <= (_Decimal64)DBL_MIN) {
		x = scalbnD (x, 300);
		s = -100;
	} else if (ax >= (_Decimal64)DBL_MAX) {
		x = scalbnD (x, -300);
		s = 100;
	}

	_Decimal64 r = cbrt (x);
	// Newton step
	r = (2 * r + x / (r * r)) / 3;

	return scalbnD (r, s);
}

/**
 * hypotD:
 * @x: first leg
 * @y: second leg
 *
 * Euclidean distance.
 *
 * Computes sqrt(@x^2+@y^2) without undue overflow or underflow, so the
 * result is correct for every pair whose true result is representable.
 * Signs are ignored.  If either argument is an infinity the result is
 * +infinity, even if the other is a NaN.  Otherwise, if either is a NaN the
 * result is a NaN.
 *
 * Returns: The length of the hypotenuse.
 */
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

/**
 * jnD:
 * @n: order
 * @x: argument
 *
 * Bessel function of the first kind, integer order.
 *
 * Defined for negative @x by J_n(-x) = (-1)^n J_n(x) and for negative @n by
 * J_-n = (-1)^n J_n.  Tiny arguments are handled by the leading term of the
 * series, so no underflow of @x occurs.  Otherwise the computation goes via
 * double, so the argument is rounded to double: the result is accurate for
 * |@x| up to about 1e22, beyond which it has effectively lost its phase
 * (and is unreliable; see the FIXME in the source).  The limit for large
 * |@x| is 0.
 *
 * Returns: J_@n(@x).
 */
_Decimal64
jnD (int n, _Decimal64 x)
{
	_Decimal64 ax = fabsD (x);

	if (ax > 0 && ax <= DECIMAL64_EPSILON && n != 0 && n != G_MININT) {
		int an = n > 0 ? n : -n;

		_Decimal64 f1 = powD (x, an);
		_Decimal64 r = f1 / (_Decimal64)(ldexp (tgamma (an + 1), an));

		if (n < 0 && (an & 1))
			r = -r;
		return r;
	}

	// FIXME: need to handle large values.  Going via "double" is no good.

	return jn (n, x);
}

/**
 * ynD:
 * @n: order
 * @x: argument
 *
 * Bessel function of the second kind, integer order.
 *
 * Defined for @x >= 0 only: a negative @x gives a NaN and ynD(n,0) is
 * -infinity for @n >= 0.  For negative @n, Y_-n = (-1)^n Y_n.  Tiny
 * positive arguments (up to #DECIMAL64_EPSILON) use the leading term of the
 * expansion about zero, so, unlike a cast to double, arguments below the
 * range of double still give correct, finite results; a result too large
 * for Decimal64 gives an infinity.  Otherwise the computation goes via
 * double, so the argument is rounded to double: the result is accurate for
 * @x up to about 1e22, beyond which it has effectively lost its phase (and
 * is unreliable; see the FIXME in the source).  The limit for large @x is
 * 0.
 *
 * Returns: Y_@n(@x).
 */
_Decimal64
ynD (int n, _Decimal64 x)
{
	// Small positive arguments.  Casting to double flushes anything
	// below ~4.9e-324 to zero, where yn gives -infinity, but the true
	// value is (for n=0) only a moderate number of order ln(x) and (for
	// n!=0) a huge but usually finite one that Decimal64 can represent.
	// We use the leading terms of the expansion about 0 instead, which
	// is accurate to full precision this close to zero (the neglected
	// relative terms are O(x^2 log x)).  Like jnD, this covers far more
	// than just the range that would underflow in double.
	//
	// x == 0 and x < 0 are left to yn: it already gives -inf/+inf
	// (following the parity rule for negative n) and NaN respectively.
	if (x > 0 && x <= DECIMAL64_EPSILON && n != G_MININT) {
		int an = n > 0 ? n : -n;
		_Decimal64 r;

		if (an == 0) {
			// Y0(x) ~ 2/pi * (ln(x/2) + gamma)
			//       = 2/pi * (ln(x) + (gamma - ln(2)))
			r = M_2_PID * (logD (x) + M_EULER_LN2D);
		} else {
			// Yn(x) ~ -(n-1)!/pi * (2/x)^n
			//
			// Divide by x one factor at a time rather than
			// forming (2/x)^n, so that we do not overflow
			// spuriously for results just below the maximum.
			// For n>=172 tgamma overflows, but then so does
			// the result for any x in this range.
			_Decimal64 k = M_1_PID *
				(_Decimal64)(ldexp (tgamma (an), an));
			r = -((k / x) / powD (x, an - 1));
		}

		if (n < 0 && (an & 1))
			r = -r;
		return r;
	}

	// FIXME: need to handle large values.  Going via "double" is no good.
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
		// (or we have really weird endianness going on)
		g_printerr ("Decimal64 numbers are not bis encoded.\n");
		g_printerr ("(Got 0x%lx, expected 0x%lx)\n", u64, expected);
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
