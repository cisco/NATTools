/***************************************************************************
 *                   T U R N  C L I E N T  LIB 
 *            Code library for interpret and react to TURN messages
 *--------------------------------------------------------------------------
 *                  (C) Copyright Tandberg Telecom AS 2008
 *==========================================================================
 * Author        : Paul Mackell (PTM), Tandberg Telecom AS.
 *
 * Compiler      : ANSI C
 * Portability   : High, No OS Calls, no printfs. 
 *
 * Switches      : None
 *
 * Compatability: TURN-16                  
 *                 
 *
 * Description   :
 * Turn-09 client implemented as an OS independent state machine.
 * Initialisation:
 *      Application calls TurnClient_init().
 *
 * Entrypoints:
 *   1. Appl. calls TurnClient_startAllocateTransaction() to initiate the turn protocol sequence. 
 *   2. Appl. calls TurnClient_HandleTick() every N msec such that it can carry out timing/retransmissions.
 *   3. Appl. calls TurnClient_HandleIncResp() when it detects incoming turn responses in the media RTP/RTCP stream.
 *   3. Appl. calls TurnClient_StartCreatePermissionReq() so that  stun probes can go thru the TURN server
 *   4. Appl. optionally calls TurnClient_StartChannelBindReq() to optimise relay traffic
 *   6. Application calls TurnClient_Deallocate() to release the allocation
 *      
 * Outputs:
 *      1. Application provides function pointer and  data ptr to receive the result of the  turn protocol.         
 *
      

             
            ------------
    ------>|    IDLE    |<----------------------------
   |        ------------                             |
   |             |                                   |
   |             |                                   |
   |             |                                   |
   |(Error)  ----------                              |
   |        |   WAIT   |<--                          | 
   |--------|   ALLOC  |   | (T (Retries))           |
   |        |   RESP   |   |                         |
   |        |  NOT_AUT |---                          |
   |         ----------                              |
   |             |                                   |
   |             | (Auth_Failure (401))              |
   |             |                                   |
   |(Error) -----------                              |
   |        |   WAIT   |<--                          |
   |        |   ALLOC  |   | (T (Retries))           |
    --------|   RESP   |---                          |
             ----------                              |
                 |                                   |
                 | (Allocate_Response)               |
                 |                                   |
            -------------                           ----------- 
           |             |                         | WAIT      |
           |  ALLOCATED  |------------------------>| RELEASE   |
           |             |                         | RESP      |
            -------------                           ----------- 
                 |        
                 |-------------------------------------
                 |            |           |           |        
                 |            |           |           |        
                 v            v           v           v
           ------------   ---------    ----------  -----------  
          |    WAIT   | | WAIT     | | WAIT     | | WAIT      | 
          |PERMISSION | | SETACTIVE| | CHANBIND | | REFRESH   | 
          |    RESP   | | DESTRESP | | RESP     | | RESP      | 
           ------------   ---------   ----------   -----------  

 ******************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>



#include "turnclient.h"
#include "turn_intern.h"
#include "stun_os.h"
#include "sockaddr_util.h"


/* instance data, dynamically created as required */
static TURN_INSTANCE_DATA **InstanceData[TURN_MAX_THREAD_CTX];

static int                  CurrentInstances[TURN_MAX_THREAD_CTX];
static int                  MaxInstances[TURN_MAX_THREAD_CTX];
static uint32_t             TimerResMsec[TURN_MAX_THREAD_CTX];      /* how often this is called */
static TURN_INFO_FUNC_PTR   TurnErrFuncPtr[TURN_MAX_THREAD_CTX];    /* function called to output management info */
static bool                 MutexLockRequired[TURN_MAX_THREAD_CTX]; /* true if called from > 1 thread */
static uint32_t             AllocRefreshTimerSec;
static uint32_t             ChanRefreshTimerSec;
static STUN_MUTEX           *InstanceMutex[TURN_MAX_THREAD_CTX];    /* created/alloc'd at startup */
static bool                 DoStunKeepAlive;

/* use when no timeout list is provided */
static uint32_t StunDefaultTimeoutList[STUNCLIENT_MAX_RETRANSMITS] = { STUNCLIENT_RETRANSMIT_TIMEOUT_LIST};
static uint32_t StunDefaultTimeoutListMs2[STUNCLIENT_MAX_RETRANSMITS] = { STUNCLIENT_RETRANSMIT_TIMEOUT_LIST_MS2};
//static uint32_t StunDefaultTimeoutList[STUNCLIENT_MAX_RETRANSMITS] = {1000, 1000, 1000, 0};

static char  SoftwareVersionStr[100];

static TurnStats_T TurnStats;

/* forward declarations */
static int   TurnClientMain(uint32_t threadCtx, int ctx, TURN_SIGNAL sig, uint8_t *payload);
static void  TurnClientFsm(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);
static void  SetNextState(TURN_INSTANCE_DATA *pInst,  TURN_STATE NextState);
static void  TurnPrint(uint32_t threadCtx, TurnInfoCategory_T category, const char *fmt, ...);
static bool  TimerHasExpired(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig);


/* forward declarations of state functions */
static void  TurnState_Idle(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);
static void  TurnState_WaitAllocRespNotAut(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);
static void  TurnState_WaitAllocResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);
static void  TurnState_Allocated(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);
static void  TurnState_WaitChanBindResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);
static void  TurnState_WaitCreatePermResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);
static void  TurnState_WaitAllocRefreshResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);
static void  TurnState_WaitSetActiveDestResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);
static void  TurnState_WaitReleaseResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload);


/*************************************************************************/
/************************ UTILS*******************************************/
/*************************************************************************/

static void  MutexCreate(uint32_t threadCtx, int inst)
{
    char s[20];

    if (!MutexLockRequired[threadCtx]) return;
    sprintf(s, "TRNC%d%d", threadCtx, inst+1); 
    if (Stun_MutexCreate(&InstanceMutex[threadCtx][inst+1], s))
        TurnPrint(threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> Stun_MutexCreate(%s) OK", inst, s);
    else
        TurnPrint(threadCtx,TurnInfoCategory_Error, "<TURNCLIENT:%02d> Stun_MutexCreate(%s) failed", inst, s);
}

static void  MutexLock(uint32_t threadCtx, int inst)
{
    if (!MutexLockRequired[threadCtx]) return;
    if (Stun_MutexLock(&InstanceMutex[threadCtx][inst+1]))
        TurnPrint(threadCtx,TurnInfoCategory_Trace, "<TURNCLIENT:%02d> Stun_MutexLock() OK", inst);
    else
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Stun_MutexLock() failed", inst);
}

static void  MutexUnlock(uint32_t threadCtx, int inst)
{
    if (!MutexLockRequired[threadCtx]) return;
    if (Stun_MutexUnlock(&InstanceMutex[threadCtx][inst+1]))
        TurnPrint(threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> Stun_MutexLUnlock() OK", inst);
    else
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Stun_MutexUnlock() failed", inst);
}

/*
 * Called when an internal TURNCLIENT wants to output managemnt info. 
 * Prints the string to a buffer.
 * If the application has defined a callback function to handle the output
 * then this is called with the string and the severity.
 */
static void TurnPrint(uint32_t threadCtx, TurnInfoCategory_T category, const char *fmt, ...)
{
  char s[TURN_MAX_ERR_STRLEN];

  va_list ap;
  va_start(ap,fmt);

  /* print string to buffer  */
  vsprintf(s, fmt, ap);

  if (TurnErrFuncPtr[threadCtx])
    /* Call the application defined "error callback function" */
    (TurnErrFuncPtr[threadCtx])(category, s);

  va_end(ap);

}

static int TurnRand(void)
{
    return rand();
}


static bool isMsStun(TURN_INSTANCE_DATA *pInst)
{
    return pInst->turnAllocateReq.isMsStun;
}

static TURN_SIGNAL StunMsgToInternalTurnSig(uint32_t threadCtx, StunMessage *msg)
{
    switch (msg->msgHdr.msgType)
    {
        case STUN_MSG_AllocateResponseMsg:               return TURN_SIGNAL_AllocateResp;
        case STUN_MSG_AllocateErrorResponseMsg:          return TURN_SIGNAL_AllocateRespError;
        case STUN_MSG_CreatePermissionResponseMsg:       return TURN_SIGNAL_CreatePermissionResp;
        case STUN_MSG_CreatePermissionErrorResponseMsg:  return TURN_SIGNAL_CreatePermissionRespError;
        case STUN_MSG_RefreshResponseMsg:                return TURN_SIGNAL_RefreshResp;
        case STUN_MSG_RefreshErrorResponseMsg:           return TURN_SIGNAL_RefreshRespError;
        case STUN_MSG_ChannelBindResponseMsg:            return TURN_SIGNAL_ChannelBindResp;
        case STUN_MSG_ChannelBindErrorResponseMsg:       return TURN_SIGNAL_ChannelBindRespError;
        case STUN_MSG_MS2_SetActiveDestReqMsg:           return TURN_SIGNAL_SetActiveDestReq;
        case STUN_MSG_MS2_SetActiveDestResponseMsg:      return TURN_SIGNAL_SetActiveDestResp;
        case STUN_MSG_MS2_SetActiveDestErrorResponseMsg: return TURN_SIGNAL_SetActiveDestRespError;

        default:
            /* Some other message */
            TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT> unknown STUN message type (0x%02x)", msg->msgHdr.msgType);
            return TURN_SIGNAL_Illegal;
            break;
    }
}


/* transaction id compare */
static bool TransIdIsEqual(const StunMsgId *a, const StunMsgId *b)
{
    return (memcmp(a, b, sizeof(StunMsgId)) == 0);
}

/* Store transaction id of a request  */
static void StoreReqTransId(TURN_INSTANCE_DATA *pInst, StunMessage  *stunReqMsg)
{
    memcpy(&pInst->StunReqTransId, &stunReqMsg->msgHdr.id, sizeof(pInst->StunReqTransId));
}

/* Store transaction id of  resp  */
static void StorePrevRespTransId(TURN_INSTANCE_DATA *pInst, StunMessage  *stunRespMsg)
{
    memcpy(&pInst->PrevRespTransId, &stunRespMsg->msgHdr.id, sizeof(pInst->PrevRespTransId));
}



/*************************************************************************/
/************************ API ********************************************/
/*************************************************************************/



bool TurnClient_Init(uint32_t threadCtx, 
                     int instances, 
                     uint32_t tMsec, 
                     TURN_INFO_FUNC_PTR InfoFuncPtr, 
                     bool MultipleThreadAccess, 
                     const char *SwVerStr)
{
    MutexLockRequired[threadCtx] = MultipleThreadAccess;

    if (InstanceData[threadCtx])
      return true;          /* already initialised */

    InstanceMutex[threadCtx] = calloc(instances+1, sizeof(STUN_MUTEX));
    if (InstanceMutex[threadCtx] == NULL) return false;

    MutexCreate(threadCtx, -1); /* global mutex */

    if (SwVerStr)
        strncpy(SoftwareVersionStr, SwVerStr, sizeof(SoftwareVersionStr)-1);

  //  if (InfoFuncPtr[threadCtx] != NULL)
        TurnErrFuncPtr[threadCtx]          = InfoFuncPtr;
    TimerResMsec[threadCtx] = tMsec;

    InstanceData[threadCtx] = calloc(instances, sizeof(TURN_INSTANCE_DATA*));
    if (InstanceData[threadCtx] == NULL) return false;

    CurrentInstances[threadCtx] = 0;
    MaxInstances[threadCtx]     = instances;
    TurnPrint(threadCtx, TurnInfoCategory_Trace, "\n<TURNCLIENT> startup, threadCtx %d MaxInstances %d CurrentInstances %d, tick %d msec MultiThread %d", 
              threadCtx, MaxInstances[threadCtx], CurrentInstances[threadCtx], tMsec, MultipleThreadAccess); 

    /* as default always send stun KeepAlives */
    DoStunKeepAlive = true;

    return true;
}



/* Create a new instance */
static int TurnClientConstruct(uint32_t threadCtx)
{
    TURN_INSTANCE_DATA *p;
    int  *curr;

    curr = &CurrentInstances[threadCtx];
    if (*curr >= MaxInstances[threadCtx])
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "\n<TURNCLIENT> Failed to Create thread %d inst %d, maximum instances %d exceeded  !!\n", 
                  threadCtx, *curr+1, MaxInstances[threadCtx]);
        return TURNCLIENT_CTX_UNKNOWN;
    }

    p =  (TURN_INSTANCE_DATA*)calloc(1, sizeof(TURN_INSTANCE_DATA));
    if (p)
    {
        MutexCreate(threadCtx, CurrentInstances[threadCtx]);
        MutexLock(threadCtx, CurrentInstances[threadCtx]);
        InstanceData[threadCtx][*curr] = p;
        InstanceData[threadCtx][*curr]->inst = *curr;
        InstanceData[threadCtx][*curr]->state = TURN_STATE_Idle;
        InstanceData[threadCtx][*curr]->inUse = true;
        InstanceData[threadCtx][*curr]->threadCtx = threadCtx;
        TurnPrint(threadCtx, TurnInfoCategory_Trace, "\n<TURNCLIENT> Create inst[%d].threadCtx %d\n", *curr, 
                  InstanceData[threadCtx][*curr]->threadCtx);
        *curr = *curr+1;
        MutexUnlock(threadCtx, CurrentInstances[threadCtx]-1);

        return *curr-1;
    }
    else
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "\n<TURNCLIENT> Failed to Create thread %d inst %d, no memory !!\n", threadCtx, *curr);
        return TURNCLIENT_CTX_UNKNOWN;
    }
}


