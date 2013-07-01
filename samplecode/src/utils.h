const int MAXBUFLEN = 200;

int recvStunMsg(int sockfd, struct sockaddr_storage *their_addr, StunMessage *stunResponse, unsigned char *buf);
int createSocket(char server[], char serverport [], char outprintName[], bool bind, struct addrinfo *servinfo, struct addrinfo **p);
int sendRawStun(int sockfd, uint8_t *buf, int len, struct sockaddr *addr, socklen_t t, void *userdata);
