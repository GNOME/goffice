/*
 * go-dtoa.c: double-to-string conversion.
 *
 * Copyright 2014 Morten Welinder <terra@gnome.org>
 *
 *
 *
 * Large portions of this code was copied from http://www.musl-libc.org/
 * under the following license:
 *
 * Copyright © 2005-2014 Rich Felker, et al.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <goffice/goffice-config.h>
#include <goffice/math/go-math.h>
#include "go-dtoa.h"
#include "go-ryu.h"
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>
#ifdef GOFFICE_WITH_DECIMAL64
#include <printf.h>
#endif

#if (defined(i386) || defined(__i386__) || defined(__i386) || defined(__x86_64__) || defined(__x86_64)) && HAVE_FPU_CONTROL_H
#define ENSURE_FPU_STATE
#include <fpu_control.h>
#endif

typedef GString FAKE_FILE;

#ifndef GOFFICE_WITH_LONG_DOUBLE
#define go_finitel isfinite
#endif


/* musl code starts here */

/* Some useful macros */

#ifdef MUSL_ORIGINAL
#define MAX(a,b) ((a)>(b) ? (a) : (b))
#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define CONCAT2(x,y) x ## y
#define CONCAT(x,y) CONCAT2(x,y)
#endif

/* Convenient bit representation for modifier flags, which all fall
 * within 31 codepoints of the space character. */

#define ALT_FORM   (1U<<('#'-' '))
#define ZERO_PAD   (1U<<('0'-' '))
#define LEFT_ADJ   (1U<<('-'-' '))
#define PAD_POS    (1U<<(' '-' '))
#define MARK_POS   (1U<<('+'-' '))
#define GROUPED    (1U<<('\''-' '))
#ifdef MUSL_ORIGINAL
#define FLAGMASK (ALT_FORM|ZERO_PAD|LEFT_ADJ|PAD_POS|MARK_POS|GROUPED)
#else
#define FLAG_TIES_AWAY_0 (1U<<('"'-' '))
#define FLAG_SHORTEST (1U<<('!'-' '))
#define FLAG_TRUNCATE (1U<<('='-' '))
#define FLAG_ASCII (1U<<(','-' '))
#endif


static void out(FAKE_FILE *f, const char *s, size_t l)
{
#ifdef MUSL_ORIGINAL
  __fwritex((void *)s, l, f);
#else
  g_string_append_len (f, s, l);
#endif
}

#ifndef MUSL_ORIGINAL
static void
outdecimal (FAKE_FILE *f, int fl)
{
	if (fl & FLAG_ASCII)
		out(f, ".", 1);
	else {
		GString const *dec = go_locale_get_decimal();
		out(f, dec->str, dec->len);
	}
}
#endif


static void pad(FAKE_FILE *f, char c, int w, int l, int fl)
{
	char pad[256];
	if (fl & (LEFT_ADJ | ZERO_PAD) || l >= w) return;
	l = w - l;
#ifdef MUSL_ORIGINAL
	memset(pad, c, l>sizeof pad ? sizeof pad : l);
	for (; l >= sizeof pad; l -= sizeof pad)
		out(f, pad, sizeof pad);
#else
	/* Just kill some warnings */
	memset(pad, c, (size_t)l>sizeof pad ? sizeof pad : (size_t)l);
	for (; (size_t)l >= sizeof pad; l -= sizeof pad)
		out(f, pad, sizeof pad);
#endif
	out(f, pad, l);
}

static const char xdigits[16] = {
	"0123456789ABCDEF"
};

static char *fmt_u(uintmax_t x, char *s)
{
	unsigned long y;
	for (   ; x>ULONG_MAX; x/=10) *--s = '0' + x%10;
	for (y=x;           y; y/=10) *--s = '0' + y%10;
	return s;
}

/* Do not override this check. The floating point printing code below
 * depends on the float.h constants being right. If they are wrong, it
 * may overflow the stack. */
#if LDBL_MANT_DIG == 53
typedef char compiler_defines_long_double_incorrectly[9-(int)sizeof(long double)];
#endif

