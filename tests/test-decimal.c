#include <goffice/goffice.h>
#include <goffice/goffice-config.h>

#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

static int n_bad;

#ifdef GOFFICE_WITH_DECIMAL64

// There does not seem to be a way to teach these warnings about the
// "W" modifier that we have hooked into libc's printf.
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

// Classify _Decimal64 values with isfiniteD(), isnanD() and signbitD();
// the type-generic macros have no decimal branch.  The "double" reference
// values use plain isfinite() and isnan().

/* ------------------------------------------------------------------------- */

static int n_section_good, n_section_bad;
static char *subsection;
static gboolean subsection_printed;

static int
double_eq (double x, double y)
{
	return memcmp (&x, &y, sizeof(x)) == 0;
}

static int
decimal_eq (_Decimal64 x, _Decimal64 y)
{
	if (signbitD (x) != signbitD (y))
		return FALSE;

	if (isnanD (x) != isnanD (y))
		return FALSE;
	else if (isnanD (x))
		return TRUE;

	return x == y;
}

static void
start_section (const char *header)
{
	g_printerr ("-----------------------------------------------------------------------------\n");
	g_printerr ("Testing %s\n\n", header);

	n_section_good = n_section_bad = 0;
}

static void
set_subsection (const char *sub)
{
	g_free (subsection);
	subsection = g_strdup (sub);
	subsection_printed = FALSE;
}

static void
end_section (void)
{
	if (n_section_bad)
		g_printerr ("\n");
	g_printerr ("For this section: good: %d, bad: %d\n\n",
		    n_section_good, n_section_bad);
	set_subsection (NULL);
}

static void
good (void)
{
	n_section_good++;
}

static void
bad (void)
{
	if (subsection && !subsection_printed) {
		g_printerr ("Trouble with %s\n", subsection);
		subsection_printed = TRUE;
	}
	n_section_bad++;
	n_bad++;
}

/* ------------------------------------------------------------------------- */

// Tests that are known to fail with the current implementation are marked
// "xfail".  A failing xfail check is reported as XFAIL and does not affect
// the exit status.  A passing one is reported as XPASS and *is* a failure:
// it means the bug was fixed and the marker should be removed.  Setting the
// environment variable GO_DECIMAL_TEST_STRICT to anything but "0" makes
// XFAILs count as failures too, which is handy while working on a fix.

static gboolean strict_xfail;
static int n_xfail, n_section_xfail;

static void G_GNUC_PRINTF (2, 3)
test_true (gboolean ok, const char *fmt, ...)
{
	if (ok) {
		good ();
	} else {
		va_list args;
		char *msg;

		va_start (args, fmt);
		msg = g_strdup_vprintf (fmt, args);
		va_end (args);

		bad ();
		g_printerr ("Failed: %s\n", msg);
		g_free (msg);
	}
}

static void G_GNUC_PRINTF (2, 3)
test_xfail (gboolean ok, const char *fmt, ...)
{
	va_list args;
	char *msg;

	va_start (args, fmt);
	msg = g_strdup_vprintf (fmt, args);
	va_end (args);

	if (ok) {
		bad ();
		g_printerr ("XPASS: %s\n", msg);
		g_printerr ("      (fixed?  remove the xfail marker)\n");
	} else if (strict_xfail) {
		bad ();
		g_printerr ("Failed (known bug): %s\n", msg);
	} else {
		n_xfail++;
		n_section_xfail++;
		g_printerr ("XFAIL: %s\n", msg);
	}
	g_free (msg);
}

// Either test_true or test_xfail
static void G_GNUC_PRINTF (3, 4)
test_expect (gboolean xfail, gboolean ok, const char *fmt, ...)
{
	va_list args;
	char *msg;

	va_start (args, fmt);
	msg = g_strdup_vprintf (fmt, args);
	va_end (args);

	if (xfail)
		test_xfail (ok, "%s", msg);
	else
		test_true (ok, "%s", msg);
	g_free (msg);
}

// Decimal64 <-> raw encoding.  We assume BID, like go-decimal.c does.
static uint64_t
d64_bits (_Decimal64 x)
{
	uint64_t u;
	memcpy (&u, &x, sizeof (u));
	return u;
}

static _Decimal64
d64_from_bits (uint64_t u)
{
	_Decimal64 x;
	memcpy (&x, &u, sizeof (x));
	return x;
}

// Number of ulps (in the 16-digit sense) between got and want.  The
// non-finite cases return 0 for "same" and something huge for "different".
static _Decimal64
ulp_err (_Decimal64 got, _Decimal64 want)
{
	int e;

	if (!isfiniteD (got) || !isfiniteD (want))
		return decimal_eq (got, want) ? 0.dd : 1e30dd;
	if (want == 0)
		return got == 0 ? 0.dd : 1e30dd;

	(void)unscalbnD (want, &e);
	// want = m * 10^e with 0.1 <= |m| < 1, so a ulp is normally 10^(e-16),
	// but clamp so this stays finite (and meaningful) all the way down
	// into the denormal range, where the actual quantum is 10^-398.
	e -= DECIMAL64_DIG;
	if (e < -398)
		e = -398;
	return fabsD (got - want) / powD (10.dd, e);
}

static void
expect_eq (const char *what, _Decimal64 got, _Decimal64 want, gboolean xfail)
{
	test_expect (xfail, decimal_eq (got, want),
		     "%s = %.16Wg, expected %.16Wg", what, got, want);
}

static void
expect_close (const char *what, _Decimal64 got, _Decimal64 want,
	      int max_ulps, gboolean xfail)
{
	_Decimal64 u = ulp_err (got, want);
	test_expect (xfail, u <= max_ulps,
		     "%s = %.16Wg, expected %.16Wg (%.1Wg ulp off, max %d)",
		     what, got, want, u, max_ulps);
}

static void
expect_nan (const char *what, _Decimal64 got, gboolean xfail)
{
	test_expect (xfail, isnanD (got),
		     "%s = %.16Wg, expected NaN", what, got);
}

#define EQ(expr, want) expect_eq (#expr, (expr), (want), FALSE)
#define EQ_XF(expr, want) expect_eq (#expr, (expr), (want), TRUE)
#define CLOSE(expr, want, ulps) expect_close (#expr, (expr), (want), (ulps), FALSE)
#define CLOSE_XF(expr, want, ulps) expect_close (#expr, (expr), (want), (ulps), TRUE)
#define NAN_(expr) expect_nan (#expr, (expr), FALSE)
#define NAN_XF(expr) expect_nan (#expr, (expr), TRUE)

#define PINF ((_Decimal64)INFINITY)
#define NINF (-(_Decimal64)INFINITY)
#define QNAN ((_Decimal64)NAN)

// Small deterministic PRNG so that failures are reproducible.
static uint64_t rng_state = 0x9e3779b97f4a7c15ull;

static uint64_t
rng (void)
{
	// xorshift64*
	rng_state ^= rng_state >> 12;
	rng_state ^= rng_state << 25;
	rng_state ^= rng_state >> 27;
	return rng_state * 0x2545f4914f6cdd1dull;
}

static int
rng_range (int lo, int hi)
{
	return lo + (int)(rng () % (uint64_t)(hi - lo + 1));
}

static const uint64_t pow10u[20] = {
	1ull, 10ull, 100ull, 1000ull, 10000ull, 100000ull, 1000000ull,
	10000000ull, 100000000ull, 1000000000ull, 10000000000ull,
	100000000000ull, 1000000000000ull, 10000000000000ull,
	100000000000000ull, 1000000000000000ull, 10000000000000000ull,
	100000000000000000ull, 1000000000000000000ull,
	10000000000000000000ull
};

// A random 16-digit-or-less mantissa, with some interesting ones mixed in
static uint64_t
rng_mant (void)
{
	static const uint64_t special[] = {
		0, 1, 5, 9, 10, 15, 25, 50, 95, 99, 100, 500, 995, 999, 5000,
		999999999999999ull, 1000000000000000ull, 1000000000000001ull,
		4999999999999999ull, 5000000000000000ull, 5000000000000001ull,
		9007199254740991ull, 9007199254740992ull, 9007199254740993ull,
		9999999999999998ull, 9999999999999999ull
	};
	int nd;

	if (rng () % 4 == 0)
		return special[rng () % G_N_ELEMENTS (special)];

	nd = rng_range (1, 16);
	return pow10u[nd - 1] + rng () % (pow10u[nd] - pow10u[nd - 1]);
}

// Exact powers of ten, 10^0 ... 10^63
static _Decimal64 P10[64];

static void
init_p10 (void)
{
	if (P10[0] == 0) {
		P10[0] = 1;
		for (int i = 1; i < 64; i++)
			P10[i] = P10[i - 1] * 10;
	}
}

// Construct (-1)^neg * m * 10^e exactly, without going through any of the
// functions under test.  Requires m <= 9999999999999999 and |e| < 64.
static _Decimal64
mk (uint64_t m, int e, int neg)
{
	_Decimal64 x;

	init_p10 ();
	x = (_Decimal64)m;
	if (e >= 0)
		x *= P10[e];
	else
		x /= P10[-e];
	return neg ? -x : x;
}

static int
test_eq (_Decimal64 a, _Decimal64 b)
{
	if (decimal_eq (a, b)) {
		good ();
		return 1;
	} else {
		bad ();
		g_printerr ("%.16Wg vs %.16Wg\n", a, b);
		return 0;
	}
}

static int
test_quad_eq (GOQuadD const *a, GOQuadD const *b, _Decimal64 maxerr)
{
	GOQuadD qd, qc;

	go_quad_subD (&qd, a, b);
	go_quad_absD (&qd, &qd);

	go_quad_initD (&qc, maxerr);
	go_quad_subD (&qd, &qd, &qc);

	if (go_quad_valueD (&qd) <= 0) {
		good ();
		return 1;
	} else {
		bad ();
		g_printerr ("Quad %.16Wg + %.16Wg\n", a->h, a->l);
		g_printerr ("  vs %.16Wg + %.16Wg\n", b->h, b->l);
		return 0;
	}
}

/* ------------------------------------------------------------------------- */

typedef struct {
	_Decimal64 *vals;
	int nvals;
} Corpus;

static Corpus *
corpus_new (int count)
{
	Corpus *res = g_new (Corpus, 1);
	res->nvals = count;
	res->vals = g_new (_Decimal64, res->nvals);
	return res;
}

static void
corpus_free (Corpus *corpus)
{
	g_free (corpus->vals);
	g_free (corpus);
}

static Corpus *
corpus_concat (Corpus *first, gboolean free_first,
	       Corpus *second, gboolean free_second)
{
	Corpus *res = corpus_new (first->nvals + second->nvals);

	memcpy (res->vals, first->vals, first->nvals * sizeof(_Decimal64));
	memcpy (res->vals + first->nvals, second->vals, second->nvals * sizeof(_Decimal64));

	if (free_first) corpus_free (first);
	if (free_second) corpus_free (second);

	return res;
}

static Corpus *
basic_corpus (void)
{
	static const _Decimal64 values64[] = {
		0.dd, 3.14dd, 0.123dd, 0.05dd, 1.5dd, 0.567dd, 999999999.5dd,
		100.dd, 1e20dd, 0.01dd, 1e-20dd, 0.1dd,
		0.3333333333333333dd, 0.5555555555555555dd, 0.9999999999999999dd,
		INFINITY, NAN,
		1e15dd, 999999999999999.9dd, 999999999999999.0dd,
		1e15dd + 1, 1e22dd,
		2.5dd, 1.5dd, // make sure we don't round-ties-to-even
		1.000000001dd,

		// Out of double range
		DECIMAL64_MAX,
		DECIMAL64_MIN,
	};
	Corpus *res = corpus_new (2 * G_N_ELEMENTS (values64));
	_Decimal64 *p;
	size_t i;

	for (i = 0, p = res->vals; i < G_N_ELEMENTS (values64); i++) {
		*p++ = values64[i];
		*p++ = -values64[i];
	}

	return res;
}

static Corpus *
linear_corpus (int count, _Decimal64 slope, _Decimal64 offset)
{
	Corpus *res = corpus_new (count);
	int i;

	for (i = 0; i < count; i++)
		res->vals[i] = i * slope + offset;

	return res;
}

static Corpus *
power_corpus (int count, _Decimal64 low, _Decimal64 f)
{
	Corpus *res = corpus_new (count);
	int i;

	for (i = 0; i < count; i++) {
		res->vals[i] = low;
		low *= f;
	}

	return res;
}

/* ------------------------------------------------------------------------- */

static void
test_rounding (const Corpus *corpus)
{
	start_section ("rounding operations (floor, ceil, round, trunc)");

	for (int v = 0; v < corpus->nvals; v++) {
		_Decimal64 x = corpus->vals[v];

		_Decimal64 f = floorD (x);
		_Decimal64 c = ceilD (x);
		_Decimal64 r = roundD (x);
		_Decimal64 t = truncD (x);
		double dx = x;
		int sanity;
		int qunderflow = (dx == 0) && (x != 0);
		int qoverflow = isfiniteD (x) && !isfinite (dx);
		int ok;

		sanity = isnanD (x)
			? 1
			: (f <= x && x <= c &&
			   (c == f || c == f + 1) &&
			   (r == f || r == c) &&
			   (x < 0 ? t == c : t == f) &&
			   r <= x + 0.5dd &&
			   r >= x - 0.5dd);
		if (qunderflow)
			ok = (r == 0 && (f == 0 || c == 0));
		else if (qoverflow)
			ok = (f == x && f == x);
		else
			ok = (double_eq (f, floor (dx)) &&
			      double_eq (r, round (dx)) &&
			      double_eq (c, ceil (dx)));

		if (ok && sanity) {
			good ();
		} else {
			uint64_t d64;
			memcpy (&d64, &x, sizeof (d64));

			bad ();
			g_printerr ("Error: 0x%08lx: %.16Wg -> (%.16Wg , %.16Wg , %.16Wg , %.16Wg)\n",
				    d64, x, f, r, c, t);
		}
	}

	end_section ();
}

static void
test_properties (const Corpus *corpus)
{
	start_section ("properties (isnanD, isfiniteD, signbitD)");

	for (int v = 0; v < corpus->nvals; v++) {
		_Decimal64 x = corpus->vals[v];

		int qnan = isnanD (x);
		int qfinite = isfiniteD (x);
		int qsign = signbitD (x);
		double dx = x;

		if (!!qnan == (x != x) &&
		    !!qfinite == (fabsD (x) <= DECIMAL64_MAX) &&
		    !!qsign == !!signbit (dx)) {
			good ();
		} else {
			uint64_t d64;
			memcpy (&d64, &x, sizeof (d64));

			bad ();
			g_printerr ("Error: 0x%08lx: %.16Wg -> %d %d %d\n",
				    d64, x, qnan, qfinite, qsign);
		}
	}

	end_section ();
}

static void
test_copysign (const Corpus *corpus)
{
	start_section ("copysign");

	for (int v1 = 0; v1 < corpus->nvals; v1++) {
		_Decimal64 x1 = corpus->vals[v1];
		for (int v2 = 0; v2 < corpus->nvals; v2++) {
			_Decimal64 x2 = corpus->vals[v2];

			_Decimal64 y = copysignD (x1, x2);
			if (decimal_eq (fabsD (y), fabsD (x1)) &&
			    signbitD (y) == signbitD (x2))
				good ();
			else {
				bad ();
				g_printerr ("Failed for %.16Wg  %.16Wg\n", x1, x2);
			}
		}
	}

	end_section ();
}


static void
test_nextafter (void)
{
	_Decimal64 m;
	start_section ("nextafter");

	m = nextafterD (0, 4);
	test_eq (m, 1e-398dd);
	test_eq (nextafterD (0, -4), -1e-398dd);
	test_eq (nextafterD (nextafterD (0, +4), -4), 0.dd);
	test_eq (nextafterD (nextafterD (0, -4), +4), -0.dd);
	test_eq (nextafterD (1111111111111111.dd, INFINITY), 1111111111111112.dd);
	test_eq (nextafterD (-1111111111111111.dd, INFINITY), -1111111111111110.dd);
	test_eq (nextafterD (1000000000000000.dd, INFINITY), 1000000000000001.dd);
	test_eq (nextafterD (1000000000000000.dd, -INFINITY),999999999999999.9dd);
	test_eq (nextafterD (1000000000000000e-10dd, INFINITY), 1000000000000001e-10dd);
	test_eq (nextafterD (1000000000000000e-10dd, -INFINITY), 999999999999999.9e-10dd);
	test_eq (nextafterD (1000000000000000e+99dd, INFINITY), 1000000000000001e+99dd);
	test_eq (nextafterD (1000000000000000e+99dd, -INFINITY), 999999999999999.9e+99dd);
	test_eq (nextafterD (1000000000000001.dd, INFINITY), 1000000000000002.dd);
	test_eq (nextafterD (999999999999999.dd, +INFINITY), 999999999999999.1dd);
	test_eq (nextafterD (999999999999999.9dd, +INFINITY), 1000000000000000.dd);
	test_eq (nextafterD (INFINITY, INFINITY), INFINITY);
	test_eq (nextafterD (-INFINITY, -INFINITY), -INFINITY);
	test_eq (nextafterD (INFINITY, NAN), NAN);
	test_eq (nextafterD (m, 1), 2 * m);
	test_eq (nextafterD (2 * m, 0), m);
	test_eq (nextafterD (9 * m, 1), 10 * m);
	test_eq (nextafterD (10 * m, 1), 11 * m);
	test_eq (nextafterD (DECIMAL64_MAX / 10, INFINITY),
		 DECIMAL64_MAX / 10 + DECIMAL64_MAX / 1e17dd);
	test_eq (nextafterD (DECIMAL64_MAX, INFINITY), INFINITY);
	test_eq (nextafterD (-DECIMAL64_MAX, -INFINITY), -INFINITY);
	test_eq (nextafterD (INFINITY, 0), DECIMAL64_MAX);

	end_section ();
}

