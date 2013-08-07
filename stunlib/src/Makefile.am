## Process this file with automake to produce Makefile.in
if LOCAL_SOCKADDR
  MY_AM_CPPFLAGS = -I../../sockaddrutil/include 
endif

AM_CPPFLAGS = $(MY_AM_CPPFLAGS) -I../include -O2 -g -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Werror -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes






include_HEADERS = ../include/stunclient.h ../include/stun_intern.h ../include/stunlib.h ../include/turnclient.h ../include/stunfifo.h ../include/stunlibapi.h ../include/turn_intern.h





lib_LTLIBRARIES = libstun.la
libstun_la_SOURCES = stunclient.c stunfifo.c stunlib.c turnclient.c
libstun_la_LDFLAGS = -version-info 1:0:0

