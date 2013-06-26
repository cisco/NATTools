#include <turnclient.h>
#include <sockaddr_util.h>
#include <sys/socket.h>

#define MAXBUFLEN 200

bool recvStunMsg(int sockfd, struct sockaddr_storage *their_addr, StunMessage *stunResponse, unsigned char *buf)
{
    socklen_t addr_len = sizeof their_addr;
    int numbytes;
    bool isMsStun;

    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)their_addr, &addr_len)) == -1) {
          perror("recvfrom");
          exit(1);
    }

    printf("stunclient: Got a packet that is %d bytes long\n", numbytes);
    if (stunlib_isStunMsg(buf, numbytes, &isMsStun)) {
        printf("   Packet is STUN\n");

        stunlib_DecodeMessage(buf, numbytes, stunResponse, NULL, false, false);

        return true;
    }

    return false;
}
