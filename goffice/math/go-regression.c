/*
 * regression.c:  Statistical regression functions.
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
 *   Andrew Chatham <andrew.chatham@duke.edu>
 *   Daniel Carrera <dcarrera@math.toronto.edu>
 */

#include <goffice/goffice-config.h>
#include "go-regression.h"
#include "go-rangefunc.h"
#include "go-math.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ------------------------------------------------------------------------- */
/* Self-inclusion magic.  */

#ifndef DOUBLE

#define DEFINE_COMMON
#define DOUBLE double
#define DOUBLE_MANT_DIG DBL_MANT_DIG
#define SUFFIX(_n) _n
#define FORMAT_f "f"
#define FORMAT_g "g"

#ifdef GOFFICE_WITH_LONG_DOUBLE

#if 0
/* We define this to cause certain "double" functions to be implemented in
   terms of their "long double" counterparts.  */
#define BOUNCE
static GORegressionResult
general_linear_regressionl (long double *const *const xss, int xdim,
			    const long double *ys, int n,
			    long double *result,
			    go_regression_stat_tl *stat_,
			    gboolean affine);
#endif


#include "go-regression.c"
#undef BOUNCE
#undef DEFINE_COMMON
#undef DOUBLE
#undef DOUBLE_MANT_DIG
#undef SUFFIX
#undef FORMAT_f
#undef FORMAT_g
#ifdef HAVE_SUNMATH_H
#include <sunmath.h>
#endif
#define DOUBLE long double
#define DOUBLE_MANT_DIG LDBL_MANT_DIG
#define SUFFIX(_n) _n ## l
#define FORMAT_f "Lf"
#define FORMAT_g "Lg"
#endif

#endif

/* ------------------------------------------------------------------------- */

#ifdef DEFINE_COMMON

#define MATRIX DOUBLE **
#define CONSTMATRIX DOUBLE *const *const
#define QUAD SUFFIX(GOQuad)
#define QMATRIX QUAD **

#undef DEBUG_NEAR_SINGULAR
#undef DEBUG_REFINEMENT

#define ALLOC_MATRIX2(var,dim1,dim2,TYPE)		\
  do { int _i, _d1, _d2;				\
       _d1 = (dim1);					\
       _d2 = (dim2);					\
       (var) = g_new (TYPE *, _d1);			\
       for (_i = 0; _i < _d1; _i++)			\
	       (var)[_i] = g_new (TYPE, _d2);		\
  } while (0)
#define ALLOC_MATRIX(var,dim1,dim2) ALLOC_MATRIX2(var,dim1,dim2,DOUBLE)

#define FREE_MATRIX(var,dim1,dim2)			\
  do { int _i, _d1;					\
       _d1 = (dim1);					\
       for (_i = 0; _i < _d1; _i++)			\
	       g_free ((var)[_i]);			\
       g_free (var);					\
  } while (0)

#define COPY_MATRIX(dst,src,dim1,dim2)		\
  do { int _i, _j, _d1, _d2;			\
       _d1 = (dim1);				\
       _d2 = (dim2);				\
       for (_i = 0; _i < _d1; _i++)		\
	 for (_j = 0; _j < _d2; _j++)		\
	   (dst)[_i][_j] = (src)[_i][_j];	\
  } while (0)

#define ZERO_VECTOR(dst,dim)			\
  do { int _i, _d;				\
       _d = (dim);				\
       for (_i = 0; _i < _d; _i++)		\
	       (dst)[_i] = 0;			\
  } while (0)

#define COPY_VECTOR(dst,src,dim)		\
  do { int _i, _d;				\
       _d = (dim);				\
       for (_i = 0; _i < _d; _i++)		\
	       (dst)[_i] = (src)[_i];		\
  } while (0)

#define PRINT_MATRIX(var,dim1,dim2)				\
  do {								\
	int _i, _j, _d1, _d2;					\
	_d1 = (dim1);						\
	_d2 = (dim2);						\
	for (_j = 0; _j < _d2; _j++) {				\
	  for (_i = 0; _i < _d1; _i++) {			\
	    g_printerr (" %12.8" FORMAT_g, (var)[_i][_j]);	\
	  }							\
	  g_printerr ("\n");					\
	}							\
  } while (0)

#endif

/*
 *       ---> i
 *
 *  |    ********
 *  |    ********
 *  |    ********        A[i][j]
 *  v    ********
 *       ********
 *  j    ********
 *       ********
 *       ********
 *
 */

/* ------------------------------------------------------------------------- */
#ifdef BOUNCE
/* Suppport routines for bouncing double to long double.  */

static double *
shrink_vector (const long double *V, int d)
{
	if (V) {
		double *V2 = g_new (double, d);
		COPY_VECTOR (V2, V, d);
		return V2;
	} else
		return NULL;
}

static void
copy_stats (go_regression_stat_t *s1,
	    const go_regression_stat_tl *s2, int xdim)
{
	if (!s2)
		return;

	g_free (s1->se);
	g_free (s1->t);
	g_free (s1->xbar);

	s1->se = shrink_vector (s2->se, xdim);
	s1->t = shrink_vector (s2->t, xdim);
	s1->sqr_r = s2->sqr_r;
	s1->adj_sqr_r = s2->adj_sqr_r;
	s1->se_y = s2->se_y;
	s1->F = s2->F;
	s1->df_reg = s2->df_reg;
	s1->df_resid = s2->df_resid;
	s1->df_total = s2->df_total;
	s1->ss_reg = s2->ss_reg;
	s1->ss_resid = s2->ss_resid;
	s1->ss_total = s2->ss_total;
	s1->ms_reg = s2->ms_reg;
	s1->ms_resid = s2->ms_resid;
	s1->ybar = s2->ybar;
	s1->xbar = shrink_vector (s2->xbar, xdim);
	s1->var = s2->var;
}
#endif

/* ------------------------------------------------------------------------- */

static GORegressionResult
SUFFIX(QR) (CONSTMATRIX A, QMATRIX Q, QMATRIX R, int m, int n)
{
	int i, j, k;

	for (i = 0; i < m; i++)
		for (j = 0; j < n; j++)
			SUFFIX(go_quad_init) (&Q[i][j], A[i][j]);

	for (k = 0; k < m; k++) {
		QUAD L;
		int i;

		SUFFIX(go_quad_dot_product) (&L, Q[k], Q[k], n);
		SUFFIX(go_quad_sqrt) (&L, &L);
#if 0
		PRINT_MATRIX (Q, m, n);
		g_printerr ("L[%d] = %20.15" FORMAT_g "\n", k, L);
#endif
		if (SUFFIX(go_quad_value)(&L) == 0)
			return GO_REG_singular;

		for (i = 0; i < n; i++)
			SUFFIX(go_quad_div) (&Q[k][i], &Q[k][i], &L);

		R[k][k] = L;
		for (i = k + 1; i < m; i++) {
			QUAD ip;
			int j;

			SUFFIX(go_quad_init) (&R[k][i], 0);

			SUFFIX(go_quad_dot_product) (&ip, Q[k], Q[i], n);
			R[i][k] = ip;

			for (j = 0; j < n; j++) {
				QUAD p;
				SUFFIX(go_quad_mul) (&p, &ip, &Q[k][j]);
				SUFFIX(go_quad_sub) (&Q[i][j], &Q[i][j], &p);
			}
		}
	}

	return GO_REG_ok;
}

#ifndef BOUNCE

