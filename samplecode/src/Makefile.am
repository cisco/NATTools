bin_PROGRAMS = stunclient stunserver turnclient  simple_proxy_server testturn

if LOCAL_SOCKADDR
  AM_CPPFLAGS_1 = -I../../sockaddrutil/include
  LDADD_1 = ../../sockaddrutil/src/libsockaddrutil.la
endif

if LOCAL_STUNLIB
  AM_CPPFLAGS_2 = -I../../stunlib/include
  LDADD_2 = ../../stunlib/src/libstun.la
endif

if LOCAL_ICELIB
  AM_CPPFLAGS_3 = -I../../icelib/include
  LDADD_3 = ../../icelib/src/libice.la
endif

AM_CPPFLAGS = -I../include $(AM_CPPFLAGS_1) $(AM_CPPFLAGS_2) $(AM_CPPFLAGS_3)		

stunclient_LDADD =  $(LDADD_3) $(LDADD_2) $(LDADD_1)
stunserver_LDADD =  $(LDADD_3) $(LDADD_2) $(LDADD_1)
turnclient_LDADD =  $(LDADD_3) $(LDADD_2) $(LDADD_1)
#iceclient_LDADD = $(LDADD_3) $(LDADD_2) $(LDADD_1)
testturn_LDADD =  $(LDADD_3) $(LDADD_2) $(LDADD_1)


stunclient_SOURCES = stunclient.c utils.c
stunserver_SOURCES = stunserver.c utils.c
turnclient_SOURCES = turnclient.c utils.c
#iceclient_SOURCES = iceclient.c utils.c turnhelper.c gather.c
testturn_SOURCES = testturn.c gather.c turninfo.c permissions.c iphelper.c view.c gather.h iphelper.h permissions.h turninfo.h view.h

simple_proxy_server_SOURCES = simple_proxy_server.c
