#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>
#define _OPEN_SYS_SOCK_IPV6
#include <netinet/in.h>

#include <sockaddr_util.h>

#include "gather.h"

#include "iphelper.h"

int getRemoteTurnServerIp(struct turn_info *turnInfo, char *fqdn)
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];




    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(fqdn, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    for(p = res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            if ( ipv4->sin_port == 0 ) {
                sockaddr_initFromIPv4Int((struct sockaddr_in *)&turnInfo->remoteIp4,
                                         ipv4->sin_addr.s_addr,
                                         htons(3478));
            }

            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            if ( ipv6->sin6_port == 0 ) {
                sockaddr_initFromIPv6Int((struct sockaddr_in6 *)&turnInfo->remoteIp6,
                                         ipv6->sin6_addr.s6_addr,
                                         htons(3478));
            }


            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        //inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        //printf("  %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res); // free the linked list
    return 0;
}

int getLocalIPaddresses(struct turn_info *turnInfo, int type, char *iface)
{

    if (!getLocalInterFaceAddrs( (struct sockaddr *)&turnInfo->localIp4, iface, AF_INET) ){
        //fprintf(stderr,"Unable to find local IPv4 interface addresses\n");
    }else{
        turnInfo->turnAlloc_44.sockfd = createLocalUDPSocket(AF_INET,
                                                             (struct sockaddr *)&turnInfo->localIp4,
                                                             (struct sockaddr *)&turnInfo->turnAlloc_44.hostAddr,
                                                             0);
        if(type == SOCK_STREAM){

            turnInfo->turnAlloc_44.sockfd = createLocalTCPSocket(AF_INET,
                                                                 turnInfo,
                                                                 3478);

        }

        turnInfo->turnAlloc_46.sockfd = createLocalUDPSocket(AF_INET,
                                                             (struct sockaddr *)&turnInfo->localIp4,
                                                             (struct sockaddr *)&turnInfo->turnAlloc_46.hostAddr,
                                                             0);
        
    }

    if (!getLocalInterFaceAddrs((struct sockaddr *)&turnInfo->localIp6, iface, AF_INET6) ){
        //fprintf(stderr,"Unable to find local IPv6 interface addresses\n");
    }else{
        turnInfo->turnAlloc_64.sockfd = createLocalUDPSocket(AF_INET6,
                                                             (struct sockaddr *)&turnInfo->localIp6,
                                                             (struct sockaddr *)&turnInfo->turnAlloc_64.hostAddr,
                                                             0);
        turnInfo->turnAlloc_66.sockfd = createLocalUDPSocket(AF_INET6,
                                                             (struct sockaddr *)&turnInfo->localIp6,
                                                             (struct sockaddr *)&turnInfo->turnAlloc_66.hostAddr,
                                                             0);
    }
    return 0;
}


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
            if (sockaddr_isAddrLinkLocal(ifa->ifa_addr)){
                continue;
            }
            if (sockaddr_isAddrSiteLocal(ifa->ifa_addr)){
                continue;
            }
            if (sockaddr_isAddrULA(ifa->ifa_addr)){
                continue;
            }
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


int createLocalTCPSocket(int ai_family,
                         struct turn_info *turnInfo,
                         uint16_t port)
{
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char service[8];
    int sockfd;
    
    snprintf(service, 8, "%d", port);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_STREAM;
    


    if ((rv = getaddrinfo(turnInfo->fqdn, service, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }
    
    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    //inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
    //          s, sizeof s);
    //printf("client: connecting to %s\n", s);
    
    freeaddrinfo(servinfo); // all done with this structure
    
    return sockfd;
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

    snprintf(service, 8, "%d", port);
    //snprintf(service, 8, "%d", 3478);

        
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
        if(p->ai_family==AF_INET6){
            //if(!inet6_is_srcaddr((struct sockaddr_in6 *)p, IPV6_PREFER_SRC_TMP)){
            //    continue;
            //}
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
            
            
            sockaddr_copy(hostaddr, p->ai_addr);
            
            
            //printf("Bound to: '%s'\n",
            //       sockaddr_toString(localIp, addr, sizeof(addr), true));
            
        }
        
        break;
    }
    
    return sockfd;
}


int sendRawUDP(int sockfd,
               const void *buf,
               size_t len,
               const struct sockaddr * p,
               socklen_t t){

    int numbytes;
    char addr[256];
    int rv;

    struct pollfd ufds[1];


    ufds[0].fd = sockfd;
    ufds[0].events = POLLOUT;

    rv = poll(ufds, 1, 3500);

    if (rv == -1) {
        perror("poll"); // error occurred in poll()
    } else if (rv == 0) {
        printf("Timeout occurred!  Not possible to send for 3.5 seconds.\n");
    } else {
        // check for events on s1:
        if (ufds[0].revents & POLLOUT) {
            numbytes = sendto(sockfd, buf, len, 0,
                              p , t);

            sockaddr_toString(p, addr, 256, true);

            if(numbytes == -1){
                int errsv = errno;
                printf("Error: %s \n", strerror(errsv));
                printf("sento( %i, buf, %zu, 0, %s, %i)\n", sockfd, len, addr, t);
            }
            //else {
            //    printf("Sending Raw (To: '%s'(%i), Bytes:%i/%i  )\n", addr, sockfd, numbytes, (int)len);
            //}
            return numbytes;
        }
    }

    return -1;
}


int SendRawStun(int sockfd,
                const uint8_t *buf,
                int len,
                const struct sockaddr *addr,
                socklen_t t,
                void *userdata){
    return  sendRawUDP(sockfd, buf, len, addr, t);

}

void turnSendFunc(const uint8_t         *buffer,
                  size_t                bufLen,
                  const struct sockaddr *dstAddr,
                  void                  *userCtx) 
{
    struct turn_allocation *alloc = (struct turn_allocation*)userCtx;
    int socklen;
    if(dstAddr->sa_family == AF_INET6) socklen = sizeof(struct sockaddr_in6);
    if(dstAddr->sa_family == AF_INET) socklen = sizeof(struct sockaddr_in);


    SendRawStun(alloc->sockfd, buffer, bufLen, dstAddr, socklen, NULL);
}

#define MAXBUFLEN 1024


int recvStunMsg(int sockfd, struct sockaddr_storage *their_addr, StunMessage *stunResponse, unsigned char *buf, int buflen)
{
    socklen_t addr_len = sizeof their_addr;
    int numbytes;

    if ((numbytes = recvfrom(sockfd, buf, buflen , 0, (struct sockaddr *)their_addr, &addr_len)) == -1) {
          perror("recvfrom");
          exit(1);
    }

    //printf("Got a packet that is %d bytes long\n", numbytes);
    if (stunlib_isStunMsg(buf, numbytes)) {
        //printf("   Packet is STUN\n");

        stunlib_DecodeMessage(buf, numbytes, stunResponse, NULL, NULL);

        return numbytes;
    }

    return -1;
}


void *stunListen(void *ptr){
    struct pollfd ufds[10];
    struct listenConfig *config = (struct listenConfig *)ptr;
    //struct  turn_info *turnInfo = (struct turn_info *)ptr;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    socklen_t addr_len;
    int rv;
    int numbytes;
    int i;
    StunMessage stunResponse;
    int keyLen = 16;
    char md5[keyLen];

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
                    if((numbytes = recvStunMsg(config->socketConfig[i].sockfd, &their_addr, &stunResponse, buf, MAXBUFLEN-1)) != -1) {
                        if (stunResponse.msgHdr.msgType == STUN_MSG_DataIndicationMsg) {
                            if (stunResponse.hasData) {
                                //Decode and do something with the data?
                            }
                        }
                        if (stunResponse.hasRealm) {
                            memcpy(config->socketConfig[i].realm, stunResponse.realm.value, STUN_MAX_STRING);
                        }
                        
                        if (stunResponse.hasMessageIntegrity) {
                            stunlib_createMD5Key((unsigned char *)md5, 
                                                 config->socketConfig[i].user, 
                                                 config->socketConfig[i].realm, 
                                                 config->socketConfig[i].pass);
                            
                            if (stunlib_checkIntegrity(buf, numbytes, &stunResponse,(unsigned char *)md5, keyLen)) {
                                //printf("     - Integrity check OK\n");
                            } else {
                                printf("     - Integrity check NOT OK\n");
                            }
                        }
                        //printf("   Sending Response to TURN library");
                        //printf("  %p\n", config->socketConfig[i].tInst);
                        TurnClient_HandleIncResp(config->socketConfig[i].tInst,
                                                 &stunResponse,
                                                 buf);
                    }
                }
            }
        }
    }
}


