
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <pthread.h>


#include "turnclient.h"
#include "sockaddr_util.h"

#include "gather.h"



static int sendRawUDP(int sockfd,
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


static int SendRawStun(int sockfd,
                       uint8_t *buf,
                       int len,
                       struct sockaddr *addr,
                       socklen_t t,
                       void *userdata){

    return  sendRawUDP(sockfd, buf, len, addr, t);

}


static void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData)
{

    struct turn_allocation_result *turnResult = (struct turn_allocation_result *)ctx;

    
    //printf("Got TURN status callback (%i)\n", retData->turnResult);

    
    if ( retData->turnResult == TurnResult_AllocOk ){

        sockaddr_copy((struct sockaddr *)&turnResult->activeTurnServerAddr, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.activeTurnServerAddr);

        
        sockaddr_copy((struct sockaddr *)&turnResult->rflxAddr, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.rflxAddr);

        sockaddr_copy((struct sockaddr *)&turnResult->relAddr, 
                      (struct sockaddr *)&retData->TurnResultData.AllocResp.relAddr);

        turnResult->update_turninfo();

    }
}

int gather(struct sockaddr *host_addr, 
           int requestedFamily, 
           char *user, 
           char *pass,
           struct turn_allocation_result *turnResult){
    
    int stunCtx;
    TurnCallBackData_T TurnCbData;
    
    
    stunCtx = TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
                                                  turnResult,
                                                  host_addr,
                                                  user,
                                                  pass,
                                                  turnResult->sockfd,                       /* socket */
                                                  requestedFamily,
                                                  SendRawStun,             /* send func */
                                                  NULL,  /* timeout list */
                                                  TurnStatusCallBack,
                                                  &TurnCbData,
                                                  false);
    turnResult->stunCtx = stunCtx;

    return stunCtx;
            
}


void gatherAll(struct turn_info *turnInfo, struct listenConfig *listenConfig, void(*update_turninfo)(void))
{

    int stunCtx;
    int idx = 0;

    turnInfo->numPending = 0;
    
    if( sockaddr_isSet((struct sockaddr *)&turnInfo->localIp4) &&
        sockaddr_isSet((struct sockaddr *)&turnInfo->remoteIp4) )
    {
        turnInfo->numPending++;
        turnInfo->turnAlloc_44.update_turninfo = update_turninfo;
        stunCtx = gather((struct sockaddr *)&turnInfo->remoteIp4, 
                         AF_INET, 
                         turnInfo->user, 
                         turnInfo->pass,
                         &turnInfo->turnAlloc_44);
        listenConfig->socketConfig[idx].stunCtx = stunCtx;
        listenConfig->socketConfig[idx].sockfd = turnInfo->turnAlloc_44.sockfd;
        listenConfig->socketConfig[idx].user = turnInfo->user;
        listenConfig->socketConfig[idx].pass = turnInfo->pass;
        idx++;
        
        turnInfo->numPending++;
        turnInfo->turnAlloc_46.update_turninfo = update_turninfo;
        stunCtx = gather((struct sockaddr *)&turnInfo->remoteIp4, 
                         AF_INET6, 
                         turnInfo->user, 
                         turnInfo->pass,
                         &turnInfo->turnAlloc_46);
        listenConfig->socketConfig[idx].stunCtx = stunCtx;
        listenConfig->socketConfig[idx].sockfd = turnInfo->turnAlloc_46.sockfd;
        listenConfig->socketConfig[idx].user = turnInfo->user;
        listenConfig->socketConfig[idx].pass = turnInfo->pass;
        idx++;
    }
    
    if( sockaddr_isSet((struct sockaddr *)&turnInfo->localIp6) &&
        sockaddr_isSet((struct sockaddr *)&turnInfo->remoteIp6) )
    {

        turnInfo->numPending++;
        turnInfo->turnAlloc_64.update_turninfo = update_turninfo;
        stunCtx = gather((struct sockaddr *)&turnInfo->remoteIp6, 
                         AF_INET, 
                         turnInfo->user, 
                         turnInfo->pass,
                         &turnInfo->turnAlloc_64);
        listenConfig->socketConfig[idx].stunCtx = stunCtx;
        listenConfig->socketConfig[idx].sockfd = turnInfo->turnAlloc_64.sockfd;
        listenConfig->socketConfig[idx].user = turnInfo->user;
        listenConfig->socketConfig[idx].pass = turnInfo->pass;
        idx++;

        turnInfo->numPending++;
        turnInfo->turnAlloc_66.update_turninfo = update_turninfo;
        stunCtx = gather((struct sockaddr *)&turnInfo->remoteIp6, 
                         AF_INET6, 
                         turnInfo->user, 
                         turnInfo->pass,
                         &turnInfo->turnAlloc_66);
        listenConfig->socketConfig[idx].stunCtx = stunCtx;
        listenConfig->socketConfig[idx].sockfd = turnInfo->turnAlloc_66.sockfd;
        listenConfig->socketConfig[idx].user = turnInfo->user;
        listenConfig->socketConfig[idx].pass = turnInfo->pass;
        idx++;

    }
    
    listenConfig->numSockets = idx;
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
                        //perror("recvfrom");
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



                        TurnClient_HandleIncResp(TEST_THREAD_CTX,
                                                 config->socketConfig[i].stunCtx,
                                                 &msg,
                                                 buf);
                    }
                }
            }
        }
    }
}


