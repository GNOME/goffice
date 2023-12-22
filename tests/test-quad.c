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

int
main (int argc, char **argv)
{
	init_tests ();
	const_tests ();
	basic4_tests ();
	pow_tests ();
	floor_tests ();
	atan2_tests ();
	hypot_tests ();
	trig_tests ();

	return 0;
}