int sendMessage(struct turn_info *turnInfo, char *dstAddr, char *message)
{
    struct sockaddr_storage addr;
    unsigned char stunBuf[200];
    int msg_len;


    if (dstAddr != NULL)
    {
        sockaddr_initFromString((struct sockaddr *)&addr,
                            dstAddr);

        /*
        msg_len = TurnClient_createSendIndication(stunBuf,
                                                  message,
                                                  sizeof(stunBuf),
                                                  strlen(message),
                                                  (struct sockaddr *)&addr,
                                                  false,
                                                  0,
                                                  0);


        if (addr.ss_family == AF_INET){
            if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddr)){

                sendRawUDP(turnInfo->turnAlloc_44.sockfd,
                           stunBuf,
                           msg_len,
                           (struct sockaddr *)&turnInfo->remoteIp4,
                           sizeof( struct sockaddr_storage));



            }

            else if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_64.relAddr)){
                
                sendRawUDP(turnInfo->turnAlloc_64.sockfd,
                           stunBuf,
                           msg_len,
                           (struct sockaddr *)&turnInfo->remoteIp6,
                           sizeof( struct sockaddr_storage));

            }

        }

        if (addr.ss_family == AF_INET6){
            
            if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_46.relAddr)){
                sendRawUDP(turnInfo->turnAlloc_46.sockfd,
                           stunBuf,
                           msg_len,
                           (struct sockaddr *)&turnInfo->remoteIp4,
                           sizeof( struct sockaddr_storage));



            }

            else if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddr)){
                sendRawUDP(turnInfo->turnAlloc_66.sockfd,
                           stunBuf,
                           msg_len,
                           (struct sockaddr *)&turnInfo->remoteIp6,
                           sizeof( struct sockaddr_storage));

            }

        }
        */
    }
    else{
        //Send messge only where channel is bound


    }

    return msg_len;
}