static void
test_modf (const Corpus *corpus)
{
	start_section ("modf");

	for (int v = 0; v < corpus->nvals; v++) {
		_Decimal64 x = corpus->vals[v];
		_Decimal64 y, z;

		z = modfD (x, &y);

		test_eq (y, copysignD (truncD (x), x));
		test_eq (z, copysignD (fabsD (x) == go_pinfD ? 0 : x - truncD (x), x));
	}

	end_section ();
}

static void
test_log (const char *name, int base, const Corpus *corpus)
{
	double (*fn_double) (double);
	_Decimal64 (*fn_decimal) (_Decimal64);

	start_section (name);

	switch (base) {
	default:
	case 2: fn_decimal = log2D; fn_double = log2; break;
	case 3: fn_decimal = logD; fn_double = log; break;
	case 10: fn_decimal = log10D; fn_double = log10; break;
	}

	for (int v = 0; v < corpus->nvals; v++) {
		_Decimal64 x = corpus->vals[v], y = fn_decimal (x);
		double dx = x, dy = fn_double (dx);
		gboolean ok;
		int qunderflow = (dx == 0) && (x != 0);
		int qoverflow = isfiniteD (x) && !isfinite (dx);

		if (fabsD (x - 1) < 0.01dd)
			continue;

		if (x < 0)
			ok = isnanD (y);
		else if (qunderflow)
			ok = isfiniteD (y) && y <= (_Decimal64)(fn_double (DBL_MIN));
		else if (qoverflow)
			ok = isfiniteD (y) && y >= (_Decimal64)(fn_double (DBL_MAX));
		else {
			ok = (!!isfiniteD (y) == !!isfinite (dy) &&
			      !!isnanD (y) == !!isnan (dy) &&
			      !!signbitD (y) == !!signbit (dy) &&
			      (y == 0) == (dy == 0));

			if (ok && isfinite (dy)) {
				ok = ((y == floorD (y)) == (dy == floor (dy)));

				if (ok && y != 0) {
					_Decimal64 d = y - (_Decimal64)dy;
					ok = fabsD (d / y) < 1e-10dd;
				}
			}
		}

		if (ok)
			good ();
		else {
			bad ();
			g_printerr ("Failed for %.16Wg -- got %.16Wg vs %.16g\n", x, y, dy);
		}
	}

	end_section ();
}

static void
test_scalbn (void)
{
	_Decimal64 x;
	int i;

	start_section ("scalbn");

	test_eq (scalbnD (0.1234567890123456dd, 10), 1234567890.123456dd);

	test_eq (scalbnD (0.0dd, G_MAXINT), 0.0dd);
	test_eq (scalbnD (0.0dd, G_MININT), 0.0dd);
	test_eq (scalbnD (0.0dd, 0), 0.0dd);
	test_eq (scalbnD (-0.0dd, G_MAXINT), -0.0dd);
	test_eq (scalbnD (-0.0dd, G_MININT), -0.0dd);

	test_eq (scalbnD ((_Decimal64)INFINITY, G_MAXINT), (_Decimal64)INFINITY);
	test_eq (scalbnD ((_Decimal64)INFINITY, G_MININT), (_Decimal64)INFINITY);
	test_eq (scalbnD ((_Decimal64)INFINITY, 0), (_Decimal64)INFINITY);
	test_eq (scalbnD (-(_Decimal64)INFINITY, G_MAXINT), -(_Decimal64)INFINITY);
	test_eq (scalbnD (-(_Decimal64)INFINITY, G_MININT), -(_Decimal64)INFINITY);

	test_eq (scalbnD ((_Decimal64)NAN, G_MAXINT), (_Decimal64)NAN);
	test_eq (scalbnD ((_Decimal64)NAN, G_MININT), (_Decimal64)NAN);
	test_eq (scalbnD ((_Decimal64)NAN, 0), (_Decimal64)NAN);
	test_eq (scalbnD (-(_Decimal64)NAN, G_MAXINT), -(_Decimal64)NAN);
	test_eq (scalbnD (-(_Decimal64)NAN, G_MININT), -(_Decimal64)NAN);

	test_eq (scalbnD (1e-398dd, 398 + 369), 1e369dd);
	test_eq (scalbnD (1e-398dd, 398 + 384), 1e384dd);
	test_eq (scalbnD (1e-398dd, 398 + 384 + 1), (_Decimal64)INFINITY);
	test_eq (scalbnD (1e384dd, -398 - 384), 1e-398dd);
	test_eq (scalbnD (9999999999999999e369dd, -369 - 383), 9999999999999999e-383dd);
	test_eq (scalbnD (1e384dd, -398 - 384 - 1), 0.dd);
	test_eq (scalbnD (-1e384dd, -398 - 384 - 1), -0.dd);
	test_eq (scalbnD (5e384dd, -398 - 384 - 1), 1e-398dd);

	test_eq (scalbnD (5e-398dd, -1), 1e-398dd);
	test_eq (scalbnD (5000000000000000e-398dd, -16), 1e-398dd);

	x = 1.dd;
	for (i = 0; i <= 384; i++) {
		test_eq (scalbnD (1.dd, i), x);
		test_eq (powD (10.dd, i), x);
		x *= 10;
	}

	end_section ();
}



static void
test_oneargs (const Corpus *corpus)
{
	static const struct {
		const char *name;
		_Decimal64 (*fn_decimal) (_Decimal64);
		double (*fn_double) (double);
	} funcs[] = {
		{ "acosD", acosD, acos },
		{ "acoshD", acoshD, acosh },
		{ "asinD", asinD, asin },
		{ "asinhD", asinhD, asinh },
		{ "atanD", atanD, atan },
		{ "atanhD", atanhD, atanh },
		{ "cbrtD", cbrtD, cbrt },
		{ "ceilD", ceilD, ceil },
		{ "cosD", cosD, cos },
		{ "coshD", coshD, cosh },
		{ "erfD", erfD, erf },
		{ "erfcD", erfcD, erfc },
		{ "expD", expD, exp },
		{ "expm1D", expm1D, expm1 },
		{ "fabsD", fabsD, fabs },
		{ "floorD", floorD, floor },
		{ "lgammaD", lgammaD, lgamma },
		// { "log10D", log10D, log10 },
		// { "log1pD", log1pD, log1p },
		// { "log2D", log2D, log2 },
		// { "logD", logD, log },
		{ "roundD", roundD, round },
		{ "sinD", sinD, sin },
		{ "sinhD", sinhD, sinh },
		{ "sqrtD", sqrtD, sqrt },
		{ "tanD", tanD, tan },
		{ "tanhD", tanhD, tanh },
		{ "truncD", truncD, trunc },
	};

	start_section ("one-arg functions");

	for (int f = 0; f < (int)G_N_ELEMENTS (funcs); f++) {
		set_subsection (funcs[f].name);
		for (int v = 0; v < corpus->nvals; v++) {
			_Decimal64 x = corpus->vals[v], y;
			double dx = x, dy;
			int ok;
			int qunderflow = (dx == 0) && (x != 0);
			int qoverflow = isfiniteD (x) && !isfinite (dx);
			_Decimal64 tol = 1e-10dd;

			if (qunderflow || qoverflow)
				continue;

			// Going through double doesn't work well here
			if (funcs[f].fn_decimal == atanhD &&
			    fabsD (fabsD (x) - 1) <= 1e-15dd)
				tol = 1e-2dd;

			y = funcs[f].fn_decimal (x);
			dy = funcs[f].fn_double (dx);

			//g_printerr ("%.16Wg  %.16Wg\n", x, y);
			//g_printerr ("%.16g  %.16g\n", dx, dy);

			ok = (!!isfiniteD (y) == !!isfinite (dy) &&
			      !!isnanD (y) == !!isnan (dy) &&
			      !!signbitD (y) == !!signbit (dy) &&
			      (y == 0) == (dy == 0));

			if (ok && isfinite (dy) && y != 0) {
				_Decimal64 d = y - (_Decimal64)dy;
				ok = fabsD (d / y) < tol;
			}

			if (ok)
				good ();
			else {
				g_printerr ("Failed for %.16Wg\n", x);
				test_eq (y, dy);
			}
		}
	}

	end_section ();
}


static void
test_dtoa (const Corpus *corpus)
{
	static const char *fmts[] = {
		"=^.0f", "=^.1f", "=^.2f", "=^.3f", "=^.4f",
		"=^.5f", "=^.6f", "=^.7f", "=^.8f", "=^.9f",
		"=^.0e", "=^.1e", "=^.2e", "=^.3e", "=^.4e",
		"=^.5e", "=^.6e", "=^.7e", "=^.8e", "=^.9e",
		"=^.0g", "=^.1g", "=^.2g", "=^.3g", "=^.4g",
		"=^.5g", "=^.6g", "=^.7g", "=^.8g", "=^.9g",
	};
	const int nfmts = G_N_ELEMENTS (fmts);
	GString *s1, *s2;

	start_section ("go_dtoa");

	s1 = g_string_new (NULL);
	s2 = g_string_new (NULL);

	if (0) {
		_Decimal64 d = .567dd;
		g_printerr ("[%.0f]\n", (double)d);
		go_dtoa (s1, "=^.0Wf", d);
		g_printerr ("[%s]\n", s1->str);
		return;
	}

	for (int f = 0; f < nfmts; f++) {
		const char *fmt = fmts[f];
		int lfmt = strlen (fmt);
		gboolean fstyle = g_ascii_toupper (fmt[lfmt - 1]) == 'F';
		char fmt2[100];
		strcpy (fmt2, fmt);
		fmt2[lfmt + 1] = 0;
		fmt2[lfmt + 0] = fmt2[lfmt - 1];
		fmt2[lfmt - 1] = 'W';

		set_subsection (fmt);

		for (int v = 0; v < corpus->nvals; v++) {
			_Decimal64 x = corpus->vals[v];
			double dx = x;
			int qunderflow = (dx == 0) && (x != 0);
			int qoverflow = isfiniteD (x) && !isfinite (dx);

			if (qunderflow || qoverflow)
				continue;

			if (fstyle && fabsD (x) > 0 && isfiniteD (x) &&
			    log10D (fabsD (x)) > 8)
				continue;

			go_dtoa (s1, fmt, dx);
			go_dtoa (s2, fmt2, x);
			if (g_string_equal (s1, s2)) {
				good ();
			} else {
				bad ();
				g_printerr ("Got [%s], expected [%s] [%.20g]\n",
					    s2->str, s1->str, dx);
			}
		}
	}
	g_string_free (s1, TRUE);
	g_string_free (s2, TRUE);

	end_section ();
}

static void
test_pow (const Corpus *corpus1, const Corpus *corpus2)
{
	start_section ("pow");

	for (int v1 = 0; v1 < corpus1->nvals; v1++) {
		_Decimal64 x1 = corpus1->vals[v1];
		double dx1 = x1;
		int qunderflow1 = (dx1 == 0) && (x1 != 0);
		int qoverflow1 = isfiniteD (x1) && !isfinite (dx1);

		if (qunderflow1 || qoverflow1)
			continue;

		for (int v2 = 0; v2 < corpus2->nvals; v2++) {
			_Decimal64 x2 = corpus2->vals[v2];
			double dx2 = x2;
			int qunderflow2 = (dx2 == 0) && (x2 != 0);
			int qoverflow2 = isfiniteD (x2) && !isfinite (dx2);
			gboolean ok;

			if (qunderflow2 || qoverflow2)
				continue;

			if (x1 < 0 && x2 < 0 &&
			    isfiniteD (x1) &&
			    isfiniteD (x2) && x2 == floorD (x2) &&
			    fmodD (x2, 2.dd) != fmodD (dx2, 2.dd))
				continue;

			double dy = pow (dx1, dx2);
			_Decimal64 y = powD (x1, x2);

			// Both sides must agree on NaN.  The sign is not
			// compared there, a NaN's sign being meaningless;
			// the other two branches do compare it, explicitly
			// and inside decimal_eq() respectively.
			if (isnanD (y) || isnan (dy))
				ok = isnanD (y) && isnan (dy);
			else if (!isfiniteD (y) || !isfinite (dy))
				ok = (!isfiniteD (y) && !isfinite (dy) &&
				      !!signbitD (y) == !!signbit (dy));
			else
				ok = ulp_err (y, dy) <= 60;  // That's a lot, but it is dy that is off.

			if (ok) {
				good ();
			} else {
				bad ();
				g_printerr ("Failed for %.16Wg  %.16Wg\n", x1, x2);
				g_printerr ("Got %.16Wg vs %.16g (%.1Wg ulp off)\n", y, dy, ulp_err (y, dy));
			}
		}
	}

	end_section ();
}

static void
test_pow2 (void)
{
	start_section ("pow2");

	for (int i = -1400; i < 1300; i++) {
		_Decimal64 tti = go_pow2D (i);
		gboolean ok;
		if (i >= -1022 && i <= 1023)
			ok = decimal_eq (tti, (_Decimal64)go_pow2 (i));
		else if (i >= 1024 && i <= 1278)
			ok = fabsD (log2D (tti) - i) < 0.01dd;  // thus not inf
		else if (i >= 1279)
			ok = (tti == go_pinfD);
		else if (i >= -1317 && i <= -1023)
			ok = fabsD (log2D (tti) - i) < 0.01dd;  // thus not zero
		else if (i >= -1322 && i <= -1018)
			// Way into denormal land
			ok = fabsD (log2D (tti) - i) < 0.49dd;  // thus not zero
		else if (i == -1323)
			// 2^-1323 == 2^-1322, strangely enough
			ok = tti > 0.0dd;
		else
			ok = decimal_eq (tti, 0.0dd);

		if (ok) {
			good ();
		} else {
			bad ();
			g_printerr ("Failed for %d; got %.16Wg\n", i, tti);
		}
	}

	end_section ();
}

static void
test_atan2 (const Corpus *corpus1, const Corpus *corpus2)
{
	start_section ("atan2");

	for (int v1 = 0; v1 < corpus1->nvals; v1++) {
		_Decimal64 x1 = corpus1->vals[v1];
		double dx1 = x1;

		for (int v2 = 0; v2 < corpus2->nvals; v2++) {
			_Decimal64 x2 = corpus2->vals[v2];
			_Decimal64 q = x1 / x2;
			double dx2 = x2;
			gboolean ok;

			double dy = (q == 0)
				? copysign (x2 > 0 ? 0 : M_PI, dx1)
				: (!isfiniteD (x1)
				   ? atan2 (dx1, x2 / 1e100dd)
				   : (x2 == 0 && x1 != 0
				      ? atan2 (x1 > 0 ? 1 : -1, dx2)
				      : (dx1 == 0 && dx2 == 0
					 ? atan2 (x1 * 1e100dd, x2 * 1e100dd)
					 : atan2 (dx1, dx2))));
			_Decimal64 y = atan2D (x1, x2);

			// Same three branches as test_pow.
			if (isnanD (y) || isnan (dy))
				ok = isnanD (y) && isnan (dy);
			else if (!isfiniteD (y) || !isfinite (dy))
				ok = (!isfiniteD (y) && !isfinite (dy) &&
				      !!signbitD (y) == !!signbit (dy));
			else if (dy == 0 && fabsD (y) <= (_Decimal64)DBL_MIN)
				ok = (!!signbitD (y) == !!signbit (dy));
			else
				ok = ulp_err(y, dy) <= 2;

			if (ok) {
				good ();
			} else {
				bad ();
				g_printerr ("Failed for %.16Wg  %.16Wg\n", x1, x2);
				g_printerr ("Got %.16Wg vs %.16g\n", y, dy);
			}
		}
	}

	end_section ();
}

