/*
 * go-matrix.c: Matrix routines.
 *
 * Authors:
 *   Morten Welinder <terra@gnome.org>
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
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <math.h>

// We need multiple versions of this code.  We're going to include ourself
// with different settings of various macros.  gdb will hate us.
#include <goffice/goffice-multipass.h>
#ifndef SKIP_THIS_PASS

#define QUAD SUFFIX(GOQuad)
#define QQR SUFFIX(GOQuadQR)
#define QMATRIX SUFFIX(GOQuadMatrix)

struct INFIX(GOQuadQR,_) {
	QMATRIX *V;
	QMATRIX *R;
	int qdet;
};


/**
 * go_quad_matrix_new: (skip)
 * @m: number of rows
 * @n: number of columns
 *
 * Returns: a new zero matrix.
 **/
QMATRIX *
SUFFIX(go_quad_matrix_new) (int m, int n)
{
	QMATRIX *res;
	int i;

	g_return_val_if_fail (m >= 1, NULL);
	g_return_val_if_fail (n >= 1, NULL);

	res = g_new (QMATRIX, 1);
	res->m = m;
	res->n = n;
	res->data = g_new (QUAD *, m);

	for (i = 0; i < m; i++)
		res->data[i] = g_new0 (QUAD, n);

	return res;
}

void
SUFFIX(go_quad_matrix_free) (QMATRIX *A)
{
	int i;

	for (i = 0; i < A->m; i++)
		g_free (A->data[i]);
	g_free (A->data);
	g_free (A);
}


/**
 * go_quad_matrix_dup: (skip)
 * @A: Matrix to duplicate
 *
 * Returns: a new matrix.
 **/
QMATRIX *
SUFFIX(go_quad_matrix_dup) (const QMATRIX *A)
{
	QMATRIX *res;

	g_return_val_if_fail (A != NULL, NULL);
	res = SUFFIX(go_quad_matrix_new) (A->m, A->n);
	SUFFIX(go_quad_matrix_copy) (res, A);
	return res;
}

/**
 * go_quad_matrix_copy:
 * @A: (out): Destination matrix.
 * @B: (transfer none): Source matrix.
 *
 * Copies B to A.
 **/
void
SUFFIX(go_quad_matrix_copy) (QMATRIX *A, const QMATRIX *B)
{
	int i, j;

	g_return_if_fail (A != NULL);
	g_return_if_fail (B != NULL);
	g_return_if_fail (A->m == B->m && A->n == B->n);

	if (A == B)
		return;

	for (i = 0; i < A->m; i++)
		for (j = 0; j < A->n; j++)
			A->data[i][j] = B->data[i][j];
}

/**
 * go_quad_matrix_transpose:
 * @A: (out): Destination matrix.
 * @B: (transfer none): Source matrix.
 *
 * Transposes B into A.
 **/
void
SUFFIX(go_quad_matrix_transpose) (QMATRIX *A, const QMATRIX *B)
{
	int i, j;

	g_return_if_fail (A != NULL);
	g_return_if_fail (B != NULL);
	g_return_if_fail (A->m == B->n && A->n == B->m);

	if (A == B)
		return;

	for (i = 0; i < A->m; i++)
		for (j = 0; j < A->n; j++)
			A->data[i][j] = B->data[j][i];
}


/**
 * go_quad_matrix_multiply:
 * @C: (out): Destination matrix.
 * @A: Source matrix.
 * @B: Source matrix.
 *
 * Multiplies A*B and stores the result in C.
 **/
void
SUFFIX(go_quad_matrix_multiply) (QMATRIX *C,
				 const QMATRIX *A,
				 const QMATRIX *B)
{
	int i, j, k;

	g_return_if_fail (C != NULL);
	g_return_if_fail (A != NULL);
	g_return_if_fail (B != NULL);
	g_return_if_fail (C->m == A->m && A->n == B->m && B->n == C->n);
	g_return_if_fail (C != A && C != B);

	for (i = 0; i < C->m; i++) {
		for (j = 0; j < C->n; j++) {
			QUAD p, acc = SUFFIX(go_quad_zero);
			for (k = 0; k < A->n; k++) {
				SUFFIX(go_quad_mul) (&p,
						     &A->data[i][k],
						     &B->data[k][j]);
				SUFFIX(go_quad_add) (&acc, &acc, &p);
			}
			C->data[i][j] = acc;
		}
	}
}

