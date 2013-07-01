/*
 * turnclient.c -- a simple turnclient demo
 *
 * This is not a socket tutorial or a complete turnclient implementation.
 * Its main purpose is to show how to initialize the STUNlib to use TURN
 * and allocate a RELAY address on a turn server.
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

#include <turnclient.h>
#include <sockaddr_util.h>

#include "utils.h"

#define SERVERPORT "3478"    // the port users will be connecting to

static const uint32_t TEST_THREAD_CTX = 1;

static void *tickTurn(void *ptr);
int SendRawStun(int sockfd,
                uint8_t *buf,
                int len,
                struct sockaddr *addr,
                socklen_t t,
                void *userdata);

void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData);
void handleInt();
void listenAndHandleResponse();

int ctx;
int sockfd;
char realm[STUN_MAX_STRING];

int main(int argc, char *argv[])
{
    struct addrinfo *servinfo, *p;

    signal(SIGINT, handleInt);

    pthread_t turnTickThread;
    TurnCallBackData_T TurnCbData;

    sockfd = createSocket(argv[1], SERVERPORT, "turnclient", 0, servinfo, &p);
    if(sockfd == -1) {
      return 1;
    } else if(sockfd == -2) {
      return 2;
    }

    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, "TestTurn");
    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);


    ctx = TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
                                        NULL,
                                        p->ai_addr,
                                        argv[2],
                                        argv[3],
                                        sockfd,                       /* socket */
                                        AF_INET,
                                        sendRawStun,             /* send func */
                                        NULL,  /* timeout list */
                                        TurnStatusCallBack,
                                        &TurnCbData,
                                        false,
                                        false,
                                        0);

    freeaddrinfo(servinfo);

    while (1) {
        //We listen on the socket for any response and feed it back to the library.
        //In a real world example this would happen in a listen thread..

        listenAndHandleResponse(argv[2], argv[3]);
    }
}

void listenAndHandleResponse(char *user, char *password)
{
    int numbytes;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    StunMessage stunResponse;
    int keyLen = 16;
    char md5[keyLen];

    if((numbytes = recvStunMsg(sockfd, &their_addr, &stunResponse, buf)) != -1) {
         if (stunResponse.msgHdr.msgType == STUN_MSG_DataIndicationMsg) {
             if (stunResponse.hasData) {
               //Decode and do something with the data?
             }
         }

         if (stunResponse.hasRealm) {
            memcpy(realm, stunResponse.realm.value, STUN_MAX_STRING);
         }

         if (stunResponse.hasMessageIntegrity) {
            printf("   Integrity attribute present.\n");
            stunlib_createMD5Key((unsigned char *)md5, user, realm, password);

            if (stunlib_checkIntegrity(buf, numbytes, &stunResponse, md5, keyLen)) {
                printf("     - Integrity check OK\n");
            } else {
                printf("     - Integrity check NOT OK\n");
            }
         }
         printf("   Sending Response to TURN library\n");
         TurnClient_HandleIncResp(TEST_THREAD_CTX,
                                  TURNCLIENT_CTX_UNKNOWN,
                                  &stunResponse,
                                  buf);
    }
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
        TurnClient_HandleTick(*ctx);
    }

}

void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData)
{
    //ctx points to whatever you initialized the library with. (Not used in this simple example.)

    if ( retData->turnResult == TurnResult_AllocOk ){
        char addr[SOCKADDR_MAX_STRLEN];

        printf("Successfull Allocation: \n");
        printf("   Active TURN server: '%s'\n",
               sockaddr_toString((struct sockaddr *)&retData->TurnResultData.AllocResp.activeTurnServerAddr,
                                 addr,
                                 sizeof(addr),
                                 true));

        printf("   RFLX addr: '%s'\n",
               sockaddr_toString((struct sockaddr *)&retData->TurnResultData.AllocResp.rflxAddr,
                                 addr,
                                 sizeof(addr),
                                 true));

        printf("   RELAY addr: '%s'\n",
               sockaddr_toString((struct sockaddr *)&retData->TurnResultData.AllocResp.relAddr,
                                 addr,
                                 sizeof(addr),
                                 true));


    } else if (retData->turnResult == TurnResult_AllocUnauthorised) {
        printf("Unable to authorize. Wrong user/pass?\n");
    }
}

void handleInt()
{
  printf("\nDeallocating...\n");
  TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
  // If one wants to avoid the server getting a destination unreachable ICMP message,
  // handle the response before quitting.
  // listenAndHandleResponse();
  close(sockfd);
  printf("Quitting...\n");
  exit(0);
}
