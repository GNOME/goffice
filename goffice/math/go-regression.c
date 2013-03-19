/*
 * regression.c:  Statistical regression functions.
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
 *   Andrew Chatham <andrew.chatham@duke.edu>
 *   Daniel Carrera <dcarrera@math.toronto.edu>
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
 */

#include <goffice/goffice-config.h>
#include "go-regression.h"
#include "go-rangefunc.h"
#include "go-math.h"

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * GORegressionResult:
 * @GO_REG_ok: success.
 * @GO_REG_invalid_dimensions: invalid dimensions.
 * @GO_REG_invalid_data: invalid data:
 * @GO_REG_not_enough_data: not enough data.
 * @GO_REG_near_singular_good: probably good result.
 * @GO_REG_near_singular_bad: probably bad result.
 * @GO_REG_singular: singularity found.
 **/

/**
 * go_regression_stat_t:
 * @se: SE for each parameter estimator.
 * @t: t values for each parameter estimator.
 * @sqr_r: squared R.
 * @adj_sqr_r:
 * @se_y: the Standard Error of Y.
 * @F:
 * @df_reg:
 * @df_resid:
 * @df_total:
 * @ss_reg:
 * @ss_resid:
 * @ss_total:
 * @ms_reg:
 * @ms_resid:
 * @ybar:
 * @xbar:
 * @var: the variance of the entire regression: sum(errors^2)/(n-xdim).
 **/

/**
 * go_regression_stat_tl:
 * @se: SE for each parameter estimator.
 * @t: t values for each parameter estimator.
 * @sqr_r: squared R.
 * @adj_sqr_r:
 * @se_y: the Standard Error of Y.
 * @F:
 * @df_reg:
 * @df_resid:
 * @df_total:
 * @ss_reg:
 * @ss_resid:
 * @ss_total:
 * @ms_reg:
 * @ms_resid:
 * @ybar:
 * @xbar:
 * @var: the variance of the entire regression: sum(errors^2)/(n-xdim).
 **/

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
general_linear_regressionl (long double *const *const xssT, int n,
			    const long double *ys, int m,
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
#else
/* It appears that gtk-doc is too dumb to handle this file.  Provide
   a dummy type getter to make things work.  */
GType go_regression_statl_get_type (void);
GType go_regression_statl_get_type (void) { return G_TYPE_NONE; }
#endif

#endif

/* Boxed types code */

static SUFFIX(go_regression_stat_t) *
SUFFIX(go_regression_stat_ref) (SUFFIX(go_regression_stat_t)* state)
{
	state->ref_count++;
	return state;
}

GType
#ifdef DEFINE_COMMON
go_regression_stat_get_type (void)
#else
go_regression_statl_get_type (void)
#endif
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static (
#ifdef DEFINE_COMMON
		     "go_regression_stat_t",
#else
		     "go_regression_stat_tl",
#endif
			 (GBoxedCopyFunc)SUFFIX(go_regression_stat_ref),
			 (GBoxedFreeFunc)SUFFIX(go_regression_stat_destroy));
	}
	return t;
}

/* ------------------------------------------------------------------------- */

#ifdef DEFINE_COMMON

#define MATRIX DOUBLE **
#define CONSTMATRIX DOUBLE *const *const
#define QUAD SUFFIX(GOQuad)
#define QMATRIX QUAD **
#define CONSTQMATRIX QUAD *const *const

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

#define PRINT_QMATRIX(var,dim1,dim2)				\
  do {								\
	int _i, _j, _d1, _d2;					\
	_d1 = (dim1);						\
	_d2 = (dim2);						\
	for (_i = 0; _i < _d1; _i++) {				\
	  for (_j = 0; _j < _d2; _j++) {			\
		  g_printerr (" %12.8" FORMAT_g, SUFFIX(go_quad_value) (&(var)[_i][_j])); \
	  }							\
	  g_printerr ("\n");					\
	}							\
  } while (0)


#endif

