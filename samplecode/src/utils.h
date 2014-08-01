
#ifndef UTILS_H
#define UTILS_H

#include <turnclient.h>
#include <sockaddr_util.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>


int recvStunMsg(int sockfd, struct sockaddr_storage *their_addr, StunMessage *stunResponse, unsigned char *buf, int buflen);
int createSocket(char host[], char port[], int ai_flags, struct addrinfo *servinfo, struct addrinfo **p);
void sendRawStun(int sockHandle,
                  const uint8_t *buf,
                  int bufLen,
                  const struct sockaddr *dstAddr,
                  bool useRelay);
void printDiscuss(StunMessage stunRequest);

#endif