static int fmt_fp(FAKE_FILE *f, long double y, int w, int p, int fl, int t)
{
	uint32_t big[(LDBL_MANT_DIG+28)/29 + 1          // mantissa expansion
		+ (LDBL_MAX_EXP+LDBL_MANT_DIG+28+8)/9]; // exponent expansion
	uint32_t *a, *d, *r, *z;
#ifdef MUSL_ORIGINAL
	int e2=0, e, i, j, l;
#else
	/* Make i unsigned kills some warnings */
	int e2=0, e, j, l;
	unsigned i;
#endif
	char buf[9+LDBL_MANT_DIG/4], *s;
	const char *prefix="-0X+0X 0X-0x+0x 0x";
	int pl;
	char ebuf0[3*sizeof(int)], *ebuf=&ebuf0[3*sizeof(int)], *estr;

	pl=1;
	if (signbit(y)) {
		y=-y;
	} else if (fl & MARK_POS) {
		prefix+=3;
	} else if (fl & PAD_POS) {
		prefix+=6;
	} else prefix++, pl=0;

	if (!go_finitel(y)) {
		const char *s = (t&32)?"inf":"INF";
		if (y!=y) s=(t&32)?"nan":"NAN";
		pad(f, ' ', w, 3+pl, fl&~ZERO_PAD);
		out(f, prefix, pl);
		out(f, s, 3);
		pad(f, ' ', w, 3+pl, fl^LEFT_ADJ);
		return MAX(w, 3+pl);
	}

	y = frexpl(y, &e2) * 2;
	if (y) e2--;

	if ((t|32)=='a') {
		if (t&32) prefix += 9;
		pl += 2;

		// From from musl commit 24ebbbde ("printf: fix edge case where
		// hex float precision was not honored"), included.
		if (p>=0 && p<(LDBL_MANT_DIG-1+3)/4) {
			double round = scalbn(1, LDBL_MANT_DIG-1-(p*4));
			if (*prefix=='-') {
				y=-y;
				y-=round;
				y+=round;
				y=-y;
			} else {
				y+=round;
				y-=round;
			}
		}

		estr=fmt_u(e2<0 ? -e2 : e2, ebuf);
		if (estr==ebuf) *--estr='0';
		*--estr = (e2<0 ? '-' : '+');
		*--estr = t+('p'-'a');

		s=buf;
		do {
			int x=y;
			*s++=xdigits[x]|(t&32);
			y=16*(y-x);
			if (s-buf==1 && (y||p>0||(fl&ALT_FORM))) *s++='.';
		} while (y);

		if (p && s-buf-2 < p)
			l = (p+2) + (ebuf-estr);
		else
			l = (s-buf) + (ebuf-estr);

		pad(f, ' ', w, pl+l, fl);
		out(f, prefix, pl);
		pad(f, '0', w, pl+l, fl^ZERO_PAD);
		out(f, buf, s-buf);
		pad(f, '0', l-(ebuf-estr)-(s-buf), 0, 0);
		out(f, estr, ebuf-estr);
		pad(f, ' ', w, pl+l, fl^LEFT_ADJ);
		return MAX(w, pl+l);
	}
	if (p<0) p=6;

	if (y) y *= 0x1p28, e2-=28;

	if (e2<0) a=r=z=big;
	else a=r=z=big+sizeof(big)/sizeof(*big) - LDBL_MANT_DIG - 1;

	do {
		*z = y;
		y = 1000000000*(y-*z++);
	} while (y);

	while (e2>0) {
		uint32_t carry=0;
		int sh=MIN(29,e2);
		for (d=z-1; d>=a; d--) {
			uint64_t x = ((uint64_t)*d<<sh)+carry;
			*d = x % 1000000000;
			carry = x / 1000000000;
		}
		if (carry) *--a = carry;
		while (z>a && !z[-1]) z--;
		e2-=sh;
	}
	while (e2<0) {
		uint32_t carry=0, *b;
		int sh=MIN(9,-e2), need=1+(p+LDBL_MANT_DIG/3+8)/9;
		for (d=a; d<z; d++) {
			uint32_t rm = *d & ((1<<sh)-1);
			*d = (*d>>sh) + carry;
			carry = (1000000000>>sh) * rm;
		}
		if (!*a) a++;
		if (carry) *z++ = carry;
		/* Avoid (slow!) computation past requested precision */
		b = (t|32)=='f' ? r : a;
		if (z-b > need) z = b+need;
		e2+=sh;
	}

	if (a<z) for (i=10, e=9*(r-a); *a>=i; i*=10, e++);
	else e=0;

	/* Perform rounding: j is precision after the radix (possibly neg) */
	j = p - ((t|32)!='f')*e - ((t|32)=='g' && p);
	if (j < 9*(z-r-1)) {
		uint32_t x;
		/* We avoid C's broken division of negative numbers */
		d = r + 1 + ((j+9*LDBL_MAX_EXP)/9 - LDBL_MAX_EXP);
		j += 9*LDBL_MAX_EXP;
		j %= 9;
		for (i=10, j++; j<9; i*=10, j++);
		x = *d % i;
		/* Are there any significant digits past j? */
		if (x || d+1!=z) {
#ifdef MUSL_ORIGINAL
			long double round = CONCAT(0x1p,LDBL_MANT_DIG);
#else
			long double round = 2 / LDBL_EPSILON;
#endif
			long double small;
#ifdef MUSL_ORIGINAL
			if (*d/i & 1) round += 2;
#else
			/*
			 * For ties-away-from-zero, just pretend we have
			 * an odd digit.
			 *
			 * Fix from musl commit 51ab6db4 ("fix float formatting
			 * of some exact halfway cases"), included.
			 */
			if ((fl & FLAG_TIES_AWAY_0) ||
			    (*d/i & 1) ||
			    (i == 1000000000 && d > a && (d[-1] & 1)))
				round += 2;
#endif
			if (x<i/2) small=0x0.8p0;
			else if (x==i/2 && d+1==z) small=0x1.0p0;
			else small=0x1.8p0;
			if (pl && *prefix=='-') round*=-1, small*=-1;
			*d -= x;
			/* Decide whether to round by probing round+small */
			if (round+small != round) {
				*d = *d + i;
				while (*d > 999999999) {
					*d--=0;
					if (d<a) *--a=0;
					(*d)++;
				}
				for (i=10, e=9*(r-a); *a>=i; i*=10, e++);
			}
		}
		if (z>d+1) z=d+1;
	}
	for (; z>a && !z[-1]; z--);

	if ((t|32)=='g') {
		if (!p) p++;
		if (p>e && e>=-4) {
			t--;
			p-=e+1;
		} else {
			t-=2;
			p--;
		}
		if (!(fl&ALT_FORM)) {
			/* Count trailing zeros in last place */
			if (z>a && z[-1]) for (i=10, j=0; z[-1]%i==0; i*=10, j++);
			else j=9;
			if ((t|32)=='f')
				p = MIN(p,MAX(0,9*(z-r-1)-j));
			else
				p = MIN(p,MAX(0,9*(z-r-1)+e-j));
		}
	}
	l = 1 + p + (p || (fl&ALT_FORM));
	if ((t|32)=='f') {
		if (e>0) l+=e;
	} else {
		estr=fmt_u(e<0 ? -e : e, ebuf);
		while(ebuf-estr<2) *--estr='0';
		*--estr = (e<0 ? '-' : '+');
		*--estr = t;
		l += ebuf-estr;
	}

	pad(f, ' ', w, pl+l, fl);
	out(f, prefix, pl);
	pad(f, '0', w, pl+l, fl^ZERO_PAD);

	if ((t|32)=='f') {
		if (a>r) a=r;
		for (d=a; d<=r; d++) {
			char *s = fmt_u(*d, buf+9);
			if (d!=a) while (s>buf) *--s='0';
			else if (s==buf+9) *--s='0';
			out(f, s, buf+9-s);
		}
#ifdef MUSL_ORIGINAL
		if (p || (fl&ALT_FORM)) out(f, ".", 1);
#else
		if (p || (fl&ALT_FORM)) outdecimal(f, fl);
#endif
		for (; d<z && p>0; d++, p-=9) {
			char *s = fmt_u(*d, buf+9);
			while (s>buf) *--s='0';
			out(f, s, MIN(9,p));
		}
		pad(f, '0', p+9, 9, 0);
	} else {
		if (z<=a) z=a+1;
		for (d=a; d<z && p>=0; d++) {
			char *s = fmt_u(*d, buf+9);
			if (s==buf+9) *--s='0';
			if (d!=a) while (s>buf) *--s='0';
			else {
				out(f, s++, 1);
#ifdef MUSL_ORIGINAL
				if (p>0||(fl&ALT_FORM)) out(f, ".", 1);
#else
				if (p>0||(fl&ALT_FORM)) outdecimal(f, fl);
#endif
			}
			out(f, s, MIN(buf+9-s, p));
			p -= buf+9-s;
		}
		pad(f, '0', p+18, 18, 0);
		out(f, estr, ebuf-estr);
	}

	pad(f, ' ', w, pl+l, fl^LEFT_ADJ);

	return MAX(w, pl+l);
}

