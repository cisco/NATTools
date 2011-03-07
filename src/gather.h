#ifndef GATHER_H
#define GATHER_H

#include <stdint.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>



#include <turnclient.h>

#include "turninfo.h"

static const uint32_t TEST_THREAD_CTX = 1;






#define MAX_LISTEN_SOCKETS 10

void (*update_turninfo)(void);

struct socketConfig{
    int stunCtx;
    int sockfd;
    char* user;
    char* pass;


};


struct listenConfig{
    struct socketConfig socketConfig[MAX_LISTEN_SOCKETS];
    int numSockets;
};




int gather(struct sockaddr *host_addr, 
           int requestedFamily, 
           char *user, 
           char *pass,
           struct turn_allocation_result *turnresult);


void gatherAll(struct turn_info *turnInfo, struct listenConfig *listenConfig, void(*update_turninfo)(void));
//void gatherAll(struct turn_info *turnInfo, struct listenConfig *listenConfig);

void *stunListen(void *ptr);

#endif