static void
SUFFIX(calc_residual) (CONSTMATRIX A, const DOUBLE *b, int dim, int n,
		       const QUAD *y, QUAD *residual, QUAD *N2)
{
	int i, j;

	SUFFIX(go_quad_init) (N2, 0);

	for (i = 0; i < n; i++) {
		QUAD d;

		SUFFIX(go_quad_init) (&d, b[i]);

		for (j = 0; j < dim; j++) {
			QUAD Aji, e;
			SUFFIX(go_quad_init) (&Aji, A[j][i]);
			SUFFIX(go_quad_mul) (&e, &Aji, &y[j]);
			SUFFIX(go_quad_sub) (&d, &d, &e);
		}

		if (residual)
			residual[i] = d;

		SUFFIX(go_quad_mul) (&d, &d, &d);
		SUFFIX(go_quad_add) (N2, N2, &d);
	}
}

static void
SUFFIX(refine) (CONSTMATRIX A, const DOUBLE *b, int dim, int n,
		QMATRIX Q, QMATRIX R, QUAD *result)
{
	QUAD *residual = g_new (QUAD, n);
	QUAD *newresidual = g_new (QUAD, n);
	QUAD *delta = g_new (QUAD, dim);
	QUAD *newresult = g_new (QUAD, dim);
	int pass;
	QUAD best_N2;

	SUFFIX(calc_residual) (A, b, dim, n, result, residual, &best_N2);
#ifdef DEBUG_REFINEMENT
	g_printerr ("-: Residual norm = %20.15" FORMAT_g "\n",
		    SUFFIX(go_quad_value) (&best_N2));
#endif

	for (pass = 1; pass < 10; pass++) {
		int i, j;
		QUAD dres, N2;

		/* newresult = R^-1 Q^T residual */
		for (i = dim - 1; i >= 0; i--) {
			QUAD acc;

			SUFFIX(go_quad_dot_product) (&acc, Q[i], residual, n);

			for (j = i + 1; j < dim; j++) {
				QUAD Rd;
				SUFFIX(go_quad_mul) (&Rd, &R[j][i], &delta[j]);
				SUFFIX(go_quad_sub) (&acc, &acc, &Rd);
			}

			SUFFIX(go_quad_div) (&delta[i], &acc, &R[i][i]);

			SUFFIX(go_quad_add) (&newresult[i],
					     &delta[i],
					     &result[i]);
#ifdef DEBUG_REFINEMENT
			g_printerr ("d[%2d] = %20.15" FORMAT_f
				    "  %20.15" FORMAT_f
				    "  %20.15" FORMAT_f "\n",
				    i,
				    SUFFIX(go_quad_value) (&delta[i]),
				    SUFFIX(go_quad_value) (&result[i]),
				    SUFFIX(go_quad_value) (&newresult[i]));
#endif
		}

		SUFFIX(calc_residual) (A, b, dim, n, newresult, newresidual, &N2);
		SUFFIX (go_quad_sub) (&dres, &N2, &best_N2);

#ifdef DEBUG_REFINEMENT
		g_printerr ("%d: Residual norm = %20.15" FORMAT_g
			    " (%.5" FORMAT_g ")\n",
			    pass,
			    SUFFIX(go_quad_value) (&N2),
			    SUFFIX(go_quad_value) (&dres));
#endif
		if (SUFFIX(go_quad_value) (&dres) >= 0)
			break;

		best_N2 = N2;
		COPY_VECTOR (result, newresult, dim);
		COPY_VECTOR (residual, newresidual, n);
	}

	g_free (residual);
	g_free (newresidual);
	g_free (newresult);
	g_free (delta);
}

#endif

/* ------------------------------------------------------------------------- */

/* Returns in res the solution to the equation L * U * res = P * b.

   This function is adapted from pseudocode in
	Introduction to Algorithms_. Cormen, Leiserson, and Rivest.
	p. 753. MIT Press, 1990.
*/
static void
SUFFIX(backsolve) (MATRIX LU, int *P, DOUBLE *b, int n, DOUBLE *res)
{
	int i, j;

#if 0
	PRINT_MATRIX(LU,n,n);
#endif

	for (i = 0; i < n; i++) {
		res[i] = b[P[i]];
		for (j = 0; j < i; j++)
			res[i] -= LU[i][j] * res[j];
	}

	for (i = n - 1; i >= 0; i--) {
		for (j = i + 1; j < n; j++)
			res[i] -= LU[i][j] * res[j];
		res[i] /= LU[i][i];
	}
}

static void
SUFFIX(rescale) (MATRIX A, DOUBLE *b, int n, DOUBLE *pdet)
{
	int i;

	*pdet = 1;
	for (i = 0; i < n; i++) {
		int j, expn;
		DOUBLE scale, max;

		(void)SUFFIX(go_range_maxabs) (A[i], n, &max);

		if (max == 0) {
			*pdet = 0;
			continue;
		}

		/* Use a power of 2 near sqrt (max) as scale.  */
		(void)SUFFIX(frexp) (SUFFIX(sqrt) (max), &expn);
		scale = SUFFIX(ldexp) (1, expn);
#ifdef DEBUG_NEAR_SINGULAR
		g_printerr ("scale[%d]=%" FORMAT_g "\n", i, scale);
#endif

		*pdet *= scale;
		b[i] /= scale;
		for (j = 0; j < n; j++)
			A[i][j] /= scale;
	}
}


/*
 * Performs an LUP Decomposition; LU and P must already be allocated.
 * A is not destroyed.
 *
 * This function is adapted from pseudocode in
 *   _Introduction to Algorithms_. Cormen, Leiserson, and Rivest.
 *   p 759. MIT Press, 1990.
 *
 * A rescaling of rows is done and the b_scaled vector is scaled
 * accordingly.
 */
static GORegressionResult
SUFFIX(LUPDecomp) (CONSTMATRIX A, MATRIX LU, int *P, int n, DOUBLE *b_scaled,
		   DOUBLE *pdet)
{
	int i, j, k, tempint;
	DOUBLE highest = 0;
	DOUBLE lowest = SUFFIX(go_pinf);
	DOUBLE cond;
	DOUBLE det = 1;

	COPY_MATRIX (LU, A, n, n);
	for (j = 0; j < n; j++)
		P[j] = j;

	*pdet = 0;

#ifdef DEBUG_NEAR_SINGULAR
	PRINT_MATRIX (LU, n, n);
#endif

	SUFFIX(rescale) (LU, b_scaled, n, &det);

	for (i = 0; i < n; i++) {
		DOUBLE max = -42;
		int mov = -1;

		for (j = i; j < n; j++)
			if (SUFFIX(fabs) (LU[j][i]) > max) {
				max = SUFFIX(fabs) (LU[j][i]);
				mov = j;
			}
#ifdef DEBUG_NEAR_SINGULAR
		PRINT_MATRIX (LU, n, n);
		g_printerr ("max[%d]=%" FORMAT_g " at %d\n", i, max, mov);
#endif
		if (max > highest)
			highest = max;
		if (max < lowest)
			lowest = max;

		if (mov == -1)
			return GO_REG_invalid_data;

		if (i != mov) {
			/*swap the two rows */

			det = 0 - det;
			tempint = P[i];
			P[i] = P[mov];
			P[mov] = tempint;
			for (j = 0; j < n; j++) {
				DOUBLE temp = LU[i][j];
				LU[i][j] = LU[mov][j];
				LU[mov][j] = temp;
			}
		}

		for (j = i + 1; j < n; j++) {
			LU[j][i] /= LU[i][i];
			for (k = i + 1; k < n; k++)
				LU[j][k] -= LU[j][i] * LU[i][k];
		}
	}

	/* Calculate the determinant.  */
	for (i = 0; i < n; i++)
		det *= LU[i][i];
	*pdet = det;

	if (det == 0)
		return GO_REG_singular;

	cond = lowest > 0
		? SUFFIX(log) (highest / lowest) / SUFFIX(log) (2)
		: SUFFIX(go_pinf);

#ifdef DEBUG_NEAR_SINGULAR
	g_printerr ("cond=%.20" FORMAT_g "\n", cond);
#endif

	/* FIXME: make some science out of this.  */
	if (cond > DOUBLE_MANT_DIG * 0.75)
		return GO_REG_near_singular_bad;
	else if (cond > DOUBLE_MANT_DIG * 0.50)
		return GO_REG_near_singular_good;
	else
		return GO_REG_ok;
}


