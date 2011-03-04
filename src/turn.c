#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>


#include "sockaddr_util.h"
#include "gather.h"
#include "ipaddress_helper.h"



/* Callback for management info  */
static void  PrintTurnInfo(TurnInfoCategory_T category, char *ErrStr)
{
    //fprintf(stderr, "%s\n", ErrStr);
}

static void *tickTurn(void *ptr){
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


int main(int argc, char *argv[])
{
    struct turn_info turnInfo;

    pthread_t turnTickThread;
    pthread_t turnListenThread;

    struct listenConfig listenConfig;
    
    

    if (argc != 5) {
        fprintf(stderr,"usage: testice  iface turnserver user pass\n");
        exit(1);
    }

    memset(&turnInfo, 0, sizeof turnInfo);

    
    strncpy(turnInfo.fqdn, argv[2], FQDN_MAX_LEN);
    strncpy(turnInfo.user, argv[3], STUN_MSG_MAX_USERNAME_LENGTH);
    strncpy(turnInfo.pass, argv[4], STUN_MSG_MAX_PASSWORD_LENGTH);
            


    getRemoteTurnServerIp(&turnInfo, argv[2]);

    getLocalIPaddresses(&turnInfo, argv[1]);



    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "TestIce");
    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);

    gatherAll(&turnInfo, &listenConfig);
    
    pthread_create( &turnListenThread, NULL, stunListen, (void*)&listenConfig);

    //stunListen(&listenConfig);


    while(1){
        int test;
        test = getchar();
        
        printTurnInfo(&turnInfo);

    };

    return 0;
}
