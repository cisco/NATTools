#ifndef IPADDRESS_HELPER_H
#define IPADDRESS_HELPER_H

#include <sockaddr_util.h>

#include "turninfo.h"



#define MAX_LISTEN_SOCKETS 10




struct socketConfig{
    int stunCtx;
    int sockfd;
    char* user;
    char* pass;


};


struct listenConfig{
    struct socketConfig socketConfig[MAX_LISTEN_SOCKETS];
    int numSockets;
    void (*update_inc_status)(char *message);
};





int getRemoteTurnServerIp(struct turn_info *turnInfo, char *fqdn);

int getLocalIPaddresses(struct turn_info *turnInfo, char *iface);

bool getLocalInterFaceAddrs(struct sockaddr *addr, char *iface, int ai_family);

int createLocalUDPSocket(int ai_family, 
                         const struct sockaddr * localIp, 
                         struct sockaddr *hostaddr, 
                         uint16_t port);

int sendMessage(struct turn_info *turnInfo, char *dstAddr, char *message);

int sendRawUDP(int sockfd,
               const void *buf,
               size_t len,
               struct sockaddr * p,
               socklen_t t);

int SendRawStun(int sockfd,
                uint8_t *buf,
                int len,
                struct sockaddr *addr,
                socklen_t t,
                void *userdata);

void *stunListen(void *ptr);


int sendKeepalive(struct turn_info *turnInfo);

#endif
