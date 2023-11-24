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

// We need multiple versions of this code.  We're going to include ourself
// with different settings of various macros.  gdb will hate us.
#include <goffice/goffice-multipass.h>
#ifndef SKIP_THIS_PASS

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

	argstep = (inverse ? DOUBLE_PI : -DOUBLE_PI) / nhalf;
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

/* ------------------------------------------------------------------------- */

// See comments at top
#endif // SKIP_THIS_PASS
#if INCLUDE_PASS < INCLUDE_PASS_LAST
#include __FILE__
#endif