/* musl code ends here */

typedef enum {
	FP_DOUBLE,
	FP_LONG_DOUBLE,
	FP_DECIMAL64
} FloatType;

typedef union {
	long double ld;
#ifdef GOFFICE_WITH_DECIMAL64
	_Decimal64 d64;
#endif
} FloatValueType;


// Largest width or precision we accept.  That keeps the arithmetic in fmt_fp
// and friends away from integer overflow.
#define MAX_WIDTH_OR_PRECISION 999999999

// Parse a non-negative decimal number.  Returns FALSE on overflow.
static gboolean
parse_number (const char **pfmt, int *res)
{
	const char *fmt = *pfmt;
	int n = 0;

	while (g_ascii_isdigit (*fmt)) {
		int dgt = *fmt++ - '0';
		if (n > (MAX_WIDTH_OR_PRECISION - dgt) / 10)
			return FALSE;
		n = n * 10 + dgt;
	}

	*pfmt = fmt;
	*res = n;
	return TRUE;
}

// Returns NULL on success, otherwise a description of what is wrong with
// the format.  Nothing is consumed from args unless that is the case.
static const char *
parse_fmt (const char *fmt, va_list args, FloatType *fltyp,
	   int *w, int *p, int *fl, int *t, FloatValueType *d)
{
	*fltyp = FP_DOUBLE;
	d->ld = 0;
	*w = 1;
	*p = -1;
	*fl = 0;
	*t = 'g';

	while (1) {
		switch (*fmt) {
		case '0': *fl |= ZERO_PAD; fmt++; continue;
		case '^': *fl |= FLAG_TIES_AWAY_0; fmt++; continue;
		case '+': *fl |= MARK_POS; fmt++; continue;
		case '-': *fl |= LEFT_ADJ; fmt++; continue;
		case '!': *fl |= FLAG_SHORTEST; fmt++; continue;
		case '=': *fl |= FLAG_TRUNCATE; fmt++; continue;
		case ',': *fl |= FLAG_ASCII; fmt++; continue;
		}
		break;
	}

	if (*fmt == '*') {
		fmt++;
		*w = va_arg (args, int);
		if (*w < 0) {
			// As in printf, a negative width means '-' and a
			// positive width.
			*fl |= LEFT_ADJ;
			*w = (*w == G_MININT) ? G_MAXINT : -*w;
		}
		if (*w > MAX_WIDTH_OR_PRECISION)
			return "width is too large";
	} else if (g_ascii_isdigit (*fmt)) {
		if (!parse_number (&fmt, w))
			return "width is too large";
	}

	if (*fmt == '.') {
		fmt++;
		if (*fmt == '*') {
			fmt++;
			*p = va_arg (args, int);
			// As in printf, a negative precision is no precision
			if (*p < 0)
				*p = -1;
			else if (*p > MAX_WIDTH_OR_PRECISION)
				return "precision is too large";
		} else {
			if (!parse_number (&fmt, p))
				return "precision is too large";
		}
	}

	if (*fmt == 'L') {
#ifdef GOFFICE_WITH_LONG_DOUBLE
		*fltyp = FP_LONG_DOUBLE;
		fmt++;
#else
		return "long double is not supported by this build of goffice";
#endif
	}
#ifdef GOFFICE_WITH_DECIMAL64
	else if (*fmt == *GO_DECIMAL64_MODIFIER &&
		 (GO_DECIMAL64_MODIFIER[1] == 0 || g_str_has_prefix (fmt + 1, GO_DECIMAL64_MODIFIER + 1))) {
		*fltyp = FP_DECIMAL64;
		fmt += strlen (GO_DECIMAL64_MODIFIER);
	}
#endif

	// Note, the terminating NUL is not a conversion even though strchr
	// will find it.
	if (*fmt == 0)
		return "the conversion is missing";
	if (!strchr ("efgaEFGA", *fmt))
		return "unsupported flag, modifier, or conversion";
	*t = *fmt;

	if (*fltyp == FP_DOUBLE)
		d->ld = va_arg (args, double);
#ifdef GOFFICE_WITH_LONG_DOUBLE
	else if (*fltyp == FP_LONG_DOUBLE)
		d->ld = va_arg (args, long double);
#endif
#ifdef GOFFICE_WITH_DECIMAL64
	else if (*fltyp == FP_DECIMAL64)
		d->d64 = va_arg (args, _Decimal64);
#endif
	else
		g_assert_not_reached ();

	return NULL;
}

