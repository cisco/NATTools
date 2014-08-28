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

#define MYPORT "3478"    // the port users will be connecting to
#define PASSWORD "VOkJxbRl1RmTxUk/WvJxBt"
#define MAXBUFLEN 500

int sockfd;


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
    
    //freeaddrinfo(servinfo);
    
    while(1) {
        printf("stunserver: waiting to recvfrom...\n");
        if((numbytes = recvStunMsg(sockfd, &their_addr, &stunRequest, buf, MAXBUFLEN-1)) != -1) {
            printf("Got bytes: %i\n", numbytes);
            if(stunlib_checkIntegrity(buf, numbytes, &stunRequest, (unsigned char*)PASSWORD, sizeof(PASSWORD)) ) {
                printf("   Integrity OK\n");

                DiscussData discussData;

                discussData.streamType=0x004;
                discussData.interactivity=0x01;
                
                discussData.networkStatus_flags = 0;
                discussData.networkStatus_nodeCnt = 0;
                discussData.networkStatus_tbd = 0;
                discussData.networkStatus_upMaxBandwidth = 0;
                discussData.networkStatus_downMaxBandwidth = 0;

                if (stunRequest.hasNetworkStatus){
                    discussData.networkStatusResp_flags = stunRequest.networkStatus.flags;
                    discussData.networkStatusResp_nodeCnt = stunRequest.networkStatus.nodeCnt;
                    discussData.networkStatusResp_tbd = 0;
                    discussData.networkStatusResp_upMaxBandwidth = stunRequest.networkStatus.upMaxBandwidth;
                    discussData.networkStatusResp_downMaxBandwidth = stunRequest.networkStatus.downMaxBandwidth;
                }


                printDiscuss(stunRequest);



                StunServer_HandleStunIncomingBindReqMsg(clientData,
                                                        &pReq,
                                                        &stunRequest,
                                                        false);

                StunServer_SendConnectivityBindingResp(clientData,
                                                       sockfd,
                                                       stunRequest.msgHdr.id,
                                                       PASSWORD,
                                                       (struct sockaddr *)&their_addr,
                                                       (struct sockaddr *)&their_addr,
                                                       NULL,
                                                       sendRawStun,
                                                       false,
                                                       200,
                                                       &discussData);


                printf("Sending response\n\n");
            }
            else{
                printf("Integrity Failed\n");
            }
        }
    }
}
