noinst_LTLIBRARIES = libgoffice-math.la

libgoffice_math_la_SOURCES =	\
	go-math.c		\
	go-rangefunc.c		\
	go-regression.c		\
	go-cspline.c		\
	go-complex.c		\
	go-fft.c		\
	go-matrix3x3.c		\
	go-quad.c		\
	go-R.c			\
	go-distribution.c

libgoffice_math_ladir = $(goffice_include_dir)/math
libgoffice_math_la_HEADERS = 	\
	goffice-math.h		\
	go-math.h		\
	go-rangefunc.h		\
	go-regression.h		\
	go-cspline.h		\
	go-complex.h		\
	go-fft.h		\
	go-matrix3x3.h		\
	go-quad.h		\
	go-R.h			\
	go-distribution.h

include $(top_srcdir)/goffice.mk

LIB_PUBLIC_HDRS = $(libgoffice_math_la_HEADERS)
