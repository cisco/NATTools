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
#include <ifaddrs.h>



#include "turnclient.h"
#include "sockaddr_util.h"



#define PORT "5799"



static bool getLocalInterFaceAddrs(struct sockaddr *addr, char *iface, int ai_family){
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        if (sockaddr_isAddrLoopBack(ifa->ifa_addr))
            continue;

        if( strcmp(iface, ifa->ifa_name)!=0 )
            continue;

        if (ai_family == AF_INET6 ){
            if (sockaddr_isAddrLinkLocal(ifa->ifa_addr))
                continue;
        }


        family = ifa->ifa_addr->sa_family;

        if (family == ai_family) {

            s = getnameinfo(ifa->ifa_addr,
                            (family == AF_INET) ? sizeof(struct sockaddr_in) :
                            sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
        }
    }
    freeifaddrs(ifaddr);

    if (sockaddr_initFromString(addr, host))
        return true;

    return false;
}

static int createLocalUDPSocket(int ai_family, struct sockaddr * localIp){
    int sockfd;

    int rv;
    int yes = 1;
    struct addrinfo hints, *ai, *p;
    char addr[SOCKADDR_MAX_STRLEN];


    sockaddr_toString(localIp, addr, sizeof addr, false);


    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST | AI_ADDRCONFIG;


    if ((rv = getaddrinfo(addr, PORT, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s ('%s')\n", gai_strerror(rv), addr);
        exit(1);
    }



    for (p = ai; p != NULL; p = p->ai_next) {

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd < 0) {
            printf("Unable to open socket\n");
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (sockaddr_isAddrAny(p->ai_addr) ){
            printf("Ignoring any\n");
            continue;

        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            printf("Bind failed\n");
            close(sockfd);
            continue;
        }

        if (localIp != NULL){
            sockaddr_copy(localIp, p->ai_addr);

            printf("Bound to: '%s'\n",
                   sockaddr_toString(localIp, addr, sizeof(addr), true));

        }

        break;
    }
    //printf("Soket open: %i\n", sockfd);
    return sockfd;
}





int main(int argc, char *argv[])
{
    int sockfd_4, sockfd_6, sockfd;

    
    struct sockaddr_storage ss_addr;
    struct sockaddr_storage localIp4, localIp6;
    

    


    


    if (argc != 5) {
        fprintf(stderr,"usage: testice  iface [ip:port] user pass\n");
        exit(1);
    }


    if (!getLocalInterFaceAddrs((struct sockaddr *)&localIp4, argv[1], AF_INET) ){
        fprintf(stderr,"Unable to find local IPv4 interface addresses\n");
    }else{
        sockfd_4 = createLocalUDPSocket(AF_INET, (struct sockaddr *)&localIp4);
    }

    if (!getLocalInterFaceAddrs((struct sockaddr *)&localIp6, argv[1], AF_INET6) ){
        fprintf(stderr,"Unable to find local IPv6 interface addresses\n");
    }else{
        sockfd_6 = createLocalUDPSocket(AF_INET6, (struct sockaddr *)&localIp6);
    }

    sockaddr_initFromString((struct sockaddr *)&ss_addr,
                            argv[2]);
    
    
    if(ss_addr.ss_family == AF_INET){
        printf("Using IPv4 Socket\n");
        //make sure the port is set
        if (((struct sockaddr_in *)&ss_addr)->sin_port == 0 ) {
            ((struct sockaddr_in *)&ss_addr)->sin_port = htons(3478);
        }
        sockfd = sockfd_4;

    }else if(ss_addr.ss_family == AF_INET6){
        printf("Using IPv6 Socket\n");
        //make sure the port is set
        if (((struct sockaddr_in6 *)&ss_addr)->sin6_port == 0 ) {
            ((struct sockaddr_in6 *)&ss_addr)->sin6_port = htons(3478);
        }
        sockfd = sockfd_6;
    }

    gather((struct sockaddr *)&ss_addr, sockfd, argv[3], argv[4]);


    while(1)
        sleep(2);

    close(sockfd);

    return 0;
}
