/*
 * go-accumulator.c: high-precision sums
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
 *
 * See http://code.activestate.com/recipes/393090/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA.
 *
 * Note: for this to work, the processor must evaluate with the right
 * precision.  For ix86 that means trouble as the default is to evaluate
 * with long-double precision internally.  We solve this by setting the
 * relevant x87 control flag.
 *
 * Note: the compiler should not rearrange expressions.
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <math.h>
#include <float.h>

#ifndef DOUBLE

#define DOUBLE double
#define SUFFIX(_n) _n

struct GOAccumulator_ {
	GArray *partials;
};
#define ACC SUFFIX(GOAccumulator)

#ifdef GOFFICE_WITH_LONG_DOUBLE
#include "go-accumulator.c"
#undef DOUBLE
#undef SUFFIX
#define DOUBLE long double
#define SUFFIX(_n) _n ## l

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

/**
 * go_accumulator_start: (skip)
 **/
/**
 * go_accumulator_startl: (skip)
 **/
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

/**
 * go_accumulator_new: (skip)
 **/
/**
 * go_accumulator_newl: (skip)
 **/
ACC *
SUFFIX(go_accumulator_new) (void)
{
	ACC *res = g_new (ACC, 1);
	res->partials = g_array_sized_new (FALSE, FALSE, sizeof (DOUBLE), 10);
	return res;
}

/**
 * go_accumulator_free: (skip)
 **/
/**
 * go_accumulator_freel: (skip)
 **/
void
SUFFIX(go_accumulator_free) (ACC *acc)
{
	g_return_if_fail (acc != NULL);

	g_array_free (acc->partials, TRUE);
	g_free (acc);
}

/**
 * go_accumulator_clear: (skip)
 **/
/**
 * go_accumulator_clearl: (skip)
 **/
void
SUFFIX(go_accumulator_clear) (ACC *acc)
{
	g_return_if_fail (acc != NULL);
	g_array_set_size (acc->partials, 0);
}

/**
 * go_accumulator_add: (skip)
 **/
/**
 * go_accumulator_addl: (skip)
 **/
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

/**
 * go_accumulator_add_quad: (skip)
 **/
/**
 * go_accumulator_add_quadl: (skip)
 **/
void
SUFFIX(go_accumulator_add_quad) (ACC *acc, const SUFFIX(GOQuad) *x)
{
	g_return_if_fail (acc != NULL);
	g_return_if_fail (x != NULL);

	SUFFIX(go_accumulator_add) (acc, x->h);
	SUFFIX(go_accumulator_add) (acc, x->l);
}

/**
 * go_accumulator_value: (skip)
 **/
/**
 * go_accumulator_valuel: (skip)
 **/
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