/**
 * go_quad_matrix_inverse: (skip)
 * @A: Source matrix.
 * @threshold: condition number threshold.
 *
 * Returns: The inverse matrix of A.  If any eigenvalues divided by the largest
 * eigenvalue is less than or equal to the given threshold, %NULL is returned
 * indicating a matrix that cannot be inverted.  (Note: this doesn't actually
 * use the eigenvalues of A, but of A after an orthogonal transformation.)
 **/
QMATRIX *
SUFFIX(go_quad_matrix_inverse) (const QMATRIX *A, DOUBLE threshold)
{
	QQR *qr;
	int i, k, n;
	QMATRIX *Z;
	const QMATRIX *R;
	gboolean ok;
	QUAD *x, *QTk;
	DOUBLE emin, emax;

	g_return_val_if_fail (A != NULL, NULL);
	g_return_val_if_fail (A->m == A->n, NULL);
	g_return_val_if_fail (threshold >= 0, NULL);

	qr = SUFFIX(go_quad_qr_new) (A);
	if (!qr)
		return NULL;

	n = A->n;
	Z = SUFFIX(go_quad_matrix_new) (n, n);
	x = g_new (QUAD, n);
	QTk = g_new (QUAD, n);

	R = SUFFIX(go_quad_qr_r) (qr);
	SUFFIX(go_quad_matrix_eigen_range) (R, &emin, &emax);
	ok = (emin > emax * threshold);

	for (k = 0; ok && k < n; k++) {
		/* Compute Q^T's k-th column.  */
		for (i = 0; i < n; i++)
			SUFFIX(go_quad_init)(&QTk[i], i == k);
		SUFFIX(go_quad_qr_multiply_qt) (qr, QTk);

		/* Solve R x = Q^T e_k */
		if (SUFFIX(go_quad_matrix_back_solve) (R, x, QTk, FALSE)) {
			ok = FALSE;
			break;
		}

		for (i = 0; i < n; i++)
			Z->data[i][k] = x[i];
	}

	SUFFIX(go_quad_qr_free) (qr);
	g_free (QTk);
	g_free (x);

	if (!ok) {
		SUFFIX(go_quad_matrix_free) (Z);
		return NULL;
	}

	return Z;
}

void
SUFFIX(go_quad_matrix_determinant) (const QMATRIX *A, QUAD *res)
{
	QQR *qr;

	g_return_if_fail (A != NULL);
	g_return_if_fail (A->m == A->n);
	g_return_if_fail (res != NULL);

	if (A->m == 1) {
		*res = A->data[0][0];
		return;
	}

	if (A->m == 2) {
		QUAD a, b;
		SUFFIX(go_quad_mul)(&a, &A->data[0][0], &A->data[1][1]);
		SUFFIX(go_quad_mul)(&b, &A->data[1][0], &A->data[0][1]);
		SUFFIX(go_quad_sub)(res, &a, &b);
		return;
	}

	qr = SUFFIX(go_quad_qr_new) (A);
	if (!qr) {
		/* Hmm... */
		SUFFIX(go_quad_init) (res, SUFFIX(go_nan));
		return;
	}

	SUFFIX(go_quad_qr_determinant) (qr, res);
	SUFFIX(go_quad_qr_free) (qr);
}

/**
 * go_quad_matrix_pseudo_inverse: (skip)
 * @A: An arbitrary matrix.
 * @threshold: condition number threshold.
 *
 * Returns: @A's pseudo-inverse.
 **/
