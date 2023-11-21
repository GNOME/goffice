#ifndef GO_DECIMAL_H__
#define GO_DECIMAL_H__

#include <goffice/goffice-features.h>
#include <glib.h>

G_BEGIN_DECLS

#ifndef __GI_SCANNER__
// Introspection doens't understand this file due to _Decimal64

/* ------------------------------------------------------------------------- */

#define GO_DECIMAL64_MODIFIER "W"
#define GO_DECIMAL128_MODIFIER "WL"

#define DECIMAL64_MIN     1e-383dd
#define DECIMAL64_MAX     9.999999999999999e384dd
#define DECIMAL64_EPSILON 1e-15dd

#define M_PID 3.141592653589793dd



_Decimal64 acoshD (_Decimal64 x);
_Decimal64 asinhD (_Decimal64 x);
_Decimal64 atanhD (_Decimal64 x);
_Decimal64 ceilD (_Decimal64 x);
_Decimal64 cosD (_Decimal64 x);
_Decimal64 coshD (_Decimal64 x);
_Decimal64 expD (_Decimal64 x);
_Decimal64 expm1D (_Decimal64 x);
_Decimal64 fabsD (_Decimal64 x);
_Decimal64 floorD (_Decimal64 x);
_Decimal64 log10D (_Decimal64 x);
_Decimal64 log1pD (_Decimal64 x);
_Decimal64 logD (_Decimal64 x);
_Decimal64 nextafterD (_Decimal64 x, _Decimal64 y);
_Decimal64 powD (_Decimal64 x, _Decimal64 y);
_Decimal64 sinD (_Decimal64 x);
_Decimal64 sinhD (_Decimal64 x);
_Decimal64 sqrtD (_Decimal64 x);
_Decimal64 tanD (_Decimal64 x);
_Decimal64 tanhD (_Decimal64 x);
int finiteD (_Decimal64 x);
int isnanD (_Decimal64 x);
int signbitD (_Decimal64 x);


_Decimal64 strtoDd (const char *s, char **end);

/* private */
void _go_decimal_init (void);

/* ------------------------------------------------------------------------- */

#endif

G_END_DECLS

#endif	/* GO_DECIMAL_H__ */