GORegressionResult
SUFFIX(go_linear_solve) (CONSTMATRIX A, const DOUBLE *b, int n, DOUBLE *res)
{
	GORegressionResult err;
	MATRIX LU;
	DOUBLE *b_scaled;
	int *P;
	DOUBLE det;

	if (n < 1)
		return GO_REG_not_enough_data;

	/* Special case.  */
	if (n == 1) {
		DOUBLE d = A[0][0];
		if (d == 0)
			return GO_REG_singular;

		res[0] = b[0] / d;
		return GO_REG_ok;
	}

	/* Special case.  */
	if (n == 2) {
		DOUBLE d = SUFFIX(go_matrix_determinant) (A, n);
		if (d == 0)
			return GO_REG_singular;

		res[0] = (A[1][1] * b[0] - A[1][0] * b[1]) / d;
		res[1] = (A[0][0] * b[1] - A[0][1] * b[0]) / d;
		return GO_REG_ok;
	}

	/*
	 * Otherwise, use LUP-decomposition to find res such that
	 * A res = b
	 */
	ALLOC_MATRIX (LU, n, n);
	P = g_new (int, n);

	b_scaled = g_new (DOUBLE, n);
	COPY_VECTOR (b_scaled, b, n);

	err = SUFFIX(LUPDecomp) (A, LU, P, n, b_scaled, &det);

	if (err == GO_REG_ok || err == GO_REG_near_singular_good)
		SUFFIX(backsolve) (LU, P, b_scaled, n, res);

	FREE_MATRIX (LU, n, n);
	g_free (P);
	g_free (b_scaled);
	return err;
}


gboolean
SUFFIX(go_matrix_invert) (MATRIX A, int n)
{
	QMATRIX Q;
	QMATRIX R;
	GORegressionResult err;
	gboolean has_result;

	ALLOC_MATRIX2 (Q, n, n, QUAD);
	ALLOC_MATRIX2 (R, n, n, QUAD);

	err = SUFFIX(QR) (A, Q, R, n, n);
	has_result = (err == GO_REG_ok || err == GO_REG_near_singular_good);

	if (has_result) {
		int i, j, k;
		for (k = 0; k < n; k++) {
			/* Solve R x = Q^T e_k */
			for (i = n - 1; i >= 0; i--) {
				QUAD d = Q[i][k];
				for (j = i + 1; j < n; j++) {
					QUAD p;
					SUFFIX(go_quad_init) (&p, A[k][j]);
					SUFFIX(go_quad_mul) (&p, &R[j][i], &p);
					SUFFIX(go_quad_sub) (&d, &d, &p);
				}
				SUFFIX(go_quad_div) (&d, &d, &R[i][i]);
				A[k][i] = SUFFIX(go_quad_value) (&d);
			}
		}
	}

	FREE_MATRIX (Q, n, n);
	FREE_MATRIX (R, n, n);

	return has_result;
}

DOUBLE
SUFFIX(go_matrix_determinant) (CONSTMATRIX A, int n)
{
	GORegressionResult err;
	MATRIX LU;
	DOUBLE *b_scaled, det;
	int *P;

	if (n < 1)
		return 0;

	/* Special case.  */
	if (n == 1)
		return A[0][0];

	/* Special case.  */
	if (n == 2)
		return A[0][0] * A[1][1] - A[1][0] * A[0][1];

	/*
	 * Otherwise, use LUP-decomposition to find res such that
	 * A res = b
	 */
	ALLOC_MATRIX (LU, n, n);
	P = g_new (int, n);
	b_scaled = g_new0 (DOUBLE, n);

	err = SUFFIX(LUPDecomp) (A, LU, P, n, b_scaled, &det);

	FREE_MATRIX (LU, n, n);
	g_free (P);
	g_free (b_scaled);

	return det;
}

/* ------------------------------------------------------------------------- */

