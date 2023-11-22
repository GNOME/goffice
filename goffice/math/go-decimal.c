 /*
 * go-decimal.c:  Support for Decimal64 numbers
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
 */

// This file contains
// * libm functions for the _Decimal64 type using a "D" suffix
//   - some have proper implement
//   - some have stubs that just defer to "double" versions
// * printf hooks for _Decimal64 and _Decimal128
//   - no intl support yet
//   - no 'a' and 'A' formats
//   - no grouping flag (single quote)
//   - THIS IS INCOMPATIBLE WITH THE "quadmath" LIBRARY due to glibc
//     deficiency.


#include <math/go-decimal.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <printf.h>

// ---------------------------------------------------------------------------

// To implement _Decimal64 versions of the usual suspects we simply
// go to double and back.  If something stands out as not accurate
// enough we'll have to come up with something better.

#define STUB1(FUNC)							\
  _Decimal64 FUNC ## D (_Decimal64 x) {					\
	  return (_Decimal64) (FUNC ((double)x));			\
  }

#define STUB2(FUNC)							\
  _Decimal64 FUNC ## D (_Decimal64 x, _Decimal64 y) {			\
	  return (_Decimal64) (FUNC ((double)x, (double) y));		\
  }

STUB1(acos)
STUB1(acosh)
STUB1(asin)
STUB1(asinh)
STUB1(atan)
STUB2(atan2)
STUB1(atanh)
STUB1(ceil)
STUB1(cos)
STUB1(cosh)
STUB1(erf)
STUB1(exp)
STUB1(expm1)
STUB1(floor)
STUB1(lgamma)
STUB2(fmod)
STUB2(hypot)
STUB1(log)
STUB1(log10)
STUB1(log1p)
STUB1(round)
STUB2(pow)
STUB1(sin)
STUB1(sinh)
STUB1(sqrt)
STUB1(tan)
STUB1(tanh)

_Decimal64 jnD (int n, _Decimal64 x) { return jn (n, x); }
_Decimal64 ynD (int n, _Decimal64 x) { return yn (n, x); }
_Decimal64 lgamma_rD (_Decimal64 x, int *signp) { return lgamma_r(x, signp); }

#if 0
	;
#endif

// ---------------------------------------------------------------------------

// Decode a _Decimal64 assuming binary integer significant encoding
static int
decode64_bis (_Decimal64 const *args0, uint64_t *pmant, int *pp10, int *sign)
{
	const int exp_bias = -398;
	uint64_t d64, mant;
	int special = 0, p10 = 0;

	memcpy (&d64, args0, sizeof(d64));

	if (sign) *sign = (d64 >> 63);

	if (((d64 >> 59) & 15) == 15) {
		special = ((d64 >> 58) & 1)
			? 1  // NAN
			: 2; // INF
	} else {
		if (((d64 >> 61) & 3) == 3) {
			p10 = exp_bias + ((d64 >> 51) & 0x3ff);
			mant = (d64 & ((1ul << 51) - 1)) | (4ul << 51);
		} else {
			p10 = exp_bias + ((d64 >> 53) & 0x3ff);
			mant = d64 & ((1ul << 53) - 1);
		}
		if (mant > 9999999999999999ull)
			special = 3; // Invalid (>= 10^17)
	}

	if (pp10) *pp10 = (special ? 0 : p10);
	if (pmant) *pmant = (special ? 0 : mant);

	return special;
}