#ifdef GOFFICE_WITH_DECIMAL64
static void
fmt_d64 (GString *dst, _Decimal64 y, int w, int p, int fl, int t)
{
	// We're in here because we want ascii and round-away-from-zero handling

	size_t oldlen = dst->len;
	struct printf_info info;

	memset (&info, 0, sizeof (info));
	info.width = w;
	info.prec = p;
	info.spec = t;
	info.alt = (fl & ALT_FORM) != 0;
	info.left = (fl & LEFT_ADJ) != 0;
	info.showsign = (fl & MARK_POS) != 0;
	info.space = (fl & PAD_POS) != 0;
	info.pad = (fl & ZERO_PAD) ? '0' : ' ';
	info.i18n = (fl & GROUPED) != 0;

	_go_decimal_format (dst, y, &info);

	if (fl & FLAG_ASCII) {
		GString const *decimal = go_locale_get_decimal ();
		char *dpos = strstr (dst->str + oldlen, decimal->str);
		if (dpos && decimal->len) {
			size_t pos = dpos - dst->str;
			g_string_erase (dst, pos + 1, decimal->len - 1);
			dst->str[pos] = '.';
		}
	}
}
#endif

static void
fmt_shortest_unpadded (GString *dst, FloatValueType *d, int fl, int t, FloatType fltyp)
{
	size_t oldlen = dst->len;
	int n, e, ndec;
	char *epos, *dpos;
	gboolean use_e_notation;
	GString const *dec = go_locale_get_decimal();
	int prec;

	switch (fltyp) {
	case FP_DOUBLE:
		g_string_set_size (dst, 53 + oldlen + dec->len);
		n = go_ryu_d2s_buffered_n ((double)(d->ld), dst->str + oldlen);
		prec = 17;
		break;
#ifdef GOFFICE_WITH_LONG_DOUBLE
	case FP_LONG_DOUBLE:
		g_string_set_size (dst, 64 + oldlen + dec->len);
		n = go_ryu_ld2s_buffered_n (d->ld, dst->str + oldlen);
		prec = 21;
		break;
#endif
#ifdef GOFFICE_WITH_DECIMAL64
	case FP_DECIMAL64: {
		/*
		 * Decimal64 needs no shortest-digits algorithm: the value
		 * already is a decimal number with at most 16 digits.  Get
		 * those digits in e-notation and the C locale, drop trailing
		 * zeros from the mantissa and the sign and padding from the
		 * exponent.  That gives the same form as ryu produces,
		 * "[-]D[.DDD]E<exp>", and the code below takes it from there.
		 */
		char *epos0;
		char mant[32];
		size_t mlen;
		int exp10;
		char *dpos0;

		fmt_d64 (dst, d->d64, 0, 15, FLAG_ASCII, 'E');
		epos0 = strchr (dst->str + oldlen, 'E');
		if (!epos0) {
			// Infinity or NaN.  Redo as printf would do it.
			g_string_set_size (dst, oldlen);
			fmt_d64 (dst, d->d64, 0, -1, fl, t);
			return;
		}

		exp10 = atoi (epos0 + 1);
		mlen = epos0 - (dst->str + oldlen);
		g_assert (mlen < sizeof (mant));
		memcpy (mant, dst->str + oldlen, mlen);
		dpos0 = memchr (mant, '.', mlen);
		if (dpos0) {
			while (mlen > (size_t)(dpos0 - mant) + 1 && mant[mlen - 1] == '0')
				mlen--;
			if (mlen == (size_t)(dpos0 - mant) + 1)
				mlen--;  // Drop the decimal point too
		}

		g_string_set_size (dst, oldlen);
		g_string_append_len (dst, mant, mlen);
		g_string_append_printf (dst, "E%d", exp10);
		n = dst->len - oldlen;

		/*
		 * Make sure we have room for the locale's decimal below so the
		 * dpos and epos pointers stay valid.
		 */
		g_string_set_size (dst, 53 + oldlen + dec->len);
		prec = 16;
		break;
	}
#endif
	default:
		g_assert_not_reached ();
	}

	g_string_set_size (dst, oldlen + n);
	dpos = strchr (dst->str + oldlen, '.');
	epos = strchr (dst->str + oldlen, 'E');
	ndec = dpos
		? (epos
		   ? epos - (dpos + 1)
		   : (int)(dst->len - (dpos + 1 - dst->str)))
		: 0;
	if (!epos)
		return; // NaN etc.

	if (dpos && !(fl & FLAG_ASCII)) {
		int dlen = dec->len;
		const char *dstr = dec->str;
		*dpos = dstr[0];
		if (dlen > 1) {
			g_string_insert_len (dst, dpos - dst->str + 1,
					     dstr + 1, dlen - 1);
			epos += dlen - 1;
			// We allocated enough that dpos/epos are still good
		}
	}

	e = atoi (epos + 1);
	use_e_notation =
		(t | 32) == 'e' ||
		((t | 32) == 'g' && (e < -4 || e >= prec));
	if (use_e_notation) {
		// Downcase 'E', if needed
		if (t & 32) *epos = 'e';
		// Use printf rules for exponents
		if (e >= 0 && e <= 9)
			g_string_insert (dst, epos - dst->str + 1, "+0");
		else if (e >= 10)
			g_string_insert_c (dst, epos - dst->str + 1, '+');
		else if (e >= -9 && e < 0)
			g_string_insert_c (dst, epos - dst->str + 2, '0');
		return;
	} else {
		// Else f-notation.  Redo.
		int precision = MAX (0, ndec - e);
		t = (t & 32) | 'F';
		g_string_set_size (dst, oldlen);
#ifdef GOFFICE_WITH_DECIMAL64
		if (fltyp == FP_DECIMAL64)
			fmt_d64 (dst, d->d64, 1, precision, fl, t);
		else
#endif
			fmt_fp (dst, d->ld, 1, precision, fl, t);
	}
}