static GORegressionResult
SUFFIX(general_linear_regression) (CONSTMATRIX xss, int xdim,
				   const DOUBLE *ys, int n,
				   DOUBLE *result,
				   SUFFIX(go_regression_stat_t) *stat_,
				   gboolean affine)
{
	GORegressionResult regerr;
#ifdef BOUNCE
	long double **xss2, *ys2, *result2;
	go_regression_stat_tl *stat2;

	ALLOC_MATRIX2 (xss2, xdim, n, long double);
	COPY_MATRIX (xss2, xss, xdim, n);

	ys2 = g_new (long double, n);
	COPY_VECTOR (ys2, ys, n);

	result2 = g_new (long double, xdim);
	stat2 = stat_ ? go_regression_stat_newl () : NULL;

	regerr = general_linear_regressionl (xss2, xdim, ys2, n,
					     result2, stat2, affine);
	COPY_VECTOR (result, result2, xdim);
	copy_stats (stat_, stat2, xdim);

	go_regression_stat_destroyl (stat2);
	g_free (result2);
	g_free (ys2);
	FREE_MATRIX (xss2, xdim, n);
#else
	QMATRIX Q;
	QMATRIX R;
	QUAD *qresult;
	int i,j;
	gboolean has_result;
	void *state;
	gboolean has_stat;

	ZERO_VECTOR (result, xdim);

	has_stat = (stat_ != NULL);
	if (!has_stat)
		stat_ = SUFFIX(go_regression_stat_new)();

	memset (stat_, 0, sizeof (stat_));

	if (xdim > n) {
		regerr = GO_REG_not_enough_data;
		goto out;
	}

	state = SUFFIX(go_quad_start) ();

	ALLOC_MATRIX2 (Q, xdim, n, QUAD);
	ALLOC_MATRIX2 (R, xdim, xdim, QUAD);
	qresult = g_new0 (QUAD, xdim);

	regerr = SUFFIX(QR) (xss, Q, R, xdim, n);
	has_result = regerr == GO_REG_ok || regerr == GO_REG_near_singular_good;

	if (has_result) {
		/* Solve R res = Q^T ys */
		for (i = xdim - 1; i >= 0; i--) {
			QUAD acc;

			SUFFIX(go_quad_init) (&acc, 0);
			for (j = 0; j < n; j++) {
				QUAD p;
				SUFFIX(go_quad_init) (&p, ys[j]);
				SUFFIX(go_quad_mul) (&p, &p, &Q[i][j]);
				SUFFIX(go_quad_add) (&acc, &acc, &p);
			}

			for (j = i + 1; j < xdim; j++) {
				QUAD d;
				SUFFIX (go_quad_mul) (&d, &R[j][i], &qresult[j]);
				SUFFIX (go_quad_sub) (&acc, &acc, &d);
			}

			SUFFIX(go_quad_div) (&qresult[i], &acc, &R[i][i]);
		}

		if (xdim > 2)
			SUFFIX(refine) (xss, ys, xdim, n, Q, R, qresult);

		/* Round to plain precision.  */
		for (i = 0; i < xdim; i++) {
			result[i] = SUFFIX(go_quad_value) (&qresult[i]);
			SUFFIX(go_quad_init) (&qresult[i], result[i]);
		}
	}

	if (has_result) {
		GORegressionResult err2;
		QUAD *inv = g_new (QUAD, xdim);
		int err;
		int k;
		QUAD N2;

		/* This should not fail since n >= 1.  */
		err = SUFFIX(go_range_average) (ys, n, &stat_->ybar);
		g_assert (err == 0);

		/* FIXME: we ought to have a devsq variant that does not
		   recompute the mean.  */
		if (affine)
			err = SUFFIX(go_range_devsq) (ys, n, &stat_->ss_total);
		else
			err = SUFFIX(go_range_sumsq) (ys, n, &stat_->ss_total);
		g_assert (err == 0);

		stat_->xbar = g_new (DOUBLE, xdim);
		for (i = 0; i < xdim; i++) {
			int err = SUFFIX(go_range_average) (xss[i], n, &stat_->xbar[i]);
			g_assert (err == 0);
		}

		SUFFIX(calc_residual) (xss, ys, xdim, n, qresult, NULL, &N2);
		stat_->ss_resid = SUFFIX(go_quad_value) (&N2);

		stat_->sqr_r = (stat_->ss_total == 0)
			? 1
			: 1 - stat_->ss_resid / stat_->ss_total;
		if (stat_->sqr_r < 0) {
			/*
			 * This is an indication that something has gone wrong
			 * numerically.
			 */
			regerr = GO_REG_near_singular_bad;
		}

		/* FIXME: we want to guard against division by zero.  */
		stat_->adj_sqr_r = 1 - stat_->ss_resid * (n - 1) /
			((n - xdim) * stat_->ss_total);
		if (n == xdim)
			SUFFIX(go_quad_init) (&N2, 0);
		else {
			QUAD d;
			SUFFIX(go_quad_init) (&d, n - xdim);
			SUFFIX(go_quad_div) (&N2, &N2, &d);
		}
		stat_->var = SUFFIX(go_quad_value) (&N2);

		err2 = GO_REG_ok;
		stat_->se = g_new (DOUBLE, xdim);
		for (k = 0; k < xdim; k++) {
			/* Solve R^T z = e_k */
			for (i = 0; i < xdim; i++) {
				QUAD d;
				SUFFIX(go_quad_init) (&d, i == k ? 1 : 0);
				for (j = 0; j < i; j++) {
					QUAD p;
					SUFFIX(go_quad_mul) (&p, &R[i][j], &inv[j]);
					SUFFIX(go_quad_sub) (&d, &d, &p);
				}
				SUFFIX(go_quad_div) (&inv[i], &d, &R[i][i]);
			}

			/* Solve R inv = z */
			for (i = xdim - 1; i >= 0; i--) {
				QUAD d = inv[i];
				for (j = i + 1; j < xdim; j++) {
					QUAD p;
					SUFFIX(go_quad_mul) (&p, &R[j][i], &inv[j]);
					SUFFIX(go_quad_sub) (&d, &d, &p);
				}
				SUFFIX(go_quad_div) (&inv[i], &d, &R[i][i]);
			}

			if (SUFFIX(go_quad_value) (&inv[k]) < 0) {
				/*
				 * If this happens, something is really
				 * wrong, numerically.
				 */
				g_printerr ("inv[%d]=%" FORMAT_g "\n",
					    k,
					    SUFFIX(go_quad_value) (&inv[k]));
				regerr = GO_REG_near_singular_bad;
			}
			{
				QUAD p;
				SUFFIX(go_quad_mul) (&p, &N2, &inv[k]);
				SUFFIX(go_quad_sqrt) (&p, &p);
				stat_->se[k] = SUFFIX(go_quad_value) (&p);
			}
		}
		g_free (inv);

		stat_->t = g_new (DOUBLE, xdim);

		for (i = 0; i < xdim; i++)
			stat_->t[i] = (stat_->se[i] == 0)
				? SUFFIX(go_pinf)
				: result[i] / stat_->se[i];

		stat_->df_resid = n - xdim;
		stat_->df_reg = xdim - (affine ? 1 : 0);
		stat_->df_total = stat_->df_resid + stat_->df_reg;

		stat_->F = (stat_->sqr_r == 1)
			? SUFFIX(go_pinf)
			: ((stat_->sqr_r / stat_->df_reg) /
			   (1 - stat_->sqr_r) * stat_->df_resid);

		stat_->ss_reg =  stat_->ss_total - stat_->ss_resid;
		stat_->se_y = SUFFIX(sqrt) (stat_->ss_total / n);
		stat_->ms_reg = (stat_->df_reg == 0)
			? 0
			: stat_->ss_reg / stat_->df_reg;
		stat_->ms_resid = (stat_->df_resid == 0)
			? 0
			: stat_->ss_resid / stat_->df_resid;
	}

	SUFFIX(go_quad_end) (state);

	g_free (qresult);
	FREE_MATRIX (Q, xdim, n);
	FREE_MATRIX (R, xdim, xdim);
out:
	if (!has_stat)
		SUFFIX(go_regression_stat_destroy) (stat_);
#endif

	return regerr;
}

/* ------------------------------------------------------------------------- */

typedef struct {
  DOUBLE min_x;
  DOUBLE max_x;
  DOUBLE min_y;
  DOUBLE max_y;
  DOUBLE mean_y;
} SUFFIX(point_cloud_measure_type);

/* Takes the current 'sign' (res[0]) and 'c' (res[3]) from the calling
 * function, transforms xs to ln(sign*(x-c)), performs a simple
 * linear regression to find the best fitting 'a' (res[1]) and 'b'
 * (res[2]) for ys and transformed xs, and computes the sum of squared
 * residuals.
 * Needs 'sign' (i.e. +1 or -1) and 'c' so adjusted that (sign*(x-c)) is
 * positive for all xs. n must be > 0. These conditions are trusted to be
 * checked by the calling functions.
 * Is called often, so do not make it too slow.
 */

static int
SUFFIX(transform_x_and_linear_regression_log_fitting)
	(DOUBLE *xs,
	 DOUBLE *transf_xs,
	 const DOUBLE *ys, int n,
	 DOUBLE *res,
	 SUFFIX(point_cloud_measure_type)
	 *point_cloud)
{
        int i;
	int result = GO_REG_ok;
	DOUBLE mean_transf_x, diff_x, resid_y;
	DOUBLE sum1 = 0;
	DOUBLE sum2 = 0;

	/* log (always > 0) */
	for (i=0; i<n; i++)
	        transf_xs[i] = SUFFIX(log) (res[0] * (xs[i] - res[3]));
	SUFFIX(go_range_average) (transf_xs, n, &mean_transf_x);
	for (i=0; i<n; i++) {
	        diff_x = transf_xs[i] - mean_transf_x;
		sum1 += diff_x * (ys[i] - point_cloud->mean_y);
		sum2 += diff_x * diff_x;
	}
	res[2] = sum1 / sum2;
	res[1] = point_cloud->mean_y - (res[2] * mean_transf_x);
	res[4] = 0;
	for (i=0; i<n; i++) {
	        resid_y = res[1] + (res[2] * transf_xs[i]) - ys[i];
		res[4] += resid_y * resid_y;
	}
	return result; /* not used for error checking for the sake of speed */
}

