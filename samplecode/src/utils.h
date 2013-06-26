const int MAXBUFLEN = 200;

int recvStunMsg(int sockfd, struct sockaddr_storage *their_addr, StunMessage *stunResponse, unsigned char *buf);
