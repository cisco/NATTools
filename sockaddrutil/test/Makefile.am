if CHECK



TESTS = check_sockaddrutil

check_PROGRAMS = check_sockaddrutil
check_sockaddrutil_SOURCES = sockaddr_test.c testmain.c ../src/sockaddr_util.c

if GCOV
  AM_CPPFLAGS = -I../include -fprofile-arcs -ftest-coverage
  check_sockaddrutil_LDFLAGS = -lcheck -lgcov
else
  AM_CPPFLAGS = -I../include
  check_sockaddrutil_LDFLAGS = -lcheck
endif

if USE_VALGRIND
  TESTS_ENVIRONMENT = ../../valgrind-wrapper
endif

include_HEADERS = ../include/sockaddr_util.h


endif
clean-local:
	rm -f *.g*