static int
SUFFIX(log_fitting) (DOUBLE *xs, const DOUBLE *ys, int n,
		     DOUBLE *res, SUFFIX(point_cloud_measure_type) *point_cloud)
{
        int result = GO_REG_ok;
	gboolean sign_plus_ok = 1, sign_minus_ok = 1;
	DOUBLE x_range, c_step, c_accuracy_int, c_offset, c_accuracy;
	DOUBLE c_range, c_start, c_end, c_dist;
	DOUBLE *temp_res;
        DOUBLE *transf_xs;

	temp_res = g_new (DOUBLE, 5);
	x_range = (point_cloud->max_x) - (point_cloud->min_x);
	/* Not needed here, but allocate it once for all subfunction calls */
	transf_xs = g_new (DOUBLE, n);
	/* Choose final accuracy of c with respect to range of xs.
	 * Make accuracy be a whole power of 10. */
	c_accuracy = SUFFIX(log10) (x_range);
	if (c_accuracy < 0)
	        if (SUFFIX(modf) (c_accuracy, &c_accuracy_int) != 0)
		        c_accuracy--;
	SUFFIX(modf) (c_accuracy, &c_accuracy_int);
	c_accuracy = c_accuracy_int;
	c_accuracy = SUFFIX(pow) (10, c_accuracy);
	c_accuracy *= GO_LOGFIT_C_ACCURACY;

	/* Determine sign. Take a c which is ``much to small'' since the part
	 * of the curve cutting the point cloud is almost not bent.
	 * If making c still smaller does not make things still worse,
	 * assume that we have to change the direction of curve bending
	 * by changing sign.
	 */
	c_step = x_range * GO_LOGFIT_C_STEP_FACTOR;
	c_range = x_range * GO_LOGFIT_C_RANGE_FACTOR;
	res[0] = 1; /* sign */
	res[3] = point_cloud->min_x - c_range;
	temp_res[0] = 1;
	temp_res[3] = res[3] - c_step;
	SUFFIX(transform_x_and_linear_regression_log_fitting) (xs, transf_xs, ys, n,
						       res, point_cloud);
	SUFFIX(transform_x_and_linear_regression_log_fitting) (xs, transf_xs, ys, n,
						       temp_res, point_cloud);
	if (temp_res[4] <= res[4])
	        sign_plus_ok = 0;
        /* check again with new sign */
	res[0] = -1; /* sign */
	res[3] = point_cloud->max_x + c_range;
	temp_res[0] = -1;
	temp_res[3] = res[3] + c_step;
	SUFFIX(transform_x_and_linear_regression_log_fitting) (xs, transf_xs, ys, n,
						       res, point_cloud);
	SUFFIX(transform_x_and_linear_regression_log_fitting) (xs, transf_xs, ys, n,
						       temp_res, point_cloud);
	if (temp_res[4] <= res[4])
	        sign_minus_ok = 0;
	/* If not exactly one of plus or minus works, give up.
	 * This happens in point clouds which are very weakly bent.
	 */
	if (sign_plus_ok && !sign_minus_ok)
	        res[0] = 1;
	else if (sign_minus_ok && !sign_plus_ok)
	        res[0] = -1;
	else {
	        result = GO_REG_invalid_data;
		goto out;
	}

	/* Start of fitted c-range. Rounded to final accuracy of c. */
	c_offset = (res[0] == 1) ? point_cloud->min_x : point_cloud->max_x;
	c_offset = c_accuracy * ((res[0] == 1) ?
				 SUFFIX(floor) (c_offset / c_accuracy)
				 : SUFFIX(ceil) (c_offset /c_accuracy));

	/* Now the adapting of c starts. Find a local minimum of sum
	 * of squared residuals. */

	/* First, catch some unsuitably shaped point clouds. */
	res[3] = c_offset - res[0] * c_accuracy;
	temp_res[3] = c_offset - res[0] * 2 * c_accuracy;
	temp_res[0] = res[0];
	SUFFIX(transform_x_and_linear_regression_log_fitting) (xs, transf_xs, ys, n,
						       res, point_cloud);
	SUFFIX(transform_x_and_linear_regression_log_fitting) (xs, transf_xs, ys, n,
						       temp_res, point_cloud);
	if (temp_res[4] >= res[4]) {
	        result = GO_REG_invalid_data;
		goto out;
	}
	/* After the above check, any minimum reached will be NOT  at
	 * the start of c-range (c_offset - sign * c_accuracy) */
	c_start = c_offset;
	c_end = c_start - res[0] * c_range;
	c_dist = res[0] * (c_start - c_end) / 2;
	res[3] = c_end + res[0] * c_dist;
	do {
	    c_dist /= 2;
		SUFFIX(transform_x_and_linear_regression_log_fitting) (xs, transf_xs,
							       ys, n, res,
							       point_cloud);
		temp_res[3] = res[3] + res[0] * c_dist;
		SUFFIX(transform_x_and_linear_regression_log_fitting) (xs, transf_xs,
							       ys, n, temp_res,
							       point_cloud);
		if (temp_res[4] <= res[4])
			COPY_VECTOR (res, temp_res, 5);
		else {
		        temp_res[3] = res[3] - res[0] * c_dist;
			SUFFIX(transform_x_and_linear_regression_log_fitting) (xs,
							          transf_xs,
								  ys, n,
								  temp_res,
							          point_cloud);
			if (temp_res[4] <= res[4])
			        COPY_VECTOR (res, temp_res, 5);
		}
	} while (c_dist > c_accuracy);

	res[3] = c_accuracy * SUFFIX(go_fake_round) (res[3] / c_accuracy);
	SUFFIX(transform_x_and_linear_regression_log_fitting) (xs, transf_xs, ys, n,
						       res, point_cloud);

	if ((res[0] * (res[3] - c_end)) < (1.1 * c_accuracy)) {
	        /* Allowing for some inaccuracy, we are at the end of the
		 * range, so this is probably no local minimum.
		 * The start of the range has been checked above. */
	        result = GO_REG_invalid_data;
		goto out;
	}

 out:
	g_free (transf_xs);
	g_free (temp_res);
	return result;
}

/**
 * go_linear_regression:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors.
 * @ys: y-vector.  (Dependent data.)
 * @n: number of data points.
 * @affine: if true, a non-zero constant is allowed.
 * @res: output place for constant[0] and slope1[1], slope2[2],... There will be dim+1 results.
 * @stat_ : non-NULL storage for additional results.
 *
 * Performs multi-dimensional linear regressions on the input points.
 * Fits to "y = b + a1 * x1 + ... ad * xd".
 *
 * Returns: #GORegressionResult as above.
 **/
GORegressionResult
SUFFIX(go_linear_regression) (MATRIX xss, int dim,
			      const DOUBLE *ys, int n,
			      gboolean affine,
			      DOUBLE *res,
			      SUFFIX(go_regression_stat_t) *stat_)
{
	GORegressionResult result;

	g_return_val_if_fail (dim >= 1, GO_REG_invalid_dimensions);
	g_return_val_if_fail (n >= 1, GO_REG_invalid_dimensions);

	if (affine) {
		int i;
		DOUBLE **xss2 = g_new (DOUBLE *, dim + 1);

		xss2[0] = g_new (DOUBLE, n);
		for (i = 0; i < n; i++)
			xss2[0][i] = 1;
		memcpy (xss2 + 1, xss, dim * sizeof (DOUBLE *));

		result = SUFFIX(general_linear_regression)
			(xss2, dim + 1, ys, n,
			 res, stat_, affine);
		g_free (xss2[0]);
		g_free (xss2);
	} else {
		res[0] = 0;
		result = SUFFIX(general_linear_regression)
			(xss, dim, ys, n,
			 res + 1, stat_, affine);
	}
	return result;
}

/**
 * go_exponential_regression:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-one multiplier is allowed
 * @res: output place for constant[0] and root1[1], root2[2],... There will be dim+1 results.
 * @stat_ : non-NULL storage for additional results.
 *
 * Performs one-dimensional linear regressions on the input points.
 * Fits to "y = b * m1^x1 * ... * md^xd " or equivalently to
 * "log y = log b + x1 * log m1 + ... + xd * log md".
 *
 * Returns: #GORegressionResult as above.
 **/
GORegressionResult
SUFFIX(go_exponential_regression) (MATRIX xss, int dim,
				   const DOUBLE *ys, int n,
				   gboolean affine,
				   DOUBLE *res,
				   SUFFIX(go_regression_stat_t) *stat_)
{
	GORegressionResult result = SUFFIX(go_exponential_regression_as_log) (xss, dim, ys, n, affine, res, stat_);
	int i;

	if (result == GO_REG_ok || result == GO_REG_near_singular_good)
		for (i = 0; i < dim + 1; i++)
			res[i] = SUFFIX(exp) (res[i]);

	return result;
}