static void
test_hypot (const Corpus *corpus1, const Corpus *corpus2)
{
	start_section ("hypot");

	for (int v1 = 0; v1 < corpus1->nvals; v1++) {
		_Decimal64 x1 = corpus1->vals[v1];
		double dx1 = x1;

		int qunderflow1 = (dx1 == 0) && (x1 != 0);
		int qoverflow1 = isfiniteD (x1) && !isfinite (dx1);

		if (qunderflow1 || qoverflow1)
			continue;

		for (int v2 = 0; v2 < corpus2->nvals; v2++) {
			_Decimal64 x2 = corpus2->vals[v2];
			double dx2 = x2;
			int qunderflow2 = (dx2 == 0) && (x2 != 0);
			int qoverflow2 = isfiniteD (x2) && !isfinite (dx2);
			gboolean ok;

			if (qunderflow2 || qoverflow2)
				continue;

			double dy = hypot (dx1, dx2);
			_Decimal64 y = hypotD (x1, x2);

			// Same three branches as test_pow, but hypot() is
			// never negative, so the finite branch needs no
			// sign check of its own.
			if (isnanD (y) || isnan (dy))
				ok = isnanD (y) && isnan (dy);
			else if (!isfiniteD (y) || !isfinite (dy))
				ok = (!isfiniteD (y) && !isfinite (dy) &&
				      !!signbitD (y) == !!signbit (dy));
			else {
				_Decimal64 y2 = dy;
				_Decimal64 diff = y - y2;
				if (fabsD (diff) <= y * 1e-15dd)
					ok = TRUE;
				else
					ok = decimal_eq (y, dy);
			}

			if (ok) {
				good ();
			} else {
				bad ();
				g_printerr ("Failed for %.16Wg  %.16Wg\n", x1, x2);
				g_printerr ("Got %.16Wg vs %.16g\n", y, dy);
			}
		}
	}

	end_section ();
}

static void
test_fmod (const Corpus *corpus1, const Corpus *corpus2)
{
	start_section ("fmodD");

	for (int v1 = 0; v1 < corpus1->nvals; v1++) {
		_Decimal64 x1 = corpus1->vals[v1];

		for (int v2 = 0; v2 < corpus2->nvals; v2++) {
			_Decimal64 x2 = corpus2->vals[v2];
			gboolean ok;
			_Decimal64 y = fmodD (x1, x2);

			ok = !signbitD (y) == !signbitD (x1);

			if (isnanD (x1) || isnanD (x2) || x2 == 0)
				ok = ok && isnanD (y);

			if ((x1 == 0 && fabsD (x2) > 0) ||
			    (isfiniteD (x1) && x2 == (_Decimal64)INFINITY))
				ok = ok && decimal_eq (y, x1);

			// This is a poor test.  We need something curated specially
			// for fmod.
			if (ok && isfiniteD (y) && isfiniteD (x2)) {
				_Decimal64 q = x1 / x2;
				_Decimal64 r = x1 - truncD (q) * x2;
				ok = (fabsD (y) < fabsD (x2));
				if (y != r) {
					//g_printerr ("   %.16Wg  %.16Wg\n", y, r);
				}
			}

			if (ok) {
				good ();
			} else {
				bad ();
				g_printerr ("Failed for %.16Wg  %.16Wg\n", x1, x2);
				g_printerr ("Got %.16Wg vs %.16g\n", y, fmod (x1, x2));
			}
		}
	}

	end_section ();
}

static void
test_quad_exp_pow (void)
{
	GOQuadD qa, qb, qc, qsqrt1000, qe;
	_Decimal64 p10;
	void *state;

	start_section ("quad exp, pow");

	state = go_quad_startD ();

	go_quad_initD (&qa, 1000);
	go_quad_sqrtD (&qb, &qa);
	qsqrt1000.h = 31.62277660168379dd;
	qsqrt1000.l =                 .3319988935444327e-14dd;  // observe ...329
	test_quad_eq (&qb, &qsqrt1000, 2e-30dd);

	qb = qsqrt1000;
	qb.h *= 1000;
	qb.l *= 1000;
	go_quad_expD (&qc, &p10, &qb);

	qe.h = 3.957132301185386dd;
	qe.l =                -.3807734103472098e-15dd;
	// * 10^13733
	test_eq (p10, 13733);
	test_quad_eq (&qc, &qe, 1e-27dd);

	// g_printerr ("c = %.16Wg + %.16Wg   (%.16Wg)\n", qc.h, qc.l, p10);
	// c = 3.957132301185386 + -3.807734103479432e-16   (13733)

	go_quad_end (state);

	end_section ();
}


static void
test_strto (void)
{
	start_section ("strto");

	// Check that rounding applies where needed for denormals
	test_eq (strtoDd ( "-1.6285971684225496E-385", NULL), -1.6285971684225e-385dd);
	test_eq (strtoDd ( "-1.62859716842254949E-385", NULL), -1.6285971684225e-385dd);
	test_eq (strtoDd ( "-4.2384999999999999E-395", NULL), -4.238e-395dd);
	test_eq (strtoDd ( "-1.5049999999999999E-396", NULL), -1.50e-396dd);
	test_eq (strtoDd ( "-3.4999999999999999E-398", NULL), -3.e-398dd);
	test_eq (strtoDd ( "3.8194999999999999E-395", NULL), 3.819e-395dd);
	test_eq (strtoDd ( "3.8414999999999999E-395", NULL), 3.841e-395dd);
	test_eq (strtoDd ( "4.1351384999999999E-392", NULL), 4.135138e-392dd);
	test_eq (strtoDd ( "12345678901234565E-399", NULL), 1.234567890123457e-383dd);
	test_eq (strtoDd ( "1.0108144713502247E-384", NULL), 1.01081447135022e-384dd);
	test_eq (strtoDd ( "1.0108144713502253E-384", NULL), 1.01081447135023e-384dd);

	test_eq (strtoDd ("123", NULL), 123.dd);
	test_eq (strtoDd ("123.", NULL), 123.dd);
	test_eq (strtoDd ("+123.", NULL), 123.dd);
	test_eq (strtoDd ("-123.0", NULL), -123.dd);
	test_eq (strtoDd (".123", NULL), .123dd);
	test_eq (strtoDd ("0.0000000000000000000000000123", NULL), 0.0000000000000000000000000123dd);
	test_eq (strtoDd ("0.0000000000000000000000000123", NULL), 0.0000000000000000000000000123dd);
	test_eq (strtoDd ("9999999999999999", NULL), 9999999999999999.dd);
	test_eq (strtoDd ("99999999999999999", NULL), 1e17dd);
	test_eq (strtoDd ("10000000000000000", NULL), 1e16dd);

	test_eq (strtoDd ("1e10", NULL), 1e10dd);
	test_eq (strtoDd ("-1e10", NULL), -1e10dd);
	test_eq (strtoDd ("+1e10", NULL), +1e10dd);
	test_eq (strtoDd ("1e-10", NULL), 1e-10dd);
	test_eq (strtoDd ("1e+10", NULL), 1e+10dd);

	test_eq (strtoDd ("1E10", NULL), 1e10dd);
	test_eq (strtoDd ("-1E10", NULL), -1e10dd);
	test_eq (strtoDd ("+1E10", NULL), +1e10dd);
	test_eq (strtoDd ("1E-10", NULL), 1e-10dd);
	test_eq (strtoDd ("1E+10", NULL), 1e+10dd);

	test_eq (strtoDd ("123e+f", NULL), 123.dd);

	test_eq (strtoDd ("infinity", NULL), INFINITY);
	test_eq (strtoDd ("-infinity", NULL), -(_Decimal64)INFINITY);
	test_eq (strtoDd ("+infinity", NULL), INFINITY);
	test_eq (strtoDd ("inf", NULL), INFINITY);
	test_eq (strtoDd ("-inf", NULL), -(_Decimal64)INFINITY);
	test_eq (strtoDd ("+inf", NULL), INFINITY);
	test_eq (strtoDd ("INFInity", NULL), INFINITY);
	test_eq (strtoDd ("-INFInity", NULL), -(_Decimal64)INFINITY);
	test_eq (strtoDd ("+INFInity", NULL), INFINITY);
	test_eq (strtoDd ("INF", NULL), INFINITY);
	test_eq (strtoDd ("-INF", NULL), -(_Decimal64)INFINITY);
	test_eq (strtoDd ("+INF", NULL), INFINITY);


	end_section ();
}



/* ------------------------------------------------------------------------- */

static void
test_encoding (void)
{
	static const _Decimal64 vals[] = {
		0.dd, 1.dd, 9.dd, 10.dd, 1e15dd, 999999999999999.dd,
		1000000000000001.dd, 9007199254740991.dd, 9007199254740992.dd,
		9007199254740993.dd, 9999999999999999.dd, 9999999999999999e10dd,
		1234567890123456.dd, 8999999999999999e-100dd, 9999999999999999e-100dd,
		1e-383dd, 9.99999999999999e-384dd, 1e-398dd, 5e-398dd,
		DECIMAL64_MAX, DECIMAL64_MIN, 1e369dd, 1e384dd,
		9999999999999999e369dd, 1000000000000000e369dd,
		INFINITY,
	};
	static const struct { uint64_t bits; int nan, fin, sign; } special[] = {
		{ 0x7800000000000000ull, 0, 0, 0 },  // +inf
		{ 0xf800000000000000ull, 0, 0, 1 },  // -inf
		{ 0x7c00000000000000ull, 1, 0, 0 },  // qNaN
		{ 0xfc00000000000000ull, 1, 0, 1 },  // -qNaN
		{ 0x7e00000000000000ull, 1, 0, 0 },  // sNaN
		{ 0x7c00000000000001ull, 1, 0, 0 },  // NaN with payload
		{ 0x7fffffffffffffffull, 1, 0, 0 },  // all ones (sign clear)
		{ 0xffffffffffffffffull, 1, 0, 1 },  // all ones
		{ 0x31c0000000000000ull, 0, 1, 0 },  // +0
		{ 0xb1c0000000000000ull, 0, 1, 1 },  // -0
		{ 0x0000000000000001ull, 0, 1, 0 },  // 1e-398
		{ 0x8000000000000001ull, 0, 1, 1 },  // -1e-398
	};
	char *s;

	start_section ("encoding: decode/make round trip and special encodings");

	// scalbnD (x, 0) decodes and re-encodes; the result must be bit-for-bit
	// what the compiler produced for the literal, so this checks decode64
	// and make64 across the 2^53 mantissa boundary, denormals and extremes.
	for (int i = 0; i < (int)G_N_ELEMENTS (vals); i++) {
		for (int sgn = 0; sgn < 2; sgn++) {
			_Decimal64 x = sgn ? -vals[i] : vals[i];
			_Decimal64 y = scalbnD (x, 0);
			test_true (d64_bits (x) == d64_bits (y),
				   "scalbnD (%.16Wg, 0): bits 0x%016lx -> 0x%016lx",
				   x, d64_bits (x), d64_bits (y));
		}
	}

	// The same, for random values
	for (int i = 0; i < 5000; i++) {
		_Decimal64 x = mk (rng_mant (), rng_range (-40, 40), rng () & 1);
		_Decimal64 y = scalbnD (x, 0);
		test_true (d64_bits (x) == d64_bits (y),
			   "scalbnD (%.16Wg, 0): bits 0x%016lx -> 0x%016lx",
			   x, d64_bits (x), d64_bits (y));
	}

	for (int i = 0; i < (int)G_N_ELEMENTS (special); i++) {
		_Decimal64 x = d64_from_bits (special[i].bits);
		test_true (!!isnanD (x) == special[i].nan &&
			   !!isfiniteD (x) == special[i].fin &&
			   !!signbitD (x) == special[i].sign,
			   "classification of 0x%016lx: nan=%d finite=%d sign=%d",
			   special[i].bits, isnanD (x), isfiniteD (x), signbitD (x));
	}

	// Non-canonical encodings are meant to be treated as zero.
	// 10^16 needs 54 bits, so it lives in the "11" form; it is a valid
	// bit pattern for the hardware but not a valid coefficient.
	{
		_Decimal64 inv1 = d64_from_bits ((3ull << 61) | (398ull << 51) | 992800745259008ull);
		_Decimal64 inv2 = d64_from_bits ((3ull << 61) | (398ull << 51) | ((1ull << 51) - 1));
		_Decimal64 invs[2];
		invs[0] = inv1;
		invs[1] = inv2;
		for (int i = 0; i < 2; i++) {
			_Decimal64 inv = invs[i];
			test_true (!isnanD (inv) && isfiniteD (inv),
				   "non-canonical %d is finite and not NaN", i);
			s = g_strdup_printf ("%Wg", inv);
			test_true (strcmp (s, "0") == 0,
				   "non-canonical %d prints as [%s]", i, s);
			g_free (s);
			EQ (hypotD (inv, 4.dd), 4.dd);
			EQ (hypotD (3.dd, inv), 3.dd);
			EQ (nextafterD (inv, INFINITY), 1e-398dd);
			EQ (nextafterD (inv, -INFINITY), -1e-398dd);
			s = g_strdup_printf ("%Wg", fmodD (inv, 3.dd));
			test_true (strcmp (s, "0") == 0,
				   "fmodD (non-canonical %d, 3) = [%s]", i, s);
			g_free (s);
			s = g_strdup_printf ("%Wg", scalbnD (inv, 5));
			test_true (strcmp (s, "0") == 0,
				   "scalbnD (non-canonical %d, 5) = [%s]", i, s);
			g_free (s);
		}
	}

	end_section ();
}

