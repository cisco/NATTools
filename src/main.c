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

#include "turnclient.h"
#include "sockaddr_util.h"


#define PORT "5793"
//#define  TEST_THREAD_CTX 1
#define MAXBUFLEN 1024

static const uint32_t TEST_THREAD_CTX = 1;

static int createLocalUDPSocket(int ai_family){
    int sockfd;
    
    int rv;
    int yes = 1;
    struct addrinfo hints, *ai, *p;


    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }


    for(p = ai; p != NULL; p = p->ai_next) {
        
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) { 
            continue;
        }
        
        // lose the pesky "address already in use" error message
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            continue;
        }
        
        break;
    }
    printf("Soket open: %i\n", sockfd);
    return sockfd;
}





static int sendRawUDP(int sockfd, 
                      const void *buf, 
                      size_t len, 
                      struct sockaddr * p, 
                      socklen_t t){
    
    int numbytes;
    char addr[256];

    sockaddr_toString(p, addr, 256, true); 
    numbytes = sendto(sockfd, buf, len, 0,
                      p , t); 
                      
    printf("Sending Raw (To: '%s'(%i), Bytes:%i/%i  )\n", addr, sockfd, numbytes, (int)len);
    
    return numbytes;

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
    printf("Got TURN status callback (%i)\n", retData->turnResult);
}


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


int main(int argc, char *argv[])
{
    int sockfd_4, sockfd_6, sockfd;
    int sendt_numbytes, rcv_numbytes;;
    struct sockaddr_storage remote_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    bool isMsSTUN;
    int stunCtx;
    struct sockaddr_storage ss_addr;
    static TurnCallBackData_T TurnCbData;

    pthread_t turnTickThread;
    pthread_t recieveThread;

    char key[] = "pem";
    
    
    

    if (argc != 4) {
        fprintf(stderr,"usage: ice  [ip:port] user pass\n");
        exit(1);
    }
    
    

    sockfd_4 = createLocalUDPSocket(AF_INET);
    sockfd_6 = createLocalUDPSocket(AF_INET6);


    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "TestIce");

    sockaddr_initFromString((struct sockaddr *)&ss_addr, 
                            argv[1]);

    if(ss_addr.ss_family == AF_INET){
        printf("Using IPv4 Socket\n");
        sockfd = sockfd_4;
    }else if(ss_addr.ss_family == AF_INET6){
        printf("Using IPv6 Socket\n");
        sockfd = sockfd_6;
    }


    stunCtx = TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
                                                  NULL,
                                                  (struct sockaddr *)&ss_addr,
                                                  argv[2],
                                                  argv[3],
                                                  sockfd,                       /* socket */
                                                  SendRawStun,             /* send func */
                                                  NULL,  /* timeout list */
                                                  TurnStatusCallBack,
                                                  &TurnCbData,
                                                  false);
    
    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);

        
    addr_len = sizeof remote_addr;
    while(1){
        if ((rcv_numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                                     (struct sockaddr *)&remote_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }
        
        printf("listener: got packet from %s\n",
               sockaddr_toString((struct sockaddr *)&remote_addr, s, sizeof s, true));
        printf("listener: packet is %d bytes long\n", rcv_numbytes);
        
        
        if ( stunlib_isStunMsg(buf, rcv_numbytes, &isMsSTUN) ){
            printf("GOT stun packet...\n", buf);
            StunMessage msg;
            char pass[128] = "pem:medianetworkservices.com:";
            
            
            stunlib_DecodeMessage(buf,
                                  rcv_numbytes,
                                  &msg,
                                  NULL,
                                  false,
                                  false);
            
            TurnClient_HandleIncResp(TEST_THREAD_CTX,
                                     stunCtx, 
                                     &msg);
            
        }
    }
    close(sockfd);

    return 0;
}
