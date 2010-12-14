/***************************************************************************
 *                   STUN CLIENT BIND  
 *--------------------------------------------------------------------------
 *                  (C) Copyright Tandberg Telecom AS 2008
 *==========================================================================
 * Version       : 1.0
 * Status        : Draft
 *
 * Filetype      : Code File 
 *
 * Author        : Paul Mackell (PTM), Tandberg Telecom AS.
 *
 * Compiler      : ANSI C
 * Portability   : 
 *
 * Switches      : 
 *
 * Import        : ref included files
 * Export        : 
                   
 *                 
 *
 * Description   :
 *
 ******************************************************************************/


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
    uint32_t           threadCtx;
    void               *userCtx;
    char               serverAddr[IPV4_ADDR_LEN_WITH_PORT];
    char               baseAddr[IPV4_ADDR_LEN_WITH_PORT];
    bool               useRelay;
    char               ufrag[300];    //TBD  =  ICE_MAX_UFRAG_LENGTH
    char               password[300]; // TBD = ICE_MAX_PASSWD_LENGTH
    uint32_t           peerPriority;
    bool               useCandidate;
    bool               iceControlling;
    uint64_t           tieBreaker;
    StunMsgId          transactionId;
    uint32_t           sockhandle;
    STUN_SENDFUNC      sendFunc;
    uint32_t           *timeoutList;
    STUNCB             stunCbFunc;
    StunCallBackData_T *stunCbData;
    uint32_t           stunTimeoutList[STUNCLIENT_MAX_RETRANSMITS];
    int                turnInst;
} StunBindReqStuct;


typedef struct
{
    char               srcAddr[IPV4_ADDR_LEN_WITH_PORT];
    StunMessage        stunRespMessage;
}
StunRespStruct;

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
    char  rflxAddr[IPV4_ADDR_LEN];
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
