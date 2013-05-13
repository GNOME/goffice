#ifndef GO_MATRIX_H__
#define GO_MATRIX_H__

#include <glib.h>

G_BEGIN_DECLS

struct GOQuadMatrix_ {
	GOQuad **data;   /* [m][n] */
	int m; /* down */
	int n; /* right */
};

GOQuadMatrix *go_quad_matrix_new (int m, int n);
GOQuadMatrix *go_quad_matrix_dup (const GOQuadMatrix *A);
void go_quad_matrix_free (GOQuadMatrix *A);

void go_quad_matrix_copy (GOQuadMatrix *A, const GOQuadMatrix *B);
void go_quad_matrix_transpose (GOQuadMatrix *A, const GOQuadMatrix *B);

void go_quad_matrix_multiply (GOQuadMatrix *C,
			      const GOQuadMatrix *A,
			      const GOQuadMatrix *B);

GOQuadMatrix *go_quad_matrix_inverse (const GOQuadMatrix *A, double threshold);

void go_quad_matrix_determinant (const GOQuadMatrix *A, GOQuad *res);

GOQuadMatrix *go_quad_matrix_pseudo_inverse (const GOQuadMatrix *A,
					     double threshold);

gboolean go_quad_matrix_back_solve (const GOQuadMatrix *R, GOQuad *x,
				    const GOQuad *b,
				    gboolean allow_degenerate);
gboolean go_quad_matrix_fwd_solve (const GOQuadMatrix *R, GOQuad *x,
				   const GOQuad *b,
				   gboolean allow_degenerate);

void go_quad_matrix_eigen_range (const GOQuadMatrix *A,
				 double *emin, double *emax);

void go_quad_matrix_dump (const GOQuadMatrix *A, const char *fmt);

/* ---------------------------------------- */

GOQuadQR *go_quad_qr_new (const GOQuadMatrix *A);
void go_quad_qr_free (GOQuadQR *qr);
void go_quad_qr_determinant (const GOQuadQR *qr, GOQuad *det);
const GOQuadMatrix *go_quad_qr_r (const GOQuadQR *qr);
void go_quad_qr_multiply_qt (const GOQuadQR *qr, GOQuad *x);
void go_quad_qr_mark_degenerate (GOQuadQR *qr, int i);

/* ---------------------------------------- */

#ifdef GOFFICE_WITH_LONG_DOUBLE
struct GOQuadMatrixl_ {
	GOQuadl **data;   /* [m][n] */
	int m; /* down */
	int n; /* right */
};

GOQuadMatrixl *go_quad_matrix_newl (int m, int n);
GOQuadMatrixl *go_quad_matrix_dupl (const GOQuadMatrixl *A);
void go_quad_matrix_freel (GOQuadMatrixl *A);

void go_quad_matrix_copyl (GOQuadMatrixl *A, const GOQuadMatrixl *B);
void go_quad_matrix_transposel (GOQuadMatrixl *A, const GOQuadMatrixl *B);

void go_quad_matrix_multiplyl (GOQuadMatrixl *C,
			       const GOQuadMatrixl *A,
			       const GOQuadMatrixl *B);

GOQuadMatrixl *go_quad_matrix_inversel (const GOQuadMatrixl *A, long double threshold);

void go_quad_matrix_determinantl (const GOQuadMatrixl *A, GOQuadl *res);

GOQuadMatrixl *go_quad_matrix_pseudo_inversel (const GOQuadMatrixl *A,
					       long double threshold);

gboolean go_quad_matrix_back_solvel (const GOQuadMatrixl *R, GOQuadl *x,
				     const GOQuadl *b,
				     gboolean allow_degenerate);
gboolean go_quad_matrix_fwd_solvel (const GOQuadMatrixl *R, GOQuadl *x,
				    const GOQuadl *b,
				    gboolean allow_degenerate);

void go_quad_matrix_eigen_rangel (const GOQuadMatrixl *A,
				  long double *emin, long double *emax);

void go_quad_matrix_dumpl (const GOQuadMatrixl *A, const char *fmt);

/* ---------------------------------------- */

GOQuadQRl *go_quad_qr_newl (const GOQuadMatrixl *A);
void go_quad_qr_freel (GOQuadQRl *qr);
void go_quad_qr_determinantl (const GOQuadQRl *qr, GOQuadl *det);
const GOQuadMatrixl *go_quad_qr_rl (const GOQuadQRl *qr);
void go_quad_qr_multiply_qtl (const GOQuadQRl *qr, GOQuadl *x);
void go_quad_qr_mark_degeneratel (GOQuadQRl *qr, int i);

#endif

G_END_DECLS

#endif