/*
 *       ---> j
 *
 *  |    ********
 *  |    ********
 *  |    ********        A[i][j]
 *  v    ********
 *       ********
 *  i    ********
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
	    const go_regression_stat_tl *s2, int n)
{
	if (!s2)
		return;

	g_free (s1->se);
	g_free (s1->t);
	g_free (s1->xbar);

	s1->se = shrink_vector (s2->se, n);
	s1->t = shrink_vector (s2->t, n);
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
	s1->xbar = shrink_vector (s2->xbar, n);
	s1->var = s2->var;
}
#endif

/* ------------------------------------------------------------------------- */

/*
 * QR decomposition of a matrix using Householder matrices.
 *
 * A (input) is an m-times-n matrix.  A[0...m-1][0..n-1]
 * If qAT is TRUE, this parameter is transposed.
 *
 * V is a pre-allocated output m-times-n matrix.  V will contrain
 * n vectors of different lengths: n, n-1, ..., 1.  These are the
 * Householder vectors (or null for the degenerate case).
 *
 * Rfinal is a pre-allocated output matrix of size n-times-n.
 *
 * qdet is an optional output location for det(Q).
 *
 * Note: the output is V and R, not Q and R.
 */
static gboolean
SUFFIX(QRH) (CONSTMATRIX A, gboolean qAT,
	     QMATRIX V, QMATRIX Rfinal, int m, int n,
	     int *qdet)
{
	QMATRIX R = NULL;
	int i, j, k;
	QUAD *tmp = g_new (QUAD, n);
	gboolean ok = TRUE;

	if (m < n || n < 1) {
		ok = FALSE;
		goto out;
	}

	ALLOC_MATRIX2 (R, m, n, QUAD);
	for (i = 0; i < m; i++) {
		for (j = 0; j < n; j++) {
			DOUBLE Aij = qAT ? A[j][i] : A[i][j];
			if (!SUFFIX(go_finite)(Aij)) {
				ok = FALSE;
				goto out;
			}
			SUFFIX(go_quad_init) (&R[i][j], Aij);
			SUFFIX(go_quad_init) (&V[i][j], -42);
		}
	}

	if (qdet)
		*qdet = 1;

	for (k = 0; k < n; k++) {
		QUAD L, L2, L2p, s;

		SUFFIX(go_quad_init) (&L2, 0);
		L2p = L2;
		for (i = m - 1; i >= k; i--) {
			V[i][k] = R[i][k];
			SUFFIX(go_quad_mul)(&s, &V[i][k], &V[i][k]);
			L2p = L2;
			SUFFIX(go_quad_add)(&L2, &L2, &s);
		}
		SUFFIX(go_quad_sqrt)(&L, &L2);

		(SUFFIX(go_quad_value)(&V[k][k]) < 0
		 ? SUFFIX(go_quad_sub)
		 : SUFFIX(go_quad_add)) (&V[k][k], &V[k][k], &L);

		/* Normalize v[k] to length 1.  */
		SUFFIX(go_quad_mul)(&s, &V[k][k], &V[k][k]);
		SUFFIX(go_quad_add)(&L2p, &L2p, &s);
		SUFFIX(go_quad_sqrt)(&L, &L2p);
		if (SUFFIX(go_quad_value)(&L) == 0) {
			/* This will be an identity so no determinant sign */
			continue;
		}
		for (i = k; i < m; i++)
			SUFFIX(go_quad_div)(&V[i][k], &V[i][k], &L);

		/* Householder matrices have determinant -1.  */
		if (qdet)
			*qdet = -*qdet;

		/* Calculate tmp = v[k]^t * R[k:m,k:n] */
		for (j = k; j < n; j++) {
			SUFFIX(go_quad_init) (&tmp[j], 0);
			for (i = k ; i < m; i++) {
				QUAD p;
				SUFFIX(go_quad_mul) (&p, &V[i][k], &R[i][j]);
				SUFFIX(go_quad_add) (&tmp[j], &tmp[j], &p);
			}
		}

		/* R[k:m,k:n] -= v[k] * tmp */
		for (j = k; j < n; j++) {
			for (i = k; i < m; i++) {
				QUAD p;
				SUFFIX(go_quad_mul) (&p, &V[i][k], &tmp[j]);
				SUFFIX(go_quad_add) (&p, &p, &p);
				SUFFIX(go_quad_sub) (&R[i][j], &R[i][j], &p);
			}
		}

		/* Explicitly zero what should become zero.  */
		for (i = k + 1; i < m; i++)
			SUFFIX(go_quad_init) (&R[i][k], 0);

#if 0
		g_printerr ("After round %d:\n", k);
		g_printerr ("R:\n");
		PRINT_QMATRIX(R, m ,n );
		g_printerr ("\n");
		g_printerr ("V:\n");
		PRINT_QMATRIX(V, m ,n );
		g_printerr ("\n\n");
#endif
	}

	for (i = 0; i < n /* Only n */; i++)
		for (j = 0; j < n; j++)
			Rfinal[i][j] = R[i][j];

out:
	if (R)
		FREE_MATRIX (R, m, n);
	g_free (tmp);

	return ok;
}

