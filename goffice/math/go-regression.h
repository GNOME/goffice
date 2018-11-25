#ifndef GO_UTILS_REGRESSION_H
#define GO_UTILS_REGRESSION_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
	GO_REG_ok,
	GO_REG_invalid_dimensions,
	GO_REG_invalid_data,
	GO_REG_not_enough_data,
	GO_REG_near_singular_good,	/* Probably good result */
	GO_REG_near_singular_bad, 	/* Probably bad result */
	GO_REG_singular
} GORegressionResult;

typedef struct {
        double *se;		/* SE for each parameter estimator */
        double *t;  		/* t values for each parameter estimator */
        double sqr_r;
	double adj_sqr_r;
        double se_y; 		/* The Standard Error of Y */
        double F;
        int    df_reg;
        int    df_resid;
        int    df_total;
        double ss_reg;
        double ss_resid;
        double ss_total;
        double ms_reg;
        double ms_resid;
	double ybar;
	double *xbar;
	double var; 		/* The variance of the entire regression: sum(errors^2)/(n-xdim) */
	/*<private>*/
	unsigned int ref_count;
} go_regression_stat_t;
#define GORegressionStat go_regression_stat_t

GType            go_regression_stat_get_type      (void);
go_regression_stat_t 	*go_regression_stat_new 	(void);
void 			 go_regression_stat_destroy 	(go_regression_stat_t *stat_);

GORegressionResult 	 go_linear_regression 		(double **xss, int dim,
							 const double *ys, int n,
							 gboolean affine,
							 double *res,
							 go_regression_stat_t *stat_);
GORegressionResult       go_linear_regression_leverage  (double **A, double *d,
							 int m, int n);
GORegressionResult 	 go_exponential_regression 	(double **xss, int dim,
							 const double *ys, int n,
							 gboolean affine,
							 double *res,
							 go_regression_stat_t *stat_);
GORegressionResult 	 go_exponential_regression_as_log 	(double **xss, int dim,
							 const double *ys, int n,
							 gboolean affine,
							 double *res,
							 go_regression_stat_t *stat_);
GORegressionResult 	 go_power_regression 	(double **xss, int dim,
							 const double *ys, int n,
							 gboolean affine,
							 double *res,
							 go_regression_stat_t *stat_);
GORegressionResult 	 go_logarithmic_regression 	(double **xss, int dim,
							 const double *ys, int n,
							 gboolean affine,
							 double *res,
							 go_regression_stat_t *stat_);

/* Final accuracy of c is: width of x-range rounded to the next smaller
 * (10^integer), the result times GO_LOGFIT_C_ACCURACY.
 * If you change it, remember to change the help-text for LOGFIT.
 * FIXME: Is there a way to stringify this macros value for the help-text? */
#define GO_LOGFIT_C_ACCURACY 0.000001

/* Stepwidth for testing for sign is: width of x-range
 * times GO_LOGFIT_C_STEP_FACTOR. Value is tested a bit. */
#define GO_LOGFIT_C_STEP_FACTOR 0.05

/* Width of fitted c-range is: width of x-range
 * times GO_LOGFIT_C_RANGE_FACTOR. Value is tested a bit.
 * Point clouds with a local minimum of squared residuals outside the fitted
 * c-range are very weakly bent. */
#define GO_LOGFIT_C_RANGE_FACTOR 100

GORegressionResult go_logarithmic_fit (double *xs,
				       const double *ys, int n,
				       double *res);

typedef GORegressionResult (*GORegressionFunction) (double * x, double * params, double *f);

GORegressionResult go_non_linear_regression (GORegressionFunction f,
					     double **xvals,
					     double *par,
					     double *yvals,
					     double *sigmas,
					     int x_dim,
					     int p_dim,
					     double *chi,
					     double *errors);

gboolean go_matrix_invert 	(double **A, int n);
double   go_matrix_determinant 	(double *const *const A, int n);

void     go_matrix_pseudo_inverse (double *const * const A, int m, int n,
				   double threshold,
				   double **B);

GORegressionResult go_linear_solve (double *const *const A,
				    const double *b,
				    int n,
				    double *res);
GORegressionResult go_linear_solve_multiple (double *const *const A,
					     double **B,
					     int n, int bn);

#ifdef GOFFICE_WITH_LONG_DOUBLE
typedef struct {
        long double *se; /*SE for each parameter estimator*/
        long double *t;  /*t values for each parameter estimator*/
        long double sqr_r;
	long double adj_sqr_r;
        long double se_y; /* The Standard Error of Y */
        long double F;
        int        df_reg;
        int        df_resid;
        int        df_total;
        long double ss_reg;
        long double ss_resid;
        long double ss_total;
        long double ms_reg;
        long double ms_resid;
	long double ybar;
	long double *xbar;
	long double var; /* The variance of the entire regression:
			sum(errors^2)/(n-xdim) */
	/*<private>*/
	unsigned int ref_count;
} go_regression_stat_tl;
#define GORegressionStatl go_regression_stat_tl

GType            go_regression_statl_get_type      (void);
go_regression_stat_tl *go_regression_stat_newl	(void);
void 		    go_regression_stat_destroyl	(go_regression_stat_tl *stat_);

GORegressionResult    go_linear_regressionl   	(long double **xss, int dim,
						 const long double *ys, int n,
						 gboolean affine,
						 long double *res,
						 go_regression_stat_tl *stat_);
GORegressionResult    go_linear_regression_leveragel (long double **A,
						      long double *d,
						      int m, int n);
GORegressionResult    go_exponential_regressionl	(long double **xss, int dim,
						 const long double *ys, int n,
						 gboolean affine,
						 long double *res,
						 go_regression_stat_tl *stat_);
GORegressionResult    go_exponential_regression_as_logl	(long double **xss, int dim,
						 const long double *ys, int n,
						 gboolean affine,
						 long double *res,
						 go_regression_stat_tl *stat_);
GORegressionResult    go_power_regressionl 	(long double **xss, int dim,
						 const long double *ys, int n,
						 gboolean affine,
						 long double *res,
						 go_regression_stat_tl *stat_);
GORegressionResult    go_logarithmic_regressionl(long double **xss, int dim,
						 const long double *ys, int n,
						 gboolean affine,
						 long double *res,
						 go_regression_stat_tl *stat_);
GORegressionResult    go_logarithmic_fitl 	(long double *xs,
						 const long double *ys, int n,
						 long double *res);

typedef GORegressionResult (*GORegressionFunctionl) (long double * x, long double * params, long double *f);

GORegressionResult    go_non_linear_regressionl	(GORegressionFunctionl f,
						 long double **xvals,
						 long double *par,
						 long double *yvals,
						 long double *sigmas,
						 int x_dim,
						 int p_dim,
						 long double *chi,
						 long double *errors);

gboolean    go_matrix_invertl 		(long double **A, int n);
long double go_matrix_determinantl 	(long double *const * const A, int n);

void     go_matrix_pseudo_inversel (long double *const * const A, int m, int n,
				    long double threshold,
				    long double **B);

GORegressionResult go_linear_solvel (long double *const *const A,
				     const long double *b,
				     int n,
				     long double *res);
GORegressionResult go_linear_solve_multiplel (long double *const *const A,
					      long double **B,
					      int n, int bn);

#endif

G_END_DECLS

#endif /* GO_UTILS_REGRESSION_H */
