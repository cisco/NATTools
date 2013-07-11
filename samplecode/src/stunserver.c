/*
** listener.c -- a datagram sockets "server" demo
*/

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
                                                       -1);


                printf("Sending response\n\n");
            }
        }
    }
}

void printMalice(StunMessage stunRequest)
{
    if (stunRequest.hasMDAgent)
    {
        printf("\nmdAgent:\n");
        if(stunRequest.mdAgent.hasFlowdataReq)
        {
            printf("    Flowdata:\n");

            printf("        UP:\n");
            printf("            DT: %d LT: %d JT: %d minBW: %d maxBW: %d\n",
                stunRequest.mdAgent.flowdataReq.flowdataUP.DT,
                stunRequest.mdAgent.flowdataReq.flowdataUP.LT,
                stunRequest.mdAgent.flowdataReq.flowdataUP.JT,
                stunRequest.mdAgent.flowdataReq.flowdataUP.minBW,
                stunRequest.mdAgent.flowdataReq.flowdataUP.maxBW);

            printf("        DN:\n");
            printf("            DT: %d LT: %d JT: %d minBW: %d maxBW: %d\n",
                stunRequest.mdAgent.flowdataReq.flowdataDN.DT,
                stunRequest.mdAgent.flowdataReq.flowdataDN.LT,
                stunRequest.mdAgent.flowdataReq.flowdataDN.JT,
                stunRequest.mdAgent.flowdataReq.flowdataDN.minBW,
                stunRequest.mdAgent.flowdataReq.flowdataDN.maxBW);
        }
    }
    if (stunRequest.hasMDRespUP)
    {
        printf("mdRespUP:\n");
        if(stunRequest.mdRespUP.hasFlowdataResp)
        {
            printf("    Flowdata:\n");
            printf("        DT: %d LT: %d JT: %d minBW: %d maxBW: %d\n",
                stunRequest.mdRespUP.flowdataResp.DT,
                stunRequest.mdRespUP.flowdataResp.LT,
                stunRequest.mdRespUP.flowdataResp.JT,
                stunRequest.mdRespUP.flowdataResp.minBW,
                stunRequest.mdRespUP.flowdataResp.maxBW);
        }
    }
    if (stunRequest.hasMDRespDN)
    {
        printf("mdRespDN:\n");
        if(stunRequest.mdRespDN.hasFlowdataResp)
        {
            printf("    Flowdata:\n");
            printf("        DT: %d LT: %d JT: %d minBW: %d maxBW: %d\n",
                stunRequest.mdRespDN.flowdataResp.DT,
                stunRequest.mdRespDN.flowdataResp.LT,
                stunRequest.mdRespDN.flowdataResp.JT,
                stunRequest.mdRespDN.flowdataResp.minBW,
                stunRequest.mdRespDN.flowdataResp.maxBW);
        }
    }
    if (stunRequest.hasMDPeerCheck)
    {
        printf("mdPeerCheck!\n");
    }
}

void teardown()
{
  close(sockfd);
  printf("Quitting...\n");
  exit(0);
}
