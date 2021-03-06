AC_INIT([MateDialog],[1.8.0],[http://www.mate-desktop.org/],[mate-dialogs])

AC_PREREQ([2.59])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src])

AM_INIT_AUTOMAKE([1.9 tar-ustar dist-xz foreign no-dist-gzip check-news])
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

MATE_COMMON_INIT

YELP_HELP_INIT

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.40.0])

AC_PROG_CC

AC_MSG_CHECKING([which gtk+ version to compile against])
AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk=2.0|3.0],[which gtk+ version to compile against (default: 2.0)])],
  [case "$with_gtk" in
     2.0|3.0) ;;
     *) AC_MSG_ERROR([invalid gtk version specified]) ;;
   esac],
  [with_gtk=2.0])
AC_MSG_RESULT([$with_gtk])

case "$with_gtk" in
  2.0) GTK_API_VERSION=2.0
       GTK_REQUIRED=2.18.0
       ;;
  3.0) GTK_API_VERSION=3.0
       GTK_REQUIRED=2.90.4
       ;;
esac

PKG_CHECK_MODULES([MATEDIALOG],[gtk+-$GTK_API_VERSION >= $GTK_REQUIRED glib-2.0])

dnl *************************
dnl Check for the GTK+ backend, if it is X11, we -lX11
dnl *************************

matedialog_save_cflags="$CFLAGS"
# pull in the CFLAGS to locate gdkconfig.h
CFLAGS=`$PKG_CONFIG --cflags gtk+-$GTK_API_VERSION`
AC_COMPILE_IFELSE([AC_LANG_SOURCE([
#include <gtk/gtk.h>
#if GTK_CHECK_VERSION (3, 0, 0)
#include <gdk/gdk.h>
#else
#include <gdkconfig.h>
#endif

int main(void) {
#ifndef GDK_WINDOWING_X11
#error GDK_WINDOWING_X11 not defined
#endif
    return 0;
}
])],
        GDK_BACKEND_X11=yes,
        GDK_BACKEND_X11=no)

CFLAGS="$matedialog_save_cflags"

if test "x$GDK_BACKEND_X11" = "xyes"; then
	PKG_CHECK_MODULES(X, x11)
	MATEDIALOG_CFLAGS="$MATEDIALOG_CFLAGS $X_CFLAGS"
	MATEDIALOG_LIBS="$MATEDIALOG_LIBS $X_LIBS"
fi

AC_SUBST([MATEDIALOG_CFLAGS])
AC_SUBST([MATEDIALOG_LIBS])

# *******************************
# X11 cflags/libs
# *******************************
PKG_CHECK_MODULES([X11], [x11])
AC_SUBST(X11_LIBS)
AC_SUBST(X11_CFLAGS)

# *******************************
# perl check
# *******************************

AC_PATH_PROG([PERL],[perl],)

# *******************************
# libnotify check
# *******************************

LIBNOTIFY_REQUIRED=0.7.0

AC_ARG_ENABLE([libnotify],
              AC_HELP_STRING([--enable-libnotify],[enable libnotify support]),,
              [enable_libnotify=yes])

if test x$enable_libnotify = xyes ; then
   PKG_CHECK_MODULES([LIBNOTIFY],[libnotify >= $LIBNOTIFY_REQUIRED],
                     [HAVE_LIBNOTIFY="yes"],[HAVE_LIBNOTIFY="no"])

   AC_SUBST([LIBNOTIFY_CFLAGS])
   AC_SUBST([LIBNOTIFY_LIBS])

   if test "x$HAVE_LIBNOTIFY" = "xyes"; then
      AC_DEFINE([HAVE_LIBNOTIFY],[1],[libnotify is available on this machine])
   fi
fi

# *******************************
# Debug
# *******************************

MATE_DEBUG_CHECK
MATE_COMPILE_WARNINGS([maximum])

# *******************************
# Internationalization
# *******************************

GETTEXT_PACKAGE=mate-dialogs
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[Gettext domain])

AM_GLIB_GNU_GETTEXT

# *******************************

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_LDFLAGS])

# *******************************

AC_CONFIG_FILES([
Makefile
po/Makefile.in
src/Makefile
data/Makefile
help/Makefile
])

AC_OUTPUT
