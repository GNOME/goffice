#ifndef GO_DECIMAL_H__
#define GO_DECIMAL_H__

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <goffice/goffice.h>
#include <glib.h>

G_BEGIN_DECLS

/* ------------------------------------------------------------------------- */

#define GO_DECIMAL64_MODIFIER "W"
#define GO_DECIMAL128_MODIFIER "WL"

#define DECIMAL64_MIN      1e-383dd
#define DECIMAL64_MAX      9.999999999999999e384dd
#define DECIMAL64_MAX_EXP  384
#define DECIMAL64_MIN_EXP  (-383)
#define DECIMAL64_EPSILON  1e-15dd
#define DECIMAL64_DIG      16
#define DECIMAL64_MANT_DIG 16

#define M_PID 3.141592653589793dd

#ifndef __GI_SCANNER__
// Disable introspection for this.  The names clash with many go_foo
// functions.

_Decimal64 acosD (_Decimal64 x);
_Decimal64 acoshD (_Decimal64 x);
_Decimal64 asinD (_Decimal64 x);
_Decimal64 asinhD (_Decimal64 x);
_Decimal64 atanD (_Decimal64 x);
_Decimal64 atan2D (_Decimal64 x, _Decimal64 y);
_Decimal64 atanhD (_Decimal64 x);
_Decimal64 cbrtD (_Decimal64 x);
_Decimal64 ceilD (_Decimal64 x);
_Decimal64 copysignD (_Decimal64 x, _Decimal64 y);
_Decimal64 cosD (_Decimal64 x);
_Decimal64 coshD (_Decimal64 x);
_Decimal64 erfD (_Decimal64 x);
_Decimal64 erfcD (_Decimal64 x);
_Decimal64 expD (_Decimal64 x);
_Decimal64 expm1D (_Decimal64 x);
_Decimal64 fabsD (_Decimal64 x);
_Decimal64 floorD (_Decimal64 x);
_Decimal64 frexpD (_Decimal64 x, int *e);
_Decimal64 fmodD (_Decimal64 x, _Decimal64 y);
_Decimal64 hypotD (_Decimal64 x, _Decimal64 y);
_Decimal64 jnD (int n, _Decimal64 x);
_Decimal64 ldexpD (_Decimal64 x, int e);
_Decimal64 lgammaD (_Decimal64 x);
_Decimal64 lgammaD_r (_Decimal64 x, int *signp);
_Decimal64 log10D (_Decimal64 x);
_Decimal64 log1pD (_Decimal64 x);
_Decimal64 log2D (_Decimal64 x);
_Decimal64 logD (_Decimal64 x);
_Decimal64 modfD (_Decimal64 x, _Decimal64 *y);
_Decimal64 nextafterD (_Decimal64 x, _Decimal64 y);
_Decimal64 powD (_Decimal64 x, _Decimal64 y);
_Decimal64 roundD (_Decimal64 x);
_Decimal64 sinD (_Decimal64 x);
_Decimal64 sinhD (_Decimal64 x);
_Decimal64 scalblnD (_Decimal64 x, long e);
_Decimal64 scalbnD (_Decimal64 x, int e);
_Decimal64 sqrtD (_Decimal64 x);
_Decimal64 tanD (_Decimal64 x);
_Decimal64 tanhD (_Decimal64 x);
_Decimal64 truncD (_Decimal64 x);
_Decimal64 unscalbnD (_Decimal64 x, int *e);
_Decimal64 ynD (int n, _Decimal64 x);
int finiteD (_Decimal64 x);
int isnanD (_Decimal64 x);
int signbitD (_Decimal64 x);

_Decimal64 strtoDd (const char *s, char **end);

#endif

/* private */
void _go_decimal_init (void);
void _go_decimal_shutdown (void);

/* ------------------------------------------------------------------------- */

G_END_DECLS

#endif	/* GO_DECIMAL_H__ */
