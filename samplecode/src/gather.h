#ifndef GATHER_H
#define GATHER_H

#include <stdint.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>



#include <turnclient.h>

#include "turninfo.h"
#include "iphelper.h"
#include "view.h"




TURN_INSTANCE_DATA *gather(TURN_INSTANCE_DATA *tInst,
           struct sockaddr *host_addr,
           int requestedFamily,
           char *user,
           char *pass,
           struct turn_allocation *turnAlloc);

void gatherAll(struct turn_info *turnInfo, 
               void(*update_turninfo)(void),
               struct listenConfig *listenConfig,
               void (*data_handler)(unsigned char *));

void releaseAll(struct turn_info *turnInfo);



#endif
