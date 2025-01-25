#include <goffice/goffice.h>

// This program answers the question of whether a constant, c, is more
// accurately represented as a double than its inverse, 1/c.
//
// The purpose is to help squeeze a few more bits out of calculations
// that involve multiplying or dividing by the constant.
//
// Consider the density function for the normal distribution:
//     d(x) = (1/sqrt(2*pi)) * exp(-x^2/2)
// As written, this multiplies by a constant, 1/sqrt(2*pi), whose value
// we could compute and state with enough decimals to guarantee that we
// end up using the best possible value of type double.  But we could
// also use division by the constant sqrt(2*pi).  Which is best?
//
// The traditional answer has been that division is expensive, so avoid
// it, but that isn't really true anymore.  Here we are only interested
// in accuracy.
//
// Assumptions:
// 1. double is base-2 floating-point with 53 mantissa bits (including
//    the implied leading bit).
// 2. we want to minimize the error on the product or quotient.
// 3. we know nothing special about the second factor that would
//    force one choice or the other.
//
// For sqrt(2pi) the answer is that it is ever so slightly better to use
// multiplication.  But it's a very close call.  But to get the right value,
// one cannot start with the best double approximating sqrt(2pi) and compute
// 1/sqrt(2pi) from that.

// ----------------------------------------
// Examining constant sqrt(2pi)
//
// Value bit pattern: 1.0100000011011001001100011111111101100010011100000101|10011 * 2^1
// Value as double: 2.50662827463100068570156508940272033214569091796875
// Relative error: -7.31205e-17
// Correct digits: 16.14
//
// Inverse bit pattern: 1.1001100010000100010100110011110101000011011001010000|10001 * 2^-2
// Inverse as double: 0.398942280401432702863218082711682654917240142822265625
// Relative error: -6.24734e-17
// Correct digits: 16.20
// Can inverse be computed as double: no
//
// Conclusion: for sqrt(2pi), use inverse
// ----------------------------------------

static GOQuad qln10;

static void
print_bits (GOQuad const *qc)
{
	GOQuad qx = *qc, qd;
	int e;
	double s;
	int b;
	int N = 53 + 12;

	g_return_if_fail (go_finite (qx.h) && qx.h != 0);

	if (qx.h < 0) {
		g_printerr ("-");
		qx.h = -qx.h;
		qx.l = -qx.l;
	}

	// Scale mantissa to [0.5 ; 1.0[
	(void)frexp (go_quad_value (&qx), &e);
	s = ldexp (1.0, -e);
	qx.h *= s;
	qx.l *= s;

	// Add to simulate rounding when we truncate below
	go_quad_init (&qd, ldexp (0.5, -N));
	go_quad_add (&qx, &qx, &qd);

	for (b = 0; b < N; b++) {
		GOQuad d;
		qx.h *= 2;
		qx.l *= 2;
		go_quad_sub (&d, &qx, &go_quad_one);
		if (go_quad_value (&d) >= 0) {
			qx = d;
			g_printerr ("1");
		} else {
			g_printerr ("0");
		}
		if (b == 0)
			g_printerr (".");
		if (b == 52)
			g_printerr ("|");
	}

	g_printerr (" * 2^%d", e - 1);
}

static void
print_decimal (GOQuad const *qc)
{
	// Note: this is highly limited implementation not fit for general
	// use.

	GOQuad qe, qd, qr;
	int d, e;
	int N = 24;

	g_return_if_fail (go_finite (qc->h) && qc->h > 0);

	go_quad_log (&qe, qc);
	go_quad_div (&qe, &qe, &qln10);
	e = (int)floor (go_quad_value (&qe));

	// Scale mantissa to [1 ; 10[
	go_quad_init (&qe, -e);
	go_quad_init (&qd, 10);
	go_quad_pow (&qe, NULL, &qd, &qe);
	go_quad_mul (&qd, qc, &qe);

	// Add to simulate rounding when we truncate below
	go_quad_init (&qe, 1 - N);
	go_quad_init (&qr, 10);
	go_quad_pow (&qr, NULL, &qr, &qe);
	go_quad_init (&qe, 0.5);
	go_quad_mul (&qr, &qr, &qe);
	go_quad_add (&qd, &qd, &qr);

	for (d = 0; d < N; d++) {
		GOQuad qw, qd1;
		go_quad_floor (&qw, &qd);
		int w = go_quad_value (&qw);
		go_quad_sub (&qd1, &qd, &qw);

		g_printerr ("%c", '0' + w);
		go_quad_sub (&qd, &qd, &qw);

		go_quad_init (&qw, 10);
		go_quad_mul (&qd, &qd, &qw);

		if (d == 0)
			g_printerr (".");
	}

	if (e)
		g_printerr (" * 10^%d", e);
}



static double
qmlogabs (GOQuad const *qx)
{
	GOQuad qy;
	qy.h = fabs (qx->h); qy.l = fabs (qx->l);
	go_quad_log (&qy, &qy);
	go_quad_div (&qy, &qy, &qln10);
	return -go_quad_value (&qy);
}

static double last_direct;
static double last_inverse;
static gboolean last_need_nl;

