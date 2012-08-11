/*
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

#include <goffice-config.h>
#include <goffice/math/go-fft.h>

#ifndef DOUBLE

#define DOUBLE double
#define SUFFIX(_n) _n
#define M_PIgo    3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117

#ifdef GOFFICE_WITH_LONG_DOUBLE
#include "go-fft.c"
#undef DOUBLE
#undef SUFFIX
#undef M_PIgo

#ifdef HAVE_SUNMATH_H
#include <sunmath.h>
#endif
#define DOUBLE long double
#define SUFFIX(_n) _n ## l
#define M_PIgo    3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117L
#endif

#endif


void
SUFFIX(go_fourier_fft) (SUFFIX(go_complex) const *in, int n, int skip, SUFFIX(go_complex) **fourier, gboolean inverse)
{
	SUFFIX(go_complex)  *fourier_1, *fourier_2;
	int        i;
	int        nhalf = n / 2;
	DOUBLE argstep;

	*fourier = g_new (SUFFIX(go_complex), n);

	if (n == 1) {
		(*fourier)[0] = in[0];
		return;
	}

	SUFFIX(go_fourier_fft) (in, nhalf, skip * 2, &fourier_1, inverse);
	SUFFIX(go_fourier_fft) (in + skip, nhalf, skip * 2, &fourier_2, inverse);

	argstep = (inverse ? M_PIgo : -M_PIgo) / nhalf;
	for (i = 0; i < nhalf; i++) {
		SUFFIX(go_complex) dir, tmp;

		SUFFIX(go_complex_from_polar) (&dir, 1, argstep * i);
		SUFFIX(go_complex_mul) (&tmp, &fourier_2[i], &dir);

		SUFFIX(go_complex_add) (&((*fourier)[i]), &fourier_1[i], &tmp);
		SUFFIX(go_complex_scale_real) (&((*fourier)[i]), 0.5);

		SUFFIX(go_complex_sub) (&((*fourier)[i + nhalf]), &fourier_1[i], &tmp);
		SUFFIX(go_complex_scale_real) (&((*fourier)[i + nhalf]), 0.5);
	}

	g_free (fourier_1);
	g_free (fourier_2);
}
