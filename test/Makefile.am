if CHECK


TESTS = check_icelib
check_PROGRAMS = check_icelib
check_icelib_SOURCES = icelib_test.c icelibtypes_test.c testmain.c ../src/icelib.c ../src/icelibtypes.c
MY_LDFLAGS_1 = -lcheck
MY_CFLAGS_1 = -I../include


if SUBMODULE
  MY_LDFLAGS_2 = ../../sockaddrutil/src/libsockaddrutil.la ../../stunlib/src/libstun.la
  MY_CFLAGS_2 = -I../../sockaddrutil/include -I../../icelib/include
endif

if GCOV
  MY_CFLAGS_3 = -I../include -fprofile-arcs -ftest-coverage
  MY_LDFLAGS_3 = -lcheck -lgcov
endif

AM_CPPFLAGS = $(MY_CFLAGS_1) $(MY_CFLAGS_2) $(MY_CFLAGS_3)
check_icelib_LDFLAGS = $(MY_LDFLAGS_1) $(MY_LDFLAGS_2) $(MY_LDFLAGS_3)

clean-local:
	rm -f *.g*


endif