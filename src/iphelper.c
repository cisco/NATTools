#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>

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

}

int getLocalIPaddresses(struct turn_info *turnInfo, char *iface)
{

    if (!getLocalInterFaceAddrs( (struct sockaddr *)&turnInfo->localIp4, iface, AF_INET) ){
        //fprintf(stderr,"Unable to find local IPv4 interface addresses\n");
    }else{
        turnInfo->turnAlloc_44.sockfd = createLocalUDPSocket(AF_INET,
                                                             (struct sockaddr *)&turnInfo->localIp4,
                                                             (struct sockaddr *)&turnInfo->turnAlloc_44.hostAddr,
                                                             0);
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

    snprintf(service, 8, "%d", port);


    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST | AI_ADDRCONFIG;


    if ((rv = getaddrinfo(addr, NULL, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s ('%s')\n", gai_strerror(rv), addr);
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next) {

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) {
            printf("Unable to open socket\n");
            continue;
        }

        if (sockaddr_isAddrAny(p->ai_addr) ){
            //printf("Ignoring any\n");
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
               struct sockaddr * p,
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
            //printf("Sending Raw (To: '%s'(%i), Bytes:%i/%i  )\n", addr, sockfd, numbytes, (int)len);

            return numbytes;
        }
    }

    return -1;
}


int SendRawStun(int sockfd,
                uint8_t *buf,
                int len,
                struct sockaddr *addr,
                socklen_t t,
                void *userdata){

    return  sendRawUDP(sockfd, buf, len, addr, t);

}

#define MAXBUFLEN 1024

void *stunListen(void *ptr){
    struct pollfd ufds[10];
    struct listenConfig *config = (struct listenConfig *)ptr;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
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
                    return;
                    }


                    if ( stunlib_isStunMsg(buf, numbytes, &isMsSTUN) ){
                        StunMessage msg;

                        stunlib_DecodeMessage(buf,
                                              numbytes,
                                              &msg,
                                              NULL,
                                              false,
                                              false);


                        if (msg.msgHdr.msgType == STUN_MSG_DataIndicationMsg){

                            if (msg.hasData){
                                buf[numbytes] = '\0';
                                //config->update_inc_status(msg.data.pData);

                                
                                config->update_inc_status(&buf[msg.data.offset]);

                            }
                        }

                        TurnClient_HandleIncResp(TEST_THREAD_CTX,
                                                 config->socketConfig[i].stunCtx,
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

    }
    else{
        //Send messge only where channel is bound


    }


}




int sendKeepalive(struct turn_info *turnInfo)
{
    char m[] = "KeepAlive";

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->remoteIp4))
    {

        if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddr))
        {
            sendRawUDP(turnInfo->turnAlloc_44.sockfd,
                       m,
                       sizeof(m),
                       (struct sockaddr *)&turnInfo->remoteIp4,
                       sizeof( struct sockaddr_storage));
        }

        if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_46.relAddr))
        {
            sendRawUDP(turnInfo->turnAlloc_46.sockfd,
                       m,
                       sizeof(m),
                       (struct sockaddr *)&turnInfo->remoteIp4,
                       sizeof( struct sockaddr_storage));
        }
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->remoteIp6)){
        if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_64.relAddr))
        {
            sendRawUDP(turnInfo->turnAlloc_64.sockfd,
                       m,
                       sizeof(m),
                       (struct sockaddr *)&turnInfo->remoteIp6,
                       sizeof( struct sockaddr_storage));
        }

        if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddr))
        {
            sendRawUDP(turnInfo->turnAlloc_66.sockfd,
                       m,
                       sizeof(m),
                       (struct sockaddr *)&turnInfo->remoteIp6,
                       sizeof( struct sockaddr_storage));
        }
    }
}

void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData)
{
    
    struct turn_allocation_result *turnResult = (struct turn_allocation_result *)ctx;

    
    if ( retData->turnResult == TurnResult_AllocOk ){

        sockaddr_copy((struct sockaddr *)&turnResult->activeTurnServerAddr, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.activeTurnServerAddr);

        
        sockaddr_copy((struct sockaddr *)&turnResult->rflxAddr, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.rflxAddr);

        sockaddr_copy((struct sockaddr *)&turnResult->relAddr, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.relAddr);

        turnResult->update_turninfo();

    }else if (retData->turnResult == TurnResult_CreatePermissionOk) {
        
        turnResult->turnPerm.ok = true;
        
        turnResult->update_turninfo();

    }else if (retData->turnResult == TurnResult_RelayReleaseComplete){
        
        turnResult->stunCtx = -1;
        
        memset(&turnResult->activeTurnServerAddr, 0,sizeof(struct sockaddr_storage));
        //memset(&turnResult->hostAddr, 0,sizeof(struct sockaddr_storage));
        memset(&turnResult->rflxAddr, 0,sizeof(struct sockaddr_storage));
        memset(&turnResult->relAddr, 0,sizeof(struct sockaddr_storage));
        memset(&turnResult->turnPerm, 0,sizeof(struct turn_permissions));
        
        update_turninfo = NULL;

        turnResult->update_turninfo();

    }
    
}
