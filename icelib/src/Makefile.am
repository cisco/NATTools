## Process this file with automake to produce Makefile.in
if SUBMODULE
  TMP_CPPFLAGS = -I../../sockaddrutil/include -I../../stunlib/include -O2 -g -Wall -Wextra -Wno-unknown-pragmas -Werror-implicit-function-declaration -Werror -Wno-unused-parameter -Wdeclaration-after-statement -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes 
endif

AM_CPPFLAGS = -I../include $(TMP_CPPFLAGS) 

include_HEADERS = ../include/icelib.h ../include/icelibtypes.h ../include/icelib_defines.h


lib_LTLIBRARIES = libice.la
libice_la_SOURCES = icelib.c icelibtypes.c icelib_intern.h
libice_la_LDFLAGS = -version-info 1:0:0