// Insert n copies of c at position pos.
static void
insert_repeated (GString *dst, size_t pos, char c, size_t n)
{
	char *fill = g_strnfill (n, c);
	g_string_insert_len (dst, pos, fill, n);
	g_free (fill);
}

// Format the shortest representation and then apply the '+', '-' and '0'
// flags and the width the same way printf would.  The width counts
// characters, not bytes, as the locale's decimal separator can be anything.
static void
fmt_shortest (GString *dst, FloatValueType *d, int w, int fl, int t, FloatType fltyp)
{
	size_t start = dst->len;
	size_t sign_len, len;

	fmt_shortest_unpadded (dst, d, fl, t, fltyp);

	// The f-notation code adds the sign for us.  Others do not.
	if ((fl & MARK_POS) && dst->str[start] != '-' && dst->str[start] != '+')
		g_string_insert_c (dst, start, '+');

	sign_len = (dst->str[start] == '-' || dst->str[start] == '+');
	len = g_utf8_strlen (dst->str + start, -1);
	if (w <= 0 || len >= (size_t)w)
		return;

	if (fl & LEFT_ADJ)
		insert_repeated (dst, dst->len, ' ', w - len);
	else if ((fl & ZERO_PAD) && g_ascii_isdigit (dst->str[start + sign_len]))
		insert_repeated (dst, start + sign_len, '0', w - len);
	else
		insert_repeated (dst, start, ' ', w - len);
}


