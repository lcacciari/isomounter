#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([isomounter], [0.1.0], [leo.cacciari@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror -Woverride silent-rules foreign])
AC_CONFIG_SRCDIR([src/isomounter.c])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_LIBOBJ_DIR([lib])

# Checks for programs.
AC_PROG_CC
AC_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG

# Checks for libraries.
PKG_CHECK_MODULES([FUSE], [fuse >= 2.8])
PKG_CHECK_MODULES([ISO9660], [libiso9660 >= 0.83])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.0.0])

AC_SUBST([FUSE_LIBS])
AC_SUBST([FUSE_CFLAGS])
AC_SUBST([ISO9660_LIBS])
AC_SUBST([ISO9660_CFLAGS])
AC_SUBST([GLIB_LIBS])
AC_SUBST([GLIB_CFLAGS])


# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_OFF_T

# Checks for library functions.

# Debug support
AC_MSG_CHECKING(if you want debug support)
debug_on=no
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],[enable debug support (def=no)]),
        debug_on="$enableval")
AC_MSG_RESULT($debug_on)

if test "$debug_on" = "no"; then
   AC_DEFINE([NDEBUG],, [Define to drop debug support])
fi

AM_CONDITIONAL(DEBUG, test "$debug_on" = yes)

AC_CONFIG_FILES([
                Makefile
                src/Makefile
	        ])


AC_OUTPUT
