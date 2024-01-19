/*
 * go-R.c
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

#include <goffice/goffice-config.h>
#include "go-math.h"
#include "go-R.h"

// We need multiple versions of this code.  We're going to include ourself
// with different settings of various macros.  gdb will hate us.
#include <goffice/goffice-multipass.h>
#ifndef SKIP_THIS_PASS

// Let's not pretend this code has been audited for _Decimal64
#if INCLUDE_PASS == INCLUDE_PASS_DECIMAL64
#define SKIP_THIS_PASS

_Decimal64
go_truncD (_Decimal64 x)
{
	return go_trunc (x);
}

_Decimal64
go_dnormD (_Decimal64 x, _Decimal64 mu, _Decimal64 sigma, gboolean give_log)
{
	return go_dnorm (x, mu, sigma, give_log);
}

_Decimal64
go_pnormD (_Decimal64 x, _Decimal64 mu, _Decimal64 sigma, gboolean lower_tail, gboolean log_p)
{
	return go_pnorm (x, mu, sigma, lower_tail, log_p);
}

void
go_pnorm_bothD (_Decimal64 x, _Decimal64 *cum, _Decimal64 *ccum, int i_tail, gboolean log_p)
{
	double dcum, dccum;
	go_pnorm_both (x, &dcum, &dccum, i_tail, log_p);
	*cum = dcum;
	*ccum = dccum;
}

_Decimal64
go_qnormD (_Decimal64 p, _Decimal64 mu, _Decimal64 sigma, gboolean lower_tail, gboolean log_p)
{
	return go_qnorm (p, mu, sigma, lower_tail, log_p);
}

_Decimal64
go_dlnormD (_Decimal64 x, _Decimal64 meanlog, _Decimal64 sdlog, gboolean give_log)
{
	return go_dlnorm (x, meanlog, sdlog, give_log);
}

_Decimal64
go_plnormD (_Decimal64 x, _Decimal64 logmean, _Decimal64 logsd, gboolean lower_tail, gboolean log_p)
{
	return go_plnorm (x, logmean, logsd, lower_tail, log_p);
}

_Decimal64
go_qlnormD (_Decimal64 p, _Decimal64 logmean, _Decimal64 logsd, gboolean lower_tail, gboolean log_p)
{
	return go_qlnorm (p, logmean, logsd, lower_tail, log_p);
}

_Decimal64
go_dweibullD (_Decimal64 x, _Decimal64 shape, _Decimal64 scale, gboolean give_log)
{
	return go_dweibull (x, shape, scale, give_log);
}

_Decimal64
go_pweibullD (_Decimal64 x, _Decimal64 shape, _Decimal64 scale, gboolean lower_tail, gboolean log_p)
{
	return go_pweibull (x, shape, scale, lower_tail, log_p);
}

_Decimal64
go_qweibullD (_Decimal64 p, _Decimal64 shape, _Decimal64 scale, gboolean lower_tail, gboolean log_p)
{
	return go_qweibull (p, shape, scale, lower_tail, log_p);
}

_Decimal64
go_dcauchyD (_Decimal64 x, _Decimal64 location, _Decimal64 scale, gboolean give_log)
{
	return go_dcauchy (x, location, scale, give_log);
}

_Decimal64
go_pcauchyD (_Decimal64 x, _Decimal64 location, _Decimal64 scale, gboolean lower_tail, gboolean log_p)
{
	return go_pcauchy (x, location, scale, lower_tail, log_p);
}

_Decimal64
go_qcauchyD (_Decimal64 p, _Decimal64 location, _Decimal64 scale, gboolean lower_tail, gboolean log_p)
{
	return go_qcauchy (p, location, scale, lower_tail, log_p);
}


#endif

#endif


#ifndef SKIP_THIS_PASS


#define M_LN_SQRT_2PI   CONST(0.918938533204672741780329736406)  /* log(sqrt(2*pi)) */
#define M_SQRT_32       CONST(5.656854249492380195206754896838)  /* sqrt(32) */
#define M_1_SQRT_2PI    CONST(0.398942280401432677939946059934)  /* 1/sqrt(2pi) */
#define M_LN2goffice	CONST(0.693147180559945309417232121458176568075500134360255254120680009493393621969694715605863326996419)
#define M_PIgoffice	CONST(3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117)
#define ML_ERR_return_NAN { return SUFFIX(go_nan); }

/* ------------------------------------------------------------------------- */
/* --- BEGIN MAGIC R HEADER 1 MARKER --- */

/* The following source code was imported from the R project.  */
/* It was automatically transformed by tools/import-R.  */

/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 2000--2007  R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 */
	/* Utilities for `dpq' handling (density/probability/quantile) */

/* give_log in "d";  log_p in "p" & "q" : */
#define give_log log_p
							/* "DEFAULT" */
							/* --------- */
#define R_D__0	(log_p ? SUFFIX (go_ninf) : 0.)		/* 0 */
#define R_D__1	(log_p ? 0. : 1.)			/* 1 */
#define R_DT_0	(lower_tail ? R_D__0 : R_D__1)		/* 0 */
#define R_DT_1	(lower_tail ? R_D__1 : R_D__0)		/* 1 */

/* Use 0.5 - p + 0.5 to perhaps gain 1 bit of accuracy */
#define R_D_Lval(p)	(lower_tail ? (p) : (0.5 - (p) + 0.5))	/*  p  */
#define R_D_Cval(p)	(lower_tail ? (0.5 - (p) + 0.5) : (p))	/*  1 - p */

#define R_D_val(x)	(log_p	? SUFFIX (log) (x) : (x))		/*  x  in pF(x,..) */
#define R_D_qIv(p)	(log_p	? SUFFIX (exp) (p) : (p))		/*  p  in qF(p,..) */
#define R_D_exp(x)	(log_p	?  (x)	 : SUFFIX (exp) (x))	/* SUFFIX (exp) (x) */
#define R_D_log(p)	(log_p	?  (p)	 : SUFFIX (log) (p))	/* SUFFIX (log) (p) */
#define R_D_Clog(p)	(log_p	? SUFFIX (log1p) (-(p)) : (0.5 - (p) + 0.5)) /* [log](1-p) */

