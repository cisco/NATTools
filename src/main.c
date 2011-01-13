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


#include "turnclient.h"

#define SERVERPORT "4950"    // the port users will be connecting to


#define PORT "5793"
#define  TEST_THREAD_CTX 1

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

    

    return sockfd;
}





static int sendRawUDP(int sockfd, const void *buf, size_t len, struct addrinfo * p){
    
    int numbytes;
    printf("Sending Raw\n");
    numbytes = sendto(sockfd, buf, len, 0,
                      p->ai_addr, p->ai_addrlen); 
                      
    
    return numbytes;

}


static int SendRawStun(int sockfd, 
                       uint8_t *buf, 
                       int len, 
                       char *addr, 
                       void *userdata){


    struct addrinfo hints, *servinfo;
    int rv;
    int numSendt;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    printf("Sending to %s\n", addr);

    if ((rv = getaddrinfo("192.168.4.5", SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    numSendt = sendRawUDP(sockfd, buf, len, servinfo);

    freeaddrinfo(servinfo);

    return numSendt;

}


/* Callback for management info  */
static void  PrintTurnInfo(TurnInfoCategory_T category, char *ErrStr)
{
    //fprintf(stderr, "%s\n", ErrStr);
}


static void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData)
{
    printf("Got TURN status callback\n");
}

int main(int argc, char *argv[])
{
    int sockfd;
    int numbytes;
    static TurnCallBackData_T TurnCbData;

    if (argc != 4) {
        fprintf(stderr,"usage: ice  [ip:port] user pass\n");
        exit(1);
    }
    

    sockfd = createLocalUDPSocket(AF_INET);


    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "TestIce");

    TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
                                        NULL,
                                        "192.268.41.2:3478",
                                        argv[2],
                                        argv[3],
                                        sockfd,                       /* socket */
                                        SendRawStun,             /* send func */
                                        NULL,  /* timeout list */
                                        TurnStatusCallBack,
                                        &TurnCbData,
                                        false);


    //numbytes = sendTo(sockfd, argv[2], sizeof(argv[2]), servinfo);
    
    

    printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);
    close(sockfd);

    return 0;
}
