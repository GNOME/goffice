/*
 * go-bezier.c
 *
 * Copyright (C) 2008 Jean Brefort (jean.brefort@normalesup.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include "goffice-config.h"
#include "go-bezier.h"
#include <goffice/math/go-math.h>

/**
 * GOBezierSpline:
 * @x: control points abscissa.
 * @y: control points ordinates.
 * @n: spline segments number.
 * @closed: whether the curve is closed.
 **/

/**
 * go_bezier_spline_init:
 * @x: the x values
 * @y: the y values
 * @n: the number of x and y values
 * @closed: whether to return a closed curve or not
 *
 * @x and @y values must be valid and finite. The returned structure
 * contains the x and y coordinates of all control points, including the
 * incoming data. the n and closed fields are just copies of the corresponding
 * arguments.
 *
 * Returns: (transfer full): a newly created struct GOBezierSpline instance
 * which should be destroyed by a call to go_bezier_spline_destroy.
 **/
GOBezierSpline *
go_bezier_spline_init (double const *x, double const *y, int n, gboolean closed)
{
	int i, j, m = n - 1; /* having n-1 in a variable is a little optimization */
	GOBezierSpline *sp;
	double *a, *b, *c, *d, t;

	/* The equation to solve in the open case is

	|2 1 ...              0||b[0]  |     |y[1]-y[0]    |
	|1 4 1 ...             ||b[1]  |     |y[2]-y[0]    |
	|0 1 4 1 ...           ||b[2]  |     |y[3]-y[1]    |
	|0 0 1 4 1 ...         ||...   | = 3 |...          |
	|...                   ||...   |     |y[n-2]-y[n-4]|
	|...              1 4 1||b[n-2]|     |y[n-1]-y[n-3]|
	|0 ...              1 2||b[n-1]|     |y[n-1]-y[n-2]|

	and in the closed case:

	|4 1 ...              1||b[0]  |     |y[1]-y[n-1]  |
	|1 4 1 ...             ||b[1]  |     |y[2]-y[0]    |
	|0 1 4 1 ...           ||b[2]  |     |y[3]-y[1]    |
	|0 0 1 4 1 ...         ||...   | = 3 |...          |
	|...                   ||...   |     |y[n-2]-y[n-4]|
	|...              1 4 1||b[n-2]|     |y[n-1]-y[n-3]|
	|1 ...              1 4||b[n-1]|     |y[0]-y[n-2]  |

			    Similar equations must also be solved for x.
	*/

	/* Create and initialize the structure */
	sp = g_new0 (GOBezierSpline, 1);
	i = (closed)? 3 * n: 3 * n - 2;
	sp->x = g_new (double, i);
	sp->y = g_new (double, i);
	sp->n = n;
	sp->closed = closed;sp->ref_count = 1;

	/* Initialize vectors for intermediate data */
	a = g_new (double, n);
	b = g_new (double, n);

	/* Solve the equations for y */
	if (closed) {
		double *e;
		/* First, populate a vector with the y differences */
		a[0] = y[1] - y[m];
		for (i = 1; i < m; i++)
			a[i] = y[i+1] - y[i-1];
		a[m] = y[0] - y[m-1];

		/* Allocate c and d values */
			c = g_new (double, n);
			d = g_new (double, n);
		/* Allocate the e vector, we only need n-1 values */
		e = g_new (double, m);

		/* Now evaluate the b values. Each b[i] can be evaluated against
		 b[i+1] and b[m] in the form b[i]=c[i]*b[i+1]+d[i]+e[i]*b[m],
		 except the last one.

		 We have b[m] + 4*b[0] + b[1] = 3*a[0], so c[0]=-.25, d[0]=.75*a[0]
		 and e[0]=-.25.

		 For other indices (i<m), we have:
		 b[i-1]+4*b[i]+b[i+1]=3*a[i]
		 replacing b[i-1], we get:
			d[i-1]+(4+c[i-1])*b[i]+b[i+1]+e[i-1]*b[m]=3*a[i]
		 hence c[i]=-1/(4+c[i-1]), d[i]=(3*a[i]-d[i-1])/(4+c[i-1]),
		 and e[i]=-e[i-1]/(4+c[i-1])
		 */
		/* fill in the c, d and e data */
		c[0] = e[0] = -.25;
		d[0] = .75 * a[0];
		for (i = 1; i < m; i++) {
			t = 4. + c[i-1];
			c[i] = -1. / t;
			d[i] = (3 * a[i] - d[i-1]) / t;
			e[i] = -e[i-1] / t;
		}

		/* Now coming to the last value. We have:
		 b[m-1]+4*b[m]+b[0]= 3*a[m]
		 b[0] which was eliminated in the first step is back, so we must
		 evaluate b[m] against it and propagate the expression down
		 the stack in the form b[i]=c[i]*b[0]+d[i]

		 for the mth value, we get, replacing b[m-1]:
		 (4+c[m-1]+e[m-1])*b[m]+d[m-1]+b[0]=3*a[m]
		 which gives c[m]=-1/(4+c[m-1]+e[m-1]) and
		 d[m]=(3*a[m]-d[m-1])/(4+c[m-1]+e[m-1])
		 */
		t = 4. + c[m-1] + e[m-1];
		c[m] = -1 / t;
		d[m] = (3 * a[m] - d[m-1]) / t;

		/* For i>1, we now get, replacing b[i+1] and b[m]:
		 b[i]=c[i]*(c[i+1]*b[0]+d[i+1]) + d[i] + e[i]*(c[m]*b[0]+d[m])
		 This evaluates to:
		 c[i]=c[i]*c[i+1]+e[i]*c[m]
		 d[i]=c[i]*d[i+1]+d[i]+e[i]*d[m]
		 */
		for (i = m - 1; i > 0; i--) {
			/* Cache the new c[i] value in t */
			t = c[i] * c[i+1] + e[i] * c[m];
			d[i] = c[i] * d[i+1] + d[i] + e[i] * d[m];
			c[i] = t;
		}
		/* We can now evaluate b[0]:
		 b[0]=c[0]*(c[1]*b[0]+d[1])+d[0]+e[0]*(c[m]*b[0]+d[m])
		 which rearranges to:
		 b[0]*(1-c[0]*c[1]-e[0]*c[m])=c[0]*d[1]+d[0]+e[0]*d[m]
		*/
		b[0] = (c[0] * d[1] + d[0] + e[0] * d[m]) / (1. - c[0] * c[1] - e[0] * c[m]);

		/* Replacing b[0] now gives the other b values */
		for (i = 1; i < n; i++) {
			b[i] = c[i] * b[0] + d[i];
		}

		/* Populate the y vector of the resulting structure with values
		  for all the points */
		 sp->y[0] =  y[0];
		 sp->y[1] =  y[0] + b[0] / 3.;
		 for (i = 1, j = 2; i < n; i++) {
			 sp->y[j++] =  y[i] - b[i] / 3.;
			 sp->y[j++] =  y[i];
			 sp->y[j++] =  y[i] + b[i] / 3.;
		 }
		sp->y[j] = y[0] - b[0] / 3.;

		/* And now, do the same thing for x */
		a[0] = x[1] - x[m];
		for (i = 1; i < m; i++)
			a[i] = x[i+1] - x[i-1];
		a[m] = x[0] - x[m-1];
		c[0] = e[0] = -.25;
		d[0] = .75 * a[0];
		for (i = 1; i < m; i++) {
			t = 4. + c[i-1];
			c[i] = -1. / t;
			d[i] = (3 * a[i] - d[i-1]) / t;
			e[i] = -e[i-1] / t;
		}
		t = 4. + c[m-1] + e[m-1];
		c[m] = -1 / t;
		d[m] = (3 * a[m] - d[m-1]) / t;
		for (i = m - 1; i > 0; i--) {
			t = c[i] * c[i+1] + e[i] * c[m];
			d[i] = c[i] * d[i+1] + d[i] + e[i] * d[m];
			c[i] = t;
		}
		b[0] = (c[0] * d[1] + d[0] + e[0] * d[m]) / (1. - c[0] * c[1] - e[0] * c[m]);
		for (i = 1; i < n; i++) {
			b[i] = c[i] * b[0] + d[i];
		}
		 sp->x[0] =  x[0];
		 sp->x[1] =  x[0] + b[0] / 3.;
		 for (i = 1, j = 2; i < n; i++) {
			 sp->x[j++] =  x[i] - b[i] / 3.;
			 sp->x[j++] =  x[i];
			 sp->x[j++] =  x[i] + b[i] / 3.;
		 }
		sp->x[j] = x[0] - b[0] / 3.;

		/* clear the e vector */
		g_free (e);
	} else {
		/* First, populate a vector with the y differences multiplied by 3 */
		a[0] = y[1] - y[0];
		for (i = 1; i < m; i++)
			a[i] = y[i+1] - y[i-1];
		a[m] = y[m] - y[m-1];

		/* Allocate c and d values. We only need n-1 c and d values */
			c = g_new (double, m);
			d = g_new (double, m);

		/* Now evaluate the b values. Each b[i] can be evaluated against
		 b[i+1] in the form b[i]=c[i]*b[i+1]+d[i], except the last one
		 which will be directy evaluated.

		 We have b[0]=1.5*a[0]-.5*b[1], so that c[0]=-.5 and d[0]=1.5*a[0].

		 for other indices, we have:
			b[i-1]+4*b[i]+b[i+1]=3*a[i]
		 replacing b[i-1], we get:
			d[i-1]+(4+c[i-1])*b[i]+b[i+1]=3*a[i]
		 hence c[i]=-1/(4+c[i-1]) and d[i]=(3*a[i]-d[i-1])/(4+c[i-1])

		 and for last value:
			d[n-2]+(2+c[n-2])*b[n-1]=3*a[n-1]
		 */

		c[0] = -.5;
		d[0] = 1.5 * a[0];
		for (i = 1; i < m; i++) {
			t = 4. + c[i-1];
			c[i] = -1. / t;
			d[i] = (3 * a[i] - d[i-1]) / t;
		}
		/* We can now evaluate b[n-1] (m is still n-1) */
		b[m] = (3 * a[m] - d[n-2]) / (2. + c[n-2]);
		/* and we can recursively obtain the others */
		for (i = m - 1; i >= 0; i--)
			b[i] = c[i] * b[i+1] + d[i];

		/* Evaluation of the control points */
		/*
		 The two control points have y values given by
		 y[i]+b[i]/3 and y[i+1]-b[i+1]/3
		 */

		 /* Populate the y vector of the resulting structure with values
		  for all the points */
		 sp->y[0] =  y[0];
		 sp->y[1] =  y[0] + b[0] / 3.;
		 for (i = 1, j = 2; i < m; i++) {
			 sp->y[j++] =  y[i] - b[i] / 3.;
			 sp->y[j++] =  y[i];
			 sp->y[j++] =  y[i] + b[i] / 3.;
		 }
		 sp->y[j++] = y[m] - b[m] / 3.;
		 sp->y[j] = y[m];

		/* And now, do the same thing for x */
		a[0] = x[1] - x[0];
		for (i = 1; i < m; i++)
			a[i] = x[i+1] - x[i-1];
		a[m] = x[m] - x[m-1];
		d[0] = 1.5 * a[0];
		for (i = 1; i < m; i++) {
			t = 4. + c[i-1];
			c[i] = -1. / t;
			d[i] = (3 * a[i] - d[i-1]) / t;
		}
		b[m] = (3 * a[m] - d[n-2]) / (2. + c[n-2]);
		for (i = m - 1; i >= 0; i--)
			b[i] = c[i] * b[i+1] + d[i];
		 sp->x[0] =  x[0];
		 sp->x[1] =  x[0] + b[0] / 3.;
		 for (i = 1, j = 2; i < m; i++) {
			 sp->x[j++] =  x[i] - b[i] / 3.;
			 sp->x[j++] =  x[i];
			 sp->x[j++] =  x[i] + b[i] / 3.;
		 }
		 sp->x[j++] = x[m] - b[m] / 3.;
		 sp->x[j] = x[m];
	}

	/* free intermediate data */
	g_free (a);
	g_free (b);
	g_free (c);
	g_free (d);
	return sp;
}