QMATRIX *
SUFFIX(go_quad_matrix_pseudo_inverse) (const QMATRIX *A, DOUBLE threshold)
{
	int i, j, m, n;
	QQR *qr;
	const QMATRIX *R;
	QMATRIX *RT;
	QMATRIX *RTR;
	QMATRIX *B0;
	QMATRIX *Bi;
	QMATRIX *B;
	DOUBLE emax;
	gboolean full_rank;
	QUAD delta;
	int steps;
	QUAD *x;

	g_return_val_if_fail (A != NULL, NULL);
	g_return_val_if_fail (threshold >= 0, NULL);

	m = A->m;
	n = A->n;
	B = SUFFIX(go_quad_matrix_new) (n, m);

	if (m < n) {
		/*
		 * The main code assumes m >= n.  Luckily, taking the
		 * pseudo-inverse commutes with transposition.
		 */

		QMATRIX *AT = SUFFIX(go_quad_matrix_new) (n, m);
		QMATRIX *BT;

		SUFFIX(go_quad_matrix_transpose) (AT, A);
		BT = SUFFIX(go_quad_matrix_pseudo_inverse) (AT, threshold);
		SUFFIX(go_quad_matrix_transpose) (B, BT);

		SUFFIX (go_quad_matrix_free) (AT);
		SUFFIX (go_quad_matrix_free) (BT);
		return B;
	}

	qr = SUFFIX(go_quad_qr_new) (A);
	if (!qr)
		goto out;
	R = SUFFIX(go_quad_qr_r) (qr);

	SUFFIX(go_quad_matrix_eigen_range) (R, NULL, &emax);
	if (emax == 0)
		goto out;

	full_rank = TRUE;
	for (i = 0; i < n; i++) {
		DOUBLE abs_e = SUFFIX(fabs) (SUFFIX(go_quad_value)(&R->data[i][i]));
		if (abs_e <= emax * threshold) {
			full_rank = FALSE;
			R->data[i][i] = SUFFIX(go_quad_zero);
		}
	}

	SUFFIX(go_quad_init) (&delta, full_rank ? 0 : emax * threshold);

	/*
	 * Starting point for the iteration:
	 *
	 *    Bi := (RT R + delta I)^-1 * RT
	 *
	 * (RT R) is positive semi-definite and (delta I) is positive definite,
	 * so the sum is positive definite and invertible.
	 */
	RT = SUFFIX(go_quad_matrix_new) (n, n);
	SUFFIX(go_quad_matrix_transpose) (RT, R);
	RTR = SUFFIX(go_quad_matrix_new) (n, n);
	SUFFIX(go_quad_matrix_multiply) (RTR, RT, R);
	for (i = 0; i < n; i++)
		SUFFIX(go_quad_add) (&RTR->data[i][i],
				     &RTR->data[i][i],
				     &delta);
	B0 = SUFFIX(go_quad_matrix_inverse) (RTR, 0.0);
	Bi = SUFFIX(go_quad_matrix_new) (n, n);
	SUFFIX(go_quad_matrix_multiply) (Bi, B0, RT);
	SUFFIX(go_quad_matrix_free) (B0);
	SUFFIX(go_quad_matrix_free) (RTR);
	SUFFIX(go_quad_matrix_free) (RT);

	/* B_{i+1} = (2 I - B_i R) B_i */
	for (steps = 0; steps < 10; steps++) {
		QMATRIX *W = SUFFIX(go_quad_matrix_new) (n, n);
		QMATRIX *Bip1 = SUFFIX(go_quad_matrix_new) (n, n);
		QUAD two;

		SUFFIX(go_quad_init)(&two, 2);

		SUFFIX(go_quad_matrix_multiply) (W, Bi, R);
		for (i = 0; i < n; i++)
			for (j = 0; j < n; j++)
				SUFFIX(go_quad_sub) (&W->data[i][j],
						     i == j ? &two : &SUFFIX(go_quad_zero),
						     &W->data[i][j]);
		SUFFIX(go_quad_matrix_multiply) (Bip1, W, Bi);
		SUFFIX(go_quad_matrix_copy) (Bi, Bip1);

		SUFFIX(go_quad_matrix_free) (Bip1);
		SUFFIX(go_quad_matrix_free) (W);
	}

	/* B := (Bi|O) Q^T */
	x = g_new (QUAD, m);
	for (j = 0; j < m; j++) {
		/* Compute Q^T e_j.  */
		for (i = 0; i < m; i++)
			SUFFIX(go_quad_init)(&x[i], i == j ? 1 : 0);
		SUFFIX(go_quad_qr_multiply_qt) (qr, x);

		for (i = 0; i < n; i++) {
			int k;

			B->data[i][j] = SUFFIX(go_quad_zero);
			for (k = 0; k < n /* Only n */; k++) {
				QUAD p;
				SUFFIX(go_quad_mul) (&p, &Bi->data[i][k], &x[k]);
				SUFFIX(go_quad_add) (&B->data[i][j], &B->data[i][j], &p);
			}
		}
	}
	g_free (x);
	SUFFIX(go_quad_matrix_free) (Bi);

out:
	SUFFIX(go_quad_qr_free) (qr);

	return B;
}

