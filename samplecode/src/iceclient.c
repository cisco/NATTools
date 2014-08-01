#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>


#include <sockaddr_util.h>
#include "utils.h"
#include "turnhelper.h"


#define PORT "9034" // the port client will be connecting to 

#define MAXDATASIZE 300

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void *proxy_handler(void *socketfd)
{
    //Get the socket descriptor
    int sock = *(int*)socketfd;
    int read_size;
    char *message , client_message[2000];

        //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {
        printf("got: %s\n", client_message);
        

    }
              
    return 0;
}


int connectProxy(const char * host){
    int sockfd, numbytes, *new_sock;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            s, sizeof s);
    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure
    
    
    pthread_t proxy_listen_thread;
    new_sock = malloc(1);
    *new_sock = sockfd;
    
    if( pthread_create( &proxy_listen_thread , NULL ,  proxy_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
    return sockfd;

}

int closeProxy(int sockfd)
{
    close(sockfd);
    
    return sockfd;
}

int sendOffer(int sockfd, char *offer, int offerLen )
{
    //sendsomething
    if (send(sockfd, offer, offerLen, 0) == -1)
        perror("send");
    
    return 0;
}



int createOffer(char *offer, struct turnData *tData){
    char addr[SOCKADDR_MAX_STRLEN];

    strcpy(offer, "INVITE\n");
    strcat(offer, "a=candidate UDP srflx ");    
    strcat(offer,
           sockaddr_toString((struct sockaddr *)&tData->rflx,
                             addr,
                             sizeof(addr),
                             true));
    strcat(offer, "\n");
    
    strcat(offer, "a=candidate UDP relay ");    
    strcat(offer,
           sockaddr_toString((struct sockaddr *)&tData->relay,
                             addr,
                             sizeof(addr),
                             true));
    strcat(offer, "\n");

    strcat(offer, "\0");
    

    return strlen(offer);

}



int main(int argc, char *argv[])
{
    struct timespec timer;
    struct timespec remaining;

    int proxy_fd, media_fd;
    char c;
    char offer[500];
    int offerLen;
    struct turnData tData;


    if (argc != 5) {
        fprintf(stderr,"usage: iceclient registrar turnserver user pass\n");
        exit(1);
    }

    proxy_fd = connectProxy(argv[1]);

    tData.turn_uri = argv[2];
    tData.turn_port = "3478";
    tData.user = argv[3];
    tData.password = argv[4];

        
    while(1){
        printf( "Press c (enter) to call (Or wait for a incomming call) :");
        c = getchar( );

        if(c == 'c'){
            printf("Calling...");
            gatherCandidates(&tData);
            while(tData.turn_state!=SUCCESS){
                timer.tv_sec = 0;
                timer.tv_nsec = 50000000;
                nanosleep(&timer, &remaining);
            }

            printf("Got it...");
            offerLen = createOffer(offer, &tData);

            sendOffer(proxy_fd, offer, offerLen);
        }
        if(c == 'e'){
            closeProxy(proxy_fd);
            exit(0);
        }

    }
}
