#ifndef IPADDRESS_HELPER_H
#define IPADDRESS_HELPER_H

#include <sockaddr_util.h>

#include "turninfo.h"



#define MAX_LISTEN_SOCKETS 10


typedef int (*STUN_HANDLER)(int, int);

struct socketConfig{
    TURN_INSTANCE_DATA *tInst;
    int sockfd;
    char* user;
    char* pass;
    char* realm;

};


struct listenConfig{
    struct socketConfig socketConfig[MAX_LISTEN_SOCKETS];
    int numSockets;
};


typedef enum IPv6_ADDR_TYPES {IPv6_ADDR_NONE, IPv6_ADDR_ULA, IPv6_ADDR_PRIVACY, IPv6_ADDR_NORMAL} IPv6_ADDR_TYPE; 


int getRemoteTurnServerIp(struct turn_info *turnInfo, char *fqdn);

int getLocalIPaddresses(struct turn_info *turnInfo, int type, char *iface);

bool getLocalInterFaceAddrs(struct sockaddr *addr, char *iface, int ai_family, IPv6_ADDR_TYPE ipv6_addr_type, bool force_privacy);

int createLocalTCPSocket(int ai_family,
                         struct turn_info *turnInfo,
                         uint16_t port);

int createLocalUDPSocket(int ai_family, 
                         const struct sockaddr * localIp, 
                         struct sockaddr *hostaddr, 
                         uint16_t port);

int sendMessage(struct turn_info *turnInfo, char *dstAddr, char *message);

int sendRawUDP(int sockfd,
               const void *buf,
               size_t len,
               const struct sockaddr * p,
               socklen_t t);

int SendRawStun(int sockfd,
                const uint8_t *buf,
                int len,
                const struct sockaddr *addr,
                socklen_t t,
                void *userdata);

void turnSendFunc(const uint8_t         *buffer,
                  size_t                bufLen,
                  const struct sockaddr *dstAddr,
                  void                  *userCtx);

void *stunListen(void *ptr);


int sendKeepalive(struct turn_info *turnInfo);

void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData);

#endif
