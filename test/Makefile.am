if CHECK


TESTS = check_icelib
#AM_CPPFLAGS = -I../include
check_PROGRAMS = check_icelib
check_icelib_SOURCES = icelib_test.c  testmain.c ../src/icelib.c ../src/icelibtypes.c
#check_icelib_CFLAGS = @CHECK_CFLAGS@  
#check_icelib_LDADD = $(top_builddir)/src/libice.la @CHECK_LIBS@


if GCOV
  AM_CPPFLAGS = -I../include -fprofile-arcs -ftest-coverage
  check_icelib_LDFLAGS = -lcheck -lgcov
else
  AM_CPPFLAGS = -I../include
  check_icelib_LDFLAGS = -lcheck
endif



clean-local:
	rm -f *.g*


endif