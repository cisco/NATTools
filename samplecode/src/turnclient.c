/*
 * turnclient.c -- a simple turnclient demo
 *
 * This is not a socket tutorial or a complete turnclient implementation.
 * Its main purpose is to show how to initialize the STUNlib to use TURN
 * and allocate a RELAY address on a turn server.
 */

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

#define MAXBUFLEN 200

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


int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    socklen_t addr_len;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    bool isMsStun;

    StunMessage stunResponse;

    pthread_t turnTickThread;
    TurnCallBackData_T    TurnCbData;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("stunlient: socket");
            continue;
        }
        
        break;
    }
    
    if (p == NULL) {
        fprintf(stderr, "turnclient: failed to bind socket\n");
        return 2;
    }

    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, "TestTurn");
    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);

    
    TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
                                        NULL,
                                        p->ai_addr,
                                        argv[2],
                                        argv[3],
                                        sockfd,                       /* socket */
                                        AF_INET,
                                        SendRawStun,             /* send func */
                                        NULL,  /* timeout list */
                                        TurnStatusCallBack,
                                        &TurnCbData,
                                        false,
                                        false,
                                        0);
    
    while(1){
        //We listen on the socket for any response and feed it back to the library. 
        //In a real world example this would happen in a listen thread..

        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                                 (struct sockaddr *)&their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        
        
        printf("stunclient: Got a packet that is %d bytes long\n", numbytes);        
        if(stunlib_isStunMsg(buf, numbytes, &isMsStun)){
            printf("   Packet is STUN\n");
            
            stunlib_DecodeMessage( buf, 
                                   numbytes, 
                                   &stunResponse, 
                                   NULL, 
                                   false, 
                                   false);
            
           if (stunResponse.msgHdr.msgType == STUN_MSG_DataIndicationMsg){

               if (stunResponse.hasData){
                   //Decode and do something with the data?
                   
               }
           }
         
           printf("   Sending Response to TURN library\n");
           TurnClient_HandleIncResp(TEST_THREAD_CTX,
                                    TURNCLIENT_CTX_UNKNOWN,
                                    &stunResponse,
                                    buf);
        }
    }
    //We never get her... but...
    close(sockfd);
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


void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData){

    //ctx points to whatever you initialized the library with. (Not used in this simple example.)
    
    if ( retData->turnResult == TurnResult_AllocOk ){
        char addr[SOCKADDR_MAX_STRLEN];

        printf("Sucsessfull Allocation: \n");
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
        
        
    }else if (retData->turnResult == TurnResult_AllocUnauthorised) {
        printf("Unable to authorize. Wrong user/pass?\n");
    }
}


int sendRawUDP(int sockfd,
               const void *buf,
               size_t len,
               struct sockaddr * p,
               socklen_t t){

    int numbytes;
    char addr[256];
    int rv;

    numbytes = sendto(sockfd, buf, len, 0,
                      p , t);

    sockaddr_toString(p, addr, 256, true);
    printf("Sending Raw (To: '%s'(%i), Bytes:%i/%i  )\n", addr, sockfd, numbytes, (int)len);
    
    return numbytes;
}


int SendRawStun(int sockfd,
                uint8_t *buf,
                int len,
                struct sockaddr *addr,
                socklen_t t,
                void *userdata){
    return  sendRawUDP(sockfd, buf, len, addr, t);
}