/* SUFFIX (log) (1 - SUFFIX (exp) (x))  in more stable form than SUFFIX (log1p) (- R_D_qIv(x))) : */
#define swap_log_tail(x)   ((x) > -M_LN2goffice ? SUFFIX (log) (-SUFFIX (expm1) (x)) : SUFFIX (log1p) (-SUFFIX (exp) (x)))

/* SUFFIX (log) (1-SUFFIX (exp) (x)):  R_D_LExp(x) == (SUFFIX (log1p) (- R_D_qIv(x))) but even more stable:*/
#define R_D_LExp(x)     (log_p ? swap_log_tail(x) : SUFFIX (log1p) (-x))

#define R_DT_val(x)	(lower_tail ? R_D_val(x)  : R_D_Clog(x))
#define R_DT_Cval(x)	(lower_tail ? R_D_Clog(x) : R_D_val(x))

/*#define R_DT_qIv(p)	R_D_Lval(R_D_qIv(p))		 *  p  in qF ! */
#define R_DT_qIv(p)	(log_p ? (lower_tail ? SUFFIX (exp) (p) : - SUFFIX (expm1) (p)) \
			       : R_D_Lval(p))

/*#define R_DT_CIv(p)	R_D_Cval(R_D_qIv(p))		 *  1 - p in qF */
#define R_DT_CIv(p)	(log_p ? (lower_tail ? -SUFFIX (expm1) (p) : SUFFIX (exp) (p)) \
			       : R_D_Cval(p))

#define R_DT_exp(x)	R_D_exp(R_D_Lval(x))		/* SUFFIX (exp) (x) */
#define R_DT_Cexp(x)	R_D_exp(R_D_Cval(x))		/* SUFFIX (exp) (1 - x) */

#define R_DT_log(p)	(lower_tail? R_D_log(p) : R_D_LExp(p))/* SUFFIX (log) (p) in qF */
#define R_DT_Clog(p)	(lower_tail? R_D_LExp(p): R_D_log(p))/* SUFFIX (go_log1p) (-p) in qF*/
#define R_DT_Log(p)	(lower_tail? (p) : swap_log_tail(p))
/* ==   R_DT_log when we already "know" log_p == TRUE :*/


#define R_Q_P01_check(p)			\
    if ((log_p	&& p > 0) ||			\
	(!log_p && (p < 0 || p > 1)) )		\
	ML_ERR_return_NAN

/* Do the boundaries exactly for q*() functions :
 * Often  _LEFT_ = SUFFIX (go_ninf) , and very often _RIGHT_ = SUFFIX (go_pinf);
 *
 * R_Q_P01_boundaries(p, _LEFT_, _RIGHT_)  :<==>
 *
 *     R_Q_P01_check(p);
 *     if (p == R_DT_0) return _LEFT_ ;
 *     if (p == R_DT_1) return _RIGHT_;
 *
 * the following implementation should be more efficient (less tests):
 */
#define R_Q_P01_boundaries(p, _LEFT_, _RIGHT_)		\
    if (log_p) {					\
	if(p > 0)					\
	    ML_ERR_return_NAN;				\
	if(p == 0) /* upper bound*/			\
	    return lower_tail ? _RIGHT_ : _LEFT_;	\
	if(p == SUFFIX (go_ninf))				\
	    return lower_tail ? _LEFT_ : _RIGHT_;	\
    }							\
    else { /* !log_p */					\
	if(p < 0 || p > 1)				\
	    ML_ERR_return_NAN;				\
	if(p == 0)					\
	    return lower_tail ? _LEFT_ : _RIGHT_;	\
	if(p == 1)					\
	    return lower_tail ? _RIGHT_ : _LEFT_;	\
    }

#define R_P_bounds_01(x, x_min, x_max) 	\
    if(x <= x_min) return R_DT_0;		\
    if(x >= x_max) return R_DT_1
/* is typically not quite optimal for (-Inf,Inf) where
 * you'd rather have */
#define R_P_bounds_Inf_01(x)			\
    if(!SUFFIX (go_finite)(x)) {				\
	if (x > 0) return R_DT_1;		\
	/* x < 0 */return R_DT_0;		\
    }



/* additions for density functions (C.Loader) */
#define R_D_fexp(f,x)     (give_log ? -0.5*SUFFIX (log) (f)+(x) : SUFFIX (exp) (x)/SUFFIX (sqrt) (f))
#define R_D_forceint(x)   SUFFIX (floor) ((x) + 0.5)
#define R_D_nonint(x) 	  (SUFFIX (fabs) ((x) - SUFFIX (floor) ((x)+0.5)) > 1e-7)
/* [neg]ative or [non int]eger : */
#define R_D_negInonint(x) (x < 0. || R_D_nonint(x))

#define R_D_nonint_check(x) 				\
   if(R_D_nonint(x)) {					\
	MATHLIB_WARNING("non-integer x = %" FORMAT_f "", x);	\
	return R_D__0;					\
   }
/* --- END MAGIC R HEADER 1 MARKER --- */


/* ------------------------------------------------------------------------- */
/* --- BEGIN MAGIC R HEADER 2 MARKER --- */
/* Cleaning up done by tools/import-R:  */
#undef R_DT_0
#undef R_DT_1
#undef R_DT_CIv
#undef R_DT_Cexp
#undef R_DT_Clog
#undef R_DT_Cval
#undef R_DT_Log
#undef R_DT_exp
#undef R_DT_log
#undef R_DT_qIv
#undef R_DT_val
#undef R_D_Clog
#undef R_D_Cval
#undef R_D_LExp
#undef R_D_Lval
#undef R_D__0
#undef R_D__1
#undef R_D_exp
#undef R_D_fexp
#undef R_D_forceint
#undef R_D_log
#undef R_D_negInonint
#undef R_D_nonint
#undef R_D_nonint_check
#undef R_D_qIv
#undef R_D_val
#undef R_Log1_Exp
#undef R_P_bounds_01
#undef R_P_bounds_Inf_01
#undef R_Q_P01_boundaries
#undef R_Q_P01_check
#undef give_log

