#include <string.h>
#include <goffice/goffice.h>

// A rather expensive way of forcing the compiler to drop any excess
// precision it might have for x.
static double
kill_excess_precision (double x)
{
	GString *s = g_string_new (NULL);
	double y;
	g_string_append_len (s, (char *)&x, sizeof (x));
	memcpy (&y, s->str, sizeof (y));
	g_string_free (s, TRUE);
	return y;
}

#define UNTEST1(a_,QOP,OP,txt)						\
do {									\
	double a, p, v;							\
	GOQuad qa, qc;							\
	void *state;							\
	a = (a_);							\
	state = go_quad_start ();					\
	go_quad_init (&qa, a);						\
	QOP (&qc, &qa);							\
	go_quad_end (state);						\
	p = kill_excess_precision (OP (a));				\
	v = kill_excess_precision (go_quad_value (&qc));		\
	g_printerr ("%s(%g) = %g  [%g]\n", txt, a, v, p);		\
	if (p == floor (p))						\
		g_assert (v == p);					\
	else								\
		g_assert (fabs (v - p) / p < 1e-14);			\
} while (0)

#define BINTEST1(a_,b_,QOP,OP,txt)					\
do {									\
	double a, b, p, v;						\
	GOQuad qa, qb, qc;						\
	void *state;							\
	a = (a_);							\
	b = (b_);							\
	state = go_quad_start ();					\
	go_quad_init (&qa, a);						\
	go_quad_init (&qb, b);						\
	QOP (&qc, &qa, &qb);						\
	go_quad_end (state);						\
	p = kill_excess_precision (OP (a, b));				\
	v = kill_excess_precision (go_quad_value (&qc));		\
	g_printerr ("%s(%g,%g) = %g  [%g]\n", txt, a, b, v, p);		\
	if (p == floor (p))						\
		g_assert (v == p);					\
	else								\
		g_assert (fabs (v - p) / p < 1e-14);			\
} while (0)

/* ------------------------------------------------------------------------- */

static void
pow_tests (void)
{
#define QUAD_POW(r_,a_,b_) go_quad_pow ((r_),NULL,(a_),(b_))
#define TEST1(a_,b_) BINTEST1(a_,b_,QUAD_POW,pow,"pow")
	TEST1 (+2.3, +1.2);
	TEST1 (+2.3, -1.2);
	TEST1 (+2.3, +0.2);
	TEST1 (+2.3, -0.2);
	TEST1 (+0.2, +2.3);
	TEST1 (+0.2, -2.3);
	TEST1 (+2.3, +3);
	TEST1 (+2.3, -3);
	TEST1 (-2.3, +3);
	TEST1 (-2.3, -3);
	TEST1 (+2.3, +4);
	TEST1 (+2.3, -4);
	TEST1 (-2.3, +4);
	TEST1 (-2.3, -4);
	TEST1 (2, 100);
	TEST1 (2, -100);
	TEST1 (1.0/256, 1.0/1024);
	TEST1 (exp(1), 1);
	TEST1 (exp(1), -1);
	TEST1 (exp(1), log(2));
	TEST1 (exp(1), -log(2));

#if 0
	{
		GOQuad a, p;
		double x = 1e6;
		double e, e0 = 1442695;
		double m, m0 = 1.028747527600573533664645566932422897;

		/* Test that exp(1e6) ~ m0 * 2^e0. */

		go_quad_init (&a, 1e6);
		go_quad_exp (&p, &e, &a);
		m = ldexp (go_quad_value (&p), e - e0);
		g_printerr ("exp(%g) = %.16g*2^%.0f  [%.16g*2^%.0f]\n",
			    x, go_quad_value (&p), e, m0, e0);
		g_assert (e > e0 - 5 && e < e0 + 5);
		g_assert (fabs (m - m0) / m0 < 1e-14);
	}
#endif
}
#undef TEST1

/* ------------------------------------------------------------------------- */

#define ATAN2PI(y_,x_) (atan2((y_),(x_))/M_PI)

static void
atan2_tests (void)
{
#define TEST1(a_,b_) do {				\
    BINTEST1(a_,b_,go_quad_atan2,atan2,"atan2");	\
    BINTEST1(a_,b_,go_quad_atan2pi,ATAN2PI,"atan2pi");	\
} while (0)
	TEST1 (0, +2);
	TEST1 (0, -2);
	TEST1 (3, 0);
	TEST1 (-3, 0);
	TEST1 (0, 0);
	TEST1 (+1, +1);
	TEST1 (-1, +1);
	TEST1 (+1, -1);
	TEST1 (-1, -1);
	TEST1 (+2.3, +1.2);
	TEST1 (+2.3, -1.2);
	TEST1 (+2.3, +0.2);
	TEST1 (+2.3, -0.2);
	TEST1 (+0.2, +2.3);
	TEST1 (+0.2, -2.3);
	TEST1 (+2.3, +3);
	TEST1 (+2.3, -3);
	TEST1 (-2.3, +3);
	TEST1 (-2.3, -3);
	TEST1 (+2.3, +4);
	TEST1 (+2.3, -4);
	TEST1 (-2.3, +4);
	TEST1 (-2.3, -4);
	TEST1 (2, 100);
	TEST1 (2, -100);
	TEST1 (1.0/256, 1.0/1024);
	TEST1 (exp(1), 1);
	TEST1 (exp(1), -1);
	TEST1 (exp(1), log(2));
	TEST1 (exp(1), -log(2));
}
#undef TEST1

/* ------------------------------------------------------------------------- */

