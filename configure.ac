#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([testice], [0.1], [palmarti@cisco.com])
AC_CONFIG_SRCDIR([src/turn.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([ssl], [SSL_library_init], [], [AC_MSG_FAILURE([could not find ssl])])
AC_CHECK_LIB([sockaddrutil], [sockaddr_initFromString],[], [AC_MSG_FAILURE([could not find sockaddrutil])])

AC_CHECK_LIB([stun], [TurnClient_Init], [], [AC_MSG_FAILURE([could not find stunlib])])

AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_FAILURE([could not find pthread])])

# AC_CHECK_LIB([curses], [getch], [], [AC_MSG_FAILURE([could not find curses])])





# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h unistd.h timer.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memset socket])

AC_CONFIG_FILES([Makefile
                src/Makefile])
AC_OUTPUT
