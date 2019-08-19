AC_PREREQ([2.59])
AC_INIT([simple-vnc-server], [0.1.0])
AC_CONFIG_MACRO_DIR([m4])

LT_PREREQ([2.4.0])
LT_INIT
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/main.cxx])


# Checks for programs.
AC_PROG_CXX
AC_PROG_LIBTOOL

# check for c++11
AX_CXX_COMPILE_STDCXX_11(noext, mandatory)

# Checks for libraries.
PKG_CHECK_MODULES(LDEPS, [
	libvncserver
	pangocairo
])

AC_SUBST(LDEPS_CFLAGS)
AC_SUBST(LDEPS_LIBS)

AC_CONFIG_FILES([
  Makefile
  src/Makefile
])
AC_OUTPUT
