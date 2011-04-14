## Process this file with automake to produce Makefile.in
if SUBMODULE
  AM_CPPFLAGS = -I../include -I../../sockaddrutil/include -I../../icelib/include
else
  AM_CPPFLAGS = -I../include
endif
include_HEADERS = ../include/icelib.h ../include/icelibtypes.h ../include/icelib_defines.h


lib_LTLIBRARIES = libice.la
libice_la_SOURCES = icelib.c icelibtypes.c
libice_la_LDFLAGS = -version-info 1:0:0

