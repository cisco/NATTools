/*
** listener.c -- a datagram sockets "server" demo
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

#include <stdbool.h>

#include <stunlib.h>

#include "utils.h"

#define MYPORT "4950"    // the port users will be connecting to

#define MAXBUFLEN 200

static char password[] = "VOkJxbRl1RmTxUk/WvJxBt";
const char *software_resp= "STUN server\0";


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
    int sockfd;
    struct addrinfo *p;
    int numbytes;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];

    StunMessage stunRequest, stunResponse;
    char response_buffer[256];
    int msg_len;

    sockfd = createSocket(NULL, MYPORT, "stunserver", AI_PASSIVE, &p);

    printf("stunserver: waiting to recvfrom...\n");

    if((numbytes = recvStunMsg(sockfd, &their_addr, &stunRequest, buf)) != -1) {

        if( stunlib_checkIntegrity(buf, numbytes, &stunRequest, password, sizeof(password)) ) {
            
            //Integrity OK, create response

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
                struct sockaddr *a = (struct sockaddr *)&their_addr;
                struct sockaddr_in *b = (struct sockaddr_in *)a;
            
                stunResponse.hasXorMappedAddress = true;
                stunlib_setIP4Address(&stunResponse.xorMappedAddress,
                                      htonl(b->sin_addr.s_addr),
                                      htons(b->sin_port));
            }
            
            if( their_addr.ss_family == AF_INET6 ) {
                struct sockaddr *a = (struct sockaddr *)&their_addr;
                struct sockaddr_in6 *b = (struct sockaddr_in6 *)a;
            
                stunResponse.hasXorMappedAddress = true;
                stunlib_setIP6Address(&stunResponse.xorMappedAddress,
                                      b->sin6_addr.s6_addr,
                                      htons(b->sin6_port));
            }            
                        
            msg_len = stunlib_encodeMessage(&stunResponse,
                                            response_buffer,
                                            256,
                                            (unsigned char*)password,
                                            strlen(password),
                                            false, /*verbose */
                                            false)  /* msice2 */;
            
            

            if ((numbytes = sendto(sockfd, response_buffer, msg_len, 0,
                                   (struct sockaddr *)&their_addr, sizeof their_addr)) == -1) {
                perror("stunclient: sendto");
                exit(1);
            }
    
    
            
        }
    }
    
    close(sockfd);

    return 0;
}




        

