TESTS = check_icelib
AM_CPPFLAGS = -I../include
check_PROGRAMS = check_icelib
check_icelib_SOURCES = icelib_test.c  testmain.c
check_icelib_CFLAGS = @CHECK_CFLAGS@  
check_icelib_LDADD = $(top_builddir)/src/libice.la @CHECK_LIBS@