static void
test_special_values (void)
{
	start_section ("special values (C99 Annex F style)");

	// classification helpers
	test_true (isnanD (QNAN) && !isnanD (PINF) && !isnanD (0.dd), "isnanD");
	test_true (!isfiniteD (PINF) && !isfiniteD (NINF) && !isfiniteD (QNAN) &&
		   isfiniteD (0.dd) && isfiniteD (DECIMAL64_MAX) &&
		   isfiniteD (1e-398dd), "isfiniteD");
	test_true (signbitD (-0.dd) && !signbitD (0.dd) &&
		   signbitD (NINF) && !signbitD (PINF), "signbitD");
	EQ (fabsD (-0.dd), 0.dd);
	EQ (fabsD (NINF), PINF);
	EQ (fabsD (-1e-398dd), 1e-398dd);
	EQ (copysignD (3.dd, -0.dd), -3.dd);
	EQ (copysignD (-3.dd, 0.dd), 3.dd);
	EQ (copysignD (0.dd, -1.dd), -0.dd);
	EQ (copysignD (PINF, -1.dd), NINF);

	// exp family
	EQ (expD (0.dd), 1.dd);
	EQ (expD (-0.dd), 1.dd);
	EQ (expD (PINF), PINF);
	EQ (expD (NINF), 0.dd);
	NAN_ (expD (QNAN));
	EQ (expm1D (0.dd), 0.dd);
	EQ (expm1D (-0.dd), -0.dd);
	EQ (expm1D (PINF), PINF);
	EQ (expm1D (NINF), -1.dd);
	EQ (expm1D (-1e5dd), -1.dd);
	EQ (expm1D (1e-350dd), 1e-350dd);
	EQ (expm1D (-1e-350dd), -1e-350dd);

	// logs
	EQ (logD (1.dd), 0.dd);
	EQ (logD (0.dd), NINF);
	EQ (logD (-0.dd), NINF);
	EQ (logD (PINF), PINF);
	NAN_ (logD (-1.dd));
	NAN_ (logD (NINF));
	NAN_ (logD (QNAN));
	EQ (log2D (1.dd), 0.dd);
	EQ (log2D (0.dd), NINF);
	EQ (log2D (PINF), PINF);
	NAN_ (log2D (-1.dd));
	EQ (log10D (1.dd), 0.dd);
	EQ (log10D (0.dd), NINF);
	EQ (log10D (-0.dd), NINF);
	EQ (log10D (PINF), PINF);
	NAN_ (log10D (-1.dd));
	NAN_ (log10D (NINF));
	EQ (log1pD (0.dd), 0.dd);
	EQ (log1pD (-0.dd), -0.dd);
	EQ (log1pD (-1.dd), NINF);
	NAN_ (log1pD (-2.dd));
	NAN_ (log1pD (NINF));
	EQ (log1pD (PINF), PINF);
	EQ (log1pD (1e-350dd), 1e-350dd);
	EQ (log1pD (-1e-350dd), -1e-350dd);

	// Exact results that must be exactly right
	for (int i = -398; i <= 384; i += 7) {
		char *what = g_strdup_printf ("log10D (10^%d)", i);
		expect_eq (what, log10D (scalbnD (1.dd, i)), (_Decimal64)i, FALSE);
		g_free (what);
	}
	// log2D (2^i) ought to be exactly i, but the "p2 + M_LG10D * p10"
	// combination in log_helper() suffers catastrophic cancellation for
	// some values, up to several ulp (16-digit sense) off.
	{
		int bad = 0, total = 0;
		_Decimal64 worst = 0;
		_Decimal64 x = 1.dd;
		int firstbad = 99, lastbad = 99;

		for (int i = -60; i <= 60; i++) {
			_Decimal64 got, want, u;

			// The "double" part of this is exact, so this produces
			// the correctly-rounded x.
			x = (_Decimal64)(ldexp (1.0, i));
			got = log2D (x);
			want = (_Decimal64)i;
			u = ulp_err (got, want);
			total++;
			if (got != want) {
				bad++;
				if (u > worst)
					worst = u;
				firstbad = MIN (firstbad, i);
				lastbad = i;
			}
		}
		test_true (worst <= 10,
			   "log2D (2^i) is up to %.1Wg ulp off for i in [%d,%d]",
			   worst, firstbad, lastbad);
		test_true (bad == 0,
			   "log2D (2^i) is not exact for %d of %d values of i "
			   "in [%d,%d] (worst %.1Wg ulp)", bad, total,
			   firstbad, lastbad, worst);
	}

	// trig
	EQ (sinD (0.dd), 0.dd);
	EQ (sinD (-0.dd), -0.dd);
	EQ (sinD (1e-350dd), 1e-350dd);
	EQ (sinD (-1e-350dd), -1e-350dd);
	NAN_ (sinD (PINF));
	NAN_ (sinD (NINF));
	NAN_ (sinD (QNAN));
	EQ (cosD (0.dd), 1.dd);
	EQ (cosD (-0.dd), 1.dd);
	EQ (cosD (1e-350dd), 1.dd);
	NAN_ (cosD (PINF));
	NAN_ (cosD (NINF));
	EQ (tanD (0.dd), 0.dd);
	EQ (tanD (-0.dd), -0.dd);
	EQ (tanD (1e-350dd), 1e-350dd);
	NAN_ (tanD (PINF));
	NAN_ (tanD (QNAN));
	EQ (asinD (0.dd), 0.dd);
	EQ (asinD (-0.dd), -0.dd);
	EQ (asinD (1e-350dd), 1e-350dd);
	NAN_ (asinD (1.000000000000001dd));
	NAN_ (asinD (-1.000000000000001dd));
	NAN_ (asinD (PINF));
	CLOSE (asinD (1.dd), 1.570796326794897dd, 1);
	CLOSE (asinD (-1.dd), -1.570796326794897dd, 1);
	EQ (acosD (1.dd), 0.dd);
	NAN_ (acosD (1.000000000000001dd));
	NAN_ (acosD (NINF));
	CLOSE (acosD (-1.dd), 3.141592653589793dd, 1);
	CLOSE (acosD (0.dd), 1.570796326794897dd, 1);
	EQ (atanD (0.dd), 0.dd);
	EQ (atanD (-0.dd), -0.dd);
	EQ (atanD (1e-350dd), 1e-350dd);
	CLOSE (atanD (PINF), 1.570796326794897dd, 1);
	CLOSE (atanD (NINF), -1.570796326794897dd, 1);
	CLOSE (atanD (1.dd), 0.7853981633974483dd, 1);
	NAN_ (atanD (QNAN));

	// hyperbolic
	EQ (sinhD (0.dd), 0.dd);
	EQ (sinhD (-0.dd), -0.dd);
	EQ (sinhD (1e-350dd), 1e-350dd);
	EQ (sinhD (PINF), PINF);
	EQ (sinhD (NINF), NINF);
	EQ (coshD (0.dd), 1.dd);
	EQ (coshD (PINF), PINF);
	EQ (coshD (NINF), PINF);
	EQ (tanhD (0.dd), 0.dd);
	EQ (tanhD (-0.dd), -0.dd);
	EQ (tanhD (1e-350dd), 1e-350dd);
	EQ (tanhD (PINF), 1.dd);
	EQ (tanhD (NINF), -1.dd);
	EQ (tanhD (1e5dd), 1.dd);
	EQ (asinhD (0.dd), 0.dd);
	EQ (asinhD (-0.dd), -0.dd);
	EQ (asinhD (1e-350dd), 1e-350dd);
	EQ (asinhD (PINF), PINF);
	EQ (asinhD (NINF), NINF);
	EQ (acoshD (1.dd), 0.dd);
	EQ (acoshD (PINF), PINF);
	NAN_ (acoshD (0.5dd));
	NAN_ (acoshD (NINF));
	EQ (atanhD (0.dd), 0.dd);
	EQ (atanhD (-0.dd), -0.dd);
	EQ (atanhD (1e-350dd), 1e-350dd);
	EQ (atanhD (1.dd), PINF);
	EQ (atanhD (-1.dd), NINF);
	NAN_ (atanhD (1.000000000000001dd));
	NAN_ (atanhD (PINF));

	// error function
	EQ (erfD (0.dd), 0.dd);
	EQ (erfD (-0.dd), -0.dd);
	EQ (erfD (PINF), 1.dd);
	EQ (erfD (NINF), -1.dd);
	CLOSE (erfD (1e-350dd), 1.128379167095513e-350dd, 1);
	EQ (erfcD (0.dd), 1.dd);
	EQ (erfcD (PINF), 0.dd);
	EQ (erfcD (NINF), 2.dd);
	EQ (erfcD (1000.dd), 0.dd);
	NAN_ (erfcD (QNAN));

	// roots
	EQ (sqrtD (0.dd), 0.dd);
	EQ (sqrtD (-0.dd), -0.dd);
	EQ (sqrtD (PINF), PINF);
	NAN_ (sqrtD (NINF));
	NAN_ (sqrtD (-1e-398dd));
	NAN_ (sqrtD (QNAN));
	EQ (cbrtD (0.dd), 0.dd);
	EQ (cbrtD (-0.dd), -0.dd);
	EQ (cbrtD (PINF), PINF);
	EQ (cbrtD (NINF), NINF);
	NAN_ (cbrtD (QNAN));

	// gamma
	EQ (lgammaD (1.dd), 0.dd);
	EQ (lgammaD (2.dd), 0.dd);
	EQ (lgammaD (0.dd), PINF);
	EQ (lgammaD (-1.dd), PINF);
	EQ (lgammaD (-2.dd), PINF);
	EQ (lgammaD (PINF), PINF);
	EQ (lgammaD (NINF), PINF);
	NAN_ (lgammaD (QNAN));

	// hypot, atan2, pow, fmod
	EQ (hypotD (PINF, QNAN), PINF);
	EQ (hypotD (QNAN, NINF), PINF);
	EQ (hypotD (NINF, 3.dd), PINF);
	NAN_ (hypotD (QNAN, 3.dd));
	NAN_ (hypotD (3.dd, QNAN));
	EQ (hypotD (0.dd, 0.dd), 0.dd);
	EQ (hypotD (-0.dd, -0.dd), 0.dd);
	EQ (hypotD (-3.dd, 4.dd), 5.dd);
	EQ (hypotD (3.dd, -4.dd), 5.dd);

	EQ (fmodD (5.dd, PINF), 5.dd);
	EQ (fmodD (-5.dd, NINF), -5.dd);
	EQ (fmodD (0.dd, 3.dd), 0.dd);
	EQ (fmodD (-0.dd, 3.dd), -0.dd);
	NAN_ (fmodD (5.dd, 0.dd));
	NAN_ (fmodD (5.dd, -0.dd));
	NAN_ (fmodD (PINF, 3.dd));
	NAN_ (fmodD (NINF, 3.dd));
	NAN_ (fmodD (QNAN, 3.dd));
	NAN_ (fmodD (3.dd, QNAN));

	// The C99 table for pow
	EQ (powD (QNAN, 0.dd), 1.dd);
	EQ (powD (QNAN, -0.dd), 1.dd);
	EQ (powD (1.dd, QNAN), 1.dd);
	EQ (powD (1.dd, PINF), 1.dd);
	EQ (powD (1.dd, NINF), 1.dd);
	EQ (powD (-3.dd, 0.dd), 1.dd);
	EQ (powD (PINF, 0.dd), 1.dd);
	EQ (powD (0.dd, 0.dd), 1.dd);
	EQ (powD (-1.dd, PINF), 1.dd);
	EQ (powD (-1.dd, NINF), 1.dd);
	NAN_ (powD (QNAN, 2.dd));
	NAN_ (powD (2.dd, QNAN));
	EQ (powD (0.dd, -1.dd), PINF);
	EQ (powD (-0.dd, -1.dd), NINF);
	EQ (powD (0.dd, -2.dd), PINF);
	EQ (powD (-0.dd, -2.dd), PINF);
	EQ (powD (-0.dd, -1.5dd), PINF);
	EQ (powD (0.dd, NINF), PINF);
	EQ (powD (-0.dd, NINF), PINF);
	EQ (powD (0.dd, 3.dd), 0.dd);
	EQ (powD (-0.dd, 3.dd), -0.dd);
	EQ (powD (0.dd, 2.dd), 0.dd);
	EQ (powD (-0.dd, 2.dd), 0.dd);
	EQ (powD (-0.dd, 1.5dd), 0.dd);
	EQ (powD (0.dd, PINF), 0.dd);
	EQ (powD (0.5dd, PINF), 0.dd);
	EQ (powD (-0.5dd, PINF), 0.dd);
	EQ (powD (2.dd, PINF), PINF);
	EQ (powD (-2.dd, PINF), PINF);
	EQ (powD (0.5dd, NINF), PINF);
	EQ (powD (2.dd, NINF), 0.dd);
	EQ (powD (NINF, 3.dd), NINF);
	EQ (powD (NINF, 2.dd), PINF);
	EQ (powD (NINF, 2.5dd), PINF);
	EQ (powD (NINF, -3.dd), -0.dd);
	EQ (powD (NINF, -2.dd), 0.dd);
	EQ (powD (NINF, -2.5dd), 0.dd);
	EQ (powD (PINF, 1.dd), PINF);
	EQ (powD (PINF, -1.dd), 0.dd);
	NAN_ (powD (-8.dd, 0.3333333333333333dd));
	NAN_ (powD (-2.dd, 0.5dd));
	EQ (powD (-2.dd, 3.dd), -8.dd);
	EQ (powD (-2.dd, 4.dd), 16.dd);
	EQ (powD (-2.dd, -3.dd), -0.125dd);
	EQ (powD (-2.dd, -2.dd), 0.25dd);
	EQ (powD (-2.dd, 1e20dd), PINF);
	EQ (powD (-2.dd, 1e20dd + 1e5dd), PINF);
	EQ (powD (10.dd, 0.dd), 1.dd);
	EQ (powD (10.dd, 2.dd), 100.dd);
	EQ (powD (10.dd, -2.dd), 0.01dd);
	EQ (powD (10.dd, 384.dd), 1e384dd);
	EQ (powD (10.dd, 385.dd), PINF);
	EQ (powD (10.dd, -398.dd), 1e-398dd);
	EQ (powD (10.dd, -399.dd), 0.dd);
	EQ (powD (10.dd, 1e12dd), PINF);
	EQ (powD (10.dd, -1e12dd), 0.dd);

	// atan2: argument order is (y, x)
	EQ (atan2D (0.dd, 1.dd), 0.dd);
	EQ (atan2D (-0.dd, 1.dd), -0.dd);
	CLOSE (atan2D (0.dd, -1.dd), 3.141592653589793dd, 1);
	CLOSE (atan2D (-0.dd, -1.dd), -3.141592653589793dd, 1);
	EQ (atan2D (0.dd, 0.dd), 0.dd);
	EQ (atan2D (-0.dd, 0.dd), -0.dd);
	CLOSE (atan2D (0.dd, -0.dd), 3.141592653589793dd, 1);
	CLOSE (atan2D (-0.dd, -0.dd), -3.141592653589793dd, 1);
	CLOSE (atan2D (1.dd, 0.dd), 1.570796326794897dd, 1);
	CLOSE (atan2D (-1.dd, 0.dd), -1.570796326794897dd, 1);
	CLOSE (atan2D (1.dd, -0.dd), 1.570796326794897dd, 1);
	EQ (atan2D (1.dd, PINF), 0.dd);
	EQ (atan2D (-1.dd, PINF), -0.dd);
	CLOSE (atan2D (1.dd, NINF), 3.141592653589793dd, 1);
	CLOSE (atan2D (-1.dd, NINF), -3.141592653589793dd, 1);
	CLOSE (atan2D (PINF, 1.dd), 1.570796326794897dd, 1);
	CLOSE (atan2D (NINF, 1.dd), -1.570796326794897dd, 1);
	CLOSE (atan2D (PINF, PINF), 0.7853981633974483dd, 1);
	CLOSE (atan2D (NINF, PINF), -0.7853981633974483dd, 1);
	CLOSE (atan2D (PINF, NINF), 2.356194490192345dd, 1);
	CLOSE (atan2D (NINF, NINF), -2.356194490192345dd, 1);
	NAN_ (atan2D (QNAN, 1.dd));
	NAN_ (atan2D (1.dd, QNAN));
	CLOSE (atan2D (1.dd, 2.dd), 0.4636476090008061dd, 1);
	CLOSE (atan2D (2.dd, 1.dd), 1.107148717794091dd, 1);
	CLOSE (atan2D (-1.dd, -1.dd), -2.356194490192345dd, 1);
	CLOSE (atan2D (1.dd, -1.dd), 2.356194490192345dd, 1);
	CLOSE (atan2D (-1.dd, 1.dd), -0.7853981633974483dd, 1);

	end_section ();
}

/* ------------------------------------------------------------------------- */

#ifdef HAVE___UINT128_T
typedef unsigned __int128 u128;

static u128
u128_pow10 (int n)
{
	u128 r = 1;
	while (n-- > 0)
		r *= 10;
	return r;
}
#endif

static void
expect_rounding (const char *fn, _Decimal64 x, _Decimal64 got, _Decimal64 want)
{
	test_true (decimal_eq (got, want),
		   "%s (%.16Wg) = %.16Wg, expected %.16Wg", fn, x, got, want);
}

static void
test_rounding_exact (void)
{
	start_section ("floor, ceil, round, trunc, modf vs. exact integer arithmetic");

	for (int i = 0; i < 40000; i++) {
		uint64_t m = rng_mant ();
		int e = rng_range (-22, 3);
		int neg = rng () & 1;
		_Decimal64 x = mk (m, e, neg);
		uint64_t q, rem = 0;
		gboolean frac = FALSE, half = FALSE;
		_Decimal64 f, c, r, t, fr, ip;

		if (e >= 0) {
			q = 0;  // unused
		} else if (-e <= 19) {
			uint64_t p = pow10u[-e];
			q = m / p;
			rem = m % p;
			frac = rem != 0;
			half = rem >= p - rem;
		} else {
			q = 0;
			rem = m;
			frac = m != 0;
			half = FALSE;
		}

		if (e >= 0) {
			f = c = r = t = x;
			fr = mk (0, 0, neg);
		} else if (!neg) {
			f = (_Decimal64)q;
			c = (_Decimal64)(q + frac);
			r = (_Decimal64)(q + half);
			t = (_Decimal64)q;
			fr = mk (rem, e, 0);
		} else {
			f = -(_Decimal64)(q + frac);
			c = -(_Decimal64)q;
			r = -(_Decimal64)(q + half);
			t = -(_Decimal64)q;
			fr = mk (rem, e, 1);
		}

		expect_rounding ("floorD", x, floorD (x), f);
		expect_rounding ("ceilD", x, ceilD (x), c);
		expect_rounding ("roundD", x, roundD (x), r);
		expect_rounding ("truncD", x, truncD (x), t);
		{
			_Decimal64 got = modfD (x, &ip);
			expect_rounding ("modfD (fraction)", x, got, fr);
			expect_rounding ("modfD (integer)", x, ip, t);
		}
	}

	end_section ();
}

