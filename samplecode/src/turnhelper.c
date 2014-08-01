#include <pthread.h>
#include <sockaddr_util.h>
#include <ifaddrs.h>
#include <sys/poll.h>

#include "turnhelper.h"
#include "utils.h"



#define MAX_LISTEN_SOCKETS 10
#define MAXBUFLEN 1024



struct socketConfig{
    int sockfd;
    char* user;
    char* pass;
    
    
};


struct listenConfig{
    struct socketConfig socketConfig[MAX_LISTEN_SOCKETS];
    int numSockets;
    struct turnData *tData;
    void (*update_inc_status)(unsigned char *message);
};




bool getLocalInterFaceAddrs(struct sockaddr *addr, char *iface, int ai_family){
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    
    /* Walk through linked list, maintaining head pointer so we
       can free list later */
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;
        
        if (sockaddr_isAddrLoopBack(ifa->ifa_addr))
            continue;
        
        if( strcmp(iface, ifa->ifa_name)!=0 )
            continue;
        
        if (ai_family == AF_INET6 ){
            if (sockaddr_isAddrLinkLocal(ifa->ifa_addr))
                continue;
        }
        
        
        family = ifa->ifa_addr->sa_family;
        
        if (family == ai_family) {
            
            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
        }
    }
    freeifaddrs(ifaddr);
    
    if (sockaddr_initFromString(addr, host))
        return true;
    
    return false;
}


