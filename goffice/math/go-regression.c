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

// We need multiple versions of this code.  We're going to include ourself
// with different settings of various macros.  gdb will hate us.
#include <goffice/goffice-multipass.h>

#ifdef SKIP_THIS_PASS

// Stub for the benefit of gtk-doc
GType INFIX(go_regression_stat,_get_type) (void);
GType INFIX(go_regression_stat,_get_type) (void) { return 0; }

#else

/* ------------------------------------------------------------------------- */

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

#define DEFAULT_THRESHOLD (256 * DOUBLE_EPSILON)

// Boxed types code

static SUFFIX(go_regression_stat_t) *
SUFFIX(go_regression_stat_ref) (SUFFIX(go_regression_stat_t)* state)
{
	state->ref_count++;
	return state;
}

GType
INFIX(go_regression_stat,_get_type) (void)
{
	static GType t = 0;

	if (t == 0) {
		t = g_boxed_type_register_static (
		     "go_regression_stat_t" SUFFIX_STR,
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

#define PRINT_MATRIX(var,dim1,dim2)				\
  do {								\
	int _i, _j, _d1, _d2;					\
	_d1 = (dim1);						\
	_d2 = (dim2);						\
	for (_i = 0; _i < _d1; _i++) {				\
	  for (_j = 0; _j < _d2; _j++) {			\
		  g_printerr (" %12.8" FORMAT_g, (var)[_i][_j]); \
	  }							\
	  g_printerr ("\n");					\
	}							\
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

static SUFFIX(GOQuadMatrix) *
SUFFIX(quad_matrix_from_matrix) (CONSTMATRIX A, int m, int n, DOUBLE *scale)
{
	int i, j;
	SUFFIX(GOQuadMatrix) *qA = SUFFIX(go_quad_matrix_new) (m, n);

	for (i = 0; i < m; i++) {
		for (j = 0; j < n; j++) {
			DOUBLE x = scale ? A[i][j] / scale[j] : A[i][j];
			SUFFIX(go_quad_init) (&qA->data[i][j], x);
		}
	}

	return qA;
}

static void
SUFFIX(copy_quad_matrix_to_matrix) (MATRIX A, const SUFFIX(GOQuadMatrix) *qA)
{
	int i, j;

	for (i = 0; i < qA->m; i++)
		for (j = 0; j < qA->n; j++)
			A[i][j] = SUFFIX(go_quad_value) (&qA->data[i][j]);

}

/* ------------------------------------------------------------------------- */

#if 0
static void
SUFFIX(print_Q) (CONSTQMATRIX V, int m, int n)
{
	QMATRIX Q;
	int i, j;
	QUAD *x;

	ALLOC_MATRIX2 (Q, m, m, QUAD);
	x = g_new (QUAD, m);

	for (j = 0; j < m; j++) {
		for (i = 0; i < m; i++)
			SUFFIX(go_quad_init)(&x[i], i == j ? 1 : 0);

		SUFFIX(multiply_Qt)(x, V, m, n);
		for (i = 0; i < m; i++)
			Q[j][i] = x[i];
	}

	PRINT_QMATRIX (Q, m, m);

	FREE_MATRIX (Q, m, m);
	g_free (x);
}
#endif

static DOUBLE
SUFFIX(calc_residual) (CONSTMATRIX AT, const DOUBLE *b, int m, int n,
		       const DOUBLE *y)
{
	int i, j;
	QUAD N2 = SUFFIX(go_quad_zero);

	for (i = 0; i < m; i++) {
		QUAD d;

		SUFFIX(go_quad_init) (&d, b[i]);

		for (j = 0; j < n; j++) {
			QUAD e;
			SUFFIX(go_quad_mul12) (&e, AT[j][i], y[j]);
			SUFFIX(go_quad_sub) (&d, &d, &e);
		}

		SUFFIX(go_quad_mul) (&d, &d, &d);
		SUFFIX(go_quad_add) (&N2, &N2, &d);
	}

	return SUFFIX(go_quad_value) (&N2);
}

/* ------------------------------------------------------------------------- */

/*
 * Solve AX=B where A is n-times-n and B and X are both n-times-bn.
 * X is stored back into B.
 */

GORegressionResult
SUFFIX(go_linear_solve_multiple) (CONSTMATRIX A, MATRIX B, int n, int bn)
{
	void *state;
	GORegressionResult regres = GO_REG_ok;
	SUFFIX(GOQuadMatrix) *qA;
	const SUFFIX(GOQuadMatrix) *R;
	SUFFIX(GOQuadQR) *qr;
	QUAD *QTb, *x;
	int i, j;

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

	qA = SUFFIX(quad_matrix_from_matrix) (A, n, n, NULL);
	qr = SUFFIX(go_quad_qr_new) (qA);
	if (!qr) {
		regres = GO_REG_invalid_data;
		goto done;
	}
	R = SUFFIX(go_quad_qr_r) (qr);

	QTb = g_new (QUAD, n);
	x = g_new (QUAD, n);

	for (j = 0; j < bn; j++) {
		/* Compute Q^T b.  */
		for (i = 0; i < n; i++)
			SUFFIX(go_quad_init)(&QTb[i], B[i][j]);
		SUFFIX(go_quad_qr_multiply_qt)(qr, QTb);

		/* Solve R x = Q^T b  */
		if (SUFFIX(go_quad_matrix_back_solve) (R, x, QTb, FALSE))
			regres = GO_REG_singular;

		/* Round for output.  */
		for (i = 0; i < n; i++)
			B[i][j] = SUFFIX(go_quad_value) (&x[i]);
	}

	SUFFIX(go_quad_qr_free) (qr);
	g_free (x);
	g_free (QTb);
done:
	SUFFIX(go_quad_matrix_free) (qA);

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
	SUFFIX(GOQuadMatrix) *qA, *qZ;
	DOUBLE threshold = DEFAULT_THRESHOLD;
	void *state;
	gboolean ok;

	state = SUFFIX(go_quad_start) ();
	qA = SUFFIX(quad_matrix_from_matrix) (A, n, n, NULL);
	qZ = SUFFIX(go_quad_matrix_inverse) (qA, threshold);
	ok = (qZ != NULL);
	SUFFIX(go_quad_matrix_free) (qA);
	if (qZ) {
		SUFFIX(copy_quad_matrix_to_matrix) (A, qZ);
		SUFFIX(go_quad_matrix_free) (qZ);
	}
	SUFFIX(go_quad_end) (state);
	return ok;
}

DOUBLE
SUFFIX(go_matrix_determinant) (CONSTMATRIX A, int n)
{
	DOUBLE res;
	QUAD qres;
	void *state;
	SUFFIX(GOQuadMatrix) *qA;

	if (n < 1)
		return 0;

	state = SUFFIX(go_quad_start) ();

	qA = SUFFIX(quad_matrix_from_matrix) (A, n, n, NULL);
	SUFFIX(go_quad_matrix_determinant) (qA, &qres);
	SUFFIX(go_quad_matrix_free) (qA);
	res = SUFFIX(go_quad_value) (&qres);

	SUFFIX(go_quad_end) (state);

	return res;
}

static DOUBLE
SUFFIX(calc_scale) (const DOUBLE *xs, int m)
{
	DOUBLE M;
	int e;

	if (SUFFIX(go_range_maxabs) (xs, m, &M) || M == 0)
		return 1;

	/*
	 * Pick a scale such that the largest value will be in the
	 * range [1;RADIX[.  The scale will be a power of radix so
	 * scaling doesn't introduce rounding errors of it own.
	 */
	(void)UNSCALBN (M, &e);
	return SUFFIX(scalbn) (1.0, e - 1);
}

/* ------------------------------------------------------------------------- */

/* Note, that this function takes a transposed matrix xssT.  */
static GORegressionResult
SUFFIX(general_linear_regression) (CONSTMATRIX xssT, int n,
				   const DOUBLE *ys, int m,
				   DOUBLE *result,
				   SUFFIX(go_regression_stat_t) *stat_,
				   gboolean affine,
				   DOUBLE threshold)
{
	GORegressionResult regerr;
	SUFFIX(GOQuadMatrix) *xss;
	SUFFIX(GOQuadQR) *qr;
	int i, j, k;
	gboolean has_result;
	void *state;
	gboolean has_stat;
	int err;
	QUAD N2;
	DOUBLE *xscale;
	gboolean debug_scale = FALSE;

	ZERO_VECTOR (result, n);

	has_stat = (stat_ != NULL);
	if (!has_stat)
		stat_ = SUFFIX(go_regression_stat_new)();

	if (n > m) {
		regerr = GO_REG_not_enough_data;
		goto out;
	}

	state = SUFFIX(go_quad_start) ();

	xscale = g_new (DOUBLE, n);
	xss = SUFFIX(go_quad_matrix_new) (m, n);
	for (j = 0; j < n; j++) {
		xscale[j] = SUFFIX(calc_scale) (xssT[j], m);
		if (debug_scale)
			g_printerr ("Scale %d: %" FORMAT_g "\n", j, xscale[j]);
		for (i = 0; i < m; i++)
			SUFFIX(go_quad_init) (&xss->data[i][j], xssT[j][i] / xscale[j]);
	}
	qr = SUFFIX(go_quad_qr_new) (xss);
	SUFFIX(go_quad_matrix_free) (xss);

	has_result = (qr != NULL);

	if (has_result) {
		const SUFFIX(GOQuadMatrix) *R = SUFFIX(go_quad_qr_r) (qr);
		QUAD *qresult = g_new0 (QUAD, n);
		QUAD *QTy = g_new (QUAD, m);
		QUAD *inv = g_new (QUAD, n);
		DOUBLE emax;
		int df_resid = m - n;
		int df_reg = n - (affine ? 1 : 0);

		regerr = GO_REG_ok;

		SUFFIX(go_quad_matrix_eigen_range) (R, NULL, &emax);
		for (i = 0; i < n; i++) {
			DOUBLE ei = SUFFIX(go_quad_value)(&R->data[i][i]);
			gboolean degenerate =
				!(SUFFIX(fabs)(ei) >= emax * threshold);
			if (degenerate) {
				SUFFIX(go_quad_qr_mark_degenerate) (qr, i);
				df_resid++;
				df_reg--;
			}
		}

		/* Compute Q^T ys.  */
		for (i = 0; i < m; i++)
			SUFFIX(go_quad_init)(&QTy[i], ys[i]);
		SUFFIX(go_quad_qr_multiply_qt)(qr, QTy);

		if (0)
			SUFFIX(go_quad_matrix_dump) (R, "%10.5" FORMAT_g);

		/* Solve R res = Q^T ys */
		if (SUFFIX(go_quad_matrix_back_solve) (R, qresult, QTy, TRUE))
			has_result = FALSE;

		for (i = 0; i < n; i++)
			result[i] = SUFFIX(go_quad_value) (&qresult[i]) / xscale[i];

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

		;
		stat_->ss_resid =
			SUFFIX(calc_residual) (xssT, ys, m, n, result);

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
			(df_resid * stat_->ss_total);
		if (df_resid == 0)
			N2 = SUFFIX(go_quad_zero);
		else {
			QUAD d;
			SUFFIX(go_quad_init) (&d, df_resid);
			SUFFIX(go_quad_init) (&N2, stat_->ss_resid);
			SUFFIX(go_quad_div) (&N2, &N2, &d);
		}
		stat_->var = SUFFIX(go_quad_value) (&N2);

		stat_->se = g_new0 (DOUBLE, n);
		for (k = 0; k < n; k++) {
			QUAD p, N;

			/* inv = e_k */
			for (i = 0; i < n; i++)
				SUFFIX(go_quad_init) (&inv[i], i == k ? 1 : 0);

			/* Solve R^T inv = e_k */
			if (SUFFIX(go_quad_matrix_fwd_solve) (R, inv, inv, TRUE)) {
				regerr = GO_REG_singular;
				break;
			}

			SUFFIX(go_quad_dot_product) (&N, inv, inv, n);
			SUFFIX(go_quad_mul) (&p, &N2, &N);
			SUFFIX(go_quad_sqrt) (&p, &p);
			stat_->se[k] = SUFFIX(go_quad_value) (&p) / xscale[k];
		}

		stat_->t = g_new (DOUBLE, n);

		for (i = 0; i < n; i++)
			stat_->t[i] = (stat_->se[i] == 0)
				? SUFFIX(go_pinf)
				: result[i] / stat_->se[i];

		stat_->df_resid = df_resid;
		stat_->df_reg = df_reg;
		stat_->df_total = stat_->df_resid + df_reg;

		stat_->F = (stat_->sqr_r == 1)
			? SUFFIX(go_pinf)
			: ((stat_->sqr_r / df_reg) /
			   (1 - stat_->sqr_r) * stat_->df_resid);

		stat_->ss_reg =  stat_->ss_total - stat_->ss_resid;
		stat_->se_y = SUFFIX(sqrt) (stat_->ss_total / m);
		stat_->ms_reg = (df_reg == 0)
			? 0
			: stat_->ss_reg / stat_->df_reg;
		stat_->ms_resid = (df_resid == 0)
			? 0
			: stat_->ss_resid / df_resid;

		g_free (QTy);
		g_free (qresult);
		g_free (inv);
	} else
		regerr = GO_REG_invalid_data;

	SUFFIX(go_quad_end) (state);

	if (qr) SUFFIX(go_quad_qr_free) (qr);
	g_free (xscale);
out:
	if (!has_stat)
		SUFFIX(go_regression_stat_destroy) (stat_);

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
	c_accuracy *= (DOUBLE)GO_LOGFIT_C_ACCURACY;

	/* Determine sign. Take a c which is ``much to small'' since the part
	 * of the curve cutting the point cloud is almost not bent.
	 * If making c still smaller does not make things still worse,
	 * assume that we have to change the direction of curve bending
	 * by changing sign.
	 */
	c_step = x_range * (DOUBLE)GO_LOGFIT_C_STEP_FACTOR;
	c_range = x_range * (DOUBLE)GO_LOGFIT_C_RANGE_FACTOR;
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

	if ((res[0] * (res[3] - c_end)) < (CONST(1.1) * c_accuracy)) {
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
 * @res: (out): place for constant[0] and slope1[1], slope2[2],... There will be dim+1 results.
 * @stat_: (out): storage for additional results.
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
	DOUBLE threshold = DEFAULT_THRESHOLD;

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
			 res, stat_, affine, threshold);
		g_free (xss2[0]);
		g_free (xss2);
	} else {
		res[0] = 0;
		result = SUFFIX(general_linear_regression)
			(xss, dim, ys, n,
			 res + 1, stat_, affine, threshold);
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
 * @stat_: (out) (optional): storage for additional results.
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
 * @stat_: (out) (optional): storage for additional results.
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
	DOUBLE threshold = DEFAULT_THRESHOLD;

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
			 n, res, stat_, affine, threshold);
		g_free (xss2[0]);
		g_free (xss2);
	} else {
		res[0] = 0;
		result = SUFFIX(general_linear_regression)
			(xss, dim, log_ys, n,
			 res + 1, stat_, affine, threshold);
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
 * @stat_: (out) (optional): storage for additional results.
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
	DOUBLE threshold = DEFAULT_THRESHOLD;

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
			 n, res, stat_, affine, threshold);
		g_free (log_xss2[0]);
		g_free (log_xss2);
	} else {
		res[0] = 0;
		result = SUFFIX(general_linear_regression)
			(log_xss, dim, log_ys, n,
			 res + 1, stat_, affine, threshold);
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
 * @stat_: (out) (optional): storage for additional results.
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
	DOUBLE threshold = DEFAULT_THRESHOLD;

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
			 res, stat_, affine, threshold);
		g_free (log_xss2[0]);
		g_free (log_xss2);
	} else {
		res[0] = 0;
		result = SUFFIX(general_linear_regression)
			(log_xss, dim, ys, n,
			 res + 1, stat_, affine, threshold);
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
#define DELTA      CONST(0.01)
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
 *   The coefficient matrix is defined by
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
	SUFFIX(GOQuadMatrix) *qA;
	SUFFIX(GOQuadQR) *qr;
	void *state = SUFFIX(go_quad_start) ();
	GORegressionResult regres;
	DOUBLE threshold = DEFAULT_THRESHOLD;
	DOUBLE *xscale, *Aj;
	int i, j;

	/*
	 * Leverages are independing of column scaling.
	 */
	xscale = g_new (DOUBLE, n);
	Aj = g_new (DOUBLE, m);
	for (j = 0; j < n; j++) {
		for (i = 0; i < m; i++)
			Aj[i] = A[i][j];
		xscale[j] = SUFFIX(calc_scale) (Aj, m);
	}
	g_free (Aj);

	qA = SUFFIX(quad_matrix_from_matrix) (A, m, n, xscale);
	qr = SUFFIX(go_quad_qr_new) (qA);
	if (qr) {
		int k;
		QUAD *b = g_new (QUAD, n);
		const SUFFIX(GOQuadMatrix) *R = SUFFIX(go_quad_qr_r) (qr);
		DOUBLE emin, emax;

		SUFFIX(go_quad_matrix_eigen_range) (R, &emin, &emax);
		regres = (emin > emax * threshold)
			? GO_REG_ok
			: GO_REG_singular;

		for (k = 0; k < m; k++) {
			int i;
			QUAD acc = SUFFIX(go_quad_zero);

			/* b = AT e_k  */
			for (i = 0; i < n; i++)
				b[i] = qA->data[k][i];

			/* Solve R^T b = AT e_k */
			if (SUFFIX(go_quad_matrix_fwd_solve) (R, b, b, FALSE)) {
				regres = GO_REG_singular;
				break;
			}

			/* Solve R newb = b */
			if (SUFFIX(go_quad_matrix_back_solve) (R, b, b, FALSE)) {
				regres = GO_REG_singular;
				break;
			}

			/* acc = (Ab)_k */
			for (i = 0; i < n; i++) {
				QUAD p;
				SUFFIX(go_quad_mul) (&p, &qA->data[k][i], &b[i]);
				SUFFIX(go_quad_add) (&acc, &acc, &p);
			}

			d[k] = SUFFIX(go_quad_value) (&acc);
		}

		g_free (b);
		SUFFIX(go_quad_qr_free) (qr);
	} else
		regres = GO_REG_invalid_data;

	SUFFIX(go_quad_matrix_free) (qA);
	g_free (xscale);

	SUFFIX(go_quad_end) (state);

	return regres;
}

/*
 * Compute the pseudo-inverse of a matrix, see
 *     http://en.wikipedia.org/wiki/Moore%E2%80%93Penrose_pseudoinverse
 *
 * Think of this as A+ = (AT A)^-1 AT, except that the pseudo-inverse
 * is defined even if AT*A isn't invertible.
 *
 * Properties:
 *
 *     A  A+ A  = A
 *     A+ A  A+ = A+
 *     (A A+)^T = A A+
 *     (A+ A)^T = A+ A
 *
 * assuming threshold==0 and no rounding errors.
 */

void
SUFFIX(go_matrix_pseudo_inverse) (CONSTMATRIX A, int m, int n,
				  DOUBLE threshold,
				  MATRIX B)
{
	SUFFIX(GOQuadMatrix) *qA, *qZ;
	void *state;

	state = SUFFIX(go_quad_start) ();
	qA = SUFFIX(quad_matrix_from_matrix) (A, m, n, NULL);
	qZ = SUFFIX(go_quad_matrix_pseudo_inverse) (qA, threshold);
	SUFFIX(go_quad_matrix_free) (qA);
	if (qZ) {
		SUFFIX(copy_quad_matrix_to_matrix) (B, qZ);
		SUFFIX(go_quad_matrix_free) (qZ);
	}
	SUFFIX(go_quad_end) (state);
}

/* ------------------------------------------------------------------------- */

// See comments at top
#endif // SKIP_THIS_PASS
#if INCLUDE_PASS < INCLUDE_PASS_LAST
#include __FILE__
#endif
