>-*- mode: m4 -*-
dnl We require Automake 1.7.2, which requires Autoconf 2.54.
dnl (It needs _AC_AM_CONFIG_HEADER_HOOK, for example.)
AC_PREREQ([2.54])
AC_CONFIG_MACRO_DIR([m4])
AX_IS_RELEASE([git-directory])

m4_define([goffice_version_epoch], [0])
m4_define([goffice_version_major], [10])
m4_define([goffice_version_minor], [60])
m4_define([goffice_version_extra], [])
m4_define([goffice_full_version],
	  [goffice_version_epoch.goffice_version_major.goffice_version_minor[]goffice_version_extra])

dnl Emphasize some of the checks.
m4_define([BIG_CHECKING], [AC_MSG_CHECKING([
======== $1])])

AC_INIT([goffice],[goffice_full_version],[https://gitlab.gnome.org/GNOME/goffice/issues])

AC_CONFIG_SRCDIR(goffice/goffice.h)
AM_INIT_AUTOMAKE([dist-bzip2])

dnl Version info for libraries = CURRENT:REVISION:AGE
dnl
dnl Within each x.y.*, ABI is maintained backward and _forward_ compatible.
dnl (As a consequence, no exported function may be added.)
dnl So it's enough to have one interface number per each x.y.* branch.
dnl
dnl OTOH, we are not able to keep ABI strictly backward compatible throughout
dnl the whole x.*.*.
dnl The easiest way is to declare no ABI compatibility, ie. AGE is always 0.
dnl
m4_define([version_iface],
	  m4_eval(100 * goffice_version_epoch + goffice_version_major))

AC_SUBST([VERSION_INFO], [version_iface:goffice_version_minor:0])
AC_SUBST([VERSION_IFACE], [version_iface])

if test `expr goffice_version_major % 2` -eq 1; then
    AC_MSG_NOTICE([NOTE: This is a development release])
    goffice_devel=yes
else
    goffice_devel=no
fi

if test `expr goffice_version_major % 2` -eq 1; then
    AC_MSG_NOTICE([NOTE: This is a development release])
    goffice_devel=yes
    goffice_api_ver=goffice_version_epoch.`expr goffice_version_major + 1`
    dnl An explicit version number should be used in place of the above
    dnl if the epoch is to change.
    dnl goffice_api_ver=1.0
else
    goffice_devel=no
    goffice_api_ver=goffice_version_epoch.goffice_version_major
fi
goffice_api_ver_=`echo $goffice_api_ver | sed -e 's/\./_/g'`

dnl Almost like epoch.major but development versions look forward to the next
dnl stable release.
AC_SUBST([GOFFICE_API_VER], [$goffice_api_ver])
AC_SUBST([GOFFICE_API_VER_], [$goffice_api_ver_])

dnl This one is created by autoheader, ...
AC_CONFIG_HEADERS(goffice/goffice-config.h)

dnl ... and this one is a small subset, maintained manually,
dnl which will be installed.
AC_CONFIG_HEADERS(goffice/goffice-features.h)
dnl
dnl Make sure these two won't clash.  Put the following to goffice-config.h:
AH_BOTTOM([/* Don't include goffice-features.h, it's a subset of goffice-config.h. */
#define GOFFICE_FEATURES_H])

AM_MAINTAINER_MODE([enable])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

dnl We need intltool >= 0.35.0 to use the LINGUAS file in the po dir
IT_PROG_INTLTOOL([0.35.0])

dnl We use $host_os:
AC_CANONICAL_HOST

AC_PROG_CC_C99
AC_PROG_CC
if test "$ac_cv_prog_cc_c99" = no; then
	AC_MSG_ERROR([*** c99 is required])
fi
AC_PROG_LN_S

AC_SYS_LARGEFILE

LT_PREREQ([2.2.6])
# Make --disable-static the default
LT_INIT([disable-static])

dnl Propagate Gnome-specific variable ACLOCAL_FLAGS to Makefile.
AC_SUBST(ACLOCAL_AMFLAGS, $ACLOCAL_FLAGS)

dnl The first call to PKG_CHECK_MODULES is inside an `if.'
dnl We have to call PKG_PROG_PKG_CONFIG explicitly; otherwise it would
dnl get expanded just before the first occurence of PKG_CHECK_MODULES
dnl and might escape execution.
dnl
dnl While we're at it, we might require a version that supports private libraries
dnl	PKG_PROG_PKG_CONFIG([0.18])
dnl but there are two problems:
dnl 1) we cannot rule out something as new as SuSE 9.3
dnl 2) current versions of PKG_PROG_PKG_CONFIG choke on old versions and woe a
dnl	"syntax error" instead of "your pkgconfig/pkg-config is too old".
dnl Let it be, for now.
PKG_PROG_PKG_CONFIG

dnl *****************************
goffice_reqs="
	glib-2.0		>= 2.40.0
	gobject-2.0		>= 2.40.0
	gmodule-2.0		>= 2.40.0
	gio-2.0			>= 2.40.0
	libgsf-1		>= 1.14.24
	libxml-2.0		>= 2.4.12
	pango			>= 1.24.0
	pangocairo		>= 1.24.0
	cairo			>= 1.10.0
	gdk-pixbuf-2.0	>= 2.22.0
	libxslt
"

goffice_gtk_reqs="
	gtk+-3.0		>= 3.8.7
"

surfaces_reqs="
	cairo-ps		>= 1.10.0
	cairo-pdf		>= 1.10.0
	cairo-svg		>= 1.10.0
"

PKG_CHECK_MODULES(surfaces, $surfaces_reqs)

EXTRA_DEPS=

dnl ***********************
dnl Should we use lasem ?
dnl ***********************

goffice_with_lasem=false
lasem=
for ver in 0.6 0.4 ; do
  if test "x$lasem" = x; then
    if $PKG_CONFIG --exists lasem-$ver; then
      lasem=lasem-$ver
    fi
  fi
done
if test "x$lasem" = x; then
  # Not important.  Things will fail below.
  lasem=lasem-0.4
fi
AC_ARG_WITH(lasem,
	AS_HELP_STRING([--with-lasem[=@<:@no/auto/yes@:>@]],
                         [Build with lasem use (default=auto)]),
	,[with_lasem=auto])

AS_CASE([$with_lasem],
	[no],[found_lasem="disabled by request"],
	[auto], [
	PKG_CHECK_MODULES(Lasem, $lasem >= 0.4.1,
	[found_lasem="yes";goffice_with_lasem=true], [found_lasem="no, missing or too old lasem library"])
	],
	[yes],[found_lasem="yes";
	PKG_CHECK_MODULES(Lasem, $lasem >= 0.4.1);
	goffice_with_lasem=true
	],[AC_MSG_ERROR([invalid argument passed to --with-lasem, should be one of @<:@no/auto/yes@:>@])])
if test "x$goffice_with_lasem" = "xtrue" ; then
	AC_DEFINE(GOFFICE_WITH_LASEM, 1, [Define if Lasem is used])
	goffice_reqs="$goffice_reqs $lasem >= 0.4.1 "
	EXTRA_DEPS="$EXTRA_DEPS $lasem"
dnl check for lsm_itex_to_mathml
	saved_CFLAGS=$CFLAGS
	saved_LIBS=$LIBS
	CFLAGS="$CFLAGS $Lasem_CFLAGS"
	LIBS="$LIBS $Lasem_LIBS"
	AC_CHECK_FUNCS(lsm_itex_to_mathml)
	CFLAGS=$saved_CFLAGS
	LIBS=$saved_LIBS
fi

AM_CONDITIONAL(GOFFICE_WITH_LASEM, $goffice_with_lasem)

dnl ***************************
dnl Should we use libspectre ?
dnl ***************************

goffice_with_eps=false
LIBGS_REQUIRED="9.06"
LIBSPECTRE_REQUIRED="0.2.6"

AC_CHECK_LIB(gs, gsapi_new_instance, have_libgs=yes, have_libgs=no)
if test "x$have_libgs" = "xyes"; then
    LIB_GS="-lgs"
    save_LIBS=$LIBS
    LIBS="$LIBS -lgs"
    AC_LANG_PUSH(C)
    AC_MSG_CHECKING([for libgs >= $LIBGS_REQUIRED])
    AC_RUN_IFELSE([AC_LANG_PROGRAM(
[[
#include <ghostscript/iapi.h>
]],
[[
    gsapi_revision_t gsrev;

    if (gsapi_revision (&gsrev, sizeof (gsrev)) != 0)
        return 1;
    if (gsrev.revision < `echo "$LIBGS_REQUIRED" | sed -e 's/\.//'`)
        return 1;
]])],
                   [have_libgs=yes],
                   [have_libgs=no],
                   [have_libgs=yes]
    )
    AC_MSG_RESULT($have_libgs)
    AC_LANG_POP(C)
    LIBS=$save_LIBS
    if test "x$have_libgs" = "xno"; then
        echo "You need libgs >= $LIBGS_REQUIRED in order to support embedded EPS"
    else
		PKG_CHECK_MODULES(libspectre, libspectre >= $LIBSPECTRE_REQUIRED,[goffice_with_eps=true], [goffice_with_eps=false])
		if test "x$goffice_with_eps" = "xfalse"; then
	        echo "You need libspectre >= $LIBSPECTRE_REQUIRED in order to support embedded EPS"
		fi
    fi
else
    echo "You need libgs in order to support embedded EPS"
fi
if test "x$goffice_with_eps" = "xtrue" ; then
	AC_DEFINE(GOFFICE_WITH_EPS, 1, [Define if EPS is supported])
	goffice_reqs="$goffice_reqs libspectre >= $LIBSPECTRE_REQUIRED"
	EXTRA_DEPS="$EXTRA_DEPS libspectre"
fi

AM_CONDITIONAL(GOFFICE_WITH_EPS, $goffice_with_eps)

dnl *****************************
AC_MSG_CHECKING([for Win32 platform])
with_win32=no
with_native_win32=no
case $host_os in
  mingw* | pw32* | cygwin*)
    with_win32=yes
    GOFFICE_PLUGIN_LDFLAGS="-Wl,--enable-runtime-pseudo-reloc,--export-all-symbols $GOFFICE_PLUGIN_LDFLAGS"
    ;;
esac
AC_MSG_RESULT($with_win32)
AM_CONDITIONAL(WITH_WIN32, test $with_win32 = yes)

AC_MSG_CHECKING([for Win32 platform with no Cygwin])
case $host_os in
  mingw* | pw32*)
    with_native_win32=yes
    AC_ARG_VAR(WINDRES, [The windres executable (used by win32 builds only).])
    AC_CHECK_TOOL(WINDRES, windres)
    AC_ARG_VAR(LIBEXE, [The lib.exe executable (used by win32 builds only).])
    AC_CHECK_PROG(LIBEXE, lib.exe, yes, no)
    ;;
esac
AC_MSG_RESULT($with_native_win32)
AM_CONDITIONAL(WITH_NATIVE_WIN32, test $with_native_win32 = yes)
AM_CONDITIONAL(HAVE_LIBEXE, test x$LIBEXE = xyes)

dnl ================================================================
dnl check for osx platform
dnl ================================================================
os_osx=no
AC_MSG_CHECKING([for Mac OS X platform])
case "$host" in
  rhapsody* | darwin[1567].*)
    AC_MSG_RESULT([darwin, but too old])
    AC_MSG_ERROR([This platform is not supported, please update to latest  darwin])
    ;;
  *-*-darwin*)
    platform_osx=yes
    ;;
  *)
    platform_osx=no
    ;;
esac
AC_MSG_RESULT([$platform_osx])
AM_CONDITIONAL(PLATFORM_OSX, test "$platform_osx" = "yes")
if test "x$platform_osx" = xyes; then
  AC_DEFINE([OS_OSX],[1],[Defined if system is Mac OS X])
  dnl check for native osx
  gdk_windowing=`$PKG_CONFIG --variable=target gdk-2.0`
  AC_MSG_CHECKING([for GDK-Quartz])

  if test "x$gdk_windowing" = xquartz; then
        platform=darwin/quartz
        AC_MSG_RESULT(yes)
        AC_DEFINE(GDK_QUARTZ,[1],[Using GDK Quartz (not X11)])
        PKG_CHECK_MODULES(GTK_MAC, gtk-mac-integration, [os_osx=yes], [os_osx=no])
        dnl AC_MSG_RESULT(${os_osx})
        if test "x$os_osx" = xyes; then
	      AC_DEFINE([GTKOSXAPPLICATION],[1],[Defined if app is GtkOSXApplication])
	      AC_SUBST(GTK_MAC_LIBS)
	      AC_SUBST(GTK_MAC_CFLAGS)
        fi
  else
	AC_MSG_RESULT(no)
    os_osx=no
  fi
else
    os_osx=no
fi


dnl ***********************************
dnl Which conf backend should be used?
dnl ***********************************
AC_ARG_WITH(config-backend, [  --with-config-backend=gsettings|keyfile|win32           Choose the config backend ])
case $with_config_backend in
	win32)
		if test $with_win32 = no; then AC_MSG_ERROR([ win32 configuration backend only works for win32 builds ]);fi;
		AC_DEFINE(GOFFICE_WITH_WINREG, 1, [Goffice uses Windows registry])
		conf_msg="Windows registry"
		;;
	gsettings)
		AC_CHECK_LIB(gio-2.0,[g_settings_new],
		[
			AC_DEFINE(GOFFICE_WITH_GSETTINGS, 1, [Goffice uses gsettings])
			conf_msg="GSettings"
		],[
			AC_MSG_ERROR([ GSettings not available ])
		])
		;;
	keyfile)
		conf_msg="keyfile"
		;;
	*)
		if test $with_win32 = yes; then
			AC_DEFINE(GOFFICE_WITH_WINREG, 1, [Goffice uses Windows registry])
			conf_msg="Windows registry"
		else
			AC_DEFINE(GOFFICE_WITH_GSETTINGS, 1, [Goffice uses gsettings])
			conf_msg="GSettings"
		fi
		;;
esac

dnl ***********************
dnl Should we use librsvg ?
dnl ***********************
AC_ARG_WITH(librsvg,
	AS_HELP_STRING([--without-librsvg],[Build without SVG support]))
if ! test "x$with_librsvg" = xno; then
	goffice_reqs="$goffice_reqs librsvg-2.0 >= 2.22.0"
	EXTRA_DEPS="$EXTRA_DEPS librsvg-2.0"
	AC_DEFINE(GOFFICE_WITH_LIBRSVG, 1, [Define if librsvg is used])
fi
AM_CONDITIONAL(WITH_LIBRSVG, [! test "x$with_librsvg" = xno])


dnl *******************
dnl Should we use gtk ?
dnl *******************
goffice_with_gtk=true
AC_ARG_WITH(gtk, [  --without-gtk           Build without UI])
if test "x$with_gtk" = xno; then
	ui_msg="None (Gtk disabled by request)"
	goffice_with_gtk=false
else
	dnl We shouldn't silently default to --without-gtk.
	dnl If the requirements are not met, fail.
	ui_msg="Gtk"
	PKG_CHECK_MODULES(GTK, [$goffice_gtk_reqs])
fi

if test "x$goffice_with_gtk" = "xtrue" ; then
	AC_DEFINE(GOFFICE_WITH_GTK, 1, [Define if UI is built])
	goffice_reqs="$goffice_reqs $goffice_gtk_reqs"
fi

AM_CONDITIONAL(WITH_GTK,   $goffice_with_gtk)

AC_DEFINE(GOFFICE_WITH_CAIRO, 1, [Goffice uses Cairo])

EXTRA_LIBS=
EXTRA_INCLUDES=

dnl ****************************
dnl now that we have selected out libraries the whole collection in one
dnl shot so that we can have a nice neat compile/link line
dnl ****************************

PKG_CHECK_MODULES(GOFFICE, $goffice_reqs)

dnl *****************************
dnl FIXME: perhaps declare with AC_ARG_VAR?
dnl	   If we use the initial value of a variable, we have to make it precious.
dnl
GOFFICE_PLUGIN_LDFLAGS="-avoid-version -no-undefined $GOFFICE_PLUGIN_LDFLAGS"
GOFFICE_PLUGIN_LIBADD="\$(top_builddir)/goffice/libgoffice-\$(GOFFICE_API_VER).la $GOFFICE_LIBS $GOFFICE_PLUGIN_LIBADD"
AC_SUBST(GOFFICE_PLUGIN_LDFLAGS)
AC_SUBST(GOFFICE_PLUGIN_LIBADD)

dnl **************************************************
dnl This needs to be done before we add aggressive -Werror=foo flags
dnl below.
AC_SUBST(GETTEXT_PACKAGE, goffice-${VERSION})
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
	[The package name, for gettext])
AM_GNU_GETTEXT_VERSION([0.13])
AM_GNU_GETTEXT([external])

dnl ****************************
dnl prep the pixmap generator
dnl ****************************

AC_ARG_VAR(GLIB_GENMARSHAL, [The glib-genmarshal executable.])
AC_CHECK_PROG(GLIB_GENMARSHAL, glib-genmarshal, glib-genmarshal)

AC_ARG_VAR(GDK_PIXBUF_CSOURCE, [The gdk-pixbuf-csource executable.])
AC_CHECK_PROG(GDK_PIXBUF_CSOURCE, gdk-pixbuf-csource, gdk-pixbuf-csource)

AC_ARG_VAR(PERL, [The Perl executable.])
AC_CHECK_PROG(PERL, perl, perl)

## this should come after `AC_PROG_CC'
set_more_warnings=yes
if test "$GCC" = yes -a "x$set_more_warnings" != xno; then
	dnl Clang needs this option, or else it will appear to support any
	dnl warning option, only to spew warnings about them later.
	uwoption="-Werror=unknown-warning-option"
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $uwoption"
	AC_MSG_CHECKING([whether gcc understands $uwoption])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([], [])],
		[has_unknown_warning_option=yes],
		[has_unknown_warning_option=no; uwoption=""])
	AC_MSG_RESULT($has_unknown_warning_option)
	CFLAGS="$SAVE_CFLAGS"

	warning_options="-Wall -Werror=init-self -Werror=missing-include-dirs \
			 -Wsign-compare -Werror=pointer-arith \
			 -Wchar-subscripts -Wwrite-strings \
			 -Wnested-externs \
			 -Wmissing-noreturn \
			 -Werror=missing-prototypes -Werror=nested-externs \
			 -Werror=implicit-function-declaration \
			 -Wmissing-declarations -Wno-pointer-sign \
			 -Werror=format-security -Wbitwise -Wcast-to-as \
			 -Wdefault-bitfield-sign -Wdo-while -Wparen-string \
			 -Wptr-subtraction-blows -Wreturn-void -Wtypesign \
			 -Wstrict-prototypes -Wno-error=format-nonliteral "
	for option in $warning_options ; do
		SAVE_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS $uwoption $option"
		AC_MSG_CHECKING([whether gcc understands $option])
		dnl Include a system header so we ignore Werror=... flags
		dnl that cause trouble.
		AC_COMPILE_IFELSE(
			[AC_LANG_PROGRAM([[#include <stdio.h>]], [])],
			[has_option=yes],
			[has_option=no])
		CFLAGS="$SAVE_CFLAGS"
		if test $has_option = yes; then
			CFLAGS="$CFLAGS $option"
		fi
		AC_MSG_RESULT($has_option)
		unset has_option
		unset SAVE_CFLAGS
	done
	unset option
fi
AC_SUBST(WARN_CFLAGS)

AC_CHECK_DECL(fdopen, fdopen_works=yes, fdopen_works=no)
if test $fdopen_works = no ; then
	unset ac_cv_have_decl_fdopen
	CFLAGS="$CFLAGS -D_POSIX_SOURCE"
	AC_MSG_NOTICE([adding -D_POSIX_SOURCE to CFLAGS])
	AC_CHECK_DECL(fdopen, fdopen_works=yes, fdopen_works=no)
	if test $fdopen_works = no ; then
		AC_MSG_ERROR([fdopen is not available])
	fi
fi

dnl M_PI
AC_MSG_CHECKING([whether M_PI is available])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([#include <math.h>], [double f = M_PI])],
	[works_without_bsd_source=yes],
	[works_without_bsd_source=no])
AC_MSG_RESULT($works_without_bsd_source)

if test $works_without_bsd_source = no ; then
	CFLAGS="$CFLAGS -D_BSD_SOURCE"
	AC_MSG_CHECKING([whether M_PI is available with -D_BSD_SOURCE])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([#include <math.h>], [double f = M_PI])],
		[m_pi_works=yes],
		[m_pi_works=no])
	AC_MSG_RESULT($m_pi_works)
	if test $m_pi_works = no ; then
		AC_MSG_ERROR([M_PI is not available])
	fi
fi

dnl
dnl On Solaris finite() needs ieeefp.h
dnl Either of these seem to signal IEEE754 math, see goffice/math/go-math.c
dnl
AC_CHECK_HEADERS(ieeefp.h ieee754.h)

dnl Check for implementations of passwd/group file entry functions
AC_CHECK_HEADERS(pwd.h grp.h)

AC_CHECK_HEADERS(fpu_control.h)

dnl Check for some functions
AC_CHECK_FUNCS(random drand48 finite memmove mkdtemp uname times sysconf)

dnl FIXME: Does this really belong here?
AC_CHECK_FUNC(bind_textdomain_codeset,,[AC_CHECK_LIB(intl,bind_textdomain_codeset)])

dnl isfinite is a macro on HPUX
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([#include <math.h>], [int a = isfinite(0.0)])],
	[AC_DEFINE(HAVE_ISFINITE, 1,
		[Define if the isfinite() macro is available]
	 )
	], [])

dnl
dnl On BSD, we seem to need -lm for finite
dnl
if test $ac_cv_func_finite = no; then
  AC_CHECK_LIB(m, finite,
        [AC_DEFINE(HAVE_FINITE, 1,
		[Define if the finite function is available]
	 )
         LIBS="$LIBS -lm"])
fi

dnl check for complete locale implementation
AC_CHECK_HEADERS(langinfo.h)

dnl Handle systems that have stuff in -lm.
AC_CHECK_FUNCS(log)
if test $ac_cv_func_log = no; then
  AC_CHECK_LIB(m, log,
        [AC_DEFINE(HAVE_LOG, 1,
		[Define if the log function is available]
	 )
         LIBS="$LIBS -lm"])
fi

AC_CHECK_FUNCS(nextafter nextafterl rint)

AC_DEFUN([GOFFICE_CHECK_FUNC],
[AC_CHECK_FUNC([$1],
	[],
	[AC_DEFINE([GOFFICE_SUPPLIED_]AS_TR_CPP([$1]),
		1,
		[Define if GOffice supplies $1.])])dnl
])
GOFFICE_CHECK_FUNC(log1p)
GOFFICE_CHECK_FUNC(expm1)
GOFFICE_CHECK_FUNC(asinh)
GOFFICE_CHECK_FUNC(acosh)
GOFFICE_CHECK_FUNC(atanh)

AC_TYPE_LONG_DOUBLE
dnl go-ryu needs __uint128_t for long double support.
AC_CHECK_TYPES([__uint128_t], [], [], [[#include <inttypes.h>
#include <stdint.h>]])
if test $ac_cv_type_long_double.$ac_cv_type___uint128_t = yes.yes; then
  with_long_double_default=yes
else
  with_long_double_default=no
fi

float_msg=no
AC_ARG_WITH(long-double,
        [  --without-long-double   disable support for long double],
        , [ with_long_double=$with_long_double_default])

if test "x$with_long_double" = "xyes"; then
  if test $with_long_double_default = yes; then
    have_mandatory_funcs=yes
    need_sunmath=0
    ss_ccprog=`echo $CC | sed -e 's/ .*//'`
    ss_cc=`command -v $ss_ccprog`
    ss_dir=`dirname "$ss_cc"`"/.."
    if test ! -f "$ss_dir/lib/libsunmath.so"; then
        ss_dir="/opt/SUNWspro"
    fi
    sunmathlinkstuff="-L$ss_dir/lib -R$ss_dir/lib -lsunmath"
    for ldfunc in fabsl logl log10l ceill floorl powl isnanl; do
	    AC_CHECK_FUNC($ldfunc,
		  ,
		  [AC_CHECK_LIB(m,
		      $ldfunc,
		      ,
		      [AC_CHECK_LIB(sunmath,
			  $ldfunc,
			  [ if test $need_sunmath = 0; then
				# FIXME: better idea?
				LDFLAGS="$LDFLAGS $sunmathlinkstuff"
				sunmathinclude=`ls -d $ss_dir/*/include/cc | sed '$!d'`
				CPPFLAGS="$CPPFLAGS -I$sunmathinclude"
			    fi
			    need_sunmath=1 ],
			  [have_mandatory_funcs=no],
			  [-L$ss_dir/lib $GOFFICE_LIBS])])])
    done
    if test $need_sunmath = 1; then
	EXTRA_LIBS="$EXTRA_LIBS $sunmathlinkstuff"
	EXTRA_INCLUDES="$EXTRA_INCLUDES -I$sunmathinclude"
	AC_CHECK_HEADERS([sunmath.h floatingpoint.h],
			 ,
			 [AC_MSG_WARN([Long doubles require the $ac_header header.])
			  have_mandatory_funcs=no])
    fi
    unset ss_cc
    unset ss_dir
    unset need_sunmath
    unset sunmathlinkstuff
    unset sunmathinclude

    GOFFICE_CHECK_FUNC(modfl)
    GOFFICE_CHECK_FUNC(ldexpl)
    GOFFICE_CHECK_FUNC(frexpl)
    AC_CHECK_FUNCS(finitel)

    AC_CHECK_FUNCS(strtold)
    if test "$ac_cv_func_strtold" = yes; then
	AC_MSG_CHECKING([if we must prototype strtold ourselves])
	AC_RUN_IFELSE(
		[AC_LANG_PROGRAM([[#include <stdlib.h>]],
			[[
				const char *s = "+3.1415e+0";
				char *theend;
				long double res = strtold (s, &theend);
				return !(*theend == 0 && res >= 3.14 && res <= 3.15);
			]])],
		[AC_MSG_RESULT(no)],
		[AC_MSG_RESULT(yes)
		 AC_DEFINE([GOFFICE_SUPPLIED_STRTOLD], 1,
			   [Define if GOffice supplies strtold.])
		],
		[AC_MSG_RESULT(assuming not)])
    else
	AC_DEFINE([GOFFICE_SUPPLIED_STRTOLD], 1,
		  [Define if GOffice supplies strtold.])
	AC_CHECK_FUNCS(string_to_decimal decimal_to_quadruple)
	if test "x$ac_cv_func_string_to_decimal" != "xyes" || \
	   test "x$ac_cv_func_decimal_to_quadruple" != "xyes" || \
	   test "x$ac_cv_header_floatingpoint_h" != "xyes"; then
		AC_MSG_WARN([You lack the strtold function -- precision will be impaired])
	fi
    fi
    if test "$have_mandatory_funcs" = yes; then
	float_msg=yes
	AC_DEFINE([GOFFICE_WITH_LONG_DOUBLE], 1,
		  [Define if GOffice supports long double.])
    else
	AC_MSG_WARN([Long double support disabled because of library problems])
    fi
    unset have_mandatory_funcs
  fi
fi

decimal_msg=no
goffice_with_decimal64=false
AC_ARG_WITH(decimal64,
        [  --without-decimal64   disable support for decimal64],
        , [ with_decimal64=no])
AC_MSG_CHECKING([if Decimal64 support is requested])
if test "x$with_decimal64" = "xyes"; then
  AC_MSG_RESULT(yes)
  AC_CHECK_TYPES([_Decimal64], [], [AC_MSG_ERROR([_Decimal64 type required])], [])
  AC_CHECK_TYPES([struct printf_info], [], [AC_MSG_ERROR([printf hook support is required])], [[#include <printf.h>]])
  goffice_with_decimal64=true
  decimal_msg="yes (Decimal64)"
  AC_DEFINE([GOFFICE_WITH_DECIMAL64], 1,
	    [Define if GOffice supports Decimal64.])
else
  AC_MSG_RESULT(no)
fi
AM_CONDITIONAL(GOFFICE_WITH_DECIMAL64, $goffice_with_decimal64)

AC_SUBST(EXTRA_LIBS)
AC_SUBST(EXTRA_INCLUDES)
AC_SUBST(EXTRA_DEPS)

SAVE_CFLAGS=$CFLAGS
SAVE_LIBS=$LIBS
CFLAGS="$CFLAGS $GOFFICE_CFLAGS"
LIBS="$GOFFICE_LIBS $LIBS"
AC_CHECK_FUNCS(g_file_info_get_modification_date_time gtk_widget_class_set_css_name gdk_display_get_monitor_at_window)
CFLAGS=$SAVE_CFLAGS
LIBS=$SAVE_LIBS

# GObject Introspection
GIR_REQ=1.0.0
AC_ARG_ENABLE(introspection,
          AS_HELP_STRING([--enable-introspection[=@<:@no/auto/yes@:>@]],
                         [Enable introspection for this build]),,
                         [enable_introspection=no])

AC_MSG_CHECKING([for gobject-introspection])

dnl presence/version checking
AS_CASE([$enable_introspection],
[no], [
    found_introspection="no (disabled, use --enable-introspection to enable)"
],
[yes],[
    PKG_CHECK_EXISTS([gobject-introspection-1.0],,
                     AC_MSG_ERROR([gobject-introspection-1.0 is not installed]))
    PKG_CHECK_EXISTS([gobject-introspection-1.0 >= $GIR_REQ libgsf-1 >= 1.14.23],
                     found_introspection=yes,
                     AC_MSG_ERROR([You need to have gobject-introspection >= $GIR_REQ and libgsf >= 1.14.23 installed to build AC_PACKAGE_NAME]))
],
[auto],[
    PKG_CHECK_EXISTS([gobject-introspection-1.0 >= $GIR_REQ libgsf-1 >= 1.14.23],
                     found_introspection=yes, found_introspection=no)
dnl Canonicalize enable_introspection
enable_introspection=$found_introspection
],
[
    AC_MSG_ERROR([invalid argument passed to --enable-introspection, should be one of @<:@no/auto/yes@:>@])
])

AC_MSG_RESULT([$found_introspection])

if test "x$found_introspection" = "xyes"; then
   dnl You can override INTROSPECTION_GIRDIR and INTROSPECTION_TYPELIBDIR
   dnl if you wish.  If you override the latter, you might want to set
   dnl GI_TYPELIB_PATH to include the same directory.  For example
   dnl
   dnl GI_TYPELIB_PATH=$PREFIX/lib64/girepository-1.0
   dnl INTROSPECTION_TYPELIBDIR=$GI_TYPELIB_PATH
   dnl INTROSPECTION_GIRDIR=$PREFIX/share/gir-1.0
   dnl
   dnl Note, that unlike binaries produced with libgoffice, nothing tells
   dnl python where to find libgoffice, so you might also need to set
   dnl LD_LIBRARY_PATH.

   INTROSPECTION_SCANNER=`$PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0`
   INTROSPECTION_COMPILER=`$PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0`
   INTROSPECTION_GENERATE=`$PKG_CONFIG --variable=g_ir_generate gobject-introspection-1.0`
   if test "x$INTROSPECTION_GIRDIR" = x; then
     INTROSPECTION_GIRDIR=`$PKG_CONFIG --variable=girdir gobject-introspection-1.0`
   fi
   if test "x$INTROSPECTION_TYPELIBDIR" = x; then
     INTROSPECTION_TYPELIBDIR="$($PKG_CONFIG --variable=typelibdir gobject-introspection-1.0)"
   fi
   INTROSPECTION_CFLAGS=`$PKG_CONFIG --cflags gobject-introspection-1.0`
   INTROSPECTION_LIBS=`$PKG_CONFIG --libs gobject-introspection-1.0`
   INTROSPECTION_MAKEFILE=`$PKG_CONFIG --variable=datadir gobject-introspection-1.0`/gobject-introspection-1.0/Makefile.introspection
else
   INTROSPECTION_SCANNER=
   INTROSPECTION_COMPILER=
   INTROSPECTION_GENERATE=
   INTROSPECTION_GIRDIR=
   INTROSPECTION_TYPELIBDIR=
   INTROSPECTION_CFLAGS=
   INTROSPECTION_LIBS=
   INTROSPECTION_MAKEFILE=
fi
AC_SUBST(INTROSPECTION_SCANNER)
AC_SUBST(INTROSPECTION_COMPILER)
AC_SUBST(INTROSPECTION_GENERATE)
AC_SUBST(INTROSPECTION_GIRDIR)
AC_SUBST(INTROSPECTION_TYPELIBDIR)
AC_SUBST(INTROSPECTION_CFLAGS)
AC_SUBST(INTROSPECTION_LIBS)
AC_SUBST(INTROSPECTION_MAKEFILE)

AM_CONDITIONAL(HAVE_INTROSPECTION, test "x$found_introspection" = "xyes")
dnl we need to change the install directories for distcheck
AC_ARG_WITH([gir-dir],
	AS_HELP_STRING(
		[--with-gir-dir],
		[
		 path to gir repository
		 (automatically detected via pkg-config)
		]
	),
	[GIRDIR=$withval],
	[GIRDIR=$INTROSPECTION_GIRDIR]
)
AC_SUBST(GIRDIR)
AC_ARG_WITH([typelib-dir],
	AS_HELP_STRING(
		[--with-typelib-dir],
		[
		 path to typelibs repository
		 (automatically detected via pkg-config)
		]
	),
	[TYPELIBDIR=$withval],
	[TYPELIBDIR=$INTROSPECTION_TYPELIBDIR]
)
AC_SUBST(TYPELIBDIR)

dnl ******************
dnl * Config defaults
dnl ******************
dnl
dnl These are changed in goffice.c for WIN32 packages
AC_SUBST(goffice_datadir, '${datadir}/goffice/${VERSION}')
AC_SUBST(goffice_libdir, '${libdir}/goffice/${VERSION}')
AC_SUBST(goffice_icondir, '${datadir}/pixmaps/goffice/${VERSION}')
AC_SUBST(goffice_localedir, '${localedir}')
dnl
AC_SUBST(goffice_plugindir, '${goffice_libdir}/plugins')
AC_SUBST(goffice_externplugindir, '${libdir}/goffice/${GOFFICE_API_VER}/plugins')

dnl Export to goffice-config.h and goffice-features.h
AC_DEFINE(GOFFICE_VERSION, "goffice_full_version",
	[The version number of this release, possibly with additional suffix])
AC_DEFINE(GO_VERSION_EPOCH, goffice_version_epoch,
	[The Epoch of this release])
AC_DEFINE(GO_VERSION_MAJOR, goffice_version_major,
	[The Major version number of this release])
AC_DEFINE(GO_VERSION_MINOR, goffice_version_minor,
	[The Minor version number of this release])
AC_DEFINE(GO_VERSION_EXTRA, "goffice_version_extra",
	[Extra, possibly empty tag for this release])

dnl AX_REQUIRE_DEFINED([GTK_DOC_CHECK])
GTK_DOC_CHECK([1.12])
if test "x$with_gtk" = "xno" -a "x$enable_gtk_doc" = "xyes"; then
	AC_MSG_ERROR([Building documentation is not allowed for a without Gtk+ build])
fi

AC_CONFIG_FILES([
Makefile
libgoffice.pc
goffice/Makefile
plugins/Makefile
plugins/plot_barcol/Makefile
plugins/plot_pie/Makefile
plugins/plot_radar/Makefile
plugins/plot_xy/Makefile
plugins/plot_surface/Makefile
plugins/plot_distrib/Makefile
plugins/reg_linear/Makefile
plugins/reg_logfit/Makefile
plugins/smoothing/Makefile
plugins/lasem/Makefile
mmlitex/Makefile
po/Makefile.in
tests/Makefile
tools/Makefile
docs/Makefile
docs/reference/Makefile
docs/reference/version.xml
])
AC_OUTPUT

echo "

Configuration:

	Source code location:	${srcdir}
	Compiler:		${CC}
	Compiler flags:		${CFLAGS}
	Long double support:	${float_msg}
	Decimal support:	${decimal_msg}
	Equation support in graphs: ${found_lasem}

	UI:			${ui_msg}
	Configuration backend:	${conf_msg}
    Build gtk_mac_integration:	${os_osx}
"
