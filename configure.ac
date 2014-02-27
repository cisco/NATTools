#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([NATtools], [0.2], [palmarti@cisco.com])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])
m4_pattern_allow([AM_PROG_AR])
AM_PROG_AR
LT_INIT


# Checks for programs.

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.


AM_CONDITIONAL(SUBMODULE, true)

AC_CONFIG_FILES([Makefile])

AC_CONFIG_SUBDIRS([sockaddrutil
                   stunlib
                   icelib])



#AC_CONFIG_FILES([Makefile
#                 sockaddrutil/src/Makefile
#                 sockaddrutil/test/Makefile])

AC_OUTPUT
