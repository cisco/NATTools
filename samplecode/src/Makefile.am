bin_PROGRAMS = stunclient stunserver turnclient malicenode

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
endif

AM_CPPFLAGS = -I../include $(AM_CPPFLAGS_1) $(AM_CPPFLAGS_2) $(AM_CPPFLAGS_3)		

stunclient_LDADD =  $(LDADD_3) $(LDADD_2) $(LDADD_1)
stunserver_LDADD =  $(LDADD_3) $(LDADD_2) $(LDADD_1)
turnclient_LDADD =  $(LDADD_3) $(LDADD_2) $(LDADD_1)

stunclient_SOURCES = stunclient.c utils.c
stunserver_SOURCES = stunserver.c utils.c
turnclient_SOURCES = turnclient.c utils.c
malicenode_SOURCES = malicenode.cc