static void
inverse_special_tests (void)
{
	GOQuad y, x, r;
	void *state = go_quad_start ();

	/* NaN and out-of-domain arguments must produce NaN, not leave
	 * the result untouched.  */
#define NAN1(f_, a_) do {						\
		go_quad_init (&y, (a_));				\
		r.h = r.l = 12345;					\
		f_ (&r, &y);						\
		g_printerr ("%s(%g) = %g  [nan]\n", #f_,		\
			    go_quad_value (&y), go_quad_value (&r));	\
		g_assert (isnan (go_quad_value (&r)));			\
	} while (0)
	NAN1 (go_quad_asin, go_nan);
	NAN1 (go_quad_acos, go_nan);
	NAN1 (go_quad_asin, 2);
	NAN1 (go_quad_acos, 2);
	NAN1 (go_quad_asin, -2);
	NAN1 (go_quad_acos, -2);

#define NAN2(y_, x_) do {						\
		go_quad_init (&y, (y_));				\
		go_quad_init (&x, (x_));				\
		r.h = r.l = 12345;					\
		go_quad_atan2 (&r, &y, &x);				\
		g_printerr ("atan2(%g, %g) = %g  [nan]\n",		\
			    (double)(y_), (double)(x_), go_quad_value (&r)); \
		g_assert (isnan (go_quad_value (&r)));			\
	} while (0)
	NAN2 (go_nan, 1);
	NAN2 (1, go_nan);
	NAN2 (0, go_nan);
	NAN2 (go_nan, go_nan);

	/* Infinite arguments; want_ is in units of pi/4.  */
#define PI4(y_, x_, want_) do {						\
		go_quad_init (&y, (y_));				\
		go_quad_init (&x, (x_));				\
		r.h = r.l = 12345;					\
		go_quad_atan2 (&r, &y, &x);				\
		g_printerr ("atan2(%g, %g) = %g  [%g]\n",		\
			    (double)(y_), (double)(x_), go_quad_value (&r), \
			    (double)(want_) * M_PI / 4);		\
		g_assert (fabs (go_quad_value (&r) -			\
				(double)(want_) * M_PI / 4) < 1e-12);	\
	} while (0)
	PI4 (1, go_pinf, 0);
	PI4 (-1, go_pinf, 0);
	PI4 (1, go_ninf, 4);
	PI4 (-1, go_ninf, -4);
	PI4 (go_pinf, 1, 2);
	PI4 (go_ninf, 1, -2);
	PI4 (go_pinf, go_pinf, 1);
	PI4 (go_ninf, go_pinf, -1);
	PI4 (go_pinf, go_ninf, 3);
	PI4 (go_ninf, go_ninf, -3);

	go_quad_end (state);
#undef NAN1
#undef NAN2
#undef PI4
}

/* ------------------------------------------------------------------------- */

static void
hypot_tests (void)
{
#define TEST1(a_,b_) BINTEST1(a_,b_,go_quad_hypot,hypot,"hypot");
	TEST1 (0, +2);
	TEST1 (0, -2);
	TEST1 (3, 0);
	TEST1 (-3, 0);
	TEST1 (0, 0);
	TEST1 (+1, +1);
	TEST1 (+2.3, +1.2);
	TEST1 (+2.3, -1.2);
	TEST1 (+2.3, +0.2);
	TEST1 (+2.3, -0.2);
	TEST1 (+0.2, +2.3);
	TEST1 (+0.2, -2.3);
	TEST1 (+2.3, +3);
	TEST1 (+2.3, -3);
	TEST1 (-2.3, +3);
	TEST1 (-2.3, -3);
	TEST1 (+2.3, +4);
	TEST1 (+2.3, -4);
	TEST1 (-2.3, +4);
	TEST1 (-2.3, -4);
	TEST1 (2, 100);
	TEST1 (2, -100);
	TEST1 (1.0/256, 1.0/1024);
	TEST1 (exp(1), 1);
	TEST1 (exp(1), -1);
	TEST1 (exp(1), log(2));
	TEST1 (exp(1), -log(2));
}
#undef TEST1

/* ------------------------------------------------------------------------- */

#define TEST1(a_,b_)				\
do {						\
	BINTEST1(a_,b_,go_quad_add,ADD,"add");	\
	BINTEST1(a_,b_,go_quad_sub,SUB,"sub");	\
	BINTEST1(a_,b_,go_quad_mul,MUL,"mul");	\
	BINTEST1(a_,b_,go_quad_div,DIV,"div");	\
} while (0)
#define ADD(a_,b_) ((a_)+(b_))
#define SUB(a_,b_) ((a_)-(b_))
#define MUL(a_,b_) ((a_)*(b_))
#define DIV(a_,b_) ((a_)/(b_))

static void
basic4_tests (void)
{
	TEST1 (1, 2);
	TEST1 (1, -2);
	TEST1 (1.0/3, 12345.0);
	TEST1 (-1e10, 0.1);
	TEST1 (0.1, 1e100);
}
#undef TEST1
#undef ADD
#undef SUB
#undef MUL
#undef DIV

/* ------------------------------------------------------------------------- */

