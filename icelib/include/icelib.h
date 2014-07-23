/*
Copyright 2014 Cisco. All rights reserved. 

Redistribution and use in source and binary forms, with or without modification, are 
permitted provided that the following conditions are met: 

   1. Redistributions of source code must retain the above copyright notice, this list of 
      conditions and the following disclaimer. 

   2. Redistributions in binary form must reproduce the above copyright notice, this list 
      of conditions and the following disclaimer in the documentation and/or other materials 
      provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY CISCO ''AS IS'' AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. 

The views and conclusions contained in the software and documentation are those of the 
authors and should not be interpreted as representing official policies, either expressed 
or implied, of Cisco.
*/



#ifndef ICELIB_H
#define ICELIB_H

#include "sockaddr_util.h"
#include "icelibtypes.h"
#include "icelib_defines.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
    #define FORMAT_CHECK( ff,ee)  __attribute__((format(printf, ff, ee)))
#else
    #define FORMAT_CHECK( ff,ee)
#endif




//-----------------------------------------------------------------------------
//
//===== Callback functions
//

//
//----- Callback for sending a Binding Request message (STUN client role)
//
typedef ICELIB_Result (*ICELIB_outgoingBindingRequest)(void                  *pUserData,
                                                       const struct sockaddr *destination,
                                                       const struct sockaddr *source,
                                                       uint32_t              userValue1,
                                                       uint32_t              userValue2,
                                                       uint32_t              componentId,
                                                       bool                  useRelay,
                                                       const char            *pUfragPair,
                                                       const char            *pPasswd,
                                                       uint32_t              peerPriority,
                                                       bool                  useCandidate,
                                                       bool                  iceControlling,
                                                       bool                  iceControlled,
                                                       uint64_t              tieBreaker,
                                                       StunMsgId             transactionId);

//
//----- Callback sending a Binding Response message (STUN server role)
//
typedef ICELIB_Result (*ICELIB_outgoingBindingResponse)(void *pUserData,
                                                        uint32_t userValue1,
                                                        uint32_t userValue2,
                                                        uint32_t componentId,
                                                        const struct sockaddr *source,
                                                        const struct sockaddr *destination,
                                                        const struct sockaddr *MappedAddress,
                                                        uint16_t errorResponse,
                                                        StunMsgId transactionId,
                                                        bool useRelay,
                                                        const char *pUfragPair,
                                                        const char *pPasswd);


//
//----- Callback to signal ICE completion
//
typedef ICELIB_Result (*ICELIB_connectivityChecksComplete)(void            *pUserData,
                                                           uint32_t         userValue1,
                                                           bool             isControlling,
                                                           bool             iceFailed);



//
//----- Callback used to generate keepalives
//
typedef ICELIB_Result (*ICELIB_sendKeepAlive)(void        *pUserData,
                                              uint32_t    userValue1,
                                              uint32_t    userValue2,
                                              uint32_t    mediaIdx);

//
//----- Callback for cancelling a Binding Request message (STUN client role)
//
typedef ICELIB_Result (*ICELIB_outgoingCancelRequest)(void      *pUserData,
                                                      uint32_t   userValue1,
                                                      StunMsgId  transactionId);

//
//----- Callback for updating password in the media stream (Used to validate STUN packets)
//
typedef ICELIB_Result (*ICELIB_passwordUpdate)(void      *pUserData,
                                               uint32_t   userValue1,
                                               uint32_t   userValue2,
                                               char      *password);

//
//----- Callback for logging
//
typedef void (*ICELIB_logCallback)(void             *pUserData,
                                   ICELIB_logLevel   logLevel,
                                   const char       *str);



//
//----- ICE callback function pointers and data
//
struct         tag_ICELIB_INSTANCE;
typedef struct tag_ICELIB_INSTANCE ICELIB_INSTANCE_;

typedef struct {
    ICELIB_connectivityChecksComplete   pICELIB_connectivityChecksComplete;
    void                                *pConnectivityChecksCompleteUserData;
    ICELIB_INSTANCE_                    *pInstance;
} ICELIB_CALLBACK_COMPLETE;

typedef struct {
    ICELIB_outgoingBindingRequest   pICELIB_sendBindingRequest;
    void                            *pBindingRequestUserData;
    ICELIB_INSTANCE_                *pInstance;
} ICELIB_CALLBACK_REQUEST;


