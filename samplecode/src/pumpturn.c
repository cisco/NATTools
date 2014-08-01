#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <time.h>
#include <pthread.h>

#include "iphelper.h"
#include "gather.h"


static int keep_ticks; //Used to determine if it is time to send keepalives
static void *tickTurn(void *ptr);

void messageCB(char *message);
void print_turnInfo();

static struct turn_info turnInfo;
static struct listenConfig listenConfig;
static pthread_t turnListenThread;


static char rcv_message[100];

pthread_mutex_t turnInfo_mutex = PTHREAD_MUTEX_INITIALIZER;


static int socket_type;

int main(int argc, char *argv[])
{
    pthread_t turnTickThread;


    if (argc != 6) {
        fprintf(stderr,"usage: pumpturn iface [TCP|UDP] turnserver user pass\n");
        exit(1);
    }

    if( strcmp(argv[2], "UDP") == 0 ){
        socket_type = SOCK_DGRAM;
    }else{
        socket_type = SOCK_STREAM;
    }
    

    //Our local struct where we store stuff
    initTurnInfo(&turnInfo);
    addCredentials(&turnInfo, argv[3], argv[4], argv[5]);

    listenConfig.update_inc_status = messageCB;

    getRemoteTurnServerIp(&turnInfo, argv[3]);

    
    getLocalIPaddresses(&turnInfo, socket_type, argv[1]);

    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, "PumpTurn");
    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);
    

    printf("Gather!!!\n");
    gatherAll(&turnInfo, &listenConfig, &print_turnInfo);
    pthread_create( &turnListenThread, NULL, stunListen, (void*)&listenConfig);
   
    printf("Her er jeg\n");
    while(1)       
    {	
        getch();
    } 
    return 0;
}


void messageCB(char *message)
{
    int n = sizeof(rcv_message);
    strncpy(rcv_message, message, n);
    if (n > 0)
        rcv_message[n - 1]= '\0';

    fprintf(stdout,"%s", rcv_message);

}

void print_turnInfo(){
    char addr[SOCKADDR_MAX_STRLEN];
    pthread_mutex_lock( &turnInfo_mutex );
    fprintf(stdout, "RELAY addr:         '%s'\n",
            sockaddr_toString((struct sockaddr *)&turnInfo.turnAlloc_44.relAddr,
                              addr,
                              sizeof(addr),
                              true));
    pthread_mutex_unlock( &turnInfo_mutex );
}




static void *tickTurn(void *ptr){
    struct timespec timer;
    struct timespec remaining;
    uint32_t  *ctx = (uint32_t *)ptr;

    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    
    for(;;){
        keep_ticks++;
        nanosleep(&timer, &remaining);
        TurnClient_HandleTick(*ctx);

        if(keep_ticks>100){
            sendKeepalive(&turnInfo);
            keep_ticks = 0;
        }
    }

}