static void
log_tests (void)
{
	static const double ds[] = {
		0.5, 1, 2, M_E, 10, 1e-300, 1e300
	};
	void *state = go_quad_start ();
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (ds); i++) {
		double d = ds[i];
		double want = log (d);
		double v;
		GOQuad a, r;

		go_quad_init (&a, d);
		r.h = r.l = 12345;
		go_quad_log (&r, &a);
		v = kill_excess_precision (go_quad_value (&r));
		g_printerr ("qlog(%g) = %.17g  [%.17g]\n", d, v, want);
		if (want == 0)
			g_assert (v == 0);
		else if (v != want)
			g_assert (fabs (v - want) / fabs (want) < 1e-15);
	}

	/* The ends of the domain are well defined.  */
	{
		GOQuad a, r;

		go_quad_init (&a, 0);
		r.h = r.l = 12345;
		go_quad_log (&r, &a);
		g_printerr ("qlog(0) = %g  [-inf]\n", go_quad_value (&r));
		g_assert (r.h == go_ninf && r.l == 0);

		go_quad_init (&a, -1);
		r.h = r.l = 12345;
		go_quad_log (&r, &a);
		g_assert (isnan (r.h));

		go_quad_init (&a, go_pinf);
		r.h = r.l = 12345;
		go_quad_log (&r, &a);
		g_assert (r.h == go_pinf && r.l == 0);
	}

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

