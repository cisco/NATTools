## Process this file with automake to produce Makefile.in
if LOCAL_SOCKADDR
  SOCKADDR_CPPFLAGS = -I../../sockaddrutil/include 
endif

if LOCAL_STUNLIB
  STUNLIB_CPPFLAGS =-I../../stunlib/include 
endif



AM_CPPFLAGS = -I../include $(SOCKADDR_CPPFLAGS) $(STUNLIB_CPPFLAGS) -O2 -g -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Werror -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes 

include_HEADERS = ../include/icelib.h ../include/icelibtypes.h ../include/icelib_defines.h


lib_LTLIBRARIES = libice.la
libice_la_SOURCES = icelib.c icelibtypes.c icelib_intern.h fifo.c timer.c
libice_la_LDFLAGS = -version-info 1:0:0