static void
examine_constant (const char *descr, GOQuad const *qc)
{
	double dc, dic, dcic;
	GOQuad qd, qic;
	double dig_direct, dig_inverse;

	g_printerr ("-----------------------------------------------------------------------------\n");
	g_printerr ("Examining constant %s\n", descr);
	g_printerr ("\n");

	last_direct = dc = go_quad_value (qc);
	g_printerr ("Value: ");
	print_decimal (qc);
	g_printerr ("\n");
	g_printerr ("Value bit pattern: ");
	print_bits (qc);
	g_printerr ("\n");
	g_printerr ("Value as double: %.55g\n", dc);
	go_quad_init (&qd, dc);
	go_quad_sub (&qd, qc, &qd);
	go_quad_div (&qd, &qd, qc);
	g_printerr ("Relative error: %g\n", go_quad_value (&qd));
	dig_direct = qmlogabs (&qd);
	g_printerr ("Correct digits: %.2f\n", dig_direct);

	g_printerr ("\n");

	// For the inverse we assume that using double-double is good enough to
	// answer our question.
	go_quad_div (&qic, &go_quad_one, qc);
	last_inverse = dic = go_quad_value (&qic);
	g_printerr ("Inverse value: ");
	print_decimal (&qic);
	g_printerr ("\n");
	g_printerr ("Inverse bit pattern: ");
	print_bits (&qic);
	g_printerr ("\n");
	g_printerr ("Inverse as double: %.55g\n", dic);
	go_quad_init (&qd, dic);
	go_quad_sub (&qd, &qic, &qd);
	go_quad_div (&qd, &qd, &qic);
	g_printerr ("Relative error: %g\n", go_quad_value (&qd));
	dig_inverse = qmlogabs (&qd);
	g_printerr ("Correct digits: %.2f\n", dig_inverse);

	dcic = (double)(1 / dc);
	g_printerr ("Can inverse be computed as double: %s\n",
		    (dcic == dic ? "yes" : "no"));

	g_printerr ("\n");

	g_printerr ("Conclusion: for %s, use %s\n", descr,
		    (dig_direct >= dig_inverse ? "direct" : "inverse"));
	last_need_nl = TRUE;
}

static void
check_computable (const char *fmla, double x, gboolean direct)
{
	gboolean ok = (x == (direct ? last_direct : last_inverse));

	g_printerr ("%sComputing %s as double yields the right value?  %s\n",
		    (last_need_nl ? "\n" : ""), fmla, (ok ? "yes" : "no"));
	last_need_nl = FALSE;
}

int
main (int argc, char **argv)
{
	void *state;
	GOQuad qc, qten;

	g_printerr ("Determine for certain constants, c, whether its representation\n");
	g_printerr ("as a double is more accurate than its inverse's representation.\n\n");

	state = go_quad_start ();

	go_quad_init (&qln10, 10);
	go_quad_log (&qln10, &qln10);

	examine_constant ("pi", &go_quad_pi);
	examine_constant ("e", &go_quad_e);
	examine_constant ("EulerGamma", &go_quad_euler);

	examine_constant ("log(2)", &go_quad_ln2);
	examine_constant ("log(10)", &qln10);
	go_quad_div (&qc, &go_quad_ln2, &qln10);
	examine_constant ("log10(2)", &qc);

	examine_constant ("sqrt(2)", &go_quad_sqrt2);
	go_quad_init (&qc, 3);
	go_quad_sqrt (&qc, &qc);
	examine_constant ("sqrt(3)", &qc);
	go_quad_init (&qc, 5);
	go_quad_sqrt (&qc, &qc);
	examine_constant ("sqrt(5)", &qc);

	go_quad_sqrt (&qc, &go_quad_pi);
	examine_constant ("sqrt(pi)", &qc);
	check_computable ("sqrt(pi)", sqrt (M_PI), TRUE);
	check_computable ("1/sqrt(pi)", 1 / sqrt (M_PI), FALSE);
	check_computable ("sqrt(1/pi)", sqrt (1 / M_PI), FALSE);

	go_quad_sqrt (&qc, &go_quad_2pi);
	examine_constant ("sqrt(2pi)", &qc);
	check_computable ("sqrt(2pi)", sqrt (2 * M_PI), TRUE);
	check_computable ("sqrt(2)*sqrt(pi)", sqrt (2) * sqrt (M_PI), TRUE);
	check_computable ("1/sqrt(2pi)", 1 / sqrt (2 * M_PI), FALSE);

	go_quad_init (&qc, 180);
	go_quad_div (&qc, &qc, &go_quad_pi);
	examine_constant ("180/pi", &qc);
	check_computable ("180/pi", 180 / M_PI, TRUE);
	check_computable ("pi/180", M_PI / 180, FALSE);

	go_quad_init (&qten, 10);
	for (int i = 23; i <= 308; i++) {
		char *txt = g_strdup_printf ("10^%d", i);
		go_quad_init (&qc, i);
		go_quad_pow (&qc, NULL, &qten, &qc);
		examine_constant (txt, &qc);
		g_free (txt);
	}

	go_quad_end (state);

	return 0;
}
