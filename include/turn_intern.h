/***************************************************************************
 *                   T U R N  C L I E N T  internals 
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
 * Description   : Internals, used by turnclient only, not part of the API
 *
 ******************************************************************************/


#ifndef TURN_INTERN_H
#define TURN_INTERN_H

//#ifndef  __WINDOWS__
//#include "ttos_semaphore.h"
//#endif

#include "stun_os.h"


#ifdef __cplusplus
extern "C" {
#endif

#define Dflt_TimerResMsec      50
#define NoStunErrorCode  0xffffffff

/* make sure both the following timers are < 5 minutes */
#define TURN_REFRESH_CHANNEL_TIMER_SEC     (3*60)  /* 3 min (spec. is 10 min) */
#define TURN_REFRESH_PERMISSION_TIMER_SEC  (4*60)  /* 4 min (spec. is 5 min)  */

#define TURN_RETRANS_TIMEOUT_RELEASE_MSEC 150
#define TURN_RETRIES_RELEASE                2


/* Internal TURN signals, inputs to turn client. */
typedef enum {
    TURN_SIGNAL_AllocateReq,
    TURN_SIGNAL_AllocateResp,
    TURN_SIGNAL_AllocateRespError,
    TURN_SIGNAL_CreatePermissionReq,      
    TURN_SIGNAL_CreatePermissionResp,      
    TURN_SIGNAL_CreatePermissionRespError, 
    TURN_SIGNAL_ChannelBindReq,
    TURN_SIGNAL_ChannelBindResp,
    TURN_SIGNAL_ChannelBindRespError,
    TURN_SIGNAL_RefreshResp,
    TURN_SIGNAL_RefreshRespError,
    TURN_SIGNAL_SetActiveDestReq,
    TURN_SIGNAL_SetActiveDestResp,
    TURN_SIGNAL_SetActiveDestRespError,
    TURN_SIGNAL_TimerTick,
    TURN_SIGNAL_TimerRetransmit,
    TURN_SIGNAL_TimerRefreshAlloc,
    TURN_SIGNAL_TimerRefreshChannel,
    TURN_SIGNAL_TimerRefreshPermission,
    TURN_SIGNAL_TimerStunKeepAlive,
    TURN_SIGNAL_DeAllocate,

    TURN_SIGNAL_Illegal = -1
} TURN_SIGNAL;

/* Internal message formats */
typedef struct {
    char                serverAddr[IPV4_ADDR_LEN_WITH_PORT];
    char                username[STUN_MSG_MAX_USERNAME_LENGTH];
    char                password[STUN_MSG_MAX_PASSWORD_LENGTH];
    uint32_t            sockhandle;
    uint32_t            stunTimeoutList[STUNCLIENT_MAX_RETRANSMITS];
    STUN_SENDFUNC       sendFunc;
    void               *userCtx;
    TURNCB              turnCbFunc;
    TurnCallBackData_T *turnCbData;
    uint32_t            threadCtx;
    bool                isMsStun;
} TurnAllocateReqStuct;


/* Internal TURN states */
typedef enum {
    TURN_STATE_Idle = 0,
    TURN_STATE_WaitAllocRespNotAuth,
    TURN_STATE_WaitAllocResp,
    TURN_STATE_Allocated,
    TURN_STATE_WaitAllocRefreshResp,
    TURN_STATE_WaitChanBindResp,
    TURN_STATE_WaitCreatePermResp,
    TURN_STATE_WaitSetActiveDestResp,
    TURN_STATE_WaitReleaseResp,
    TURN_STATE_End  /* must be last */
} TURN_STATE;



/********************************************/
/******  instance data ********  (internal) */
/********************************************/

 
typedef struct
{
    uint32_t                 channelNumber;
    char                     peerTrnspAddrStr[IPV4_ADDR_LEN_WITH_PORT];
    bool                     createPermission;
}
TurnChannelBindInfo_T;

typedef struct
{
    uint32_t                 numberOfPeers;
    char                     peerTrnspAddrStr[TURN_MAX_PERMISSION_PEERS][IPV4_ADDR_LEN_WITH_PORT];
}
TurnCreatePermissionInfo_T;


typedef struct 
{
    TURN_STATE             state;
    bool                   inUse;
    int                    inst;
    TurnAllocateReqStuct   turnAllocateReq;
    uint32_t               threadCtx;                  /* identified which thread is using this instance */
    StunMsgId              StunReqTransId;                       /* transaction id of request */
    StunMsgId              PrevRespTransId;                      /* transaction id of last recevied */
    uint8_t                stunReqMsgBuf[STUN_MAX_PACKET_SIZE]; /* encoded STUN request    */
    int                    stunReqMsgBufLen;                     /* of encoded STUN request */

    STUN_USER_CREDENTIALS userCredentials;
    bool authenticated;
    bool permissionsInstalled;
    bool channelBound;

    /* returned in allocate resp */
    char  rflxAddr[IPV4_ADDR_LEN];
    int   rflxPort;
    char  relAddr[IPV4_ADDR_LEN];
    int   relPort;
    uint32_t lifetime;               /* Seconds */
    uint32_t bandwidthKbps;          /* kbps */
    bool   hasMsSeqNr;               /* MS2: Sequence number must be used */
    StunAttrSequenceNum MsSeqNum;    /* MS2 only */
    TurnChannelBindInfo_T      channelBindInfo;
    TurnCreatePermissionInfo_T createPermInfo;
    /* timers */
    int32_t TimerRetransmit;      
    int32_t TimerRefreshAlloc;    
    int32_t TimerRefreshChannel;
    int32_t TimerRefreshPermission;
    int32_t TimerStunKeepAlive;
    int     retransmits;

    void *userData;
} TURN_INSTANCE_DATA;


/* state function */
typedef void (*TURN_STATE_FUNC)(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);

/* entry in state table */
typedef struct 
{
    TURN_STATE_FUNC Statefunc;
    const char *StateStr;
}
TURN_STATE_TABLE;

#if defined(UNITTEST)
int  TurnClientSimulateSig(uint32_t threadCtx, int ctx, TURN_SIGNAL sig);
#endif


#ifdef __cplusplus
}
#endif


#endif  /* TURN_INTERN_H */
