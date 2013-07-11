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

#include "utils.h"

#define MYPORT "4951"    // the port users will be connecting to

#define MAXBUFLEN 500

static char password[] = "VOkJxbRl1RmTxUk/WvJxBt";
const char *software_resp= "STUN server\0";
int sockfd;
void teardown();

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
    char response_buffer[256];
    int msg_len;

    signal(SIGINT, teardown);

    sockfd = createSocket(NULL, MYPORT, "stunserver", AI_PASSIVE, servinfo, &p);
    freeaddrinfo(servinfo);

    while(1) {
        printf("stunserver: waiting to recvfrom...\n");
        if((numbytes = recvStunMsg(sockfd, &their_addr, &stunRequest, buf)) != -1) {

            if(stunlib_checkIntegrity(buf, numbytes, &stunRequest, password, sizeof(password)) ) {
                printf("   Integrity OK\n");

                if (stunRequest.hasMDAgent)
                {
                    printf("mdAgent:\n");
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

                memset(&stunResponse, 0, sizeof(StunMessage));
                /*Header*/
                stunResponse.msgHdr.msgType = STUN_MSG_BindResponseMsg;
                /*TransactionID from request*/
                memcpy(&stunResponse.msgHdr.id.octet, &stunRequest.msgHdr.id.octet ,12);
                /*Server*/
                stunResponse.hasSoftware = true;
                memcpy( stunResponse.software.value, software_resp, strlen(software_resp));
                stunResponse.software.sizeValue = strlen(software_resp);

                if( their_addr.ss_family == AF_INET ) {
                    printf("\nRequest was IPv4\n");
                    struct sockaddr *a = (struct sockaddr *)&their_addr;
                    struct sockaddr_in *b = (struct sockaddr_in *)a;

                    stunResponse.hasXorMappedAddress = true;
                    stunlib_setIP4Address(&stunResponse.xorMappedAddress,
                                          htonl(b->sin_addr.s_addr),
                                          htons(b->sin_port));
                }

                if( their_addr.ss_family == AF_INET6 ) {
                    printf("Request was IPv6\n");
                    struct sockaddr *a = (struct sockaddr *)&their_addr;
                    struct sockaddr_in6 *b = (struct sockaddr_in6 *)a;

                    stunResponse.hasXorMappedAddress = true;
                    stunlib_setIP6Address(&stunResponse.xorMappedAddress,
                                          b->sin6_addr.s6_addr,
                                          htons(b->sin6_port));
                }

                printf("Encoding response\n");
                msg_len = stunlib_encodeMessage(&stunResponse,
                                                response_buffer,
                                                256,
                                                (unsigned char*)password,
                                                strlen(password),
                                                false, /*verbose */
                                                false)  /* msice2 */;


                printf("Sending response\n\n");
                if ((numbytes = sendto(sockfd, response_buffer, msg_len, 0,
                                       (struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
                    perror("stunclient: sendto");
                    exit(1);
                }
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
