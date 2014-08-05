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

#ifndef ICELIBTYPES_H
#define ICELIBTYPES_H

#include "icelib_defines.h"
#include "stunlib.h"

#include <stdint.h>
#include <netinet/in.h>

struct console_output;

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
#endif


#define ICE_MAX_UFRAG_LENGTH                (256+1) /*zero terminated*/
#define ICE_MAX_PASSWD_LENGTH               (256+1) /*zero terminated*/
#define ICE_MAX_CANDIDATES                  12
#define ICE_MAX_FOUNDATION_LENGTH           (32+1)  /*zero terminated*/
#define ICE_MAX_MEDIALINES                  6
#define ICE_MAX_COMPONENTS                  2


//-----------------------------------------------------------------------------
//
//----- List of effective components in a check list
//
typedef struct {
    unsigned int    numberOfComponents;
    uint32_t        componentIds[ ICELIB_MAX_COMPONENTS];
} ICELIB_COMPONENTLIST;


//
//----- Timer
//
typedef enum {
    ICELIB_timerStopped,
    ICELIB_timerRunning,
    ICELIB_timerTimeout
} ICELIB_timerState;


typedef struct {
    ICELIB_timerState   timerState;
    unsigned int        timeoutValueMS;
    unsigned int        countUpMS;
    unsigned int        tickIntervalMS;
} ICELIB_TIMER;

//
//----- Fifo (Triggered Checks)
//
typedef uint32_t                      ICELIB_FIFO_ELEMENT;
#define ICELIB_FIFO_IS_EMPTY          0
#define ICELIB_FIFO_ELEMENT_REMOVED   0xFFFFFFFF

typedef struct {
    ICELIB_FIFO_ELEMENT elements[ ICELIB_MAX_FIFO_ELEMENTS];
    unsigned int        inIndex;
    unsigned int        outIndex;
    bool                isFull;
} ICELIB_TRIGGERED_FIFO;


typedef struct {
    ICELIB_TRIGGERED_FIFO *fifo;
    unsigned int           index;
    bool                   atEnd;
} ICELIB_TRIGGERED_FIFO_ITERATOR;

typedef enum {
    ICELIB_logDebug   = -1,
    ICELIB_logInfo    = 0,
    ICELIB_logWarning = 1,
    ICELIB_logError   = 2,
    ICELIB_logDisable = 3
} ICELIB_logLevel;


typedef enum {
    ICELIB_Result_None,
    ICELIB_Result_Error,
    ICELIB_Result_CandidatesReady,
    ICELIB_Result_Finished,
    ICELIB_Result_OK
} ICELIB_Result;


typedef struct {
    uint64_t  ms64;
    uint64_t  ls64;
} ICELIB_uint128_t;

//
//----- ICE candidate pair states
//
typedef enum {
    ICELIB_PAIR_IDLE,
    ICELIB_PAIR_PAIRED,
    ICELIB_PAIR_REMOVED,
    ICELIB_PAIR_FROZEN,
    ICELIB_PAIR_WAITING,
    ICELIB_PAIR_INPROGRESS,
    ICELIB_PAIR_SUCCEEDED,
    ICELIB_PAIR_FAILED
} ICELIB_PAIR_STATE;


//
//----- ICE check list states
//
typedef enum {
    ICELIB_CHECKLIST_IDLE,
    ICELIB_CHECKLIST_RUNNING,
    ICELIB_CHECKLIST_COMPLETED,
    ICELIB_CHECKLIST_FAILED
} ICELIB_CHECKLIST_STATE;


//
//----- ICE states
//
typedef enum {
    ICELIB_IDLE,
    ICELIB_RUNNING,
    ICELIB_COMPLETED,
    ICELIB_MANGLED,
    ICELIB_FAILED
} ICELIB_STATE;



typedef enum {
    ICE_CAND_TYPE_NONE,
    ICE_CAND_TYPE_HOST,
    ICE_CAND_TYPE_SRFLX,
    ICE_CAND_TYPE_RELAY,
    ICE_CAND_TYPE_PRFLX
} ICE_CANDIDATE_TYPE;

typedef enum {
    ICE_TURN_IDLE,
    ICE_TURN_ALLOCATING,
    ICE_TURN_ALLOCATED,
    ICE_TURN_SETTING_ACTIVE,
    ICE_TURN_ACTIVE
}ICE_TURN_STATE;


/*!
 * ICE single candidate
 *
 * From draft-ietf-mmusic-ice-18:
 *
 *  foundation      = 1*32 ice-char
 *  componentid     = 1*5 digit  (0..65535)
 *  priority        = 1*10 digit (0..2147483647)
 *  connectionAddr  = address including port
 *  relAddr         = host addres when sending relayed candidates (Optional, used for debugging)
 */
typedef struct {
    char                    foundation[ ICE_MAX_FOUNDATION_LENGTH];
    uint32_t                componentid;
    uint32_t                priority;
    struct sockaddr_storage connectionAddr;
    ICE_CANDIDATE_TYPE      type;
    struct sockaddr_storage relAddr;
    uint32_t                userValue1;
    uint32_t                userValue2;
} ICE_CANDIDATE;

