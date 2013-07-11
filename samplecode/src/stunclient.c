/*
** talker.c -- a datagram "client" demo
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

#include "utils.h"

#define SERVERPORT "4950"    // the port users will be connecting to

static const uint32_t TEST_THREAD_CTX = 1;

static void *tickTurn(void *ptr);
void StunStatusCallBack(void *ctx, StunCallBackData_T *retData);
void teardown();

int sockfd;
int ctx;

int main(int argc, char *argv[])
{
    struct addrinfo *servinfo, *p;
    int numbytes;
    pthread_t stunTickThread;
    StunCallBackData_T stunCbData;

    StunMessage stunResponse;
    char buffer[256];
    int msg_len;

    struct sockaddr_storage their_addr;
    struct sockaddr my_addr;
    socklen_t addrLen = sizeof(my_addr);

    unsigned char buf[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];

    char username[] = "evtj:h6vY";
    char password[] = "VOkJxbRl1RmTxUk/WvJxBt";
    const uint32_t priority = 1845494271;
    const uint64_t tieBreaker = 0x932FF9B151263B36LL;
    char *software= "STUN test client\0";
    StunMsgId stunMsgId;
    char msgId[] = "\xb7\xe7\xa7\x01"
        "\xbc\x34\xd6\x86"
        "\xfa\x87\xdf\xae";

    /*********************** start MALICE specific **************************/
    MaliceMetadata maliceMetadata;
    
    maliceMetadata.hasMDAgent = true;
    maliceMetadata.mdAgent.hasFlowdataReq = true;
    maliceMetadata.mdAgent.flowdataReq.flowdataUP.DT = 0;
    maliceMetadata.mdAgent.flowdataReq.flowdataUP.LT = 1;
    maliceMetadata.mdAgent.flowdataReq.flowdataUP.JT = 2;
    maliceMetadata.mdAgent.flowdataReq.flowdataUP.minBW = 333;
    maliceMetadata.mdAgent.flowdataReq.flowdataUP.maxBW = 444;
    maliceMetadata.mdAgent.flowdataReq.flowdataDN.DT = 3;
    maliceMetadata.mdAgent.flowdataReq.flowdataDN.LT = 4;
    maliceMetadata.mdAgent.flowdataReq.flowdataDN.JT = 2;
    maliceMetadata.mdAgent.flowdataReq.flowdataDN.minBW = 111;
    maliceMetadata.mdAgent.flowdataReq.flowdataDN.maxBW = 222;

    maliceMetadata.hasMDRespUP = true;
    maliceMetadata.mdRespUP.hasFlowdataResp = true;
    maliceMetadata.mdRespUP.flowdataResp.DT = 0;
    maliceMetadata.mdRespUP.flowdataResp.LT = 1;
    maliceMetadata.mdRespUP.flowdataResp.JT = 2;
    maliceMetadata.mdRespUP.flowdataResp.minBW = 333;
    maliceMetadata.mdRespUP.flowdataResp.maxBW = 444;

    maliceMetadata.hasMDRespDN = true;
    maliceMetadata.mdRespDN.hasFlowdataResp = true;
    maliceMetadata.mdRespDN.flowdataResp.DT = 3;
    maliceMetadata.mdRespDN.flowdataResp.LT = 4;
    maliceMetadata.mdRespDN.flowdataResp.JT = 2;
    maliceMetadata.mdRespDN.flowdataResp.minBW = 111;
    maliceMetadata.mdRespDN.flowdataResp.maxBW = 222;

    maliceMetadata.hasMDPeerCheck = true;

    /*********************** end MALICE specific **************************/

    memcpy(stunMsgId.octet, msgId, sizeof(stunMsgId.octet));

    if (argc != 2) {
        fprintf(stderr,"usage: stunclient hostname\n");
        exit(1);
    }

    sockfd = createSocket(argv[1], SERVERPORT, "stunclient", 0, servinfo, &p);
    signal(SIGINT, teardown);

    freeaddrinfo(servinfo);

    StunClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, software);
    pthread_create(&stunTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);

    if (getsockname(sockfd, &my_addr, &addrLen) == -1) {
      perror("getsockname");
    }



    ctx = StunClient_startBindTransaction(TEST_THREAD_CTX,
                                          NULL,
                                          p->ai_addr,
                                          &my_addr,
                                          false,
                                          username,
                                          password,
                                          priority,
                                          false,
                                          false,
                                          tieBreaker,
                                          stunMsgId,
                                          sockfd,
                                          sendRawStun,
                                          NULL,
                                          StunStatusCallBack,
                                          &stunCbData,
                                          0,
                                          &maliceMetadata);

    while(1)
    {
        if((numbytes = recvStunMsg(sockfd, &their_addr, &stunResponse, buf)) != -1) {
            if( stunlib_checkIntegrity(buf,
                                       numbytes,
                                       &stunResponse,
                                       password,
                                       sizeof(password)) ) {
                printf("Integrity Check OK\n");

                StunClient_HandleIncResp(TEST_THREAD_CTX, &stunResponse, p->ai_addr);

                printMalice(stunResponse);

                break;
            }
        }
    }

    teardown();
}

void StunStatusCallBack(void *ctx, StunCallBackData_T *retData)
{
    //ctx points to whatever you initialized the library with. (Not used in this simple example.)
    if(retData->stunResult == StunResult_BindOk)
    {
        char addr[SOCKADDR_MAX_STRLEN];
        printf("   RFLX addr: '%s'\n",
            sockaddr_toString((struct sockaddr *)&retData->bindResp.rflxAddr,
                addr,
                sizeof(addr),
                true),
            retData->bindResp.rflxPort);
        printf("   SRC addr: '%s'\n",
            sockaddr_toString((struct sockaddr *)&retData->srcAddrStr,
            addr,
            sizeof(addr),
            true));
        printf("   DstBase addr: '%s'\n",
            sockaddr_toString((struct sockaddr *)&retData->dstBaseAddrStr,
            addr,
            sizeof(addr),
            true));
    }
    else
        printf("   StunResult returned not OK\n");

}

static void *tickTurn(void *ptr)
{
    struct timespec timer;
    struct timespec remaining;
    uint32_t  *ctx = (uint32_t *)ptr;

    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    for(;;) {
        nanosleep(&timer, &remaining);
        StunClient_HandleTick(*ctx);
    }
}

void teardown()
{
  close(sockfd);
  printf("Quitting...\n");
  exit(0);
}