static void
abs_negate_scalbn_tests (void)
{
	GOQuad a, r;
	void *state = go_quad_start ();

	go_quad_init (&a, -2.5);
	a.l = ldexp (1, -60);
	go_quad_abs (&r, &a);
	g_printerr ("qabs(-2.5+2^-60) = (%g, %g)\n", r.h, r.l);
	g_assert (r.h == 2.5 && r.l == -ldexp (1, -60));

	go_quad_init (&a, 2.5);
	a.l = ldexp (1, -60);
	go_quad_abs (&r, &a);
	g_assert (r.h == 2.5 && r.l == ldexp (1, -60));

	go_quad_negate (&r, &a);
	g_assert (r.h == -2.5 && r.l == -ldexp (1, -60));

	/* go_quad_scalbn shifts both limbs exactly.  */
	go_quad_init (&a, 1);
	a.l = ldexp (1, -80);

	r.h = r.l = 12345;
	go_quad_scalbn (&r, &a, 3);
	g_assert (r.h == 8 && r.l == ldexp (1, -77));

	r.h = r.l = 12345;
	go_quad_scalbn (&r, &a, -80);
	g_assert (r.h == ldexp (1, -80) && r.l == ldexp (1, -160));

	/* Extreme exponents saturate instead of half-updating the value.  */
	r.h = r.l = 12345;
	go_quad_scalbn (&r, &a, 4000);
	g_printerr ("qscalbn(1+2^-80, 4000) = (%g, %g)\n", r.h, r.l);
	g_assert (isinf (r.h) && r.h > 0 && isinf (r.l) && r.l > 0);

	r.h = r.l = 12345;
	go_quad_scalbn (&r, &a, -4000);
	g_assert (r.h == 0 && r.l == 0);

	/* Non-finite inputs keep the IEEE behaviour of the high part.  */
	go_quad_init (&a, go_pinf);
	r.h = r.l = 12345;
	go_quad_scalbn (&r, &a, 3);
	g_assert (r.h == go_pinf && r.l == 0);

	go_quad_init (&a, go_ninf);
	r.h = r.l = 12345;
	go_quad_scalbn (&r, &a, 3);
	g_assert (r.h == go_ninf && r.l == 0);

	go_quad_init (&a, go_nan);
	r.h = r.l = 12345;
	go_quad_scalbn (&r, &a, 3);
	g_assert (isnan (r.h) && r.l == 0);

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

static void
mul12_tests (void)
{
	static const double ps[][2] = {
		{ 3, 4 },
		{ 0.1, 10 },
		{ 1 + ldexp (1, -27), 1 - ldexp (1, -27) },
		{ 1e200, 1e-200 },
		{ -2.5, 1.25 },
		{ 0, 5 }
	};
	void *state = go_quad_start ();
	unsigned i;

	/* go_quad_mul12 agrees with a full quad multiplication.  */
	for (i = 0; i < G_N_ELEMENTS (ps); i++) {
		GOQuad x, y, m, m12;

		go_quad_init (&x, ps[i][0]);
		go_quad_init (&y, ps[i][1]);
		go_quad_mul (&m, &x, &y);
		go_quad_mul12 (&m12, ps[i][0], ps[i][1]);
		g_assert (m.h == m12.h && m.l == m12.l);
	}

	/* The low limb holds bits the double product cannot represent.  */
	{
		GOQuad r;
		r.h = r.l = 12345;
		go_quad_mul12 (&r, 1 + ldexp (1, -27), 1 - ldexp (1, -27));
		g_printerr ("qmul12(1+2^-27, 1-2^-27) = (%g, %g)\n", r.h, r.l);
		g_assert (r.h == 1 && r.l == -ldexp (1, -54));
	}

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

static void
dot_product_tests (void)
{
	GOQuad a[3], b[3], r;
	void *state = go_quad_start ();

	go_quad_init (&a[0], 1);
	go_quad_init (&a[1], 2);
	go_quad_init (&a[2], 3);
	go_quad_init (&b[0], 4);
	go_quad_init (&b[1], 5);
	go_quad_init (&b[2], 6);
	r.h = r.l = 12345;
	go_quad_dot_product (&r, a, b, 3);
	g_printerr ("dot([1,2,3],[4,5,6]) = %g  [32]\n", go_quad_value (&r));
	g_assert (r.h == 32 && r.l == 0);

	/* Empty input is the empty sum.  */
	r.h = r.l = 12345;
	go_quad_dot_product (&r, a, b, 0);
	g_assert (r.h == 0 && r.l == 0);

	/* Quad accumulation keeps the 1 in 1e100 + 1 - 1e100.  */
	go_quad_init (&a[0], 1e100);
	go_quad_init (&a[1], 1);
	go_quad_init (&a[2], -1e100);
	go_quad_init (&b[0], 1);
	go_quad_init (&b[1], 1);
	go_quad_init (&b[2], 1);
	r.h = r.l = 12345;
	go_quad_dot_product (&r, a, b, 3);
	g_assert (r.h == 1 && r.l == 0);

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

static void
constant8_tests (void)
{
	static const guint8 d10[] = { 1, 2, 3 };
	static const guint8 d16[] = { 0xa, 0xb, 0xc };
	GOQuad r;
	void *state = go_quad_start ();

	/* Digits are interpreted as a base-n fraction.  */
	r.h = r.l = 12345;
	go_quad_constant8 (&r, d10, 3, 10, 1);
	g_printerr ("constant8(123, 10, 1) = %.17g\n", r.h + r.l);
	g_assert (fabs ((r.h + r.l) - 0.123) < 1e-16);

	r.h = r.l = 12345;
	go_quad_constant8 (&r, d10, 3, 10, 0.5);
	g_assert (fabs ((r.h + r.l) - 0.0615) < 1e-16);

	/* 0xabc / 16^3 is exact in binary.  */
	r.h = r.l = 12345;
	go_quad_constant8 (&r, d16, 3, 16, 1);
	g_assert (r.h == 0.6708984375 && r.l == 0);

	r.h = r.l = 12345;
	go_quad_constant8 (&r, d10, 0, 10, 1);
	g_assert (r.h == 0 && r.l == 0);

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

#define TEST1(a_) UNTEST1(a_,go_quad_floor,floor,"floor")

static void
floor_tests (void)
{
	GOQuad a, b, r;
	void *state;

	TEST1 (0);
	TEST1 (1);
	TEST1 (-1);
	TEST1 (1.0/3);
	TEST1 (-1.0/3);

	state = go_quad_start ();
	go_quad_floor (&a, &go_quad_sqrt2);
	go_quad_end (state);
	g_printerr ("floor(sqrt(2))=%g\n", go_quad_value (&a));
	g_assert (go_quad_value (&a) == 1);

	state = go_quad_start ();
	go_quad_init (&a, 11);
	go_quad_init (&b, ldexp (1, -80));
	go_quad_sub (&a, &a, &b);
	go_quad_floor (&r, &a);
	go_quad_end (state);
	g_printerr ("floor(11-2^-80)=%g\n", go_quad_value (&r));
	g_assert (go_quad_value (&r) == 10);

	state = go_quad_start ();
	go_quad_init (&a, 11);
	go_quad_init (&b, ldexp (1, -80));
	go_quad_add (&a, &a, &b);
	go_quad_floor (&r, &a);
	go_quad_end (state);
	g_printerr ("floor(11+2^-80)=%g\n", go_quad_value (&r));
	g_assert (go_quad_value (&r) == 11);

	state = go_quad_start ();
	go_quad_init (&a, -11);
	go_quad_init (&b, ldexp (1, -80));
	go_quad_sub (&a, &a, &b);
	go_quad_floor (&r, &a);
	go_quad_end (state);
	g_printerr ("floor(-11-2^-80)=%g\n", go_quad_value (&r));
	g_assert (go_quad_value (&r) == -12);

	state = go_quad_start ();
	go_quad_init (&a, -11);
	go_quad_init (&b, ldexp (1, -80));
	go_quad_add (&a, &a, &b);
	go_quad_floor (&r, &a);
	go_quad_end (state);
	g_printerr ("floor(-11+2^-80)=%g\n", go_quad_value (&r));
	g_assert (go_quad_value (&r) == -11);

	state = go_quad_start ();
	go_quad_init (&a, ldexp (11, 80));
	go_quad_init (&b, 1);
	go_quad_sub (&a, &a, &b);
	go_quad_floor (&r, &a);
	go_quad_sub (&b, &a, &r);
	go_quad_end (state);
	g_printerr ("floor(11*2^-80-1)=%g\n", go_quad_value (&r));
	g_assert (go_quad_value (&b) == 0);

	state = go_quad_start ();
	go_quad_init (&a, ldexp (11, 80));
	go_quad_init (&b, 0.5);
	go_quad_sub (&a, &a, &b);
	go_quad_floor (&r, &a);
	go_quad_sub (&b, &a, &r);
	go_quad_end (state);
	g_printerr ("floor(11*2^-80-1)=%g\n", go_quad_value (&r));
	g_assert (go_quad_value (&b) == 0.5);

	state = go_quad_start ();
	go_quad_init (&a, ldexp (11, 80));
	go_quad_init (&b, 0.5);
	go_quad_add (&a, &a, &b);
	go_quad_floor (&r, &a);
	go_quad_sub (&b, &a, &r);
	go_quad_end (state);
	g_printerr ("floor(11*2^-80-1)=%g\n", go_quad_value (&r));
	g_assert (go_quad_value (&b) == 0.5);

	state = go_quad_start ();
	go_quad_init (&a, ldexp (11, 80));
	go_quad_init (&b, -0.5);
	go_quad_sub (&a, &a, &b);
	go_quad_floor (&r, &a);
	go_quad_sub (&b, &a, &r);
	go_quad_end (state);
	g_printerr ("floor(11*2^-80-1)=%g\n", go_quad_value (&r));
	g_assert (go_quad_value (&b) == 0.5);

	state = go_quad_start ();
	go_quad_init (&a, ldexp (11, 80));
	go_quad_init (&b, -0.5);
	go_quad_add (&a, &a, &b);
	go_quad_floor (&r, &a);
	go_quad_sub (&b, &a, &r);
	go_quad_end (state);
	g_printerr ("floor(11*2^-80-1)=%g\n", go_quad_value (&r));
	g_assert (go_quad_value (&b) == 0.5);
}

#undef TEST1

/* ------------------------------------------------------------------------- */

#define TEST1(a_) do {				\
	UNTEST1(a_,go_quad_asin,asin,"asin");	\
	UNTEST1(a_,go_quad_acos,acos,"acos");	\
} while (0)

#define TEST2(a_) do {				\
	UNTEST1(a_,go_quad_sin,sin,"sin");	\
	UNTEST1(a_,go_quad_cos,cos,"cos");	\
	UNTEST1(a_,go_quad_sinpi,go_sinpi,"sinpi");	\
	UNTEST1(a_,go_quad_cospi,go_cospi,"cospi");	\
} while (0)

static void
trig_tests (void)
{
	double d;

	TEST1 (0);
	TEST1 (0.25);
	TEST1 (0.5);
	TEST1 (0.75);
	TEST1 (1);
	TEST1 (-0.25);
	TEST1 (-0.5);
	TEST1 (-0.75);
	TEST1 (-1);

	for (d = 0; d < 10; d += 0.125) {
		TEST2(d);
		TEST2(-d);
	}
}

#undef TEST1

/* ------------------------------------------------------------------------- */

static void
const_tests (void)
{
	g_assert (fabs (go_quad_value (&go_quad_pi) - M_PI) < 1e-14);
	g_assert (fabs (go_quad_value (&go_quad_2pi) - 2 * M_PI) < 1e-14);
	g_assert (fabs (go_quad_value (&go_quad_pihalf) - M_PI / 2) < 1e-14);
	g_assert (fabs (go_quad_value (&go_quad_e) - exp(1)) < 1e-14);
	g_assert (fabs (go_quad_value (&go_quad_ln2) - log(2)) < 1e-14);
	g_assert (fabs (go_quad_value (&go_quad_ln10) - log(10)) < 1e-13);
	g_assert (fabs (go_quad_value (&go_quad_sqrt2) - sqrt(2)) < 1e-14);
	g_assert (fabs (go_quad_value (&go_quad_euler) - 0.57721566490153286060) < 1e-14);
	g_assert (go_quad_value (&go_quad_zero) == 0);
	g_assert (go_quad_value (&go_quad_one) == 1);
	g_assert (go_quad_value (&go_quad_half) == 0.5);
}

static void
init_tests (void)
{
	GOQuad a, b, c;
	void *state = go_quad_start ();

	go_quad_init (&a, 42.125);
	g_assert (go_quad_value (&a) == 42.125);

	go_quad_init (&b, ldexp (1.0, -80));
	g_assert (go_quad_value (&b) == ldexp (1.0, -80));

	go_quad_add (&c, &a, &b);
	g_assert (go_quad_value (&c) >= 42.125);
	g_assert (go_quad_value (&c) < 42.125 + ldexp (1.0, -40));

	go_quad_sub (&c, &c, &a);
	g_assert (go_quad_value (&c) == ldexp (1.0, -80));

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

static void
expm1_tests (void)
{
	static const double ds[] = {
		0.1, -0.1, 0.25, -0.25, 0.5, -0.5, 0.75, -0.75,
		2, -2, 20, -20, 1e-10, -1e-10
	};
	void *state = go_quad_start ();
	unsigned i;

	for (i = 0; i < G_N_ELEMENTS (ds); i++) {
		double d = ds[i];
		double want = expm1 (d);
		GOQuad a, r;
		double v;

		go_quad_init (&a, d);
		go_quad_expm1 (&r, &a);
		v = kill_excess_precision (go_quad_value (&r));
		g_printerr ("expm1(%g) = %.17g  [%.17g]\n", d, v, want);
		g_assert (copysign (1, v) == copysign (1, want));
		g_assert (fabs (v - want) / fabs (want) < 1e-14);
	}

	/* expm1(-inf) is -1 and expm1(+inf) is +inf.  */
	{
		GOQuad a, r;

		go_quad_init (&a, go_ninf);
		go_quad_expm1 (&r, &a);
		g_printerr ("expm1(-inf) = %g  [-1]\n", go_quad_value (&r));
		g_assert (go_quad_value (&r) == -1);

		go_quad_init (&a, go_pinf);
		go_quad_expm1 (&r, &a);
		g_printerr ("expm1(+inf) = %g  [inf]\n", go_quad_value (&r));
		g_assert (go_quad_value (&r) == go_pinf);
	}

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

static void
check_binop (const char *name,
	     void (*op) (GOQuad *, const GOQuad *, const GOQuad *),
	     double da, double db, double want_h, double want_l)
{
	GOQuad a, b, r;

	go_quad_init (&a, da);
	go_quad_init (&b, db);
	r.h = r.l = 12345;
	op (&r, &a, &b);
	g_printerr ("%s(%g, %g) = (%g, %g)  [(%g, %g)]\n",
		    name, da, db, r.h, r.l, want_h, want_l);
	g_assert (r.h == want_h || (isnan (r.h) && isnan (want_h)));
	g_assert (r.l == want_l || (isnan (r.l) && isnan (want_l)));
}

static void
nonfinite_tests (void)
{
	void *state = go_quad_start ();
	GOQuad a, r;

	check_binop ("add", go_quad_add, 1e308, 1e308, go_pinf, 0);
	check_binop ("add", go_quad_add, go_pinf, 5, go_pinf, 0);
	check_binop ("add", go_quad_add, go_ninf, 5, go_ninf, 0);
	check_binop ("add", go_quad_add, 0, go_nan, go_nan, 0);
	check_binop ("sub", go_quad_sub, go_pinf, go_pinf, go_nan, 0);
	check_binop ("sub", go_quad_sub, -1e308, 1e308, go_ninf, 0);
	check_binop ("sub", go_quad_sub, 5, go_pinf, go_ninf, 0);
	check_binop ("mul", go_quad_mul, go_pinf, 5, go_pinf, 0);
	check_binop ("mul", go_quad_mul, 1e308, 1e308, go_pinf, 0);
	check_binop ("mul", go_quad_mul, go_pinf, 0, go_nan, 0);
	check_binop ("div", go_quad_div, 1, 0, go_pinf, 0);
	check_binop ("div", go_quad_div, -1, 0, go_ninf, 0);
	check_binop ("div", go_quad_div, 0, 0, go_nan, 0);
	check_binop ("div", go_quad_div, 5, go_pinf, 0, 0);
	check_binop ("div", go_quad_div, go_pinf, 5, go_pinf, 0);
	check_binop ("div", go_quad_div, go_pinf, go_pinf, go_nan, 0);
	check_binop ("div", go_quad_div, 1, 5e-324, go_pinf, 0);

	/* mul12 with overflow and infinities.  */
	{
		GOQuad u;
		u.h = u.l = 12345;
		go_quad_mul12 (&u, 1e308, 1e308);
		g_assert (u.h == go_pinf && u.l == 0);
		u.h = u.l = 12345;
		go_quad_mul12 (&u, go_pinf, 5);
		g_assert (u.h == go_pinf && u.l == 0);
	}

	/* sqrt: NaN, negative, +infinity, and {0,l}.  */
	go_quad_init (&a, -4);
	r.h = r.l = 12345;
	go_quad_sqrt (&r, &a);
	g_printerr ("sqrt(-4) = (%g, %g)  [nan]\n", r.h, r.l);
	g_assert (isnan (r.h));
	go_quad_init (&a, go_nan);
	r.h = r.l = 12345;
	go_quad_sqrt (&r, &a);
	g_assert (isnan (r.h));
	go_quad_init (&a, go_pinf);
	r.h = r.l = 12345;
	go_quad_sqrt (&r, &a);
	g_assert (r.h == go_pinf && r.l == 0);
	a.h = 0;
	a.l = 1e-300;
	r.h = r.l = 12345;
	go_quad_sqrt (&r, &a);
	g_printerr ("sqrt({0,1e-300}) = (%g, %g)  [1e-150]\n", r.h, r.l);
	g_assert (fabs (r.h - 1e-150) < 1e-164);

	/* Negative exponents keep subnormal results.  */
	go_quad_init (&a, -710);
	r.h = r.l = 12345;
	go_quad_exp (&r, NULL, &a);
	g_printerr ("exp(-710) = %g  [%g]\n", go_quad_value (&r), exp (-710));
	g_assert (fabs (go_quad_value (&r) - exp (-710)) < 1e-12 * exp (-710));
	{
		GOQuad b, q;
		go_quad_init (&a, 2);
		go_quad_init (&b, -3000);
		q.h = q.l = 12345;
		go_quad_pow (&q, NULL, &a, &b);
		g_printerr ("pow(2,-3000) = %g  [0]\n", go_quad_value (&q));
		g_assert (go_quad_value (&q) == 0);
	}

	/* The finite path is unchanged.  */
	{
		GOQuad b, q;
		go_quad_init (&a, 1);
		go_quad_init (&b, 1e-308);
		q.h = q.l = 12345;
		go_quad_div (&q, &a, &b);
		g_printerr ("div(1,1e-308) = (%g, %g)\n", q.h, q.l);
		g_assert (q.h == 1e308);
		g_assert (isfinite (q.l));
	}

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

static void
pow_negative_tests (void)
{
	GOQuad x, y, r;
	void *state = go_quad_start ();

#define POW(x_, y_, want_) do {						\
		go_quad_init (&x, (x_));				\
		go_quad_init (&y, (y_));				\
		r.h = r.l = 12345;					\
		go_quad_pow (&r, NULL, &x, &y);				\
		g_printerr ("pow(%g, %g) = %g  [%g]\n",			\
			    (double)(x_), (double)(y_),			\
			    go_quad_value (&r), (double)(want_));	\
		g_assert (go_quad_value (&r) == (want_));		\
	} while (0)

	POW (-2.0, 2.0, 4);
	POW (-2.0, 3.0, -8);
	POW (-2.0, -2.0, 0.25);
	POW (-2.0, -3.0, -0.125);
	POW (-1.5, 2.0, 2.25);
	POW (-1.5, 3.0, -3.375);
	POW (-1.0, 2.0, 1);
	POW (-1.0, 3.0, -1);

	/* Fractional powers of a negative base are undefined.  */
	{
		static const double bad[] = { 0.5, -0.5, 0.25, 2.5, -2.5 };
		unsigned i;

		for (i = 0; i < G_N_ELEMENTS (bad); i++) {
			go_quad_init (&x, -2);
			go_quad_init (&y, bad[i]);
			r.h = r.l = 12345;
			go_quad_pow (&r, NULL, &x, &y);
			g_printerr ("pow(-2, %g) = %g  [nan]\n",
				    bad[i], go_quad_value (&r));
			g_assert (isnan (go_quad_value (&r)));
		}
	}
	{
		go_quad_init (&x, -0.25);
		go_quad_init (&y, 0.25);
		r.h = r.l = 12345;
		go_quad_pow (&r, NULL, &x, &y);
		g_printerr ("pow(-0.25, 0.25) = %g  [nan]\n", go_quad_value (&r));
		g_assert (isnan (go_quad_value (&r)));
	}
	{
		go_quad_init (&x, -1.0);
		go_quad_init (&y, 0.5);
		r.h = r.l = 12345;
		go_quad_pow (&r, NULL, &x, &y);
		g_assert (isnan (go_quad_value (&r)));
	}

	go_quad_end (state);
#undef POW
}

/* ------------------------------------------------------------------------- */

static void
matrix_tests (void)
{
	GOQuadMatrix *A, *B, *C;
	void *state = go_quad_start ();
	double v;
	int i, j;

	/* In-place transpose of a square matrix.  */
	A = go_quad_matrix_new (2, 2);
	v = 1;
	for (i = 0; i < 2; i++)
		for (j = 0; j < 2; j++) {
			go_quad_init (&A->data[i][j], v);
			v += 1;
		}
	go_quad_matrix_transpose (A, A);
	g_printerr ("transpose(A,A) = [[%g %g], [%g %g]]  [[1 3], [2 4]]\n",
		    go_quad_value (&A->data[0][0]), go_quad_value (&A->data[0][1]),
		    go_quad_value (&A->data[1][0]), go_quad_value (&A->data[1][1]));
	g_assert (go_quad_value (&A->data[0][0]) == 1);
	g_assert (go_quad_value (&A->data[0][1]) == 3);
	g_assert (go_quad_value (&A->data[1][0]) == 2);
	g_assert (go_quad_value (&A->data[1][1]) == 4);

	/* Doing it twice is the identity.  */
	go_quad_matrix_transpose (A, A);
	g_assert (go_quad_value (&A->data[0][1]) == 2);
	g_assert (go_quad_value (&A->data[1][0]) == 3);
	go_quad_matrix_free (A);

	/* Non-square transposes still work.  */
	B = go_quad_matrix_new (3, 2);
	C = go_quad_matrix_new (2, 3);
	v = 1;
	for (i = 0; i < 3; i++)
		for (j = 0; j < 2; j++) {
			go_quad_init (&B->data[i][j], v);
			v += 1;
		}
	go_quad_matrix_transpose (C, B);
	g_assert (go_quad_value (&C->data[0][0]) == 1);
	g_assert (go_quad_value (&C->data[0][1]) == 3);
	g_assert (go_quad_value (&C->data[0][2]) == 5);
	g_assert (go_quad_value (&C->data[1][0]) == 2);
	g_assert (go_quad_value (&C->data[1][2]) == 6);
	go_quad_matrix_free (B);
	go_quad_matrix_free (C);

	/* Multiplication, determinant and inverse of [[1,2],[3,4]].  */
	{
		GOQuadMatrix *A, *inv, *I;
		GOQuad det;
		int i2, j2;
		double u = 1;

		A = go_quad_matrix_new (2, 2);
		for (i2 = 0; i2 < 2; i2++)
			for (j2 = 0; j2 < 2; j2++) {
				go_quad_init (&A->data[i2][j2], u);
				u += 1;
			}

		go_quad_matrix_determinant (A, &det);
		g_printerr ("det([[1,2],[3,4]]) = %g  [-2]\n",
			    go_quad_value (&det));
		g_assert (go_quad_value (&det) == -2);

		inv = go_quad_matrix_inverse (A, 0);
		g_assert (inv != NULL);
		if (inv) {
			g_assert (go_quad_value (&inv->data[0][0]) == -2);
			g_assert (go_quad_value (&inv->data[0][1]) == 1);
			g_assert (go_quad_value (&inv->data[1][0]) == 1.5);
			g_assert (go_quad_value (&inv->data[1][1]) == -0.5);

			I = go_quad_matrix_new (2, 2);
			go_quad_matrix_multiply (I, A, inv);
			for (i2 = 0; i2 < 2; i2++)
				for (j2 = 0; j2 < 2; j2++) {
					double want = (i2 == j2) ? 1 : 0;
					g_assert (fabs (go_quad_value (&I->data[i2][j2]) - want) < 1e-15);
				}
			go_quad_matrix_free (I);
			go_quad_matrix_free (inv);
		}
		go_quad_matrix_free (A);
	}

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

#ifdef GOFFICE_WITH_LONG_DOUBLE
static void
long_double_tests (void)
{
	GOQuadl a, b, r;
	void *state = go_quad_startl ();

	go_quad_initl (&a, 3);
	go_quad_initl (&b, 4);
	g_assert (go_quad_valuel (&a) == 3);

	go_quad_addl (&r, &a, &b);
	g_assert (go_quad_valuel (&r) == 7);
	go_quad_subl (&r, &a, &b);
	g_assert (go_quad_valuel (&r) == -1);
	go_quad_mull (&r, &a, &b);
	g_assert (go_quad_valuel (&r) == 12);
	go_quad_divl (&r, &a, &b);
	g_assert (go_quad_valuel (&r) == 0.75L);

	go_quad_initl (&a, 2);
	go_quad_sqrtl (&r, &a);
	g_printerr ("quadl sqrt(2) = %.21Lg\n", go_quad_valuel (&r));
	g_assert (fabsl (go_quad_valuel (&r) - sqrtl (2)) < 1e-18L);

	go_quad_logl (&r, &a);
	g_assert (fabsl (go_quad_valuel (&r) - logl (2)) < 1e-18L);

	go_quad_expl (&r, NULL, &a);
	g_assert (fabsl (go_quad_valuel (&r) - expl (2)) < 1e-18L);

	/* Same limb-by-limb behaviour as the double version.  */
	go_quad_initl (&a, -2.5L);
	a.l = ldexpl (1, -60);
	go_quad_absl (&r, &a);
	g_assert (r.h == 2.5L && r.l == -ldexpl (1, -60));
	go_quad_negatel (&r, &a);
	g_assert (r.h == 2.5L && r.l == -ldexpl (1, -60));

	go_quad_initl (&a, 1);
	a.l = ldexpl (1, -80);
	go_quad_scalbnl (&r, &a, 3);
	g_assert (r.h == 8 && r.l == ldexpl (1, -77));

	go_quad_endl (state);
}
#endif

/* ------------------------------------------------------------------------- */

static void
pow_precision_tests (void)
{
	GOQuad x, n, n1, one, A, B, C, d, y1, y2, r1, r2;
	void *state = go_quad_start ();
	double rel, got, want;

	/* (1+2^-52)^(2^54+1): the low bit of the exponent used to get
	 * rounded away in double precision.  */
	go_quad_init (&x, 1);
	x.l = ldexp (1, -52);
	go_quad_init (&n, ldexp (1, 54));
	go_quad_init (&one, 1);
	go_quad_add (&n1, &n, &one);

	go_quad_pow (&A, NULL, &x, &n1);
	go_quad_pow (&B, NULL, &x, &n);
	go_quad_mul (&C, &B, &x);
	go_quad_sub (&d, &A, &C);
	rel = (fabs (d.h) + fabs (d.l)) / fabs (go_quad_value (&C));
	g_printerr ("pow low-bit: rel diff %.3g  [<1e-25]\n", rel);
	g_assert (rel < 1e-25);

	/* 2^(0.5-2^-60) must not round the exponent to 1/2; compare with
	 * 2^(0.5-2^-59), the ratio being 2^(2^-60).  */
	go_quad_init (&y1, 0.5);
	y1.l = -ldexp (1, -60);
	go_quad_init (&y2, 0.5);
	y2.l = -ldexp (1, -59);
	go_quad_init (&x, 2);
	go_quad_pow (&r1, NULL, &x, &y1);
	go_quad_pow (&r2, NULL, &x, &y2);
	go_quad_sub (&d, &r1, &r2);
	got = go_quad_value (&d) / go_quad_value (&r1);
	want = log (2) * ldexp (1, -60);
	g_printerr ("pow frac-bit: (r1-r2)/r1 = %.9g  [%.9g]\n", got, want);
	g_assert (fabs (got - want) < 1e-12 * want);

	go_quad_end (state);
}

/* ------------------------------------------------------------------------- */

static void
pow_special_tests (void)
{
	GOQuad x, y, r;
	void *state = go_quad_start ();

#define POW(x_, y_, want_) do {						\
		go_quad_init (&x, (x_));				\
		go_quad_init (&y, (y_));				\
		r.h = r.l = 12345;					\
		go_quad_pow (&r, NULL, &x, &y);				\
		g_printerr ("pow(%g, %g) = %g  [%g]\n",			\
			    (double)(x_), (double)(y_),			\
			    go_quad_value (&r), (double)(want_));	\
		g_assert (go_quad_value (&r) == (want_));		\
	} while (0)

	POW (2.0, go_pinf, go_pinf);
	POW (2.0, go_ninf, 0);
	POW (0.5, go_pinf, 0);
	POW (0.5, go_ninf, go_pinf);
	POW (-2.0, go_pinf, go_pinf);
	POW (-2.0, go_ninf, 0);
	POW (-0.5, go_pinf, 0);
	POW (-0.5, go_ninf, go_pinf);
	POW (0.0, go_pinf, 0);
	POW (0.0, go_ninf, go_pinf);
	POW (1.0, go_pinf, 1);
	POW (1.0, go_ninf, 1);
	POW (-1.0, go_pinf, 1);
	POW (-1.0, go_ninf, 1);

	/* A NaN exponent gives NaN, not uninitialized memory.  */
	go_quad_init (&x, 2.0);
	go_quad_init (&y, go_nan);
	r.h = r.l = 12345;
	go_quad_pow (&r, NULL, &x, &y);
	g_assert (isnan (go_quad_value (&r)));

	/* exp(a) is pow(e, a) in the base-2 configuration.  */
	go_quad_init (&x, go_pinf);
	r.h = r.l = 12345;
	go_quad_exp (&r, NULL, &x);
	g_printerr ("exp(+inf) = %g  [inf]\n", go_quad_value (&r));
	g_assert (go_quad_value (&r) == go_pinf);

	go_quad_init (&x, go_ninf);
	r.h = r.l = 12345;
	go_quad_exp (&r, NULL, &x);
	g_printerr ("exp(-inf) = %g  [0]\n", go_quad_value (&r));
	g_assert (go_quad_value (&r) == 0);

	go_quad_end (state);
#undef POW
}

/* ------------------------------------------------------------------------- */

int
main (G_GNUC_UNUSED int argc, G_GNUC_UNUSED char **argv)
{
	libgoffice_init ();

	init_tests ();
	const_tests ();
	basic4_tests ();
	log_tests ();
	abs_negate_scalbn_tests ();
	mul12_tests ();
	dot_product_tests ();
	constant8_tests ();
	pow_tests ();
	pow_special_tests ();
	pow_negative_tests ();
	pow_precision_tests ();
	nonfinite_tests ();
	matrix_tests ();
	floor_tests ();
	atan2_tests ();
	inverse_special_tests ();
	hypot_tests ();
	trig_tests ();
	expm1_tests ();
#ifdef GOFFICE_WITH_LONG_DOUBLE
	long_double_tests ();
#endif

	libgoffice_shutdown ();

	return 0;
}
