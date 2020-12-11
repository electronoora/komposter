AC_INIT([komposter], [2.0.0], [email], [komposter])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign])

AC_CHECK_C99
#AC_CHECK_CXX

AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_C_BIGENDIAN

dnl AC_PREFIX_DEFAULT([/usr/local])

AC_COMPILE_MODE([ENABLE_DEBUG])

AC_INCORPORATE_FREETYPE2([9.16.0], [ENABLE_FREETYPE], [yes])
AC_INCORPORATE_GLUT([ENABLE_GLUT], [yes])
AC_INCORPORATE_OPENAL([0.0.8], [ENABLE_OPENAL], [yes])
AC_INCORPORATE_OPENGL([ENABLE_OPENGL], [yes])

AC_CHECK_LIB([m],[log])

AC_CHECK_LIB([pthread], [pthread_create])

dnl resourcepath="$prefix/share/$PACKAGE_TARNAME"
dnl AC_DEFINE_UNQUOTED([RESOURCEPATH], "$resourcepath")

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
Configuration for $PACKAGE_TARNAME $PACKAGE_VERSION:
	Enable debug:             $ENABLE_DEBUG
	Enable optimizations:     $optimizations
	Compiler:                 $CC
	Compiler flags:           $CFLAGS
	Linker flags:             $LDFLAGS
	Libs:                     $LIBS
	Install prefix:           $prefix
	Install resources to:     $prefix
Type 'make' to build and 'make install' to install $PACKAGE."
