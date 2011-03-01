
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






int gather(struct sockaddr *host_addr, 
            int sockfd, 
            int requestedFamily, 
            char *user, 
            char *pass){
    
    int stunCtx;
    TurnCallBackData_T TurnCbData;
    
    
    
    printf("Gather: Sockfd:%i\n", sockfd);
    

    return  TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
                                                NULL,
                                                host_addr,
                                                user,
                                                pass,
                                                sockfd,                       /* socket */
                                                requestedFamily,
                                                SendRawStun,             /* send func */
                                                NULL,  /* timeout list */
                                                TurnStatusCallBack,
                                                &TurnCbData,
                                                false);

            
}
