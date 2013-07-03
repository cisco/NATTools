#include <turnclient.h>
#include <sockaddr_util.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAXBUFLEN 200 
int recvStunMsg(int sockfd, struct sockaddr_storage *their_addr, StunMessage *stunResponse, unsigned char *buf)
{
    socklen_t addr_len = sizeof their_addr;
    int numbytes;
    bool isMsStun;

    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)their_addr, &addr_len)) == -1) {
          perror("recvfrom");
          exit(1);
    }

    printf("Got a packet that is %d bytes long\n", numbytes);
    if (stunlib_isStunMsg(buf, numbytes, &isMsStun)) {
        printf("   Packet is STUN\n");

        stunlib_DecodeMessage(buf, numbytes, stunResponse, NULL, false, false);

        return numbytes;
    }

    return -1;
}

int createSocket(char host[], char port[], char outprintName[], int ai_flags, struct addrinfo *servinfo, struct addrinfo **p)
{
    struct addrinfo hints;
    int rv, sockfd;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
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
            perror("createSocket: socket");
            continue;
        }

        if (ai_flags != 0 && bind(sockfd, (*p)->ai_addr, (*p)->ai_addrlen) == -1) {
            close(sockfd);
            perror("createSocket: bind");
            continue;
        }

        break;
    }

    if ((*p) == NULL) {
        fprintf(stderr, "%s: failed to bind socket\n", outprintName);
        return -2;
    } else {
        fprintf(stderr, "%s: created socket\n", outprintName);
    }

    return sockfd;
}

int sendRawUDP(int sockfd,
               const void *buf,
               size_t len,
               struct sockaddr * p,
               socklen_t t)
{
    int numbytes;
    char addr[256];
    int rv;

    numbytes = sendto(sockfd, buf, len, 0, p, t);

    sockaddr_toString(p, addr, 256, true);
    printf("Sending Raw (To: '%s'(%i), Bytes:%i/%i  )\n", addr, sockfd, numbytes, (int)len);

    return numbytes;
}

int sendRawStun(int sockfd,
                uint8_t *buf,
                int len,
                struct sockaddr *addr,
                socklen_t t,
                void *userdata)
{
    return sendRawUDP(sockfd, buf, len, addr, t);
}
