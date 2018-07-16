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
#include <string.h>
#include <inttypes.h>
#include <stdarg.h>

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

#ifndef MUSL_ORIGINAL
	if (fl & FLAG_TRUNCATE) g_string_truncate (f, 0);
#endif

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
		if (y!=y) s=(t&32)?"nan":"NAN", pl=0;
		pad(f, ' ', w, 3+pl, fl&~ZERO_PAD);
		out(f, prefix, pl);
		out(f, s, 3);
		pad(f, ' ', w, 3+pl, fl^LEFT_ADJ);
		return MAX(w, 3+pl);
	}

	y = frexpl(y, &e2) * 2;
	if (y) e2--;

	if ((t|32)=='a') {
		long double round = 8.0;
		int re;

		if (t&32) prefix += 9;
		pl += 2;

		if (p<0 || p>=LDBL_MANT_DIG/4-1) re=0;
		else re=LDBL_MANT_DIG/4-1-p;

		if (re) {
			while (re--) round*=16;
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
			 */
			if ((fl & FLAG_TIES_AWAY_0) || (*d/i & 1)) round += 2;
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

static void
parse_fmt (const char *fmt, va_list args, gboolean *is_long,
	   int *w, int *p, int *fl, int *t, long double *d)
{
	*is_long = FALSE;
	*w = 1;
	*p = -1;
	*fl = 0;
	*d = 0;
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

	while (g_ascii_isdigit (*fmt))
		*w = *w * 10 + (*fmt++ - '0');

	if (*fmt == '.') {
		if (fmt[1] == '*') {
			fmt += 2;
			*p = va_arg (args, int);
		} else {
			*p = 0;
			fmt++;
			while (g_ascii_isdigit (*fmt))
				*p = *p * 10 + (*fmt++ - '0');
		}
	}

	if (*fmt == 'L') {
		*is_long = TRUE;
		fmt++;
	}

	if (!strchr ("efgaEFGA", *fmt))
		return;
	*t = *fmt;

	if (*is_long)
		*d = va_arg (args, long double);
	else
		*d = va_arg (args, double);
}

static long double
strto (const char *str, gboolean is_long, gboolean is_ascii)
{
	if (is_long) {
#ifdef GOFFICE_WITH_LONG_DOUBLE
		/*
		 * FIXME: ignore ascii flag for now.  Given the limited
		 * and checked usage of this function this should still
		 * be safe, if suboptimal.
		 */
		return go_strtold (str, NULL);
#else
		return go_nan;
#endif
	} else {
		return is_ascii
			? g_ascii_strtod (str, NULL)
			: go_strtod (str, NULL);
	}
}


void
go_dtoa (GString *dst, const char *fmt, ...)
{
	int w, p, fl, t;
	va_list args;
	long double d;
	gboolean is_long;
	gboolean debug = FALSE;
	size_t oldlen;
#ifdef ENSURE_FPU_STATE
	fpu_control_t oldstate;
	const fpu_control_t mask = _FPU_EXTENDED | _FPU_DOUBLE | _FPU_SINGLE;
#endif

	va_start (args, fmt);
	parse_fmt (fmt, args, &is_long, &w, &p, &fl, &t, &d);
	va_end (args);

	if (fl & FLAG_SHORTEST) p = is_long ? 20 : 17;
	oldlen = (fl & FLAG_TRUNCATE) ? 0 : dst->len;

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

	if (debug) g_printerr ("%Lg [%s] t=%c  p=%d\n", d, fmt, t, p);
	fmt_fp (dst, d, w, p, fl, t);
	if (debug) g_printerr ("  --> %s\n", dst->str);

	if (fl & FLAG_SHORTEST) {
		GString *alt = g_string_new (NULL);
		const char *dec = (fl & FLAG_ASCII)
			? "."
			: go_locale_get_decimal()->str;
		while (p > 0) {
			ssize_t len_dif;
			const char *dot = strstr (dst->str + oldlen, dec);
			long double dalt;
			if (!dot)
				break;

			/*
			 * This is crude.  We have a dot, so try to render
			 * the same number with less precision and check
			 * that the result round-trips.
			 */

			fmt_fp (alt, d, w, p - 1, fl | FLAG_TRUNCATE, t);
			len_dif = dst->len - oldlen - alt->len;

			if (len_dif == 0) {
				/* Same string so we've already removed 0s. */
				break;
			}

			dalt = strto (alt->str, is_long, (fl & FLAG_ASCII));
			if (dalt != d)
				break;

			g_string_truncate (dst, oldlen);
			go_string_append_gstring (dst, alt);
			if (debug) g_printerr ("  --> %s\n", dst->str);

			p--;

			if (len_dif > 1)
				break;
		}
		g_string_free (alt, TRUE);
	}

#ifdef ENSURE_FPU_STATE
	if ((oldstate & mask) != _FPU_EXTENDED) {
		_FPU_SETCW (oldstate);
	}
#endif
}
