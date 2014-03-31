const int MAXBUFLEN = 200;

int recvStunMsg(int sockfd, struct sockaddr_storage *their_addr, StunMessage *stunResponse, unsigned char *buf);
int createSocket(char host[], char port[], int ai_flags, struct addrinfo *servinfo, struct addrinfo **p);
void sendRawStun(int sockHandle,
                  const uint8_t *buf,
                  int bufLen,
                  const struct sockaddr *dstAddr,
                  bool useRelay);
void printDiscuss(StunMessage stunRequest);