/**
 * go_quad_matrix_fwd_solve:
 * @R: An upper triangular matrix.
 * @x: (out): Result vector.
 * @b: Input vector.
 * @allow_degenerate: If %TRUE, then degenerate dimensions are ignored other
 * than being given a zero result.  A degenerate dimension is one whose
 * diagonal entry is zero.
 *
 * Returns: %TRUE on error.
 *
 * This function solves the triangular system RT*x=b.
 **/
gboolean
SUFFIX(go_quad_matrix_fwd_solve) (const QMATRIX *R, QUAD *x, const QUAD *b,
				  gboolean allow_degenerate)
{
	int i, j, n;

	g_return_val_if_fail (R != NULL, TRUE);
	g_return_val_if_fail (R->m == R->n, TRUE);
	g_return_val_if_fail (x != NULL, TRUE);
	g_return_val_if_fail (b != NULL, TRUE);

	n = R->m;

	for (i = 0; i < n; i++) {
		QUAD d = b[i];
		QUAD Rii = R->data[i][i];

		if (SUFFIX(go_quad_value)(&Rii) == 0) {
			if (allow_degenerate) {
				x[i] = SUFFIX(go_quad_zero);
				continue;
			} else {
				while (i < n)
					x[i++] = SUFFIX(go_quad_zero);
				return TRUE;
			}
		}

		for (j = 0; j < i; j++) {
			QUAD p;
			SUFFIX(go_quad_mul) (&p, &R->data[j][i], &x[j]);
			SUFFIX(go_quad_sub) (&d, &d, &p);
		}

		SUFFIX(go_quad_div) (&x[i], &d, &Rii);
	}

	return FALSE;
}

/**
 * go_quad_matrix_back_solve:
 * @R: An upper triangular matrix.
 * @x: (out): Result vector.
 * @b: Input vector.
 * @allow_degenerate: If %TRUE, then degenerate dimensions are ignored other
 * than being given a zero result.  A degenerate dimension is one whose
 * diagonal entry is zero.
 *
 * Returns: %TRUE on error.
 *
 * This function solves the triangular system R*x=b.
 **/
gboolean
SUFFIX(go_quad_matrix_back_solve) (const QMATRIX *R, QUAD *x, const QUAD *b,
				   gboolean allow_degenerate)
{
	int i, j, n;

	g_return_val_if_fail (R != NULL, TRUE);
	g_return_val_if_fail (R->m == R->n, TRUE);
	g_return_val_if_fail (x != NULL, TRUE);
	g_return_val_if_fail (b != NULL, TRUE);

	n = R->m;

	for (i = n - 1; i >= 0; i--) {
		QUAD d = b[i];
		QUAD Rii = R->data[i][i];

		if (SUFFIX(go_quad_value)(&Rii) == 0) {
			if (allow_degenerate) {
				x[i] = SUFFIX(go_quad_zero);
				continue;
			} else {
				while (i >= 0)
					x[i--] = SUFFIX(go_quad_zero);
				return TRUE;
			}
		}

		for (j = i + 1; j < n; j++) {
			QUAD p;
			SUFFIX(go_quad_mul) (&p, &R->data[i][j], &x[j]);
			SUFFIX(go_quad_sub) (&d, &d, &p);
		}

		SUFFIX(go_quad_div) (&x[i], &d, &Rii);
	}

	return FALSE;
}

/**
 * go_quad_matrix_eigen_range:
 * @A: Triangular matrix.
 * @emin: (out): Smallest absolute eigen value.
 * @emax: (out): Largest absolute eigen value.
 **/