int TurnClient_startAllocateTransaction(uint32_t               threadCtx,
                                        void                  *userCtx,
                                        const struct sockaddr *serverAddr,
                                        const char            *userName,
                                        const char            *password,
                                        uint32_t               sockhandle,
                                        STUN_SENDFUNC          sendFunc,
                                        uint32_t              *timeoutList,
                                        TURNCB                 turnCbFunc,
                                        TurnCallBackData_T    *turnCbData,
                                        bool                   isMsStun)
{
    TurnAllocateReqStuct m;
    uint32_t i;
    uint32_t *pTimeoutList;
    int ret;
    
    if (InstanceData[threadCtx] == NULL)
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT> startAllocateTransaction failed,  Not initialised or  no memory, threadCtx %d", threadCtx);
        return TURNCLIENT_CTX_UNKNOWN;
    }

    memset(&m, 0, sizeof(m));

    
    sockaddr_copy((struct sockaddr *)&m.serverAddr, serverAddr);
    strncpy(m.username, userName, sizeof(m.username));
    strncpy(m.password, password, sizeof(m.password));
    m.sockhandle     = sockhandle;
    m.sendFunc       = sendFunc;
    m.userCtx        = userCtx;
    m.threadCtx      = threadCtx;
    m.isMsStun       = isMsStun;

    
    /* use default timeout list if none provided */
    pTimeoutList = timeoutList;
    if (pTimeoutList == NULL)
        pTimeoutList = isMsStun ?  StunDefaultTimeoutListMs2 : StunDefaultTimeoutList;

    for (i=0; i < sizeof(m.stunTimeoutList)/sizeof(m.stunTimeoutList[0]); i++)
    {
        if (pTimeoutList[i])
            m.stunTimeoutList[i] = pTimeoutList[i];
        else
            break; /* 0 terminated list */
    }

    /* callback and  data (owned by caller) */
    m.turnCbFunc = turnCbFunc;
    m.turnCbData = turnCbData;

    ret = TurnClientMain(threadCtx, TURNCLIENT_CTX_UNKNOWN, TURN_SIGNAL_AllocateReq, (uint8_t*)&m);

    return ret;

}

void TurnClientGetActiveTurnServerAddr(uint32_t threadCtx, int turnInst, struct sockaddr *s)
{
    TURN_INSTANCE_DATA *pInst;

    if ((turnInst < 0) || (turnInst >= CurrentInstances[threadCtx]))
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "TurnClientGetActiveTurnServerAddr() invalid inst %d (thread %d)\n",  turnInst, threadCtx);
        return;
    }
    pInst = InstanceData[threadCtx][turnInst];
    if (!pInst)
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "TurnClientGetActiveTurnServerAddr() NULL data inst %d (thread %d)\n",  turnInst, threadCtx);
        return;
    }

    if ( !sockaddr_isSet((struct sockaddr *)&pInst->turnAllocateReq.serverAddr) )
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "TurnClientGetActiveTurnServerAddr() serveraddr not set, inst %d (thread %d)\n",  turnInst, threadCtx);
        return;
    }
    //TurnPrint(threadCtx, TurnInfoCategory_Trace, "TurnClientGetActiveTurnServerAddr() inst %d (thread %d), server %s\n",  turnInst, threadCtx, pInst->turnAllocateReq.serverAddr); 
    sockaddr_copy(s, (struct sockaddr *)&pInst->turnAllocateReq.serverAddr);

}

/* range 0x4000 - 0xFFFF, with channel number 0xFFFF reserved */
static  bool IsValidBindChannelNumber(uint32_t  chanNum)
{
    return (chanNum >= 0x4000 && chanNum != 0xFFFF);
}

static bool ChannelBindReqParamsOk(uint32_t               threadCtx,
                                   int                    ctx,
                                   uint32_t               channelNumber,
                                   const struct sockaddr *peerTrnspAddr)

{
    if (ctx >= CurrentInstances[threadCtx])
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT> ChannelBindReq - illegal context %d exceeds %d\n ", ctx, CurrentInstances[threadCtx]-1); 
        return false;
    }
    if (!IsValidBindChannelNumber(channelNumber))  /* channel number ignored if creating a permission */
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> ChannelBindReq - illegal channel number %0X\n ", ctx, channelNumber); 
        return false;
    }

    if ( !sockaddr_isSet(peerTrnspAddr))
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> ChannelBindReq - illegal peerTRansport Address\n ", ctx);
        return false;
    }
    
    return true;
}

static bool CreatePermReqParamsOk(uint32_t         threadCtx,
                                  int              ctx,
                                  uint32_t         numberOfPeers,
                                  struct sockaddr *peerTrnspAddr[])

{
    uint32_t i;
    bool ret = true;

    if (ctx >= CurrentInstances[threadCtx])
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT> CreatePerm - illegal context %d exceeds %d\n ", ctx, CurrentInstances[threadCtx]-1); 
        ret = false;
    }
    for (i=0; i < numberOfPeers; i++)
    {
        if ( !sockaddr_isSet(peerTrnspAddr[i]) )
        {
            TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> CreatePerm - illegal peerTRansport Address\n ", ctx);
            ret = false;
        }
    }
    return ret;
}



bool TurnClient_StartChannelBindReq(uint32_t threadCtx,
                                    int      ctx,
                                    uint32_t channelNumber,
                                    const struct sockaddr *peerTrnspAddr)
{
    if (ChannelBindReqParamsOk(threadCtx, ctx, channelNumber, peerTrnspAddr))
    {
        TurnChannelBindInfo_T msg;
        memset(&msg, 0, sizeof(msg));

        msg.channelNumber = channelNumber;
        sockaddr_copy((struct sockaddr *)&msg.peerTrnspAddr, 
                      peerTrnspAddr);
        TurnClientMain(threadCtx, ctx, TURN_SIGNAL_ChannelBindReq, (uint8_t*)&msg);
        return true;
    }
    return false;
}

bool TurnClient_StartCreatePermissionReq(uint32_t         threadCtx,
                                         int              ctx,
                                         uint32_t         noOfPeers,
                                         struct sockaddr *peerTrnspAddr[])
{
    if (CreatePermReqParamsOk(threadCtx, ctx, noOfPeers, peerTrnspAddr))
    {
        uint32_t i;
        TurnCreatePermissionInfo_T msg;
        memset(&msg, 0, sizeof(msg));

        for (i=0; i < noOfPeers; i++)
        {
            sockaddr_copy((struct sockaddr *)&msg.peerTrnspAddr[i], 
                          peerTrnspAddr[i]);
            msg.numberOfPeers++;
        }

        TurnClientMain(threadCtx, ctx, TURN_SIGNAL_CreatePermissionReq, (uint8_t*)&msg);
        return true;
    }
    return false;
}


void TurnClient_HandleTick(uint32_t threadCtx)
{
   /* call fsm for each timer that has expired */
    int i;

    for (i=0; i < CurrentInstances[threadCtx]; i++)
    {
        TURN_INSTANCE_DATA *pInst;
        pInst = InstanceData[threadCtx][i];
        if (TimerHasExpired(pInst, TURN_SIGNAL_TimerRetransmit))
            TurnClientFsm  (pInst, TURN_SIGNAL_TimerRetransmit, NULL);
        if (TimerHasExpired(pInst, TURN_SIGNAL_TimerRefreshAlloc))
            TurnClientFsm  (pInst, TURN_SIGNAL_TimerRefreshAlloc, NULL);
        if (TimerHasExpired(pInst, TURN_SIGNAL_TimerRefreshChannel))
            TurnClientFsm  (pInst, TURN_SIGNAL_TimerRefreshChannel, NULL);
        if (TimerHasExpired(pInst, TURN_SIGNAL_TimerRefreshPermission))
            TurnClientFsm  (pInst, TURN_SIGNAL_TimerRefreshPermission, NULL);
        if (TimerHasExpired(pInst, TURN_SIGNAL_TimerStunKeepAlive))
            TurnClientFsm  (pInst, TURN_SIGNAL_TimerStunKeepAlive, NULL);
    }
}


int  TurnClient_HandleIncResp(uint32_t threadCtx, int ctx, StunMessage *msg)
{
    if (ctx >= CurrentInstances[threadCtx])
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT> ChannelBindReq - illegal context %d exceeds %d\n ", ctx, CurrentInstances[threadCtx]-1); 
        return ctx;
    }

    /* if context not known, search all instances for matching transId on last sent request */
    if (ctx == TURNCLIENT_CTX_UNKNOWN)
    {
        int i;

        for (i=0; i < CurrentInstances[threadCtx]; i++)
        {
            if (TransIdIsEqual(&msg->msgHdr.id, &InstanceData[threadCtx][i]->StunReqTransId))
            {
                if (TransIdIsEqual(&msg->msgHdr.id, &InstanceData[threadCtx][i]->PrevRespTransId))
                {
                    /* silent discard duplicate msg */
                    TurnPrint(threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> %02x..%02x %s silent discard duplicate\n", 
                              i, msg->msgHdr.id.octet[0], msg->msgHdr.id.octet[11], stunlib_getMessageName(msg->msgHdr.msgType));
                }
                else
                {   
                    TurnPrint(threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> %02x..%02x %s\n", 
                              i, msg->msgHdr.id.octet[0], msg->msgHdr.id.octet[11], stunlib_getMessageName(msg->msgHdr.msgType));
                    StorePrevRespTransId(InstanceData[threadCtx][i], msg); 
                    TurnClientMain(threadCtx, i, StunMsgToInternalTurnSig(threadCtx, msg), (void*)msg);
                }
                return i;
            }
        }
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT> no instance with transId %02x..%02x, discarding, msgType %d\n ", 
                  msg->msgHdr.id.octet[0], msg->msgHdr.id.octet[11], msg->msgHdr.msgType);
        return TURNCLIENT_CTX_UNKNOWN;
    }

    if (TransIdIsEqual(&msg->msgHdr.id, &InstanceData[threadCtx][ctx]->PrevRespTransId))
    {
        /* silent discard duplicate msg */
        TurnPrint(threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> %02x..%02x %s silent discard duplicate\n", 
                  ctx, msg->msgHdr.id.octet[0], msg->msgHdr.id.octet[11], stunlib_getMessageName(msg->msgHdr.msgType));
        return ctx;
    }


    /* context known, just check transId matches last sent request on this instance */
    if (TransIdIsEqual(&msg->msgHdr.id, &InstanceData[threadCtx][ctx]->StunReqTransId))
    {
        TurnPrint(threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> %02x..%02x %s\n", 
                  ctx, msg->msgHdr.id.octet[0], msg->msgHdr.id.octet[11], stunlib_getMessageName(msg->msgHdr.msgType));
        StorePrevRespTransId(InstanceData[threadCtx][ctx], msg); 
        TurnClientMain(threadCtx, ctx, StunMsgToInternalTurnSig(threadCtx,msg), (void*)msg);
        return ctx;
    }
    else
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> mismatched transId rec: %02x..%02x, exp: %02x..%02x discarding, msgType %s\n ", ctx, 
                  msg->msgHdr.id.octet[0], msg->msgHdr.id.octet[11], 
                  InstanceData[threadCtx][ctx]->StunReqTransId.octet[0], 
                  InstanceData[threadCtx][ctx]->StunReqTransId.octet[11],
                  stunlib_getMessageName(msg->msgHdr.msgType)); 
        return TURNCLIENT_CTX_UNKNOWN;
    }
}


