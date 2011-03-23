
if CHECK


TESTS = check_stunlib
#AM_CPPFLAGS = -I../include
check_PROGRAMS = check_stunlib
check_stunlib_SOURCES = turnclienttest.c testvector.c  turn_message.c testmain.c ../src/stunclient.c  ../src/stunfifo.c  ../src/stunlib.c  ../src/stun_os.c  ../src/turnclient.c
check_stunlib_CFLAGS = 
#check_stunlib_LDADD = $(top_builddir)/src/libstun.la


if GCOV
  AM_CPPFLAGS = -I../include -fprofile-arcs -ftest-coverage
  check_stunlib_LDFLAGS = -lcheck -lgcov
else
  AM_CPPFLAGS = -I../include
  check_stunlib_LDFLAGS = -lcheck
endif

clean-local:
	rm -f *.g*


endif