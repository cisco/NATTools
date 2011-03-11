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

static const uint32_t TEST_THREAD_CTX = 1;








void (*update_turninfo)(void);



int gather(struct sockaddr *host_addr, 
           int requestedFamily, 
           char *user, 
           char *pass,
           struct turn_allocation_result *turnresult);


void gatherAll(struct turn_info *turnInfo, struct listenConfig *listenConfig, void(*update_turninfo)(void));
//void gatherAll(struct turn_info *turnInfo, struct listenConfig *listenConfig);

void *stunListen(void *ptr);

#endif
