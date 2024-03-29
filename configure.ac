#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([rlu], [0.01], [sadjad@cs.stanford.edu])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/rlu.hh])
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_SUBST(ARFLAGS, [cr])

# Add picky CXXFLAGS
CXX17_FLAGS="-std=c++17 -pthread"
PICKY_CXXFLAGS="-pedantic -Wall -Wextra -Weffc++ -Werror"
AC_SUBST([CXX17_FLAGS])
AC_SUBST([PICKY_CXXFLAGS])

AC_ARG_ENABLE([sanitize],
  [AS_HELP_STRING([--enable-sanitize],
     [build with address and undefined-behavior santizers])],
  [EXTRA_CXXFLAGS="-fsanitize=address -fsanitize=undefined -fuse-ld=gold"],
  [sanitize=false])

# Checks for programs.
AC_PROG_CXX
AC_PROG_RANLIB

# Checks for libraries.
PKG_CHECK_MODULES([URCU], [liburcu])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT16_T

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 benchmarks/Makefile
                 tests/Makefile])
AC_OUTPUT