int createLocalUDPSocket(int ai_family,
                         const struct sockaddr *localIp,
                         struct sockaddr *hostaddr,
                         uint16_t port)
{
    int sockfd;
    
    int rv;
    int yes = 1;
    struct addrinfo hints, *ai, *p;
    char addr[SOCKADDR_MAX_STRLEN];
    char service[8];

    sockaddr_toString(localIp, addr, sizeof addr, false);

    //itoa(port, service, 10);

    //snprintf(service, 8, "%d", port);
    snprintf(service, 8, "%d", 3478);

        
    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST | AI_ADDRCONFIG;

    
    if ((rv = getaddrinfo(addr, service, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s ('%s')\n", gai_strerror(rv), addr);
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {

        if (sockaddr_isAddrAny(p->ai_addr) ){
            //printf("Ignoring any\n");
            continue;

        }
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

                
                    
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            printf("Bind failed\n");
            close(sockfd);
            continue;
        }
    
        if (localIp != NULL){
            struct sockaddr_storage ss;
            socklen_t len = sizeof(ss);

            if (getsockname(sockfd, (struct sockaddr *)&ss, &len) == -1){
                perror("getsockname");
            }
            else{
                if (ss.ss_family == AF_INET) {
                    ((struct sockaddr_in *)p->ai_addr)->sin_port = ((struct sockaddr_in *)&ss)->sin_port;
                }else{
                    ((struct sockaddr_in6 *)p->ai_addr)->sin6_port = ((struct sockaddr_in6 *)&ss)->sin6_port;
                }
            }
            
            if(hostaddr !=NULL){
                sockaddr_copy(hostaddr, p->ai_addr);
            }
            
            //printf("Bound to: '%s'\n",
            //       sockaddr_toString(localIp, addr, sizeof(addr), true));
            
        }
        
        break;
    }
    
    return sockfd;
}

int createLocalSockets(struct mediaStream *mStream, int type, char *iface)
{
    
    if (!getLocalInterFaceAddrs( (struct sockaddr *)&mStream->localIPv4, iface, AF_INET) ){
        //fprintf(stderr,"Unable to find local IPv4 interface addresses\n");
    }else{
        mStream->tData_44->sockfd = createLocalUDPSocket(AF_INET,
                                                         (struct sockaddr *)&mStream->localIPv4,
                                                         NULL, //(struct sockaddr *)&turnInfo->turnAlloc_44.hostAddr,
                                                         0);
        
        mStream->tData_46->sockfd = createLocalUDPSocket(AF_INET,
                                                         (struct sockaddr *)&mStream->localIPv4,
                                                         NULL, //(struct sockaddr *)&turnInfo->turnAlloc_46.hostAddr,
                                                         0);
        
    }

    if (!getLocalInterFaceAddrs((struct sockaddr *)&mStream->localIPv6, iface, AF_INET6) ){
        //fprintf(stderr,"Unable to find local IPv6 interface addresses\n");
    }else{
        mStream->tData_64->sockfd= createLocalUDPSocket(AF_INET6,
                                                        (struct sockaddr *)&mStream->localIPv6,
                                                        NULL, //(struct sockaddr *)&turnInfo->turnAlloc_64.hostAddr,
                                                        0);
        
        mStream->tData_66->sockfd = createLocalUDPSocket(AF_INET6,
                                                         (struct sockaddr *)&mStream->localIPv6,
                                                         NULL, //(struct sockaddr *)&turnInfo->turnAlloc_66.hostAddr,
                                                         0);
    }
    
    return 0;
}


int gatherCandidates(struct turnData *tData)
{
    struct addrinfo *servinfo, *p;
    
    
    pthread_t turnTickThread, turnListenThread;
    TurnCallBackData_T TurnCbData;

    //tData->sockfd = createSocket(tData->turn_uri, tData->turn_port, 0, servinfo, &p);
    
    TurnClient_StartAllocateTransaction(&tData->instData,
                                        50,
                                        turnInfoFunc,
                                        "IceClient",
                                        tData, // *userCtx
                                        p->ai_addr,
                                        tData->user,
                                        tData->password,
                                        AF_INET,
                                        turnSendFunc,
                                        turnCbFunc,
                                        false,
                                        0);
    
    pthread_create(&turnTickThread, NULL, &tickTurn, tData->instData);
    pthread_create(&turnListenThread, NULL, &handleTurnResponse, tData);
    
    //freeaddrinfo(servinfo);
    return 0;
}






void *turnListen(void *ptr)
{
    struct pollfd ufds[10];
    struct listenConfig *config = (struct listenConfig *)ptr;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    socklen_t addr_len;
    int rv;
    int numbytes;
    bool isMsSTUN;
    int i;
    
    
    for (i=0;i<config->numSockets;i++){
        ufds[i].fd = config->socketConfig[i].sockfd;
        ufds[i].events = POLLIN;
    }
    
    addr_len = sizeof their_addr;
    
    while(1){
        
        rv = poll(ufds, config->numSockets, -1);
        

        if (rv == -1) {
            perror("poll"); // error occurred in poll()
        } else if (rv == 0) {
            printf("Timeout occurred! (Should not happen)\n");
        } else {
            // check for events on s1:

            for(i=0;i<config->numSockets;i++){
                if (ufds[i].revents & POLLIN) {

                    if ((numbytes = recvfrom(config->socketConfig[i].sockfd, buf, MAXBUFLEN-1 , 0,
                                             (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                        perror("recvfrom");
                        //exit(1);
                    return 0;
                    }
                    
                    
                    if ( stunlib_isStunMsg(buf, numbytes ) ){
                        StunMessage msg;
                        
                        stunlib_DecodeMessage(buf,
                                              numbytes,
                                              &msg,
                                              NULL,
                                              NULL);
                        
                        
                        
                        if (msg.msgHdr.msgType == STUN_MSG_DataIndicationMsg){
                            
                            if (msg.hasData){
                                buf[numbytes] = '\0';
                                //config->update_inc_status(msg.data.pData);

                                
                                config->update_inc_status(&buf[msg.data.offset]);

                            }
                        }

                        TurnClient_HandleIncResp(config->tData->instData,
                                                 &msg,
                                                 buf);
                    }
                    else{
                        buf[numbytes] = '\0';
                        config->update_inc_status(buf);

                    }
                }
            }

        }

    }
return 0;
}

/*
static void *handleTurnResponse(void *ptr)
{
    int numbytes;
    struct sockaddr_storage their_addr;
    unsigned char buf[1500];
    StunMessage stunResponse;
    int keyLen = 16;
    char md5[keyLen];
    char realm[STUN_MAX_STRING];
    struct turnData *tData = (struct turnData*)ptr;

    for(;;){
        //printf("Listening....\n");
        if((numbytes = recvStunMsg(tData->sockfd, &their_addr, &stunResponse, buf, 1499)) != -1) {
            if (stunResponse.msgHdr.msgType == STUN_MSG_DataIndicationMsg) {
                if (stunResponse.hasData) {
                    //Decode and do something with the data?
                }
            }
            
            if (stunResponse.hasRealm) {
                memcpy(realm, stunResponse.realm.value, STUN_MAX_STRING);
            }
            
            if (stunResponse.hasMessageIntegrity) {
                //printf("   Integrity attribute present.\n");
                stunlib_createMD5Key((unsigned char *)md5, tData->user, realm, tData->password);
                
                if (stunlib_checkIntegrity(buf, numbytes, &stunResponse,(unsigned char *)md5, keyLen)) {
                    //printf("     - Integrity check OK\n");
                } else {
                    //printf("     - Integrity check NOT OK\n");
                }
            }
            //printf("   Sending Response to TURN library\n");
            TurnClient_HandleIncResp(tData->instData,
                                     &stunResponse,
                                     buf);
        }
    }
}
*/


void turnCbFunc(void *userCtx, TurnCallBackData_T *turnCbData)
{
    
    struct turnData *tData = (struct turnData*)userCtx;

    if ( turnCbData->turnResult == TurnResult_AllocOk ){
        tData->turn_state = SUCCESS;
        sockaddr_copy(&tData->active_turn_server, (struct sockaddr *)&turnCbData->TurnResultData.AllocResp.activeTurnServerAddr);
        sockaddr_copy(&tData->rflx, (struct sockaddr *)&turnCbData->TurnResultData.AllocResp.srflxAddr);
        sockaddr_copy(&tData->relay, (struct sockaddr *)&turnCbData->TurnResultData.AllocResp.relAddr);
        /*
        
        */
    } else if (turnCbData->turnResult == TurnResult_AllocUnauthorised) {
        tData->turn_state = ERROR;
        printf("Unable to authorize. Wrong user/pass?\n");
    }
}


static void turnInfoFunc(void *userCtx, TurnInfoCategory_T category, char *ErrStr) {

}

static void *tickTurn( void *ptr)
{
    struct timespec timer;
    struct timespec remaining;
    TURN_INSTANCE_DATA *instData = (TURN_INSTANCE_DATA *)ptr;
    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    for(;;) {
        nanosleep(&timer, &remaining);
        TurnClient_HandleTick(instData);
    }

}


static void turnSendFunc(const uint8_t         *buffer,
                         size_t                bufLen,
                         const struct sockaddr *dstAddr,
                         void                  *userCtx) 
{
    struct turnData *tData = (struct turnData*)userCtx;

    sendRawStun(tData->sockfd, buffer, bufLen, dstAddr, false);
}


