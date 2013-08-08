#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <stdbool.h>

#include <stunlib.h>
#include <stunclient.h>

#include "utils.h"

#define MYPORT "4950"    // the port users will be connecting to
#define MAXBUFLEN 500

static const uint32_t TEST_THREAD_CTX = 1;

int sockfd;
void teardown();
void printMalice(StunMessage strunRequest);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    struct addrinfo *servinfo, *p;
    int numbytes;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];

    StunMessage stunRequest, stunResponse;
    STUN_INCOMING_REQ_DATA pReq;
    char response_buffer[256];
    int msg_len;

    char username[] = "evtj:h6vY2";
    char password[] = "VOkJxbRl1RmTxUk/WvJxBt";
    char *software_resp= "STUN server\0";

    signal(SIGINT, teardown);

    sockfd = createSocket(NULL, MYPORT, "stunserver", AI_PASSIVE, servinfo, &p);
    freeaddrinfo(servinfo);

    while(1) {
        printf("stunserver: waiting to recvfrom...\n");
        if((numbytes = recvStunMsg(sockfd, &their_addr, &stunRequest, buf)) != -1) {

            if(stunlib_checkIntegrity(buf, numbytes, &stunRequest, password, sizeof(password)) ) {
                printf("   Integrity OK\n");

                printMalice(stunRequest);

                StunServer_HandleStunIncomingBindReqMsg(TEST_THREAD_CTX,
                                                        &pReq,
                                                        &stunRequest,
                                                        false);

                StunServer_SendConnectivityBindingResp(TEST_THREAD_CTX,
                                                       sockfd,
                                                       stunRequest.msgHdr.id,
                                                       username,
                                                       password,
                                                       &their_addr,
                                                       sizeof(their_addr),
                                                       &their_addr,
                                                       sizeof(their_addr),
                                                       NULL,
                                                       sendRawStun,
                                                       false,
                                                       -1,
                                                       &stunRequest.maliceMetadata);


                printf("Sending response\n\n");
            }
        }
    }
}

void teardown()
{
  close(sockfd);
  printf("Quitting...\n");
  exit(0);
}