static void
SUFFIX(multiply_Qt) (QUAD *x, CONSTQMATRIX V, int m, int n)
{
	int i, k;

	for (k = 0; k < n; k++) {
		QUAD s;
		SUFFIX(go_quad_init)(&s, 0);
		for (i = k; i < m; i++) {
			QUAD p;
			SUFFIX(go_quad_mul) (&p, &x[i], &V[i][k]);
			SUFFIX(go_quad_add) (&s, &s, &p);
		}
		SUFFIX(go_quad_add) (&s, &s, &s);
		for (i = k; i < m; i++) {
			QUAD p;
			SUFFIX(go_quad_mul) (&p, &s, &V[i][k]);
			SUFFIX(go_quad_sub) (&x[i], &x[i], &p);
		}
	}
}

/*
 * Solve Rx=b.
 *
 * Return TRUE in case of error.
 */
static gboolean
SUFFIX(back_solve) (CONSTQMATRIX R, QUAD *x, const QUAD *b, int n)
{
	int i, j;

	for (i = n - 1; i >= 0; i--) {
		QUAD d = b[i];
		for (j = i + 1; j < n; j++) {
			QUAD p;
			SUFFIX(go_quad_mul) (&p, &R[i][j], &x[j]);
			SUFFIX(go_quad_sub) (&d, &d, &p);
		}
		if (SUFFIX(go_quad_value)(&R[i][i]) == 0) {
			while (i >= 0) SUFFIX(go_quad_init) (&x[i--], 0);
			return TRUE;
		}
		SUFFIX(go_quad_div) (&x[i], &d, &R[i][i]);
	}

	return FALSE;
}

/*
 * Solve RTx=b.
 *
 * Return TRUE in case of error.
 */
static gboolean
SUFFIX(fwd_solve) (CONSTQMATRIX R, QUAD *x, const QUAD *b, int n)
{
	int i, j;

	for (i = 0; i < n; i++) {
		QUAD d = b[i];
		for (j = 0; j < i; j++) {
			QUAD p;
			SUFFIX(go_quad_mul) (&p, &R[j][i], &x[j]);
			SUFFIX(go_quad_sub) (&d, &d, &p);
		}
		if (SUFFIX(go_quad_value)(&R[i][i]) == 0) {
			while (i < n) SUFFIX(go_quad_init) (&x[i], 0);
			return TRUE;
		}
		SUFFIX(go_quad_div) (&x[i], &d, &R[i][i]);
	}

	return FALSE;
}

