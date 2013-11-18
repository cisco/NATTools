
if CHECK
  MY_CFLAGS_1 = -I../include	
  MY_LDFLAGS_1 = -lcheck
  TESTS = check_stunlib
  check_PROGRAMS = check_stunlib
  check_stunlib_SOURCES = turnclienttest.c testvector.c  turn_message.c testmain.c testrealworldpackets.c teststunclient.c teststunserver.c ../src/stunclient.c  ../src/stunfifo.c  ../src/stunlib.c  ../src/turnclient.c

if LOCAL_SOCKADDR
  MY_LDFLAGS_2 = ../../sockaddrutil/src/libsockaddrutil.la
  MY_CFLAGS_2 = -I../include -I../../sockaddrutil/include
endif

if GCOV
  MY_CFLAGS_3 =  -fprofile-arcs -ftest-coverage
  MY_LDFLAGS_3 = -lgcov
else

endif

if USE_VALGRIND
  TESTS_ENVIRONMENT = ../../valgrind-wrapper
endif

AM_CPPFLAGS = $(MY_CFLAGS_1) $(MY_CFLAGS_2) $(MY_CFLAGS_3)
check_stunlib_LDFLAGS = $(MY_LDFLAGS_1) $(MY_LDFLAGS_2) $(MY_LDFLAGS_3)


clean-local:
	rm -f *.g*


endif
