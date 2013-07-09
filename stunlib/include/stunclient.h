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

#ifndef STUNCLIENT_H
#define STUNCLIENT_H


#include "stunlib.h"   /* stun enc/dec and msg formats*/
#include <stdint.h>
#include <stdbool.h>
#include <sockaddr_util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STUN_MAX_THREAD_CTX      4
#define STUNCLIENT_CTX_UNKNOWN  -1
#define STUN_MAX_ERR_STRLEN    256  /* max size of string in STUN_INFO_FUNC */

/* Result of  binding  protocol, returned in callback */
typedef enum 
{
    StunResult_Empty,
    StunResult_BindOk,                   /* successful */
    StunResult_BindFail,                 /* Received BindErrorResp */
    StunResult_BindFailNoAnswer,         /* Bind Req failed - no contact with stun server */
    StunResult_BindFailUnauthorised,     /* passwd/username is incorrect */
    StunResult_CancelComplete,           /* Request is cancelled and timed out */
    StunResult_InternalError,             
    StunResult_MalformedResp
} StunResult_T;


/* 
 * Result of successful Stun Binding request has the following format.
 *     rflxddr/Port -   Reflexive Address/port.  This is source addr/port of the AllocateRequest as seen by the turn server
 */
typedef struct
{
    char rflxAddr[SOCKADDR_MAX_STRLEN];
    int  rflxPort;
} StunBindResp;


/* Signalled back to the caller as a paramter in the TURN callback (see TURNCB) */
typedef struct
{
    StunMsgId               msgId;
    StunResult_T            stunResult;
    StunBindResp            bindResp;
    struct sockaddr_storage srcAddrStr;
    struct sockaddr_storage dstBaseAddrStr; //The destination seen from the sender of the response
} StunCallBackData_T;

/******************* start MALICE specific ************************/
typedef struct
{
    bool                    hasMDAgent;
    MaliceAttrAgent         mdAgent;
    bool                    hasMDRespUP;
    MaliceAttrResp          mdRespUP;
    bool                    hasMDRespDN;
    MaliceAttrResp          mdRespDN;
    bool                    hasMDPeerCheck;
    MaliceAttrPeerCheck     mdPeerCheck;
} MaliceMetadata;

/******************* end MALICE specific ************************/

/* category of info sent in STUN_INFO_FUNC */
typedef enum
{
    StunInfoCategory_Info,
    StunInfoCategory_Error,
    StunInfoCategory_Trace

} StunInfoCategory_T;


typedef struct{
    int32_t globalSocketId;          
    STUN_SENDFUNC sendFunc;

    /*Request Data*/
    char     ufrag[STUN_MAX_STRING];
    uint32_t ufragLen;
    uint32_t peerPriority;
    bool     useCandidate;
    bool     iceControlling;
    bool     iceControlled;
    uint64_t tieBreaker;
    char     *dstAddr; 
    char     *srcAddr;
    uint8_t  transactionId[12];
    bool     fromRelay;
    uint32_t peerPort;
} STUN_INCOMING_REQ_DATA;

typedef struct
{
    uint32_t InProgress;
    uint32_t BindReqSent;
    uint32_t BindReqSent_ViaRelay;
    uint32_t BindRespReceived;
    uint32_t BindRespReceived_AfterCancel;
    uint32_t BindRespReceived_InIdle;
    uint32_t BindRespReceived_ViaRelay;
    uint32_t BindRespErrReceived;
    uint32_t BindReqReceived;
    uint32_t BindReqReceived_ViaRelay;
    uint32_t BindRespSent;
    uint32_t BindRespSent_ViaRelay;
    uint32_t Retransmits;
    uint32_t Failures;
}StunClientStats_T;


/*********************************************************************************************************************/
/******************************************* start MALICE specific  **************************************************/
/*********************************************************************************************************************/

typedef struct
{

} MALICE_REQ;

/*********************************************************************************************************************/
/******************************************* end MALICE specific  **************************************************/
/*********************************************************************************************************************/



/* Signalling back to user e.g. result of BindResp.
 *   userCtx        - User provided context, as provided in StunClient_startxxxx(userCtx,...)
 *   stunCbData     - User provided stun callback data. stunbind writes status here. e.g. Alloc ok + reflexive + relay address
 */
typedef void (*STUNCB)(void *userCtx, StunCallBackData_T *stunCbData);

/* for output of managment info (optional) */
typedef void  (*STUN_INFO_FUNC_PTR)(StunInfoCategory_T category, char *ErrStr);


/*
 *  initialisation:
 *    Should be called (once only) before StunClient_startxxxxxx(). 
 *    If this is not called then the 1st time StunClient_startStunAllocateTransaction() is called,
 *    a default of 50 instances and with a tick of 50 msec and no management function is setup internally.  
 *
 *    threadCtx   - Unique thread context. (0..STUN_MAX_THREAD_CTX)
 *    instances   - max. number of simultaneous  STUN transactions to support.
 *
 *    tickMsec    - Tells stunclientbind how often StunClientBind_HandleTick() is called.
 *
 *    funcPtr     - Will be called by Stun when it outputs management info and trace. 
 *                  If this is NULL, then there is no output.  You can provide a function such as below::
 *
 *                  static void  PrintStunInfo(StunInfoCategory_T category, char *ErrStr)
 *                  {
 *                       fprintf(stderr, "%s\n", ErrStr);
 *                  }
 *
 *    MultipleThreadAccess - Set to true if the stunclient is called from > 1 thread. (mutex will be created internally).
 *                           Note - Under WIN32 this mutex is a CRITICAL_SECTION  (much faster than WIN32 MUTEX).
 *                           Set to false if the stunclientbind is called from 1 thread.  (no internal mutex required)
 *
 *    SwVerStr    - Softwre version string to be sent in STUN Requests
 */