/* The following source code was imported from the R project.  */
/* It was automatically transformed by tools/import-R.  */

/*
 *  R : A Computer Language for Statistical Data Analysis
 *  Copyright (C) 2000--2007  R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 */
	/* Utilities for `dpq' handling (density/probability/quantile) */

/* give_log in "d";  log_p in "p" & "q" : */
#define give_log log_p
							/* "DEFAULT" */
							/* --------- */
#define R_D__0	(log_p ? SUFFIX (go_ninf) : 0.)		/* 0 */
#define R_D__1	(log_p ? 0. : 1.)			/* 1 */
#define R_DT_0	(lower_tail ? R_D__0 : R_D__1)		/* 0 */
#define R_DT_1	(lower_tail ? R_D__1 : R_D__0)		/* 1 */

/* Use 0.5 - p + 0.5 to perhaps gain 1 bit of accuracy */
#define R_D_Lval(p)	(lower_tail ? (p) : (0.5 - (p) + 0.5))	/*  p  */
#define R_D_Cval(p)	(lower_tail ? (0.5 - (p) + 0.5) : (p))	/*  1 - p */

#define R_D_val(x)	(log_p	? SUFFIX (log) (x) : (x))		/*  x  in pF(x,..) */
#define R_D_qIv(p)	(log_p	? SUFFIX (exp) (p) : (p))		/*  p  in qF(p,..) */
#define R_D_exp(x)	(log_p	?  (x)	 : SUFFIX (exp) (x))	/* SUFFIX (exp) (x) */
#define R_D_log(p)	(log_p	?  (p)	 : SUFFIX (log) (p))	/* SUFFIX (log) (p) */
#define R_D_Clog(p)	(log_p	? SUFFIX (log1p) (-(p)) : (0.5 - (p) + 0.5)) /* [log](1-p) */

/* SUFFIX (log) (1 - SUFFIX (exp) (x))  in more stable form than SUFFIX (log1p) (- R_D_qIv(x))) : */
#define swap_log_tail(x)   ((x) > -M_LN2goffice ? SUFFIX (log) (-SUFFIX (expm1) (x)) : SUFFIX (log1p) (-SUFFIX (exp) (x)))

/* SUFFIX (log) (1-SUFFIX (exp) (x)):  R_D_LExp(x) == (SUFFIX (log1p) (- R_D_qIv(x))) but even more stable:*/
#define R_D_LExp(x)     (log_p ? swap_log_tail(x) : SUFFIX (log1p) (-x))

#define R_DT_val(x)	(lower_tail ? R_D_val(x)  : R_D_Clog(x))
#define R_DT_Cval(x)	(lower_tail ? R_D_Clog(x) : R_D_val(x))

/*#define R_DT_qIv(p)	R_D_Lval(R_D_qIv(p))		 *  p  in qF ! */
#define R_DT_qIv(p)	(log_p ? (lower_tail ? SUFFIX (exp) (p) : - SUFFIX (expm1) (p)) \
			       : R_D_Lval(p))

/*#define R_DT_CIv(p)	R_D_Cval(R_D_qIv(p))		 *  1 - p in qF */
#define R_DT_CIv(p)	(log_p ? (lower_tail ? -SUFFIX (expm1) (p) : SUFFIX (exp) (p)) \
			       : R_D_Cval(p))

#define R_DT_exp(x)	R_D_exp(R_D_Lval(x))		/* SUFFIX (exp) (x) */
#define R_DT_Cexp(x)	R_D_exp(R_D_Cval(x))		/* SUFFIX (exp) (1 - x) */

#define R_DT_log(p)	(lower_tail? R_D_log(p) : R_D_LExp(p))/* SUFFIX (log) (p) in qF */
#define R_DT_Clog(p)	(lower_tail? R_D_LExp(p): R_D_log(p))/* SUFFIX (go_log1p) (-p) in qF*/
#define R_DT_Log(p)	(lower_tail? (p) : swap_log_tail(p))
/* ==   R_DT_log when we already "know" log_p == TRUE :*/


#define R_Q_P01_check(p)			\
    if ((log_p	&& p > 0) ||			\
	(!log_p && (p < 0 || p > 1)) )		\
	ML_ERR_return_NAN

/* Do the boundaries exactly for q*() functions :
 * Often  _LEFT_ = SUFFIX (go_ninf) , and very often _RIGHT_ = SUFFIX (go_pinf);
 *
 * R_Q_P01_boundaries(p, _LEFT_, _RIGHT_)  :<==>
 *
 *     R_Q_P01_check(p);
 *     if (p == R_DT_0) return _LEFT_ ;
 *     if (p == R_DT_1) return _RIGHT_;
 *
 * the following implementation should be more efficient (less tests):
 */
#define R_Q_P01_boundaries(p, _LEFT_, _RIGHT_)		\
    if (log_p) {					\
	if(p > 0)					\
	    ML_ERR_return_NAN;				\
	if(p == 0) /* upper bound*/			\
	    return lower_tail ? _RIGHT_ : _LEFT_;	\
	if(p == SUFFIX (go_ninf))				\
	    return lower_tail ? _LEFT_ : _RIGHT_;	\
    }							\
    else { /* !log_p */					\
	if(p < 0 || p > 1)				\
	    ML_ERR_return_NAN;				\
	if(p == 0)					\
	    return lower_tail ? _LEFT_ : _RIGHT_;	\
	if(p == 1)					\
	    return lower_tail ? _RIGHT_ : _LEFT_;	\
    }

#define R_P_bounds_01(x, x_min, x_max) 	\
    if(x <= x_min) return R_DT_0;		\
    if(x >= x_max) return R_DT_1
/* is typically not quite optimal for (-Inf,Inf) where
 * you'd rather have */
#define R_P_bounds_Inf_01(x)			\
    if(!SUFFIX (go_finite)(x)) {				\
	if (x > 0) return R_DT_1;		\
	/* x < 0 */return R_DT_0;		\
    }



