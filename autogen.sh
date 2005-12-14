#!/bin/sh
# Run this to generate all the initial makefiles, etc.

PKG_NAME="goffice"

REQUIRED_AUTOMAKE_VERSION=1.7.2
REQUIRED_LIBTOOL_VERSION=1.4.3
#REQUIRED_PKG_CONFIG_VERSION=0.18.0

# We need intltool >= 0.27.2 to extract the UTF-8 chars from source code:
REQUIRED_INTLTOOL_VERSION=0.27.2

# We require Automake 1.7.2, which requires Autoconf 2.54.
# (It needs _AC_AM_CONFIG_HEADER_HOOK, for example.)
REQUIRED_AUTOCONF_VERSION=2.54

USE_GNOME2_MACROS=1
USE_COMMON_DOC_BUILD=yes

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(test -f $srcdir/configure.in	\
  && test -d $srcdir/goffice	\
  && test -f $srcdir/goffice/goffice.h) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level goffice directory"
    exit 1
}

ifs_save="$IFS"; IFS=":"
for dir in $PATH ; do
  IFS="$ifs_save"
  test -z "$dir" && dir=.
  if test -f $dir/gnome-autogen.sh ; then
    gnome_autogen="$dir/gnome-autogen.sh"
    gnome_datadir=`echo $dir | sed -e 's,/bin$,/share,'`
    break
  fi
done

if test -z "$gnome_autogen" ; then
  echo "You need to install the gnome-common module and make"
  echo "sure the gnome-autogen.sh script is in your \$PATH."
  exit 1
fi

GNOME_DATADIR="$gnome_datadir"

. $gnome_autogen