void TurnClient_Deallocate(uint32_t threadCtx, int ctx)
{
    TurnClientMain(threadCtx, ctx, TURN_SIGNAL_DeAllocate, NULL);
}


int TurnClient_createSendIndication(unsigned char   *stunBuf,
                                    uint8_t         *dataBuf, 
                                    uint32_t         maxBufSize,
                                    uint32_t         payloadLength, 
                                    struct sockaddr *dstAddr,           /* peer Address (e.g. as received in SDP) */
                                    bool             isMsStun,
                                    uint32_t         threadCtx,
                                    int              turnInst)        
{
    StunMessage stunMsg;
    StunIPAddress activeDstAddr; 
    TURN_INSTANCE_DATA *pInst;
    int length = 0;
    
    if (!sockaddr_isSet(dstAddr)){
        return -1;
    }


    memset(&stunMsg, 0, sizeof(StunMessage));
    stunlib_createId(&stunMsg.msgHdr.id, TurnRand(), 0);
    
    if (dstAddr->sa_family == AF_INET){
        
        activeDstAddr.familyType =  STUN_ADDR_IPv4Family;
        activeDstAddr.addr.v4.port = ((struct sockaddr_in *)dstAddr)->sin_port;
        activeDstAddr.addr.v4.addr = ((struct sockaddr_in *)dstAddr)->sin_addr.s_addr;

    }else if (dstAddr->sa_family == AF_INET6){
        activeDstAddr.familyType =  STUN_ADDR_IPv6Family;
        activeDstAddr.addr.v6.port = ((struct sockaddr_in6 *)dstAddr)->sin6_port;
        memcpy( activeDstAddr.addr.v6.addr ,
                ((struct sockaddr_in6 *)dstAddr)->sin6_addr.s6_addr,
                sizeof(activeDstAddr.addr.v6.addr) );
        
    }else{

        return -1;
    }

    if (!isMsStun)
    {
        /* STD TURN: sendInd(XorPeerAddr, Data)   no integrity */
        stunMsg.msgHdr.msgType =  STUN_MSG_SendIndicationMsg;
        stunMsg.xorPeerAddrEntries = 1;
        memcpy(&stunMsg.xorPeerAddress[0], &activeDstAddr, sizeof(StunIPAddress));
        stunMsg.xorPeerAddress[0].familyType =  STUN_ADDR_IPv4Family;
        stunMsg.hasData      = true;
        stunMsg.data.dataLen = payloadLength; 
        stunMsg.data.pData   = dataBuf;             /*The data (RTP packet) follows anyway..*/
        length = stunlib_encodeMessage(&stunMsg,
                                   stunBuf,
                                   maxBufSize,
                                   NULL,   /* no message integrity */
                                   0,      /* no message integrity */
                                   false,  /* verbose */
                                   isMsStun);
    }
    else
    {
        /* MS TURN: sendReq(Cookie,MsVers,[SeqNr],Username,DestAddr,Data,Integrity) */
        pInst = InstanceData[threadCtx][turnInst];

        if ((pInst == NULL) || (pInst->state == TURN_STATE_Idle))
        {
            TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURN:Send_Ind> MSSTUN,No instance data Ctx %d inst  %d or state is idle\n", threadCtx, turnInst);
            return -1;
        }

        stunMsg.msgHdr.msgType = STUN_MSG_MS2_SendRequestMsg;
        stunlib_addMsCookie(&stunMsg);
        stunlib_addMsVersion(&stunMsg, STUN_MS2_VERSION);
        if (pInst->hasMsSeqNr)
        {
            pInst->MsSeqNum.sequenceNumber++;
            stunlib_addMsSeqNum(&stunMsg, &pInst->MsSeqNum);
        }
        stunlib_addUserName(&stunMsg, pInst->userCredentials.stunUserName, STUN_DFLT_PAD);

        stunMsg.hasDestinationAddress = true;
        memcpy(&stunMsg.destinationAddress, &activeDstAddr, sizeof(StunIPAddress));
        stunMsg.destinationAddress.familyType =  STUN_ADDR_IPv4Family;
        stunMsg.hasData      = true;
        stunMsg.data.dataLen = payloadLength; 
        stunMsg.data.pData   = dataBuf;             /*The data (RTP packet) follows anyway..*/
        length = stunlib_encodeMessage(&stunMsg,
                                   stunBuf,
                                   maxBufSize,
                                   pInst->userCredentials.key,   /* message integrity */
                                   16,                           /* message integrity */
                                   false,                        /* verbose */
                                   isMsStun);
    }
    
    return length;
}

/* managemnet/statistics */
void TurnClientGetStats(TurnStats_T *Stats)
{
    int i, j;
    TurnStats.CurrentAllocations = 0;
    for (i=0; i < TURN_MAX_THREAD_CTX; i++)
    {
        for (j=0; j < CurrentInstances[i]; j++)
        {
            if (InstanceData[i][j]->state >= TURN_STATE_Allocated)
            {

                sockaddr_copy((struct sockaddr *)&TurnStats.instance[TurnStats.CurrentAllocations].AllocResp.rflxAddr,
                              (struct sockaddr *)&InstanceData[i][j]->rflxAddr);
                
                sockaddr_copy((struct sockaddr *)&TurnStats.instance[TurnStats.CurrentAllocations].AllocResp.relAddr,
                              (struct sockaddr *)&InstanceData[i][j]->relAddr);

                TurnStats.instance[TurnStats.CurrentAllocations].channelBound = InstanceData[i][j]->channelBound;
                if (InstanceData[i][j]->channelBound)
                {
                    TurnStats.instance[TurnStats.CurrentAllocations].channelNumber = InstanceData[i][j]->channelBindInfo.channelNumber;
                    sockaddr_copy((struct sockaddr *)&TurnStats.instance[TurnStats.CurrentAllocations].BoundPeerTrnspAddr, 
                                  (struct sockaddr *)&InstanceData[i][j]->channelBindInfo.peerTrnspAddr);
                    /* a bound channel also creates a permission, so show this  */
                    TurnStats.instance[TurnStats.CurrentAllocations].permissionsInstalled = 1;
                    TurnStats.instance[TurnStats.CurrentAllocations].numberOfPeers = 1;
                    sockaddr_copy((struct sockaddr *)&TurnStats.instance[TurnStats.CurrentAllocations].PermPeerTrnspAddr[0], 
                                  (struct sockaddr *)&InstanceData[i][j]->channelBindInfo.peerTrnspAddr);
                }
                else
                {
                    TurnStats.instance[TurnStats.CurrentAllocations].permissionsInstalled = InstanceData[i][j]->permissionsInstalled;
                    if (TurnStats.instance[TurnStats.CurrentAllocations].permissionsInstalled)
                    {
                        uint32_t k;
                        TurnStats.instance[TurnStats.CurrentAllocations].numberOfPeers = InstanceData[i][j]->createPermInfo.numberOfPeers;
                        for (k=0; k <  TurnStats.instance[TurnStats.CurrentAllocations].numberOfPeers; k++)
                            sockaddr_copy((struct sockaddr *)&TurnStats.instance[TurnStats.CurrentAllocations].PermPeerTrnspAddr[k], 
                                          (struct sockaddr *)&InstanceData[i][j]->createPermInfo.peerTrnspAddr[k]);
                            
                    }
                }
                TurnStats.CurrentAllocations++;
            }
        }
    }
    memcpy(Stats, &TurnStats, sizeof(TurnStats)); 
}

/* management/statistics */
void TurnClientClearStats(void)
{
    memset(&TurnStats, 0, sizeof(TurnStats)); 
}

/* MSICE. Calculate size of  SendReq header, excluding triling  integrity
 *    
 */
uint32_t TurnClientGetSendReqHdrSize(uint32_t threadCtx, int turnInst)
{
    TURN_INSTANCE_DATA *pInst = InstanceData[threadCtx][turnInst];
    if (pInst)
    {
        if (pInst->hasMsSeqNr)
            return TURN_SEND_REQ_HDR_SIZE_MS2+TURN_SEQ_LEN_MS2+strlen(pInst->userCredentials.stunUserName);
        else
            return TURN_SEND_REQ_HDR_SIZE_MS2+strlen(pInst->userCredentials.stunUserName);
    }
    else
    {
        TurnPrint(threadCtx, TurnInfoCategory_Error, "<TurnClientGetSendReqHdrSize> MSSTUN,No instance data Ctx %d inst  %d or state is idle\n", threadCtx, turnInst);
        return TURN_SEND_REQ_HDR_SIZE_MS2;
    }
}

/*************************************************************************/
/************************ FSM Framework **********************************/
/*************************************************************************/


static const TURN_STATE_TABLE StateTable[] = 
{
    /* function ptr */                      /* str */
    { TurnState_Idle,                       "Idle"                          },
    { TurnState_WaitAllocRespNotAut,        "WaitAllocRespNotAut"           },
    { TurnState_WaitAllocResp,              "WaitAllocResp"                 },
    { TurnState_Allocated,                  "Allocated"                     },
    { TurnState_WaitAllocRefreshResp,       "WaitAllocRefreshResp"          },
    { TurnState_WaitChanBindResp,           "WaitChanBindResp"              },
    { TurnState_WaitCreatePermResp,         "WaitCreatePermResp"            },
    { TurnState_WaitSetActiveDestResp,      "WaitActiveDestResp"            },
    { TurnState_WaitReleaseResp,            "WaitReleaseResp"               },

};

static const uint32_t NoOfStates = sizeof(StateTable)/sizeof(StateTable[0]);


static const char *TurnsigToStr(TURN_SIGNAL sig)
{
    switch (sig)
    {
        case TURN_SIGNAL_AllocateReq:               return "AllocateReq";
        case TURN_SIGNAL_AllocateResp:              return "AllocateResp";
        case TURN_SIGNAL_AllocateRespError:         return "AllocateRespError";
        case TURN_SIGNAL_ChannelBindReq:            return "ChannelBindReq";
        case TURN_SIGNAL_ChannelBindResp:           return "ChannelBindResp";
        case TURN_SIGNAL_ChannelBindRespError:      return "ChannelBindRespError";
        case TURN_SIGNAL_CreatePermissionReq:       return "CreatePermissionReq";      
        case TURN_SIGNAL_CreatePermissionResp:      return "CreatePermissionResp";      
        case TURN_SIGNAL_CreatePermissionRespError: return "CreatePermissionRespError";      
        case TURN_SIGNAL_RefreshResp:               return "RefreshResp";
        case TURN_SIGNAL_RefreshRespError:          return "RefreshRespError";
        case TURN_SIGNAL_SetActiveDestReq:          return "SetActiveDestReq";        
        case TURN_SIGNAL_SetActiveDestResp:         return "SetActiveDestResp";
        case TURN_SIGNAL_SetActiveDestRespError:    return "SetActiveDestRespError";  
        case TURN_SIGNAL_TimerTick:                 return "TimerTick";
        case TURN_SIGNAL_TimerRetransmit:           return "TimerRetransmit";
        case TURN_SIGNAL_TimerRefreshAlloc:         return "TimerRefreshAlloc";
        case TURN_SIGNAL_TimerRefreshChannel:       return "TimerRefreshChannel";
        case TURN_SIGNAL_TimerRefreshPermission:    return "TimerRefreshPermission";
        case TURN_SIGNAL_TimerStunKeepAlive:        return "TimerStunKeepAlive";
        case TURN_SIGNAL_DeAllocate:                return "DeAllocate";
        default:                                    return "???";
    }
}


/* */
static int getTurnInst(uint32_t threadCtx, TURN_SIGNAL *sig, uint8_t *payload)
{
  switch (*sig)
  {
      /* outgoing requests */
      case TURN_SIGNAL_AllocateReq:
      {
          int i;
          int ret;
          TurnAllocateReqStuct *pMsgIn = (TurnAllocateReqStuct*)payload;

          MutexLock(threadCtx, -1); /* Global Lock */

          for (i=0; i < CurrentInstances[threadCtx]; i++)
          {
              MutexLock(threadCtx, i);
              if (!InstanceData[threadCtx][i]->inUse)
              {   
                  InstanceData[threadCtx][i]->inUse = true;
                  InstanceData[threadCtx][i]->threadCtx = pMsgIn->threadCtx;
                  MutexUnlock(threadCtx, i);
                  MutexUnlock(threadCtx,-1);
                  return i;
              }
              MutexUnlock(threadCtx, i);
          }

          /* no free instances, try to construct a new one */
          ret = TurnClientConstruct(pMsgIn->threadCtx);  /* create/alloc a new instance */
          TurnPrint(threadCtx, TurnInfoCategory_Trace, "\n<TURNCLIENT> Construct thread %d inst %d\n", threadCtx, ret);

          MutexUnlock(threadCtx, -1);
          return ret;
      }
      break;

      /* May support routing of incoming TURN to correct instance here at a later date...  */

      default:
          return TURNCLIENT_CTX_UNKNOWN;
      break;
  }
  return TURNCLIENT_CTX_UNKNOWN;
}

