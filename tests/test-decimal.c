#include <goffice/goffice.h>

static int n_bad;

#ifdef GOFFICE_WITH_DECIMAL64

// There does not seem to be a way to teach these warnings about the
// "W" modifier than we have hooked into libc's printf.
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"

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

static int
test_eq (_Decimal64 a, _Decimal64 b)
{
	if (decimal_eq (a, b)) {
		good ();
		return 1;
	} else {
		g_printerr ("%.16Wg vs %.16Wg\n", a, b);
		bad ();
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

/* ------------------------------------------------------------------------- */

static void
test_rounding (Corpus *corpus)
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
		int qoverflow = finiteD (x) && !finite (dx);
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

			g_printerr ("Error: 0x%08lx: %.16Wg -> (%.16Wg , %.16Wg , %.16Wg , %.16Wg)\n",
				    d64, x, f, r, c, t);
			bad ();
		}
	}

	end_section ();
}

static void
test_properties (Corpus *corpus)
{
	start_section ("properties (isnan, finite, signbit)");

	for (int v = 0; v < corpus->nvals; v++) {
		_Decimal64 x = corpus->vals[v];

		int qnan = isnanD (x);
		int qfinite = finiteD (x);
		int qsign = signbit (x);
		double dx = x;

		if (!!qnan == (x != x) &&
		    !!qfinite == (fabsD (x) <= DECIMAL64_MAX) &&
		    !!qsign == !!signbit (dx)) {
			good ();
		} else {
			uint64_t d64;
			memcpy (&d64, &x, sizeof (d64));

			g_printerr ("Error: 0x%08lx: %.16Wg -> %d %d %d\n",
				    d64, x, qnan, qfinite, qsign);
			bad ();
		}
	}

	end_section ();
}

static void
test_copysign (Corpus *corpus)
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
				g_printerr ("Failed for %.16Wg  %.16Wg\n", x1, x2);
				bad ();
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

	end_section ();
}

static void
test_modf (Corpus *corpus)
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
test_oneargs (Corpus *corpus)
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
		{ "log10D", log10D, log10 },
		{ "log1pD", log1pD, log1p },
		{ "log2D", log2D, log2 },
		{ "logD", logD, log },
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
			int qoverflow = finiteD (x) && !finite (dx);

			if (qunderflow || qoverflow)
				continue;

			y = funcs[f].fn_decimal (x);
			dy = funcs[f].fn_double (dx);

			//g_printerr ("%.16Wg  %.16Wg\n", x, y);
			//g_printerr ("%.16g  %.16g\n", dx, dy);

			ok = (!!finiteD (y) == !!finite (dy) &&
			      !!isnanD (y) == !!isnan (dy) &&
			      !!signbitD (y) == !!signbit (dy) &&
			      (y == 0) == (dy == 0));

			if (ok && finite (dy) && y != 0) {
				_Decimal64 d = y - (_Decimal64)dy;
				ok = fabsD (d / y) < 1e-10dd;
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
test_dtoa (Corpus *corpus)
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
			int qoverflow = finiteD (x) && !finite (dx);

			if (qunderflow || qoverflow)
				continue;

			if (fstyle && fabsD (x) > 0 && finiteD (x) &&
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

/* ------------------------------------------------------------------------- */

#endif

int
main (int argc, char **argv)
{
#ifdef GOFFICE_WITH_DECIMAL64
	Corpus *corpus, *corpus2;

	libgoffice_init ();

	corpus = basic_corpus ();

	test_rounding (corpus);
	test_properties (corpus);
	test_copysign (corpus);
	test_nextafter ();
	test_oneargs (corpus);
	test_modf (corpus);

	// The offset here is partly for the benefit of going through
	// double for the reference string and partly to test something
	// else
	corpus2 = corpus_concat
		(corpus, 0,
		 corpus_concat (linear_corpus (10001, 1e-3dd, 1e-14dd), 1,
				linear_corpus (10001, 1e-3dd, -1e-14dd), 1), 1);
	test_dtoa (corpus2);
	corpus_free (corpus2);

	if (n_bad)
		g_printerr ("A total of %d failures.\n", n_bad);
	else
		g_printerr ("Pass.\n");

	corpus_free (corpus);

	libgoffice_shutdown ();

#else
	g_printerr ("Not compiled with Decimal64 support, so no testing.\n");
#endif

	return n_bad ? 1 : 0;
}
