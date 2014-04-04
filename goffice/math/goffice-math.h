#ifndef __GOFFICE_MATH_H
#define __GOFFICE_MATH_H

/* Forward stuff */
typedef struct GOAccumulator_ GOAccumulator;
typedef struct GOQuad_ GOQuad;
typedef struct GOQuadMatrix_ GOQuadMatrix;
typedef struct GOQuadQR_ GOQuadQR;

#ifdef GOFFICE_WITH_LONG_DOUBLE
typedef struct GOAccumulatorl_ GOAccumulatorl;
typedef struct GOQuadl_ GOQuadl;
typedef struct GOQuadMatrixl_ GOQuadMatrixl;
typedef struct GOQuadQRl_ GOQuadQRl;
#endif

#include <goffice/math/go-accumulator.h>
#include <goffice/math/go-complex.h>
#include <goffice/math/go-cspline.h>
#include <goffice/math/go-distribution.h>
#include <goffice/math/go-dtoa.h>
#include <goffice/math/go-fft.h>
#include <goffice/math/go-math.h>
#include <goffice/math/go-matrix.h>
#include <goffice/math/go-matrix3x3.h>
#include <goffice/math/go-quad.h>
#include <goffice/math/go-R.h>
#include <goffice/math/go-rangefunc.h>
#include <goffice/math/go-regression.h>

#endif
