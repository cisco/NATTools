
bin_PROGRAMS = testturn pumpturn testice

if LOCAL_SOCKADDR
  AM_CPPFLAGS_1 = -I../../sockaddrutil/include
  testturn_LDADD_1 = ../../sockaddrutil/src/libsockaddrutil.la
  pumpturn_LDADD_1 = ../../sockaddrutil/src/libsockaddrutil.la
  testice_LDADD_1 = ../../sockaddrutil/src/libsockaddrutil.la
endif

if LOCAL_STUNLIB
  AM_CPPFLAGS_2 = -I../../stunlib/include
  testturn_LDADD_2 = ../../stunlib/src/libstun.la
  pumpturn_LDADD_2 = ../../stunlib/src/libstun.la
  testice_LDADD_2 = ../../stunlib/src/libstun.la
endif

if LOCAL_ICELIB
  AM_CPPFLAGS_3 = -I../../icelib/include
  testturn_LDADD_3 = 
  pumpturn_LDADD_3 = 
  testice_LDADD_3 = ../../icelib/src/libice.la
endif

AM_CPPFLAGS = -I../include $(AM_CPPFLAGS_1) $(AM_CPPFLAGS_2) $(AM_CPPFLAGS_3)		

testturn_LDADD = $(testturn_LDADD_1) $(testturn_LDADD_2) $(testturn_LDADD_3) 
pumpturn_LDADD = $(pumpturn_LDADD_1) $(pumpturn_LDADD_2) $(pumpturn_LDADD_3) 
testice_LDADD =  $(testice_LDADD_1) $(testice_LDADD_2) $(testice_LDADD_3)

testturn_SOURCES = testturn.c gather.c turninfo.c permissions.c iphelper.c view.c gather.h iphelper.h permissions.h turninfo.h view.h

pumpturn_SOURCES = pumpturn.c gather.c turninfo.c permissions.c iphelper.c gather.h iphelper.h permissions.h turninfo.h


testice_SOURCES = ice.c gather.c turninfo.c permissions.c iphelper.c gather.h iphelper.h permissions.h turninfo.h view.h