/**
 * go_exponential_regression_as_log:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-one multiplier is allowed
 * @res: output place for constant[0] and root1[1], root2[2],... There will be dim+1 results.
 * @stat_ : non-NULL storage for additional results.
 *
 * Performs one-dimensional linear regressions on the input points as 
 * go_exponential_regression, but returns the logarithm of the coefficients instead
 * or the coefficients themselves.
 * Fits to "y = b * exp (m1*x1) * ... * exp (md*xd) " or equivalently to
 * "ln y = ln b + x1 * m1 + ... + xd * md".
 *
 * Returns: #GORegressionResult as above.
 **/
GORegressionResult
SUFFIX(go_exponential_regression_as_log) (MATRIX xss, int dim,
				   const DOUBLE *ys, int n,
				   gboolean affine,
				   DOUBLE *res,
				   SUFFIX(go_regression_stat_t) *stat_)
{
	DOUBLE *log_ys;
	GORegressionResult result;
	int i;

	g_return_val_if_fail (dim >= 1, GO_REG_invalid_dimensions);
	g_return_val_if_fail (n >= 1, GO_REG_invalid_dimensions);

	log_ys = g_new (DOUBLE, n);
	for (i = 0; i < n; i++)
		if (ys[i] > 0)
			log_ys[i] = SUFFIX(log) (ys[i]);
		else {
			result = GO_REG_invalid_data;
			goto out;
		}

	if (affine) {
		int i;
		DOUBLE **xss2 = g_new (DOUBLE *, dim + 1);

		xss2[0] = g_new (DOUBLE, n);
		for (i = 0; i < n; i++)
			xss2[0][i] = 1;
		memcpy (xss2 + 1, xss, dim * sizeof (DOUBLE *));

		result = SUFFIX(general_linear_regression)
			(xss2, dim + 1, log_ys,
			 n, res, stat_, affine);
		g_free (xss2[0]);
		g_free (xss2);
	} else {
		res[0] = 0;
		result = SUFFIX(general_linear_regression)
			(xss, dim, log_ys, n,
			 res + 1, stat_, affine);
	}

 out:
	g_free (log_ys);
	return result;
}
 
/**
 * go_power_regression:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-one multiplier is allowed
 * @res: output place for constant[0] and root1[1], root2[2],... There will be dim+1 results.
 * @stat_ : non-NULL storage for additional results.
 *
 * Performs one-dimensional linear regressions on the input points.
 * Fits to "y = b * x1^m1 * ... * xd^md " or equivalently to
 * "log y = log b + m1 * log x1 + ... + md * log xd".
 *
 * Returns: #GORegressionResult as above.
 **/
GORegressionResult
SUFFIX(go_power_regression) (MATRIX xss, int dim,
			     const DOUBLE *ys, int n,
			     gboolean affine,
			     DOUBLE *res,
			     SUFFIX(go_regression_stat_t) *stat_)
{
	DOUBLE *log_ys, **log_xss = NULL;
	GORegressionResult result;
	int i, j;

	g_return_val_if_fail (dim >= 1, GO_REG_invalid_dimensions);
	g_return_val_if_fail (n >= 1, GO_REG_invalid_dimensions);

	log_ys = g_new (DOUBLE, n);
	for (i = 0; i < n; i++)
		if (ys[i] > 0)
			log_ys[i] = SUFFIX(log) (ys[i]);
		else {
			result = GO_REG_invalid_data;
			goto out;
		}

	ALLOC_MATRIX (log_xss, dim, n);
	for (i = 0; i < dim; i++)
	        for (j = 0; j < n; j++)
		        if (xss[i][j] > 0)
		                log_xss[i][j] = SUFFIX(log) (xss[i][j]);
			else {
			        result = GO_REG_invalid_data;
				goto out;
			}

	if (affine) {
		int i;
		DOUBLE **log_xss2 = g_new (DOUBLE *, dim + 1);

		log_xss2[0] = g_new (DOUBLE, n);
		for (i = 0; i < n; i++)
			log_xss2[0][i] = 1;
		memcpy (log_xss2 + 1, log_xss, dim * sizeof (DOUBLE *));

		result = SUFFIX(general_linear_regression)
			(log_xss2, dim + 1, log_ys,
			 n, res, stat_, affine);
		g_free (log_xss2[0]);
		g_free (log_xss2);
	} else {
		res[0] = 0;
		result = SUFFIX(general_linear_regression)
			(log_xss, dim, log_ys, n,
			 res + 1, stat_, affine);
	}

 out:
	if (log_xss != NULL)
		FREE_MATRIX (log_xss, dim, n);
	g_free (log_ys);
	return result;
}


/**
 * go_logarithmic_regression:
 * @xss: x-vectors (i.e. independent data)
 * @dim: number of x-vectors
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @affine: if %TRUE, a non-zero constant is allowed
 * @res: output place for constant[0] and factor1[1], factor2[2],... There will be dim+1 results.
 * @stat_ : non-NULL storage for additional results.
 *
 * This is almost a copy of linear_regression and produces multi-dimensional
 * linear regressions on the input points after transforming xss to ln(xss).
 * Fits to "y = b + a1 * z1 + ... ad * zd" with "zi = ln (xi)".
 * Problems with arrays in the calling function: see comment to
 * gnumeric_linest, which is also valid for gnumeric_logreg.
 *
 * (Errors: less than two points, all points on a vertical line, non-positive x data.)
 *
 * Returns: #GORegressionResult as above.
 **/
GORegressionResult
SUFFIX(go_logarithmic_regression) (MATRIX xss, int dim,
				   const DOUBLE *ys, int n,
				   gboolean affine,
				   DOUBLE *res,
				   SUFFIX(go_regression_stat_t) *stat_)
{
        MATRIX log_xss;
	GORegressionResult result;
	int i, j;

	g_return_val_if_fail (dim >= 1, GO_REG_invalid_dimensions);
	g_return_val_if_fail (n >= 1, GO_REG_invalid_dimensions);

	ALLOC_MATRIX (log_xss, dim, n);
	for (i = 0; i < dim; i++)
	        for (j = 0; j < n; j++)
		        if (xss[i][j] > 0)
		                log_xss[i][j] = SUFFIX(log) (xss[i][j]);
			else {
			        result = GO_REG_invalid_data;
				goto out;
			}


	if (affine) {
		int i;
		DOUBLE **log_xss2 = g_new (DOUBLE *, dim + 1);

		log_xss2[0] = g_new (DOUBLE, n);
		for (i = 0; i < n; i++)
			log_xss2[0][i] = 1;
		memcpy (log_xss2 + 1, log_xss, dim * sizeof (DOUBLE *));

		result = SUFFIX(general_linear_regression)
			(log_xss2, dim + 1, ys, n,
			 res, stat_, affine);
		g_free (log_xss2[0]);
		g_free (log_xss2);
	} else {
		res[0] = 0;
		result = SUFFIX(general_linear_regression)
			(log_xss, dim, ys, n,
			 res + 1, stat_, affine);
	}

 out:
	FREE_MATRIX (log_xss, dim, n);
	return result;
}

/**
 * go_logarithmic_fit:
 * @xs: x-vector (i.e. independent data)
 * @ys: y-vector (dependent data)
 * @n: number of data points
 * @res: output place for sign[0], a[1], b[2], c[3], and sum of squared residuals[4].
 *
 * Performs a two-dimensional non-linear fitting on the input points.
 * Fits to "y = a + b * ln (sign * (x - c))", with sign in {-1, +1}.
 * The graph is a logarithmic curve moved horizontally by c and possibly
 * mirrored across the y-axis (if sign = -1).
 *
 * Fits c (and sign) by iterative trials, but seems to be fast enough even
 * for automatic recomputation.
 *
 * Adapts c until a local minimum of squared residuals is reached. For each
 * new c tried out the corresponding a and b are calculated by linear
 * regression. If no local minimum is found, an error is returned. If there
 * is more than one local minimum, the one found is not necessarily the
 * smallest (i.e., there might be cases in which the returned fit is not the
 * best possible). If the shape of the point cloud is to different from
 * ``logarithmic'', either sign can not be determined (error returned) or no
 * local minimum will be found.
 *
 * (Requires: at least 3 different x values, at least 3 different y values.)
 *
 * Returns: #GORegressionResult as above.
 */

