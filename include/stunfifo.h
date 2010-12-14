#ifndef STUNFIFO_H
#define STUNFIFO_H


#include <stdint.h>
#include <stdbool.h>

#define STUN_FIFO_SIZE 50
#define STUN_FIFO_MAX_THREAD_CTX  4


typedef struct
{
    char    *passwd;
    int32_t sockPairId;
}
StunFifoElement;

typedef struct
{
    StunFifoElement elem[STUN_FIFO_SIZE];
    uint32_t passLen;
    bool init;
    int  w;
}
StunPassFifo_T;

/* */
void StunFifo_Init(uint32_t threadCtx, uint32_t passLen);

/* */
void StunFifo_InsertLast(uint32_t threadCtx, int32_t sockPairId, char  *passwd);

/* search whole fifo backwards for SockId */
char *StunFifo_FindPasswdBySockPairId(uint32_t threadCtx, int32_t sockPairId);

StunFifoElement *StunFifo_FindElemBySockPairId(uint32_t threadCtx, int32_t sockPairId);


/* */
bool StunFifo_DeleteBySockPairId(uint32_t threadCtx, int32_t sockPairId);


#endif /* STUNFIFO_H */

