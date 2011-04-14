bin_PROGRAMS = testturn testice

if SUBMODULE
  AM_CPPFLAGS = -I../include -I../../sockaddrutil/include -I../../icelib/include
  testturn_LDADD = ../../stunlib/src/libstun.la ../../sockaddrutil/src/libsockaddrutil.la
  testice_LDADD = ../../stunlib/src/libstun.la ../../sockaddrutil/src/libsockaddrutil.la ../../icelib/src/libice.la
else
  AM_CPPFLAGS = -I../include	
endif

testturn_SOURCES = testturn.c gather.c turninfo.c permissions.c iphelper.c view.c gather.h iphelper.h permissions.h turninfo.h view.h

testice_SOURCES = ice.c gather.c turninfo.c permissions.c iphelper.c gather.h iphelper.h permissions.h turninfo.h view.h