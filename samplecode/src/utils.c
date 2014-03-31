#include <turnclient.h>
#include <sockaddr_util.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>

#define MAXBUFLEN 500 


int createSocket(char host[], char port[], int ai_flags, struct addrinfo *servinfo, struct addrinfo **p)
{
    struct addrinfo hints;
    int rv, sockfd;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = ai_flags; // use my IP if not 0

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for((*p) = servinfo; (*p) != NULL; (*p) = (*p)->ai_next) {
        if ((sockfd = socket((*p)->ai_family, (*p)->ai_socktype,
                (*p)->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }

        if (ai_flags != 0 && bind(sockfd, (*p)->ai_addr, (*p)->ai_addrlen) == -1) {
            close(sockfd);
            perror("bind");
            continue;
        }

        break;
    }

    if ((*p) == NULL) {
        fprintf(stderr, "failed to bind socket\n");
        return -2;
    }

    return sockfd;
}


int sendRawStun(int sockHandle,
                uint8_t *buf,
                int bufLen,
                struct sockaddr *dstAddr,
                bool useRelay)
{
    int tos = 0x28;
    int numbytes;
    char addrStr[SOCKADDR_MAX_STRLEN];


    setsockopt(sockHandle, IPPROTO_IP, IP_TOS,  &tos, sizeof(tos));

    if ((numbytes = sendto(sockHandle, buf, bufLen, 0, dstAddr, sizeof(*dstAddr))) == -1) {
        perror("sendto");
        exit(1);
    }

    sockaddr_toString(dstAddr, addrStr, SOCKADDR_MAX_STRLEN, true);
    printf("Sending Raw (To: '%s'(%i), Bytes:%i/%i  )\n", addrStr, sockHandle, numbytes, bufLen);

    return numbytes;
}

int recvStunMsg(int sockfd, struct sockaddr_storage *their_addr, StunMessage *stunResponse, unsigned char *buf)
{
    socklen_t addr_len = sizeof their_addr;
    int numbytes;

    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)their_addr, &addr_len)) == -1) {
          perror("recvfrom");
          exit(1);
    }

    printf("Got a packet that is %d bytes long\n", numbytes);
    if (stunlib_isStunMsg(buf, numbytes)) {
        printf("   Packet is STUN\n");

        stunlib_DecodeMessage(buf, numbytes, stunResponse, NULL, NULL);

        return numbytes;
    }

    return -1;
}




void printDiscuss(StunMessage stunRequest)
{
    if (stunRequest.hasStreamType)
    {
        printf("  StreamType \n");
        printf("    Type: %#06x  \n", stunRequest.streamType.type);
        printf("    Interactivity: %#04x  \n", stunRequest.streamType.interactivity);
        
    }
    
    if (stunRequest.hasNetworkStatus){
        printf("  NetworkStatus \n");
        printf("  Flags: %#04x  \n", stunRequest.networkStatus.flags);
        printf("  NodeCnt: %i  \n", stunRequest.networkStatus.nodeCnt);
        printf("  UpMax: %i  \n", stunRequest.networkStatus.upMaxBandwidth);
        printf("  DownMax: %i  \n", stunRequest.networkStatus.downMaxBandwidth);
    }
}

