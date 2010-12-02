
AC_INIT([gcx], [1.1])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([src/config.h])

AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC
AC_PROG_RANLIB


AC_CHECK_HEADERS(libgen.h)
AC_REPLACE_FUNCS(basename)
AC_REPLACE_FUNCS(dirname)
AC_REPLACE_FUNCS(getline)


AM_PATH_GTK_2_0(2.4.0, , AC_MSG_ERROR(Cannot find GTK: is pkg-config in path?))

AC_CONFIG_FILES([
	makedeb
	dscupload
	Makefile
	src/Makefile
	src/ccd/Makefile
	src/gsc/Makefile
	docs/Makefile
])
AC_OUTPUT

