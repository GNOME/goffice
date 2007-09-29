#ifndef GOFFICE_FFT_H
#define GOFFICE_FFT_H

#include <goffice/math/go-complex.h>

G_BEGIN_DECLS

void go_fourier_fft (go_complex const *in, int n, int skip, go_complex **fourier, gboolean inverse);

#ifdef GOFFICE_WITH_LONG_DOUBLE

void go_fourier_fftl (go_complexl const *in, int n, int skip, go_complexl **fourier, gboolean inverse);

#endif

G_END_DECLS

#endif	/* GOFFICE_FFT_H */