/**
 * go_dtoa:
 * @dst: destination
 * @fmt: format
 * @...: single value to format (but see below)
 *
 * Formats a single floating-point value.  @fmt should be a printf-style
 * format fragment for a single floating-point item without the initial '%':
 * flags, an optional width, an optional precision, an optional modifier, and
 * a conversion, in that order.
 *
 * The flags '+', '-', and '0' work as in printf.  The flags ' ', '#', and
 * "'" are not supported.  The conversions are 'e', 'f', 'g', 'a', and the
 * upper case versions of those.
 *
 * If goffice has been compiled with support for type "long double", an "L"
 * modifier can be used.  If goffice has been compiled with support for
 * type "decimal64", a "D" modifier can be used.
 *
 * '*' for width and/or precision are allowed and consume an extra int
 * argument each, as in printf.
 *
 * Certain extra flags are supported:
 * flag '^' means to round exact ties away from zero (as opposed to
 * to even);
 * flag '!' means to use the shortest possible representation of the
 * number that reads back as the same value.  The conversion decides the
 * notation: 'e' always uses an exponent, 'f' never does, and 'g' uses one
 * for large and small numbers.  The precision is ignored, but the width
 * and the flags '+', '-', and '0' are honoured.  The conversions 'a' and
 * 'A' are not supported with '!' and are treated as 'f' and 'F';
 * flag '=' means to clear the @dst before formatting (as opposed to
 * appending);
 * flag ',' means to format as in C locale.
 *
 * If @fmt is not valid, a critical warning is issued and @dst is left
 * unchanged.
 */