bool StunClient_Init(uint32_t threadCtx,
                         int instances, 
                         uint32_t tickMsec, 
                         STUN_INFO_FUNC_PTR funcPtr, 
                         bool MultipleThreadAccess,
                         char *SwVerStr);

/*Release all memory allocated in init */
void StunClient_Destruct(uint32_t threadCtx);

/*
 *  Initiate a Stun Bind Transaction
 *     threadCtx        -  if turnclientlib is used by > 1 thread this must be provided and must be unique for each calling thread.
 *     userCtx          -  user specific context info (e.g. CallId/ChanId).  Optional, can be NULL. STUN does not write to this data.
 *     StunServerAddr   -  Address of TURN server in format  "a.b.c.d:port"
 *     BaseAddr         -  Address of BASE in format  "a.b.c.d:port"
 *     uFrag            -  Combination of  local and remote ufrag exchanged in INVITE(LFRAG) / OK(RFRAG) in format <LFRAG>:<RFRAG>  
 *     password         -  Remote password, exchanged in invite/ok.    \0 terminated string. Max 512 chars.
 *     peerPriority     -  Candidate Priority. See ICE-19 spec.
 *     useCandidate     - 
 *     iceControlling   -  
 *     tieBreaker       -  
 *     transactionId    - 
 *     sockhandle       -  used as 1st parameter in STUN_SENDFUNC(), typically a socket.
 *     sendFunc         -  function used to send STUN packet. send(sockhandle,buff, len, turnServerAddr, userCtx)
 *     timeoutList      -  Defines interval between each retry in msec. 0 terminated.  e.g. 500, 1000 results in 
 *                         retry after 500msec and retry after 1000 msec. If this is NULL, then TURN uses the default
 *                         stun retries defined in stunlib.h
 *     stunCbFunc       -  user provided callback function used by turn to signal the result of an allocation or channel bind etc...
 *     stunCbData       -  user provided callback turn data. turn writes to this data area.
 *     turnInst         -  Associated turn instance
 *         
 *     returns          -  Turn instance/context. Application should store this in further calls to TurnClient_StartChannelBindReq(), TurnClient_HandleIncResp(). 
 */
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
                                     int                 turnInst,
                                     MaliceMetadata     *maliceMetadata);




/* 
 * This function must be called by the application every N msec. N must be same as in StunClientBind_Init(instances, N)
 *     threadCtx  -  if stunclientlib is used by > 1 thread this must be provided and must be unique for each calling thread.
 */
void StunClient_HandleTick(uint32_t threadCtx);

/*
 *  threadCtx     - Unique thread context. (0..STUN_MAX_THREAD_CTX)
 *  msg           - Decoded STUN message.
 *  srcAddrString - Source adress in format  "a.b.c.d:port"
 *
 * return - instance/ctx. 
 *
 */
int StunClient_HandleIncResp(uint32_t threadCtx, StunMessage *msg, struct sockaddr *srcAddr);


/*  
 * Cancel a transaction.
 *  threadCtx - Unique thread context. (0..STUN_MAX_THREAD_CTX)
 *  ctx       - context/instance number
 */
void StunClient_Deallocate(uint32_t threadCtx, int ctx);

/*  
 * Cancel a transaction with  matching  transaction id
 *      threadCtx      - Unique thread context. (0..STUN_MAX_THREAD_CTX)
 *      transactionId  - Transaction id.
 * return -  if  transaction found returns ctx/instance
 *        -  if  no instance found with transactionid, returns  STUNCLIENT_CTX_UNKNOWN
 */
int StunClient_cancelBindingTransaction(uint32_t threadCtx, StunMsgId transactionId);


/********* Server handling: send STUN BIND RESP *************/
void StunServer_SendConnectivityBindingResp(uint32_t         threadCtx,
                                            int32_t          globalSocketId,
                                            StunMsgId        transactionId,
                                            char            *username,
                                            char            *password,
                                            struct sockaddr *mappedAddrStr,
                                            socklen_t        mappedAddrlen,
                                            struct sockaddr *dstAddr,
                                            socklen_t        dstAddrLen,
                                            void            *userData,
                                            STUN_SENDFUNC    sendFunc,
                                            bool             useRelay,
                                            int          turnInst);

/********** Server handling:  incoming STUN BIND REQ **********/
bool StunServer_HandleStunIncomingBindReqMsg(uint32_t     threadCtx,
                                                 STUN_INCOMING_REQ_DATA  *pReq,
                                                 StunMessage *stunMsg,
                                                 bool fromRelay);

/* management */
void StunClientGetStats(StunClientStats_T *Stats);
void StunClientClearStats(void);


#ifdef __cplusplus
}
#endif

#endif /* STUNCLIENT_H */