static GORegressionResult
SUFFIX(regres_from_condition) (CONSTQMATRIX R, int n)
{
	DOUBLE emin = go_pinf;
	DOUBLE emax = go_ninf;
	DOUBLE c, lc;
	int i;

	/*
	 * R is triangular, so all the eigenvalues are in the diagonal.
	 * We need the absolute largest and smallest.
	 */
	for (i = 0; i < n; i++) {
		DOUBLE e = SUFFIX(fabs)(SUFFIX(go_quad_value)(&R[i][i]));
		if (e < emin) emin = e;
		if (e > emax) emax = e;
	}

	if (emin == 0)
		return GO_REG_singular;

	/* Condition number */
	c = emax / emin;

	/* Number of bits destroyed by matrix.  Since the calculations are
	   done with QUADs, we can afford to lose a lot.  */
	lc = SUFFIX(log)(c) / SUFFIX(log)(FLT_RADIX);

#if 1
	if (lc > 0.95* DOUBLE_MANT_DIG)
		return GO_REG_near_singular_bad;
	if (lc > 0.75 * DOUBLE_MANT_DIG)
		return GO_REG_near_singular_good;
#endif

	return GO_REG_ok;
}

#ifndef BOUNCE

static void
SUFFIX(calc_residual) (CONSTMATRIX AT, const DOUBLE *b, int m, int n,
		       const QUAD *y, QUAD *residual, QUAD *N2)
{
	int i, j;

	SUFFIX(go_quad_init) (N2, 0);

	for (i = 0; i < m; i++) {
		QUAD d;

		SUFFIX(go_quad_init) (&d, b[i]);

		for (j = 0; j < n; j++) {
			QUAD Aij, e;
			SUFFIX(go_quad_init) (&Aij, AT[j][i]);
			SUFFIX(go_quad_mul) (&e, &Aij, &y[j]);
			SUFFIX(go_quad_sub) (&d, &d, &e);
		}

		if (residual)
			residual[i] = d;

		SUFFIX(go_quad_mul) (&d, &d, &d);
		SUFFIX(go_quad_add) (N2, N2, &d);
	}
}