/* stop specific timer signal */
static void StopAllTimers(TURN_INSTANCE_DATA *pInst)
{
    TurnPrint(pInst->threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> StopAllTimers", pInst->inst);
    pInst->TimerRetransmit        = 0;
    pInst->TimerRefreshAlloc      = 0;
    pInst->TimerRefreshChannel    = 0;
    pInst->TimerRefreshPermission = 0;
    pInst->TimerStunKeepAlive     = 0;
}


static  void SetNextState(TURN_INSTANCE_DATA *pInst,  TURN_STATE NextState)
{
    if  (NextState >= NoOfStates)
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> SetNextState, Illegal State %d", pInst->inst, NextState);
        return;
    }

    if (pInst->state != NextState)
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> State (%s -> %s)", pInst->inst, StateTable[pInst->state].StateStr, StateTable[NextState].StateStr);
        pInst->state = NextState;
    }

    /* always free instance and stop all timers on return to idle */
    if (NextState == TURN_STATE_Idle)
    {
        StopAllTimers(pInst);
        pInst->inUse = false;
    }
}


/* check if timer has expired and return the timer signal */
static bool TimerHasExpired(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig)
{
    int  *timer;

    switch (sig)
    {
        case TURN_SIGNAL_TimerRefreshAlloc:
            timer =  &pInst->TimerRefreshAlloc;
        break;
        case TURN_SIGNAL_TimerRefreshChannel:
            timer = &pInst->TimerRefreshChannel; 
        break;
        case TURN_SIGNAL_TimerRefreshPermission:
            timer = &pInst->TimerRefreshPermission; 
        break;
        case TURN_SIGNAL_TimerRetransmit:
            timer = &pInst->TimerRetransmit;
        break;
        case TURN_SIGNAL_TimerStunKeepAlive:
            timer = &pInst->TimerStunKeepAlive;
        break;
        default:
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> illegal timer expiry %d ", pInst->inst,  sig); 
            return false;
        break;
    }

    if (*timer) /* check timer is running */
    {
        *timer -= TimerResMsec[pInst->threadCtx];
        if (*timer <= 0)
        {
            *timer = 0;
            return true;
        }
    }
    return false;
}

/* check if timer has expired and return the timer signal */
static void StartTimer(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint32_t durationMsec)
{
    TurnPrint(pInst->threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> StartTimer(%s, %dms)", pInst->inst, TurnsigToStr(sig), durationMsec);

    switch (sig)
    {
        case TURN_SIGNAL_TimerRetransmit:
            pInst->TimerRetransmit = durationMsec;      
        break;
        case TURN_SIGNAL_TimerRefreshAlloc:
            pInst->TimerRefreshAlloc = durationMsec;
        break;
        case TURN_SIGNAL_TimerRefreshChannel:
            pInst->TimerRefreshChannel = durationMsec;
        break;
        case TURN_SIGNAL_TimerRefreshPermission:
            pInst->TimerRefreshPermission = durationMsec;
        break;
        case TURN_SIGNAL_TimerStunKeepAlive:
            pInst->TimerStunKeepAlive = durationMsec;
        break;
        default:
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> illegal StartTimer %d, duration %d", pInst->inst,  sig, durationMsec); 
        break;
    }
}

/* stop specific timer signal */
static void StopTimer(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig)
{
    TurnPrint(pInst->threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> StopTimer(%s)", pInst->inst, TurnsigToStr(sig));

    switch (sig)
    {
        case TURN_SIGNAL_TimerRetransmit:
            pInst->TimerRetransmit = 0;
        break;
        case TURN_SIGNAL_TimerRefreshAlloc:
            pInst->TimerRefreshAlloc = 0;
        break;
        case TURN_SIGNAL_TimerRefreshChannel:
            pInst->TimerRefreshChannel = 0;
        break;
        case TURN_SIGNAL_TimerRefreshPermission:
            pInst->TimerRefreshPermission = 0;
        break;
        case TURN_SIGNAL_TimerStunKeepAlive:
            pInst->TimerStunKeepAlive = 0;
        break;
        default:
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> illegal StopTimer %d", pInst->inst,  sig); 
        break;
    }
}




int  TurnClientSimulateSig(uint32_t threadCtx, int ctx, TURN_SIGNAL sig)
{
    return TurnClientMain(threadCtx, ctx, sig, NULL);
}


static int  TurnClientMain(uint32_t threadCtx, int ctx, TURN_SIGNAL sig, uint8_t *payload)
{
    int ret = TURNCLIENT_CTX_UNKNOWN;

    /* if  context is already known, just call the  fsm */
    if (ctx != TURNCLIENT_CTX_UNKNOWN)
    {
        if (ctx < CurrentInstances[threadCtx])
            TurnClientFsm(InstanceData[threadCtx][ctx], sig, payload);
        else
            TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT> sig: %s illegal context %d exceeds %d\n ", TurnsigToStr(sig), ctx, CurrentInstances[threadCtx]-1); 
        ret = ctx;
    }
    else 
    {
        /* get ptr to instance data and call the state machine */
        ctx = getTurnInst(threadCtx, &sig, payload);

        if (ctx >= 0)
        {
            TurnClientFsm(InstanceData[threadCtx][ctx], sig, payload);

            ret = ctx; /* returns context */
        }
        else
        {
            TurnPrint(threadCtx, TurnInfoCategory_Error, "<TURNCLIENT> No free instances, sig: %s", TurnsigToStr(sig));
            ret = TURNCLIENT_CTX_UNKNOWN;
        }
    }
    return ret;
}


/*************************************************************************/
/************************ FSM functions  *********************************/
/*************************************************************************/

/* clear everything except  instance, state, inUse, threadCtx, lifetime  */
static void InitInstData(TURN_INSTANCE_DATA *pInst)
{
   TURN_STATE state;
   uint32_t   inst;
   bool inUse;
   uint32_t threadCtx;
   uint32_t lifetime;

   state     = pInst->state;
   inst      = pInst->inst;
   inUse     = pInst->inUse;
   threadCtx = pInst->threadCtx;
   lifetime  = pInst->lifetime;
   memset(pInst, 0, sizeof(*pInst));
   pInst->state = state;
   pInst->inst = inst;
   pInst->inUse = inUse;
   pInst->threadCtx = threadCtx;
   pInst->lifetime = lifetime;
}



static void StoreAllocateReq(TURN_INSTANCE_DATA *pInst, TurnAllocateReqStuct *pMsgIn)
{
    /* copy whole msg */
    memcpy(&pInst->turnAllocateReq, pMsgIn, sizeof(TurnAllocateReqStuct));

    /* copy crendentials to seperate area */
    memcpy(pInst->userCredentials.stunUserName, pMsgIn->username, sizeof(pInst->userCredentials.stunUserName));
    memcpy(pInst->userCredentials.stunPassword, pMsgIn->password, sizeof(pInst->userCredentials.stunPassword));
}

static void StoreChannelBindReq(TURN_INSTANCE_DATA *pInst, TurnChannelBindInfo_T *pMsgIn)
{
    memcpy(&pInst->channelBindInfo, pMsgIn, sizeof(pInst->channelBindInfo));
}

static void StoreCreatePermReq(TURN_INSTANCE_DATA *pInst, TurnCreatePermissionInfo_T *pMsgIn)
{
    memcpy(&pInst->createPermInfo, pMsgIn, sizeof(pInst->createPermInfo));
}

static bool StoreRealm(TURN_INSTANCE_DATA *pInst, StunMessage *pResp)
{
    if (pResp->hasRealm)
    {
        memcpy(pInst->userCredentials.realm, pResp->realm.value, pResp->realm.sizeValue);  
        return true;
    }
    else
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> No REALM in message!", pInst->inst);
        return false;
    }
}

static bool StoreNonce(TURN_INSTANCE_DATA *pInst, StunMessage *pResp)
{
    if (pResp->hasNonce)
    {
        memcpy(pInst->userCredentials.nonce, pResp->nonce.value, pResp->nonce.sizeValue);  
        return true;
    }
    else
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> No NONCE in message!", pInst->inst);
        return false;
    }
}

static bool StoreRealmAndNonce(TURN_INSTANCE_DATA *pInst, StunMessage *pResp)
{
    return StoreRealm(pInst, pResp) && StoreNonce(pInst, pResp);
}


static bool StoreServerReflexiveAddress(TURN_INSTANCE_DATA *pInst, StunMessage *stunRespMsg)
{
    struct sockaddr_storage addr;

    if (stunRespMsg->hasXorMappedAddress)
    {

        if (stunRespMsg->xorMappedAddress.familyType == STUN_ADDR_IPv4Family){
            sockaddr_initFromIPv4Int((struct sockaddr_in *)&addr, 
                                     stunRespMsg->xorMappedAddress.addr.v4.addr,
                                     stunRespMsg->xorMappedAddress.addr.v4.port);
        }
        else if (stunRespMsg->xorMappedAddress.familyType == STUN_ADDR_IPv6Family){
            sockaddr_initFromIPv6Int((struct sockaddr_in6 *)&addr, 
                                     stunRespMsg->xorMappedAddress.addr.v6.addr,
                                     stunRespMsg->xorMappedAddress.addr.v6.port);
        }
        


        sockaddr_copy((struct sockaddr *)&pInst->rflxAddr,
                      (struct sockaddr *)&addr);
        
        return true;
    }
    else
    {
       TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Missing XorMappedAddress AllocResp",pInst->inst);
       return false;
    }
}


static bool StoreRelayAddressStd(TURN_INSTANCE_DATA *pInst, StunMessage *stunRespMsg)
{
    struct sockaddr_storage addr;

    if (stunRespMsg->hasXorRelayAddress)
    {

        if (stunRespMsg->xorRelayAddress.familyType == STUN_ADDR_IPv4Family){
            sockaddr_initFromIPv4Int((struct sockaddr_in *)&addr, 
                                     stunRespMsg->xorRelayAddress.addr.v4.addr,
                                     stunRespMsg->xorRelayAddress.addr.v4.port);
        }
        else if (stunRespMsg->xorRelayAddress.familyType == STUN_ADDR_IPv6Family){
            sockaddr_initFromIPv6Int((struct sockaddr_in6 *)&addr, 
                                     stunRespMsg->xorRelayAddress.addr.v6.addr,
                                     stunRespMsg->xorRelayAddress.addr.v6.port);
        }
        


        sockaddr_copy((struct sockaddr *)&pInst->relAddr,
                      (struct sockaddr *)&addr);
        
        return true;
    } 
    else 
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Missing Xor RelayAddress AllocResp",pInst->inst);
        return false;
    }
}

static bool StoreRelayAddressMs2(TURN_INSTANCE_DATA *pInst, StunMessage *stunRespMsg)
{
    struct sockaddr_storage addr;

    if (stunRespMsg->hasMappedAddress)
    {
        if (stunRespMsg->mappedAddress.familyType == STUN_ADDR_IPv4Family){
            sockaddr_initFromIPv4Int((struct sockaddr_in *)&addr, 
                                     stunRespMsg->mappedAddress.addr.v4.addr,
                                     stunRespMsg->mappedAddress.addr.v4.port);
        }
        else if (stunRespMsg->xorRelayAddress.familyType == STUN_ADDR_IPv6Family){
            sockaddr_initFromIPv6Int((struct sockaddr_in6 *)&addr, 
                                     stunRespMsg->mappedAddress.addr.v6.addr,
                                     stunRespMsg->mappedAddress.addr.v6.port);
        }
        

        
        sockaddr_copy((struct sockaddr *)&pInst->relAddr,
                      (struct sockaddr *)&addr);
        return true;
    } 
    else 
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Missing RelayAddress (MappedAddress) in AllocResp",pInst->inst);
        return false;
    }
}

static bool StoreRelayAddress(TURN_INSTANCE_DATA *pInst, StunMessage *stunRespMsg)
{
    return  isMsStun(pInst) ? StoreRelayAddressMs2(pInst, stunRespMsg) : StoreRelayAddressStd(pInst, stunRespMsg);
}