static void
test_rounding_table (void)
{
	start_section ("floor, ceil, round, trunc: table");

	// Around the 10^15 where floorD and friends change strategy
	EQ (floorD (999999999999999.9dd), 999999999999999.dd);
	EQ (ceilD (999999999999999.1dd), 1e15dd);
	EQ (roundD (999999999999999.5dd), 1e15dd);
	EQ (roundD (999999999999999.4dd), 999999999999999.dd);
	EQ (truncD (999999999999999.9dd), 999999999999999.dd);
	EQ (floorD (1e15dd), 1e15dd);
	EQ (ceilD (1e15dd), 1e15dd);
	EQ (roundD (1e15dd), 1e15dd);
	EQ (floorD (1e16dd), 1e16dd);
	EQ (ceilD (9999999999999999e300dd), 9999999999999999e300dd);
	EQ (roundD (DECIMAL64_MAX), DECIMAL64_MAX);
	EQ (floorD (-DECIMAL64_MAX), -DECIMAL64_MAX);

	// Denormals and other tiny values
	EQ (floorD (1e-398dd), 0.dd);
	EQ (ceilD (1e-398dd), 1.dd);
	EQ (roundD (1e-398dd), 0.dd);
	EQ (truncD (1e-398dd), 0.dd);
	EQ (floorD (-1e-398dd), -1.dd);
	EQ (ceilD (-1e-398dd), -0.dd);
	EQ (roundD (-1e-398dd), -0.dd);
	EQ (truncD (-1e-398dd), -0.dd);
	EQ (floorD (1e-17dd), 0.dd);
	EQ (ceilD (1e-17dd), 1.dd);
	EQ (roundD (1e-17dd), 0.dd);

	// Ties go away from zero
	EQ (roundD (0.5dd), 1.dd);
	EQ (roundD (1.5dd), 2.dd);
	EQ (roundD (2.5dd), 3.dd);
	EQ (roundD (-0.5dd), -1.dd);
	EQ (roundD (-2.5dd), -3.dd);
	EQ (roundD (0.4999999999999999dd), 0.dd);
	EQ (roundD (-0.4999999999999999dd), -0.dd);
	EQ (roundD (0.5000000000000001dd), 1.dd);

	// Signed zeros and non-finite
	EQ (floorD (-0.dd), -0.dd);
	EQ (ceilD (-0.dd), -0.dd);
	EQ (roundD (-0.dd), -0.dd);
	EQ (truncD (-0.dd), -0.dd);
	EQ (floorD (0.dd), 0.dd);
	EQ (floorD (PINF), PINF);
	EQ (floorD (NINF), NINF);
	EQ (ceilD (PINF), PINF);
	EQ (ceilD (NINF), NINF);
	EQ (roundD (PINF), PINF);
	EQ (roundD (NINF), NINF);
	EQ (truncD (PINF), PINF);
	EQ (truncD (NINF), NINF);
	NAN_ (floorD (QNAN));
	NAN_ (ceilD (QNAN));
	NAN_ (roundD (QNAN));
	NAN_ (truncD (QNAN));

	// -0.3 has floor -1 but the others give -0
	EQ (floorD (-0.3dd), -1.dd);
	EQ (ceilD (-0.3dd), -0.dd);
	EQ (truncD (-0.3dd), -0.dd);
	EQ (roundD (-0.3dd), -0.dd);
	EQ (floorD (0.3dd), 0.dd);
	EQ (ceilD (0.3dd), 1.dd);

	// modf
	{
		_Decimal64 ip;
		EQ (modfD (3.25dd, &ip), 0.25dd);
		EQ (ip, 3.dd);
		EQ (modfD (-3.25dd, &ip), -0.25dd);
		EQ (ip, -3.dd);
		EQ (modfD (-0.25dd, &ip), -0.25dd);
		EQ (ip, -0.dd);
		EQ (modfD (5.dd, &ip), 0.dd);
		EQ (ip, 5.dd);
		EQ (modfD (-5.dd, &ip), -0.dd);
		EQ (ip, -5.dd);
		EQ (modfD (PINF, &ip), 0.dd);
		EQ (ip, PINF);
		EQ (modfD (NINF, &ip), -0.dd);
		EQ (ip, NINF);
		NAN_ (modfD (QNAN, &ip));
		NAN_ (ip);
		EQ (modfD (1e-398dd, &ip), 1e-398dd);
		EQ (ip, 0.dd);
	}

	end_section ();
}

static void
test_nextafter_exact (void)
{
	start_section ("nextafter vs. exact arithmetic");

	for (int i = 0; i < 20000; i++) {
		uint64_t m = rng_mant ();
		int e = rng_range (-30, 30);
		int neg = rng () & 1;
		gboolean up = rng () & 1;
		_Decimal64 x = mk (m, e, neg);
		_Decimal64 want, got;

		if (m == 0) {
			// Smallest denormals
			want = up ? 1e-398dd : -1e-398dd;
		} else {
			// Away from zero?
			gboolean away = (up != !!neg);
			uint64_t m2 = m;
			int e2 = e;
			while (m2 < 1000000000000000ull) {
				m2 *= 10;
				e2--;
			}
			if (away) {
				m2++;
				if (m2 == 10000000000000000ull) {
					m2 = 1000000000000000ull;
					e2++;
				}
			} else {
				if (m2 == 1000000000000000ull) {
					m2 = 9999999999999999ull;
					e2--;
				} else
					m2--;
			}
			want = mk (m2, e2, neg);
		}

		got = nextafterD (x, up ? PINF : NINF);
		test_true (decimal_eq (got, want),
			   "nextafterD (%.16Wg, %cinf) = %.16Wg, expected %.16Wg",
			   x, up ? '+' : '-', got, want);
	}

	// Explicit boundary cases
	EQ (nextafterD (1.dd, 2.dd), 1.000000000000001dd);
	EQ (nextafterD (1.dd, 0.dd), 0.9999999999999999dd);
	EQ (nextafterD (-1.dd, -2.dd), -1.000000000000001dd);
	EQ (nextafterD (-1.dd, 0.dd), -0.9999999999999999dd);
	EQ (nextafterD (10.dd, 11.dd), 10.00000000000001dd);
	EQ (nextafterD (10.dd, 9.dd), 9.999999999999999dd);
	EQ (nextafterD (DECIMAL64_MIN, 0.dd), 9.99999999999999e-384dd);
	EQ (nextafterD (9.99999999999999e-384dd, 1.dd), DECIMAL64_MIN);
	EQ (nextafterD (1e-398dd, -1.dd), 0.dd);
	EQ (nextafterD (-1e-398dd, 1.dd), -0.dd);
	EQ (nextafterD (1e-398dd, 1.dd), 2e-398dd);
	EQ (nextafterD (2e-398dd, 0.dd), 1e-398dd);
	EQ (nextafterD (0.dd, 1.dd), 1e-398dd);
	EQ (nextafterD (-0.dd, 1.dd), 1e-398dd);
	EQ (nextafterD (0.dd, -1.dd), -1e-398dd);
	EQ (nextafterD (DECIMAL64_MAX, 0.dd), 9.999999999999998e384dd);
	EQ (nextafterD (-DECIMAL64_MAX, 0.dd), -9.999999999999998e384dd);
	EQ (nextafterD (9.999999999999998e384dd, PINF), DECIMAL64_MAX);
	EQ (nextafterD (1e384dd, 0.dd), 9.999999999999999e383dd);

	// Equal arguments: y is returned, including its zero sign
	EQ (nextafterD (1.dd, 1.dd), 1.dd);
	EQ (nextafterD (0.dd, -0.dd), -0.dd);
	EQ (nextafterD (-0.dd, 0.dd), 0.dd);
	NAN_ (nextafterD (QNAN, 1.dd));
	NAN_ (nextafterD (1.dd, QNAN));
	EQ (nextafterD (NINF, 0.dd), -DECIMAL64_MAX);
	EQ (nextafterD (PINF, NINF), DECIMAL64_MAX);

	// Stepping through the ulp wraps around the decades smoothly
	{
		_Decimal64 x = 9999999999999990.dd;
		for (int i = 0; i < 20; i++) {
			_Decimal64 y = nextafterD (x, PINF);
			test_true (y > x, "nextafterD (%.16Wg) must increase", x);
			test_true (nextafterD (y, NINF) == x,
				   "nextafterD steps down from %.16Wg to %.16Wg",
				   y, x);
			x = y;
		}
	}

	end_section ();
}

static void
test_fmod_exact (void)
{
	start_section ("fmodD vs. exact integer arithmetic");

#ifdef HAVE___UINT128_T
	for (int i = 0; i < 40000; i++) {
		uint64_t mx = rng_mant (), my = rng_mant ();
		int ey = rng_range (-20, 20);
		int ex = ey + rng_range (-22, 22);
		int negx = rng () & 1, negy = rng () & 1;
		_Decimal64 x, y, got, want;
		uint64_t r;
		int er;

		if (my == 0)
			continue;
		if (ex >= ey) {
			u128 X = (u128)mx * u128_pow10 (ex - ey);
			r = (uint64_t)(X % my);
			er = ey;
		} else {
			u128 Y = (u128)my * u128_pow10 (ey - ex);
			r = (uint64_t)((u128)mx % Y);
			er = ex;
		}

		x = mk (mx, ex, negx);
		y = mk (my, ey, negy);
		want = mk (r, er, negx);
		got = fmodD (x, y);
		test_true (decimal_eq (got, want),
			   "fmodD (%.16Wg, %.16Wg) = %.16Wg, expected %.16Wg",
			   x, y, got, want);
	}
#endif

	EQ (fmodD (5.5dd, 2.dd), 1.5dd);
	EQ (fmodD (-5.5dd, 2.dd), -1.5dd);
	EQ (fmodD (5.5dd, -2.dd), 1.5dd);
	EQ (fmodD (-5.5dd, -2.dd), -1.5dd);
	EQ (fmodD (6.dd, 3.dd), 0.dd);
	EQ (fmodD (-6.dd, 3.dd), -0.dd);
	EQ (fmodD (1.dd, 3.dd), 1.dd);
	EQ (fmodD (1.dd, 0.1dd), 0.dd);
	EQ (fmodD (0.3dd, 0.1dd), 0.dd);
	EQ (fmodD (0.7dd, 0.2dd), 0.1dd);
	EQ (fmodD (1e-398dd * 7, 1e-398dd * 3), 1e-398dd);
	EQ (fmodD (DECIMAL64_MAX, 1e-398dd), 0.dd);
	EQ (fmodD (DECIMAL64_MAX, DECIMAL64_MAX), 0.dd);
	EQ (fmodD (-DECIMAL64_MAX, DECIMAL64_MAX), -0.dd);
	EQ (fmodD (1e-398dd, DECIMAL64_MAX), 1e-398dd);
	EQ (fmodD (9999999999999999e369dd, 3.dd), 0.dd);
	EQ (fmodD (1e300dd, 7.dd), 1.dd);            // 10^300 mod 7 == 1
	EQ (fmodD (1e30dd, 1e15dd + 1), 1.dd);       // 10^15 == -1 (mod 10^15+1)
	EQ (fmodD (123456789.123456dd, 0.001dd), 0.000456dd);
	EQ (fmodD (1234567890123456.dd, 1000.dd), 456.dd);

	end_section ();
}

static void
test_scalbn_frexp (void)
{
	_Decimal64 m;
	int e;

	start_section ("scalbln, unscalbn, frexp, ldexp");

	EQ (scalblnD (1.dd, G_MAXLONG), PINF);
	EQ (scalblnD (-1.dd, G_MAXLONG), NINF);
	EQ (scalblnD (1.dd, G_MINLONG), 0.dd);
	EQ (scalblnD (-1.dd, G_MINLONG), -0.dd);
	EQ (scalblnD (0.dd, G_MAXLONG), 0.dd);
	EQ (scalblnD (1.dd, 1000000), PINF);
	EQ (scalblnD (1.dd, -1000000), 0.dd);
	// scalbnD multiplies by 10^n; below 1e-398 it rounds, ties away from 0
	EQ (scalbnD (1e-398dd, -1), 0.dd);
	EQ (scalbnD (4e-398dd, -1), 0.dd);
	EQ (scalbnD (14e-398dd, -1), 1e-398dd);
	EQ (scalbnD (15e-398dd, -1), 2e-398dd);
	EQ (scalbnD (25e-398dd, -1), 3e-398dd);
	EQ (scalbnD (-25e-398dd, -1), -3e-398dd);
	EQ (scalbnD (-4e-398dd, -1), -0.dd);
	EQ (scalbnD (1.dd, 384), 1e384dd);
	EQ (scalbnD (9999999999999999.dd, 369), 9999999999999999e369dd);
	EQ (scalbnD (9999999999999999.dd, 370), PINF);
	// EQ (scalbnD (1000000000000000.dd, 370), 1e385dd);
	EQ (scalbnD (1.dd, 385), PINF);

	m = unscalbnD (123.45dd, &e);
	EQ (m, 0.12345dd);
	test_true (e == 3, "unscalbnD (123.45) exponent %d", e);
	m = unscalbnD (-5.5dd, &e);
	EQ (m, -0.55dd);
	test_true (e == 1, "unscalbnD (-5.5) exponent %d", e);
	m = unscalbnD (1e-398dd, &e);
	EQ (m, 0.1dd);
	test_true (e == -397, "unscalbnD (1e-398) exponent %d", e);
	m = unscalbnD (9999999999999999e369dd, &e);
	EQ (m, 0.9999999999999999dd);
	test_true (e == 385, "unscalbnD (max) exponent %d", e);
	m = unscalbnD (0.dd, &e);
	EQ (m, 0.dd);
	test_true (e == 0, "unscalbnD (0) exponent %d", e);
	m = unscalbnD (-0.dd, &e);
	EQ (m, -0.dd);
	m = unscalbnD (PINF, &e);
	EQ (m, PINF);
	m = unscalbnD (NINF, &e);
	EQ (m, NINF);
	m = unscalbnD (QNAN, &e);
	NAN_ (m);

	for (int i = 0; i < 5000; i++) {
		uint64_t mant = rng_mant ();
		int ex = rng_range (-40, 40);
		_Decimal64 x = mk (mant, ex, rng () & 1);
		m = unscalbnD (x, &e);
		if (mant == 0)
			continue;
		test_true (fabsD (m) >= 0.1dd && fabsD (m) < 1.dd &&
			   scalbnD (m, e) == x,
			   "unscalbnD (%.16Wg) = %.16Wg * 10^%d", x, m, e);
	}

	m = frexpD (8.dd, &e);
	CLOSE (m, 0.5dd, 1);
	test_true (e == 4, "frexpD (8) exponent %d", e);
	m = frexpD (1.dd, &e);
	CLOSE (m, 0.5dd, 1);
	test_true (e == 1, "frexpD (1) exponent %d", e);
	m = frexpD (0.75dd, &e);
	CLOSE (m, 0.75dd, 1);
	test_true (e == 0, "frexpD (0.75) exponent %d", e);
	m = frexpD (-3.dd, &e);
	CLOSE (m, -0.75dd, 1);
	test_true (e == 2, "frexpD (-3) exponent %d", e);
	m = frexpD (0.dd, &e);
	EQ (m, 0.dd);
	test_true (e == 0, "frexpD (0) exponent %d", e);
	m = frexpD (-0.dd, &e);
	EQ (m, -0.dd);
	m = frexpD (PINF, &e);
	EQ (m, PINF);
	m = frexpD (QNAN, &e);
	NAN_ (m);

	// frexp/ldexp round trip over the full range, including the parts
	// beyond the range of double
	for (int i = -398; i <= 384; i += 3) {
		_Decimal64 x = scalbnD (7.dd / 3, i);
		m = frexpD (x, &e);
		test_true (fabsD (m) >= 0.5dd && fabsD (m) <= 1.dd,
			   "frexpD (%.16Wg) mantissa %.16Wg out of range", x, m);
		CLOSE (ldexpD (m, e), x, 4);
	}

	EQ (ldexpD (1.dd, 3), 8.dd);
	EQ (ldexpD (3.dd, -1), 1.5dd);
	EQ (ldexpD (1.dd, 10), 1024.dd);
	EQ (ldexpD (-1.dd, 10), -1024.dd);
	EQ (ldexpD (0.dd, 100), 0.dd);
	EQ (ldexpD (-0.dd, 100), -0.dd);
	EQ (ldexpD (PINF, -100), PINF);
	NAN_ (ldexpD (QNAN, 1));
	EQ (ldexpD (1.dd, 0), 1.dd);
	CLOSE (ldexpD (1.dd, 60), 1152921504606846976.dd, 1);
	CLOSE (ldexpD (1.dd, -10), 0.0009765625dd, 1);
	CLOSE (ldexpD (1.dd, -1000), 9.332636185032189e-302dd, 1);

	end_section ();
}

/* ------------------------------------------------------------------------- */

