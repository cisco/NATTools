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



/*************************************************************************************
 * Description   :
 * implemented as an OS independent state machine.
 * Initialisaiton:
 *      Application calls StunClientBind_init().
 *
 * Entrypoints:
 *      1. Application calls StunClientBind_startxxxxx() to initiate the stun/ice protocol sequence. 
 *      2. StunClientBind_HandleTick() must be called by appl. every N msec such that it can carry out timing/retransmissions.
 *      3. Application calls StunClientBind_HandleIncResp() when it detects incoming stun responses in the media RTP/RTCP stream.
 *      
 * Outputs:
 *      1. Application provides function pointer and  data ptr to receive the result of the  stun/ice protocol.         
 *
 ******************************************************************************/



#include "stunclient.h"
#include "stun_os.h"
#include "stun_intern.h"

#include "turnclient.h"

#include "sockaddr_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* instance data */
static STUN_INSTANCE_DATA **InstanceData[STUN_MAX_THREAD_CTX];

static int                  CurrentInstances[STUN_MAX_THREAD_CTX];
static int                  MaxInstances[STUN_MAX_THREAD_CTX];
static uint32_t             TimerResMsec[STUN_MAX_THREAD_CTX];      /* how often this is called */
static STUN_INFO_FUNC_PTR   StunErrFuncPtr[STUN_MAX_THREAD_CTX];    /* function called to output management info */
static bool                 MutexLockRequired[STUN_MAX_THREAD_CTX]; /* true if called from > 1 thread */
static STUN_MUTEX           *InstanceMutex[STUN_MAX_THREAD_CTX];

/* use when no timeout list is provided */
static uint32_t StunDefaultTimeoutList[STUNCLIENT_MAX_RETRANSMITS] = { STUNCLIENT_RETRANSMIT_TIMEOUT_LIST};

//static uint32_t StunDefaultTimeoutList[STUNCLIENT_MAX_RETRANSMITS] = {1000, 1000, 1000, 0};    test  

static char  SoftwareVersionStr[100];

/* management */
static StunClientStats_T StunClientStats;

/* forward declarations of statics */
static int   StunClientMain(uint32_t threadCtx, int ctx, STUN_SIGNAL sig, uint8_t *payload);
static void  StunClientFsm(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload);
static void  SetNextState(STUN_INSTANCE_DATA *pInst,  STUN_STATE NextState);
static void  StunPrint(uint32_t threadCtx, StunInfoCategory_T category, const char *fmt, ...);

/* forward declarations of state functions */
static void  StunState_Idle(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload);
static void  StunState_WaitBindResp(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload);
static void  StunState_Cancelled(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload);
static bool  TimerHasExpired(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig);



/*************************************************************************/
/************************ UTILS*******************************************/
/*************************************************************************/
static void  MutexCreate(uint32_t threadCtx, int inst)
{
    char s[20];

    if (!MutexLockRequired[threadCtx]) return;
    sprintf(s, "STNC%d%d", threadCtx, inst+1); 
    if (Stun_MutexCreate(&InstanceMutex[threadCtx][inst+1], s))
        StunPrint(threadCtx, StunInfoCategory_Trace, "<StunCLIENT:%02d> Stun_MutexCreate(%s) OK", inst, s);
    else
        StunPrint(threadCtx, StunInfoCategory_Error, "<StunCLIENT:%02d> Stun_MutexCreate(%s) failed", inst, s);
}

static void  MutexLock(uint32_t threadCtx, int inst)
{
    if (!MutexLockRequired[threadCtx]) return;
    if (Stun_MutexLock(&InstanceMutex[threadCtx][inst+1]))
        StunPrint(threadCtx, StunInfoCategory_Trace, "<StunCLIENT:%02d> Stun_MutexLock() OK", inst);
    else
        StunPrint(threadCtx, StunInfoCategory_Error, "<StunCLIENT:%02d> Stun_MutexLock() failed", inst);
}

