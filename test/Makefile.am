if CHECK


TESTS = check_icelib
check_PROGRAMS = check_icelib
check_icelib_SOURCES = icelib_test.c icelibtypes_test.c testmain.c ../src/icelib.c ../src/icelibtypes.c


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