typedef struct {
    ICELIB_outgoingBindingResponse  pICELIB_sendBindingResponse;
    void                            *pBindingResponseUserData;
    ICELIB_INSTANCE_                *pInstance;
} ICELIB_CALLBACK_RESPONSE;


typedef struct {
    ICELIB_outgoingCancelRequest    pICELIB_sendCancelRequest;
    void                            *pCancelRequestUserData;
    ICELIB_INSTANCE_                *pInstance;
} ICELIB_CALLBACK_CANCEL_REQUEST;

typedef struct {
    ICELIB_sendKeepAlive            pICELIB_sendKeepAlive;
    void                            *pUserDataKeepAlive;
    ICELIB_INSTANCE_                *pInstance;
} ICELIB_CALLBACK_KEEPALIVE;


typedef struct {
    ICELIB_passwordUpdate            pICELIB_passwordUpdate;
    void                            *pUserDataPasswordUpdate;
    ICELIB_INSTANCE_                *pInstance;
} ICELIB_CALLBACK_PASSWORD_UPDATE;

typedef struct {
    ICELIB_logCallback              pICELIB_logCallback;
    void                            *pLogUserData;
    ICELIB_INSTANCE_                *pInstance;
} ICELIB_CALLBACK_LOG;


typedef struct {
    ICELIB_CALLBACK_REQUEST         callbackRequest;
    ICELIB_CALLBACK_RESPONSE        callbackResponse;
    ICELIB_CALLBACK_KEEPALIVE       callbackKeepAlive;
    ICELIB_CALLBACK_COMPLETE        callbackComplete;
    ICELIB_CALLBACK_CANCEL_REQUEST  callbackCancelRequest;
    ICELIB_CALLBACK_PASSWORD_UPDATE callbackPasswordUpdate;
    ICELIB_CALLBACK_LOG             callbackLog;
} ICELIB_CALLBACKS;




//
//----- ICE configuration data
//
typedef struct {
    unsigned int    tickIntervalMS;
    unsigned int    keepAliveIntervalS;
    unsigned int    maxCheckListPairs;
    bool            aggressiveNomination;
    bool            iceLite;
    ICELIB_logLevel logLevel;
} ICELIB_CONFIGURATION;


//
//----- ICE instance data
//
typedef struct tag_ICELIB_INSTANCE {
    ICELIB_STATE                iceState;
    ICELIB_CONFIGURATION        iceConfiguration;
    ICELIB_CALLBACKS            callbacks;
    ICE_MEDIA                   localIceMedia;
    ICE_MEDIA                   remoteIceMedia;
    bool                        iceControlling;
    bool                        iceControlled;
    bool                        iceSupportVerified;
    uint64_t                    tieBreaker;
    ICELIB_STREAM_CONTROLLER    streamControllers[ ICE_MAX_MEDIALINES];
    unsigned int                numberOfMediaStreams;
    unsigned int                roundRobinStreamControllerIndex;
    uint32_t                    tickCount;
    uint32_t                    keepAliveTickCount;
} ICELIB_INSTANCE;



void ICELIB_logString( const ICELIB_CALLBACK_LOG *pCallbackLog,
                       ICELIB_logLevel            logLevel,
                       const char                *str);

void FORMAT_CHECK(3,4) ICELIB_logVaString( const ICELIB_CALLBACK_LOG *pCallbackLog,
                                           ICELIB_logLevel            logLevel,
                                           const char                *fmt,
                                           ...);
//-----------------------------------------------------------------------------
//
//----- TEST API Functions
//
void ICELIB_timerConstructor(ICELIB_TIMER *timer,
                             unsigned int tickIntervalMS);
void ICELIB_timerStart(ICELIB_TIMER *timer,
                       unsigned int timeoutMS);
void ICELIB_timerStop(ICELIB_TIMER *timer);
void ICELIB_timerTick(ICELIB_TIMER *timer);
bool ICELIB_timerIsRunning(const ICELIB_TIMER *timer);
bool ICELIB_timerIsTimedOut(const ICELIB_TIMER *timer);


//-----------------------------------------------------------------------------
//
//----- API Functions
//


void ICELIB_Constructor(ICELIB_INSTANCE            *pInstance,
                        const ICELIB_CONFIGURATION *pConfiguration);
void ICELIB_Destructor (ICELIB_INSTANCE *pInstance);

bool ICELIB_Start(ICELIB_INSTANCE *pInstance, bool controlling);
void ICELIB_Stop(ICELIB_INSTANCE *pInstance);

void ICELIB_ReStart(ICELIB_INSTANCE *pInstance);