/* additions for density functions (C.Loader) */
#define R_D_fexp(f,x)     (give_log ? -0.5*SUFFIX (log) (f)+(x) : SUFFIX (exp) (x)/SUFFIX (sqrt) (f))
#define R_D_forceint(x)   SUFFIX (floor) ((x) + 0.5)
#define R_D_nonint(x) 	  (SUFFIX (fabs) ((x) - SUFFIX (floor) ((x)+0.5)) > 1e-7)
/* [neg]ative or [non int]eger : */
#define R_D_negInonint(x) (x < 0. || R_D_nonint(x))

#define R_D_nonint_check(x) 				\
   if(R_D_nonint(x)) {					\
	MATHLIB_WARNING("non-integer x = %" FORMAT_f "", x);	\
	return R_D__0;					\
   }
/* --- END MAGIC R HEADER 2 MARKER --- */


// ???


/* ------------------------------------------------------------------------- */
/* --- BEGIN MAGIC R SOURCE MARKER --- */

/* The following source code was imported from the R project.  */
/* It was automatically transformed by tools/import-R.  */

/* Imported src/nmath/ftrunc.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  SYNOPSIS
 *
 *    #include <Rmath.h>
 *    double ftrunc(double x);
 *
 *  DESCRIPTION
 *
 *    Truncation toward zero.
 */


DOUBLE SUFFIX (go_trunc) (DOUBLE x)
{
	if(x >= 0) return SUFFIX (floor) (x);
	else return SUFFIX (ceil) (x);
}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/dnorm.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000	    The R Development Core Team
 *  Copyright (C) 2003	    The R Foundation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  SYNOPSIS
 *
 *	double dnorm4(double x, double mu, double sigma, int give_log)
 *	      {dnorm (..) is synonymous and preferred inside R}
 *
 *  DESCRIPTION
 *
 *	Compute the density of the normal distribution.
 */


