
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



#include "turnclient.h"
#include "sockaddr_util.h"

#include "gather.h"


//#define  TEST_THREAD_CTX 1
#define MAXBUFLEN 1024




void *tickTurn(void *ptr){
    struct timespec timer;
    struct timespec remaining;
    uint32_t  *ctx = (uint32_t *)ptr;

    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    for(;;){
        nanosleep(&timer, &remaining);
        TurnClient_HandleTick(*ctx);
    }
}


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
            printf("Sending Raw (To: '%s'(%i), Bytes:%i/%i  )\n", addr, sockfd, numbytes, (int)len);

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


/* Callback for management info  */
static void  PrintTurnInfo(TurnInfoCategory_T category, char *ErrStr)
{
    //fprintf(stderr, "%s\n", ErrStr);
}


static void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData)
{
    char addr[SOCKADDR_MAX_STRLEN];
    printf("Got TURN status callback (%i)\n", retData->turnResult);
    if ( retData->turnResult == TurnResult_AllocOk ){
        printf("Active TURN server: '%s'\n",
               sockaddr_toString((struct sockaddr *)&retData->TurnResultData.AllocResp.activeTurnServerAddr,
                                 addr,
                                 sizeof(addr),
                                 true));

        printf("RFLX addr: '%s'\n",
               sockaddr_toString((struct sockaddr *)&retData->TurnResultData.AllocResp.rflxAddr,
                                 addr,
                                 sizeof(addr),
                                 true));

        printf("RELAY addr: '%s'\n",
               sockaddr_toString((struct sockaddr *)&retData->TurnResultData.AllocResp.relAddr,
                                 addr,
                                 sizeof(addr),
                                 true));

        retData->TurnResultData.AllocResp;
    }

}




void *stunListen(void *ptr){
    struct pollfd ufds[1];
    struct listenConfig *config = (struct listenConfig *)ptr;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    int rv;
    int numbytes;
    bool isMsSTUN;

    ufds[0].fd = config->sockfd;
    ufds[0].events = POLLIN | POLLPRI; // check for normal or out-of-band

    addr_len = sizeof their_addr;

    while(1){
        rv = poll(ufds, 1, -1);

        if (rv == -1) {
            perror("poll"); // error occurred in poll()
        } else if (rv == 0) {
            printf("Timeout occurred! (Should not happen)\n");
        } else {
            // check for events on s1:
            if (ufds[0].revents & POLLIN) {

                if ((numbytes = recvfrom(config->sockfd, buf, MAXBUFLEN-1 , 0,
                                         (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom");
                    exit(1);
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
                                             config->stunCtx,
                                             &msg,
                                             buf);
                }
            }
        }
    }
}


void gather(struct sockaddr *host_addr, int sockfd, char *user, char *pass){
    struct listenConfig listenConfig;
    int stunCtx;
    TurnCallBackData_T TurnCbData;
    pthread_t turnTickThread;
    pthread_t listenThread;

    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "TestIce");

    


    stunCtx = TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
                                                  NULL,
                                                  host_addr,
                                                  user,
                                                  pass,
                                                  sockfd,                       /* socket */
                                                  AF_INET6,
                                                  SendRawStun,             /* send func */
                                                  NULL,  /* timeout list */
                                                  TurnStatusCallBack,
                                                  &TurnCbData,
                                                  false);

    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);

    listenConfig.stunCtx = stunCtx;
    listenConfig.sockfd = sockfd;
    listenConfig.user = user;
    listenConfig.pass = pass;



    pthread_create( &listenThread,   NULL, stunListen, (void*) &listenConfig);

    //stunListen(&listenConfig);
    pthread_join( &listenThread, NULL);
}