static bool StoreLifetime(TURN_INSTANCE_DATA *pInst, StunMessage *stunRespMsg)
{
    if (stunRespMsg->hasLifetime)
    {
        pInst->lifetime = stunRespMsg->lifetime.value;
        return true;
    } 
    else 
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Missing Lifetime in AllocResp",pInst->inst);
        return false;
    }
}

static bool StoreSequenceNum(TURN_INSTANCE_DATA *pInst, StunMessage *stunRespMsg)
{
    if (isMsStun(pInst) && stunRespMsg->hasSequenceNumber)
    {
        memcpy(&pInst->MsSeqNum, &stunRespMsg->SequenceNum, sizeof(pInst->MsSeqNum));
        pInst->hasMsSeqNr = true;
    }
    else
        pInst->hasMsSeqNr = false;
    return true;
}

static uint32_t GetErrCode(StunMessage *pResp)
{
    if (pResp->hasErrorCode)
        return pResp->errorCode.errorClass*100+pResp->errorCode.number;
    else 
        return 0xffffffff;
}


/*
 * The initial allocate request has no authentication info.
 * TURN server will usually reply with  Error(401-notAuth, Realm, Nonce)  
 */
static void  BuildInitialAllocateReq(TURN_INSTANCE_DATA *pInst, StunMessage  *pReq)
{
    unsigned long rndval = TurnRand();

    memset(pReq, 0, sizeof(StunMessage));
    pReq->msgHdr.msgType = STUN_MSG_AllocateRequestMsg;
    stunlib_createId(&pReq->msgHdr.id, rndval, 0);

    if (!isMsStun(pInst))
    {
        stunlib_addSoftware(pReq, SoftwareVersionStr, STUN_DFLT_PAD);
        stunlib_addRequestedTransport(pReq, STUN_REQ_TRANSPORT_UDP);
    }
    else
    {
        stunlib_addMsCookie(pReq);
        stunlib_addMsVersion(pReq, STUN_MS2_VERSION);
        /* TBD - need bandwidth, ServiceQuality ?? */
    }
}


/*
 * Initial request has been sent. TURN server has replies with  Error(401=NotAuth, Realm, Nonce).
 * Rebuild the AllocateReq as above but add Realm,Nonce,UserName. Calculate MD5Key and add it. 
 */
static void  BuildNewAllocateReq(TURN_INSTANCE_DATA *pInst, StunMessage  *pReq)
{
    unsigned long rndval = TurnRand();

    memset(pReq, 0, sizeof(StunMessage));
    pReq->msgHdr.msgType = STUN_MSG_AllocateRequestMsg;
    stunlib_createId(&pReq->msgHdr.id, rndval, 1);
    stunlib_addRealm   (pReq, pInst->userCredentials.realm, STUN_DFLT_PAD);
    stunlib_addUserName(pReq, pInst->userCredentials.stunUserName, STUN_DFLT_PAD);
    stunlib_addNonce   (pReq, pInst->userCredentials.nonce, STUN_DFLT_PAD);

    if (!isMsStun(pInst))
    {
        stunlib_addRequestedTransport(pReq, STUN_REQ_TRANSPORT_UDP);
        stunlib_addSoftware(pReq, SoftwareVersionStr, STUN_DFLT_PAD);
    }
    else
    {
        stunlib_addMsCookie(pReq);
        stunlib_addMsVersion(pReq, STUN_MS2_VERSION);
        /* TBD - need bandwidth, SequenceNumber, serviceQuality ?? */
    }

    stunlib_createMD5Key(pInst->userCredentials.key, 
                         pInst->userCredentials.stunUserName,
                         pInst->userCredentials.realm,
                         pInst->userCredentials.stunPassword);
}

static void  BuildRefreshAllocateReq(TURN_INSTANCE_DATA *pInst, StunMessage  *pReq, uint32_t lifetimeSec)
{
    unsigned long rndval = TurnRand();

    memset(pReq, 0, sizeof(StunMessage));
    stunlib_createId(&pReq->msgHdr.id, rndval, 1);
    pReq->hasLifetime = true;
    pReq->lifetime.value = lifetimeSec;
    
    stunlib_addRealm   (pReq, pInst->userCredentials.realm, STUN_DFLT_PAD);
    stunlib_addUserName(pReq, pInst->userCredentials.stunUserName, STUN_DFLT_PAD);
    stunlib_addNonce   (pReq, pInst->userCredentials.nonce, STUN_DFLT_PAD);

    if (!isMsStun(pInst))
    {
        pReq->msgHdr.msgType = STUN_MSG_RefreshRequestMsg;
        stunlib_addSoftware(pReq, SoftwareVersionStr, STUN_DFLT_PAD);
    }
    else
    {
        pReq->msgHdr.msgType = STUN_MSG_AllocateRequestMsg;
        stunlib_addMsCookie(pReq);
        stunlib_addMsVersion(pReq, STUN_MS2_VERSION);
        if (pInst->hasMsSeqNr)
        {
            pInst->MsSeqNum.sequenceNumber++;
            stunlib_addMsSeqNum(pReq, &pInst->MsSeqNum);
        }
        /* TBD - need bandwidth, serviceQuality ?? */
    }

    stunlib_createMD5Key(pInst->userCredentials.key, 
                         pInst->userCredentials.stunUserName,
                         pInst->userCredentials.realm,
                         pInst->userCredentials.stunPassword);
}



static void BuildChannelBindReq(TURN_INSTANCE_DATA *pInst, StunMessage  *pReq)
{
    StunIPAddress peerTrnspAddr;
    struct sockaddr * peerAddr = (struct sockaddr *)&pInst->channelBindInfo.peerTrnspAddr;

    memset(pReq, 0, sizeof(StunMessage));
    pReq->msgHdr.msgType = STUN_MSG_ChannelBindRequestMsg;
    stunlib_createId(&pReq->msgHdr.id, TurnRand(), 0);

    if (peerAddr->sa_family == AF_INET){
        
        peerTrnspAddr.familyType =  STUN_ADDR_IPv4Family;
        peerTrnspAddr.addr.v4.port = ((struct sockaddr_in *)peerAddr)->sin_port;
        peerTrnspAddr.addr.v4.addr = ((struct sockaddr_in *)peerAddr)->sin_addr.s_addr;

    }else if (peerAddr->sa_family == AF_INET6){
        peerTrnspAddr.familyType =  STUN_ADDR_IPv6Family;
        peerTrnspAddr.addr.v6.port = ((struct sockaddr_in6 *)peerAddr)->sin6_port;
        memcpy( peerTrnspAddr.addr.v6.addr ,
                ((struct sockaddr_in6 *)peerAddr)->sin6_addr.s6_addr,
                sizeof(peerTrnspAddr.addr.v6.addr) );
        
    }

    memcpy(&pReq->xorPeerAddress[0], &peerTrnspAddr, sizeof(StunIPAddress));
    pReq->xorPeerAddrEntries=1;

    /* Channel No */
    stunlib_addChannelNumber(pReq, (uint16_t)pInst->channelBindInfo.channelNumber); 
    stunlib_addRealm   (pReq, pInst->userCredentials.realm, STUN_DFLT_PAD);
    stunlib_addUserName(pReq, pInst->userCredentials.stunUserName, STUN_DFLT_PAD);
    stunlib_addNonce   (pReq, pInst->userCredentials.nonce, STUN_DFLT_PAD);
    stunlib_createMD5Key(pInst->userCredentials.key, 
                         pInst->userCredentials.stunUserName,
                         pInst->userCredentials.realm,
                         pInst->userCredentials.stunPassword);
}

static void BuildCreatePermReq(TURN_INSTANCE_DATA *pInst, StunMessage  *pReq)
{
    StunIPAddress peerTrnspAddr;
    uint32_t i;

    struct sockaddr *peerAddr;

    memset(pReq, 0, sizeof(StunMessage));
    pReq->msgHdr.msgType = STUN_MSG_CreatePermissionRequestMsg;
    stunlib_createId(&pReq->msgHdr.id, TurnRand(), 0);

    /* peer address(es) */
    for (i = 0; i < pInst->createPermInfo.numberOfPeers; i++)
    {
        peerAddr = (struct sockaddr *)&pInst->createPermInfo.peerTrnspAddr[i];

        if (peerAddr->sa_family == AF_INET){
            
            peerTrnspAddr.familyType =  STUN_ADDR_IPv4Family;
            peerTrnspAddr.addr.v4.port = ((struct sockaddr_in *)peerAddr)->sin_port;
            peerTrnspAddr.addr.v4.addr = ((struct sockaddr_in *)peerAddr)->sin_addr.s_addr;
            
        }else if (peerAddr->sa_family == AF_INET6){
            peerTrnspAddr.familyType =  STUN_ADDR_IPv6Family;
            peerTrnspAddr.addr.v6.port = ((struct sockaddr_in6 *)peerAddr)->sin6_port;
            memcpy( peerTrnspAddr.addr.v6.addr ,
                    ((struct sockaddr_in6 *)peerAddr)->sin6_addr.s6_addr,
                sizeof(peerTrnspAddr.addr.v6.addr) );
            
        }

        memcpy(&pReq->xorPeerAddress[i], &peerTrnspAddr, sizeof(StunIPAddress));
        pReq->xorPeerAddrEntries++;
    }

    stunlib_addRealm   (pReq, pInst->userCredentials.realm, STUN_DFLT_PAD);
    stunlib_addUserName(pReq, pInst->userCredentials.stunUserName, STUN_DFLT_PAD);
    stunlib_addNonce   (pReq, pInst->userCredentials.nonce, STUN_DFLT_PAD);
    stunlib_createMD5Key(pInst->userCredentials.key, 
                         pInst->userCredentials.stunUserName,
                         pInst->userCredentials.realm,
                         pInst->userCredentials.stunPassword);
}



/* MS-STUN only */
static void BuildSetActiveDestReq(TURN_INSTANCE_DATA *pInst, StunMessage  *pReq)
{
    StunIPAddress peerTrnspAddr;
    struct sockaddr *peerAddr;
    
    memset(pReq, 0, sizeof(StunMessage));
    pReq->msgHdr.msgType = STUN_MSG_MS2_SetActiveDestReqMsg;
    stunlib_createId(&pReq->msgHdr.id, TurnRand(), 0);
    stunlib_addMsCookie(pReq);
    stunlib_addMsVersion(pReq, STUN_MS2_VERSION);

    peerAddr = (struct sockaddr *)&pInst->channelBindInfo.peerTrnspAddr;

    if (peerAddr->sa_family == AF_INET){
            
            peerTrnspAddr.familyType =  STUN_ADDR_IPv4Family;
            peerTrnspAddr.addr.v4.port = ((struct sockaddr_in *)peerAddr)->sin_port;
            peerTrnspAddr.addr.v4.addr = ((struct sockaddr_in *)peerAddr)->sin_addr.s_addr;
            
        }else if (peerAddr->sa_family == AF_INET6){
            peerTrnspAddr.familyType =  STUN_ADDR_IPv6Family;
            peerTrnspAddr.addr.v6.port = ((struct sockaddr_in6 *)peerAddr)->sin6_port;
            memcpy( peerTrnspAddr.addr.v6.addr ,
                    ((struct sockaddr_in6 *)peerAddr)->sin6_addr.s6_addr,
                sizeof(peerTrnspAddr.addr.v6.addr) );
            
    }
    
    
    pReq->hasDestinationAddress = true;
    memcpy(&pReq->destinationAddress, &peerTrnspAddr, sizeof(StunIPAddress));

    stunlib_addRealm   (pReq, pInst->userCredentials.realm, STUN_DFLT_PAD);
    stunlib_addUserName(pReq, pInst->userCredentials.stunUserName, STUN_DFLT_PAD);
    stunlib_addNonce   (pReq, pInst->userCredentials.nonce, STUN_DFLT_PAD);

    if (pInst->hasMsSeqNr)
    {
        pInst->MsSeqNum.sequenceNumber++;
        stunlib_addMsSeqNum(pReq, &pInst->MsSeqNum);
    }

    stunlib_createMD5Key(pInst->userCredentials.key, 
                         pInst->userCredentials.stunUserName,
                         pInst->userCredentials.realm,
                         pInst->userCredentials.stunPassword);
}


