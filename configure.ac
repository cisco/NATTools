#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([icelib], [0.1], [palmarti@cisco.com])
AC_CONFIG_SRCDIR([src/icelib.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
LT_INIT


# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
AC_CHECK_LIB([check], [srunner_create])
AM_PATH_CHECK()

AC_CHECK_LIB([sockaddrutil], [sockaddr_initFromString],[], [AC_MSG_FAILURE([could not find sockaddrutil])])

AC_CHECK_LIB([stun], [TurnClient_Init], [], [AC_MSG_FAILURE([could not find stunlib])])



# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strchr])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile])


AC_OUTPUT
