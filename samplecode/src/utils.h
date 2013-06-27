int recvStunMsg(int sockfd, struct sockaddr_storage *their_addr, StunMessage *stunResponse, unsigned char *buf);
int createSocket(char server[], char serverport [], char outprintName[], bool bind, struct addrinfo *p);
