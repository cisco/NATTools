
#include <stdio.h>
#include <stdlib.h>


#include <sockaddr_util.h>

#include "gather.h"

#include "ipaddress_helper.h"

int getRemoteTurnServerIp(struct turn_info *turnInfo, char *fqdn)
{
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];


    

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(fqdn, NULL, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }
    
    for(p = res;p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            if ( ipv4->sin_port == 0 ) {
                sockaddr_initFromIPv4Int((struct sockaddr_in *)&turnInfo->remoteIp4,
                                         ipv4->sin_addr.s_addr,
                                         htons(3478));
            }

            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            if ( ipv6->sin6_port == 0 ) {
                sockaddr_initFromIPv6Int((struct sockaddr_in6 *)&turnInfo->remoteIp6,
                                         ipv6->sin6_addr.s6_addr,
                                         htons(3478));
            }


            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }

    freeaddrinfo(res); // free the linked list

}

int getLocalIPaddresses(struct turn_info *turnInfo, char *iface)
{

    if (!getLocalInterFaceAddrs( (struct sockaddr *)&turnInfo->localIp4, iface, AF_INET) ){
        fprintf(stderr,"Unable to find local IPv4 interface addresses\n");
    }else{
        turnInfo->turnAlloc_44.sockfd = createLocalUDPSocket(AF_INET,
                                                             (struct sockaddr *)&turnInfo->localIp4,
                                                             0);
        turnInfo->turnAlloc_46.sockfd = createLocalUDPSocket(AF_INET,
                                                             (struct sockaddr *)&turnInfo->localIp4,
                                                             0);
    }

    if (!getLocalInterFaceAddrs((struct sockaddr *)&turnInfo->localIp6, iface, AF_INET6) ){
        fprintf(stderr,"Unable to find local IPv6 interface addresses\n");
    }else{
        turnInfo->turnAlloc_64.sockfd = createLocalUDPSocket(AF_INET6,
                                                             (struct sockaddr *)&turnInfo->localIp6,
                                                             0);
        turnInfo->turnAlloc_66.sockfd = createLocalUDPSocket(AF_INET6,
                                                             (struct sockaddr *)&turnInfo->localIp6,
                                                             0);
    }

}


bool getLocalInterFaceAddrs(struct sockaddr *addr, char *iface, int ai_family){
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

int createLocalUDPSocket(int ai_family, struct sockaddr *localIp, uint16_t port){
    int sockfd;

    int rv;
    int yes = 1;
    struct addrinfo hints, *ai, *p;
    char addr[SOCKADDR_MAX_STRLEN];
    char service[8];

    sockaddr_toString(localIp, addr, sizeof addr, false);

    //itoa(port, service, 10);
    
    snprintf(service, 8, "%d", port);


    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = ai_family;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST | AI_ADDRCONFIG;


    if ((rv = getaddrinfo(addr, NULL, &hints, &ai)) != 0) {
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
        //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
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
            struct sockaddr_storage ss;
            socklen_t len = sizeof(ss);
            
            if (getsockname(sockfd, (struct sockaddr *)&ss, &len) == -1){
                perror("getsockname");
            }
            else{
                if (ss.ss_family == AF_INET) { 
                    ((struct sockaddr_in *)p->ai_addr)->sin_port = ((struct sockaddr_in *)&ss)->sin_port;
                }else{
                    ((struct sockaddr_in6 *)p->ai_addr)->sin6_port = ((struct sockaddr_in6 *)&ss)->sin6_port;
                }
            }
            

            sockaddr_copy(localIp, p->ai_addr);
            

            printf("Bound to: '%s'\n",
                   sockaddr_toString(localIp, addr, sizeof(addr), true));

        }

        break;
    }
   
    return sockfd;
}

