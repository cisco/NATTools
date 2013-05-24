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
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    unsigned char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    bool isMsStun;

    StunMessage stunRequest, stunResponse;
    char response_buffer[256];
    int msg_len;
    



    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("stunserver: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("stunserver: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "stunserver: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);

    printf("stunserver: waiting to recvfrom...\n");

    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
        (struct sockaddr *)&their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    printf("stunserver: got packet from %s\n",
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s));
    printf("stunserver: packet is %d bytes long\n", numbytes);
    
    if(stunlib_isStunMsg(buf, numbytes, &isMsStun)){
        printf("Packet is STUN\n");
        
        stunlib_DecodeMessage( buf, 
                               numbytes, 
                               &stunRequest, 
                               NULL, 
                               false, 
                               false);

        if( stunlib_checkIntegrity(buf,
                                   numbytes,
                                   &stunRequest,
                                   password,
                                   sizeof(password)) ) {
            
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
                                   (struct sockaddr *)&their_addr, addr_len)) == -1) {
                perror("stunclient: sendto");
                exit(1);
            }
    
    
            
        }
    }
    
    close(sockfd);

    return 0;
}




        

