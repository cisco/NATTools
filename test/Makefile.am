TESTS = check_sockaddrutil
AM_CPPFLAGS = -I../include
check_PROGRAMS = check_sockaddrutil
check_sockaddrutil_SOURCES = sockaddr_test.c testmain.c
check_sockaddrutil_CFLAGS = @CHECK_CFLAGS@
check_sockaddrutil_LDADD = $(top_builddir)/src/libsockaddrutil.la @CHECK_LIBS@