DOUBLE SUFFIX (go_dnorm) (DOUBLE x, DOUBLE mu, DOUBLE sigma, gboolean give_log)
{
#ifdef IEEE_754
    if (SUFFIX (isnan) (x) || SUFFIX (isnan) (mu) || SUFFIX (isnan) (sigma))
	return x + mu + sigma;
#endif
    if(!SUFFIX (go_finite)(sigma)) return R_D__0;
    if(!SUFFIX (go_finite)(x) && mu == x) return SUFFIX (go_nan);/* x-mu is NaN */
    if (sigma <= 0) {
	if (sigma < 0) ML_ERR_return_NAN;
	/* sigma == 0 */
	return (x == mu) ? SUFFIX (go_pinf) : R_D__0;
    }
    x = (x - mu) / sigma;

    if(!SUFFIX (go_finite)(x)) return R_D__0;
    return (give_log ?
	    -(M_LN_SQRT_2PI  +	0.5 * x * x + SUFFIX (log) (sigma)) :
	    M_1_SQRT_2PI * SUFFIX (exp) (-0.5 * x * x)  /	  sigma);
    /* M_1_SQRT_2PI = 1 / SUFFIX (sqrt) (2 * pi) */
}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/pnorm.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998	    Ross Ihaka
 *  Copyright (C) 2000-2002 The R Development Core Team
 *  Copyright (C) 2003	    The R Foundation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  SYNOPSIS
 *
 *   #include <Rmath.h>
 *
 *   double pnorm5(double x, double mu, double sigma, int lower_tail,int log_p);
 *	   {pnorm (..) is synonymous and preferred inside R}
 *
 *   void   pnorm_both(double x, double *cum, double *ccum,
 *		       int i_tail, int log_p);
 *
 *  DESCRIPTION
 *
 *	The main computation evaluates near-minimax approximations derived
 *	from those in "Rational Chebyshev approximations for the error
 *	function" by W. J. Cody, Math. Comp., 1969, 631-637.  This
 *	transportable program uses rational functions that theoretically
 *	approximate the normal distribution function to at least 18
 *	significant decimal digits.  The accuracy achieved depends on the
 *	arithmetic system, the compiler, the intrinsic functions, and
 *	proper selection of the machine-dependent constants.
 *
 *  REFERENCE
 *
 *	Cody, W. D. (1993).
 *	ALGORITHM 715: SPECFUN - A Portable FORTRAN Package of
 *	Special Function Routines and Test Drivers".
 *	ACM Transactions on Mathematical Software. 19, 22-32.
 *
 *  EXTENSIONS
 *
 *  The "_both" , lower, upper, and log_p  variants were added by
 *  Martin Maechler, Jan.2000;
 *  as well as log1p() and similar improvements later on.
 *
 *  James M. Rath contributed bug report PR#699 and patches correcting SIXTEN
 *  and if() clauses {with a bug: "|| instead of &&" -> PR #2883) more in line
 *  with the original Cody code.
 */

DOUBLE SUFFIX (go_pnorm) (DOUBLE x, DOUBLE mu, DOUBLE sigma, gboolean lower_tail, gboolean log_p)
{
    DOUBLE p, cp;

    /* Note: The structure of these checks has been carefully thought through.
     * For example, if x == mu and sigma == 0, we get the correct answer 1.
     */
#ifdef IEEE_754
    if(SUFFIX (isnan) (x) || SUFFIX (isnan) (mu) || SUFFIX (isnan) (sigma))
	return x + mu + sigma;
#endif
    if(!SUFFIX (go_finite)(x) && mu == x) return SUFFIX (go_nan);/* x-mu is NaN */
    if (sigma <= 0) {
	if(sigma < 0) ML_ERR_return_NAN;
	/* sigma = 0 : */
	return (x < mu) ? R_DT_0 : R_DT_1;
    }
    p = (x - mu) / sigma;
    if(!SUFFIX (go_finite)(p))
	return (x < mu) ? R_DT_0 : R_DT_1;
    x = p;

    SUFFIX (go_pnorm_both) (x, &p, &cp, (lower_tail ? 0 : 1), log_p);

    return(lower_tail ? p : cp);
}

#define SIXTEN	16 /* Cutoff allowing exact "*" and "/" */

void SUFFIX (go_pnorm_both) (DOUBLE x, DOUBLE *cum, DOUBLE *ccum, int i_tail, gboolean log_p)
{
/* i_tail in {0,1,2} means: "lower", "upper", or "both" :
   if(lower) return  *cum := P[X <= x]
   if(upper) return *ccum := P[X >  x] = 1 - P[X <= x]
*/
    const static DOUBLE a[5] = {
	CONST (2.2352520354606839287),
	CONST (161.02823106855587881),
	CONST (1067.6894854603709582),
	CONST (18154.981253343561249),
	CONST (0.065682337918207449113)
    };
    const static DOUBLE b[4] = {
	CONST (47.20258190468824187),
	CONST (976.09855173777669322),
	CONST (10260.932208618978205),
	CONST (45507.789335026729956)
    };
    const static DOUBLE c[9] = {
	CONST (0.39894151208813466764),
	CONST (8.8831497943883759412),
	CONST (93.506656132177855979),
	CONST (597.27027639480026226),
	CONST (2494.5375852903726711),
	CONST (6848.1904505362823326),
	CONST (11602.651437647350124),
	CONST (9842.7148383839780218),
	CONST (1.0765576773720192317e-8)
    };
    const static DOUBLE d[8] = {
	CONST (22.266688044328115691),
	CONST (235.38790178262499861),
	CONST (1519.377599407554805),
	CONST (6485.558298266760755),
	CONST (18615.571640885098091),
	CONST (34900.952721145977266),
	CONST (38912.003286093271411),
	CONST (19685.429676859990727)
    };
    const static DOUBLE p[6] = {
	CONST (0.21589853405795699),
	CONST (0.1274011611602473639),
	CONST (0.022235277870649807),
	CONST (0.001421619193227893466),
	CONST (2.9112874951168792e-5),
	CONST (0.02307344176494017303)
    };
    const static DOUBLE q[5] = {
	CONST (1.28426009614491121),
	CONST (0.468238212480865118),
	CONST (0.0659881378689285515),
	CONST (0.00378239633202758244),
	CONST (7.29751555083966205e-5)
    };

    DOUBLE xden, xnum, temp, del, eps, xsq, y;
#ifdef NO_DENORMS
    DOUBLE min = DOUBLE_MIN;
#endif
    int i, lower, upper;

#ifdef IEEE_754
    if(SUFFIX (isnan) (x)) { *cum = *ccum = x; return; }
#endif

    /* Consider changing these : */
    eps = DOUBLE_EPSILON * 0.5;

    /* i_tail in {0,1,2} =^= {lower, upper, both} */
    lower = i_tail != 1;
    upper = i_tail != 0;

    y = SUFFIX (fabs) (x);
    if (y <= CONST (0.67448975)) { /* SUFFIX (go_qnorm) (3/4) = .6744.... -- earlier had CONST (0.66291) */
	if (y > eps) {
	    xsq = x * x;
	    xnum = a[4] * xsq;
	    xden = xsq;
	    for (i = 0; i < 3; ++i) {
		xnum = (xnum + a[i]) * xsq;
		xden = (xden + b[i]) * xsq;
	    }
	} else xnum = xden = 0.0;

	temp = x * (xnum + a[3]) / (xden + b[3]);
	if(lower)  *cum = 0.5 + temp;
	if(upper) *ccum = 0.5 - temp;
	if(log_p) {
	    if(lower)  *cum = SUFFIX (log) (*cum);
	    if(upper) *ccum = SUFFIX (log) (*ccum);
	}
    }
    else if (y <= M_SQRT_32) {

	/* Evaluate SUFFIX (go_pnorm)  for 0.674.. = SUFFIX (go_qnorm) (3/4) < |x| <= SUFFIX (sqrt) (32) ~= 5.657 */

	xnum = c[8] * y;
	xden = y;
	for (i = 0; i < 7; ++i) {
	    xnum = (xnum + c[i]) * y;
	    xden = (xden + d[i]) * y;
	}
	temp = (xnum + c[7]) / (xden + d[7]);

#define do_del(X)							\
	xsq = SUFFIX (go_trunc) (X * SIXTEN) / SIXTEN;				\
	del = (X - xsq) * (X + xsq);					\
	if(log_p) {							\
	    *cum = (-xsq * xsq * 0.5) + (-del * 0.5) + SUFFIX (log) (temp);	\
	    if((lower && x > 0.) || (upper && x <= 0.))			\
		  *ccum = SUFFIX (log1p) (-SUFFIX (exp) (-xsq * xsq * 0.5) *		\
				SUFFIX (exp) (-del * 0.5) * temp);		\
	}								\
	else {								\
	    *cum = SUFFIX (exp) (-xsq * xsq * 0.5) * SUFFIX (exp) (-del * 0.5) * temp;	\
	    *ccum = 1.0 - *cum;						\
	}

#define swap_tail						\
	if (x > 0.) {/* swap  ccum <--> cum */			\
	    temp = *cum; if(lower) *cum = *ccum; *ccum = temp;	\
	}

	do_del(y);
	swap_tail;
    }

/* else	  |x| > SUFFIX (sqrt) (32) = 5.657 :
 * the next two case differentiations were really for lower=T, log=F
 * Particularly	 *not*	for  log_p !

 * Cody had (-37.5193 < x  &&  x < 8.2924) ; R originally had y < 50
 *
 * Note that we do want symmetry(0), lower/upper -> hence use y
 */
    else if(log_p
	/*  ^^^^^ MM FIXME: can speedup for log_p and much larger |x| !
	 * Then, make use of  Abramowitz & Stegun, 26.2.13, something like

	 xsq = x*x;

	 if(xsq * DOUBLE_EPSILON < 1.)
	    del = (1. - (1. - 5./(xsq+6.)) / (xsq+4.)) / (xsq+2.);
	 else
	    del = 0.;
	 *cum = -.5*xsq - M_LN_SQRT_2PI - SUFFIX (log) (x) + SUFFIX (log1p) (-del);
	 *ccum = SUFFIX (log1p) (-SUFFIX (exp) (*cum)); /.* ~ SUFFIX (log) (1) = 0 *./

 	 swap_tail;

	*/
	    || (lower && -37.5193 < x  &&  x < 8.2924)
	    || (upper && -8.2924  < x  &&  x < 37.5193)
	) {

	/* Evaluate SUFFIX (go_pnorm)  for x in (-37.5, -5.657) union (5.657, 37.5) */
	xsq = 1.0 / (x * x);
	xnum = p[5] * xsq;
	xden = xsq;
	for (i = 0; i < 4; ++i) {
	    xnum = (xnum + p[i]) * xsq;
	    xden = (xden + q[i]) * xsq;
	}
	temp = xsq * (xnum + p[4]) / (xden + q[4]);
	temp = (M_1_SQRT_2PI - temp) / y;

	do_del(x);
	swap_tail;
    }
    else { /* no log_p , large x such that probs are 0 or 1 */
	if(x > 0) {	*cum = 1.; *ccum = 0.;	}
	else {	        *cum = 0.; *ccum = 1.;	}
    }


#ifdef NO_DENORMS
    /* do not return "denormalized" -- we do in R */
    if(log_p) {
	if(*cum > -min)	 *cum = -0.;
	if(*ccum > -min)*ccum = -0.;
    }
    else {
	if(*cum < min)	 *cum = 0.;
	if(*ccum < min)	*ccum = 0.;
    }
#endif
    return;
}
/* Cleaning up done by tools/import-R:  */
#undef SIXTEN
#undef do_del
#undef swap_tail

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/qnorm.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998       Ross Ihaka
 *  Copyright (C) 2000--2005 The R Development Core Team
 *  based on AS 111 (C) 1977 Royal Statistical Society
 *  and   on AS 241 (C) 1988 Royal Statistical Society
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  SYNOPSIS
 *
 *	double qnorm5(double p, double mu, double sigma,
 *		      int lower_tail, int log_p)
 *            {qnorm (..) is synonymous and preferred inside R}
 *
 *  DESCRIPTION
 *
 *	Compute the quantile function for the normal distribution.
 *
 *	For small to moderate probabilities, algorithm referenced
 *	below is used to obtain an initial approximation which is
 *	polished with a final Newton step.
 *
 *	For very large arguments, an algorithm of Wichura is used.
 *
 *  REFERENCE
 *
 *	Beasley, J. D. and S. G. Springer (1977).
 *	Algorithm AS 111: The percentage points of the normal distribution,
 *	Applied Statistics, 26, 118-121.
 *
 *      Wichura, M.J. (1988).
 *      Algorithm AS 241: The Percentage Points of the Normal Distribution.
 *      Applied Statistics, 37, 477-484.
 */


DOUBLE SUFFIX (go_qnorm) (DOUBLE p, DOUBLE mu, DOUBLE sigma, gboolean lower_tail, gboolean log_p)
{
    DOUBLE p_, q, r, val;

#ifdef IEEE_754
    if (SUFFIX (isnan) (p) || SUFFIX (isnan) (mu) || SUFFIX (isnan) (sigma))
	return p + mu + sigma;
#endif
    R_Q_P01_boundaries(p, SUFFIX (go_ninf), SUFFIX (go_pinf));

    if(sigma  < 0)	ML_ERR_return_NAN;
    if(sigma == 0)	return mu;

    p_ = R_DT_qIv(p);/* real lower_tail prob. p */
    q = p_ - 0.5;

#ifdef DEBUG_qnorm
    REprintf("SUFFIX (go_qnorm) (p=%10.7" FORMAT_g ", m=%" FORMAT_g ", s=%" FORMAT_g ", l.t.= %d, log= %d): q = %" FORMAT_g "\n",
	     p,mu,sigma, lower_tail, log_p, q);
#endif


/*-- use AS 241 --- */
/* DOUBLE ppnd16_(DOUBLE *p, long *ifault)*/
/*      ALGORITHM AS241  APPL. STATIST. (1988) VOL. 37, NO. 3

        Produces the normal deviate Z corresponding to a given lower
        tail area of P; Z is accurate to about 1 part in 10**16.

        (original fortran code used PARAMETER(..) for the coefficients
         and provided hash codes for checking them...)
*/
    if (SUFFIX (fabs) (q) <= .425) {/* 0.075 <= p <= 0.925 */
        r = CONST (.180625) - q * q;
	val =
            q * (((((((r * CONST (2509.0809287301226727) +
                       CONST (33430.575583588128105)) * r + CONST (67265.770927008700853)) * r +
                     CONST (45921.953931549871457)) * r + CONST (13731.693765509461125)) * r +
                   CONST (1971.5909503065514427)) * r + CONST (133.14166789178437745)) * r +
                 CONST (3.387132872796366608))
            / (((((((r * CONST (5226.495278852854561) +
                     CONST (28729.085735721942674)) * r + CONST (39307.89580009271061)) * r +
                   CONST (21213.794301586595867)) * r + CONST (5394.1960214247511077)) * r +
                 CONST (687.1870074920579083)) * r + CONST (42.313330701600911252)) * r + 1.);
    }
    else { /* closer than 0.075 from {0,1} boundary */

	/* r = min(p, 1-p) < 0.075 */
	if (q > 0)
	    r = R_DT_CIv(p);/* 1-p */
	else
	    r = p_;/* = R_DT_Iv(p) ^=  p */

	r = SUFFIX (sqrt) (- ((log_p &&
		     ((lower_tail && q <= 0) || (!lower_tail && q > 0))) ?
		    p : /* else */ SUFFIX (log) (r)));
        /* r = SUFFIX (sqrt) (-SUFFIX (log) (r))  <==>  min(p, 1-p) = SUFFIX (exp) ( - r^2 ) */
#ifdef DEBUG_qnorm
	REprintf("\t close to 0 or 1: r = %7" FORMAT_g "\n", r);
#endif

        if (r <= 5.) { /* <==> min(p,1-p) >= SUFFIX (exp) (-25) ~= 1.3888e-11 */
            r += -1.6;
            val = (((((((r * CONST (7.7454501427834140764e-4) +
                       CONST (.0227238449892691845833)) * r + CONST (.24178072517745061177)) *
                     r + CONST (1.27045825245236838258)) * r +
                    CONST (3.64784832476320460504)) * r + CONST (5.7694972214606914055)) *
                  r + CONST (4.6303378461565452959)) * r +
                 CONST (1.42343711074968357734))
                / (((((((r *
                         CONST (1.05075007164441684324e-9) + CONST (5.475938084995344946e-4)) *
                        r + CONST (.0151986665636164571966)) * r +
                       CONST (.14810397642748007459)) * r + CONST (.68976733498510000455)) *
                     r + CONST (1.6763848301838038494)) * r +
                    CONST (2.05319162663775882187)) * r + 1.);
        }
        else { /* very close to  0 or 1 */
            r += -5.;
            val = (((((((r * CONST (2.01033439929228813265e-7) +
                       CONST (2.71155556874348757815e-5)) * r +
                      CONST (.0012426609473880784386)) * r + CONST (.026532189526576123093)) *
                    r + CONST (.29656057182850489123)) * r +
                   CONST (1.7848265399172913358)) * r + CONST (5.4637849111641143699)) *
                 r + CONST (6.6579046435011037772))
                / (((((((r *
                         CONST (2.04426310338993978564e-15) + CONST (1.4215117583164458887e-7))*
                        r + CONST (1.8463183175100546818e-5)) * r +
                       CONST (7.868691311456132591e-4)) * r + CONST (.0148753612908506148525))
                     * r + CONST (.13692988092273580531)) * r +
                    CONST (.59983220655588793769)) * r + 1.);
        }

	if(q < 0.0)
	    val = -val;
        /* return (q >= 0.)? r : -r ;*/
    }
    return mu + sigma * val;
}




/* ------------------------------------------------------------------------ */
/* Imported src/nmath/dlnorm.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000-8 The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  DESCRIPTION
 *
 *    The density of the lognormal distribution.
 */


DOUBLE SUFFIX (go_dlnorm) (DOUBLE x, DOUBLE meanlog, DOUBLE sdlog, gboolean give_log)
{
    DOUBLE y;

#ifdef IEEE_754
    if (SUFFIX (isnan) (x) || SUFFIX (isnan) (meanlog) || SUFFIX (isnan) (sdlog))
	return x + meanlog + sdlog;
#endif
    if(sdlog <= 0) ML_ERR_return_NAN;

    if(x <= 0) return R_D__0;

    y = (SUFFIX (log) (x) - meanlog) / sdlog;
    return (give_log ?
	    -(M_LN_SQRT_2PI   + 0.5 * y * y + SUFFIX (log) (x * sdlog)) :
	    M_1_SQRT_2PI * SUFFIX (exp) (-0.5 * y * y)  /	 (x * sdlog));
    /* M_1_SQRT_2PI = 1 / SUFFIX (sqrt) (2 * pi) */

}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/plnorm.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000-8 The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  DESCRIPTION
 *
 *    The lognormal distribution function.
 */


DOUBLE SUFFIX (go_plnorm) (DOUBLE x, DOUBLE meanlog, DOUBLE sdlog, gboolean lower_tail, gboolean log_p)
{
#ifdef IEEE_754
    if (SUFFIX (isnan) (x) || SUFFIX (isnan) (meanlog) || SUFFIX (isnan) (sdlog))
	return x + meanlog + sdlog;
#endif
    if (sdlog <= 0) ML_ERR_return_NAN;

    if (x > 0)
	return SUFFIX (go_pnorm) (SUFFIX (log) (x), meanlog, sdlog, lower_tail, log_p);
    return lower_tail ? 0 : 1;
}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/qlnorm.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000-8 The R Development Core Team
 *  Copyright (C) 2005 The R Foundation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  DESCRIPTION
 *
 *    This the lognormal quantile function.
 */


DOUBLE SUFFIX (go_qlnorm) (DOUBLE p, DOUBLE meanlog, DOUBLE sdlog, gboolean lower_tail, gboolean log_p)
{
#ifdef IEEE_754
    if (SUFFIX (isnan) (p) || SUFFIX (isnan) (meanlog) || SUFFIX (isnan) (sdlog))
	return p + meanlog + sdlog;
#endif
    R_Q_P01_boundaries(p, 0, SUFFIX (go_pinf));

    return SUFFIX (exp) (SUFFIX (go_qnorm) (p, meanlog, sdlog, lower_tail, log_p));
}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/dweibull.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000-6 The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  DESCRIPTION
 *
 *    The density function of the Weibull distribution.
 */


DOUBLE SUFFIX (go_dweibull) (DOUBLE x, DOUBLE shape, DOUBLE scale, gboolean give_log)
{
    DOUBLE tmp1, tmp2;
#ifdef IEEE_754
    if (SUFFIX (isnan) (x) || SUFFIX (isnan) (shape) || SUFFIX (isnan) (scale))
	return x + shape + scale;
#endif
    if (shape <= 0 || scale <= 0) ML_ERR_return_NAN;

    if (x < 0) return R_D__0;
    if (!SUFFIX (go_finite)(x)) return R_D__0;
    /* need to handle x == 0 separately */
    if(x == 0 && shape < 1) return SUFFIX (go_pinf);
    tmp1 = SUFFIX (pow) (x / scale, shape - 1);
    tmp2 = tmp1 * (x / scale);
    /* These are incorrect if tmp1 == 0 */
    return  give_log ?
	-tmp2 + SUFFIX (log) (shape * tmp1 / scale) :
	shape * tmp1 * SUFFIX (exp) (-tmp2) / scale;
}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/pweibull.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000-2002 The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  DESCRIPTION
 *
 *    The distribution function of the Weibull distribution.
 */


DOUBLE SUFFIX (go_pweibull) (DOUBLE x, DOUBLE shape, DOUBLE scale, gboolean lower_tail, gboolean log_p)
{
#ifdef IEEE_754
    if (SUFFIX (isnan) (x) || SUFFIX (isnan) (shape) || SUFFIX (isnan) (scale))
	return x + shape + scale;
#endif
    if(shape <= 0 || scale <= 0) ML_ERR_return_NAN;

    if (x <= 0)
	return R_DT_0;
    x = -SUFFIX (pow) (x / scale, shape);
    if (lower_tail)
	return (log_p
		/* SUFFIX (log) (1 - SUFFIX (exp) (x))  for x < 0 : */
		? (x > -M_LN2goffice ? SUFFIX (log) (-SUFFIX (expm1) (x)) : SUFFIX (log1p) (-SUFFIX (exp) (x)))
		: -SUFFIX (expm1) (x));
    /* else:  !lower_tail */
    return R_D_exp(x);
}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/qweibull.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000 The R Development Core Team
 *  Copyright (C) 2005 The R Foundation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  DESCRIPTION
 *
 *    The quantile function of the Weibull distribution.
 */


DOUBLE SUFFIX (go_qweibull) (DOUBLE p, DOUBLE shape, DOUBLE scale, gboolean lower_tail, gboolean log_p)
{
#ifdef IEEE_754
    if (SUFFIX (isnan) (p) || SUFFIX (isnan) (shape) || SUFFIX (isnan) (scale))
	return p + shape + scale;
#endif
    if (shape <= 0 || scale <= 0) ML_ERR_return_NAN;

    R_Q_P01_boundaries(p, 0, SUFFIX (go_pinf));

    return scale * SUFFIX (pow) (- R_DT_Clog(p), 1./shape) ;
}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/dcauchy.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000 The R Development Core Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  DESCRIPTION
 *
 *    The density of the Cauchy distribution.
 */


DOUBLE SUFFIX (go_dcauchy) (DOUBLE x, DOUBLE location, DOUBLE scale, gboolean give_log)
{
    DOUBLE y;
#ifdef IEEE_754
    /* NaNs propagated correctly */
    if (SUFFIX (isnan) (x) || SUFFIX (isnan) (location) || SUFFIX (isnan) (scale))
	return x + location + scale;
#endif
    if (scale <= 0) ML_ERR_return_NAN;

    y = (x - location) / scale;
    return give_log ?
	- SUFFIX (log) (M_PIgoffice * scale * (1. + y * y)) :
	1. / (M_PIgoffice * scale * (1. + y * y));
}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/pcauchy.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998 Ross Ihaka
 *  Copyright (C) 2000 The R Development Core Team
 *  Copyright (C) 2004 The R Foundation
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, a copy is available at
 *  http://www.r-project.org/Licenses/
 *
 *  DESCRIPTION
 *
 *	The distribution function of the Cauchy distribution.
 */


DOUBLE SUFFIX (go_pcauchy) (DOUBLE x, DOUBLE location, DOUBLE scale,
	       gboolean lower_tail, gboolean log_p)
{
#ifdef IEEE_754
    if (SUFFIX (isnan) (x) || SUFFIX (isnan) (location) || SUFFIX (isnan) (scale))
	return x + location + scale;
#endif
    if (scale <= 0) ML_ERR_return_NAN;

    x = (x - location) / scale;
    if (SUFFIX (isnan) (x)) ML_ERR_return_NAN;
#ifdef IEEE_754
    if(!SUFFIX (go_finite)(x)) {
	if(x < 0) return R_DT_0;
	else return R_DT_1;
    }
#endif
    if (!lower_tail)
	x = -x;
    /* for large x, the standard formula suffers from cancellation.
     * This is from Morten Welinder thanks to  Ian Smith's  atan(1/x) : */
    if (SUFFIX (fabs) (x) > 1) {
	DOUBLE y = atan(1/x) / M_PIgoffice;
	return (x > 0) ? R_D_Clog(y) : R_D_val(-y);
    } else
	return R_D_val(0.5 + atan(x) / M_PIgoffice);
}

/* ------------------------------------------------------------------------ */
/* Imported src/nmath/qcauchy.c from R.  */
/*
 *  Mathlib : A C Library of Special Functions
 *  Copyright (C) 1998   Ross Ihaka
 *  Copyright (C) 2000   The R Development Core Team
 *  Copyright (C) 2005-6 The R Foundation
 *
 *  This version is based on a suggestion by Morten Welinder.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  http://www.gnu.org/copyleft/gpl.html.
 *
 *  DESCRIPTION
 *
 *	The quantile function of the Cauchy distribution.
 */


DOUBLE SUFFIX (go_qcauchy) (DOUBLE p, DOUBLE location, DOUBLE scale,
	       gboolean lower_tail, gboolean log_p)
{
#ifdef IEEE_754
    if (SUFFIX (isnan) (p) || SUFFIX (isnan) (location) || SUFFIX (isnan) (scale))
	return p + location + scale;
#endif
    R_Q_P01_check(p);
    if (scale <= 0 || !SUFFIX (go_finite)(scale)) {
	if (scale == 0) return location;
	/* else */ ML_ERR_return_NAN;
    }

#define my_INF location + (lower_tail ? scale : -scale) * SUFFIX (go_pinf)
    if (log_p) {
	if (p > -1) {
	    /* when ep := SUFFIX (exp) (p),
	     * SUFFIX (tan) (pi*ep)= -SUFFIX (tan) (pi*(-ep))= -SUFFIX (tan) (pi*(-ep)+pi) = -SUFFIX (tan) (pi*(1-ep)) =
	     *		 = -SUFFIX (tan) (pi*(-SUFFIX (expm1) (p))
	     * for p ~ 0, SUFFIX (exp) (p) ~ 1, SUFFIX (tan) (~0) may be better than SUFFIX (tan) (~pi).
	     */
	    if (p == 0.) /* needed, since 1/SUFFIX (tan) (-0) = -Inf  for some arch. */
		return my_INF;
	    lower_tail = !lower_tail;
	    p = -SUFFIX (expm1) (p);
	} else
	    p = SUFFIX (exp) (p);
    } else if (p == 1.)
	return my_INF;

    return location + (lower_tail ? -scale : scale) / SUFFIX (tan) (M_PIgoffice * p);
    /*	-1/SUFFIX (tan) (pi * p) = -cot(pi * p) = SUFFIX (tan) (pi * (p - 1/2))  */
}
/* Cleaning up done by tools/import-R:  */
#undef my_INF

/* ------------------------------------------------------------------------ */
/* --- END MAGIC R SOURCE MARKER --- */

/* ------------------------------------------------------------------------- */

// See comments at top
#endif // SKIP_THIS_PASS
#if INCLUDE_PASS < INCLUDE_PASS_LAST
#include __FILE__
#endif
