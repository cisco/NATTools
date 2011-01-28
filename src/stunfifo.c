#include "stunfifo.h"
#include <string.h>
#include <stdlib.h>

static StunPassFifo_T  StunPassFifo[STUN_FIFO_MAX_THREAD_CTX];


/* */
void StunFifo_Init(uint32_t threadCtx, uint32_t passLen)
{
    int i;
    StunPassFifo[threadCtx].passLen = passLen;
    for (i=0; i < STUN_FIFO_SIZE; i++)
    {
        StunPassFifo[threadCtx].elem[i].sockPairId = -1;
        StunPassFifo[threadCtx].elem[i].passwd = calloc(1, passLen); 
    }
    StunPassFifo[threadCtx].init = true;
}

/* insert last in ring buffer */
void StunFifo_InsertLast(uint32_t threadCtx, int32_t sockPairId, char  *passwd)
{
    /* check if exists */
    StunFifoElement *p =StunFifo_FindElemBySockPairId(threadCtx, sockPairId);
    if (!p)
    {
        /* not found, point to last in ring buffer */
        p = &StunPassFifo[threadCtx].elem[StunPassFifo[threadCtx].w];
        StunPassFifo[threadCtx].w = (StunPassFifo[threadCtx].w + 1) % STUN_FIFO_SIZE;
    }
    /* copy */
    p->sockPairId = sockPairId;
    strncpy(p->passwd, passwd, StunPassFifo[threadCtx].passLen);
}

/* search whole fifo backwards for SockId */
StunFifoElement *StunFifo_FindElemBySockPairId(uint32_t threadCtx, int32_t sockPairId)
{
    int startIx, i;
    startIx = (StunPassFifo[threadCtx].w == 0 ? STUN_FIFO_SIZE-1 : StunPassFifo[threadCtx].w-1);

    for (i = 0; i < STUN_FIFO_SIZE; i++)
    {
        if (StunPassFifo[threadCtx].elem[startIx].sockPairId ==  sockPairId)
            return &StunPassFifo[threadCtx].elem[startIx];
        startIx = (startIx == 0 ? STUN_FIFO_SIZE-1 : startIx-1);
    }
    return NULL;
}

char *StunFifo_FindPasswdBySockPairId(uint32_t threadCtx, int32_t sockPairId)
{
    StunFifoElement *p = StunFifo_FindElemBySockPairId(threadCtx, sockPairId);
    return p ? p->passwd : NULL;
}

