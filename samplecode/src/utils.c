#include <turnclient.h>
#include <sockaddr_util.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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


void sendRawStun(int sockHandle,
                uint8_t *buf,
                int bufLen,
                struct sockaddr *dstAddr,
                bool useRelay)
{
    int numbytes;
    char addrStr[SOCKADDR_MAX_STRLEN];

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


void printMalice(StunMessage stunRequest)
{
    if (stunRequest.maliceMetadata.hasMDAgent)
    {
        printf("\nmdAgent:\n");
        if(stunRequest.maliceMetadata.mdAgent.hasFlowdataReq)
        {
            printf("    Flowdata:\n");

            printf("        UP:\n");
            printf("            DT: %d LT: %d JT: %d minBW: %d maxBW: %d\n",
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataUP.DT,
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataUP.LT,
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataUP.JT,
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataUP.minBW,
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataUP.maxBW);

            printf("        DN:\n");
            printf("            DT: %d LT: %d JT: %d minBW: %d maxBW: %d\n",
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataDN.DT,
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataDN.LT,
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataDN.JT,
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataDN.minBW,
                stunRequest.maliceMetadata.mdAgent.flowdataReq.flowdataDN.maxBW);
        }
    }
    if (stunRequest.maliceMetadata.hasMDRespUP)
    {
        printf("mdRespUP:\n");
        if(stunRequest.maliceMetadata.mdRespUP.hasFlowdataResp)
        {
            printf("    Flowdata:\n");
            printf("        DT: %d LT: %d JT: %d minBW: %d maxBW: %d\n",
                stunRequest.maliceMetadata.mdRespUP.flowdataResp.DT,
                stunRequest.maliceMetadata.mdRespUP.flowdataResp.LT,
                stunRequest.maliceMetadata.mdRespUP.flowdataResp.JT,
                stunRequest.maliceMetadata.mdRespUP.flowdataResp.minBW,
                stunRequest.maliceMetadata.mdRespUP.flowdataResp.maxBW);
        }
    }
    if (stunRequest.maliceMetadata.hasMDRespDN)
    {
        printf("mdRespDN:\n");
        if(stunRequest.maliceMetadata.mdRespDN.hasFlowdataResp)
        {
            printf("    Flowdata:\n");
            printf("        DT: %d LT: %d JT: %d minBW: %d maxBW: %d\n",
                stunRequest.maliceMetadata.mdRespDN.flowdataResp.DT,
                stunRequest.maliceMetadata.mdRespDN.flowdataResp.LT,
                stunRequest.maliceMetadata.mdRespDN.flowdataResp.JT,
                stunRequest.maliceMetadata.mdRespDN.flowdataResp.minBW,
                stunRequest.maliceMetadata.mdRespDN.flowdataResp.maxBW);
        }
    }
    if (stunRequest.maliceMetadata.hasMDPeerCheck)
    {
        printf("mdPeerCheck!\n");
    }
}

