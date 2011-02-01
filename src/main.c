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
#include <time.h>

#include "turnclient.h"
#include "sockaddr_util.h"


#define PORT "5799"
//#define  TEST_THREAD_CTX 1
#define MAXBUFLEN 1024

static const uint32_t TEST_THREAD_CTX = 1;


struct listenConfig{
    int stunCtx;
    int sockfd;
    char* user;
    char* pass;


};

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
    int rv;

    struct pollfd ufds[1];


    ufds[0].fd = sockfd;
    ufds[0].events = POLLOUT;

    rv = poll(ufds, 1, 3500);
    
    if (rv == -1) {
        perror("poll"); // error occurred in poll()
    } else if (rv == 0) {
        printf("Timeout occurred!  No data after 3.5 seconds.\n");
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


void *stunListen(void *ptr){

}

int main(int argc, char *argv[])
{
    int sockfd_4, sockfd_6, sockfd;
        
    socklen_t addr_len;
    
    int stunCtx;
    struct sockaddr_storage ss_addr;
    static TurnCallBackData_T TurnCbData;

    struct listenConfig listenConfig;
  

    pthread_t turnTickThread;
    pthread_t listenThread;
    
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];
    bool isMsSTUN;
    char realm[256];
    
    int rv;
    struct pollfd ufds[1];


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

    listenConfig.stunCtx = stunCtx;
    listenConfig.sockfd = sockfd;
    listenConfig.user = argv[2];
    listenConfig.pass = argv[3];



    //pthread_create( &listenThread,   NULL, stunListen, (void*) &listenConfig);

    //stunListen(&listenConfig);
    //pthread_join( &listenThread, NULL);
        
    //addr_len = 
    ufds[0].fd = sockfd;
    ufds[0].events = POLLIN | POLLPRI; // check for normal or out-of-band
    
    while(1){
        rv = poll(ufds, 1, 3500);
        
        if (rv == -1) {
            perror("poll"); // error occurred in poll()
        } else if (rv == 0) {
            printf("Timeout occurred!  No data after 3.5 seconds.\n");
        } else {
            // check for events on s1:
            if (ufds[0].revents & POLLIN) {
                
                
                addr_len = sizeof their_addr;
                if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                                         (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom");
                    exit(1);
                }
                
                if ( stunlib_isStunMsg(buf, numbytes, &isMsSTUN) ){
                    printf("GOT stun packet...\n", buf);
                    StunMessage msg;
                    
                    
                    
                    
                    stunlib_DecodeMessage(buf,
                                          numbytes,
                                          &msg,
                                          NULL,
                                          false,
                                          false);

                    
                            
                    TurnClient_HandleIncResp(TEST_THREAD_CTX,
                                             stunCtx, 
                                             &msg,
                                             buf);
                    
                }
            
            }
        }
    }
    
    close(sockfd);
    
    return 0;
}
