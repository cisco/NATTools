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
#include "gather.h"



struct listenConfig{
    int stunCtx;
    int sockfd;
    char* user;
    char* pass;


};



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

static int createLocalUDPSocket(int ai_family, struct sockaddr * localIp, uint16_t port){
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


    if ((rv = getaddrinfo(addr, service, &hints, &ai)) != 0) {
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


/* Callback for management info  */
static void  PrintTurnInfo(TurnInfoCategory_T category, char *ErrStr)
{
    //fprintf(stderr, "%s\n", ErrStr);
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




#define MAXBUFLEN 1024

void stunListen(struct listenConfig config[], int len){
    struct pollfd ufds[10];
    //struct listenConfig *config = (struct listenConfig *)ptr;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    int rv;
    int numbytes;
    bool isMsSTUN;
    int i;


    for (i=0;i<len;i++){
        printf("Listen: %i\n", config[i].sockfd);

        ufds[i].fd = config[i].sockfd;
        ufds[i].events = POLLIN | POLLPRI; // check for normal or out-of-band
    }

    addr_len = sizeof their_addr;

    while(1){

        rv = poll(ufds, len, -1);

        //printf("rv:%i\n", rv);
        if (rv == -1) {
            perror("poll"); // error occurred in poll()
        } else if (rv == 0) {
            printf("Timeout occurred! (Should not happen)\n");
        } else {
            // check for events on s1:

            for(i=0;i<len;i++){
                if (ufds[i].revents & POLLIN) {

                    if ((numbytes = recvfrom(config[i].sockfd, buf, MAXBUFLEN-1 , 0,
                                             (struct sockaddr *)&their_addr, &addr_len)) == -1) {
                    perror("recvfrom");
                    exit(1);
                    }

                    printf("Event on : %i, bytes: %i\n", i, numbytes);
                    if ( stunlib_isStunMsg(buf, numbytes, &isMsSTUN) ){
                        StunMessage msg;


                        stunlib_DecodeMessage(buf,
                                              numbytes,
                                              &msg,
                                              NULL,
                                              false,
                                              false);



                        TurnClient_HandleIncResp(TEST_THREAD_CTX,
                                                 config[i].stunCtx,
                                                 &msg,
                                                 buf);
                    }
                }
            }
        }
    }
}


enum turn_ip_type{
    turn_44 = 1,
    turn_46 = 2,
    turn_64 = 4,
    turn_66 = 8,
};

#define FQDN_MAX_LEN 1024

struct turn_info{
    //Fully Qualified Domain Name
    char fqdn[FQDN_MAX_LEN];

    //Actual TURN server IP adresses
    //Might not be what FQDN resolved to
    //because of the alternate server mechanism
    struct sockaddr_storage remoteIp4;
    struct sockaddr_storage remoteIp6;


    //Local IP adresses
    struct sockaddr_storage localIp4;
    struct sockaddr_storage localIp6;


    //socket descriptors
    int sockfd_44;
    int sockfd_46;
    int sockfd_64;
    int sockfd_66;

};



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

    printf("IP addresses for %s:\n\n", fqdn);

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
        turnInfo->sockfd_44 = createLocalUDPSocket(AF_INET,
                                                   (struct sockaddr *)&turnInfo->localIp4,
                                                   53000);
        turnInfo->sockfd_46 = createLocalUDPSocket(AF_INET,
                                                   (struct sockaddr *)&turnInfo->localIp4,
                                                   53001);
    }

    if (!getLocalInterFaceAddrs((struct sockaddr *)&turnInfo->localIp6, iface, AF_INET6) ){
        fprintf(stderr,"Unable to find local IPv6 interface addresses\n");
    }else{
        turnInfo->sockfd_64 = createLocalUDPSocket(AF_INET6,
                                                   (struct sockaddr *)&turnInfo->localIp6,
                                                   54000);
        turnInfo->sockfd_66 = createLocalUDPSocket(AF_INET6,
                                                   (struct sockaddr *)&turnInfo->localIp6,
                                                   54001);
    }

}

int main(int argc, char *argv[])
{
    int sockfd;
    int stunCtx;

    struct turn_info turnInfo;

    pthread_t turnTickThread;

    struct listenConfig listenConfig[10];

    bool useIPv6 = false;


    if (argc != 5) {
        fprintf(stderr,"usage: testice  iface [ip:port] user pass\n");
        exit(1);
    }

    getRemoteTurnServerIp(&turnInfo, argv[2]);

    getLocalIPaddresses(&turnInfo, argv[1]);



    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "TestIce");
    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);


    stunCtx = gather((struct sockaddr *)&turnInfo.remoteIp4, turnInfo.sockfd_44, AF_INET, argv[3], argv[4]);
    printf("Stunctx: %i\n", stunCtx);
    listenConfig[0].stunCtx = stunCtx;
    listenConfig[0].sockfd = turnInfo.sockfd_44;
    listenConfig[0].user = argv[3];
    listenConfig[0].pass = argv[4];


    stunCtx= gather((struct sockaddr *)&turnInfo.remoteIp4, turnInfo.sockfd_46, AF_INET6, argv[3], argv[4]);
    printf("Stunctx: %i\n", stunCtx);
    listenConfig[1].stunCtx = stunCtx;
    listenConfig[1].sockfd = turnInfo.sockfd_46;
    listenConfig[1].user = argv[3];
    listenConfig[1].pass = argv[4];

    stunCtx = gather((struct sockaddr *)&turnInfo.remoteIp6, turnInfo.sockfd_64, AF_INET, argv[3], argv[4]);
    printf("Stunctx: %i\n", stunCtx);
    listenConfig[2].stunCtx = stunCtx;
    listenConfig[2].sockfd = turnInfo.sockfd_64;
    listenConfig[2].user = argv[3];
    listenConfig[2].pass = argv[4];

    stunCtx= gather((struct sockaddr *)&turnInfo.remoteIp6, turnInfo.sockfd_66, AF_INET6, argv[3], argv[4]);
    printf("Stunctx: %i\n", stunCtx);
    listenConfig[3].stunCtx = stunCtx;
    listenConfig[3].sockfd = turnInfo.sockfd_66;
    listenConfig[3].user = argv[3];
    listenConfig[3].pass = argv[4];


    stunListen(listenConfig, 4);


    close(sockfd);

    return 0;
}
