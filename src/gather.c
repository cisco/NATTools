
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


void printAllocationResult(struct turn_allocation_result *result)
{

    char addr[SOCKADDR_MAX_STRLEN];
    
    printf("   Active TURN server: '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->activeTurnServerAddr,
                             addr,
                             sizeof(addr),
                             true));

    printf("   RFLX addr: '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->rflxAddr,
                             addr,
                             sizeof(addr),
                             true));
    
    printf("   RELAY addr: '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->relAddr,
                             addr,
                             sizeof(addr),
                             true));
    

}


int printAllocationResultw(WINDOW *win, int startx, int starty, struct turn_allocation_result *result)
{

    char addr[SOCKADDR_MAX_STRLEN];
    
    mvwprintw(win, starty++, startx,"Active TURN server: '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->activeTurnServerAddr,
                             addr,
                             sizeof(addr),
                             true));

    mvwprintw(win, starty++, startx,"HOST addr:          '%s'\n",
              sockaddr_toString((struct sockaddr *)&result->hostAddr,
                                addr,
                                sizeof(addr),
                                true));
    

    mvwprintw(win, starty++, startx,"RFLX addr:          '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->rflxAddr,
                             addr,
                             sizeof(addr),
                             true));
    
    mvwprintw(win, starty++, startx,"RELAY addr:         '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->relAddr,
                             addr,
                             sizeof(addr),
                             true));
    
    return starty;
}


void printTurnInfo( struct turn_info *turnInfo )
{
    char addr[SOCKADDR_MAX_STRLEN];

    printf("--- TURN Info (start) -----\n");
    printf("TURN Server     : '%s'\n", turnInfo->fqdn);
    printf("TURN Server IPv4: '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->remoteIp4,
                                                         addr,
                                                         sizeof(addr),
                                                         true));

    printf("TURN Server IPv6: '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->remoteIp6,
                                                         addr,
                                                         sizeof(addr),
                                                         true));
    printf("\n");
    
    printf("Loval IPv4      : '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->localIp4,
                                                         addr,
                                                         sizeof(addr),
                                                         true));

    printf("Loval IPv6      : '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->localIp6,
                                                         addr,
                                                         sizeof(addr),
                                                         true));

    printf("TURN 44 Result\n");
    printAllocationResult(&turnInfo->turnAlloc_44);

    printf("TURN 46 Result\n");
    printAllocationResult(&turnInfo->turnAlloc_46);

    printf("TURN 64 Result\n");
    printAllocationResult(&turnInfo->turnAlloc_64);
    
    printf("TURN 66 Result\n");
    printAllocationResult(&turnInfo->turnAlloc_66);

    printf("--- TURN Info (end) -----\n");
    
}

void wprintTurnInfow( WINDOW *win, struct turn_info *turnInfo )
{
    char addr[SOCKADDR_MAX_STRLEN];
    int starty = 1;
    int startx = 2;

    mvwprintw(win, starty++, startx, "TURN Server     : '%s'\n", turnInfo->fqdn);
    mvwprintw(win, starty++, startx,"TURN Server IPv4: '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->remoteIp4,
                                                         addr,
                                                         sizeof(addr),
                                                         true));

    mvwprintw(win, starty++, startx,"TURN Server IPv6: '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->remoteIp6,
                                                         addr,
                                                         sizeof(addr),
                                                         true));
    
    
    mvwprintw(win, starty++, startx,"Loval IPv4      : '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->localIp4,
                                                         addr,
                                                         sizeof(addr),
                                                         false));

    mvwprintw(win, starty++, startx,"Loval IPv6      : '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->localIp6,
                                                         addr,
                                                         sizeof(addr),
                                                         false));

    mvwprintw(win, starty++, startx,"TURN 44 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_44);

    mvwprintw(win, starty++, startx,"TURN 46 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_46);

    mvwprintw(win, starty++, startx,"TURN 64 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_64);
    
    mvwprintw(win, starty++, startx,"TURN 66 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_66);

    mvwprintw(win, starty, startx, "Pending allocations: %i", turnInfo->numPending);
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
    
    
    return  TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
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
    
    listenConfig->numSockets = idx+1;
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
        ufds[i].events = POLLIN | POLLPRI; // check for normal or out-of-band
    }

    addr_len = sizeof their_addr;

    while(1){

        rv = poll(ufds, config->numSockets, -1);

        //printf("rv:%i\n", rv);
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