void
go_dtoa (GString *dst, const char *fmt, ...)
{
	int w, p, fl, t;
	va_list args;
	const char *error;
	FloatValueType d;
	FloatType fltyp;
	gboolean debug = FALSE;
#ifdef ENSURE_FPU_STATE
	fpu_control_t oldstate;
	const fpu_control_t mask = _FPU_EXTENDED | _FPU_DOUBLE | _FPU_SINGLE;
#endif

	va_start (args, fmt);
	error = parse_fmt (fmt, args, &fltyp, &w, &p, &fl, &t, &d);
	va_end (args);

	if (error) {
		g_critical ("go_dtoa: bad format \"%s\": %s", fmt, error);
		return;
	}

	if (fl & FLAG_TRUNCATE) g_string_truncate (dst, 0);

#ifdef ENSURE_FPU_STATE
	// fmt_fp depends on "long double" behaving right.  That means that the
	// fpu must not be in round-to-double mode.
	// This code ought to do nothing on Linux, but Windows and FreeBSD seem
	// to have round-to-double as default.
	_FPU_GETCW (oldstate);
	if ((oldstate & mask) != _FPU_EXTENDED) {
		fpu_control_t newstate = (oldstate & ~mask) | _FPU_EXTENDED;
		_FPU_SETCW (newstate);
	}
#endif

	if (fl & FLAG_SHORTEST)
		fmt_shortest (dst, &d, w, fl, t, fltyp);
#ifdef GOFFICE_WITH_DECIMAL64
	else if (fltyp == FP_DECIMAL64)
		fmt_d64 (dst, d.d64, w, p, fl, t);
#endif
	else {
		fmt_fp (dst, d.ld, w, p, fl, t);
	}

#ifdef ENSURE_FPU_STATE
	if ((oldstate & mask) != _FPU_EXTENDED) {
		_FPU_SETCW (oldstate);
	}
#endif

	if (debug) g_printerr ("  --> %s\n", dst->str);
}