/* send stun keepalive (BindingInd) to keep NAT binding open (fire and forget) */
static void SendStunKeepAlive(TURN_INSTANCE_DATA *pInst)
{
    uint32_t encLen;
    StunMsgId transId;
    uint8_t buf[STUN_MIN_PACKET_SIZE];
    encLen = stunlib_encodeStunKeepAliveReq(StunKeepAliveUsage_Ice, &transId, buf, sizeof(buf));

    TurnPrint(pInst->threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d>  OUT-->STUNKEEPALIVE: Len=%i to %s",
              pInst->inst,
              encLen, pInst->turnAllocateReq.serverAddr); 

    pInst->turnAllocateReq.sendFunc(pInst->turnAllocateReq.sockhandle,
                                    buf,
                                    encLen,
                                    (struct sockaddr *)&pInst->turnAllocateReq.serverAddr,
                                    sizeof(pInst->turnAllocateReq.serverAddr),
                                    pInst->turnAllocateReq.userCtx);
}


static bool GetServerAddrFromAltServer(TURN_INSTANCE_DATA *pInst, StunMessage *pResp)
{
    if (pResp->hasAlternateServer)
    {
        if (pResp->alternateServer.familyType == STUN_ADDR_IPv4Family)
        {
    
            sockaddr_initFromIPv4Int((struct sockaddr_in *)&pInst->turnAllocateReq.serverAddr,
                                     htonl(pResp->alternateServer.addr.v4.addr), 
                                     htons(pResp->alternateServer.addr.v4.port));

            
            return true;
        }
        else if (pResp->alternateServer.familyType == STUN_ADDR_IPv6Family){
            sockaddr_initFromIPv6Int((struct sockaddr_in6 *)&pInst->turnAllocateReq.serverAddr,
                                     pResp->alternateServer.addr.v6.addr,
                                     htons(pResp->alternateServer.addr.v6.port));
            return true;
        }

        else
        {
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Alternative Server %d not supported in AllocRespErr", pInst->inst, pResp->alternateServer.familyType);
            return false;
        }
    }
    else
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Missing Alternative Server in AllocRespErr", pInst->inst);
        return false;
    }
    return false;
}


/* Refresh response with error code  STALE_NONCE must have  NONCE and  REALM.  */
static  bool CheckRefreshRespError(TURN_INSTANCE_DATA *pInst, StunMessage *pResp)
{
    if (pResp->hasErrorCode)
    {
        if (GetErrCode(pResp) != STUN_ERROR_STALE_NONCE)
            return false;
        if (!pResp->hasRealm)
        {
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> No REALM in RefreshRespError", pInst->inst);
            return false;
        }
        if (!pResp->hasNonce)
        {
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> No NONCE in RefreshRespError", pInst->inst);
            return false;
        }
        return true;
    }
    else
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> No error code in RefreshRespError",pInst->inst);
        return false;
    }
}


static bool HandleStunAllocateResponseMsg(TURN_INSTANCE_DATA *pInst, StunMessage *pResp)
                                                   
{
    if (StoreRelayAddress(pInst, pResp) 
    && StoreServerReflexiveAddress(pInst, pResp) 
    && StoreLifetime(pInst, pResp)
    && StoreSequenceNum(pInst, pResp)) 
        return true;

    return false;
}

/* signal the result of allocation back via callback using data supplied by client */
static void  AllocateResponseCallback(TURN_INSTANCE_DATA *pInst)
{
    TurnCallBackData_T *pRes = pInst->turnAllocateReq.turnCbData;
    TurnAllocResp      *pData;

    if (pRes)
    {
        char rflxaddr[SOCKADDR_MAX_STRLEN];
        char reladdr[SOCKADDR_MAX_STRLEN];
        char activeaddr[SOCKADDR_MAX_STRLEN];

        pData = &pInst->turnAllocateReq.turnCbData->TurnResultData.AllocResp;
        pRes->turnResult = TurnResult_AllocOk;
        sockaddr_copy((struct sockaddr *)&pData->activeTurnServerAddr, 
                      (struct sockaddr *)&pInst->turnAllocateReq.serverAddr);
                      
        sockaddr_copy((struct sockaddr *)&pData->relAddr,  
                      (struct sockaddr *)&pInst->relAddr);
            
        sockaddr_copy((struct sockaddr *)&pData->rflxAddr, 
                      (struct sockaddr *)&pInst->rflxAddr);
        
        TurnPrint(pInst->threadCtx, 
                  TurnInfoCategory_Info, 
                  "<TURNCLIENT:%02d> AllocResp Relay: %s  Rflx: %s lifetime %d sec from Server %s",
                  pInst->inst, 
                  sockaddr_toString((struct sockaddr *)&pData->relAddr, reladdr,SOCKADDR_MAX_STRLEN, true),
                  sockaddr_toString((struct sockaddr *)&pData->rflxAddr, rflxaddr, SOCKADDR_MAX_STRLEN, true),
                  pInst->lifetime, 
                  sockaddr_toString((struct sockaddr *)&pData->activeTurnServerAddr, 
                                    activeaddr, SOCKADDR_MAX_STRLEN, true));
    }

    if (pInst->turnAllocateReq.turnCbFunc)
        (pInst->turnAllocateReq.turnCbFunc)(pInst->turnAllocateReq.userCtx, pInst->turnAllocateReq.turnCbData);
}

static void CallBack(TURN_INSTANCE_DATA *pInst, TurnResult_T turnResult)
{
    if (pInst->turnAllocateReq.turnCbData)
        pInst->turnAllocateReq.turnCbData->turnResult = turnResult;
    if (pInst->turnAllocateReq.turnCbFunc)
       (pInst->turnAllocateReq.turnCbFunc)(pInst->turnAllocateReq.userCtx, pInst->turnAllocateReq.turnCbData);
}

static void InitRetryCounters(TURN_INSTANCE_DATA *pInst)
{
    pInst->retransmits = 0;
}


static bool  SendTurnReq(TURN_INSTANCE_DATA *pInst, StunMessage  *stunReqMsg)
{
    int len;
    char addrStr[SOCKADDR_MAX_STRLEN];
    
    len = stunlib_encodeMessage(stunReqMsg,
                            (unsigned char*) (pInst->stunReqMsgBuf),
                            STUN_MAX_PACKET_SIZE, 
                            pInst->userCredentials.realm[0] != '\0' ? pInst->userCredentials.key : NULL,
                            pInst->userCredentials.realm[0] != '\0' ? 16 : 0,
                            false,  /* not verbose */
                            pInst->turnAllocateReq.isMsStun);
    pInst->stunReqMsgBufLen = len;

    
    if (!len )
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d>  SendTurnReq(), failed encode", pInst->inst);
        return false;
    }

    TurnPrint(pInst->threadCtx, TurnInfoCategory_Trace, 
              "<TURNCLIENT:%02d> %02x..%02x OUT-->STUN: %s sockh=%d Len=%i to %s",
              pInst->inst,
              stunReqMsg->msgHdr.id.octet[0], stunReqMsg->msgHdr.id.octet[11],
              stunlib_getMessageName(stunReqMsg->msgHdr.msgType), 
              pInst->turnAllocateReq.sockhandle,
              len, 
              sockaddr_toString((struct sockaddr *)&pInst->turnAllocateReq.serverAddr, 
                                addrStr, 
                                SOCKADDR_MAX_STRLEN, 
                                true)); 
    
    pInst->turnAllocateReq.sendFunc(pInst->turnAllocateReq.sockhandle,
                                    pInst->stunReqMsgBuf,
                                    pInst->stunReqMsgBufLen,
                                    (struct sockaddr *)&pInst->turnAllocateReq.serverAddr,
                                    sizeof(pInst->turnAllocateReq.serverAddr),
                                    pInst->turnAllocateReq.userCtx);

    /* store transaction id, so we can match the response */
    StoreReqTransId(pInst, stunReqMsg);

    return true;   
}

static void RetransmitLastReq(TURN_INSTANCE_DATA *pInst)
{
    pInst->turnAllocateReq.sendFunc(pInst->turnAllocateReq.sockhandle,
                                    pInst->stunReqMsgBuf,
                                    pInst->stunReqMsgBufLen,
                                    (struct sockaddr *)&pInst->turnAllocateReq.serverAddr,
                                    sizeof(pInst->turnAllocateReq.serverAddr),
                                    pInst->turnAllocateReq.userCtx);

}

/* api (for tuning/debugging only) */
void TurnClient_SetAllocRefreshTimerSec(uint32_t v)
{
    AllocRefreshTimerSec = v;
}

/* api (for tuning/debugging only) */
uint32_t TurnClient_GetAllocRefreshTimerSec(void)
{
    if (AllocRefreshTimerSec > 0) 
        return AllocRefreshTimerSec;
    else
        /* just use instance 0 if it exists */
        return InstanceData[0] ? InstanceData[0][0]->lifetime : 0;
}

static void StartAllocRefreshTimer(TURN_INSTANCE_DATA *pInst)
{
   if (AllocRefreshTimerSec)
       /* use "debug" value */
       StartTimer(pInst, TURN_SIGNAL_TimerRefreshAlloc, AllocRefreshTimerSec*1000);
   else
       /* Normal case - use value returned in AllocResp */
       StartTimer(pInst, TURN_SIGNAL_TimerRefreshAlloc, (pInst->lifetime/2)*1000);
}


/* api (for tuning/debugging only) */
void TurnClient_SetChanRefreshTimerSec(uint32_t v)
{
    ChanRefreshTimerSec = v;
}

/* api (for tuning/debugging only) */
uint32_t TurnClient_GetChanRefreshTimerSec(void)
{
    return  ChanRefreshTimerSec ? ChanRefreshTimerSec : TURN_REFRESH_CHANNEL_TIMER_SEC;
}


static void StartChannelBindRefreshTimer(TURN_INSTANCE_DATA *pInst)
{
    if (ChanRefreshTimerSec)
        /* use "debug" value */
        StartTimer(pInst, TURN_SIGNAL_TimerRefreshChannel, ChanRefreshTimerSec*1000);
    else
        StartTimer(pInst, TURN_SIGNAL_TimerRefreshChannel, TURN_REFRESH_CHANNEL_TIMER_SEC*1000);
}

static void StartCreatePermissionRefreshTimer(TURN_INSTANCE_DATA *pInst)
{
    StartTimer(pInst, TURN_SIGNAL_TimerRefreshPermission, TURN_REFRESH_PERMISSION_TIMER_SEC*1000);
}


/* api (for tuning/debugging only) */
void TurnClient_SetStunKeepAlive(bool onOff)
{
    DoStunKeepAlive = onOff;
}
bool TurnClient_GetStunKeepAlive(void)
{
    return DoStunKeepAlive;
}


static void StartStunKeepAliveTimer(TURN_INSTANCE_DATA *pInst)
{
    if (DoStunKeepAlive)
        StartTimer(pInst, TURN_SIGNAL_TimerStunKeepAlive, STUN_KEEPALIVE_TIMER_SEC*1000);
}


static void StartFirstRetransmitTimer(TURN_INSTANCE_DATA *pInst)
{
    pInst->retransmits = 0;
    StartTimer(pInst, TURN_SIGNAL_TimerRetransmit, pInst->turnAllocateReq.stunTimeoutList[pInst->retransmits]);
}

static void StartNextRetransmitTimer(TURN_INSTANCE_DATA *pInst)
{
    StartTimer(pInst, TURN_SIGNAL_TimerRetransmit, pInst->turnAllocateReq.stunTimeoutList[pInst->retransmits]);
}



/* Common signal handling for all states */
static void  TurnAllState(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {
        default:
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> undefned signal %s in state %d", pInst->inst, TurnsigToStr(sig), pInst->state);
    }
}

/* Common signal handling for all states after successful relay allocation */
static void  TurnAllState_Allocated(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {
        case TURN_SIGNAL_DeAllocate:
        {
            StunMessage  stunReqMsg;
            StopAllTimers(pInst);
            pInst->lifetime = 0;
            BuildRefreshAllocateReq(pInst, &stunReqMsg, pInst->lifetime);
            SendTurnReq(pInst, &stunReqMsg);
            pInst->retransmits=0;
            StartTimer(pInst, TURN_SIGNAL_TimerRetransmit, TURN_RETRANS_TIMEOUT_RELEASE_MSEC);
            SetNextState(pInst, TURN_STATE_WaitReleaseResp);
        }
        break;

        case TURN_SIGNAL_TimerStunKeepAlive:
        {
            SendStunKeepAlive(pInst);
            StartStunKeepAliveTimer(pInst);
        }
        break;

        default:
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> undefned signal %s in state %d", pInst->inst, TurnsigToStr(sig), pInst->state);
    }
}



