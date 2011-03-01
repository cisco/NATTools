#ifndef GATHER_H
#define GATHER_H

#include <stdint.h>

static const uint32_t TEST_THREAD_CTX = 1;


int gather(struct sockaddr *host_addr, int sockfd, int requestedFamily, char *user, char *pass);



#endif
