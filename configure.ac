dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([xfce4-launcher-plugin], [0.1.0], [your-email@example.com])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-bzip2 tar-ustar foreign])
AM_MAINTAINER_MODE()
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS([config.h])

dnl Check for basic programs
AC_PROG_CC()
AC_PROG_CC_C_O()
AC_PROG_INSTALL()

dnl Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl Check for i18n support
XDT_I18N()

dnl Check for required packages
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.22.0])
XDT_CHECK_PACKAGE([XFCE4UTIL], [libxfce4util-1.0], [4.12.0])
XDT_CHECK_PACKAGE([XFCE4PANEL], [libxfce4panel-2.0], [4.12.0])

dnl Check for debugging support
XDT_FEATURE_DEBUG()

dnl Substitute platform flags
AC_SUBST([PLATFORM_CPPFLAGS])
AC_SUBST([PLATFORM_CFLAGS])
AC_SUBST([PLATFORM_LDFLAGS])

AC_CONFIG_FILES([
  Makefile
  po/Makefile.in
])
AC_OUTPUT
