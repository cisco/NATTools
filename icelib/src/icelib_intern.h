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


#ifndef ICELIB_INTERN_H
#define ICELIB_INTERN_H

#include "icelib.h"
#include "icelibtypes.h"


#ifdef __cplusplus
extern "C" {
#endif


#define ICELIB_log1(pCallbacks, level, message, arg1) ICELIB_log_(pCallbacks, \
                                                                  level, \
                                                                  __FUNCTION__, \
                                                                  __FILE__, \
                                                                  __LINE__, \
                                                                  message, \
                                                                  arg1)

#define ICELIB_log(pCallbacks, level, message) ICELIB_log_(pCallbacks,  \
                                                           level,       \
                                                           __FUNCTION__, \
                                                           __FILE__,    \
                                                           __LINE__,    \
                                                           message)


    char *ICELIB_strncpy(char *dst, const char *src, int maxlength);
    char *ICELIB_strncat(char *dst, const char *src, int maxlength);

    uint64_t ICELIB_random64(void);
    ICELIB_uint128_t ICELIB_random128(void);

    int ICELIB_compareTransactionId(const StunMsgId *pid1,
                                    const StunMsgId *pid2);

    bool ICELIB_veryfyICESupportOnStream(const ICELIB_INSTANCE *pInstance,
                                         const ICE_MEDIA_STREAM *stream);

    bool ICELIB_verifyICESupport(const ICELIB_INSTANCE *pInstance,
                                 const ICE_MEDIA *iceRemoteMedia);

    const char *pICELIB_splitUfragPair(const char *pUfragPair, size_t *pColonIndex);

    bool ICELIB_compareUfragPair(const char *pUfragPair,
                                 const char *pUfragLeft,
                                 const char *pUfragRight);

    void ICELIB_logStringBasic(const ICELIB_CALLBACK_LOG *pCallbackLog,
                               ICELIB_logLevel logLevel,
                               const char *str);

    void ICELIB_log_(const ICELIB_CALLBACK_LOG *pCallbackLog,
                     ICELIB_logLevel logLevel,
                     const char *function,
                     const char *file,
                     unsigned int line,
                     const char *fmt, ...);

    void ICELIB_startAllStoppingTimers(ICELIB_INSTANCE *pInstance);
    void ICELIB_tickAllStoppingTimers(ICELIB_INSTANCE *pInstance);

    void ICELIB_longToIcechar(long data, char *b64, int maxLength);
    void ICELIB_createRandomString(char *dst, int maxlength);

    uint32_t ICELIB_calculatePriority(ICE_CANDIDATE_TYPE type,
                                      uint16_t compid, uint16_t local_pref);

    bool ICELIB_isEmptyCandidate(const ICE_CANDIDATE *pCandidate);
    bool ICELIB_isNonValidCandidate(const ICE_CANDIDATE *pCandidate);
    bool ICELIB_isEmptyOrNonValidCandidate(const ICE_CANDIDATE *pCandidate);

    void ICELIB_clearRedundantCandidates(ICE_CANDIDATE candidates[]);
    void ICELIB_compactTable(ICE_CANDIDATE candidates[]);
    int ICELIB_countCandidates(ICE_CANDIDATE candidates[]);

    int ICELIB_eliminateRedundantCandidates(ICE_CANDIDATE candidates[]);

    void ICELIB_EliminateRedundantCandidates(ICELIB_INSTANCE *pInstance);

    void ICELIB_fillLocalCandidate(ICE_CANDIDATE *cand,
                                   uint32_t componentId,
                                   const struct sockaddr *connectionAddr,
                                   const struct sockaddr *relAddr,
                                   ICE_CANDIDATE_TYPE candType,
                                   uint16_t local_pref);

    void ICELIB_fillRemoteCandidate(ICE_CANDIDATE *cand,
                                    uint32_t componentId,
                                    const char* foundation,
                                    uint32_t foundationLen,
                                    uint32_t priority,
                                    struct sockaddr *connectionAddr,
                                    ICE_CANDIDATE_TYPE candType);

    int ICELIB_candidateSort(const void *x, const void *y);

    const char *ICELIB_toString_CheckListState(ICELIB_CHECKLIST_STATE state);
    const char *ICELIB_toString_CheckListPairState(ICELIB_PAIR_STATE state);

    void ICELIB_resetCheckList(ICELIB_CHECKLIST *pCheckList, unsigned int checkListId);
    void ICELIB_resetPair(ICELIB_LIST_PAIR *pPair);
    void ICELIB_resetCandidate(ICE_CANDIDATE *pCandidate);

    int ICELIB_comparePairsCL(const void *cp1, const void *cp2);
    void ICELIB_computePairPriority(ICELIB_LIST_PAIR *pCheckListPair,
                                    bool iceControlling);

    bool ICELIB_isComponentIdPresent(const ICELIB_COMPONENTLIST *pComponentList,
                                     uint32_t componentId);

    bool ICELIB_addComponentId(ICELIB_COMPONENTLIST *pComponentList,
                               uint32_t componentId);

    bool ICELIB_addComponentIdIfUnique(ICELIB_COMPONENTLIST *pComponentList,
                                       uint32_t componentId);

    bool ICELIB_collectEffectiveCompontIds(ICELIB_CHECKLIST *pCheckList);

    void ICELIB_prunePairsReplaceWithBase(ICELIB_CHECKLIST *pCheckList,
                                          const ICE_CANDIDATE *pBbaseServerReflexiveRtp,
                                          const ICE_CANDIDATE *pBaseServerReflexiveRtcp);

    bool ICELIB_prunePairsIsEqual(const ICELIB_LIST_PAIR *pPair1,
                                  const ICELIB_LIST_PAIR *pPair2);

    bool ICELIB_prunePairsIsClear(const ICELIB_LIST_PAIR *pPair);
    void ICELIB_prunePairsClear(ICELIB_LIST_PAIR *pPair);
    void ICELIB_prunePairsClearDuplicates(ICELIB_CHECKLIST *pCheckList);
    void ICELIB_prunePairsCompact(ICELIB_CHECKLIST *pCheckList);
    unsigned int ICELIB_prunePairsCountPairs(ICELIB_LIST_PAIR pairs[]);

    void ICELIB_computeStatesSetState(ICELIB_CHECKLIST *pCheckList,
                                      ICELIB_PAIR_STATE state,
                                      ICELIB_CALLBACK_LOG *pCallbackLog);

    void ICELIB_makeAllCheckLists(ICELIB_INSTANCE *pInstance);

    bool ICELIB_isPairAddressMatchInChecklist(ICELIB_CHECKLIST *pCheckList,
                                              ICELIB_LIST_PAIR *pair);

    ICELIB_LIST_PAIR *ICELIB_getPairById(ICELIB_CHECKLIST *pCheckList,
                                         uint32_t pairId);

    bool ICELIB_isTriggeredFifoPairPresent(ICELIB_TRIGGERED_FIFO *pFifo,
                                           ICELIB_LIST_PAIR *pPair, ICELIB_CALLBACK_LOG *pCallbackLog);

    bool ICELIB_triggeredFifoPutIfNotPresent(ICELIB_TRIGGERED_FIFO *pFifo,
                                             ICELIB_LIST_PAIR *pPair,
                                             ICELIB_CALLBACK_LOG *pCallbackLog);

    void ICELIB_triggeredFifoRemove(ICELIB_TRIGGERED_FIFO *pFifo,
                                    ICELIB_LIST_PAIR *pPair);

    void ICELIB_triggeredfifoIteratorConstructor(ICELIB_TRIGGERED_FIFO_ITERATOR *pIterator,
                                                 ICELIB_TRIGGERED_FIFO *pFifo);

    ICELIB_LIST_PAIR *pICELIB_triggeredfifoIteratorNext(ICELIB_CHECKLIST *pCheckList,
                                                        ICELIB_CALLBACK_LOG *pCallbackLog,
                                                        ICELIB_TRIGGERED_FIFO_ITERATOR *pIterator);

    void ICELIB_resetStreamController(ICELIB_STREAM_CONTROLLER *pStreamController,
                                      unsigned int tickIntervalMS);

    ICELIB_LIST_PAIR *pICELIB_findPairToScedule(ICELIB_STREAM_CONTROLLER *pController,
                                                ICELIB_CALLBACK_LOG *pCallbackLog);

    bool ICELIB_insertTransactionId(ICELIB_LIST_PAIR *pair,
                                    StunMsgId id);

    void ICELIB_scheduleCheck(ICELIB_INSTANCE *pInstance,
                              ICELIB_CHECKLIST *pCheckList,
                              ICELIB_LIST_PAIR *pPair);

    void ICELIB_tickStreamController(ICELIB_INSTANCE *pInstance);

    void ICELIB_unfreezeAllFrozenCheckLists(ICELIB_STREAM_CONTROLLER streamControllers[],
                                            unsigned int numberOfMediaStreams,
                                            ICELIB_CALLBACK_LOG *pCallbackLog);

    void ICELIB_recomputeAllPairPriorities(ICELIB_STREAM_CONTROLLER streamControllers[],
                                           unsigned int numberOfMediaStreams,
                                           bool iceControlling);

    unsigned int ICELIB_findStreamByAddress(ICELIB_STREAM_CONTROLLER streamControllers[],
                                            unsigned int numberOfMediaStreams,
                                            const struct sockaddr *pHostAddr);

    int ICELIB_listCompareVL(const void *cp1, const void *cp2);
    void ICELIB_listSortVL(ICELIB_LIST_VL *pList);
    void ICELIB_listConstructorVL(ICELIB_LIST_VL *pList);
    unsigned int ICELIB_listCountVL(const ICELIB_LIST_VL *pList);

    ICELIB_VALIDLIST_ELEMENT *pICELIB_listGetElementVL(ICELIB_LIST_VL *pList,
                                                       unsigned int index);
    bool ICELIB_listAddBackVL(ICELIB_LIST_VL *pList,
                              ICELIB_VALIDLIST_ELEMENT *pPair);

    bool ICELIB_listInsertVL(ICELIB_LIST_VL *pList,
                             ICELIB_VALIDLIST_ELEMENT *pPair);

    void ICELIB_validListConstructor(ICELIB_VALIDLIST *pValidList);

    ICELIB_VALIDLIST_ELEMENT *pICELIB_validListGetElement(ICELIB_VALIDLIST *pValidList,
                                                          unsigned int index);

    uint32_t ICELIB_validListLastPairId(const ICELIB_VALIDLIST *pValidList);

    void ICELIB_validListIteratorConstructor(ICELIB_VALIDLIST_ITERATOR *pIterator,
                                             ICELIB_VALIDLIST *pValidList);

    ICELIB_VALIDLIST_ELEMENT *pICELIB_validListIteratorNext(ICELIB_VALIDLIST_ITERATOR *pIterator);
    ICELIB_VALIDLIST_ELEMENT *pICELIB_validListFindPairById(ICELIB_VALIDLIST *pValidList,
                                                            uint32_t pairId);

    void ICELIB_storeRemoteCandidates(ICELIB_INSTANCE *pInstance);

    bool ICELIB_validListNominatePair(ICELIB_VALIDLIST *pValidList,
                                      ICELIB_LIST_PAIR *pPair,
                                      const struct sockaddr *pMappedAddress);

    bool ICELIB_validListAddBack(ICELIB_VALIDLIST *pValidList,
                                 ICELIB_VALIDLIST_ELEMENT *pPair);

    bool ICELIB_validListInsert(ICELIB_VALIDLIST *pValidList,
                                ICELIB_VALIDLIST_ELEMENT *pPair);

    unsigned int ICELIB_countNominatedPairsInValidList(ICELIB_VALIDLIST *pValidList);

    ICELIB_VALIDLIST_ELEMENT *ICELIB_findElementInValidListByid(ICELIB_VALIDLIST *pValidList,
                                                                uint32_t id);

    bool ICELIB_isPairForEachComponentInValidList(ICELIB_VALIDLIST *pValidList,
                                                  const ICELIB_COMPONENTLIST *pComponentList);

    void ICELIB_unfreezePairsByMatchingFoundation(ICELIB_VALIDLIST *pValidList,
                                                  ICELIB_CHECKLIST *pCheckList,
                                                  ICELIB_CALLBACK_LOG *pCallbackLog);

    bool ICELIB_atLeastOneFoundationMatch(ICELIB_VALIDLIST *pValidList,
                                          ICELIB_CHECKLIST *pCheckList);

    void ICELIB_resetCallbacks(ICELIB_CALLBACKS *pCallbacks);

    void ICELIB_callbackConstructor(ICELIB_CALLBACKS *pCallbacks,
                                    ICELIB_INSTANCE *pInstance);

    bool ICELIB_checkSourceAndDestinationAddr(const ICELIB_LIST_PAIR *pPair,
                                              const struct sockaddr *source, const struct sockaddr *destination);

    ICE_CANDIDATE *ICELIB_addDiscoveredCandidate(ICE_MEDIA_STREAM *pMediaStream,
                                                 const ICE_CANDIDATE *pPeerCandidate);

    void ICELIB_makePeerLocalReflexiveCandidate(ICE_CANDIDATE *pPeerCandidate,
                                                ICELIB_CALLBACK_LOG *pCallbackLog,
                                                const struct sockaddr *pMappedAddress,
                                                uint16_t componentId);

    void ICELIB_makePeerRemoteReflexiveCandidate(ICE_CANDIDATE *pPeerCandidate,
                                                 ICELIB_CALLBACK_LOG *pCallbackLog,
                                                 const struct sockaddr *sourceAddr,
                                                 uint32_t peerPriority,
                                                 uint16_t componentId);

    ICELIB_LIST_PAIR *pICELIB_findPairInCheckList(ICELIB_CHECKLIST *pCheckList,
                                                  const ICELIB_LIST_PAIR *pPair);

    bool ICELIB_isAllPairsFailedOrSucceded(const ICELIB_CHECKLIST *pCheckList);

    void ICELIB_updateCheckListState(ICELIB_CHECKLIST *pCheckList,
                                     ICELIB_VALIDLIST *pValidList,
                                     ICELIB_STREAM_CONTROLLER streamControllers[],
                                     unsigned int numberOfMediaStreams,
                                     ICELIB_CALLBACK_LOG *pCallbackLog);

    void ICELIB_processSuccessResponse(ICELIB_INSTANCE *pInstance,
                                       const ICE_MEDIA_STREAM *pLocalMediaStream,
                                       ICE_MEDIA_STREAM *pDiscoveredLocalCandidates,
                                       ICELIB_CHECKLIST *pCurrentCheckList,
                                       ICELIB_VALIDLIST *pValidList,
                                       ICELIB_LIST_PAIR *pPair,
                                       const struct sockaddr *pMappedAddress,
                                       bool iceControlling);

    void ICELIB_sendBindingResponse(ICELIB_INSTANCE *pInstance,
                                    const struct sockaddr *source,
                                    const struct sockaddr *destination,
                                    const struct sockaddr *MappedAddress,
                                    uint32_t userValue1,
                                    uint32_t userValue2,
                                    uint16_t componentId,
                                    uint16_t errorResponse,
                                    StunMsgId transactionId,
                                    bool useRelay,
                                    const char *pUfragPair,
                                    const char *pPasswd);

    void ICELIB_processSuccessRequest(ICELIB_INSTANCE *pInstance,
                                      StunMsgId transactionId,
                                      const struct sockaddr *source,
                                      const struct sockaddr *destination,
                                      const struct sockaddr *relayBaseAddr,
                                      uint32_t userValue1,
                                      uint32_t userValue2,
                                      uint32_t peerPriority,
                                      const ICE_MEDIA_STREAM *pLocalMediaStream,
                                      const ICE_MEDIA_STREAM *pRemoteMediaStream,
                                      ICE_MEDIA_STREAM *pDiscoveredRemoteCandidates,
                                      ICE_MEDIA_STREAM *pDiscoveredLocalCandidates,
                                      ICELIB_CHECKLIST *pCurrentCheckList,
                                      ICELIB_VALIDLIST *pCurrentValidList,
                                      ICELIB_TRIGGERED_FIFO *pTriggeredFifo,
                                      bool iceControlling,
                                      bool useCandidate,
                                      bool fromRelay,
                                      uint16_t componentId);

    void ICELIB_processIncommingLite(ICELIB_INSTANCE *pInstance,
                                     uint32_t userValue1,
                                     uint32_t userValue2,
                                     const char *pUfragPair,
                                     uint32_t peerPriority,
                                     bool useCandidate,
                                     bool iceControlling,
                                     bool iceControlled,
                                     uint64_t tieBreaker,
                                     StunMsgId transactionId,
                                     const struct sockaddr *source,
                                     const struct sockaddr *destination,
                                     uint16_t componentId);

    void ICELIB_processIncommingFull(ICELIB_INSTANCE *pInstance,
                                     uint32_t userValue1,
                                     uint32_t userValue2,
                                     const char *pUfragPair,
                                     uint32_t peerPriority,
                                     bool useCandidate,
                                     bool iceControlling,
                                     bool iceControlled,
                                     uint64_t tieBreaker,
                                     StunMsgId transactionId,
                                     const struct sockaddr *source,
                                     const struct sockaddr *destination,
                                     bool fromRelay,
                                     const struct sockaddr *relayBaseAddr,
                                     uint16_t componentId);

    bool ICELIB_isNominatingCriteriaMet(ICELIB_VALIDLIST *pValidList);
    bool ICELIB_isNominatingCriteriaMetForAllMediaStreams(ICELIB_INSTANCE *pInstance);

    void ICELIB_stopChecks(ICELIB_INSTANCE *pInstance,
                           ICELIB_CHECKLIST *pCheckList,
                           ICELIB_TRIGGERED_FIFO *pTriggeredChecksFifo);

    ICELIB_LIST_PAIR *pICELIB_pickValidPairForNominationNormalMode(ICELIB_VALIDLIST *pValidList,
                                                                   uint32_t componentId);

    ICELIB_LIST_PAIR *pICELIB_pickValidPairForNomination(ICELIB_INSTANCE *pInstance,
                                                         ICELIB_VALIDLIST *pValidList,
                                                         ICELIB_CHECKLIST *pCheckList,
                                                         uint32_t componentId);

    void ICELIB_enqueueValidPair(ICELIB_TRIGGERED_FIFO *pTriggeredChecksFifo,
                                 ICELIB_CHECKLIST *pCheckList,
                                 ICELIB_CALLBACK_LOG *pCallbackLog,
                                 ICELIB_LIST_PAIR *pValidPair);

    void ICELIB_nominateAggressive(ICELIB_INSTANCE *pInstance);
    void ICELIB_nominateRegularIfComplete(ICELIB_INSTANCE *pInstance);

    void ICELIB_removePairFromCheckList(ICELIB_CHECKLIST *pCheckList,
                                        ICELIB_LIST_PAIR *pPair,
                                        ICELIB_CALLBACK_LOG *pCallbackLog);

    void ICELIB_removeWaitingAndFrozenByComponentFromCheckList(ICELIB_CHECKLIST *pCheckList,
                                                               uint32_t componentId,
                                                               ICELIB_CALLBACK_LOG *pCallbackLog);

    void ICELIB_removeWaitingAndFrozenByComponentFromTriggeredChecksFifo(ICELIB_CHECKLIST *pCheckList,
                                                                         ICELIB_TRIGGERED_FIFO *pTriggeredChecksFifo,
                                                                         ICELIB_CALLBACK_LOG *pCallbackLog,
                                                                         uint32_t componentId);

    void ICELIB_removeWaitingAndFrozen(ICELIB_CHECKLIST *pCheckList,
                                       ICELIB_VALIDLIST *pValidList,
                                       ICELIB_TRIGGERED_FIFO *pTriggeredChecksFifo,
                                       ICELIB_CALLBACK_LOG *pCallbackLog);

    void ICELIB_ceaseRetransmissions(ICELIB_CHECKLIST *pCheckList,
                                     ICELIB_VALIDLIST *pValidList,
                                     ICELIB_TRIGGERED_FIFO *pTriggeredChecksFifo);

    void ICELIB_updateCheckListStateConcluding(ICELIB_CHECKLIST *pCheckList,
                                               ICELIB_VALIDLIST *pValidList,
                                               ICELIB_TRIGGERED_FIFO *pTriggeredChecksFifo,
                                               ICELIB_CALLBACK_LOG *pCallbackLog);

    void ICE_concludeFullIfComplete(ICELIB_INSTANCE *pInstance);
    void ICE_concludingLite(ICELIB_INSTANCE *pInstance);
    void ICELIB_concludeICEProcessingIfComplete(ICELIB_INSTANCE *pInstance);

    void ICELIB_resetIceInstance(ICELIB_INSTANCE *pInstance);
    void ICELIB_resetAllStreamControllers(ICELIB_INSTANCE *pInstance);

    uint32_t ICELIB_getWeightTimeMultiplier(ICELIB_INSTANCE *pInstance);

    void ICELIB_updateValidPairReadyToNominateWeightingMediaStream(ICELIB_CHECKLIST *pCheckList,
                                                                   ICELIB_VALIDLIST *pValidList,
                                                                   uint32_t timeMultiplier);

    void ICELIB_updateValidPairReadyToNominateWeighting(ICELIB_INSTANCE *pInstance);

    void ICELIB_PasswordUpdate(ICELIB_INSTANCE *pInstance);

    unsigned int ICELIB_numberOfMediaStreams(ICELIB_INSTANCE *pInstance);

    void ICELIB_doKeepAlive(ICELIB_INSTANCE *pInstance);

    StunMsgId ICELIB_generateTransactionId(void);

    char *ICELIB_makeUsernamePair(char       *dst,
                                  int         maxlength,
                                  const char *ufrag1,
                                  const char *ufrag2);

    uint64_t ICELIB_makeTieBreaker(void);

    void ICELIB_createUfrag (char *dst, int maxLength);
    void ICELIB_createPasswd(char *dst, int maxLength);

    void ICELIB_createFoundation(char *dst,
                                 ICE_CANDIDATE_TYPE type,
                                 int maxlength);

    void ICELIB_removChecksFromCheckList(ICELIB_CHECKLIST *pCheckList );

    void ICELIB_resetCheckList(ICELIB_CHECKLIST *pCheckList,
                               unsigned int checkListId);

    void ICELIB_resetCandidate(ICE_CANDIDATE    *pCandidate);

    void ICELIB_saveUfragPasswd(ICELIB_CHECKLIST *pCheckList,
                                const ICE_MEDIA_STREAM *pLocalMediaStream,
                                const ICE_MEDIA_STREAM *pRemoteMediaStream);

    void ICELIB_formPairs(ICELIB_CHECKLIST       *pCheckList,
                          ICELIB_CALLBACK_LOG    *pCallbackLog,
                          const ICE_MEDIA_STREAM *pLocalMediaStream,
                          const ICE_MEDIA_STREAM *pRemoteMediaStream,
                          unsigned int           maxPairs);

    uint64_t ICELIB_pairPriority(uint32_t G, uint32_t D);

    void ICELIB_computeListPairPriority(ICELIB_CHECKLIST *pCheckList,
                                        bool              iceControlling);

    void ICELIB_sortPairsCL(ICELIB_CHECKLIST *pCheckList);

    bool ICELIB_findReflexiveBaseAddresses(const ICE_CANDIDATE **ppBaseServerReflexiveRtp,
                                           const ICE_CANDIDATE **ppBaseServerReflexiveRtcp,
                                           const ICE_MEDIA_STREAM *pLocalMediaStream);

    void ICELIB_prunePairs(ICELIB_CHECKLIST *pCheckList,
                           const ICE_CANDIDATE *pBbaseServerReflexiveRtp,
                           const ICE_CANDIDATE *pBaseServerReflexiveRtcp);

    void ICELIB_computeStatesSetWaitingFrozen(ICELIB_CHECKLIST *pCheckList,
                                              ICELIB_CALLBACK_LOG *pCallbackLog);

    bool ICELIB_makeCheckList(ICELIB_CHECKLIST       *pCheckList,
                              ICELIB_CALLBACK_LOG    *pCallbackLog,
                              const ICE_MEDIA_STREAM *pLocalMediaStream,
                              const ICE_MEDIA_STREAM *pRemoteMediaStream,
                              bool                   iceControlling,
                              unsigned int           maxPairs,
                              unsigned int           checkListId);

    bool ICELIB_insertIntoCheckList(ICELIB_CHECKLIST  *pCheckList,
                                    ICELIB_LIST_PAIR  *pPair);

    ICELIB_LIST_PAIR *pICELIB_findPairByState(ICELIB_CHECKLIST *pCheckList,
                                              ICELIB_PAIR_STATE pairState);

    ICELIB_LIST_PAIR *pICELIB_chooseOrdinaryPair(ICELIB_CHECKLIST *pCheckList);

    bool ICELIB_isActiveCheckList(const ICELIB_CHECKLIST *pCheckList);
    bool ICELIB_isFrozenCheckList(const ICELIB_CHECKLIST *pCheckList);

    void ICELIB_unfreezeFrozenCheckList(  ICELIB_CHECKLIST *pCheckList, ICELIB_CALLBACK_LOG *pCallbackLog);
    void ICELIB_unfreezePairsByFoundation(ICELIB_CHECKLIST *pCheckList,
                                          const char pairFoundationToMatch[],
                                          ICELIB_CALLBACK_LOG *pCallbackLog);

    void         ICELIB_fifoClear(        ICELIB_TRIGGERED_FIFO *pFifo);
    unsigned int ICELIB_fifoCount(  const ICELIB_TRIGGERED_FIFO *pFifo);
    bool         ICELIB_fifoIsEmpty(const ICELIB_TRIGGERED_FIFO *pFifo);
    bool         ICELIB_fifoIsFull( const ICELIB_TRIGGERED_FIFO *pFifo);
    bool         ICELIB_fifoPut(          ICELIB_TRIGGERED_FIFO *pFifo,
                                          ICELIB_FIFO_ELEMENT element);
    ICELIB_FIFO_ELEMENT ICELIB_fifoGet(   ICELIB_TRIGGERED_FIFO *pFifo);

    void ICELIB_fifoIteratorConstructor(ICELIB_TRIGGERED_FIFO_ITERATOR *pIterator,
                                        ICELIB_TRIGGERED_FIFO          *pFifo);
    ICELIB_FIFO_ELEMENT *pICELIB_fifoIteratorNext(ICELIB_TRIGGERED_FIFO_ITERATOR *pIterator);

    void         ICELIB_triggeredFifoClear(         ICELIB_TRIGGERED_FIFO *pFifo);
    unsigned int ICELIB_triggeredFifoCount(   const ICELIB_TRIGGERED_FIFO *pFifo);
    bool         ICELIB_triggeredFifoIsEmpty( const ICELIB_TRIGGERED_FIFO *pFifo);
    bool         ICELIB_triggeredFifoIsFull(  const ICELIB_TRIGGERED_FIFO *pFifo);
    bool         ICELIB_triggeredFifoPut(           ICELIB_TRIGGERED_FIFO *pFifo,
                                                    ICELIB_LIST_PAIR      *pPair);
    ICELIB_LIST_PAIR *pICELIB_triggeredFifoGet(ICELIB_CHECKLIST      *pCheckList,
                                               ICELIB_CALLBACK_LOG   *pCallbackLog,
                                               ICELIB_TRIGGERED_FIFO *pFifo);


    bool ICELIB_scheduleSingle(ICELIB_INSTANCE          *pInstance,
                               ICELIB_STREAM_CONTROLLER *pController,
                               ICELIB_CALLBACK_LOG      *pCallbackLog);


    char *ICELIB_getCheckListLocalUsernamePair(char                   *dst,
                                               int                     maxlength,
                                               const ICELIB_CHECKLIST *pCheckList);

    char *ICELIB_getCheckListRemoteUsernamePair(char                   *dst,
                                                int                     maxlength,
                                                const ICELIB_CHECKLIST *pCheckList,
                                                bool                    outgoing);

    const char *ICELIB_getCheckListRemotePasswd(const ICELIB_CHECKLIST *pCheckList);
    const char *ICELIB_getCheckListLocalPasswd(const ICELIB_CHECKLIST *pCheckList);

    bool ICELIB_isPairAddressMatch(const ICELIB_LIST_PAIR *pPair1,
                                   const ICELIB_LIST_PAIR *pPair2);

    ICELIB_LIST_PAIR *pICELIB_correlateToRequest(unsigned int       *pStreamIndex,
                                                 ICELIB_INSTANCE    *pInstance,
                                                 const StunMsgId    *pTransactionId);

    unsigned int ICELIB_validListCount(const ICELIB_VALIDLIST *pValidList);

    void ICELIB_updatingStates(ICELIB_INSTANCE *pInstance);

    uint32_t ICELIB_getCandidateTypeWeight(ICE_CANDIDATE_TYPE type);

    uint32_t ICELIB_calculateReadyWeight(ICE_CANDIDATE_TYPE localType,
                                         ICE_CANDIDATE_TYPE remoteType,
                                         uint32_t timeMultiplier);

    const ICE_CANDIDATE *pICELIB_findCandidate(const ICE_MEDIA_STREAM *pMediaStream,
                                               const struct sockaddr         *address);

    void ICELIB_validListIteratorConstructor(ICELIB_VALIDLIST_ITERATOR *pIterator,
                                             ICELIB_VALIDLIST          *pValidList);
    ICELIB_VALIDLIST_ELEMENT *pICELIB_validListIteratorNext(ICELIB_VALIDLIST_ITERATOR *pIterator);
    const char *ICELIB_toString_CheckListPairState(ICELIB_PAIR_STATE state);


    char *ICELIB_getPairFoundation(char                   *dst,
                                   int                     maxlength,
                                   const ICELIB_LIST_PAIR *pPair);


    void ICELIB_netAddrDumpLog(      const ICELIB_CALLBACK_LOG   *pCallbackLog,
                                     ICELIB_logLevel              logLevel,
                                     const struct sockaddr        *netAddr);
    void ICELIB_transactionIdDumpLog(const ICELIB_CALLBACK_LOG   *pCallbackLog,
                                     ICELIB_logLevel              logLevel,
                                     StunMsgId                    tId);
    void ICELIB_candidateDumpLog(    const ICELIB_CALLBACK_LOG   *pCallbackLog,
                                     ICELIB_logLevel              logLevel,
                                     const ICE_CANDIDATE         *candidate);
    void ICELIB_componentIdsDumpLog( const ICELIB_CALLBACK_LOG   *pCallbackLog,
                                     ICELIB_logLevel              logLevel,
                                     const ICELIB_COMPONENTLIST  *pComponentList);
    void ICELIB_pairDumpLog(         const ICELIB_CALLBACK_LOG   *pCallbackLog,
                                     ICELIB_logLevel              logLevel,
                                     const ICELIB_LIST_PAIR      *pPair);
    void ICELIB_checkListDumpLog(    const ICELIB_CALLBACK_LOG *pCallbackLog,
                                     ICELIB_logLevel            logLevel,
                                     const ICELIB_CHECKLIST    *pCheckList);
    void ICELIB_checkListDumpLogLog( const ICELIB_CALLBACK_LOG   *pCallbackLog,
                                     ICELIB_logLevel              logLevel,
                                     const ICELIB_CHECKLIST      *pCheckList);
    void ICELIB_validListDumpLog(    const ICELIB_CALLBACK_LOG   *pCallbackLog,
                                     ICELIB_logLevel              logLevel,
                                     ICELIB_VALIDLIST            *pValidList);

    void ICELIB_netAddrDump(const struct sockaddr              *netAddr);
    void ICELIB_transactionIdDump(StunMsgId                   tId);
    void ICELIB_candidateDump(const ICE_CANDIDATE         *candidate);
    void ICELIB_componentIdsDump(const ICELIB_COMPONENTLIST  *pComponentList);
    void ICELIB_checkListDumpPair(const ICELIB_LIST_PAIR      *pPair);
    void ICELIB_checkListDump(const ICELIB_CHECKLIST      *pCheckList);
    void ICELIB_checkListDumpAll(const ICELIB_INSTANCE       *pInstance);
    void ICELIB_validListDump(ICELIB_VALIDLIST            *pValidList);

#ifdef __cplusplus
}
#endif


#endif  /* ICELIB_INTERN_H */