int sendKeepalive(struct turn_info *turnInfo)
{
    char m[] = "KeepAlive";
    
    int len = 0;

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->remoteIp4))
    {

        if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddrIPv4))
        {
            len = sendRawUDP(turnInfo->turnAlloc_44.sockfd,
                             m,
                             sizeof(m),
                             (struct sockaddr *)&turnInfo->remoteIp4,
                             sizeof( struct sockaddr_storage));
        }

        if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_46.relAddrIPv6)){
            len = sendRawUDP(turnInfo->turnAlloc_46.sockfd,
                             m,
                             sizeof(m),
                             (struct sockaddr *)&turnInfo->remoteIp4,
                             sizeof( struct sockaddr_storage));
        }
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->remoteIp6)){
        if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_64.relAddrIPv4))
        {
            len = sendRawUDP(turnInfo->turnAlloc_64.sockfd,
                             m,
                             sizeof(m),
                             (struct sockaddr *)&turnInfo->remoteIp6,
                             sizeof( struct sockaddr_storage));
        }

        if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddrIPv6))
        {
            len = sendRawUDP(turnInfo->turnAlloc_66.sockfd,
                             m,
                             sizeof(m),
                             (struct sockaddr *)&turnInfo->remoteIp6,
                             sizeof( struct sockaddr_storage));
        }
    }
    return len;
}

void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData)
{
    
    struct turn_allocation *turnResult = (struct turn_allocation *)ctx;

    if ( retData->turnResult == TurnResult_AllocOk ){

        sockaddr_copy((struct sockaddr *)&turnResult->activeTurnServerAddr, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.activeTurnServerAddr);

        
        sockaddr_copy((struct sockaddr *)&turnResult->rflxAddr, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.srflxAddr);

        sockaddr_copy((struct sockaddr *)&turnResult->relAddrIPv4, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.relAddrIPv4);

        sockaddr_copy((struct sockaddr *)&turnResult->relAddrIPv6, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.relAddrIPv6);

        
    }else if (retData->turnResult == TurnResult_CreatePermissionOk) {
        
        turnResult->turnPerm.ok = true;
        
        
    }else if (retData->turnResult == TurnResult_RelayReleaseComplete){
        
                
        memset(&turnResult->activeTurnServerAddr, 0,sizeof(struct sockaddr_storage));
        //memset(&turnResult->hostAddr, 0,sizeof(struct sockaddr_storage));
        memset(&turnResult->rflxAddr, 0,sizeof(struct sockaddr_storage));
        memset(&turnResult->relAddrIPv4, 0,sizeof(struct sockaddr_storage));
        memset(&turnResult->relAddrIPv6, 0,sizeof(struct sockaddr_storage));
        memset(&turnResult->turnPerm, 0,sizeof(struct turn_permissions));
        
        
    }
    turnResult->update_turninfo();
}