static void  MutexUnlock(uint32_t threadCtx, int inst)
{
    if (!MutexLockRequired[threadCtx]) return;
    if (Stun_MutexUnlock(&InstanceMutex[threadCtx][inst+1]))
        StunPrint(threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> Stun_MutexLUnlock() OK", inst);
    else
        StunPrint(threadCtx, StunInfoCategory_Error, "<STUNCLIENT:%02d> Stun_MutexUnlock() failed", inst);
}


/*
 * Called when an internal STUNCLIENT wants to output managemnt info. 
 * Prints the string to a buffer.
 * If the application has defined a callback function to handle the output
 * then this is called with the string and the severity.
 */
static void StunPrint(uint32_t threadCtx, StunInfoCategory_T category, const char *fmt, ...)
{
    char s[STUN_MAX_ERR_STRLEN];
    va_list ap;
    va_start(ap,fmt);
    
    /* print string to buffer  */
    vsprintf(s, fmt, ap);
    
    if (StunErrFuncPtr[threadCtx])
      /* Call the application defined "error callback function" */
      (StunErrFuncPtr[threadCtx])(category, s);
    
    va_end(ap);
}

/* debug,trace */
static STUN_SIGNAL StunMsgToInternalStunSig(uint32_t threadCtx, StunMessage *msg)
{
    switch (msg->msgHdr.msgType)
    {
        case STUN_MSG_BindRequestMsg:         return STUN_SIGNAL_BindReq;
        case STUN_MSG_BindResponseMsg:        return STUN_SIGNAL_BindResp;
        case STUN_MSG_BindErrorResponseMsg:   return STUN_SIGNAL_BindRespError;
        default:
            /* Some other message */
            StunPrint(threadCtx, StunInfoCategory_Error, "<STUNCLIENT> unknown STUN message type (0x%02x)", msg->msgHdr.msgType);
            return STUN_SIGNAL_Illegal;
            break;
    }
}


/* transaction id compare */
static bool TransIdIsEqual(const StunMsgId *a, const StunMsgId *b)
{
    return (memcmp(a, b, STUN_MSG_ID_SIZE) == 0);
}


/*************************************************************************/
/************************ API ********************************************/
/*************************************************************************/


bool StunClient_Init(uint32_t threadCtx, int instances, uint32_t tMsec, STUN_INFO_FUNC_PTR InfoFuncPtr, bool MultipleThreadAccess, char *SwVerStr)
{
    if (threadCtx >= STUN_MAX_THREAD_CTX)
    {
        StunPrint(0, StunInfoCategory_Error, "\n<STUNCLIENT> Init() threadCtx %d too big, max is %d\n", threadCtx, STUN_MAX_THREAD_CTX-1);
        return false;
    }

    MutexLockRequired[threadCtx] = MultipleThreadAccess;

    if (InstanceData[threadCtx])
      return true;          /* already initialised */

    InstanceMutex[threadCtx] = calloc(instances+1, sizeof(STUN_MUTEX));
    if (InstanceMutex[threadCtx] == NULL) return false;

    MutexCreate(threadCtx, -1); /* global mutex */

    strncpy(SoftwareVersionStr, SwVerStr, sizeof(SoftwareVersionStr)-1);

    StunErrFuncPtr[threadCtx] = InfoFuncPtr;
    TimerResMsec[threadCtx]   = tMsec;

    InstanceData[threadCtx] = calloc(instances, sizeof(STUN_INSTANCE_DATA*));
    if (InstanceData[threadCtx] == NULL) return false;

    CurrentInstances[threadCtx] = 0;
    MaxInstances[threadCtx]     = instances;
    StunPrint(threadCtx, StunInfoCategory_Trace, "\n<STUNCLIENT> startup, threadCtx %d MaxInstances %d CurrentInstances %d, tick %d msec MultiThread %d", 
              threadCtx, MaxInstances[threadCtx], CurrentInstances[threadCtx], tMsec, MultipleThreadAccess); 
    return true;
}

void StunClient_Destruct(uint32_t threadCtx)
{
  if ( InstanceData[threadCtx])
  {
    free (InstanceData[threadCtx]);
    InstanceData[threadCtx]= NULL;
  }
  if (InstanceMutex[threadCtx])
  {
    free (InstanceMutex[threadCtx]);
    InstanceMutex[threadCtx] = NULL;
  }
}

/* Create a new instance */
static int StunClientConstruct(uint32_t threadCtx)
{
    STUN_INSTANCE_DATA *p;
    int  *curr;

    curr = &CurrentInstances[threadCtx];
    if (*curr >= MaxInstances[threadCtx])
    {
        StunPrint(threadCtx, StunInfoCategory_Error, "\n<STUNCLIENT> Failed to Create thread %d inst %d, maximum instances %d exceeded  !!\n", 
                  threadCtx, *curr+1, MaxInstances[threadCtx]);
        return STUNCLIENT_CTX_UNKNOWN;
    }

    p =  (STUN_INSTANCE_DATA*)calloc(1, sizeof(STUN_INSTANCE_DATA));
    if (p)
    {
        MutexCreate(threadCtx, CurrentInstances[threadCtx]);
        MutexLock(threadCtx, CurrentInstances[threadCtx]);
        InstanceData[threadCtx][*curr] = p;
        InstanceData[threadCtx][*curr]->inst = *curr;
        InstanceData[threadCtx][*curr]->state = STUN_STATE_Idle;
        InstanceData[threadCtx][*curr]->inUse = true;
        InstanceData[threadCtx][*curr]->threadCtx = threadCtx;
        StunPrint(threadCtx, StunInfoCategory_Trace, "\n<STUNCLIENT> Create inst[%d].threadCtx %d\n", *curr, 
                  InstanceData[threadCtx][*curr]->threadCtx);
        *curr = *curr+1;
        MutexUnlock(threadCtx, CurrentInstances[threadCtx]-1);

        return *curr-1;
    }
    else
    {
        StunPrint(threadCtx, StunInfoCategory_Error, "\n<STUNCLIENT> Failed to Create thread %d inst %d, no memory !!\n", threadCtx, *curr);
        return STUNCLIENT_CTX_UNKNOWN;
    }
}


void StunClient_HandleTick(uint32_t threadCtx)
{
   /* call fsm for each timer that has expired */
    int i;

    if ((threadCtx >= STUN_MAX_THREAD_CTX) ||  (InstanceData[threadCtx] == NULL))
    {
        StunPrint(0, StunInfoCategory_Error, "<STUNCLIENT> HandleTick() failed,  Not initialised or no memory, threadCtx %d", threadCtx);
        return;
    }

    for (i=0; i < CurrentInstances[threadCtx]; i++)
    {
        STUN_INSTANCE_DATA *pInst;
        pInst = InstanceData[threadCtx][i];
        if (TimerHasExpired(pInst, STUN_SIGNAL_TimerRetransmit))
            StunClientFsm  (pInst, STUN_SIGNAL_TimerRetransmit, NULL);
    }
}


int  StunClient_startBindTransaction(uint32_t            threadCtx,
                                     void               *userCtx,
                                     struct sockaddr    *serverAddr,
                                     struct sockaddr    *baseAddr,
                                     bool                useRelay,
                                     char               *ufrag,
                                     char               *password,
                                     uint32_t            peerPriority,
                                     bool                useCandidate,
                                     bool                iceControlling,
                                     uint64_t            tieBreaker,
                                     StunMsgId           transactionId,
                                     uint32_t            sockhandle,
                                     STUN_SENDFUNC       sendFunc,
                                     uint32_t           *timeoutList,
                                     STUNCB              stunCbFunc,
                                     StunCallBackData_T *stunCbData,
                                     int                 turnInst)
{

    StunBindReqStuct m;
    uint32_t i;
    uint32_t *pTimeoutList;
    int ret;

    if ((threadCtx >= STUN_MAX_THREAD_CTX) ||  (InstanceData[threadCtx] == NULL))
    {
        StunPrint(0, StunInfoCategory_Error, "<STUNCLIENT> startBindTransaction() failed,  Not initialised or no memory, threadCtx %d", threadCtx);
        return STUNCLIENT_CTX_UNKNOWN;
    }
    memset(&m, 0, sizeof(m));
    m.threadCtx = threadCtx;
    m.userCtx        = userCtx;
    sockaddr_copy((struct sockaddr *)&m.serverAddr, serverAddr);
    sockaddr_copy((struct sockaddr *)&m.baseAddr, baseAddr);
    strncpy(m.ufrag, ufrag, sizeof(m.ufrag));
    strncpy(m.password,   password, sizeof(m.password));
    m.useRelay = useRelay;
    m.peerPriority = peerPriority;
    m.useCandidate = useCandidate;
    m.iceControlling = iceControlling;
    m.tieBreaker = tieBreaker;
    m.transactionId = transactionId;
    m.sockhandle     = sockhandle;
    m.sendFunc       = sendFunc;
    m.turnInst = turnInst;

    /* use default timeout list if none provided */
    pTimeoutList = (timeoutList == NULL ? StunDefaultTimeoutList : timeoutList);

    for (i=0; i < sizeof(m.stunTimeoutList)/sizeof(m.stunTimeoutList[0]); i++)
    {
        if (pTimeoutList[i])
            m.stunTimeoutList[i] = pTimeoutList[i];
        else
            break; /* 0 terminated list */
    }

    /* callback and  data (owned by caller) */
    m.stunCbFunc = stunCbFunc;
    m.stunCbData = stunCbData;
    ret = StunClientMain(threadCtx, STUNCLIENT_CTX_UNKNOWN, STUN_SIGNAL_BindReq, (uint8_t*)&m);
    return ret;
}


int  StunClient_HandleIncResp(uint32_t threadCtx, StunMessage *msg, struct sockaddr *srcAddr)
{
    int i;

    if ((threadCtx >= STUN_MAX_THREAD_CTX) ||  (InstanceData[threadCtx] == NULL))
    {
        StunPrint(0, StunInfoCategory_Error, "<STUNCLIENT> HandleIncResp() failed,  Not initialised or no memory, threadCtx %d", threadCtx);
        return STUNCLIENT_CTX_UNKNOWN;
    }

    for (i=0; i < CurrentInstances[threadCtx]; i++)
    {
        if (TransIdIsEqual(&msg->msgHdr.id, &InstanceData[threadCtx][i]->stunBindReq.transactionId))
        {
            StunRespStruct m;
            memcpy(&m.stunRespMessage, msg, sizeof(m.stunRespMessage));
            sockaddr_copy((struct sockaddr *)&m.srcAddr, srcAddr);
            StunClientMain(threadCtx,i, StunMsgToInternalStunSig(threadCtx, msg), (void*)&m);
            return i;
        }
    }
    StunPrint(threadCtx, StunInfoCategory_Trace, "<STUNCLIENT> no instance with transId, discarding, msgType %d\n ", msg->msgHdr.msgType);
    return STUNCLIENT_CTX_UNKNOWN;
}


void StunClient_Deallocate(uint32_t threadCtx, int ctx)
{
    if ((threadCtx >= STUN_MAX_THREAD_CTX) ||  (InstanceData[threadCtx] == NULL))
    {
        StunPrint(0, StunInfoCategory_Error, "<STUNCLIENT> Deallocate() failed,  Not initialised or no memory, threadCtx %d", threadCtx);
        return;
    }

    StunClientMain(threadCtx, ctx, STUN_SIGNAL_DeAllocate, NULL);
}

/*  
 * Cancel a transaction with  matching  transaction id
 *      transactionId  - Transaction id.
 * return -  if  transaction found returns ctx/instance
 *        -  if  no instance found with transactionid, returns  STUNCLIENT_CTX_UNKNOWN
 */
int StunClient_cancelBindingTransaction(uint32_t threadCtx, StunMsgId transactionId)
{
    int i;

    if ((threadCtx >= STUN_MAX_THREAD_CTX) ||  (InstanceData[threadCtx] == NULL))
    {
        StunPrint(0, StunInfoCategory_Error, "<STUNCLIENT> cancelBindingTransaction() failed,  Not initialised or no memory, threadCtx %d", threadCtx);
        return STUNCLIENT_CTX_UNKNOWN;
    }

    for (i = 0; i < CurrentInstances[threadCtx]; i++)
    {
        if (TransIdIsEqual(&transactionId, &InstanceData[threadCtx][i]->stunBindReq.transactionId))
        {
            StunClientMain(threadCtx, i, STUN_SIGNAL_Cancel, NULL);
            return i;
        }
    }
    return STUNCLIENT_CTX_UNKNOWN;
}

static bool CreateConnectivityBindingResp(uint32_t threadCtx,
                                          StunMessage *stunMsg,
                                          StunMsgId transactionId,
                                          char *userName,
                                          struct sockaddr *mappedSockAddr)
{
    StunIPAddress mappedAddr;


    if (!sockaddr_isSet((struct sockaddr *)mappedSockAddr)){
        return false;
    }

    memset(stunMsg, 0, sizeof(StunMessage));
    stunMsg->msgHdr.msgType = STUN_MSG_BindResponseMsg;
    
    if (mappedSockAddr->sa_family == AF_INET){

        mappedAddr.familyType =  STUN_ADDR_IPv4Family;
        mappedAddr.addr.v4.port = ntohs (((struct sockaddr_in *)mappedSockAddr)->sin_port);
        mappedAddr.addr.v4.addr = ntohl (((struct sockaddr_in *)mappedSockAddr)->sin_addr.s_addr);

    }else if (mappedSockAddr->sa_family == AF_INET6){
        mappedAddr.familyType =  STUN_ADDR_IPv6Family;
        mappedAddr.addr.v6.port = ntohs (((struct sockaddr_in6 *)mappedSockAddr)->sin6_port);

        /*TODO: will this be correct ? */
        memcpy( mappedAddr.addr.v6.addr ,
                ((struct sockaddr_in6 *)mappedSockAddr)->sin6_addr.s6_addr,
                sizeof(mappedAddr.addr.v6.addr) );
        
    }else{

        return false;
    }

    /*id*/
    stunMsg->msgHdr.id = transactionId;

    //The XOR addres MUST be added acording to the RFC
    stunMsg->hasXorMappedAddress = true;
    stunMsg->xorMappedAddress = mappedAddr;

    /* Username */
    stunlib_addUserName(stunMsg, userName, STUN_DFLT_PAD);

    return true;
}


/* managemnet/statistics */
void StunClientGetStats(StunClientStats_T *Stats)
{
    int i,j;
    StunClientStats.InProgress = 0;
    for (i=0; i < STUN_MAX_THREAD_CTX; i++)
    {
       for (j=0; j < CurrentInstances[i]; j++)
       {
           if (InstanceData[i][j]->state != STUN_STATE_Idle) 
               StunClientStats.InProgress++;
       }
    }
    memcpy(Stats, &StunClientStats, sizeof(StunClientStats)); 
}

/* management/statistics */
void StunClientClearStats(void)
{
    memset(&StunClientStats, 0, sizeof(StunClientStats)); 
}



static bool SendConnectivityBindResponse(uint32_t         threadCtx, 
                                         int32_t          globalSocketId,
                                         StunMessage     *stunRespMsg, 
                                         char            *password,
                                         struct sockaddr *dstAddr,
                                         socklen_t       t,
                                         void            *userData,
                                         STUN_SENDFUNC    sendFunc,
                                         bool             useRelay,
                                         int              turnInst)
{
    uint8_t stunBuff[STUN_MAX_PACKET_SIZE];
    int stunLen;

    /* encode bind Response */
    stunLen = stunlib_encodeMessage(stunRespMsg,
                                    (uint8_t*)stunBuff,
                                    STUN_MAX_PACKET_SIZE, 
                                    (unsigned char*)password, /* md5key */
                                    strlen(password),    /* keyLen */
                                    false,
                                    false);
    if (!stunLen )
    {
        StunPrint(threadCtx, StunInfoCategory_Error, "<STUNCLIENT>  Failed to encode Binding request response\n");
        return false;
    }



    if (useRelay && (turnInst >= 0))
    {
        /* tunnel the BindResp in a Turn SendInd */
        uint8_t TxBuff[STUN_MAX_PACKET_SIZE];
        struct sockaddr_storage TurnServerAddr;
        char turnservaddrStr[SOCKADDR_MAX_STRLEN];
        int turnLen;

        /* encode a turn SendIndication, put the BindReq in the data attribute */
        turnLen = TurnClient_createSendIndication(TxBuff,                   /* dest=encoded sendInd */
                                              (uint8_t*)stunBuff,           /* DataAttr=encoded BindReq */
                                              STUN_MAX_PACKET_SIZE,
                                              stunLen,                      /* len of encoded DataAttr (BindReq) */
                                              dstAddr,                      /* peer address */  
                                              false,                        /* no  MsStun yet TBD ..*/
                                              RTP_STUN_THREAD_CTX,          /* no  MsStun yet TBD ..*/
                                              turnInst);                    /* turninst, only msStun */

        TurnClientGetActiveTurnServerAddr( RTP_STUN_THREAD_CTX, 
                                           turnInst, 
                                           (struct sockaddr *)&TurnServerAddr);
        
        StunPrint(threadCtx, 
                  StunInfoCategory_Trace, 
                  "<STUNCLIENT> OUT --> STUN: %s Len=%i to %s", 
                  stunlib_getMessageName(stunRespMsg->msgHdr.msgType), 
                  turnLen, 
                  sockaddr_toString((struct sockaddr *)&TurnServerAddr, 
                                    turnservaddrStr, 
                                    SOCKADDR_MAX_STRLEN,
                                    true));  
        
        /* send to turn server */
        sendFunc(globalSocketId,
                 TxBuff,
                 turnLen,
                 (struct sockaddr *)&TurnServerAddr,
                 t,
                 userData);
    
        StunClientStats.BindRespSent_ViaRelay++;
    }
    else
    {

        /* send */
        sendFunc(globalSocketId, stunBuff, stunLen, dstAddr, t,userData);
    }

    StunClientStats.BindRespSent++;

    return true;
}

/********* Server handling of STUN BIND RESP *************/
void StunServer_SendConnectivityBindingResp(uint32_t             threadCtx,
                                            int32_t          globalSocketId,
                                            StunMsgId        transactionId,
                                            char            *username,
                                            char            *password,
                                            struct sockaddr *mappedAddr,
                                            socklen_t        mappedAddrlen,
                                            struct sockaddr *dstAddr,
                                            socklen_t        dstAddrLen,
                                            void            *userData,
                                            STUN_SENDFUNC    sendFunc,
                                            bool             useRelay,
                                            int              turnInst)
                                            
{
    StunMessage stunRespMsg;

    /* format */
    if (CreateConnectivityBindingResp(threadCtx, 
                                      &stunRespMsg, 
                                      transactionId, 
                                      username, 
                                      mappedAddr))
    {
        /* encode and send */
        SendConnectivityBindResponse(threadCtx, 
                                     globalSocketId, 
                                     &stunRespMsg, 
                                     password, 
                                     dstAddr, 
                                     dstAddrLen, 
                                     userData, 
                                     sendFunc, 
                                     useRelay, 
                                     turnInst);
    }
}


/********** Server handling of incoming STUN BIND REQ **********/
bool StunServer_HandleStunIncomingBindReqMsg(uint32_t threadCtx,
                                                 STUN_INCOMING_REQ_DATA  *pReq,
                                                 StunMessage *stunMsg,
                                                 bool fromRelay)
{
    memcpy(&pReq->transactionId, &stunMsg->msgHdr.id, sizeof(StunMsgId)); 

    pReq->fromRelay = fromRelay;

    if(stunMsg->hasUsername){
        strncpy(pReq->ufrag, stunMsg->username.value, min(stunMsg->username.sizeValue, STUN_MAX_STRING)); 
        if(stunMsg->username.sizeValue < STUN_MAX_STRING){
            pReq->ufrag[stunMsg->username.sizeValue]= '\0';
        }else{
            pReq->ufrag[STUN_MAX_STRING-1]='\0';
        }
    }else{
        StunPrint(threadCtx, StunInfoCategory_Error, "<STUNCLIENT> Missing Username in Binding Request\n");
        return  false;
    }
    if(stunMsg->hasPriority){
        pReq->peerPriority = stunMsg->priority.value;
    }else{
        StunPrint(threadCtx, StunInfoCategory_Error, "<STUNCLIENT> Missing Priority in Binding Request\n");
        return  false;
    }

    if(stunMsg->hasUseCandidate){
        pReq->useCandidate = true;
    }else{
        pReq->useCandidate = false;
    }
    
    if(stunMsg->hasControlling){
        pReq->iceControlling = true;
        pReq->tieBreaker = stunMsg->controlling.value;
    }else{
        pReq->iceControlling = false;
    }
    
    if(stunMsg->hasControlled){
        pReq->iceControlled = true;
        pReq->tieBreaker = stunMsg->controlled.value;
    } else{
        pReq->iceControlled = false;
    }

    if (fromRelay)
    {
        StunClientStats.BindReqReceived_ViaRelay++;
    }
    StunClientStats.BindReqReceived++;
    return true;
}





/*************************************************************************/
/************************ FSM Framework **********************************/
/*************************************************************************/


static const STUN_STATE_TABLE StateTable[] = 
{
    /* function ptr */                      /* str */
    { StunState_Idle,                       "Idle"                          },
    { StunState_WaitBindResp,               "WaitBindResp"                  },
    { StunState_Cancelled,                  "Cancelled"                     },
};

static const uint32_t NoOfStates = sizeof(StateTable)/sizeof(StateTable[0]);


static const char *StunsigToStr(STUN_SIGNAL sig)
{
    switch (sig)
    {
        case STUN_SIGNAL_BindReq:            return "BindReq";
        case STUN_SIGNAL_BindResp:           return "BindResp";
        case STUN_SIGNAL_BindRespError:      return "BindRespError";
        case STUN_SIGNAL_TimerTick:          return "TimerTick";
        case STUN_SIGNAL_TimerRetransmit:    return "TimerRetransmit";
        case STUN_SIGNAL_DeAllocate:         return "DeAllocate";
        case STUN_SIGNAL_Cancel:             return "Cancel";
        default:                             return "???";
    }
}


/* */
static int AllocFreeInst(uint32_t threadCtx, STUN_SIGNAL *sig, uint8_t *payload)
{
    int i;
    int ret;
    StunBindReqStuct *pMsgIn = (StunBindReqStuct*)payload;

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
    ret = StunClientConstruct(pMsgIn->threadCtx);  /* create/alloc a new instance */
    MutexUnlock(threadCtx, -1);
    return ret;
}

static  void SetNextState(STUN_INSTANCE_DATA *pInst,  STUN_STATE NextState)
{
    if  (NextState >= NoOfStates)
    {
        StunPrint(pInst->threadCtx, StunInfoCategory_Error, "<STUNCLIENT:%02d> SetNextState, Illegal State %d", pInst->inst, NextState);
        return;
    }

    if (pInst->state != NextState)
    {
        StunPrint(pInst->threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> State (%s -> %s)", pInst->inst, StateTable[pInst->state].StateStr, StateTable[NextState].StateStr);
        pInst->state = NextState;
    }

    /* always free instance on return to idle */
    if (NextState == STUN_STATE_Idle)
        pInst->inUse = false;
}


/* check if timer has expired and return the timer signal */
static bool TimerHasExpired(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig)
{
    int  *timer;

    switch (sig)
    {
        case STUN_SIGNAL_TimerRetransmit:
            timer = &pInst->TimerRetransmit;
        break;
        default:
            StunPrint(pInst->threadCtx, StunInfoCategory_Error, "<STUNCLIENT:%02d> illegal timer expiry %d ", pInst->inst,  sig); 
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
static void StartTimer(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint32_t durationMsec)
{
    StunPrint(pInst->threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> StartTimer(%s, %dms)", pInst->inst, StunsigToStr(sig), durationMsec);

    switch (sig)
    {
        case STUN_SIGNAL_TimerRetransmit:
            pInst->TimerRetransmit = durationMsec;      
        break;
        default:
            StunPrint(pInst->threadCtx, StunInfoCategory_Error, "<STUNCLIENT:%02d> illegal StartTimer %d, duration %d", pInst->inst,  sig, durationMsec); 
        break;
    }
}

/* check if timer has expired and return the timer signal */
static void StopTimer(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig)
{
    StunPrint(pInst->threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> StopTimer(%s)", pInst->inst, StunsigToStr(sig));

    switch (sig)
    {
        case STUN_SIGNAL_TimerRetransmit:
            pInst->TimerRetransmit = 0;
        break;
        default:
            StunPrint(pInst->threadCtx, StunInfoCategory_Error, "<STUNCLIENT:%02d> illegal StopTimer %d", pInst->inst,  sig); 
        break;
    }
}


static int  StunClientMain(uint32_t threadCtx, int ctx, STUN_SIGNAL sig, uint8_t *payload)
{
    int ret= STUNCLIENT_CTX_UNKNOWN;

    /* if  context is already known, just call the  fsm */
    if (ctx != STUNCLIENT_CTX_UNKNOWN)
    {
        if (ctx < CurrentInstances[threadCtx])
            StunClientFsm(InstanceData[threadCtx][ctx], sig, payload);
        else
            StunPrint(threadCtx, StunInfoCategory_Error, "<STUNCLIENT> sig: %s illegal context %d exceeds %d\n ", StunsigToStr(sig), ctx, CurrentInstances[threadCtx]-1); 
        ret = ctx;
    }
    else if (sig == STUN_SIGNAL_BindReq)
    {
        /* get ptr to instance data and call the state machine */
        ctx = AllocFreeInst(threadCtx, &sig, payload);
        if (ctx >= 0)
        {
            StunClientFsm(InstanceData[threadCtx][ctx], sig, payload);
            ret = ctx; /* returns context */
        }
        else
        {
            StunPrint(threadCtx, StunInfoCategory_Error, "<STUNCLIENT> No free instances, sig: %s", StunsigToStr(sig));
            ret = STUNCLIENT_CTX_UNKNOWN;
        }
        return ret;
    }
    return ret;
}


/*************************************************************************/
/************************ FSM functions  *********************************/
/*************************************************************************/

/* clear everything except  instance, state */
static void InitInstData(STUN_INSTANCE_DATA *pInst)
{
   STUN_STATE state;
   uint32_t   inst;
   uint32_t   threadCtx;
   bool       inUse;

   state = pInst->state;
   inst  = pInst->inst;
   threadCtx = pInst->threadCtx;
   inUse = pInst->inUse;
   memset(pInst, 0, sizeof(*pInst));
   pInst->state = state;
   pInst->inst = inst;
   pInst->threadCtx = threadCtx;
   pInst->inUse = inUse;

}

static void StoreStunBindReq(STUN_INSTANCE_DATA *pInst, StunBindReqStuct *pMsgIn)
{
    /* copy whole msg */
    memcpy(&pInst->stunBindReq, pMsgIn, sizeof(StunBindReqStuct));
}

static void  BuildStunBindReq(STUN_INSTANCE_DATA *pInst, StunMessage  *stunReqMsg)
{
    memset(stunReqMsg, 0, sizeof(StunMessage));
    stunReqMsg->msgHdr.msgType = STUN_MSG_BindRequestMsg;

    /* transaction id */
    memcpy(&stunReqMsg->msgHdr.id, &pInst->stunBindReq.transactionId, sizeof(StunMsgId));

    /* Username */
    stunReqMsg->hasUsername = true;
    strncpy(stunReqMsg->username.value, pInst->stunBindReq.ufrag, STUN_MAX_STRING - 1);
    stunReqMsg->username.sizeValue = min(STUN_MAX_STRING, strlen(pInst->stunBindReq.ufrag));
    
    /* Priority */
    stunReqMsg->hasPriority    = true;
    stunReqMsg->priority.value = pInst->stunBindReq.peerPriority;
    
    /* useCandidate */
    stunReqMsg->hasUseCandidate = pInst->stunBindReq.useCandidate ? true : false;
    
    /* controlling */
    stunReqMsg->hasControlling = (pInst->stunBindReq.iceControlling ? true : false);
    stunReqMsg->controlling.value =  pInst->stunBindReq.tieBreaker;

    stunlib_addSoftware(stunReqMsg, SoftwareVersionStr, STUN_DFLT_PAD);

}


/* encode and send */
static bool  SendStunReq(STUN_INSTANCE_DATA *pInst, StunMessage  *stunReqMsg)
{
    /* encode the BindReq */
    pInst->stunReqMsgBufLen = stunlib_encodeMessage(stunReqMsg,
                                                (unsigned char*) (pInst->stunReqMsgBuf),
                                                STUN_MAX_PACKET_SIZE, 
                                                (unsigned char*)&pInst->stunBindReq.password,  /* key */
                                                strlen( pInst->stunBindReq.password ) ,     /* keyLen  */
                                                false,
                                                false);

    if (!pInst->stunReqMsgBufLen)
    {
        StunPrint(pInst->threadCtx, StunInfoCategory_Error, "<STUNCLIENT:%02d>  SendStunReq(BindReq), failed encode", pInst->inst);
        return false;
    }

    if (pInst->stunBindReq.useRelay)
    {
        uint8_t                    TxBuff[STUN_MAX_PACKET_SIZE] = {0,};
        struct sockaddr_storage    TurnServerAddr;
        char                       TurnServerAddrStr[SOCKADDR_MAX_STRLEN] = {0,};
        char                       peer[SOCKADDR_MAX_STRLEN] = {0,};
        int len;

        sockaddr_toString((struct sockaddr *)&pInst->stunBindReq.serverAddr,
                                    peer,
                                    SOCKADDR_MAX_STRLEN, 
                                    true);

        /* encode a turn SendIndication, put the BindReq in the data attribute */
        len = TurnClient_createSendIndication(TxBuff,                         /* dest=encoded sendInd */
                                              (uint8_t*)pInst->stunReqMsgBuf, /* DataAttr=encoded BindReq */
                                              STUN_MAX_PACKET_SIZE,
                                              pInst->stunReqMsgBufLen,        /* len of encoded DataAttr (BindReq) */
                                              (struct sockaddr *)&pInst->stunBindReq.serverAddr,  /* peer address */  
                                              false,                        /* no MsStun yet  TBD ..*/
                                              RTP_STUN_THREAD_CTX,          /* no  MsStun yet TBD ..*/
                                              pInst->stunBindReq.turnInst); /* turnInst */

        TurnClientGetActiveTurnServerAddr(RTP_STUN_THREAD_CTX, 
                                          pInst->stunBindReq.turnInst, 
                                          (struct sockaddr *)&TurnServerAddr);

        StunPrint(pInst->threadCtx, 
                  StunInfoCategory_Trace, 
                  "<STUNCLIENT:%02d> OUT --> STUN: %s Len=%i to peer %s via %s", 
                  pInst->inst, 
                  stunlib_getMessageName(stunReqMsg->msgHdr.msgType), 
                  pInst->stunReqMsgBufLen, 
                  pInst->stunBindReq.serverAddr,
                  sockaddr_toString((struct sockaddr *)&TurnServerAddr, 
                                    TurnServerAddrStr,
                                    SOCKADDR_MAX_STRLEN, 
                                    true));  
        
        


        sockaddr_copy((struct sockaddr *)&pInst->stunBindReq.serverAddr, 
                      (struct sockaddr *)&TurnServerAddr);


        memcpy(pInst->stunReqMsgBuf, TxBuff, len); 
        pInst->stunReqMsgBufLen = len;
        StunClientStats.BindReqSent_ViaRelay++;
    }
    else
        StunPrint(pInst->threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> OUT --> STUN: %s Len=%i to peer %s", 
                  pInst->inst, stunlib_getMessageName(stunReqMsg->msgHdr.msgType), pInst->stunReqMsgBufLen, pInst->stunBindReq.serverAddr); 
    
    pInst->stunBindReq.sendFunc(pInst->stunBindReq.sockhandle,
                                pInst->stunReqMsgBuf,
                                pInst->stunReqMsgBufLen,
                                (struct sockaddr *)&pInst->stunBindReq.serverAddr,
                                sizeof(pInst->stunBindReq.serverAddr),
                                pInst->stunBindReq.userCtx);
    
    StunClientStats.BindReqSent++;

    return true;
}


static void  StunClientFsm(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload)
{
    if (pInst->state < STUN_STATE_End)
    {
        StunPrint(pInst->threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> IN <-- %s (state %s)", pInst->inst, StunsigToStr(sig), StateTable[pInst->state].StateStr);
        MutexLock(pInst->threadCtx, pInst->inst);
        (StateTable[pInst->state].Statefunc)(pInst, sig, payload);
        MutexUnlock(pInst->threadCtx, pInst->inst);
    }
    else
        StunPrint(pInst->threadCtx, StunInfoCategory_Error, "<STUNCLIENT:%02d> undefned state %d, sig %s",pInst->inst, pInst->state, StunsigToStr(sig));
}

static void RetransmitLastReq(STUN_INSTANCE_DATA *pInst, struct sockaddr_storage * destAddr)
{
    pInst->stunBindReq.sendFunc(pInst->stunBindReq.sockhandle,
                                pInst->stunReqMsgBuf,
                                pInst->stunReqMsgBufLen,
                                (struct sockaddr *)destAddr,
                                sizeof(pInst->stunBindReq.serverAddr),
                                pInst->stunBindReq.userCtx);
}


static void StartFirstRetransmitTimer(STUN_INSTANCE_DATA *pInst)
{
    pInst->retransmits = 0;
    StartTimer(pInst, STUN_SIGNAL_TimerRetransmit, pInst->stunBindReq.stunTimeoutList[pInst->retransmits]);
}


static void StartNextRetransmitTimer(STUN_INSTANCE_DATA *pInst)
{
    StartTimer(pInst, STUN_SIGNAL_TimerRetransmit, pInst->stunBindReq.stunTimeoutList[pInst->retransmits]);
}

static void CallBack(STUN_INSTANCE_DATA *pInst, StunResult_T stunResult)
{
    if (pInst->stunBindReq.stunCbData)
    {
        memcpy(&pInst->stunBindReq.stunCbData->msgId, &pInst->stunBindReq.transactionId, 
               sizeof(StunMsgId));
        pInst->stunBindReq.stunCbData->stunResult = stunResult;
    }
    if (pInst->stunBindReq.stunCbFunc)
       (pInst->stunBindReq.stunCbFunc)(pInst->stunBindReq.userCtx, pInst->stunBindReq.stunCbData);
}


static void  CommonRetryTimeoutHandler(STUN_INSTANCE_DATA *pInst, StunResult_T stunResult, const char *errStr, STUN_STATE FailedState)
{
    if ((pInst->retransmits < STUNCLIENT_MAX_RETRANSMITS)
    && (pInst->stunBindReq.stunTimeoutList[pInst->retransmits] != 0))  /* can be 0 terminated if using fewer retransmits */
    {
        char peer [SOCKADDR_MAX_STRLEN] ={0,};
        sockaddr_toString ((struct sockaddr *) &pInst->stunBindReq.serverAddr, peer, sizeof (peer), true);

        if (pInst->stunBindReq.useRelay)
        {
            struct sockaddr_storage    TurnServerAddr;
            char                       TurnServerAddrStr[SOCKADDR_MAX_STRLEN] = {0,};

            TurnClientGetActiveTurnServerAddr(RTP_STUN_THREAD_CTX,
                                              pInst->stunBindReq.turnInst,
                                              (struct sockaddr *)&TurnServerAddr);

            sockaddr_toString((struct sockaddr *)&TurnServerAddr, 
                                TurnServerAddrStr,
                                SOCKADDR_MAX_STRLEN, 
                                true);


            StunPrint(pInst->threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> Retrans %s Retry: %d to %s via %s",
              pInst->inst, errStr, pInst->retransmits+1, peer, TurnServerAddrStr);
            RetransmitLastReq(pInst, &TurnServerAddr);
        }
        else
        {
            StunPrint(pInst->threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> Retrans %s Retry: %d to %s",
                pInst->inst, errStr, pInst->retransmits+1, peer);
            RetransmitLastReq(pInst, &pInst->stunBindReq.serverAddr);
        }
        StartNextRetransmitTimer(pInst);
        pInst->retransmits++;
        StunClientStats.Retransmits++;
    }
    else
    {
        //char transid[STUN_MSG_ID_SIZE*2+3];
        //StunPrint(pInst->threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> %s failed from %s to %s ('%s')",pInst->inst, errStr, pInst->stunBindReq.baseAddr, pInst->stunBindReq.serverAddr, stunlib_transactionIdtoStr( transid, pInst->stunBindReq.transactionId));
        CallBack(pInst, stunResult);
        SetNextState(pInst, FailedState);
        StunClientStats.Failures++;
    }
}


static void  CancelRetryTimeoutHandler(STUN_INSTANCE_DATA *pInst )
{
    if ((pInst->retransmits < STUNCLIENT_MAX_RETRANSMITS)
    && (pInst->stunBindReq.stunTimeoutList[pInst->retransmits] != 0))  /* can be 0 terminated if using fewer retransmits */
    {
        StartNextRetransmitTimer(pInst);
        pInst->retransmits++;
    }
    else
    {
        StunPrint(pInst->threadCtx, StunInfoCategory_Trace, "<STUNCLIENT:%02d> Cancel complete %s to %s",pInst->inst, pInst->stunBindReq.baseAddr, pInst->stunBindReq.serverAddr);
        CallBack(pInst, StunResult_CancelComplete);
        SetNextState(pInst, STUN_STATE_Idle);
    }
}



static void InitRetryCounters(STUN_INSTANCE_DATA *pInst)
{
    pInst->retransmits = 0;
}

static bool StoreBindResp(STUN_INSTANCE_DATA *pInst, StunMessage *pResp)
{
    struct sockaddr_storage addr;

    if (pResp->hasXorMappedAddress)
    {
        if (pResp->xorMappedAddress.familyType == STUN_ADDR_IPv4Family){
            sockaddr_initFromIPv4Int((struct sockaddr_in *)&addr, 
                                     pResp->xorMappedAddress.addr.v4.addr,
                                     pResp->xorMappedAddress.addr.v4.port);
        }
        else if (pResp->xorMappedAddress.familyType == STUN_ADDR_IPv6Family){
            sockaddr_initFromIPv6Int((struct sockaddr_in6 *)&addr, 
                                     pResp->xorMappedAddress.addr.v6.addr,
                                     pResp->xorMappedAddress.addr.v6.port);
        }
        sockaddr_copy((struct sockaddr *)&pInst->rflxAddr,
                      (struct sockaddr *)&addr);
        
        return true;
    }
    else
    {
       StunPrint(pInst->threadCtx, StunInfoCategory_Error, "<STUNCLIENT:%02d> Missing XorMappedAddress BindResp",pInst->inst);
       return false;
    }
}

static void  BindRespCallback(STUN_INSTANCE_DATA *pInst, struct sockaddr *srcAddr)
{
    StunCallBackData_T *pRes = pInst->stunBindReq.stunCbData;
    StunBindResp       *pData;

    if (pRes)
    {
        memcpy(&pRes->msgId, &pInst->stunBindReq.transactionId, 
               sizeof(StunMsgId));

        pData = &pInst->stunBindReq.stunCbData->bindResp;
        pRes->stunResult = StunResult_BindOk;
        memcpy(pData->rflxAddr, pInst->rflxAddr, sizeof(pData->rflxAddr));
        pData->rflxPort = pInst->rflxPort;
        
        sockaddr_copy((struct sockaddr *)&pRes->srcAddrStr, 
                      srcAddr);

        sockaddr_copy((struct sockaddr*)&pRes->dstBaseAddrStr, 
                      (struct sockaddr*)&pInst->stunBindReq.baseAddr);
        //printf("<stunclient> Response base addr: '%s'\n", pInst->stunBindReq.baseAddr );

        StunPrint(pInst->threadCtx, StunInfoCategory_Info, "<STUNCLIENT:%02d> BindResp Rflx: %s:%d from src: %s",
                pInst->inst, pData->rflxAddr, pData->rflxPort, pRes->srcAddrStr);
    }

    if (pInst->stunBindReq.stunCbFunc)
        (pInst->stunBindReq.stunCbFunc)(pInst->stunBindReq.userCtx, pInst->stunBindReq.stunCbData);
}


/* Common signal handling for all states */
static void  StunAllState(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload)
{
    

    switch (sig)
    {
        default:
        {
            //char transid[STUN_MSG_ID_SIZE*2+3];
            //StunPrint(pInst->threadCtx, StunInfoCategory_Error, "<STUNCLIENT:%02d> undefned signal %s in state %d ('%s')", pInst->inst, StunsigToStr(sig), pInst->state, stunlib_transactionIdtoStr( transid, pInst->stunBindReq.transactionId) );
        }
    }
}

static void  StunState_Idle(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {
        case STUN_SIGNAL_BindReq:
        {
            StunBindReqStuct *pMsgIn = (StunBindReqStuct*)payload;
            StunMessage  stunReqMsg; /* decoded */
            
            /* clear instance data */
            InitInstData(pInst);

            /* store msg */
            StoreStunBindReq(pInst,  pMsgIn);

            /* build and send stun bind req */
            BuildStunBindReq(pInst, &stunReqMsg);
            SendStunReq(pInst, &stunReqMsg);
            
            InitRetryCounters(pInst);
            StartFirstRetransmitTimer(pInst);
            SetNextState(pInst, STUN_STATE_WaitBindResp);
        }
        break;

        case STUN_SIGNAL_DeAllocate: /* ignore extra clears */
        case STUN_SIGNAL_Cancel:
        break;

        case STUN_SIGNAL_BindResp:
            StunClientStats.BindRespReceived_InIdle++;
        break;

        default:
          StunAllState(pInst, sig, payload);
        break;
    }

} /* StunState_Idle() */


/*
 * Bind request has been sent, waiting for response.
 */
static void  StunState_WaitBindResp(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {

        case STUN_SIGNAL_BindResp:
        {
            StunRespStruct *pMsgIn = (StunRespStruct*)payload;
            StunMessage *pResp = &pMsgIn->stunRespMessage;

            StopTimer(pInst, STUN_SIGNAL_TimerRetransmit);
            if (StoreBindResp(pInst, pResp))
            {
                BindRespCallback(pInst, (struct sockaddr *)&pMsgIn->srcAddr); 
            }
            else
            {
                CallBack(pInst, StunResult_MalformedResp);
            }
            StunClientStats.BindRespReceived++;
            SetNextState(pInst, STUN_STATE_Idle);
        }
        break;

        case STUN_SIGNAL_BindRespError:
        {
            CallBack(pInst, StunResult_BindFail);
            StunClientStats.BindRespErrReceived++;
            SetNextState(pInst, STUN_STATE_Idle);
        }
        break;

        case STUN_SIGNAL_Cancel:
        {
            SetNextState(pInst, STUN_STATE_Cancelled);
        }
        break;

        case STUN_SIGNAL_TimerRetransmit:
        {
            CommonRetryTimeoutHandler(pInst, StunResult_BindFailNoAnswer, "BindReq", STUN_STATE_Idle);
        }
        break;

        case STUN_SIGNAL_DeAllocate:
        {
            StopTimer(pInst, STUN_SIGNAL_TimerRetransmit);
            SetNextState(pInst, STUN_STATE_Idle);
        }
        break;

        default:
            StunAllState(pInst, sig, payload);
            break;
    }

} /* StunState_WaitBindResp() */



/*
 * Cancel hes been received, still waiting for response.
 * Do not fail if timeout
 */
static void  StunState_Cancelled(STUN_INSTANCE_DATA *pInst, STUN_SIGNAL sig, uint8_t *payload)
{
    switch (sig)
    {

        case STUN_SIGNAL_BindResp:
        {
            StunRespStruct *pMsgIn = (StunRespStruct*)payload;
            StunMessage *pResp = &pMsgIn->stunRespMessage;

            StopTimer(pInst, STUN_SIGNAL_TimerRetransmit);
            if (StoreBindResp(pInst, pResp))
            {
                BindRespCallback(pInst, (struct sockaddr *)&pMsgIn->srcAddr); 
            }
            else
            {
                CallBack(pInst, StunResult_MalformedResp);
            }
            StunClientStats.BindRespReceived_AfterCancel++;
            SetNextState(pInst, STUN_STATE_Idle);
        }
        break;

        case STUN_SIGNAL_BindRespError:
        {
            CallBack(pInst, StunResult_BindFail);
            StunClientStats.BindRespErrReceived++;
            SetNextState(pInst, STUN_STATE_Idle);
        }
        break;

        case STUN_SIGNAL_TimerRetransmit:
        {
            CancelRetryTimeoutHandler(pInst);
        }
        break;

        case STUN_SIGNAL_DeAllocate:
        {
            StopTimer(pInst, STUN_SIGNAL_TimerRetransmit);
            SetNextState(pInst, STUN_STATE_Idle);
        }
        break;

        default:
            StunAllState(pInst, sig, payload);
            break;
    }

} /* StunState_Cancelled() */