static void
test_range (void)
{
	start_section ("results outside the range of double");

	// Decimal64 reaches 1e-398 ... 1e385, double only 1e-308 ... 1e308.
	// These have results in the former but not the latter range.

	// Ones that work
	CLOSE (expD (700.dd), 1.014232054735005e304dd, 3);
	CLOSE (expD (-700.dd), 9.859676543759771e-305dd, 3);
	CLOSE (expD (1.dd), 2.718281828459045dd, 1);
	CLOSE (expD (-1.dd), 0.3678794411714423dd, 1);
	CLOSE (expD (10.dd), 22026.46579480672dd, 1);
	CLOSE (expm1D (1e-5dd), 1.000005000016667e-5dd, 1);
	CLOSE (sinhD (1.dd), 1.175201193643801dd, 1);
	CLOSE (coshD (1.dd), 1.543080634815244dd, 1);
	CLOSE (tanhD (0.5dd), 0.4621171572600098dd, 1);
	CLOSE (sinhD (700.dd), 5.071160273675023e303dd, 3);
	CLOSE (powD (3.dd, 200.dd), 2.6561398887587476e95dd, 3);
	CLOSE (powD (9.215870288754e16dd, 19), 2.119291601259874e322dd, 3);

	// exp, sinh, cosh: the true result is far away from overflowing
	CLOSE (expD (800.dd), 2.726374572112567e347dd, 3);
	CLOSE (expD (885.dd), 2.241901277130624e384dd, 3);
	EQ (expD (888.dd), PINF);
	CLOSE (expD (-800.dd), 3.667874584177687e-348dd, 3);
	CLOSE (expD (-900.dd), 1.364477212365683e-391dd, 3);
	EQ (expD (-1000.dd), 0.dd);
	CLOSE (expm1D (800.dd), 2.726374572112567e347dd, 3);
	EQ (expm1D (-800.dd), -1.dd);
	CLOSE (sinhD (800.dd), 1.363187286056283e347dd, 3);
	CLOSE (sinhD (-800.dd), -1.363187286056283e347dd, 3);
	CLOSE (coshD (800.dd), 1.363187286056283e347dd, 3);
	CLOSE (coshD (-800.dd), 1.363187286056283e347dd, 3);
	CLOSE (sinhD (750.dd), 2.629247270727402e325dd, 3);
	EQ (sinhD (1000.dd), PINF);
	EQ (coshD (-1000.dd), PINF);

	// pow
	CLOSE (powD (2.dd, 1100.dd), 1.358298529049386e331dd, 3);
	CLOSE (powD (-2.dd, 1101.dd), -2.716597058098772e331dd, 3);
	CLOSE (powD (0.5dd, 1200.dd), 5.807713756217503e-362dd, 3);
	CLOSE (powD (3.dd, 700.dd), 9.657802140591758e333dd, 3);
	EQ (powD (0.9dd, -9000.dd), PINF);
	EQ (powD (2.dd, 1300.dd), PINF);
	EQ (powD (2.dd, -1400.dd), 0.dd);

	// lgamma
	CLOSE (lgammaD (1e300dd), 6.897755278982137e302dd, 2);
	CLOSE (lgammaD (1e306dd), 7.03591038456178e308dd, 2);
	EQ (lgammaD (DECIMAL64_MAX), PINF);   // ~7e387, truly out of range
	CLOSE (lgammaD (1e-300dd), 690.7755278982137dd, 2);
	CLOSE (lgammaD (1e-320dd), 736.8272297580946dd, 2);
	CLOSE (lgammaD (-1e-320dd), 736.8272297580946dd, 2);
	CLOSE (lgammaD (-1e-350dd), 805.904782547916dd, 2);

	// erfc: the answer is representable long after double gives up
	CLOSE (erfcD (26.dd), 5.663192408856143e-296dd, 3);
	CLOSE (erfcD (27.dd), 5.237048923789256e-319dd, 3);
	CLOSE (erfcD (28.dd), 6.563215840328784e-343dd, 3);
	CLOSE (erfcD (30.dd), 2.564656203756112e-393dd, 3);
	EQ (erfcD (31.dd), 0.dd);   // below the smallest denormal

	// atan2 with a tiny quotient
	CLOSE (atanD (1e-350dd), 1e-350dd, 1);
	CLOSE (atan2D (1e-300dd, 1e50dd), 1e-350dd, 1);
	CLOSE (atan2D (-1e-300dd, 1e50dd), -1e-350dd, 1);
	CLOSE (atan2D (1e-390dd, 1e-100dd), 1e-290dd, 1);
	CLOSE (atan2D (1e-300dd, 1e-200dd), 1e-100dd, 1);
	CLOSE (atan2D (1e-200dd, 1e50dd), 1e-250dd, 1);
	CLOSE (atan2D (1e-200dd, -1e50dd), 3.141592653589793dd, 1);

	// asinh, acosh: beyond double the log branch is used
	CLOSE (asinhD (1e320dd), 737.5203769386546dd, 2);
	CLOSE (acoshD (1e320dd), 737.5203769386546dd, 2);
	CLOSE (asinhD (1e300dd), 691.4686750787737dd, 2);
	CLOSE (acoshD (1e300dd), 691.4686750787737dd, 2);
	CLOSE (asinhD (-1e320dd), -737.5203769386546dd, 2);

	// Bessel: J_n(x) ~ (x/2)^n / n! underflows double before decimal does
	CLOSE (jnD (0, 1e-300dd), 1.dd, 1);
	CLOSE (jnD (1, 1e-320dd), 5e-321dd, 1);
	CLOSE (jnD (-1, 1e-320dd), -5e-321dd, 1);
	CLOSE (jnD (3, 1e-100dd), 2.083333333333333e-302dd, 3);
	CLOSE (jnD (2, 1e-190dd), 1.25e-381dd, 3);
	CLOSE (jnD (3, 1e-120dd), 2.083333333333333e-362dd, 3);
	CLOSE (jnD (3, 5e-107dd), 2.604166666666667e-321dd, 3);

	// ldexp: 2^n is computed in double
	CLOSE (ldexpD (1e-300dd, 1400), 2.766902970275812e121dd, 2);
	CLOSE (ldexpD (1e300dd, -1500), 2.851060964896706e-152dd, 2);
	CLOSE (ldexpD (1e-300dd, 900), 8.452712498170644e-30dd, 2);
	end_section ();
}

static void
test_lgamma (void)
{
	int sign;

	start_section ("lgamma");

	CLOSE (lgammaD (0.5dd), 0.5723649429247001dd, 2);
	CLOSE (lgammaD (10.dd), 12.80182748008147dd, 2);
	CLOSE (lgammaD (100.dd), 359.1342053695754dd, 2);
	CLOSE (lgammaD (1e20dd), 4.505170185988091e21dd, 2);
	CLOSE (lgammaD (-0.5dd), 1.265512123484645dd, 2);
	CLOSE (lgammaD (-1.5dd), 0.860047015376481dd, 2);

	EQ (lgammaD_r (0.5dd, &sign), lgammaD (0.5dd));
	test_true (sign == 1, "sign of gamma (0.5) is %d", sign);
	(void)lgammaD_r (-0.5dd, &sign);
	test_true (sign == -1, "sign of gamma (-0.5) is %d", sign);
	(void)lgammaD_r (-1.5dd, &sign);
	test_true (sign == 1, "sign of gamma (-1.5) is %d", sign);
	(void)lgammaD_r (-2.5dd, &sign);
	test_true (sign == -1, "sign of gamma (-2.5) is %d", sign);
	(void)lgammaD_r (1e-320dd, &sign);
	test_true (sign == 1, "sign of gamma (1e-320) is %d", sign);
	(void)lgammaD_r (-1e-320dd, &sign);
	test_true (sign == -1, "sign of gamma (-1e-320) is %d", sign);
	(void)lgammaD_r (0.dd, &sign);
	test_true (sign == 1, "sign of gamma (+0) is %d", sign);

	// C99: lgamma (-0) returns +inf and sets the sign to -1
	sign = 0;
	EQ (lgammaD_r (-0.dd, &sign), PINF);
	test_true (sign == -1, "sign of gamma (-0) is %d, expected -1", sign);

	end_section ();
}

static void
test_bessel (void)
{
	start_section ("Bessel functions jnD, ynD");

	// ----------------------------------------------------------------
	// Moderate arguments.  jnD/ynD are thin wrappers around libm's
	// jn()/yn() here (see the FIXME in go-decimal.c), so this mostly
	// guards against a broken wrapper (wrong argument order, sign
	// slip, off-by-one in n, etc.) rather than testing new math -- but
	// unlike every other one-arg function in go-decimal.c, jnD/ynD had
	// *no* coverage at all for this, their normal operating range;
	// the only existing tests exercised the tiny-x power-series branch
	// of jnD.  Reference values via mpmath.besselj/bessely.
	set_subsection ("moderate x, n=0..3");
	CLOSE (jnD (0, 0.5dd), 0.9384698072408129dd, 20);
	CLOSE (jnD (0, 1.dd), 0.7651976865579666dd, 20);
	CLOSE (jnD (0, 2.dd), 0.2238907791412357dd, 20);
	CLOSE (jnD (0, 5.dd), -0.1775967713143383dd, 20);
	CLOSE (jnD (0, 10.dd), -0.2459357644513483dd, 20);
	CLOSE (jnD (0, 100.dd), 0.01998585030422312dd, 20);
	CLOSE (jnD (1, 0.5dd), 0.2422684576748739dd, 20);
	CLOSE (jnD (1, 1.dd), 0.4400505857449335dd, 20);
	CLOSE (jnD (1, 2.dd), 0.5767248077568734dd, 20);
	CLOSE (jnD (1, 5.dd), -0.3275791375914652dd, 20);
	CLOSE (jnD (1, 10.dd), 0.04347274616886144dd, 20);
	CLOSE (jnD (1, 100.dd), -0.07714535201411216dd, 20);
	CLOSE (jnD (2, 0.5dd), 0.03060402345868264dd, 20);
	CLOSE (jnD (2, 1.dd), 0.1149034849319005dd, 20);
	CLOSE (jnD (2, 2.dd), 0.3528340286156377dd, 20);
	CLOSE (jnD (2, 5.dd), 0.04656511627775222dd, 20);
	CLOSE (jnD (2, 10.dd), 0.2546303136851206dd, 20);
	CLOSE (jnD (2, 100.dd), -0.02152875734450537dd, 20);
	CLOSE (jnD (3, 0.5dd), 0.002563729994587244dd, 20);
	CLOSE (jnD (3, 1.dd), 0.01956335398266841dd, 20);
	CLOSE (jnD (3, 2.dd), 0.1289432494744020dd, 20);
	CLOSE (jnD (3, 5.dd), 0.3648312306136670dd, 20);
	CLOSE (jnD (3, 10.dd), 0.05837937930518681dd, 20);
	CLOSE (jnD (3, 100.dd), 0.07628420172033194dd, 20);

	CLOSE (ynD (0, 0.5dd), -0.4445187335067066dd, 20);
	CLOSE (ynD (0, 1.dd), 0.08825696421567696dd, 20);
	CLOSE (ynD (0, 2.dd), 0.5103756726497451dd, 20);
	CLOSE (ynD (0, 5.dd), -0.3085176252490338dd, 20);
	CLOSE (ynD (0, 10.dd), 0.05567116728359939dd, 20);
	CLOSE (ynD (0, 100.dd), -0.07724431336508315dd, 20);
	CLOSE (ynD (1, 0.5dd), -1.471472392670243dd, 20);
	CLOSE (ynD (1, 1.dd), -0.7812128213002887dd, 20);
	CLOSE (ynD (1, 2.dd), -0.1070324315409376dd, 20);
	CLOSE (ynD (1, 5.dd), 0.1478631433912268dd, 20);
	CLOSE (ynD (1, 10.dd), 0.2490154242069539dd, 20);
	CLOSE (ynD (1, 100.dd), -0.02037231200275979dd, 20);
	CLOSE (ynD (2, 0.5dd), -5.441370837174266dd, 20);
	CLOSE (ynD (2, 1.dd), -1.650682606816254dd, 20);
	CLOSE (ynD (2, 2.dd), -0.6174081041906827dd, 20);
	CLOSE (ynD (2, 5.dd), 0.3676628826055245dd, 20);
	CLOSE (ynD (2, 10.dd), -0.005868082442208615dd, 20);
	CLOSE (ynD (2, 100.dd), 0.07683686712502796dd, 20);
	CLOSE (ynD (3, 0.5dd), -42.05949430472388dd, 20);
	CLOSE (ynD (3, 1.dd), -5.821517605964729dd, 20);
	CLOSE (ynD (3, 2.dd), -1.127783776840428dd, 20);
	CLOSE (ynD (3, 5.dd), 0.1462671626931928dd, 20);
	CLOSE (ynD (3, 10.dd), -0.2513626571838373dd, 20);
	CLOSE (ynD (3, 100.dd), 0.02344578668776091dd, 20);

	// ----------------------------------------------------------------
	// Special values.  J_n(0) = 1 for n=0, else 0; Y_n(0) = -inf for
	// every n >= 0 (it is an infinite discontinuity at x=0, not a
	// signed pole the way the negative-order identity might suggest);
	// both are NaN for negative or NaN x, and go to (signed) zero as
	// x -> +-inf.
	set_subsection ("special values at x=0, +-inf, nan, and x<0");
	EQ (jnD (0, 0.dd), 1.dd);
	EQ (jnD (1, 0.dd), 0.dd);
	EQ (jnD (2, 0.dd), 0.dd);
	EQ (jnD (0, -0.dd), 1.dd);
	EQ (ynD (0, 0.dd), NINF);
	EQ (ynD (1, 0.dd), NINF);
	EQ (ynD (2, 0.dd), NINF);
	EQ (ynD (3, 0.dd), NINF);

	// Unlike ynD, jnD *is* defined for negative x at integer order,
	// via J_n(-x) = (-1)^n J_n(x); it must not turn that into NaN.
	EQ (jnD (0, -1.dd), jnD (0, 1.dd));
	EQ (jnD (1, -1.dd), -jnD (1, 1.dd));
	NAN_ (ynD (0, -1.dd));
	NAN_ (ynD (1, -1.dd));
	NAN_ (jnD (0, QNAN));
	NAN_ (ynD (0, QNAN));

	EQ (jnD (0, PINF), 0.dd);
	EQ (jnD (1, PINF), 0.dd);
	EQ (ynD (0, PINF), 0.dd);
	EQ (ynD (1, PINF), 0.dd);

	// ----------------------------------------------------------------
	// Tiny arguments.  Casting these to double flushes anything below
	// ~4.9e-324 to zero, for which yn() returns -infinity, although the
	// true value is finite: ~(2/pi)(ln(x/2)+gamma) for n=0, and
	// ~-(n-1)!/pi*(2/x)^n otherwise.  Reference values via mpmath.
	set_subsection ("ynD: tiny x, including x that underflow in double");
	CLOSE (ynD (0, 1e-15dd), -22.06187226149152dd, 4);
	CLOSE (ynD (0, 1e-20dd), -29.3912282502858dd, 4);
	CLOSE (ynD (0, 1e-100dd), -146.6609240709942dd, 4);
	CLOSE (ynD (0, 1e-300dd), -439.8351636227653dd, 4);
	CLOSE (ynD (0, 1e-320dd), -469.1525875779424dd, 4);
	CLOSE (ynD (0, 1e-330dd), -483.811299555531dd, 4);
	CLOSE (ynD (0, 1e-398dd), -583.4905410031332dd, 4);
	CLOSE (ynD (1, 1e-15dd), -636619772367581.3dd, 4);
	CLOSE (ynD (1, 1e-100dd), -6.366197723675813e99dd, 4);
	CLOSE (ynD (1, 1e-300dd), -6.366197723675813e299dd, 4);
	CLOSE (ynD (1, 1e-320dd), -6.366197723675813e319dd, 4);
	CLOSE (ynD (1, 1e-330dd), -6.366197723675813e329dd, 4);
	CLOSE (ynD (1, 1e-380dd), -6.366197723675813e379dd, 4);
	// Result fits even though 2/x does not
	CLOSE (ynD (1, 1e-385dd), -6.366197723675813e384dd, 4);
	EQ (ynD (1, 1e-386dd), NINF);
	CLOSE (ynD (2, 1e-15dd), -1.273239544735163e30dd, 4);
	CLOSE (ynD (2, 1e-100dd), -1.273239544735163e200dd, 4);
	CLOSE (ynD (2, 1e-120dd), -1.273239544735163e240dd, 4);
	CLOSE (ynD (3, 1e-15dd), -5.092958178940651e45dd, 4);
	CLOSE (ynD (3, 1e-100dd), -5.092958178940651e300dd, 4);
	CLOSE (ynD (4, 1e-50dd), -3.05577490736439e201dd, 4);
	// True result beyond the range of Decimal64
	EQ (ynD (2, 1e-320dd), NINF);
	EQ (ynD (3, 1e-300dd), NINF);
	EQ (ynD (200, 1e-15dd), NINF);
	EQ (ynD (G_MAXINT, 1e-300dd), NINF);
	// Negative orders: Y_{-n} = (-1)^n Y_n
	CLOSE (ynD (-1, 1e-20dd), 6.366197723675813e19dd, 4);
	CLOSE (ynD (-2, 1e-20dd), -1.273239544735163e40dd, 4);
	CLOSE (ynD (-3, 1e-20dd), 5.092958178940651e60dd, 4);
	CLOSE (ynD (-1, 1e-320dd), 6.366197723675813e319dd, 4);
	test_true (ynD (-1, 1e-320dd) > 0, "ynD (-1, tiny) is positive");
	EQ (ynD (-3, 1e-300dd), PINF);
	// Still consistent with the unspecial-cased poles
	EQ (ynD (0, 0.dd), NINF);
	NAN_ (ynD (0, -1e-320dd));
	NAN_ (ynD (1, -1e-20dd));

	// ----------------------------------------------------------------
	// Order-negation identities: J_{-n}(x) = (-1)^n J_n(x) and
	// Y_{-n}(x) = (-1)^n Y_n(x), for x > 0.
	set_subsection ("negative order identities");
	for (int n = 0; n <= 4; n++) {
		_Decimal64 x = 3.5dd;
		_Decimal64 sign = (n & 1) ? -1.dd : 1.dd;
		CLOSE (jnD (-n, x), sign * jnD (n, x), 4);
		CLOSE (ynD (-n, x), sign * ynD (n, x), 4);
	}

	// ----------------------------------------------------------------
	// Known limitation (see the FIXME comments on jnD/ynD in
	// go-decimal.c): both functions fall back to libm's jn()/yn() on
	// a plain "double" cast of x for anything outside jnD's tiny-x
	// series branch, with no large-argument handling of their own.
	// _Decimal64 can represent huge values like 1e30 *exactly*, but
	// the nearest "double" to 1e30 is off by a relative ~1e-16, i.e.
	// by tens of units at that magnitude -- and since these are
	// oscillatory functions with period 2*pi, an absolute error that
	// large in the argument leaves the phase (and so the sign and
	// magnitude of the result) effectively unrelated to the true
	// answer.  This first becomes visible somewhere around x=1e23;
	// by x=1e30 it is already unambiguous.  We first confirm the
	// still-good regime below that threshold, then mark the broken
	// one as xfail with the true reference value (via mpmath,
	// evaluated at the *exact* decimal argument, which is how a fix
	// would need to do it -- not at the double-rounded argument).
	set_subsection ("large x: below the phase-loss threshold (should be fine)");
	CLOSE (jnD (0, 1e15dd), 6.156638646885022e-09dd, 20);
	CLOSE (ynD (0, 1e15dd), 2.446866512377132e-08dd, 20);
	CLOSE (jnD (0, 1e20dd), 6.698009040703426e-12dd, 20);
	CLOSE (ynD (0, 1e20dd), -7.95068198242545e-11dd, 20);

	set_subsection ("large x: known phase-loss bug above the threshold (xfail)");
	CLOSE_XF (jnD (0, 1e30dd), -6.127370231742229e-16dd, 1000);
	CLOSE_XF (ynD (0, 1e30dd), 5.110509884533765e-16dd, 1000);
	CLOSE_XF (jnD (0, 1e50dd), -7.916714451809859e-26dd, 1000);
	CLOSE_XF (ynD (0, 1e50dd), -9.93785452330156e-27dd, 1000);
	CLOSE_XF (jnD (0, 1e100dd), 5.444250503374563e-51dd, 1000);
	CLOSE_XF (ynD (0, 1e100dd), -5.832847820170174e-51dd, 1000);
	CLOSE_XF (jnD (0, 1e200dd), -7.804001009113744e-101dd, 1000);
	CLOSE_XF (jnD (0, 1e300dd), -4.316312013000372e-151dd, 1000);

	end_section ();
}

