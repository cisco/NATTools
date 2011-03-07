#ifndef GATHER_H
#define GATHER_H

#include <stdint.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <curses.h>

#include <turnclient.h>

static const uint32_t TEST_THREAD_CTX = 1;



enum turn_ip_type{
    turn_44 = 1,
    turn_46 = 2,
    turn_64 = 4,
    turn_66 = 8,
};

#define FQDN_MAX_LEN 1024


struct turn_allocation_result{
    int sockfd;
    struct sockaddr_storage activeTurnServerAddr;
    struct sockaddr_storage hostAddr;
    struct sockaddr_storage rflxAddr;
    struct sockaddr_storage relAddr;
    
    void (*update_turninfo)(void);

};

struct turn_info{
    int numPending;


    //Fully Qualified Domain Name
    char fqdn[FQDN_MAX_LEN];

    char user[STUN_MSG_MAX_USERNAME_LENGTH];
    char pass[STUN_MSG_MAX_PASSWORD_LENGTH];

    //TURN server IP adresses (As resolved by the FQDN)
    struct sockaddr_storage remoteIp4;
    struct sockaddr_storage remoteIp6;


    //Local IP adresses
    struct sockaddr_storage localIp4;
    struct sockaddr_storage localIp6;

    
    struct turn_allocation_result turnAlloc_44;
    struct turn_allocation_result turnAlloc_46;
    struct turn_allocation_result turnAlloc_64;
    struct turn_allocation_result turnAlloc_66;


};


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

void printAllocationResult(struct turn_allocation_result *result);
void printTurnInfo( struct turn_info *turnInfo );

#endif
