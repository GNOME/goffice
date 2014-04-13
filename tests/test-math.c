#include <goffice/goffice.h>

#define REFTEST(a_,f_,r_, txt_)				\
do {							\
	double a = (a_);				\
	double r = (r_);				\
	double fa = f_(a);				\
	g_printerr ("%s(%g) = %g  [%g]\n",		\
		    txt_, a, fa, r);			\
	if (r == floor (r))				\
		g_assert (r == fa);			\
	else						\
		g_assert (fabs (r - fa) / r < 1e-14);	\
} while (0)

#define REFTEST2(a_,b_,f_,r_, txt_)			\
do {							\
	double a = (a_);				\
	double b = (b_);				\
	double r = (r_);				\
	double fab = f_(a, b);				\
	g_printerr ("%s(%g,%g) = %g  [%g]\n",		\
		    txt_, a, b, fab, r);		\
	if (r == floor (r))				\
		g_assert (r == fab);			\
	else						\
		g_assert (fabs (r - fab) / r < 1e-14);	\
} while (0)

/* ------------------------------------------------------------------------- */

static double fake_sinpi (double x) { return x == floor (x) ? 0 : sin (x * M_PI); }
static double fake_cospi (double x) { return fake_sinpi (x + 0.5); }
static double fake_atan2pi (double y, double x) { return atan2 (y,x) / M_PI; }


#define TEST1(a_) do {					\
	REFTEST(a_,go_sinpi,fake_sinpi(a_),"sinpi");	\
	REFTEST(a_,go_cospi,fake_cospi(a_),"cospi");	\
	REFTEST(a_,go_tanpi,(fake_sinpi(a_)/fake_cospi(a_)),"tanpi");	\
} while (0)

#define TEST2(a_,b_) do {						\
		REFTEST2(a_,b_,go_atan2pi,fake_atan2pi(a_,b_),"atan2pi"); \
} while (0)

static void
trig_tests (void)
{
	double d;

	for (d = 0; d < 10; d += 0.125) {
		TEST1(d);
		TEST1(-d);
	}

	TEST2 (0, +2);
	TEST2 (0, -2);
	TEST2 (3, 0);
	TEST2 (-3, 0);
	TEST2 (0, 0);
	TEST2 (+1, +1);
	TEST2 (-1, +1);
	TEST2 (+1, -1);
	TEST2 (-1, -1);
	TEST2 (+2.3, +1.2);
	TEST2 (+2.3, -1.2);
	TEST2 (+2.3, +0.2);
	TEST2 (+2.3, -0.2);
	TEST2 (+0.2, +2.3);
	TEST2 (+0.2, -2.3);
	TEST2 (+2.3, +3);
	TEST2 (+2.3, -3);
	TEST2 (-2.3, +3);
	TEST2 (-2.3, -3);
	TEST2 (+2.3, +4);
	TEST2 (+2.3, -4);
	TEST2 (-2.3, +4);
	TEST2 (-2.3, -4);
	TEST2 (2, 100);
	TEST2 (2, -100);
	TEST2 (1.0/256, 1.0/1024);
	TEST2 (exp(1), 1);
	TEST2 (exp(1), -1);
	TEST2 (exp(1), log(2));
	TEST2 (exp(1), -log(2));
}

#undef TEST1
#undef TEST2

/* ------------------------------------------------------------------------- */

int
main (int argc, char **argv)
{
	libgoffice_init ();

	trig_tests ();

	libgoffice_shutdown ();

	return 0;
}
