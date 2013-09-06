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

#define MYPORT "4950"    // the port users will be connecting to
#define PASSWORD "VOkJxbRl1RmTxUk/WvJxBt"
#define MAXBUFLEN 500

int sockfd;

void sendRawStun(int sockHandle,
                uint8_t *buf,
                int bufLen,
                struct sockaddr *dstAddr,
                bool useRelay);

void teardown()
{
  close(sockfd);
  printf("Quitting...\n");
  exit(0);
}

int main(void)
{
    struct addrinfo *servinfo, *p;
    int numbytes;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];

    StunMessage stunRequest;
    STUN_INCOMING_REQ_DATA pReq;

    STUN_CLIENT_DATA *clientData;

    StunClient_Alloc(&clientData);

    signal(SIGINT, teardown);

    sockfd = createSocket(NULL, MYPORT, AI_PASSIVE, servinfo, &p);
    freeaddrinfo(servinfo);

    while(1) {
        printf("stunserver: waiting to recvfrom...\n");
        if((numbytes = recvStunMsg(sockfd, &their_addr, &stunRequest, buf)) != -1) {

            if(stunlib_checkIntegrity(buf, numbytes, &stunRequest, PASSWORD, sizeof(PASSWORD)) ) {
                printf("   Integrity OK\n");

                printMalice(stunRequest);

                StunServer_HandleStunIncomingBindReqMsg(clientData,
                                                        &pReq,
                                                        &stunRequest,
                                                        false);

                StunServer_SendConnectivityBindingResp(clientData,
                                                       sockfd,
                                                       stunRequest.msgHdr.id,
                                                       PASSWORD,
                                                       &their_addr,
                                                       &their_addr,
                                                       NULL,
                                                       sendRawStun,
                                                       false,
                                                       200,
                                                       &stunRequest.maliceMetadata);


                printf("Sending response\n\n");
            }
        }
    }
}