static void
SUFFIX(refine) (CONSTMATRIX AT, const DOUBLE *b, int m, int n,
		CONSTQMATRIX V, CONSTQMATRIX R, QUAD *result)
{
	QUAD *residual = g_new (QUAD, m);
	QUAD *newresidual = g_new (QUAD, m);
	QUAD *QTres = g_new (QUAD, m);
	QUAD *delta = g_new (QUAD, n);
	QUAD *newresult = g_new (QUAD, n);
	int pass;
	QUAD best_N2;

	SUFFIX(calc_residual) (AT, b, m, n, result, residual, &best_N2);
#ifdef DEBUG_REFINEMENT
	g_printerr ("-: Residual norm = %20.15" FORMAT_g "\n",
		    SUFFIX(go_quad_value) (&best_N2));
#endif

	for (pass = 1; pass < 10; pass++) {
		int i;
		QUAD dres, N2;

		/* Compute Q^T residual.  */
		for (i = 0; i < m; i++)
			QTres[i] = residual[i];
		SUFFIX(multiply_Qt)(QTres, V, m, n);

		/* Solve R delta = Q^T residual */
		if (SUFFIX(back_solve) (R, delta, QTres, n))
			break;

		/* newresult = result + delta */
		for (i = 0; i < n; i++) {
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

		SUFFIX(calc_residual) (AT, b, m, n, newresult, newresidual, &N2);
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
		COPY_VECTOR (result, newresult, n);
		COPY_VECTOR (residual, newresidual, m);
	}

	g_free (residual);
	g_free (newresidual);
	g_free (QTres);
	g_free (newresult);
	g_free (delta);
}

#endif

/* ------------------------------------------------------------------------- */

/*
 * Solve AX=B where A is n-times-n and B and X are both n-times-bn.
 * X is stored back into B.
 */

GORegressionResult
SUFFIX(go_linear_solve_multiple) (CONSTMATRIX A, MATRIX B, int n, int bn)
{
	QMATRIX V;
	QMATRIX R;
	void *state;
	GORegressionResult regres;
	gboolean ok;

	if (n < 1 || bn < 1)
		return GO_REG_not_enough_data;

	/* Special case.  */
	if (n == 1) {
		int i;
		DOUBLE d = A[0][0];
		if (d == 0)
			return GO_REG_singular;

		for (i = 0; i < bn; i++)
			B[0][i] /= d;
		return GO_REG_ok;
	}

	state = SUFFIX(go_quad_start) ();

	ALLOC_MATRIX2 (V, n, n, QUAD);
	ALLOC_MATRIX2 (R, n, n, QUAD);
	ok = SUFFIX(QRH)(A, FALSE, V, R, n, n, NULL);
	if (ok) {
		QUAD *QTb = g_new (QUAD, n);
		QUAD *x = g_new (QUAD, n);
		int i, j;

		regres = SUFFIX(regres_from_condition)(R, n);

		for (j = 0; j < bn; j++) {
			/* Compute Q^T b.  */
			for (i = 0; i < n; i++)
				SUFFIX(go_quad_init)(&QTb[i], B[i][j]);
			SUFFIX(multiply_Qt)(QTb, V, n, n);

			/* Solve R x = Q^T b  */
			if (SUFFIX(back_solve) (R, x, QTb, n))
				regres = GO_REG_singular;

			/* Round for output.  */
			for (i = 0; i < n; i++)
				B[i][j] = SUFFIX(go_quad_value) (&x[i]);
		}

		g_free (x);
		g_free (QTb);
	} else
		regres = GO_REG_invalid_data;

	SUFFIX(go_quad_end) (state);

	return regres;
}

GORegressionResult
SUFFIX(go_linear_solve) (CONSTMATRIX A, const DOUBLE *b, int n, DOUBLE *res)
{
	MATRIX B;
	int i;
	GORegressionResult regres;

	if (n < 1)
		return GO_REG_not_enough_data;

	ALLOC_MATRIX (B, n, 1);
	for (i = 0; i < n; i++)
		B[i][0] = b[i];

	regres = SUFFIX(go_linear_solve_multiple) (A, B, n, 1);

	for (i = 0; i < n; i++)
		res[i] = B[i][0];

	FREE_MATRIX (B, n, 1);

	return regres;
}


gboolean
SUFFIX(go_matrix_invert) (MATRIX A, int n)
{
	QMATRIX V;
	QMATRIX R;
	gboolean has_result;
	void *state = SUFFIX(go_quad_start) ();

	ALLOC_MATRIX2 (V, n, n, QUAD);
	ALLOC_MATRIX2 (R, n, n, QUAD);

	has_result = SUFFIX(QRH)(A, FALSE, V, R, n, n, NULL);

	if (has_result) {
		int i, k;
		QUAD *x = g_new (QUAD, n);
		QUAD *QTk = g_new (QUAD, n);
		GORegressionResult regres =
			SUFFIX(regres_from_condition) (R, n);

		if (regres != GO_REG_ok && regres != GO_REG_near_singular_good)
			has_result = FALSE;

		for (k = 0; has_result && k < n; k++) {
			/* Compute Q^T's k-th column.  */
			for (i = 0; i < n; i++)
				SUFFIX(go_quad_init)(&QTk[i], i == k);
			SUFFIX(multiply_Qt)(QTk, V, n, n);

			/* Solve R x = Q^T e_k */
			if (SUFFIX(back_solve) (R, x, QTk, n))
				has_result = FALSE;

			/* Round for output.  */
			for (i = 0; i < n; i++)
				A[i][k] = SUFFIX(go_quad_value) (&x[i]);
		}

		g_free (QTk);
		g_free (x);
	}

	FREE_MATRIX (V, n, n);
	FREE_MATRIX (R, n, n);

	SUFFIX(go_quad_end) (state);

	return has_result;
}

DOUBLE
SUFFIX(go_matrix_determinant) (CONSTMATRIX A, int n)
{
	gboolean ok;
	QMATRIX R;
	QMATRIX V;
	int qdet;
	DOUBLE res;
	void *state;

	if (n < 1)
		return 0;

	/* Special case.  */
	if (n == 1)
		return A[0][0];

	state = SUFFIX(go_quad_start) ();

	/* Special case.  */
	if (n == 2) {
		QUAD a, b, det;
		SUFFIX(go_quad_mul12)(&a, A[0][0], A[1][1]);
		SUFFIX(go_quad_mul12)(&b, A[1][0], A[0][1]);
		SUFFIX(go_quad_sub)(&det, &a, &b);
		res = SUFFIX(go_quad_value)(&det);
		goto done;
	}

	ALLOC_MATRIX2 (V, n, n, QUAD);
	ALLOC_MATRIX2 (R, n, n, QUAD);
	ok = SUFFIX(QRH)(A, FALSE, V, R, n, n, &qdet);
	if (ok) {
		int i;
		QUAD det;
		SUFFIX(go_quad_init)(&det, qdet);
		for (i = 0; i < n; i++)
			SUFFIX(go_quad_mul)(&det, &det, &R[i][i]);
		res = SUFFIX(go_quad_value)(&det);
	} else
		res = go_nan;

	FREE_MATRIX (V, n, n);
	FREE_MATRIX (R, n, n);

done:
	SUFFIX(go_quad_end) (state);

	return res;
}

/* ------------------------------------------------------------------------- */
/* Note, that this function takes a transposed matrix xssT.  */

static GORegressionResult
SUFFIX(general_linear_regression) (CONSTMATRIX xssT, int n,
				   const DOUBLE *ys, int m,
				   DOUBLE *result,
				   SUFFIX(go_regression_stat_t) *stat_,
				   gboolean affine)
{
	GORegressionResult regerr;
#ifdef BOUNCE
	long double **xssT2, *ys2, *result2;
	go_regression_stat_tl *stat2;

	ALLOC_MATRIX2 (xssT2, n, m, long double);
	COPY_MATRIX (xssT2, xssT, n, m);

	ys2 = g_new (long double, m);
	COPY_VECTOR (ys2, ys, m);

	result2 = g_new (long double, n);
	stat2 = stat_ ? go_regression_stat_newl () : NULL;

	regerr = general_linear_regressionl (xssT2, n, ys2, m,
					     result2, stat2, affine);
	COPY_VECTOR (result, result2, n);
	copy_stats (stat_, stat2, n);

	go_regression_stat_destroyl (stat2);
	g_free (result2);
	g_free (ys2);
	FREE_MATRIX (xssT2, n, m);
#else
	QMATRIX V;
	QMATRIX R;
	QUAD *qresult;
	QUAD *QTy;
	QUAD *inv;
	int i, k;
	gboolean has_result;
	void *state;
	gboolean has_stat;
	int err;
	QUAD N2;

	ZERO_VECTOR (result, n);

	has_stat = (stat_ != NULL);
	if (!has_stat)
		stat_ = SUFFIX(go_regression_stat_new)();

	if (n > m) {
		regerr = GO_REG_not_enough_data;
		goto out;
	}

	state = SUFFIX(go_quad_start) ();

	ALLOC_MATRIX2 (V, m, n, QUAD);
	ALLOC_MATRIX2 (R, n, n, QUAD);
	qresult = g_new0 (QUAD, n);
	QTy = g_new (QUAD, m);
	inv = g_new (QUAD, n);

	has_result = SUFFIX(QRH) (xssT, TRUE, V, R, m, n, NULL);

	if (has_result) {
		regerr = GO_REG_ok;

		/* Compute Q^T ys.  */
		for (i = 0; i < m; i++)
			SUFFIX(go_quad_init)(&QTy[i], ys[i]);
		SUFFIX(multiply_Qt)(QTy, V, m, n);

		/* Solve R res = Q^T ys */
		if (SUFFIX(back_solve) (R, qresult, QTy, n))
			has_result = FALSE;

		if (n > 2)
			SUFFIX(refine) (xssT, ys, m, n, V, R, qresult);

		/* Round to plain precision.  */
		for (i = 0; i < n; i++) {
			result[i] = SUFFIX(go_quad_value) (&qresult[i]);
			SUFFIX(go_quad_init) (&qresult[i], result[i]);
		}

		/* This should not fail since n >= 1.  */
		err = SUFFIX(go_range_average) (ys, m, &stat_->ybar);
		g_assert (err == 0);

		/* FIXME: we ought to have a devsq variant that does not
		   recompute the mean.  */
		if (affine)
			err = SUFFIX(go_range_devsq) (ys, m, &stat_->ss_total);
		else
			err = SUFFIX(go_range_sumsq) (ys, m, &stat_->ss_total);
		g_assert (err == 0);

		stat_->xbar = g_new (DOUBLE, n);
		for (i = 0; i < n; i++) {
			int err = SUFFIX(go_range_average) (xssT[i], m, &stat_->xbar[i]);
			g_assert (err == 0);
		}

		SUFFIX(calc_residual) (xssT, ys, m, n, qresult, NULL, &N2);
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
		stat_->adj_sqr_r = 1 - stat_->ss_resid * (m - 1) /
			((m - n) * stat_->ss_total);
		if (m == n)
			SUFFIX(go_quad_init) (&N2, 0);
		else {
			QUAD d;
			SUFFIX(go_quad_init) (&d, m - n);
			SUFFIX(go_quad_div) (&N2, &N2, &d);
		}
		stat_->var = SUFFIX(go_quad_value) (&N2);

		stat_->se = g_new0 (DOUBLE, n);
		for (k = 0; k < n; k++) {
			QUAD p;

			/* inv = e_k */
			for (i = 0; i < n; i++)
				SUFFIX(go_quad_init) (&inv[i], i == k ? 1 : 0);

			/* Solve R^T inv = e_k */
			if (SUFFIX(fwd_solve) (R, inv, inv, n)) {
				regerr = GO_REG_singular;
				break;
			}

			/* Solve R newinv = inv */
			if (SUFFIX(back_solve) (R, inv, inv, n)) {
				regerr = GO_REG_singular;
				break;
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
				break;
			}

			SUFFIX(go_quad_mul) (&p, &N2, &inv[k]);
			SUFFIX(go_quad_sqrt) (&p, &p);
			stat_->se[k] = SUFFIX(go_quad_value) (&p);
		}

		stat_->t = g_new (DOUBLE, n);

		for (i = 0; i < n; i++)
			stat_->t[i] = (stat_->se[i] == 0)
				? SUFFIX(go_pinf)
				: result[i] / stat_->se[i];

		stat_->df_resid = m - n;
		stat_->df_reg = n - (affine ? 1 : 0);
		stat_->df_total = stat_->df_resid + stat_->df_reg;

		stat_->F = (stat_->sqr_r == 1)
			? SUFFIX(go_pinf)
			: ((stat_->sqr_r / stat_->df_reg) /
			   (1 - stat_->sqr_r) * stat_->df_resid);

		stat_->ss_reg =  stat_->ss_total - stat_->ss_resid;
		stat_->se_y = SUFFIX(sqrt) (stat_->ss_total / m);
		stat_->ms_reg = (stat_->df_reg == 0)
			? 0
			: stat_->ss_reg / stat_->df_reg;
		stat_->ms_resid = (stat_->df_resid == 0)
			? 0
			: stat_->ss_resid / stat_->df_resid;
	} else
		regerr = GO_REG_invalid_data;

	SUFFIX(go_quad_end) (state);

	g_free (QTy);
	g_free (qresult);
	g_free (inv);
	FREE_MATRIX (V, m, n);
	FREE_MATRIX (R, n, n);
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
 * @stat_: non-NULL storage for additional results.
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
 * @stat_: non-NULL storage for additional results.
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
 * @stat_: non-NULL storage for additional results.
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
 * @stat_: non-NULL storage for additional results.
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
 * @stat_: non-NULL storage for additional results.
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
	stat_->ref_count = 1;

	return stat_;
}

/* ------------------------------------------------------------------------- */

void
SUFFIX(go_regression_stat_destroy) (SUFFIX(go_regression_stat_t) *stat_)
{
	if (!stat_)
		return;

	g_return_if_fail (stat_->ref_count > 0);

	stat_->ref_count--;
	if (stat_->ref_count > 0)
		return;

	g_free (stat_->se);
	g_free (stat_->t);
	g_free (stat_->xbar);
	g_free (stat_);
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

/**
 * go_non_linear_regression:
 * @f: (scope call): the model function
 * @xvals: independent values.
 * @par: model parameters.
 * @yvals: dependent values.
 * @sigmas: stahdard deviations for the dependent values.
 * @x_dim: Number of data points.
 * @p_dim: Number of parameters.
 * @chi: Chi Squared of the final result.  This value is not very
 * meaningful without the sigmas.
 * @errors: MUST ALREADY BE ALLOCATED.  These are the approximated standard
 * deviation for each parameter.
 *
 * SYNOPSIS:
 *   result = non_linear_regression (f, xvals, par, yvals, sigmas,
 *                                   x_dim, p_dim, &chi, errors)
 * Non linear regression.
 * Returns: the results of the non-linear regression from the given initial
 * values.
 * The resulting parameters are placed back into @par.
 **/
/**
 * go_non_linear_regressionl:
 * @f: (scope call): the model function
 * @xvals: independent values.
 * @par: model parameters.
 * @yvals: dependent values.
 * @sigmas: stahdard deviations for the dependent values.
 * @x_dim: Number of data points.
 * @p_dim: Number of parameters.
 * @chi: Chi Squared of the final result.  This value is not very
 * meaningful without the sigmas.
 * @errors: MUST ALREADY BE ALLOCATED.  These are the approximated standard
 * deviation for each parameter.
 *
 * SYNOPSIS:
 *   result = non_linear_regression (f, xvals, par, yvals, sigmas,
 *                                   x_dim, p_dim, &chi, errors)
 * Non linear regression.
 * Returns: the results of the non-linear regression from the given initial
 * values.
 * The resulting parameters are placed back into @par.
 **/
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

/*
 * Compute the diagonal of A (AT A)^-1 AT
 *
 * A is full-rank m-times-n.
 * d is output for m-element diagonal.
 */

GORegressionResult
SUFFIX(go_linear_regression_leverage) (MATRIX A, DOUBLE *d, int m, int n)
{
	QMATRIX V;
	QMATRIX R;
	gboolean has_result;
	void *state = SUFFIX(go_quad_start) ();
	GORegressionResult regres;

	ALLOC_MATRIX2 (V, m, n, QUAD);
	ALLOC_MATRIX2 (R, n, n, QUAD);

	has_result = SUFFIX(QRH)(A, FALSE, V, R, m, n, NULL);

	if (has_result) {
		int k;
		QUAD *b = g_new (QUAD, n);

		regres = SUFFIX(regres_from_condition) (R, n);

		for (k = 0; k < m; k++) {
			int i;
			QUAD acc;

			/* b = AT e_k  */
			for (i = 0; i < n; i++)
				SUFFIX(go_quad_init) (&b[i], A[k][i]);

			/* Solve R^T newb = b */
			if (SUFFIX(fwd_solve) (R, b, b, n)) {
				regres = GO_REG_singular;
				break;
			}

			/* Solve R newb = b */
			if (SUFFIX(back_solve) (R, b, b, n)) {
				regres = GO_REG_singular;
				break;
			}

			/* acc = (Ab)_k */
			SUFFIX(go_quad_init) (&acc, 0);
			for (i = 0; i < n; i++) {
				QUAD p;
				SUFFIX(go_quad_init) (&p, A[k][i]);
				SUFFIX(go_quad_mul) (&p, &p, &b[i]);
				SUFFIX(go_quad_add) (&acc, &acc, &p);
			}

			d[k] = SUFFIX(go_quad_value) (&acc);
		}

		g_free (b);
	} else
		regres = GO_REG_invalid_data;

	FREE_MATRIX (V, m, n);
	FREE_MATRIX (R, n, n);

	SUFFIX(go_quad_end) (state);

	return regres;
}
