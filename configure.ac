
AC_INIT([gcx], [1.6])
AM_INIT_AUTOMAKE([foreign -Wall -Werror subdir-objects])

AC_CONFIG_SRCDIR([src/gcx.c])

AC_CONFIG_HEADERS([src/config.h])

AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_RANLIB

AM_PROG_AR

PKG_PROG_PKG_CONFIG()

AC_CHECK_HEADERS(libgen.h)
AC_REPLACE_FUNCS(basename)
AC_REPLACE_FUNCS(dirname)
AC_REPLACE_FUNCS(getline)


AM_PATH_GTK_3_0(3.0.0, , AC_MSG_ERROR(Cannot find GTK >= 3.0))

AC_CHECK_HEADERS([zlib.h])
AC_CHECK_LIB(z, inflate,,AC_MSG_ERROR(Cannot find zlib))

AC_CHECK_HEADERS([tiff.h])
AC_CHECK_LIB(tiff, TIFFOpen,,AC_MSG_ERROR(Cannot find libtiff))

AC_CHECK_HEADERS([jpeglib.h])
AC_CHECK_LIB(jpeg, jpeg_CreateDecompress,,AC_MSG_ERROR(Cannot find libjpeg))

AC_CHECK_LIB(m, floor,,AC_MSG_ERROR(Cannot find libm))

AC_CONFIG_FILES([
	Makefile
	docs/Makefile
	src/Makefile
	src/ccd/Makefile
	src/gsc/Makefile
])
AC_OUTPUT