static void
test_log_accuracy (void)
{
	static const struct {
		_Decimal64 x, log2, ln, log10;
	} tab[] = {
		{ 0.6296650893515833dd, -0.6673434129506684dd, -0.4625672051520071dd, -0.2008903847069261dd },
		{ 0.3333333333333333dd, -1.584962500721156dd, -1.09861228866811dd, -0.4771212547196625dd },
		{ 0.1234567890123456dd, -3.017921920981519dd, -2.091864070678394dd, -0.9084850227873004dd },
		{ 3.141592653589793dd, 1.651496129472319dd, 1.1447298858494dd, 0.4971498726941338dd },
		{ 1.234567890123456dd, 0.3040061739058436dd, 0.2107210223156519dd, 0.09151497721269962dd },
		{ 7.777777777777777dd, 2.959358015502654dd, 2.05127066471314dd, 0.8908555305749319dd },
		{ 0.7071067811865476dd, -0.4999999999999998dd, -0.3465735902799725dd, -0.1505149978319906dd },
		{ 2.623398742333038dd, 1.391437106064881dd, 0.9644707069953619dd, 0.4188643060054137dd },
		{ 0.4321098765432109dd, -1.210529888544139dd, -0.839075379227915dd, -0.3644058070995619dd },
	};

	start_section ("log accuracy on 16-digit arguments");

	for (int i = 0; i < (int)G_N_ELEMENTS (tab); i++) {
		_Decimal64 x = tab[i].x;
		char *what;

		// log2D suffers cancellation (see log2D (2^i) above) and can be
		// off by tens of ulp even for an ordinary 16-digit argument.
		what = g_strdup_printf ("log2D (%.16Wg)", x);
		expect_close (what, log2D (x), tab[i].log2, 100, FALSE);
		g_free (what);
		what = g_strdup_printf ("logD (%.16Wg)", x);
		expect_close (what, logD (x), tab[i].ln, 5, FALSE);
		g_free (what);
		what = g_strdup_printf ("log10D (%.16Wg)", x);
		expect_close (what, log10D (x), tab[i].log10, 2, FALSE);
		g_free (what);
	}

	CLOSE (logD (2.718281828459045dd), 1.dd, 1);
	CLOSE (logD (1e-398dd), -916.428867011630dd, 5);
	CLOSE (logD (DECIMAL64_MAX), 886.4952608027076dd, 2);
	CLOSE (log1pD (-0.99dd), -4.605170185988091dd, 2);
	CLOSE (log1pD (1e300dd), 690.7755278982137dd, 2);
	CLOSE (log1pD (1e-5dd), 9.999950000333331e-6dd, 2);
	CLOSE (log1pD (-0.999999dd), -13.81551055796427dd, 2);
	CLOSE (log1pD (-0.9999999999999999dd), -36.84136148790473dd, 2);
	CLOSE (log1pD (-0.99999999999dd), -25.3284360229345dd, 2);

	end_section ();
}

static void
test_trig_accuracy (void)
{
	start_section ("trig accuracy");

	// Small arguments: fine
	CLOSE (sinD (1.5dd), 0.9974949866040544dd, 2);
	CLOSE (sinD (100.dd), -0.5063656411097588dd, 2);
	CLOSE (sinD (-100.dd), 0.5063656411097588dd, 2);
	CLOSE (cosD (1e6dd), 0.9367521275331448dd, 2);
	CLOSE (sinD (1e22dd), -0.8522008497671888dd, 2);
	CLOSE (asinD (0.5dd), 0.5235987755982989dd, 1);
	CLOSE (acosD (0.5dd), 1.047197551196598dd, 1);
	CLOSE (atanD (0.5dd), 0.4636476090008061dd, 1);
	CLOSE (atanhD (0.95dd), 1.831780823064823dd, 2);
	CLOSE (atanhD (0.9999999999999999dd), 18.76725433423234dd, 2);

	// The argument is converted to double and so is off by up to 1.1e-16
	// relative, which is a lot when it gets multiplied by the argument.
	CLOSE_XF (sinD (1000.1dd), 0.8788928116493108dd, 2);
	CLOSE_XF (cosD (12345.678dd), 0.7101193587161447dd, 2);
	CLOSE (sinD (100000.5dd), -0.4477465716572126dd, 2);
	CLOSE (tanD (1000.5dd), 10.24926078683731dd, 2);
	// 3.141592653589793 is not the double closest to pi, and so
	// sin (M_PID) is 2.38e-16, not 1.22e-16.
	CLOSE_XF (sinD (M_PID), 2.384626433832795e-16dd, 2);
	CLOSE (cosD (M_PID), -1.dd, 1);
	// Not representable in double, and no argument reduction
	CLOSE_XF (sinD (1.23456789e20dd), 0.604109571088993dd, 2);
	CLOSE_XF (tanD (1e300dd), -0.4911375502073446dd, 2);

	end_section ();
}

static void
test_roots_hypot (void)
{
	int sqrt_scaled_bad = 0, sqrt_scaled_total = 0;
	_Decimal64 sqrt_scaled_worst = 0;
	int cbrt_bad = 0, cbrt_total = 0;
	_Decimal64 cbrt_worst = 0;

	start_section ("sqrt, cbrt, hypot: exact and near-exact results");

	// sqrtD (k^2) for integer k is exact
	for (int k = 1; k <= 3000; k++) {
		_Decimal64 dk = k;
		char *what = g_strdup_printf ("sqrtD (%d^2)", k);
		expect_eq (what, sqrtD (dk * dk), dk, FALSE);
		g_free (what);
	}

	// sqrtD (k^2 / 10^4): still an exactly representable perfect square,
	// but scaling away from a bare integer coefficient is enough to lose
	// the last-digit exactness in some cases.
	for (int k = 1; k <= 3000; k++) {
		_Decimal64 dk = k;
		_Decimal64 got = sqrtD (dk * dk / 10000);
		_Decimal64 want = dk / 100;
		_Decimal64 u = ulp_err (got, want);
		sqrt_scaled_total++;
		if (got != want) {
			sqrt_scaled_bad++;
			if (u > sqrt_scaled_worst)
				sqrt_scaled_worst = u;
		}
	}
	test_true (sqrt_scaled_worst <= 2,
		   "sqrtD (k^2 / 1e4) is up to %.1Wg ulp off", sqrt_scaled_worst);
	test_true (sqrt_scaled_bad == 0,
		   "sqrtD (k^2 / 1e4) is not exact in %d of %d cases (worst %.1Wg ulp)",
		   sqrt_scaled_bad, sqrt_scaled_total, sqrt_scaled_worst);

	// cbrtD (k^3) for integer k: often but not always exact.
	for (int k = 1; k <= 2000; k++) {
		_Decimal64 dk = k;
		for (int neg = 0; neg < 2; neg++) {
			_Decimal64 base = neg ? -dk : dk;
			_Decimal64 got = cbrtD (base * base * base);
			_Decimal64 u = ulp_err (got, base);
			cbrt_total++;
			if (got != base) {
				cbrt_bad++;
				if (u > cbrt_worst)
					cbrt_worst = u;
			}
		}
	}
	test_true (cbrt_worst <= 5,
		   "cbrtD (k^3) is up to %.1Wg ulp off", cbrt_worst);
	test_true (cbrt_bad == 0,
		   "cbrtD (k^3) is not exact in %d of %d cases (worst %.1Wg ulp)",
		   cbrt_bad, cbrt_total, cbrt_worst);

	EQ (sqrtD (0.01dd), 0.1dd);
	EQ (sqrtD (2.25dd), 1.5dd);
	EQ (sqrtD (1e300dd), 1e150dd);
	EQ (sqrtD (1e-300dd), 1e-150dd);
	EQ (sqrtD (1e-396dd), 1e-198dd);
	EQ (sqrtD (1e384dd), 1e192dd);
	EQ (cbrtD (0.001dd), 0.1dd);
	EQ (cbrtD (-8.dd), -2.dd);
	EQ (cbrtD (1e300dd), 1e100dd);
	EQ (cbrtD (1e-300dd), 1e-100dd);
	EQ (cbrtD (-1e384dd), -1e128dd);
	CLOSE (sqrtD (2.dd), 1.414213562373095dd, 1);
	CLOSE (sqrtD (DECIMAL64_MAX), 3.162277660168379e192dd, 1);
	CLOSE (sqrtD (1e-398dd), 1e-199dd, 1);
	CLOSE (cbrtD (2.dd), 1.259921049894873dd, 1);
	CLOSE (cbrtD (DECIMAL64_MAX), 2.154434690031884e128dd, 1);

	// hypot
	EQ (hypotD (3.dd, 4.dd), 5.dd);
	EQ (hypotD (5.dd, 12.dd), 13.dd);
	EQ (hypotD (8.dd, 15.dd), 17.dd);
	EQ (hypotD (0.3dd, 0.4dd), 0.5dd);
	EQ (hypotD (3e-200dd, 4e-200dd), 5e-200dd);
	EQ (hypotD (3e200dd, 4e200dd), 5e200dd);
	EQ (hypotD (3e-398dd, 4e-398dd), 5e-398dd);
	EQ (hypotD (1e-398dd, 0.dd), 1e-398dd);
	EQ (hypotD (0.dd, DECIMAL64_MAX), DECIMAL64_MAX);
	EQ (hypotD (DECIMAL64_MAX, 1.dd), DECIMAL64_MAX);
	EQ (hypotD (DECIMAL64_MAX, DECIMAL64_MAX), PINF);
	EQ (hypotD (5e-398dd, 5e-398dd), 7e-398dd);
	CLOSE (hypotD (1e200dd, 1e200dd), 1.414213562373095e200dd, 1);
	CLOSE (hypotD (1.dd, 1.dd), 1.414213562373095dd, 1);

	end_section ();
}

static void
test_pow_exact (void)
{
	int wrong = 0, total = 0;
	_Decimal64 worst = 0;

	start_section ("pow with integer exponents");

	// Things that are exactly representable
	EQ (powD (2.dd, 10.dd), 1024.dd);
	EQ (powD (2.dd, 50.dd), 1125899906842624.dd);
	EQ (powD (3.dd, 5.dd), 243.dd);
	EQ (powD (5.dd, 3.dd), 125.dd);
	EQ (powD (0.5dd, 3.dd), 0.125dd);
	EQ (powD (0.1dd, 1.dd), 0.1dd);
	EQ (powD (1.5dd, 2.dd), 2.25dd);
	EQ (powD (-1.5dd, 3.dd), -3.375dd);
	EQ (powD (7.dd, 0.dd), 1.dd);
	EQ (powD (7.dd, 1.dd), 7.dd);
	EQ (powD (7.dd, -1.dd), 0.1428571428571429dd);  // off by 1 ulp: 0.1428571428571428
	EQ (powD (4.dd, 0.5dd), 2.dd);
	EQ (powD (8.dd, 0.3333333333333333dd) > 1.999999999999999dd, 1.dd);
	EQ (powD (10.dd, 3.dd), 1000.dd);
	EQ (powD (10.dd, -3.dd), 0.001dd);
	EQ (powD (-10.dd, 3.dd), -1000.dd);

#ifdef HAVE___UINT128_T
	// Compounding: x^n for 4-digit x.  Compare against exact integer
	// arithmetic, rounding the 16 digits half-even (the digit string of
	// m^n is at most 37 digits here so it fits in a u128).
	for (int m = 1001; m < 2000; m += 7) {
		u128 p = 1;
		for (int n = 1; n <= 11; n++) {
			int drop;
			u128 lim, q, r, half;
			uint64_t mant;
			_Decimal64 want, got;

			p *= m;
			if (n < 2)
				continue;
			// value = p * 10^(-3n); scale p to 16 digits
			drop = 0;
			lim = 10000000000000000ull;
			q = p;
			while (q >= lim) {
				q /= 10;
				drop++;
			}
			r = p - q * u128_pow10 (drop);
			half = u128_pow10 (drop) / 2;
			mant = (uint64_t)q;
			if (drop > 0 && (r > half || (r == half && (mant & 1))))
				mant++;
			want = mk (mant, drop - 3 * n, 0);
			got = powD ((_Decimal64)m / 1000, (_Decimal64)n);
			total++;
			if (got != want) {
				_Decimal64 u = ulp_err (got, want);
				wrong++;
				if (u > worst)
					worst = u;
			}
		}
	}
	// Doing this through double is not correctly rounded.  Being off in
	// the last digit or two is unfortunate but tolerable; being off by
	// 10 or more is not.
	test_true (worst <= 10, "powD (x, n) is up to %.1Wg ulp off", worst);
	test_xfail (wrong == 0,
		    "powD (x, n) for 4-digit x is not correctly rounded in %d of %d cases (worst %.1Wg ulp)",
		    wrong, total, worst);
#endif

	end_section ();
}

