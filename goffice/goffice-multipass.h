// There should be no include guard for this file

// When we need multiple versions of some file -- one version per number
// system -- we include this file.  This sets up various macros on a per-
// number system basis.
//
// Note: gdb gets confused over having multiple functions defined on the
// same file/line.

#define INCLUDE_PASS_DOUBLE 1
#define INCLUDE_PASS_LONG_DOUBLE 2
#define INCLUDE_PASS_DECIMAL64 3
#define INCLUDE_PASS_LAST 3


#undef SKIP_THIS_PASS
#undef DEFINE_COMMON
#undef DOUBLE
#undef SUFFIX
#undef SUFFIX_STR
#undef INFIX
#undef DOUBLE_MANT_DIG
#undef DOUBLE_EPSILON
#undef DOUBLE_DIG
#undef DOUBLE_MIN
#undef DOUBLE_RADIX
#undef STRTO
#undef UNSCALBN
#undef CONST
#undef FORMAT_e
#undef FORMAT_f
#undef FORMAT_g
#undef FORMAT_E
#undef FORMAT_F
#undef FORMAT_G

#ifndef INCLUDE_PASS
  #define INCLUDE_PASS INCLUDE_PASS_DOUBLE
  #define DEFINE_COMMON
  // double
  #define DOUBLE double
  #define SUFFIX(_n) _n
  #define SUFFIX_STR ""
  #define INFIX(_a,_b) _a ## _b
  #define DOUBLE_MANT_DIG DBL_MANT_DIG
  #define DOUBLE_EPSILON DBL_EPSILON
  #define DOUBLE_DIG DBL_DIG
  #define DOUBLE_MIN DBL_MIN
  #define DOUBLE_RADIX FLT_RADIX
  #define STRTO go_strtod
  #define UNSCALBN frexp
  #define CONST(c) c
  #define FORMAT_e "e"
  #define FORMAT_f "f"
  #define FORMAT_g "g"
  #define FORMAT_E "E"
  #define FORMAT_F "F"
  #define FORMAT_G "G"
#elif INCLUDE_PASS == INCLUDE_PASS_DOUBLE
  #undef INCLUDE_PASS
  #define INCLUDE_PASS INCLUDE_PASS_LONG_DOUBLE
  #ifndef GOFFICE_WITH_LONG_DOUBLE
    #define SKIP_THIS_PASS
  #endif
  #define DOUBLE long double
  #define SUFFIX(_n) _n ## l
  #define SUFFIX_STR "l"
  #define INFIX(_a,_b) _a ## l ## _b
  #define DOUBLE_MANT_DIG LDBL_MANT_DIG
  #define DOUBLE_EPSILON LDBL_EPSILON
  #define DOUBLE_DIG LDBL_DIG
  #define DOUBLE_MIN LDBL_MIN
  #define DOUBLE_RADIX FLT_RADIX
  #define STRTO go_strtold
  #define UNSCALBN frexpl
  #define CONST(_c) _c ## l
  #define FORMAT_e "Le"
  #define FORMAT_f "Lf"
  #define FORMAT_g "Lg"
  #define FORMAT_E "LE"
  #define FORMAT_F "LF"
  #define FORMAT_G "LG"
#elif INCLUDE_PASS == INCLUDE_PASS_LONG_DOUBLE
  #undef INCLUDE_PASS
  #define INCLUDE_PASS INCLUDE_PASS_DECIMAL64
  #ifndef GOFFICE_WITH_DECIMAL64
    #define SKIP_THIS_PASS
  #endif
  #define DOUBLE _Decimal64
  #define SUFFIX(_n) _n ## D
  #define SUFFIX_STR "D"
  #define INFIX(_a,_b) _a ## D ## _b
  #define DOUBLE_MANT_DIG DECIMAL64_MANT_DIG
  #define DOUBLE_EPSILON DECIMAL64_EPSILON
  #define DOUBLE_DIG 16
  #define DOUBLE_MIN DECIMAL64_MIN
  #define DOUBLE_RADIX 10
  #define STRTO go_strtoDd
  #define UNSCALBN unscalbnD
  #define CONST(_c) _c ## dd
  #define FORMAT_e "We"
  #define FORMAT_f "Wf"
  #define FORMAT_g "Wg"
  #define FORMAT_E "WE"
  #define FORMAT_F "WF"
  #define FORMAT_G "WG"

  // There does not seem to be a way to teach these warnings about the
  // "W" modifier than we have hooked into libc's printf.  (Note: nothing
  // turns these off again.)
  #pragma GCC diagnostic ignored "-Wformat"
  #pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif

#if defined(GOFFICE_WITH_DECIMAL64)
#define LAST_INCLUDE_PASS (INCLUDE_PASS == INCLUDE_PASS_DECIMAL64)
#elif defined(GOFFICE_WITH_LONG_DOUBLE)
#define LAST_INCLUDE_PASS (INCLUDE_PASS == INCLUDE_PASS_LONG_DOUBLE)
#else
#define LAST_INCLUDE_PASS (INCLUDE_PASS == INCLUDE_PASS_DOUBLE)
#endif

#define DOUBLE_PI CONST(3.14159265358979323846264338327950288)