void ICELIB_Tick(ICELIB_INSTANCE *pInstance);

void ICELIB_setCallbackConnecitivityChecksComplete(ICELIB_INSTANCE                    *pInstance,
                                                   ICELIB_connectivityChecksComplete  pICELIB_connectivityChecksComplete,
                                                   void                               *userData);

void ICELIB_setCallbackOutgoingBindingRequest(ICELIB_INSTANCE                *pInstance,
                                              ICELIB_outgoingBindingRequest   pICELIB_sendBindingRequest,
                                              void                           *pBindingRequestUserData);

void ICELIB_setCallbackOutgoingBindingResponse(ICELIB_INSTANCE                *pInstance,
                                               ICELIB_outgoingBindingResponse  pICELIB_sendBindingResponse,
                                               void                           *pBindingResponseUserData);

void ICELIB_setCallbackOutgoingCancelRequest(ICELIB_INSTANCE                *pInstance,
                                             ICELIB_outgoingCancelRequest   pICELIB_sendBindingCancelRequest,
                                             void                           *pCancelRequestUserData);


void ICELIB_setCallbackKeepAlive(ICELIB_INSTANCE                *pInstance,
                                 ICELIB_sendKeepAlive            pICELIB_sendKeepAlive,
                                 void                           *pUserDataKeepAlive);

void ICELIB_setCallbackPasswordUpdate(ICELIB_INSTANCE                *pInstance,
                                      ICELIB_passwordUpdate           pICELIB_passwordUpdate,
                                      void                           *pUserDataPasswordUpdate);

void ICELIB_setCallbackLog(ICELIB_INSTANCE                *pInstance,
                           ICELIB_logCallback              pICELIB_logCallback,
                           void                           *pLogUserData,
                           ICELIB_logLevel                 logLevel);

void ICELIB_incomingBindingResponse(ICELIB_INSTANCE         *pInstance,
                                     uint16_t                errorResponse,
                                     StunMsgId               transactionId,
                                     const struct sockaddr   *source,         // From response
                                     const struct sockaddr   *destination,    // From response
                                     const struct sockaddr   *mappedAddress);

void ICELIB_incomingTimeout(ICELIB_INSTANCE   *pInstance,
                             StunMsgId          Transactionid);

void ICELIB_incomingBindingRequest(ICELIB_INSTANCE       *pInstance,
                                    uint32_t              userValue1,
                                    uint32_t              userValue2,
                                    const char            *pUfragPair,
                                    uint32_t              peerPriority,
                                    bool                  useCandidate,
                                    bool                  iceControlling,
                                    bool                  iceControlled,
                                    uint64_t              tieBreaker,
                                    StunMsgId             transactionId,
                                    const struct sockaddr *source,
                                    const struct sockaddr *destination,
                                    bool                  fromRelay,
                                    const struct sockaddr *peerAddr,
                                    uint16_t               componentId);

ICE_TURN_STATE ICELIB_getTurnState(const ICELIB_INSTANCE  *pInstance,
                                   uint32_t mediaIdx);

void ICELIB_setTurnState(ICELIB_INSTANCE  *pInstance,
                         uint32_t mediaIdx,
                         ICE_TURN_STATE turnState);

int32_t ICELIB_getNumberOfLocalICEMediaLines(const ICELIB_INSTANCE *pInstance );
int32_t ICELIB_getNumberOfRemoteICEMediaLines(const ICELIB_INSTANCE *pInstance );

int32_t ICELIB_getNumberOfLocalCandidates(const ICELIB_INSTANCE *pInstance, uint32_t idx);
int32_t ICELIB_getNumberOfRemoteCandidates(const ICELIB_INSTANCE *pInstance, uint32_t idx);

const char * ICELIB_getLocalPassword(const ICELIB_INSTANCE *pInstance, uint32_t idx);

uint32_t ICELIB_getLocalComponentId(const ICELIB_INSTANCE *pInstance,
                                    uint32_t mediaIdx,
                                    uint32_t candIdx);

uint32_t ICELIB_getRemoteComponentId(const ICELIB_INSTANCE *pInstance,
                                     uint32_t mediaIdx,
                                     uint32_t candIdx);

struct sockaddr const *ICELIB_getLocalConnectionAddr(const ICELIB_INSTANCE *pInstance,
                                                     uint32_t mediaIdx,
                                                     uint32_t candIdx);

