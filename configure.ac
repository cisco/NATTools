#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([stunlib], [0.1], [palmarti@cisco.com])
AC_CONFIG_SRCDIR([include/stunlib.h])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
LT_INIT

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O


AC_ARG_ENABLE(gcov,
AS_HELP_STRING([--enable-gcov],
               [enable test coverage with gcov @<:@default=no@:>@]),
              [case "${enableval}" in
                 yes) gcov=true ;;
                 no)  gcov=false ;;
                 *)   AC_MSG_ERROR([bad value ${enableval} for --enable-gcov]) ;;
               esac],
              [gcov=false])

AM_CONDITIONAL(GCOV, test x"$gcov" = "xtrue")




AM_CONDITIONAL(CHECK, "false")
AC_ARG_WITH([check],
  [AS_HELP_STRING([--with-check],
  [use Check for unit testing @<:@default=no@:>@])],
  [],
  [with_check=no])
          
  LIBCHECK=
  AS_IF([test "x$with_check" != xno],
    [AC_CHECK_LIB([check], [srunner_create],
    [AC_SUBST([LIBCHECK], ["-lcheck"])
     AM_CONDITIONAL(CHECK, "true")
     AC_DEFINE([HAVE_CHECK], [1],
               [Define if you have check])],
    [AC_MSG_FAILURE(
      [--with-check was given, but test for check failed])],
      [])])

# Checks for libraries.
#AC_CHECK_LIB([check], [srunner_create])

AC_ARG_ENABLE(submodule,
AS_HELP_STRING([--enable-submodule],
               [enable submodule build @<:@default=no@:>@]),
              [case "${enableval}" in
                 yes) submodule=true ;;
                 no)  submodule=false ;;
                 *)   AC_MSG_ERROR([bad value ${enableval} for --enable-submodule]) ;;
               esac],
              [submodule=false])


AM_CONDITIONAL(SUBMODULE, test x"$submodule" = "xtrue")

AM_COND_IF(SUBMODULE,
           [],
[AC_CHECK_LIB([sockaddrutil], [sockaddr_initFromString],[], [AC_MSG_FAILURE([could not find sockaddrutil])])])

dnl AC_CHECK_LIB([ssl], [SSL_library_init], [], [AC_MSG_FAILURE([could not find ssl])])

AC_CHECK_LIB([crypto], [MD5], [], [AC_MSG_FAILURE([could not find crypto])])

dnl AC_CHECK_LIB([gsasl], [gsasl_check_version], [], [AC_MSG_FAILURE([could not find gsasl])])



# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h stddef.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 test/Makefile])

AC_OUTPUT