static void  TurnClientFsm(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    if (pInst->state < TURN_STATE_End)
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Trace, 
                  "<TURNCLIENT:%02d> IN <-- %s (state %s)", 
                  pInst->inst, TurnsigToStr(sig), 
                  StateTable[pInst->state].StateStr);
        MutexLock(pInst->threadCtx, pInst->inst);
        (StateTable[pInst->state].Statefunc)(pInst, sig, payload);
        MutexUnlock(pInst->threadCtx, pInst->inst);
    }
    else
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> undefned state %d, sig %s",pInst->inst, pInst->state, TurnsigToStr(sig));
}

static void  CommonRetryTimeoutHandler(TURN_INSTANCE_DATA *pInst, TurnResult_T turnResult, const char *errStr, TURN_STATE FailedState)
{

    if ((pInst->retransmits < STUNCLIENT_MAX_RETRANSMITS)
    && (pInst->turnAllocateReq.stunTimeoutList[pInst->retransmits] != 0))  /* can be 0 terminated if using fewer retransmits */
    {
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> %02x..%02x Retransmit %s Retry: %d",
                  pInst->inst, pInst->stunReqMsgBuf[8], pInst->stunReqMsgBuf[19],  errStr, pInst->retransmits+1);
        RetransmitLastReq(pInst);
        StartNextRetransmitTimer(pInst);
        pInst->retransmits++;
        TurnStats.Retransmits++;
    }
    else
    {
        TurnStats.Failures++;
        TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Retransmit %s failed after %d retries",pInst->inst, errStr, pInst->retransmits);
        SetNextState(pInst, FailedState);
        CallBack(pInst, turnResult);
    }
}


static void  TurnState_Idle(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {
        case TURN_SIGNAL_AllocateReq:
        {
            StunMessage  stunReqMsg;
            TurnAllocateReqStuct *pMsgIn = (TurnAllocateReqStuct*)payload;

            /* clear instance data */
            InitInstData(pInst);

            /* store request */
            StoreAllocateReq(pInst,  pMsgIn);

            /* */
            BuildInitialAllocateReq(pInst, &stunReqMsg);
            
            InitRetryCounters(pInst);

            SendTurnReq(pInst, &stunReqMsg);
            
            StartFirstRetransmitTimer(pInst);
            
            SetNextState(pInst, TURN_STATE_WaitAllocRespNotAuth);
            
        }
        break;

        case TURN_SIGNAL_DeAllocate:  /* ignore extra clears */
        break;

        case TURN_SIGNAL_RefreshResp: /* may arrive after sending refresh(0) */
        break;

        default:
          TurnAllState(pInst, sig, payload);
        break;
    }

} /* TurnState_Idle() */


/*
 * initial "empty" AllocateReq has been sent, waiting for response.
 */
static void  TurnState_WaitAllocRespNotAut(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {

        case TURN_SIGNAL_AllocateRespError:
        {
            StunMessage *pResp = (StunMessage*)payload;
            uint32_t errCode;

            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);

            errCode = GetErrCode(pResp);
            switch (errCode)
            {
                case STUN_ERROR_UNAUTHORIZED:
                {
                    if (!StoreRealmAndNonce(pInst, pResp))
                    {
                        SetNextState(pInst, TURN_STATE_Idle);
                        CallBack(pInst, TurnResult_MalformedRespWaitAlloc);
                    }
                    else
                    {
                        StunMessage  stunReqMsg;
                        BuildNewAllocateReq(pInst, &stunReqMsg);
                        SendTurnReq(pInst, &stunReqMsg);
                        StartFirstRetransmitTimer(pInst);
                        SetNextState(pInst, TURN_STATE_WaitAllocResp);
                    }
                }
                break;

                case STUN_ERROR_TRY_ALTERNATE:
                {
                    /* start again, using  alternate server, stay in this state  */
                    if (GetServerAddrFromAltServer(pInst, pResp))
                    {
                        StunMessage  stunReqMsg;
                        BuildInitialAllocateReq(pInst, &stunReqMsg);
                        InitRetryCounters(pInst);
                        SendTurnReq(pInst, &stunReqMsg);
                        StartFirstRetransmitTimer(pInst);
                    }
                    else
                    {
                        SetNextState(pInst, TURN_STATE_Idle);
                        CallBack(pInst, TurnResult_MalformedRespWaitAlloc);
                    }
                }
                break;

                case NoStunErrorCode:
                {
                    TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Missing error code in AllocRespErr",pInst->inst);
                    SetNextState(pInst, TURN_STATE_Idle);
                    CallBack(pInst, TurnResult_MalformedRespWaitAlloc);
                }
                break;

                default:
                {
                    TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> Unhandled error code %d in AllocRespErr",pInst->inst, errCode);
                    SetNextState(pInst, TURN_STATE_Idle);
                    CallBack(pInst, TurnResult_MalformedRespWaitAlloc);
                }
                break;

            } /* switch on errCode */
        }   /* TURN_SIGNAL_AllocateRespError */
        break;

        case TURN_SIGNAL_AllocateResp:   /* e.g if authentication is not necessary */
        {
            StunMessage *pResp = (StunMessage*)payload;

            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);
            if (HandleStunAllocateResponseMsg(pInst, pResp))
            {
                StartAllocRefreshTimer(pInst);
                //StartStunKeepAliveTimer(pInst);
                SetNextState(pInst, TURN_STATE_Allocated);
                AllocateResponseCallback(pInst);
            }
            else
            {
                SetNextState(pInst, TURN_STATE_Idle);
                CallBack(pInst, TurnResult_MalformedRespWaitAlloc);
            }

        }
        break;


        case TURN_SIGNAL_TimerRetransmit:
        {
            CommonRetryTimeoutHandler(pInst, TurnResult_AllocFailNoAnswer, "initial allocateReq", TURN_STATE_Idle);
        }
        break;

        case TURN_SIGNAL_DeAllocate:
        {
            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);
            SetNextState(pInst, TURN_STATE_Idle);
        }
        break;

        default:
            TurnAllState(pInst, sig, payload);
            break;
    }

} /* TurnState_WaitAllocRespNotAut() */


/*
 * Second AllocateReq has been sent, waiting for response.
 */
static void  TurnState_WaitAllocResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {
        case TURN_SIGNAL_AllocateRespError:
        {
            uint32_t errCode;
            StunMessage *pResp = (StunMessage*)payload;
            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);
            errCode = GetErrCode(pResp);
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Info, 
                      "<TURNCLIENT:%02d> Authorisation failed code %d",pInst->inst, errCode);
            switch (errCode)
            {
                case STUN_ERROR_QUOTA_REACHED:
                {
                    CallBack(pInst, TurnResult_CreatePermissionQuotaReached);
                    SetNextState(pInst, TURN_STATE_Idle);
                }
                break;
             
                case STUN_ERROR_TRY_ALTERNATE:
                {
                    /* start again, using  alternate server, stay in this state  */
                    if (GetServerAddrFromAltServer(pInst, pResp))
                    {
                        StunMessage  stunReqMsg;
                        BuildNewAllocateReq(pInst, &stunReqMsg);
                        InitRetryCounters(pInst);
                        SendTurnReq(pInst, &stunReqMsg);
                        StartFirstRetransmitTimer(pInst);
                    }
                    else
                    {
                        SetNextState(pInst, TURN_STATE_Idle);
                        CallBack(pInst, TurnResult_MalformedRespWaitAlloc);
                    }
                }
                break;
                
                case STUN_ERROR_STALE_NONCE:
                {
                    StunMessage  stunReqMsg;
                    /* store new nonce and realm, recalculate and resend channel refresh */
                    TurnPrint(pInst->threadCtx, TurnInfoCategory_Info, "<TURNCLIENT:%02d> Stale Nonce %d",pInst->inst, errCode);

                    StoreRealmAndNonce(pInst, pResp); 
                    BuildNewAllocateReq(pInst, &stunReqMsg);
                    SendTurnReq(pInst, &stunReqMsg);
                    StartFirstRetransmitTimer(pInst);
                }
                break;


                default:
                    SetNextState(pInst, TURN_STATE_Idle);
                    CallBack(pInst, TurnResult_AllocUnauthorised);
                break;
            }
        }
        break;

        case TURN_SIGNAL_AllocateResp:
        {
            StunMessage *pResp = (StunMessage*)payload;

            

            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);
            if (HandleStunAllocateResponseMsg(pInst, pResp))
            {
                StartAllocRefreshTimer(pInst);
                //StartStunKeepAliveTimer(pInst);
                SetNextState(pInst, TURN_STATE_Allocated);
                AllocateResponseCallback(pInst);
                
            }
            else
            {
                SetNextState(pInst, TURN_STATE_Idle);
                CallBack(pInst, TurnResult_MalformedRespWaitAlloc);
            }
        }
        break;

        case TURN_SIGNAL_TimerRetransmit:
        {
            CommonRetryTimeoutHandler(pInst, TurnResult_AllocFailNoAnswer, "allocateReq", TURN_STATE_Idle);
        }
        break;

        case TURN_SIGNAL_DeAllocate:
        {
            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);
            SetNextState(pInst, TURN_STATE_Idle);
        }
        break;

        default:
            TurnAllState(pInst, sig, payload);
            break;
    }

} /*TurnState_WaitAllocResp () */


/* Have an allocated relay */
static void  TurnState_Allocated(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    StunMessage  stunReqMsg;

    switch (sig)
    {
        case TURN_SIGNAL_CreatePermissionReq:
        {
            uint32_t i;
            TurnCreatePermissionInfo_T *pMsgIn = (TurnCreatePermissionInfo_T *)payload;
            char addrStr[SOCKADDR_MAX_STRLEN];

            StoreCreatePermReq(pInst, pMsgIn);

            if (isMsStun(pInst))
                break; /* MsICE has no equiv. to createPermission, so ignore */

            for (i = 0; i < pMsgIn->numberOfPeers; i++)
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Info, "<TURNCLIENT:%02d> CreatePermReq Peer %s", 
                          pInst->inst, 
                          sockaddr_toString((struct sockaddr *)&pMsgIn->peerTrnspAddr[i],
                                            addrStr,
                                            SOCKADDR_MAX_STRLEN,
                                            true)); 

            BuildCreatePermReq(pInst, &stunReqMsg);
            SendTurnReq(pInst, &stunReqMsg);
            StartFirstRetransmitTimer(pInst);
            SetNextState(pInst, TURN_STATE_WaitCreatePermResp);
        }
        break;

        case TURN_SIGNAL_ChannelBindReq:
        {
            TurnChannelBindInfo_T *pMsgIn = (TurnChannelBindInfo_T *)payload;
            char addrStr[SOCKADDR_MAX_STRLEN];
            StoreChannelBindReq(pInst, pMsgIn);
            TurnPrint(pInst->threadCtx, 
                      TurnInfoCategory_Info, 
                      "<TURNCLIENT:%02d> ChannelBindReq chan: %d Peer %s", 
                      pInst->inst, 
                      pMsgIn->channelNumber, 
                      sockaddr_toString((struct sockaddr *)&pMsgIn->peerTrnspAddr,
                                            addrStr,
                                            SOCKADDR_MAX_STRLEN,
                                            true)); 

            if (isMsStun(pInst))
                BuildSetActiveDestReq(pInst, &stunReqMsg);
            else
                BuildChannelBindReq(pInst, &stunReqMsg);
            SendTurnReq(pInst, &stunReqMsg);
            StartFirstRetransmitTimer(pInst);
            SetNextState(pInst, isMsStun(pInst) ? TURN_STATE_WaitSetActiveDestResp : TURN_STATE_WaitChanBindResp);
        }
        break;

        case TURN_SIGNAL_TimerRefreshAlloc:
        {
            /* build and send an allocation refresh */
            BuildRefreshAllocateReq(pInst, &stunReqMsg, pInst->lifetime);
            SendTurnReq(pInst, &stunReqMsg);
            StartFirstRetransmitTimer(pInst);
            SetNextState(pInst, TURN_STATE_WaitAllocRefreshResp);
        }
        break;

        case TURN_SIGNAL_TimerRefreshChannel:
        {
            /* build and send a channel bind refresh */
            BuildChannelBindReq(pInst, &stunReqMsg);
            SendTurnReq(pInst, &stunReqMsg);
            StartFirstRetransmitTimer(pInst);
            SetNextState(pInst, isMsStun(pInst) ? TURN_STATE_WaitSetActiveDestResp : TURN_STATE_WaitChanBindResp);
        }
        break;

        case TURN_SIGNAL_TimerRefreshPermission:
        {
            /* no need to refresh permissions if channel is bound
             * coz a channel bind creates a permission
             */
            if (!pInst->channelBound)
            {
                /* build and send a Permission refresh */
                BuildCreatePermReq(pInst, &stunReqMsg);
                SendTurnReq(pInst, &stunReqMsg);
                StartFirstRetransmitTimer(pInst);
                SetNextState(pInst, TURN_STATE_WaitCreatePermResp);
            }
            else
            {    
                pInst->permissionsInstalled = false;
            }

        }
        break;


        default:
            TurnAllState_Allocated(pInst, sig, payload);
            break;
    }

} /* TurnState_AllocatedChan() */