struct sockaddr const *ICELIB_getRemoteConnectionAddr(const ICELIB_INSTANCE *pInstance,
                                                      uint32_t mediaIdx,
                                                      uint32_t candIdx);

ICE_CANDIDATE_TYPE ICELIB_getLocalCandidateType(const ICELIB_INSTANCE *pInstance,
                                                uint32_t mediaIdx,
                                                uint32_t candIdx);

ICE_CANDIDATE_TYPE ICELIB_getRemoteCandidateType(const ICELIB_INSTANCE *pInstance,
                                                 uint32_t mediaIdx,
                                                 uint32_t candIdx);

ICE_MEDIA const *ICELIB_getLocalIceMedia(const ICELIB_INSTANCE *pInstance );


int32_t ICELIB_addRemoteMediaStream(ICELIB_INSTANCE *pInstance,
                                    const char *ufrag,
                                    const char *pwd,
                                    const struct sockaddr *defaultAddr );

int32_t ICELIB_setRemoteMediaStream(ICELIB_INSTANCE *pInstance,
                                    uint32_t mediaIdx,
                                    const char *ufrag,
                                    const char *pwd,
                                    const struct sockaddr *defaultAddr);

int32_t ICELIB_addRemoteCandidate(ICELIB_INSTANCE *pInstance,
                                  uint32_t mediaIdx,
                                  const char *foundation,
                                  uint32_t foundationLen,
                                  uint32_t componentId,
                                  uint32_t priority,
                                  const char *connectionAddr,
                                  uint16_t port,
                                  ICE_CANDIDATE_TYPE candType );

int32_t
ICELIB_updateLocalMediaStreamDefaultCandidate(ICELIB_INSTANCE *pInstance,
                                              uint32_t mediaIdx,
                                              ICE_CANDIDATE_TYPE defaultCandType);


int32_t ICELIB_addLocalMediaStream(ICELIB_INSTANCE *pInstance,
                                   uint32_t userValue1,
                                   uint32_t userValue2,
                                   ICE_CANDIDATE_TYPE defaultCandType );

int32_t ICELIB_setLocalMediaStream(ICELIB_INSTANCE *pInstance,
                                   uint32_t mediaIdx,
                                   uint32_t userValue1,
                                   uint32_t userValue2,
                                   ICE_CANDIDATE_TYPE defaultCandType );


int32_t ICELIB_addLocalCandidate(ICELIB_INSTANCE *pInstance,
                                 uint32_t mediaIdx,
                                 uint32_t componentId,
                                 const struct sockaddr *connectionAddr,
                                 const struct sockaddr *relAddr,
                                 ICE_CANDIDATE_TYPE candType,
                                 uint16_t local_pref);


bool ICELIB_isControlling(const ICELIB_INSTANCE *pInstance);


void ICELIB_checkListDumpAllLog(const ICELIB_CALLBACK_LOG   *pCallbackLog,
                                ICELIB_logLevel              logLevel,
                                const ICELIB_INSTANCE       *pInstance);


bool ICELIB_isRestart(ICELIB_INSTANCE *pInstance, unsigned int mediaIdx,
                      const char *ufrag, const char *passwd);


struct sockaddr const *ICELIB_getLocalRelayAddr(const ICELIB_INSTANCE *pInstance,
                                         uint32_t mediaIdx);

struct sockaddr const *ICELIB_getLocalRelayAddrFromHostAddr(const ICELIB_INSTANCE *pInstance,
                                                            const struct sockaddr *hostAddr);

ICE_CANDIDATE const *ICELIB_getActiveCandidate(const ICELIB_INSTANCE *pInstance,
                                               int mediaLineId,
                                               uint32_t componentId );

ICE_REMOTE_CANDIDATES const *ICELIB_getActiveRemoteCandidates(const ICELIB_INSTANCE *pInstance,
                                                              int mediaLineId);

char *ICELIB_getCheckListRemoteUsernamePair(char                    *dst,
                                            int                      maxlength,
                                            const ICELIB_CHECKLIST  *pCheckList,
                                            bool outgoing);



bool ICELIB_isRunning(const ICELIB_INSTANCE *pInstance);
bool ICELIB_isIceComplete(const ICELIB_INSTANCE *pInstance);
bool ICELIB_isMangled (const ICELIB_INSTANCE *pInstance);


const ICE_MEDIA_STREAM * ICELIB_getLocalMediaStream(const ICELIB_INSTANCE *pInstance, uint32_t mediaIdx);

#ifdef __cplusplus
}
#endif

#endif
