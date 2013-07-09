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

#ifndef STUN_INTERN_H
#define STUN_INTERN_H


#ifdef __cplusplus
extern "C" {
#endif

#define Dflt_TimerResMsec      50

/* Internal STUN signals, inputs to stun bind client. */
typedef enum {
    STUN_SIGNAL_BindReq,
    STUN_SIGNAL_BindResp,
    STUN_SIGNAL_BindRespError,
    STUN_SIGNAL_TimerTick,
    STUN_SIGNAL_TimerRetransmit,
    STUN_SIGNAL_DeAllocate,
    STUN_SIGNAL_Cancel,

    STUN_SIGNAL_Illegal = -1
} STUN_SIGNAL;

/* Internal message formats */          
typedef struct {
    uint32_t                 threadCtx;
    void                    *userCtx;
    struct sockaddr_storage  serverAddr;
    struct sockaddr_storage  baseAddr;
    bool                     useRelay;
    char                     ufrag[300];    //TBD  =  ICE_MAX_UFRAG_LENGTH
    char                     password[300]; // TBD = ICE_MAX_PASSWD_LENGTH
    uint32_t                 peerPriority;
    bool                     useCandidate;
    bool                     iceControlling;
    uint64_t                 tieBreaker;
    StunMsgId                transactionId;
    uint32_t                 sockhandle;
    STUN_SENDFUNC            sendFunc;
    uint32_t                *timeoutList;
    STUNCB                   stunCbFunc;
    StunCallBackData_T      *stunCbData;
    uint32_t                 stunTimeoutList[STUNCLIENT_MAX_RETRANSMITS];
    int                      turnInst;
    MaliceMetadata          *maliceMetadata; // nullptr if no malice attributes should be sent.
} StunBindReqStuct;

typedef struct
{
    struct sockaddr_storage srcAddr;
    StunMessage             stunRespMessage;
} StunRespStruct;

/* Internal STUN  states */
typedef enum {
    STUN_STATE_Idle = 0,
    STUN_STATE_WaitBindResp,
    STUN_STATE_Cancelled,
    STUN_STATE_End  /* must be last */
} STUN_STATE;



/********************************************/
/******  instance data ********  (internal) */
/********************************************/


typedef struct 
{
    STUN_STATE             state;
    bool                   inUse;
    uint32_t               inst;
    StunBindReqStuct       stunBindReq;
    uint32_t               threadCtx;                  /* identified which thread is using this instance */


    uint8_t      stunReqMsgBuf[STUN_MAX_PACKET_SIZE];  /* encoded STUN request    */
    int          stunReqMsgBufLen;                     /* of encoded STUN request */

    STUN_USER_CREDENTIALS userCredentials;
    bool authenticated;

    /* returned in allocate resp */
    char  rflxAddr[IPV6_ADDR_LEN];
    int   rflxPort;

    /* timers */
    int32_t TimerRetransmit;      
    int     retransmits;

    void *userData;

} STUN_INSTANCE_DATA;


/* state function */
typedef void (*STUN_STATE_FUNC)(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload);

/* entry in state table */
typedef struct 
{
    STUN_STATE_FUNC Statefunc;
    const char *StateStr;
}
STUN_STATE_TABLE;



#ifdef __cplusplus
}
#endif


#endif  /* STUN_INTERN_H */
