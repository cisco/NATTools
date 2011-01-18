TESTS = check_stunlib
AM_CPPFLAGS = -I../include
check_PROGRAMS = check_stunlib
check_stunlib_SOURCES = stunclient_test.c stunlib_test.c testmain.c
check_stunlib_CFLAGS = @CHECK_CFLAGS@  
check_stunlib_LDADD = $(top_builddir)/src/libstun.la @CHECK_LIBS@
