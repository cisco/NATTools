/*
** talker.c -- a datagram "client" demo
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

#include <stunlib.h>

#include "utils.h"

#define SERVERPORT "4950"    // the port users will be connecting to

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo *p;
    int numbytes;

    StunMessage stunRequest, stunResponse;
    char buffer[256];
    int msg_len;

    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];

    const char username[] = "evtj:h6vY";
    const char password[] = "VOkJxbRl1RmTxUk/WvJxBt";
    const uint32_t priority = 1845494271;
    const uint64_t tieBreaker = 0x932FF9B151263B36LL;
    const char *software= "STUN test client\0";
    const unsigned char idOctet[] = "\xb7\xe7\xa7\x01"
        "\xbc\x34\xd6\x86"
        "\xfa\x87\xdf\xae";




    if (argc != 2) {
        fprintf(stderr,"usage: stunclient hostname\n");
        exit(1);
    }

    sockfd = createSocket(argv[1], SERVERPORT, "stunclient", 0, &p);

    //Create STUN message and send it..

    memset(&stunRequest, 0, sizeof(StunMessage));
    stunRequest.msgHdr.msgType = STUN_MSG_BindRequestMsg;
    memcpy(&stunRequest.msgHdr.id.octet,&idOctet,12);

    stunlib_addUserName(&stunRequest, username, '\x20');
    stunlib_addSoftware(&stunRequest, software, '\x20');
    stunRequest.hasPriority = true;
    stunRequest.priority.value = priority;
    stunRequest.hasControlled = true;
    stunRequest.controlled.value = tieBreaker;

    msg_len = stunlib_encodeMessage(&stunRequest,
                                    buffer,
                                    256,
                                    (unsigned char*)password,
                                    strlen(password),
                                    false, /*verbose */
                                    false)  /* msice2 */;

    if ((numbytes = sendto(sockfd, buffer, msg_len, 0,
                       p->ai_addr, p->ai_addrlen)) == -1) {
        perror("stunclient: sendto");
        exit(1);
    }


    printf("stunclient: sent %d bytes to %s\n", numbytes, argv[1]);

    if((numbytes = recvStunMsg(sockfd, &their_addr, &stunResponse, buf)) != -1) {
        if( stunlib_checkIntegrity(buf,
                                   numbytes,
                                   &stunResponse,
                                   password,
                                   sizeof(password)) ) {
            printf("Integrity Check OK\n");

            printf("Could print some attributes here, but check wireshark instead..\n");

        }
    }

    close(sockfd);

    return 0;
}

