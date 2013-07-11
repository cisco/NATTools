#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([samplecode], [0.1], [palmarti@cisco.com])
AC_CONFIG_SRCDIR([src/stunclient.c])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX



# Checks for libraries.
AC_CHECK_LIB([ssl], [SSL_library_init], [], [AC_MSG_FAILURE([could not find ssl])])




AC_CHECK_FILE(../sockaddrutil/include/sockaddr_util.h, local_sockaddr=true, local_sockaddr=false )
AM_CONDITIONAL(LOCAL_SOCKADDR, test x"$local_sockaddr" = "xtrue")
AM_COND_IF(LOCAL_SOCKADDR,
           [],
           [AC_CHECK_LIB([sockaddrutil], [sockaddr_initFromString],[], [AC_MSG_FAILURE([could not find sockaddrutil])])])


AC_CHECK_FILE(../stunlib/include/stunlib.h, local_stunlib=true, local_stunlib=false )
AM_CONDITIONAL(LOCAL_STUNLIB, test x"$local_stunlib" = "xtrue")
AM_COND_IF(LOCAL_STUNLIB,
           [],
           [AC_CHECK_LIB([stun], [TurnClient_Init], [], [AC_MSG_FAILURE([could not find stunlib])])])

AC_CHECK_FILE(../icelib/include/icelib.h, local_icelib=true, local_icelib=false )
AM_CONDITIONAL(LOCAL_ICELIB, test x"$local_icelib" = "xtrue")
AM_COND_IF(LOCAL_ICELIB,
           [],
           [AC_CHECK_LIB([ice], [ICELIB_Constructor], [], [AC_MSG_FAILURE([could not find icelib])])])
           




#AC_CHECK_LIB([stun], [TurnClient_Init], [], [AC_MSG_FAILURE([could not find stunlib])])
#AC_CHECK_LIB([ice], [ICELIB_Constructor], [], [AC_MSG_FAILURE([could not find icelib])])


AC_CHECK_LIB([pthread], [pthread_create], [], [AC_MSG_FAILURE([could not find pthread])])
AC_CHECK_LIB([netfilter_queue], [nfq_open], [], [AC_MSG_FAILURE([could not find netfilter_queue])])
AC_CHECK_LIB([nfnetlink], [nfnl_fd], [], [AC_MSG_FAILURE([could not find nfnetlink])])



#AC_CHECK_LIB([curses], [getch], [], [AC_MSG_FAILURE([could not find curses ( sudo apt-get install libncurses5-dev?)])])
#AC_CHECK_LIB([form], [new_form], [], [AC_MSG_FAILURE([could not find form])])
#AC_CHECK_LIB([panel], [new_panel], [], [AC_MSG_FAILURE([could not find panel])])
#AC_CHECK_LIB([menu], [new_menu], [], [AC_MSG_FAILURE([could not find menu])])





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