/**
 * go_bezier_spline_destroy:
 * @sp: (transfer full): a struct GOBezierSpline instance
 *
 * Destroys the given structures after cleaning all allocated fields.
 **/
void
go_bezier_spline_destroy (GOBezierSpline *sp)
{
	g_return_if_fail (sp);
	if (sp->ref_count-- > 1)
		return;
	g_free (sp->x);
	g_free (sp->y);
	g_free (sp);
}

static GOBezierSpline *
go_bezier_spline_ref (GOBezierSpline *sp)
{
	g_return_val_if_fail (sp, NULL);
	sp->ref_count++;
	return sp;
}

GType
go_bezier_spline_get_type (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static ("GOBezierSpline",
			 (GBoxedCopyFunc)go_bezier_spline_ref,
			 (GBoxedFreeFunc)go_bezier_spline_destroy);
	}
	return t;
}

/**
 * go_bezier_spline_to_path:
 * @sp: a struct GOBezierSpline instance returned by go_bezier_spline_init
 *
 * Builds a GOPath using the control points evaluated in go_bezier_spline_init.
 *
 * Returns: (transfer full): GOPath matching spline
 **/
GOPath *
go_bezier_spline_to_path (GOBezierSpline *sp)
{
	int i, j;
	GOPath *path = go_path_new ();
	go_path_move_to (path, sp->x[0], sp->y[0]);
	for (i = j = 1; i < sp->n; i++, j += 3)
		go_path_curve_to (path, sp->x[j], sp->y[j], sp->x[j+1], sp->y[j+1], sp->x[j+2], sp->y[j+2]);
	if (sp->closed) {
		go_path_curve_to (path, sp->x[j], sp->y[j], sp->x[j+1], sp->y[j+1], sp->x[0], sp->y[0]);
		go_path_close (path);
	}
	return path;
}

/**
 * go_bezier_spline_to_cairo:
 * @sp: a struct GOBezierSpline instance returned by go_bezier_spline_init
 * @cr: a cairo context
 * @horiz_flip: whether to flip horizontally (for a RTL canvas).
 *
 * Renders the spline in \a cr
 **/
void
go_bezier_spline_to_cairo (GOBezierSpline *sp, cairo_t *cr, gboolean horiz_flip)
{
	int i, j;
	double sign = horiz_flip? -1: 1;
	cairo_new_path (cr);
	cairo_move_to (cr, sp->x[0] * sign, sp->y[0]);
	for (i = j = 1; i < sp->n; i++, j += 3)
		cairo_curve_to (cr, sp->x[j] * sign, sp->y[j], sp->x[j+1] * sign, sp->y[j+1], sp->x[j+2] * sign, sp->y[j+2]);
	if (sp->closed) {
		cairo_curve_to (cr, sp->x[j] * sign, sp->y[j], sp->x[j+1] * sign, sp->y[j+1], sp->x[0] * sign, sp->y[0]);
		cairo_close_path (cr);
	}
}
