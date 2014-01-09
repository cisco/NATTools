AUTOMAKE_OPTIONS = subdir-objects
if CHECK

TESTS = check_icelib
check_PROGRAMS = check_icelib

check_icelib_SOURCES = icelib_test.c icelib_running_test.c icelibtypes_test.c icelib_trickle_test.c testmain.c ../src/icelib.c ../src/fifo.c ../src/timer.c ../src/icelibtypes.c


MY_LDFLAGS_1 = -lcheck
MY_CFLAGS_1 = -I../include


if LOCAL_SOCKADDR
  MY_LDFLAGS_2 = ../../sockaddrutil/src/libsockaddrutil.la 
  MY_CFLAGS_2 = -I../../sockaddrutil/include -I../../icelib/include
endif

if LOCAL_STUNLIB
  MY_LDFLAGS_3 = ../../stunlib/src/libstun.la
  MY_CFLAGS_3 = -I../../stunlib/include -I../../icelib/include
endif

if GCOV
  MY_CFLAGS_4 = -I../include -fprofile-arcs -ftest-coverage
  MY_LDFLAGS_4 = -lgcov
endif

if USE_VALGRIND
  TESTS_ENVIRONMENT = ../../valgrind-wrapper
endif

AM_CPPFLAGS = $(MY_CFLAGS_1) $(MY_CFLAGS_2) $(MY_CFLAGS_3) $(MY_CFLAGS_4)
check_icelib_LDFLAGS = $(MY_LDFLAGS_1) $(MY_LDFLAGS_2) $(MY_LDFLAGS_3) $(MY_LDFLAGS_4)

clean-local:
	rm -f *.g*


endif