GORegressionResult
SUFFIX(go_logarithmic_fit) (DOUBLE *xs, const DOUBLE *ys, int n, DOUBLE *res)
{
	SUFFIX(point_cloud_measure_type) point_cloud_measures;
	int i, result;
	gboolean more_2_y = 0, more_2_x = 0;

	/* Store useful measures for using them here and in subfunctions.
	 * The checking of n is paranoid -- the calling function should
	 * have cared for that. */
	g_return_val_if_fail (n > 2, GO_REG_invalid_dimensions);
	result = SUFFIX(go_range_min) (xs, n, &(point_cloud_measures.min_x));
	result = SUFFIX(go_range_max) (xs, n, &(point_cloud_measures.max_x));
	result = SUFFIX(go_range_min) (ys, n, &(point_cloud_measures.min_y));
	result = SUFFIX(go_range_max) (ys, n, &(point_cloud_measures.max_y));
	result = SUFFIX(go_range_average) (ys, n, &(point_cloud_measures.mean_y));
	/* Checking of error conditions. */
	/* less than 2 different ys or less than 2 different xs */
	g_return_val_if_fail (((point_cloud_measures.min_y !=
				point_cloud_measures.max_y) &&
			       (point_cloud_measures.min_x !=
				point_cloud_measures.max_x)),
			      GO_REG_invalid_data);
	/* less than 3 different ys */
	for (i=0; i<n; i++) {
	        if ((ys[i] != point_cloud_measures.min_y) &&
		    (ys[i] != point_cloud_measures.max_y)) {
		        more_2_y = 1;
			break;
		}
	}
	g_return_val_if_fail (more_2_y, GO_REG_invalid_data);
	/* less than 3 different xs */
	for (i=0; i<n; i++) {
	        if ((xs[i] != point_cloud_measures.min_x) &&
		    (xs[i] != point_cloud_measures.max_x)) {
		        more_2_x = 1;
			break;
		}
	}
	g_return_val_if_fail (more_2_x, GO_REG_invalid_data);

	/* no errors */
	result = SUFFIX(log_fitting) (xs, ys, n, res, &point_cloud_measures);
	return result;
}

/* ------------------------------------------------------------------------- */

SUFFIX(go_regression_stat_t) *
SUFFIX(go_regression_stat_new) (void)
{
	SUFFIX(go_regression_stat_t) * stat_ =
		g_new0 (SUFFIX(go_regression_stat_t), 1);

	stat_->se = NULL;
	stat_->t = NULL;
	stat_->xbar = NULL;

	return stat_;
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_regression_stat_destroy) (SUFFIX(go_regression_stat_t) *stat_)
{
	if (stat_) {
		g_free(stat_->se);
		g_free(stat_->t);
		g_free(stat_->xbar);
		g_free (stat_);
	}
}

/* ------------------------------------------------------------------------- */

#ifdef DEFINE_COMMON
#define DELTA      0.01
/* FIXME:  I pulled this number out of my hat.
 * I need some testing to pick a sensible value.
 */
#define MAX_STEPS   200
#endif

/*
 * SYNOPSIS:
 *      result = derivative( f, &df, x, par, i)
 *
 * Approximates the partial derivative of a given function, at (x;params)
 * with respect to the parameter indicated by ith parameter.  The resulst
 * is stored in 'df'.
 *
 * See the header file for more information.
 */
static GORegressionResult
SUFFIX(derivative) (SUFFIX(GORegressionFunction) f,
	    DOUBLE *df,
	    DOUBLE *x, /* Only one point, not the whole data set. */
	    DOUBLE *par,
	    int index)
{
	DOUBLE y1, y2;
	GORegressionResult result;
	DOUBLE par_save = par[index];

	par[index] = par_save - DELTA;
	result = (*f) (x, par, &y1);
	if (result != GO_REG_ok) {
		par[index] = par_save;
		return result;
	}

	par[index] = par_save + DELTA;
	result = (*f) (x, par, &y2);
	if (result != GO_REG_ok) {
		par[index] = par_save;
		return result;
	}

#ifdef DEBUG
	g_printerr ("y1 = %lf\n", y1);
	g_printerr ("y2 = %lf\n", y2);
	g_printerr ("DELTA = %lf\n",DELTA);
#endif

	*df = (y2 - y1) / (2 * DELTA);
	par[index] = par_save;
	return GO_REG_ok;
}

/*
 * SYNOPSIS:
 *   result = chi_squared (f, xvals, par, yvals, sigmas, x_dim, &chisq)
 *
 *                            /  y  - f(x ; par) \ 2
 *            2              |    i      i        |
 *         Chi   ==   Sum (  | ------------------ |   )
 *                            \     sigma        /
 *                                       i
 *
 * sigmas  -> Measurement errors in the dataset (along the y-axis).
 *            NULL means "no errors available", so they are all set to 1.
 *
 * x_dim   -> Number of data points.
 *
 * This value is not very meaningful without the sigmas.  However, it is
 * still useful for the fit.
 */
static GORegressionResult
SUFFIX(chi_squared) (SUFFIX(GORegressionFunction) f,
		     CONSTMATRIX xvals,    /* The entire data set. */
		     DOUBLE *par,
		     DOUBLE *yvals,   /* Ditto. */
		     DOUBLE *sigmas,  /* Ditto. */
		     int x_dim,       /* Number of data points. */
		     DOUBLE *chisq)   /* Chi Squared */
{
	int i;
	GORegressionResult result;
	DOUBLE tmp, y;
	*chisq = 0;

	for (i = 0; i < x_dim; i++) {
		result = f (xvals[i], par, &y);
		if (result != GO_REG_ok)
			return result;

		tmp = (yvals[i] - y ) / (sigmas ? sigmas[i] : 1);

		*chisq += tmp * tmp;
	}

	return GO_REG_ok;
}


/*
 * SYNOPSIS:
 *      result = chi_derivative (f, &dchi, xvals, par, i, yvals,
 *                               sigmas, x_dim)
 *
 * This is a simple adaptation of the derivative() function specific to
 * the Chi Squared.
 */
static GORegressionResult
SUFFIX(chi_derivative) (SUFFIX(GORegressionFunction) f,
			DOUBLE *dchi,
			MATRIX xvals, /* The entire data set. */
			DOUBLE *par,
			int index,
			DOUBLE *yvals,  /* Ditto. */
			DOUBLE *sigmas, /* Ditto. */
			int x_dim)
{
	DOUBLE y1, y2;
	GORegressionResult result;
	DOUBLE par_save = par[index];

	par[index] = par_save - DELTA;
	result = SUFFIX(chi_squared) (f, xvals, par, yvals, sigmas, x_dim, &y1);
	if (result != GO_REG_ok) {
		par[index] = par_save;
		return result;
	}

	par[index] = par_save + DELTA;
	result = SUFFIX(chi_squared) (f, xvals, par, yvals, sigmas, x_dim, &y2);
	if (result != GO_REG_ok) {
                par[index] = par_save;
		return result;
	}

#ifdef DEBUG
	g_printerr ("y1 = %lf\n", y1);
	g_printerr ("y2 = %lf\n", y2);
	g_printerr ("DELTA = %lf\n", DELTA);
#endif

	*dchi = (y2 - y1) / (2 * DELTA);
	par[index] = par_save;
	return GO_REG_ok;
}

