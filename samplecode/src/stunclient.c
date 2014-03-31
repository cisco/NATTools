/*
** stunclient.c A simple STUN demo
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <pthread.h>

#include <stunlib.h>
#include <stunclient.h>
#include <stun_intern.h>
#include "utils.h"


#define SERVERPORT "4950"    // the port users will be connecting to
#define USERNAME "evtj:h6vY"
#define PASSWORD "VOkJxbRl1RmTxUk/WvJxBt"

#define MAXBUFLEN 500

int sockfd;


void StunStatusCallBack(void *userCtx, StunCallBackData_T *stunCbData)
{
    //ctx points to whatever you initialized the library with. (Not used in this simple example.)
    if(stunCbData->stunResult == StunResult_BindOk)
    {
        char addr[SOCKADDR_MAX_STRLEN];
        printf("   RFLX addr: '%s'\n",
            sockaddr_toString((struct sockaddr *)&stunCbData->rflxAddr,
                addr,
                sizeof(addr),
                true));
        printf("   SRC addr: '%s'\n",
            sockaddr_toString((struct sockaddr *)&stunCbData->srcAddr,
            addr,
            sizeof(addr),
            true));
        printf("   DstBase addr: '%s'\n",
            sockaddr_toString((struct sockaddr *)&stunCbData->dstBaseAddr,
            addr,
            sizeof(addr),
            true));
    }
    else
        printf("   StunResult returned not OK\n");

}


static void *tickStun(STUN_CLIENT_DATA *clientData)
{
    struct timespec timer;
    struct timespec remaining;

    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    for(;;) {
        nanosleep(&timer, &remaining);
        StunClient_HandleTick(clientData, 50);
    }
}


void teardown()
{
  close(sockfd);
  printf("Quitting...\n");
  exit(0);
}


int main(int argc, char *argv[])
{
    struct addrinfo *servinfo, *p;
    int numbytes;
    pthread_t stunTickThread;

    StunMessage stunResponse;

    struct sockaddr_storage their_addr;
    struct sockaddr my_addr;
    socklen_t addrLen = sizeof(my_addr);

    unsigned char buf[MAXBUFLEN];

    const uint32_t priority = 1845494271;
    const uint64_t tieBreaker = 0x932FF9B151263B36LL;
    StunMsgId stunMsgId;
    char msgId[] = "\xb7\xe7\xa7\x01"
        "\xbc\x34\xd6\x86"
        "\xfa\x87\xdf\xae";
    
    STUN_CLIENT_DATA *clientData;

    char addrStr[SOCKADDR_MAX_STRLEN];

    DiscussData discussData;

    memcpy(stunMsgId.octet, msgId, sizeof(stunMsgId.octet));

    if (argc != 2) {
        fprintf(stderr,"usage: stunclient hostname\n");
        exit(1);
    }

    sockfd = createSocket(argv[1], SERVERPORT, 0, servinfo, &p);
    signal(SIGINT, teardown);

    //freeaddrinfo(servinfo);

    StunClient_Alloc(&clientData);
    
    pthread_create(&stunTickThread, NULL, (void *)tickStun, clientData);

    if (getsockname(sockfd, &my_addr, &addrLen) == -1) {
        perror("getsockname");
    }

    discussData.streamType=0x004;
    discussData.interactivity=0x01;

    discussData.networkStatus_flags = 0;
    discussData.networkStatus_nodeCnt = 0;
    discussData.networkStatus_tbd = 0;
    discussData.networkStatus_upMaxBandwidth = 0;
    discussData.networkStatus_downMaxBandwidth = 0;

    StunClient_startBindTransaction(clientData,
                                  NULL,
                                  p->ai_addr,
                                  &my_addr,
                                  false,
                                  USERNAME,
                                  PASSWORD,
                                  priority,
                                  false,
                                  false,
                                  tieBreaker,
                                  stunMsgId,
                                  sockfd,
                                  sendRawStun,
                                  StunStatusCallBack,
                                  &discussData);

    while(1)
    {
        if((numbytes = recvStunMsg(sockfd, &their_addr, &stunResponse, buf)) != -1) {
            if( stunlib_checkIntegrity(buf,
                                       numbytes,
                                       &stunResponse,
                                       (unsigned char*)PASSWORD,
                                       sizeof(PASSWORD)) ) {
                printf("Integrity Check OK\n");

                StunClient_HandleIncResp(clientData, &stunResponse, p->ai_addr);

                printDiscuss(stunResponse);

                break;
            }
        }
    }

    teardown();
}