void
SUFFIX(go_quad_matrix_eigen_range) (const QMATRIX *A,
				    DOUBLE *emin, DOUBLE *emax)
{
	int i;
	DOUBLE abs_e;

	g_return_if_fail (A != NULL);
	g_return_if_fail (A->m == A->n);

	abs_e = SUFFIX(fabs) (SUFFIX(go_quad_value) (&A->data[0][0]));
	if (emin) *emin = abs_e;
	if (emax) *emax = abs_e;
	for (i = 1; i < A->m; i++) {
		abs_e = SUFFIX(fabs) (SUFFIX(go_quad_value) (&A->data[i][i]));
		if (emin) *emin = MIN (abs_e, *emin);
		if (emax) *emax = MAX (abs_e, *emax);
	}
}


void
SUFFIX(go_quad_matrix_dump) (const QMATRIX *A, const char *fmt)
{
	int i, j;

	for (i = 0; i < A->m; i++) {
		for (j = 0; j < A->n; j++) {
			DOUBLE x = SUFFIX(go_quad_value) (&A->data[i][j]);
			g_printerr (fmt, x);
		}
		g_printerr ("\n");
	}
}

/* -------------------------------------------------------------------------- */

/**
 * go_quad_qr_new: (skip)
 * @A: Source matrix.
 *
 * QR decomposition of a matrix using Householder matrices.
 *
 * A (input) is an m-times-n matrix.  A[0...m-1][0..n-1]
 * If qAT is TRUE, this parameter is transposed.
 *
 * V is a pre-allocated output m-times-n matrix.  V will contrain
 * n vectors of different lengths: n, n-1, ..., 1.  These are the
 * Householder vectors (or null for the degenerate case).  The
 * matrix Q of size m-times-m is implied from V.
 *
 * R is a matrix of size n-times-n.  (To get the m-times-n version
 * of R, simply add m-n null rows.)
 * Returns: (transfer full): a new #GOQuadQR.
 **/
QQR *
SUFFIX(go_quad_qr_new) (const QMATRIX *A)
{
	QQR *qr;
	int qdet = 1;
	QMATRIX *R;
	QMATRIX *V;
	int i, j, k, m, n;
	QUAD *tmp;

	g_return_val_if_fail (A != NULL, NULL);
	g_return_val_if_fail (A->m >= A->n, NULL);

	m = A->m;
	n = A->n;

	qr = g_new (QQR, 1);
	V = qr->V = SUFFIX(go_quad_matrix_new) (m, n);
	qr->R = SUFFIX(go_quad_matrix_new) (n, n);

	/* Temporary m-by-n version of R.  */
	R = SUFFIX(go_quad_matrix_dup) (A);

	tmp = g_new (QUAD, n);

	for (k = 0; k < n; k++) {
		QUAD L, L2 = SUFFIX(go_quad_zero), L2p = L2, s;

		for (i = m - 1; i >= k; i--) {
			V->data[i][k] = R->data[i][k];
			SUFFIX(go_quad_mul)(&s, &V->data[i][k], &V->data[i][k]);
			L2p = L2;
			SUFFIX(go_quad_add)(&L2, &L2, &s);
		}
		SUFFIX(go_quad_sqrt)(&L, &L2);

		(SUFFIX(go_quad_value)(&V->data[k][k]) < 0
		 ? SUFFIX(go_quad_sub)
		 : SUFFIX(go_quad_add)) (&V->data[k][k], &V->data[k][k], &L);

		/* Normalize v[k] to length 1.  */
		SUFFIX(go_quad_mul)(&s, &V->data[k][k], &V->data[k][k]);
		SUFFIX(go_quad_add)(&L2p, &L2p, &s);
		SUFFIX(go_quad_sqrt)(&L, &L2p);
		if (SUFFIX(go_quad_value)(&L) == 0) {
			/* This will be an identity so no determinant sign */
			continue;
		}
		for (i = k; i < m; i++)
			SUFFIX(go_quad_div)(&V->data[i][k], &V->data[i][k], &L);

		/* Householder matrices have determinant -1.  */
		qdet = -qdet;

		/* Calculate tmp = v[k]^t * R[k:m,k:n] */
		for (j = k; j < n; j++) {
			tmp[j] = SUFFIX(go_quad_zero);
			for (i = k ; i < m; i++) {
				QUAD p;
				SUFFIX(go_quad_mul) (&p, &V->data[i][k], &R->data[i][j]);
				SUFFIX(go_quad_add) (&tmp[j], &tmp[j], &p);
			}
		}

		/* R[k:m,k:n] -= v[k] * tmp */
		for (j = k; j < n; j++) {
			for (i = k; i < m; i++) {
				QUAD p;
				SUFFIX(go_quad_mul) (&p, &V->data[i][k], &tmp[j]);
				SUFFIX(go_quad_add) (&p, &p, &p);
				SUFFIX(go_quad_sub) (&R->data[i][j], &R->data[i][j], &p);
			}
		}

		/* Explicitly zero what should become zero.  */
		for (i = k + 1; i < m; i++)
			R->data[i][k] = SUFFIX(go_quad_zero);
	}

	g_free (tmp);

	for (i = 0; i < n /* Only n */; i++)
		for (j = 0; j < n; j++)
			qr->R->data[i][j] = R->data[i][j];

	qr->qdet = qdet;

	SUFFIX(go_quad_matrix_free) (R);

	return qr;
}

