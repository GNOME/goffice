/*
 * go-accumulator.c: high-precision sums
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
 *
 * See http://code.activestate.com/recipes/393090/
 *
 * Note: for this to work, the processor must evaluate with the right
 * precision.  For ix86 that means trouble as the default is to evaluate
 * with long-double precision internally.  We solve this by setting the
 * relevant x87 control flag.
 *
 * Note: the compiler should not rearrange expressions.
 */

#include <goffice/goffice-config.h>
#include "go-accumulator.h"
#include "go-quad.h"
#include <math.h>
#include <float.h>

#ifndef DOUBLE

#define DOUBLE double
#define SUFFIX(_n) _n
#define DOUBLE_MANT_DIG DBL_MANT_DIG

struct GOAccumulator_ {
	GArray *partials;
};
#define ACC SUFFIX(GOAccumulator)

#ifdef GOFFICE_WITH_LONG_DOUBLE
#include "go-accumulator.c"
#undef DOUBLE
#undef SUFFIX
#undef DOUBLE_MANT_DIG
#define DOUBLE long double
#define SUFFIX(_n) _n ## l
#define DOUBLE_MANT_DIG LDBL_MANT_DIG

struct GOAccumulatorl_ {
	GArray *partials;
};

#endif
#endif




gboolean
SUFFIX(go_accumulator_functional) (void)
{
	return SUFFIX(go_quad_functional) ();
}

void *
SUFFIX(go_accumulator_start) (void)
{
	return SUFFIX(go_quad_start) ();
}

void
SUFFIX(go_accumulator_end) (void *state)
{
	SUFFIX(go_quad_end) (state);
}

ACC *
SUFFIX(go_accumulator_new) (void)
{
	ACC *res = g_new (ACC, 1);
	res->partials = g_array_sized_new (FALSE, FALSE, sizeof (DOUBLE), 10);
	return res;
}

void
SUFFIX(go_accumulator_free) (ACC *acc)
{
	g_return_if_fail (acc != NULL);

	g_array_free (acc->partials, TRUE);
	g_free (acc);
}

void
SUFFIX(go_accumulator_clear) (ACC *acc)
{
	g_return_if_fail (acc != NULL);
	g_array_set_size (acc->partials, 0);
}

void
SUFFIX(go_accumulator_add) (ACC *acc, DOUBLE x)
{
	unsigned ui = 0, uj;

	g_return_if_fail (acc != NULL);

	for (uj = 0; uj < acc->partials->len; uj++) {
		DOUBLE y = g_array_index (acc->partials, DOUBLE, uj);
		DOUBLE hi, lo;
		if (SUFFIX(fabs)(x) < SUFFIX(fabs)(y)) {
			DOUBLE t = x;
			x = y;
			y = t;
		}
		hi = x + y;
		lo = y - (hi - x);
		if (lo != 0) {
			g_array_index (acc->partials, DOUBLE, ui) = lo;
			ui++;
		}
		x = hi;
	}
	g_array_set_size (acc->partials, ui + 1);
	g_array_index (acc->partials, DOUBLE, ui) = x;
}

DOUBLE
SUFFIX(go_accumulator_value) (ACC *acc)
{
	unsigned ui;
	DOUBLE sum = 0;

	g_return_val_if_fail (acc != NULL, 0);

	for (ui = 0; ui < acc->partials->len; ui++)
		sum += g_array_index (acc->partials, DOUBLE, ui);

	return sum;
}
