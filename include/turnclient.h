/***************************************************************************
 *                   T U R N  C L I E N T  LIB 
 *            Code library to handle and react to TURN messages
 *--------------------------------------------------------------------------
 *==========================================================================
 * Version       : 1.0
 * Status        : Draft
 *
 * Filetype      : Code File 
 *
 * Author        : Paul Mackell (PTM), Tandberg Telecom AS.
 *
 * Compiler      : ANSI C
 * Portability   : Any ANSI C compiler, no OS calls required.
 *
 * Switches      : None
 *
 * Import        : TURN is a usage of  STUN and therefore includes the stun api.
 *                  
 * Compatibility: RFC5766               
 *
 * Description   : Application interface to the turn client library.
 *                 
 *
 ******************************************************************************/


#ifndef TURNCLIENT_H
#define TURNCLIENT_H


#include <string.h>

#include <netinet/in.h>

#include "stunlib.h"   /* stun enc/dec and msg formats*/

#ifdef __cplusplus
extern "C" {
#endif



//Where to put this
#define RTP_STUN_THREAD_CTX      1
#define BFCP_STUN_THREAD_CTX     2

#define TURN_DFLT_PORT        3478

#define TURN_MAX_THREAD_CTX      4
#define TURN_MAX_ERR_STRLEN    256  /* max size of string in TURN_INFO_FUNC */
#define TURNCLIENT_CTX_UNKNOWN  -1

/* max. number of  Peers in a createPermissionRequest */ 
#define TURN_MAX_PERMISSION_PEERS 10 


/* Result of  Turn  protocol, returned in callback */
typedef enum 
{
    TurnResult_Empty,                       /* Used for testing */
    TurnResult_AllocOk,                     /* Turn allocation was successful */
    TurnResult_AllocFail,                   /* Turn Allocation failed */
    TurnResult_AllocFailNoAnswer,           /* Turn Allocation failed - no contact with turn server */
    TurnResult_AllocUnauthorised,           /* passwd/username is incorrect */
    TurnResult_CreatePermissionOk,          /* successfull CreatePermission */
    TurnResult_CreatePermissionFail,        /* Failed CreatePermission - no contact with turn server */
    TurnResult_CreatePermissionNoAnswer,    /* CreatePermission failed  */
    TurnResult_CreatePermissionQuotaReached,/* Quouta reached */
    TurnResult_PermissionRefreshFail,       /* Refresh Permission failed  */
    TurnResult_ChanBindOk,                  /* successful Channel Bind */
    TurnResult_ChanBindFail,                /* Channel Bind failed */
    TurnResult_ChanBindFailNoanswer,        /* Channel bind failed - no contact with turn server */
    TurnResult_SetActiveDestOk,             /* successful SetActiveDestination (MSSTUN) */
    TurnResult_SetActiveDestFail,           /* Failed SetActiveDestination (MSSTUN) */
    TurnResult_SetActiveDestFailNoAnswer,   /* Failed SetActiveDestination (MSSTUN), no answer */
    TurnResult_RefreshFail,                 /* Allocation Refresh failed */
    TurnResult_RefreshFailNoAnswer,         /* Allocation Refresh failed */
    TurnResult_RelayReleaseComplete,        /* Relay has been sucessfully released */
    TurnResult_RelayReleaseFailed,          /* Relay released failed */
    TurnResult_InternalError,             
    TurnResult_MalformedRespWaitAlloc       /* server problem occurred when waiting for alloc resp */
} TurnResult_T;


/* 
 * Result of successful Turn allocation (TurnResult_AllocOk) has the following format.
 *     rflxddr/Port -   Reflexive Address/port.  This is source addr/port of the AllocateRequest as seen by the turn server
 *     relAddr/port -   Relay Address/Port. As allocated on the  turn server.
 */
typedef struct
{
    struct sockaddr_storage activeTurnServerAddr;
    struct sockaddr_storage rflxAddr;
    struct sockaddr_storage relAddr;
} TurnAllocResp;


/* */
typedef struct
{
    int dummy;
} TurnChanBindResp;


/* Signalled back to the caller as a paramter in the TURN callback (see TURNCB) */
typedef struct
{
    TurnResult_T turnResult;
    union
    {
        TurnAllocResp    AllocResp;
        TurnChanBindResp ChanBindResp;
    } TurnResultData;

} TurnCallBackData_T;

/* category of info sent in TURN_INFO_FUNC */
typedef enum
{
    TurnInfoCategory_Info,
    TurnInfoCategory_Error,
    TurnInfoCategory_Trace

} TurnInfoCategory_T;


typedef struct
{
    uint32_t CurrentAllocations;
    uint32_t Retransmits;
    uint32_t Failures;
    struct
    {
        TurnAllocResp AllocResp;
        bool channelBound;
        uint32_t channelNumber;
        uint32_t expiry;
        bool permissionsInstalled;
        struct sockaddr_storage BoundPeerTrnspAddr;
        struct sockaddr_storage PermPeerTrnspAddr[TURN_MAX_PERMISSION_PEERS];
        uint32_t numberOfPeers; /* in permission */
    } instance[50];
}
TurnStats_T;


/* Signalling back to user e.g. result of AllocateResp, ChanBindResp etc...
 *   userCtx        - User provided context, as provided in TurnClient_startAllocateTransaction(userCtx,...)
 *   TurnCbData     - User provided turn callback data. Turn writes status here. e.g. Alloc ok + reflexive + relay address
 */
typedef void (*TURNCB)(void *userCtx, TurnCallBackData_T *turnCbData);

/* for output of managment info (optional) */
typedef void  (*TURN_INFO_FUNC_PTR)(TurnInfoCategory_T category, char *ErrStr);


/*
 *  initialisation:
 *    Should be called (once only) before TurnClient_startStunAllocateTransaction(). 
 *
 *    threadCtx   - thread context. (0..TURN_MAX_THREAD_CTX-1)
 *    instances   - max. number of simultaneous  TURN transactions to support.
 *
 *    tickMsec    - Tells turnclient how often TurnClient_HandleTick() is called.
 *
 *    funcPtr     - Will be called by Turn when it outputs management info and trace. 
 *                  If this is NULL, then there is no output.  You can provide a function such as below::
 *
 *                  static void  PrintTurnInfo(TurnInfoCategory_T category, char *ErrStr)
 *                  {
 *                       fprintf(stderr, "%s\n", ErrStr);
 *                  }
 *
 *    MultipleThreadAccess - Set to true if the turnclient is called from > 1 thread. (mutex will be created internally).
 *                           Note - Under WIN32 this mutex is a CRITICAL_SECTION  (much faster than WIN32 MUTEX).
 *                           Set to false if the turnclient is called from 1 thread.  (no internal mutex required)
 *
 *    SwVerStr    - Software version string to be sent in TURN Requests
 */
bool TurnClient_Init(uint32_t  threadCtx,
                     int instances, 
                     uint32_t tickMsec, 
                     TURN_INFO_FUNC_PTR funcPtr, 
                     bool MultipleThreadAccess,
                     const char *SwVerStr);


/*
 *  Initiate a Turn Allocate Transaction
 *     threadCtx        -  thread context. (0..TURN_MAX_THREAD_CTX-1)
 *     userCtx          -  user specific context info (e.g. CallId/ChanId).  Optional, can be NULL. TURN does not write to this data.
 *     turnServerAddr   -  Address of TURN server in format  "a.b.c.d:port"
 *     userName         -  \0 terminated string. Max 512 chars.
 *     password         -  \0 terminated string. Max 512 chars.
 *     sockhandle       -  used as 1st parameter in STUN_SENDFUNC(), typically a socket.
 *     addrFamily      -  requested address family (AF_INET or AF_INET6)
 *     sendFunc         -  function used to send STUN packet. send(sockhandle,buff, len, turnServerAddr, userCtx)
 *     timeoutList      -  Defines interval between each retry in msec. 0 terminated.  e.g. 500, 1000 results in 
 *                         retry after 500msec and retry after 1000 msec. If this is NULL, then TURN uses the default
 *                         stun retries defined in stunlib.h
 *     turnCbFunc       -  user provided callback function used by turn to signal the result of an allocation or channel bind etc...
 *     TurnCbData       -  user provided callback turn data. turn writes to this data area.
 *     isMsStun         -  true=microsoft stun implementation, false = standard.
 *         
 *     returns          -  Turn instance/context. Application should store this in further calls to TurnClient_StartChannelBindReq(), TurnClient_HandleIncResp(). 
 */
int  TurnClient_startAllocateTransaction(uint32_t               threadCtx,
                                         void                  *userCtx,
                                         const struct sockaddr *turnServerAddr,
                                         const char            *userName,
                                         const char            *password,
                                         uint32_t               sockhandle,
                                         uint16_t               addrFamily,
                                         STUN_SENDFUNC          sendFunc,
                                         uint32_t              *timeoutList,
                                         TURNCB                 TurnCbFunc,
                                         TurnCallBackData_T    *TurnCbData,
                                         bool                   isMsStun);

/*
 * Bind Channel Number to peer transport adreess.
 *     threadCtx        -  thread context. (0..TURN_MAX_THREAD_CTX-1)
 *     ctx              - As returned from Allocations, i.e. ctx =  TurnClient_startAllocateTransaction()
 *     channelNumber    - Valid range is  0x4000-0xFFFE
 *     peerTrnspAddrStr - Peer string in format "a.b.c.d:port"
 *
 */
bool TurnClient_StartChannelBindReq(uint32_t               threadCtx,
                                    int                    ctx,
                                    uint32_t               channelNumber,
                                    const struct sockaddr *peerTrnspAddr);

/*
 * Create a permission in turn server.  i.e. CreatePermission(List of RemotePeers).
 * This will enable the turn server to route DataIndicatins from the Remote peers.
 * 
 *     threadCtx        -  thread context. (0..TURN_MAX_THREAD_CTX-1)
 *     ctx              - As returned from Allocations, i.e. ctx =  TurnClient_startAllocateTransaction()
 *     noOfPeers        - Number of peer addresses in peerTrnspAddrStr string array
 *     peerTrnspAddrStr - Pointer to array of strings in format "a.b.c.d:port". Note - Port is not used in create permission.
 *
 */
bool TurnClient_StartCreatePermissionReq(uint32_t         threadCtx,
                                         int              ctx,
                                         uint32_t         noOfPeers,
                                         struct sockaddr *peerTrnspAddr[]);


/* 
 * This function must be called by the application every N msec. N must be same as in TurnClient_Init(instances, N) 
 *     threadCtx        -  thread context. (0..TURN_MAX_THREAD_CTX-1)
 */
void TurnClient_HandleTick(uint32_t threadCtx);

/*
 *  threadCtx - thread context. (0..TURN_MAX_THREAD_CTX-1)
 *  ctx       - Appl. should use the ctx returned from TurnClient_startAllocateTransaction(). However
 *              if this is not known, use TURNCLIENT_CTX_UNKNOWN and the turn client will internally search all instances for a match. 
 *  msg       - Decoded STUN message.
 *  buf       - Original buffer holding unencoded message. (Needed to calculate integrity)
 *
 * return - instance/ctx (of use only when TURNCLIENT_CTX_UNKNOWN is input). 
 *
 */
int TurnClient_HandleIncResp(uint32_t     threadCtx,
                             int          ctx, 
                             StunMessage *msg,
                             char         *buf);
/*
 * Encode a TURN send indication.  
 * \param  stunbuf          Buffer to store encoded STUN message in
 * \param  dataBuf          Data packet to send. Note that setting this to NULL means that the data is already positioned 
 *                          at stunbuf+36 and there is no need to memcpy this. (zero copy).
 * \param  maxBufSize       Length of StunBuf
 * \param  payloadLength    
 * \param  dstAddr          Peer Address "a.b.c.d"
 * \param  dstPort          Peer Port
 * \param  isMsStun         true=Microsoft Stun
 * \param  turnInst         Only reelvant when isMsStun=true. Used to find SeqNr and Username attributes. 
 * Returns encoded length of packet 
 */
int TurnClient_createSendIndication(unsigned char   *stunBuf,
                                    uint8_t         *dataBuf,
                                    uint32_t         maxBufSize,
                                    uint32_t         payloadLength, 
                                    struct sockaddr *dstAddr,
                                    bool             isMsStun,
                                    uint32_t         threadCtx,
                                    int              turnInst);  


/* TURN will be active for the duration of the Call Session. 
 * TurnClient_Deallocate() must be called when the session terminates
 *  threadCtx - thread context. (0..TURN_MAX_THREAD_CTX-1)
 *  ctx       - Appl. should use the ctx returned from TurnClient_startAllocateTransaction().
 */
void TurnClient_Deallocate(uint32_t threadCtx, int ctx);

/* Store a copy of Active TurnServer address in s */
void TurnClientGetActiveTurnServerAddr(uint32_t threadCtx, int turnInst, struct sockaddr *s);

/* return header length of SendRequest  (MSSTUN only) */
uint32_t TurnClientGetSendReqHdrSize(uint32_t threadCtx, int turnInst);


/* management */
void TurnClientGetStats(TurnStats_T *Stats);
void TurnClientClearStats(void);

/* timer tuning/debugging only */
void     TurnClient_SetAllocRefreshTimerSec(uint32_t ValueSec);
uint32_t TurnClient_GetAllocRefreshTimerSec(void);
void     TurnClient_SetChanRefreshTimerSec(uint32_t ValueSec);
uint32_t TurnClient_GetChanRefreshTimerSec(void);
void     TurnClient_SetStunKeepAlive(bool onOff);
bool     TurnClient_GetStunKeepAlive(void);




#ifdef __cplusplus
}
#endif

#endif /* TURNCLIENT_H */