typedef struct {
    uint32_t                componentId;
    struct sockaddr_storage connectionAddr;
    ICE_CANDIDATE_TYPE      type;
} ICE_REMOTE_CANDIDATE;

typedef struct {
    ICE_REMOTE_CANDIDATE remoteCandidate[ICE_MAX_COMPONENTS];
    uint32_t  numberOfComponents;
} ICE_REMOTE_CANDIDATES;

/*!
 * ICE candidates for a single media stream
 */
typedef struct {
    char                    ufrag    [ ICE_MAX_UFRAG_LENGTH];
    char                    passwd   [ ICE_MAX_PASSWD_LENGTH];
    ICE_CANDIDATE           candidate[ ICE_MAX_CANDIDATES];
    uint32_t                numberOfCandidates;
    ICE_TURN_STATE          turnState;
    uint32_t                userValue1;
    uint32_t                userValue2;
    struct sockaddr_storage defaultAddr;
    ICE_CANDIDATE_TYPE      defaultCandType;
} ICE_MEDIA_STREAM;


/*!
 * ICE candidates for all media streams
 */
typedef struct {
    bool                controlling;
    ICE_MEDIA_STREAM    mediaStream[ ICE_MAX_MEDIALINES ];
    uint32_t            numberOfICEMediaLines;
} ICE_MEDIA;

#define ICELIB_MAX_NO_OF_TRANSID 5

//
//----- ICE check list and valid list pair
//
typedef struct {
    ICELIB_PAIR_STATE       pairState;
    uint32_t                pairId;
    uint32_t                refersToPairId;
    bool                    defaultPair;
    bool                    useCandidate;
    bool                    triggeredUseCandidate;
    bool                    nominatedPair;
    bool                    sentUseCandidateAlready;
    uint64_t                pairPriority;
    const ICE_CANDIDATE    *pLocalCandidate;
    const ICE_CANDIDATE    *pRemoteCandidate;
    StunMsgId transactionIdTable[ICELIB_MAX_NO_OF_TRANSID];
    unsigned int numberOfTransactionIds;
    //StunMsgId transactionId;
} ICELIB_LIST_PAIR;

//
//----- Valid List
//
#define ICELIB_MAX_VALID_ELEMENTS  ICELIB_MAX_PAIRS
typedef ICELIB_LIST_PAIR           ICELIB_VALIDLIST_ELEMENT;


typedef struct {
    ICELIB_VALIDLIST_ELEMENT  elements[ ICELIB_MAX_VALID_ELEMENTS];
    unsigned int              numberOfElements;
} ICELIB_LIST_VL;

typedef struct {
    ICELIB_LIST_VL  pairs;
    uint32_t        nextPairId;
    uint32_t        readyToNominateWeighting;
    uint32_t        nominatedPathScore;
} ICELIB_VALIDLIST;


typedef struct {
    ICELIB_VALIDLIST  *pValidList;
    unsigned int       index;
} ICELIB_VALIDLIST_ITERATOR;


//
//----- ICE check list
//
typedef struct {
    unsigned int            id;
    const char             *ufragLocal;
    const char             *ufragRemote;
    const char             *passwdLocal;
    const char             *passwdRemote;
    ICELIB_CHECKLIST_STATE  checkListState;
    bool                    timerRunning;
    bool                    stopChecks; //Almost complete, don't fire of any new checks.
    unsigned int            numberOfPairs;
    ICELIB_LIST_PAIR        checkListPairs[ ICELIB_MAX_PAIRS];
    ICELIB_COMPONENTLIST    componentList;
    uint32_t                nextPairId;
} ICELIB_CHECKLIST;

//
//----- One controller per media stream
//
typedef struct {
    ICELIB_CHECKLIST        checkList;
    ICELIB_VALIDLIST        validList;
    ICELIB_TRIGGERED_FIFO   triggeredChecksFifo;
    ICE_MEDIA_STREAM        discoveredLocalCandidates;
    ICE_MEDIA_STREAM        discoveredRemoteCandidates;
    ICE_REMOTE_CANDIDATES   remoteCandidates;   /*When ICE is complete, the remote
                                                  candidates will be stored here */
} ICELIB_STREAM_CONTROLLER;

void ICELIBTYPES_ICE_MEDIA_STREAM_reset(ICE_MEDIA_STREAM *iceMediaStream);
bool ICELIBTYPES_ICE_MEDIA_STREAM_isEmpty(const ICE_MEDIA_STREAM *iceMediaStream);

void ICELIBTYPES_ICE_MEDIA_reset(ICE_MEDIA *iceMedia);
bool ICELIBTYPES_ICE_MEDIA_isEmpty(const ICE_MEDIA *iceMedia);

char const * ICELIBTYPES_ICE_CANDIDATE_TYPE_toString( const ICE_CANDIDATE_TYPE candidateType);

#ifdef __cplusplus
}
#endif

#endif
