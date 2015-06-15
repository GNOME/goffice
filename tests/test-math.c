#include <goffice/goffice.h>

#define REFTEST(a_,f_,r_, txt_)						\
	do {								\
		double a = (a_);					\
		double r = (r_);					\
		double fa = f_(a);					\
		g_printerr ("%s(%g) = %g  [%g]\n",			\
			    txt_, a, fa, r);				\
		g_assert (copysign (1,r) == copysign (1,fa));		\
		if (isnan (r))						\
			g_assert (isnan (fa));				\
		else if (r == floor (r))				\
			g_assert (r == fa);				\
		else							\
			g_assert (fabs (r - fa) / fabs (r) < 1e-14);	\
	} while (0)

#define REFTEST2(a_,b_,f_,r_, txt_)					\
	do {								\
		double a = (a_);					\
		double b = (b_);					\
		double r = (r_);					\
		double fab = f_(a, b);					\
		g_printerr ("%s(%g,%g) = %g  [%g]\n",			\
			    txt_, a, b, fab, r);			\
		g_assert (copysign (1,r) == copysign(1,fab));		\
		if (r * 256 == floor (r * 256))				\
			g_assert (r == fab);				\
		else							\
			g_assert (fabs (r - fab) / fabs (r) < 1e-14);	\
	} while (0)

#ifdef GOFFICE_WITH_LONG_DOUBLE

#define REFTESTl(a_,f_,r_, txt_)					\
	do {								\
		long double a = (a_);					\
		long double r = (r_);					\
		long double fa = f_(a);					\
		g_printerr ("%s(%Lg) = %Lg  [%Lg]\n",			\
			    txt_, a, fa, r);				\
		g_assert (copysignl (1,r) == copysignl (1,fa));		\
		if (isnanl (r))						\
			g_assert (isnanl (fa));				\
		else if (r == floorl (r))				\
			g_assert (r == fa);				\
		else							\
			g_assert (fabsl (r - fa) / fabsl (r) < 1e-14);	\
	} while (0)

#define REFTEST2l(a_,b_,f_,r_, txt_)					\
	do {								\
		long double a = (a_);					\
		long double b = (b_);					\
		long double r = (r_);					\
		long double fab = f_(a, b);				\
		g_printerr ("%s(%Lg,%Lg) = %Lg  [%Lg]\n",		\
			    txt_, a, b, fab, r);			\
		g_assert (copysignl (1,r) == copysignl(1,fab));		\
		if (r * 256 == floorl (r * 256))			\
			g_assert (r == fab);				\
		else							\
			g_assert (fabsl (r - fab) / fabsl (r) < 1e-14);	\
	} while (0)
#else
#define REFTESTl(a_,f_,r_, txt_)
#define REFTEST2l(a_,b_,f_,r_, txt_)
#endif

/* ------------------------------------------------------------------------- */

static double fake_sinpi (double x) { return x == floor (x) ? copysign(0,x) : sin (x * M_PI); }
static double fake_cospi (double x) { return x + 0.5 == floor (x + 0.5) ? +0 : cos (x * M_PI); }
static double fake_tanpi (double x) { x = fmod(x,1); return x + 0.5 == floor (x + 0.5) ? copysign (go_nan,x) : tan (x * M_PI); }
static double fake_cotpi (double x) { x = fmod(x,1); return x == 0 ? copysign (go_nan,x) : fake_cospi (x) / fake_sinpi (x); }
static double fake_atanpi (double x) { return atan (x) / M_PI; }
static double fake_atan2pi (double y, double x) { return atan2 (y,x) / M_PI; }


#define TEST1(a_) do {							\
	REFTEST(a_,go_sinpi,fake_sinpi(a_),"sinpi");			\
	REFTEST(a_,go_cospi,fake_cospi(a_),"cospi");			\
	REFTEST(a_,go_tanpi,fake_tanpi(a_),"tanpi");			\
	REFTEST(a_,go_cotpi,fake_cotpi(a_),"cotpi");			\
	REFTEST(a_,go_atanpi,fake_atanpi(a_),"atanpi");			\
									\
	REFTESTl(a_,go_sinpil,fake_sinpi(a_),"sinpil");			\
	REFTESTl(a_,go_cospil,fake_cospi(a_),"cospil");			\
	REFTESTl(a_,go_tanpil,fake_tanpi(a_),"tanpil");			\
	REFTESTl(a_,go_cotpil,fake_cotpi(a_),"cotpil");			\
	REFTESTl(a_,go_atanpil,fake_atanpi(a_),"atanpil");		\
} while (0)

#define TEST2(a_,b_) do {						\
		REFTEST2(a_,b_,go_atan2pi,fake_atan2pi(a_,b_),"atan2pi"); \
		REFTEST2l(a_,b_,go_atan2pil,fake_atan2pi(a_,b_),"atan2pil"); \
} while (0)

static void
trig_tests (void)
{
	double d;

	for (d = 0; d < 5; d += 0.125) {
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

	/* Since the results are n/4 these are tested exactly.  */
	REFTEST2(0,1,go_atan2pi,0,"atan2pi");
	REFTEST2(1,1,go_atan2pi,0.25,"atan2pi");
	REFTEST2(1,0,go_atan2pi,0.5,"atan2pi");
	REFTEST2(1,-1,go_atan2pi,0.75,"atan2pi");
	REFTEST2(0,-1,go_atan2pi,1,"atan2pi");
	REFTEST2(-1,1,go_atan2pi,-0.25,"atan2pi");
	REFTEST2(-1,0,go_atan2pi,-0.5,"atan2pi");
	REFTEST2(-1,-1,go_atan2pi,-0.75,"atan2pi");

	/* Since the results are n/4 these are tested exactly.  */
	REFTEST2l(0,1,go_atan2pil,0,"atan2pil");
	REFTEST2l(1,1,go_atan2pil,0.25,"atan2pil");
	REFTEST2l(1,0,go_atan2pil,0.5,"atan2pil");
	REFTEST2l(1,-1,go_atan2pil,0.75,"atan2pil");
	REFTEST2l(0,-1,go_atan2pil,1,"atan2pil");
	REFTEST2l(-1,1,go_atan2pil,-0.25,"atan2pil");
	REFTEST2l(-1,0,go_atan2pil,-0.5,"atan2pil");
	REFTEST2l(-1,-1,go_atan2pil,-0.75,"atan2pil");
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