// Decode a _Decimal128 assuming binary integer significant encoding
static int
decode128_bis (_Decimal128 const *args0, uint64_t *pmantu, uint64_t *pmantl,
	       int *pp10, int *sign)
{
	const int exp_bias = -6176;
	uint64_t l64, u64, mantl, mantu;
	int special = 0, p10 = 0;

	// Hmm...  Little endian, I hope
	memcpy (&l64, (uint64_t const *)args0, sizeof(l64));
	memcpy (&u64, (uint64_t const *)args0 + 1, sizeof(u64));

	if (sign) *sign = (u64 >> 63);

	if (((u64 >> 59) & 15) == 15) {
		special = ((u64 >> 58) & 1)
			? 1  // NAN
			: 2; // INF
	} else {
		if (((u64 >> 61) & 3) == 3) {
			p10 = exp_bias + ((u64 >> 47) & 0x3fff);
			mantu = (u64 & ((1ul << 47) - 1)) | (4ul << 47);
		} else {
			p10 = exp_bias + ((u64 >> 49) & 0x3fff);
			mantu = u64 & ((1ul << 49) - 1);
		}
		mantl = l64;
		if (mantu > 0x1ed09bead87c0ull ||
		    (mantu == 0x1ed09bead87c0ull &&
		     mantl > 0x378d8e63ffffffffull))
			special = 3; // Invalid (>= 10^34)
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
decimal_arginfo(const struct printf_info *info, size_t n,
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
do_round (char *buf, int ix)
{
	if (ix < 0) {
		ix = 0;
	} else {
		char rc = buf[ix];

		if (rc >= '5') {
			int i = ix - 1;
			while (i >= 0 && (buf[i])++ == '9')
				i--;
			if (i == -1) {
				buf[0] = '1';
				memset (buf + 1, '0', ix);
				ix++;
			}
		}
	}

	if (ix == 0)
		buf[ix++] = '0';
	return ix;
}

static int
decimal_format(FILE *stream, const struct printf_info *info,
	       const void *const *args)
{
	char buffer[1024];
	int special, p10, sign;
	int len = 0;
	int qupper = (info->spec <= 'Z');
	char signchar;

	if (info->user & decimal64_modifier) {
		_Decimal64 const *args0 = *(_Decimal64 **)(args[0]);
		uint64_t mant;
		special = decode64_bis (args0, &mant, &p10, &sign);
		if (special == 0) render128 (buffer, 0, mant);
	} else if (info->user & decimal128_modifier) {
		_Decimal128 const *args0 = *(_Decimal128 **)(args[0]);
		uint64_t mantu, mantl;
		special = decode128_bis (args0, &mantu, &mantl, &p10, &sign);
		if (special == 0) render128 (buffer, mantu, mantl);
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
	case 0: {
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
			if (effp10 < -4 || effp10 > prec) {
				estyle = 1;
				prec--;
			} else {
				fstyle = 1;
				if (effp10 >= 0) {
					prec -= effp10;
				} else {
					prec -= effp10 + 1;
				}
			}
		}

		if (estyle) {
			int decimals = len - 1, ap10;
			p10 += decimals;

			if (decimals > prec) {
				int cut = decimals - prec;
				int ix = len - cut;
				len = do_round (buffer, ix);
				if (ix >= 0 && len != ix) {
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
				memmove (buffer + 2, buffer + 1, len - 1);
				buffer[1] = '.';
				len++;
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
				len = do_round (buffer, len - cut);
				decimals = prec;
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
				memmove (buffer + diff + 2, buffer, len);
				memset (buffer, '0', diff + 2);
				buffer[1] = '.';
				len += diff + 2;
			} else if (decimals || info->alt) {
				int need0 = (decimals == len);
				memmove (buffer + len - decimals + 1 + need0,
					 buffer + len - decimals,
					 decimals);
				if (need0) buffer[0] = '0';
				buffer[len + need0 - decimals] = '.';
				len += 1 + need0;
			}
			buffer[len] = 0;
		} else {
			// Shouldn't happen
			buffer[0] = '?';
			len = 1;
		}
		break;
	}
	case 1:
		strcpy (buffer, (qupper ? "NAN" : "nan"));
		len = 3;
		break;
	case 2:
		strcpy (buffer, (qupper ? "INF" : "inf"));
		len = 3;
		break;
	case 3:
		// Invalid
		buffer[0] = '0';
		len = 1;
		p10 = 0;
		break;
	}

	if (signchar) len++;

	while (!info->left && len < info->width) {
		len++;
		putc (' ', stream);
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

int
isnanD (_Decimal64 x)
{
	return decode64_bis (&x, NULL, NULL, NULL) == 1;
}

int
finiteD (_Decimal64 x)
{
	return (decode64_bis (&x, NULL, NULL, NULL) ^ 2) < 2;
}

int
signbitD (_Decimal64 x)
{
	int sign;
	(void)decode64_bis (&x, NULL, NULL, &sign);
	return sign;
}

_Decimal64
fabsD (_Decimal64 x)
{
	return signbitD (x) ? -x : x;
}

_Decimal64
nextafterD (_Decimal64 x, _Decimal64 y)
{
	int special, qadd, sign, p10;
	uint64_t mant;

	special = decode64_bis (&x, &mant, &p10, &sign);
	if (special == 1)
		return x;  // NAN

	if (x < y)
		qadd = 1;
	else if (x > y)
		qadd = 0;
	else
		return y; // Either equal or y is NAN

	if (special == 2) {
		// Inf
		return sign ? -DECIMAL64_MAX : DECIMAL64_MAX;
	}

	if (mant == 0) {
		// Denormal
		return qadd ? 1e-398dd : -1e-398dd;
	}

	// FIXME

	abort ();
}

_Decimal64
ldexpD (_Decimal64 x, int e)
{
	return x * (_Decimal64)(ldexp(1, e));
}

_Decimal64
frexpD (_Decimal64 x, int *e)
{
	return frexp (x, e);
}

_Decimal64
strtoDd (const char *s, char **end)
{
	// FIXME
	return strtold (s, end);
}

// ---------------------------------------------------------------------------

void
_go_decimal_init (void)
{
	init_decimal_printf_support ();
}

// ---------------------------------------------------------------------------