/*
 * SYNOPSIS:
 *   result = coefficient_matrix (A, f, xvals, par, yvals, sigmas,
 *                                x_dim, p_dim, r)
 *
 * RETURNS:
 *   The coefficient matrix of the LM method.
 *
 * DETAIS:
 *   The coefficient matrix matrix is defined by
 *
 *            N        1      df  df
 *     A   = Sum  ( -------   --  --  ( i == j ? 1 + r : 1 ) a)
 *      ij   k=1    sigma^2   dp  dp
 *                       k      i   j
 *
 * A      -> p_dim X p_dim coefficient matrix.  MUST ALREADY BE ALLOCATED.
 *
 * sigmas -> Measurement errors in the dataset (along the y-axis).
 *           NULL means "no errors available", so they are all set to 1.
 *
 * x_dim  -> Number of data points.
 *
 * p_dim  -> Number of parameters.
 *
 * r      -> Positive constant.  Its value is altered during the LM procedure.
 */

static GORegressionResult
SUFFIX(coefficient_matrix) (MATRIX A, /* Output matrix. */
			    SUFFIX(GORegressionFunction) f,
			    MATRIX xvals, /* The entire data set. */
			    DOUBLE *par,
			    DOUBLE *yvals,  /* Ditto. */
			    DOUBLE *sigmas, /* Ditto. */
			    int x_dim,      /* Number of data points. */
			    int p_dim,      /* Number of parameters.  */
			    DOUBLE r)
{
	int i, j, k;
	GORegressionResult result;
	DOUBLE df_i, df_j;
	DOUBLE sum, sigma;

	/* Notice that the matrix is symmetric.  */
	for (i = 0; i < p_dim; i++) {
		for (j = 0; j <= i; j++) {
			sum = 0;
			for (k = 0; k < x_dim; k++) {
				result = SUFFIX(derivative) (f, &df_i, xvals[k],
						     par, i);
				if (result != GO_REG_ok)
					return result;

				result = SUFFIX(derivative) (f, &df_j, xvals[k],
						     par, j);
				if (result != GO_REG_ok)
					return result;

				sigma = (sigmas ? sigmas[k] : 1);

				sum += (df_i * df_j) / (sigma * sigma) *
					(i == j ? 1 + r : 1) ;
			}
			A[i][j] = A[j][i] = sum;
		}
	}

	return GO_REG_ok;
}


/*
 * SYNOPSIS:
 *   result = parameter_errors (f, xvals, par, yvals, sigmas,
 *                              x_dim, p_dim, errors)
 *
 * Returns the errors associated with the parameters.
 * If an error is infinite, it is set to -1.
 *
 * sigmas  -> Measurement errors in the dataset (along the y-axis).
 *            NULL means "no errors available", so they are all set to 1.
 *
 * x_dim   -> Number of data points.
 *
 * p_dim   -> Number of parameters.
 *
 * errors  -> MUST ALREADY BE ALLOCATED.
 */

/* FIXME:  I am not happy with the behaviour with infinite errors.  */
static GORegressionResult
SUFFIX(parameter_errors) (SUFFIX(GORegressionFunction) f,
			  MATRIX xvals, /* The entire data set. */
			  DOUBLE *par,
			  DOUBLE *yvals,  /* Ditto. */
			  DOUBLE *sigmas, /* Ditto. */
			  int x_dim,      /* Number of data points. */
			  int p_dim,      /* Number of parameters.  */
			  DOUBLE *errors)
{
	GORegressionResult result;
	MATRIX A;
	int i;

	ALLOC_MATRIX (A, p_dim, p_dim);

	result = SUFFIX(coefficient_matrix) (A, f, xvals, par, yvals, sigmas,
				     x_dim, p_dim, 0);
	if (result == GO_REG_ok) {
		for (i = 0; i < p_dim; i++)
			/* FIXME: these were "[i][j]" which makes no sense.  */
			errors[i] = (A[i][i] != 0
				     ? 1 / SUFFIX(sqrt) (A[i][i])
				     : -1);
	}

	FREE_MATRIX (A, p_dim, p_dim);
	return result;
}


/*
 * SYNOPSIS:
 *   result = non_linear_regression (f, xvals, par, yvals, sigmas,
 *                                   x_dim, p_dim, &chi, errors)
 *
 * Returns the results of the non-linear regression from the given initial
 * values.
 * The resulting parameters are placed back into 'par'.
 *
 * PARAMETERS:
 *
 * sigmas  -> Measurement errors in the dataset (along the y-axis).
 *            NULL means "no errors available", so they are all set to 1.
 *
 * x_dim   -> Number of data points.
 *
 * p_dim   -> Number of parameters.
 *
 * errors  -> MUST ALREADY BE ALLOCATED.  These are the approximated standard
 *            deviation for each parameter.
 *
 * chi     -> Chi Squared of the final result.  This value is not very
 *            meaningful without the sigmas.
 */
GORegressionResult
SUFFIX(go_non_linear_regression) (SUFFIX(GORegressionFunction) f,
				  MATRIX xvals, /* The entire data set. */
				  DOUBLE *par,
				  DOUBLE *yvals,  /* Ditto. */
				  DOUBLE *sigmas, /* Ditto. */
				  int x_dim,      /* Number of data points. */
				  int p_dim,      /* Number of parameters.  */
				  DOUBLE *chi,
				  DOUBLE *errors)
{
	DOUBLE r = 0.001; /* Pick a conservative initial value. */
	DOUBLE *b, **A;
	DOUBLE *dpar;
	DOUBLE *tmp_par;
	DOUBLE chi_pre, chi_pos, dchi;
	GORegressionResult result;
	int i, count;

	result = SUFFIX(chi_squared) (f, xvals, par, yvals, sigmas, x_dim, &chi_pre);
	if (result != GO_REG_ok)
		return result;

	ALLOC_MATRIX (A, p_dim, p_dim);
	dpar    = g_new (DOUBLE, p_dim);
	tmp_par = g_new (DOUBLE, p_dim);
	b       = g_new (DOUBLE, p_dim);
#ifdef DEBUG
	g_printerr ("Chi Squared : %lf", chi_pre);
#endif

	for (count = 0; count < MAX_STEPS; count++) {
		for (i = 0; i < p_dim; i++) {
			/*
			 *          d Chi
			 *  b   ==  -----
			 *   k       d p
			 *              k
			 */
			result = SUFFIX(chi_derivative) (f, &dchi, xvals, par, i,
						 yvals, sigmas, x_dim);
			if (result != GO_REG_ok)
				goto out;

			b[i] = - dchi;
		}

		result = SUFFIX(coefficient_matrix) (A, f, xvals, par, yvals,
					     sigmas, x_dim, p_dim, r);
		if (result != GO_REG_ok)
			goto out;

		result = SUFFIX(go_linear_solve) (A, b, p_dim, dpar);
		if (result != GO_REG_ok)
			goto out;

		for(i = 0; i < p_dim; i++)
			tmp_par[i] = par[i] + dpar[i];

		result = SUFFIX(chi_squared) (f, xvals, tmp_par, yvals, sigmas,
				      x_dim, &chi_pos);
		if (result != GO_REG_ok)
			goto out;

#ifdef DEBUG
		g_printerr ("Chi Squared : %lf", chi_pre);
		g_printerr ("Chi Squared : %lf", chi_pos);
		g_printerr ("r  :  %lf", r);
#endif

		if (chi_pos <= chi_pre + DELTA / 2) {
			/* There is improvement */
			r /= 10;
			for(i = 0; i < p_dim; i++)
				par[i] = tmp_par[i];

			if (SUFFIX(fabs) (chi_pos - chi_pre) < DELTA)
				break;

			chi_pre = chi_pos;
		} else {
			r *= 10;
		}
	}

	result = SUFFIX(parameter_errors) (f, xvals, par, yvals, sigmas,
				   x_dim, p_dim, errors);
	if (result != GO_REG_ok)
		goto out;

	*chi = chi_pos;

 out:
	FREE_MATRIX (A, p_dim, p_dim);
	g_free (dpar);
	g_free (tmp_par);
	g_free (b);

	return result;
}
