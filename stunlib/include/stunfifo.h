/*
Copyright 2011 Cisco. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY CISCO ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Cisco.
*/

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
void StunFifo_destruct (uint32_t threadCtx);

/* */
void StunFifo_InsertLast(uint32_t threadCtx, int32_t sockPairId, char  *passwd);

/* search whole fifo backwards for SockId */
char *StunFifo_FindPasswdBySockPairId(uint32_t threadCtx, int32_t sockPairId);

StunFifoElement *StunFifo_FindElemBySockPairId(uint32_t threadCtx, int32_t sockPairId);


/* */
bool StunFifo_DeleteBySockPairId(uint32_t threadCtx, int32_t sockPairId);


#endif /* STUNFIFO_H */