/*
 * (Allocation) Refresh sent, waiting for response. 
 */
static void  TurnState_WaitAllocRefreshResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    StunMessage  stunReqMsg;

    switch (sig)
    {
        case TURN_SIGNAL_RefreshResp:
        {
            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);
            StartAllocRefreshTimer(pInst);
            SetNextState(pInst, TURN_STATE_Allocated);
        }
        break;

        case TURN_SIGNAL_RefreshRespError:
        {
            StunMessage *pResp = (StunMessage*)payload;

            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);

            if (CheckRefreshRespError(pInst, pResp))
            {
                StartFirstRetransmitTimer(pInst);    /* store new nonce and realm, recalculate and resend channel refresh */
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Info, "<TURNCLIENT:%02d> Stale Nonce %d",pInst->inst, GetErrCode(pResp));

                StoreRealmAndNonce(pInst, pResp); 
                BuildRefreshAllocateReq(pInst, &stunReqMsg, pInst->lifetime);
                SendTurnReq(pInst, &stunReqMsg);
                StartFirstRetransmitTimer(pInst);
            }
            else
            {
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Info, "<TURNCLIENT:%02d> Refresh failed code %d",pInst->inst, GetErrCode(pResp));
                StopAllTimers(pInst);
                SetNextState(pInst, TURN_STATE_Idle);
                CallBack(pInst, TurnResult_RefreshFail);
            }
        }
        break;


        case TURN_SIGNAL_TimerRetransmit:
        {
            CommonRetryTimeoutHandler(pInst, TurnResult_RefreshFailNoAnswer, "refreshReqChanId", TURN_STATE_Idle);
        }
        break;

        case TURN_SIGNAL_TimerRefreshChannel:
        {
            /* just delay it til later, were busy with Allocation refresh  */
            StartTimer(pInst, TURN_SIGNAL_TimerRefreshChannel, 2*1000);
        }
        break;

        case TURN_SIGNAL_TimerRefreshPermission:
        {
            /* just delay it til later, we're busy */
            StartTimer(pInst, TURN_SIGNAL_TimerRefreshPermission, 3*1000);
        }
        break;


        default:
            TurnAllState_Allocated(pInst, sig, payload);
        break;

    }

} /* TurnState_WaitAllocRefreshResp() */



/*
 * ChannelBindReq has been sent. Waiting for  response. 
 */
static void  TurnState_WaitChanBindResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {
        case TURN_SIGNAL_ChannelBindResp:
        {
            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);

            StartChannelBindRefreshTimer(pInst);
            SetNextState(pInst,TURN_STATE_Allocated);

            /* only do the callback on initial success, and not for every refresh  */
            if (!pInst->channelBound)
            {
                pInst->channelBound = true;
                CallBack(pInst, TurnResult_ChanBindOk);
            }
        }
        break;

        case TURN_SIGNAL_ChannelBindRespError:
        {
            StunMessage *pResp = (StunMessage*)payload;
            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);

            if (CheckRefreshRespError(pInst, pResp))
            {
                StunMessage  stunReqMsg;

                /* store new nonce and realm, recalculate and resend channel refresh */
                StartFirstRetransmitTimer(pInst);    
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Info, "<TURNCLIENT:%02d> ChannelBind Refresh Stale Nonce %d",pInst->inst, GetErrCode(pResp));
                StoreRealmAndNonce(pInst, pResp); 
                BuildChannelBindReq(pInst, &stunReqMsg);
                SendTurnReq(pInst, &stunReqMsg);
                StartFirstRetransmitTimer(pInst);
            }
            else
            {
                pInst->channelBound = false;
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d>  ChannelBind Refresh got ErrorCode %d",pInst->inst, GetErrCode((StunMessage*)payload));
                SetNextState(pInst,TURN_STATE_Allocated);
                CallBack(pInst, TurnResult_ChanBindFail);
            }
        }
        break;

        case TURN_SIGNAL_TimerRetransmit:
        {
            CommonRetryTimeoutHandler(pInst, TurnResult_ChanBindFailNoanswer, "channelBindReq", TURN_STATE_Allocated);
        }
        break;

        case TURN_SIGNAL_TimerRefreshAlloc:
        {
            /* just delay it til later, we're busy */
            StartTimer(pInst, TURN_SIGNAL_TimerRefreshAlloc, 3*1000);
        }
        break;

        case TURN_SIGNAL_TimerRefreshPermission:
        {
            /* just delay it til later, we're busy */
            StartTimer(pInst, TURN_SIGNAL_TimerRefreshPermission, 2*1000);
        }
        break;

        default:
            TurnAllState_Allocated(pInst, sig, payload);
            break;
    }

} /* TurnState_WaitChanBindResp() */


/*
 * CreatePermissionReq has been sent. Waiting for  response. 
 */
static void  TurnState_WaitCreatePermResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {
        case TURN_SIGNAL_CreatePermissionResp:
        {
            StopTimer(pInst,   TURN_SIGNAL_TimerRetransmit);

            StartCreatePermissionRefreshTimer(pInst);
            SetNextState(pInst,TURN_STATE_Allocated);

            /* only do the callback on initial success, and not for every refresh  */
            if (!pInst->permissionsInstalled)
            {
                pInst->permissionsInstalled = true;
                CallBack(pInst, TurnResult_CreatePermissionOk);
            }
        }
        break;

        case TURN_SIGNAL_CreatePermissionRespError:
        {
            StunMessage *pResp = (StunMessage*)payload;
            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);
            pInst->permissionsInstalled = false;

            if (CheckRefreshRespError(pInst, pResp))
            {
                StunMessage  stunReqMsg;

                /* store new nonce and realm, recalculate and resend CreatePermissionReq */
                StartFirstRetransmitTimer(pInst);    
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Info, "<TURNCLIENT:%02d> Stale Nonce %d",pInst->inst, GetErrCode(pResp));
                StoreRealmAndNonce(pInst, pResp); 
                BuildCreatePermReq(pInst, &stunReqMsg);
                SendTurnReq(pInst, &stunReqMsg);
                StartFirstRetransmitTimer(pInst);
            }
            else
            {
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> WaitCreatePermResp got ErrorCode %d",pInst->inst, GetErrCode((StunMessage*)payload));
                SetNextState(pInst,TURN_STATE_Allocated);
                CallBack(pInst, TurnResult_PermissionRefreshFail);
            }
        }
        break;



        case TURN_SIGNAL_TimerRetransmit:
        {
           CommonRetryTimeoutHandler(pInst, TurnResult_CreatePermissionNoAnswer, "createPermissionReq", TURN_STATE_Allocated);
        }
        break;

        case TURN_SIGNAL_TimerRefreshAlloc:
        {
            /* just delay it til later, we're busy */
            StartTimer(pInst, TURN_SIGNAL_TimerRefreshAlloc, 3*1000);
        }
        break;

        case TURN_SIGNAL_TimerRefreshChannel:
        {
            /* just delay it til later, we're busy */
            StartTimer(pInst, TURN_SIGNAL_TimerRefreshChannel, 2*1000);
        }
        break;

        default:
            TurnAllState_Allocated(pInst, sig, payload);
            break;
    }

} /* TurnState_WaitCreatePermResp() */



/* MSSTUN: Sent SetActiveDestinationReq, waiting for Resp */
static void  TurnState_WaitSetActiveDestResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {
        /* MSSTUN only, equivalent to Channel BindingResp */
        case TURN_SIGNAL_SetActiveDestResp:
        {
            StopTimer(pInst,   TURN_SIGNAL_TimerRetransmit);
            SetNextState(pInst,TURN_STATE_Allocated);
            CallBack(pInst, TurnResult_SetActiveDestOk);
        }
        break;

        case TURN_SIGNAL_SetActiveDestRespError:
        {
            StopTimer(pInst,   TURN_SIGNAL_TimerRetransmit);
            TurnPrint(pInst->threadCtx, TurnInfoCategory_Error, "<TURNCLIENT:%02d> SetActiveDestRespError ErrorCode %d", pInst->inst, GetErrCode((StunMessage*)payload));
            SetNextState(pInst,TURN_STATE_Allocated);
            CallBack(pInst, TurnResult_SetActiveDestFail);
        }
        break;

        case TURN_SIGNAL_TimerRetransmit:
        {
            CommonRetryTimeoutHandler(pInst, TurnResult_SetActiveDestFailNoAnswer, "SetActiveDestReq", TURN_STATE_Allocated);
        }
        break;

        case TURN_SIGNAL_TimerRefreshAlloc:
        {
            /* just delay it til later, we're busy */
            StartTimer(pInst, TURN_SIGNAL_TimerRefreshAlloc, 2*1000);
        }
        break;

        default:
            TurnAllState_Allocated(pInst, sig, payload);
            break;


    }
} /* TurnState_WaitSetActiveDestResp */


/* Refresh(0) sent. Waiting for  Resp. 
 * Note that in many cases (unfortunately) the socket gets closed by the application after sending the Refresh(0).
 * This means that ResfreshResp will not be received. So hence use a relatively short timer to get back to idle quick
 * and not lock up resources. 
 */
static void  TurnState_WaitReleaseResp(TURN_INSTANCE_DATA *pInst, TURN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {
        case TURN_SIGNAL_RefreshResp:
        {
            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);
            SetNextState(pInst, TURN_STATE_Idle);
            CallBack(pInst, TurnResult_RelayReleaseComplete); 
        }
        break;

        /* have to handle stale nonce, otherwise the relay will not be released ! */
        case TURN_SIGNAL_RefreshRespError:
        {
            StunMessage *pResp = (StunMessage*)payload;

            StopTimer(pInst, TURN_SIGNAL_TimerRetransmit);

            if (CheckRefreshRespError(pInst, pResp))
            {
                StunMessage  stunReqMsg;

                /* store new nonce and realm, recalculate and resend channel refresh */
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Info, "<TURNCLIENT:%02d> Stale Nonce %d",pInst->inst, GetErrCode(pResp));

                StoreRealmAndNonce(pInst, pResp); 
                BuildRefreshAllocateReq(pInst, &stunReqMsg, pInst->lifetime);
                SendTurnReq(pInst, &stunReqMsg);
                pInst->retransmits=0;
                StartTimer(pInst, TURN_SIGNAL_TimerRetransmit, TURN_RETRANS_TIMEOUT_RELEASE_MSEC);
            }
            else
            {
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Info, "<TURNCLIENT:%02d> Refresh failed code %d",pInst->inst, GetErrCode(pResp));
                SetNextState(pInst, TURN_STATE_Idle);
                CallBack(pInst, TurnResult_RelayReleaseFailed); 
            }
        }
        break;

        case TURN_SIGNAL_TimerRetransmit:
        {
            if (pInst->retransmits < TURN_RETRIES_RELEASE)
            {
                TurnPrint(pInst->threadCtx, TurnInfoCategory_Trace, "<TURNCLIENT:%02d> %02x..%02x Retransmit Refresh(0) Retry: %d",
                          pInst->inst, pInst->stunReqMsgBuf[8], pInst->stunReqMsgBuf[19], pInst->retransmits+1);
                RetransmitLastReq(pInst);
                pInst->retransmits++;
                TurnStats.Retransmits++;
                StartTimer(pInst, TURN_SIGNAL_TimerRetransmit, TURN_RETRANS_TIMEOUT_RELEASE_MSEC);
            }
            else
            {   
                SetNextState(pInst, TURN_STATE_Idle);
                CallBack(pInst, TurnResult_RelayReleaseFailed); 
            }
        }
        break;

        /* ignore duplicate disconnects */
        case TURN_SIGNAL_DeAllocate:
        break;

        default:
            TurnAllState(pInst, sig, payload);
            break;
    }
} /* TurnState_WaitReleaseResp() */