static void
test_strtoDd_extra (void)
{
	char *end;
	_Decimal64 x;

	start_section ("strtoDd: additional coverage");

	// end pointer placement
	x = strtoDd ("  +1.5abc", &end);
	EQ (x, 1.5dd);
	test_true (strcmp (end, "abc") == 0, "strtoDd end pointer after \"  +1.5abc\": [%s]", end);
	x = strtoDd ("-.5", &end);
	EQ (x, -0.5dd);
	test_true (*end == 0, "strtoDd end pointer after \"-.5\"");
	x = strtoDd ("5.", &end);
	EQ (x, 5.dd);
	test_true (*end == 0, "strtoDd end pointer after \"5.\"");
	x = strtoDd ("1e", &end);
	EQ (x, 1.dd);
	test_true (strcmp (end, "e") == 0, "strtoDd \"1e\" does not consume a bare \"e\": [%s]", end);
	x = strtoDd ("1e+", &end);
	EQ (x, 1.dd);
	test_true (strcmp (end, "e+") == 0, "strtoDd \"1e+\" does not consume \"e+\": [%s]", end);
	x = strtoDd ("1e+5x", &end);
	EQ (x, 1e5dd);
	test_true (strcmp (end, "x") == 0, "strtoDd end pointer after \"1e+5x\": [%s]", end);
	x = strtoDd ("no digits here", &end);
	EQ (x, 0.dd);
	test_true (strcmp (end, "no digits here") == 0,
		   "strtoDd consumes nothing from \"no digits here\": [%s]", end);
	x = strtoDd ("", &end);
	EQ (x, 0.dd);
	x = strtoDd ("   ", &end);
	EQ (x, 0.dd);
	test_true (strcmp (end, "   ") == 0, "strtoDd consumes nothing from spaces alone");
	x = strtoDd ("+", &end);
	EQ (x, 0.dd);
	test_true (strcmp (end, "+") == 0, "strtoDd does not consume a lone sign");
	x = strtoDd (".", &end);
	EQ (x, 0.dd);
	test_true (strcmp (end, ".") == 0, "strtoDd does not consume a lone dot");
	x = strtoDd ("1,5", &end);
	EQ (x, 1.dd);
	test_true (strcmp (end, ",5") == 0, "strtoDd stops at a comma in the \"C\" locale: [%s]", end);

	// IEEE 754 / the compiler's own literal parsing round exact ties at
	// the 17th significant digit to even.  strtoDd instead rounds such
	// ties away from zero, so it disagrees with "1.0000000000000005dd"
	// as parsed by the compiler (which is exactly 1.0, since the 16th
	// digit is already even).
	EQ_XF (strtoDd ("1.0000000000000005", NULL), 1.dd);
	EQ (strtoDd ("1.0000000000000015", NULL), 1.000000000000002dd);  // half-even == half-up here
	EQ_XF (strtoDd ("1.0000000000000025", NULL), 1.000000000000002dd);
	EQ (strtoDd ("9999999999999999.5", NULL), 1e16dd);  // half-even == half-up here
	EQ (strtoDd ("2.5", NULL), 2.5dd);

	// Extreme exponents: overflow/underflow must saturate, not wrap
	EQ (strtoDd ("1e2147483647", NULL), PINF);
	EQ (strtoDd ("1e-2147483647", NULL), 0.dd);
	EQ (strtoDd ("1e2147483648", NULL), PINF);   // one more than INT_MAX
	EQ (strtoDd ("-1e2147483648", NULL), NINF);
	EQ (strtoDd ("9999999999999999e2147483647", NULL), PINF);
	EQ (strtoDd ("0.0000000000000001e-2147483647", NULL), 0.dd);
	EQ (strtoDd ("1e400", NULL), PINF);
	EQ (strtoDd ("1e-400", NULL), 0.dd);
	EQ (strtoDd ("1e385", NULL), PINF);
	EQ (strtoDd ("1e384", NULL), 1e384dd);
	EQ (strtoDd ("1e-398", NULL), 1e-398dd);
	EQ (strtoDd ("1e-399", NULL), 0.dd);
	EQ_XF (strtoDd ("5e-399", NULL), 0.dd);  // exact tie; half-even rounds to 0, this rounds to 1e-398
	EQ (strtoDd ("6e-399", NULL), 1e-398dd);
	EQ (strtoDd ("1.5e-398", NULL), 2e-398dd);   // half-even == half-up here
	EQ_XF (strtoDd ("2.5e-398", NULL), 2e-398dd);  // tie rounds away from zero instead

	// Very many digits before the decimal point, with a compensating
	// negative exponent: must not overflow the internal scale counter.
	{
		GString *s = g_string_new (NULL);
		g_string_append (s, "1");
		for (int i = 0; i < 200; i++)
			g_string_append_c (s, '0');
		g_string_append (s, "e-200");
		x = strtoDd (s->str, &end);
		EQ (x, 1.dd);
		test_true (*end == 0, "strtoDd consumed all of a 200-zero literal");
		g_string_free (s, TRUE);
	}
	{
		// A huge number of digits paired with a huge negative exponent
		// that would overflow a 32 bit accumulator (the digit count is
		// added to the exponent internally).
		GString *s = g_string_new (NULL);
		for (int i = 0; i < 300; i++)
			g_string_append_c (s, '1');
		g_string_append (s, "e-2147483400");
		x = strtoDd (s->str, &end);
		EQ (x, 0.dd);
		g_string_free (s, TRUE);
	}

	// Leading zeros do not count against the 17-significant-digit window
	EQ (strtoDd ("0000000000001.5", NULL), 1.5dd);
	EQ (strtoDd ("00000000000000000000000.5", NULL), 0.5dd);
	EQ (strtoDd (".00000000000000000000001", NULL), 1e-23dd);

	end_section ();
}

static void
test_printf_extra (void)
{
	char *s;

#define PF(fmt, val, want) \
	do { \
		char *got = g_strdup_printf (fmt, (_Decimal64)(val)); \
		test_true (strcmp (got, (want)) == 0, \
			   "printf (\"%s\", %.16Wg) = [%s], expected [%s]", \
			   fmt, (_Decimal64)(val), got, want); \
		g_free (got); \
	} while (0)

// Like PF, but for cases where the current implementation is known not
// to match plain-double printf semantics.
#define PF_XF(fmt, val, want) \
	do { \
		char *got = g_strdup_printf (fmt, (_Decimal64)(val)); \
		test_xfail (strcmp (got, (want)) == 0, \
			    "printf (\"%s\", %.16Wg) = [%s], expected [%s]", \
			    fmt, (_Decimal64)(val), got, want); \
		g_free (got); \
	} while (0)

	start_section ("printf: additional coverage");

	PF ("%.2Wf", 3.14159dd, "3.14");
	PF ("%.0Wf", 3.14159dd, "3");
	PF ("%.5Wf", 1.dd, "1.00000");
	PF ("%Wf", 1.dd, "1.000000");
	PF ("%.2Wf", -1.5dd, "-1.50");
	PF ("%08.2Wf", 1.5dd, "00001.50");
	PF ("%+.2Wf", 1.5dd, "+1.50");
	PF ("% .2Wf", 1.5dd, " 1.50");
	PF ("%-8.2Wf|", 1.5dd, "1.50    |");
	PF ("%.2We", 12345.dd, "1.23e+04");
	PF ("%.2WE", 12345.dd, "1.23E+04");
	PF ("%.0We", 9.dd, "9e+00");
	PF ("%.3Wg", 0.0001234dd, "0.000123");
	PF ("%.3Wg", 123400.dd, "1.23e+05");
	PF ("%Wg", 100.dd, "100");
	PF ("%Wg", 0.dd, "0");
	PF ("%Wg", -0.dd, "-0");
	PF ("%Wf", 0.dd, "0.000000");
	PF ("%Wf", -0.dd, "-0.000000");
	PF ("%We", PINF, "inf");
	PF ("%WE", PINF, "INF");
	PF ("%We", NINF, "-inf");
	PF ("%Wf", QNAN, "nan");
	PF ("%WF", QNAN, "NAN");
	PF ("%Wg", PINF, "inf");
	PF ("%5We", PINF, "  inf");
	PF ("%-5We|", PINF, "inf  |");

	// The zero-fill flag has no effect on nonfinite values (C99 7.19.6.1p8)
	PF ("%05We", PINF, "  inf");
	PF ("%05We", QNAN, "  nan");

	// %#: keep the decimal point / trailing zeros for %f, %e, %g
	PF ("%#.0Wf", 3.dd, "3.");
	PF_XF ("%#.0We", 3.dd, "3.e+00");
	PF_XF ("%#.3Wg", 1.dd, "1.00");
	PF_XF ("%#.0Wg", 100.dd, "1.e+02");

	// precision 0 for %g means precision 1
	PF ("%.0Wg", 123.dd, "1e+02");
	PF ("%.0Wg", 0.dd, "0");

	// Sign handling combined with zero-padding: the sign must stay to
	// the left of the padding, matching plain double printf.
	PF_XF ("%06.1Wf", -1.dd, "-001.0");
	PF_XF ("%+06.1Wf", 1.dd, "+001.0");

	// %g boundary: exponent >= -4 and < precision uses %f style, and
	// the decision must use the *rounded* exponent, not the pre-round one.
	PF ("%.1Wg", 0.99dd, "1");
	PF ("%.1Wg", 9.99dd, "1e+01");
	PF_XF ("%.0Wg", 0.000099999dd, "0.0001");
	PF_XF ("%.3Wg", 0.000099999dd, "0.0001");
	PF_XF ("%.1Wg", 0.000095dd, "0.0001");
	PF_XF ("%.6Wg", 99999.95dd, "1e+05");

	// Very high precision: exercise the buffer sizing, especially
	// around Decimal64's ~385-digit integer part.
	// Ideally "%.120Wf" would print 120 digits after the point, but the
	// precision is silently clamped to 100.
	s = g_strdup_printf ("%.120Wf", 1.dd);
	test_xfail (strlen (s) == 122 && g_str_has_prefix (s, "1.") &&
		    strspn (s + 2, "0") == 120,
		    "printf (\"%%.120Wf\", 1) has the right shape: got %d chars, wanted 122",
		    (int)strlen (s));
	test_true (strlen (s) == 102 && g_str_has_prefix (s, "1.") &&
		   strspn (s + 2, "0") == 100,
		   "printf (\"%%.120Wf\", 1) is clamped to 100 digits: [%d chars]",
		   (int)strlen (s));
	g_free (s);
	s = g_strdup_printf ("%.2Wf", 1e384dd);
	test_true (s[0] == '1' && strlen (s) == 388,
		   "printf (\"%%.2Wf\", 1e384) has the right length: %d",
		   (int)strlen (s));
	g_free (s);

	// A width larger than the formatted text must still be honored.
	s = g_strdup_printf ("%400.2Wf", 1.dd);
	test_true (strlen (s) == 400, "printf (\"%%400.2Wf\", 1) has length %d",
		   (int)strlen (s));
	g_free (s);

	end_section ();
}

/* ------------------------------------------------------------------------- */

// A handful of regressions that are severe enough (crashes, hangs, memory
// corruption) that we do not want a single bad case to take the whole test
// binary down with it.  Each one is run in a forked child so a crash is
// reported as a failure instead of losing every later test too.

typedef void (*ChildFunc) (void);

static gboolean
run_in_child (ChildFunc fn)
{
#ifdef G_OS_UNIX
	pid_t pid = fork ();
	if (pid == 0) {
		// Child.  Silence the child's own stdout/stderr chatter (if
		// any) and let a crash speak for itself via the exit status.
		signal (SIGABRT, SIG_DFL);
		fn ();
		_exit (0);
	} else if (pid > 0) {
		int status;
		while (waitpid (pid, &status, 0) != pid)
			; /* nothing */
		return WIFEXITED (status) && WEXITSTATUS (status) == 0;
	} else {
		// fork failed; run inline and hope for the best
		fn ();
		return TRUE;
	}
#else
	fn ();
	return TRUE;
#endif
}

static void
child_decimal128_wide_printf (void)
{
	char buf[64];
	// A Decimal128 value whose decimal exponent is far larger than
	// Decimal64's ~385-digit range.  decimal_format's fixed-size
	// on-stack buffer for the integer part is sized for Decimal64.
	snprintf (buf, sizeof (buf), "%.2WLf", 1e3000dl);
}

static void
child_huge_exponent_strtoDd (void)
{
	// Many leading digits plus a huge exponent; the internal exponent
	// accumulator is (at least in older builds) a plain "int".
	char s[400];
	int i;
	for (i = 0; i < 300; i++)
		s[i] = '1';
	strcpy (s + i, "e2147483400");
	(void) strtoDd (s, NULL);
}

static void
test_regressions (void)
{
	start_section ("regressions: things that must not crash, corrupt, or race "
			"(run last, and each in its own forked child, so a crash "
			"here cannot take any other test down with it; for a real "
			"check of the threading case, rerun this binary under "
			"-fsanitize=thread or valgrind --tool=helgrind, since a "
			"plain run can pass even when this is unsafe)");

	test_true (run_in_child (child_decimal128_wide_printf),
		   "printf (\"%%.2WLf\", 1e3000dl) must not overflow a stack buffer");
	test_true (run_in_child (child_huge_exponent_strtoDd),
		   "strtoDd on 300 digits + a huge exponent must not misbehave "
		   "on integer overflow of the internal exponent accumulator");

	// Beyond just "must not crash": now that the scale accumulator
	// cannot silently overflow, this specific input must also produce
	// the mathematically correct answer.  Its true magnitude is far
	// beyond Decimal64's range, so it must saturate to +Infinity --
	// not, as the unpatched overflow used to silently produce, 0.
	{
		GString *s = g_string_new (NULL);
		for (int i = 0; i < 300; i++)
			g_string_append_c (s, '1');
		g_string_append (s, "e2147483400");
		EQ (strtoDd (s->str, NULL), PINF);
		g_string_free (s, TRUE);
	}
	end_section ();
}

/* ------------------------------------------------------------------------- */

#endif

int
main (int argc, char **argv)
{
#ifdef GOFFICE_WITH_DECIMAL64
	Corpus *corpus, *corpus2;
	const char *strict_env = g_getenv ("GO_DECIMAL_TEST_STRICT");

	strict_xfail = strict_env && strcmp (strict_env, "0") != 0;

	libgoffice_init ();

	corpus = basic_corpus ();

	test_encoding ();
	test_special_values ();
	test_rounding_exact ();
	test_rounding_table ();
	test_nextafter_exact ();
	test_fmod_exact ();
	test_scalbn_frexp ();
	test_range ();
	test_lgamma ();
	test_bessel ();
	test_log_accuracy ();
	test_trig_accuracy ();
	test_roots_hypot ();
	test_pow_exact ();
	test_strtoDd_extra ();
	test_printf_extra ();

	test_rounding (corpus);
	test_properties (corpus);
	test_copysign (corpus);
	test_nextafter ();
	test_oneargs (corpus);
	test_modf (corpus);
	test_scalbn ();

	corpus2 = corpus_concat
		(corpus, 0,
		 corpus_concat (power_corpus (50, 1.dd, 2), 1,
				power_corpus (16, 1.dd, 0.5dd), 1), 1);
	test_log ("log2", 2, corpus2);
	corpus_free (corpus2);

	test_log ("log", 3, corpus);

	corpus2 = corpus_concat
		(corpus, 0,
		 power_corpus (300, 1.dd, 10), 1);
	test_log ("log10", 10, corpus2);
	corpus_free (corpus2);

#ifdef HAVE_LONG_DOUBLE
	if (strtold ("1e-400", NULL) > 0) {
		// The offset here is partly for the benefit of going through
		// double for the reference string and partly to test something
		// else
		corpus2 = corpus_concat
			(corpus, 0,
			 corpus_concat (linear_corpus (10001, 1e-3dd, 1e-14dd), 1,
					linear_corpus (10001, 1e-3dd, -1e-14dd), 1), 1);
		test_dtoa (corpus2);
		corpus_free (corpus2);
	} else {
		start_section ("dummy");
		g_printerr ("Not running dtoa test due to buggy valgrind\n");
		end_section ();
	}
#endif

	test_atan2 (corpus, corpus);

	test_hypot (corpus, corpus);

	test_fmod (corpus, corpus);

	// Very preliminary
	test_pow (corpus, corpus);
	test_pow2 ();
	test_quad_exp_pow ();

	test_strto ();

	// Run last: forks its own children, and can legitimately crash or
	// abort a child process to demonstrate a real bug (see the section
	// header).  Keeping it last means every other test above has already
	// reported its own pass/fail before that happens.
	test_regressions ();

	g_printerr ("-----------------------------------------------------------------------------\n");

	if (n_xfail)
		g_printerr ("(%d known-bug failures were expected via xfail "
			    "and not counted; set GO_DECIMAL_TEST_STRICT=1 "
			    "to count them.)\n", n_xfail);

	if (n_bad)
		g_printerr ("FAIL: A total of %d failures.\n", n_bad);
	else
		g_printerr ("Pass.\n");

	corpus_free (corpus);

	libgoffice_shutdown ();

#else
	g_printerr ("Not compiled with Decimal64 support, so no testing.\n");
#endif

	return n_bad ? 1 : 0;
}