void
SUFFIX(go_quad_qr_free) (QQR *qr)
{
	g_return_if_fail (qr != NULL);

	SUFFIX(go_quad_matrix_free) (qr->V);
	SUFFIX(go_quad_matrix_free) (qr->R);
	g_free (qr);
}

void
SUFFIX(go_quad_qr_determinant) (const QQR *qr, QUAD *det)
{
	int i;

	g_return_if_fail (qr != NULL);
	g_return_if_fail (det != NULL);

	SUFFIX(go_quad_init) (det, qr->qdet);
	for (i = 0; i < qr->R->n; i++)
		SUFFIX(go_quad_mul) (det, det, &qr->R->data[i][i]);
}

/**
 * go_quad_qr_r:
 * @qr: A QR decomposition.
 *
 * Returns: the small R from the decomposition, i.e., a square matrix
 * of size n.  To get the large R, if needed, add m-n zero rows.
 **/
const QMATRIX *
SUFFIX(go_quad_qr_r) (const QQR *qr)
{
	g_return_val_if_fail (qr != NULL, NULL);

	return qr->R;
}

/**
 * go_quad_qr_multiply_qt:
 * @qr: A QR decomposition.
 * @x: (inout): a vector.
 *
 * Replaces @x by Q^t * x
 **/
void
SUFFIX(go_quad_qr_multiply_qt) (const QQR *qr, QUAD *x)
{
	int i, k;
	QMATRIX *V = qr->V;

	for (k = 0; k < V->n; k++) {
		QUAD s = SUFFIX(go_quad_zero);
		for (i = k; i < V->m; i++) {
			QUAD p;
			SUFFIX(go_quad_mul) (&p, &x[i], &V->data[i][k]);
			SUFFIX(go_quad_add) (&s, &s, &p);
		}
		SUFFIX(go_quad_add) (&s, &s, &s);
		for (i = k; i < V->m; i++) {
			QUAD p;
			SUFFIX(go_quad_mul) (&p, &s, &V->data[i][k]);
			SUFFIX(go_quad_sub) (&x[i], &x[i], &p);
		}
	}
}

/**
 * go_quad_qr_mark_degenerate: (skip)
 * @qr: A QR decomposition.
 * @i: a dimension
 *
 * Marks dimension i of the qr decomposition as degenerate.  In practice
 * this means setting the i-th eigenvalue of R to zero.
 **/
void
SUFFIX(go_quad_qr_mark_degenerate) (QQR *qr, int i)
{
	g_return_if_fail (qr != NULL);
	g_return_if_fail (i >= 0 && i < qr->R->n);

	qr->R->data[i][i] = SUFFIX(go_quad_zero);
}

/* ------------------------------------------------------------------------- */

// See comments at top
#endif // SKIP_THIS_PASS
#if INCLUDE_PASS < INCLUDE_PASS_LAST
#include __FILE__
#endif
