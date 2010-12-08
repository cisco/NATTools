#include "icelib.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "sockaddr_util.h"
#include "icelib_intern.h"


#if !defined(max)
#define max(a, b)       ((a) > (b) ? (a) : (b))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#endif


typedef union {
    ICELIB_uint128_t   ui128;
    uint8_t            ba[ 16];
} ui128byteArray_t;


//-----------------------------------------------------------------------------
//
//===== Additional debug
//
static const char *state2char4[] = { "IDLE", "PAIR", "REMD", "FROZ", "WAIT", "PROG", "SUCC", "FAIL" };
static const char *type2char2[] = { "--", "HO", "SX", "RY", "PX" };

static void debug_pair(ICELIB_LIST_PAIR *pair, const char *msg)
{
    char local[22], remote[22];
    sockaddr_toString((struct sockaddr *)&pair->pLocalCandidate->connectionAddr, local, sizeof(local), true);
    sockaddr_toString((struct sockaddr *)&pair->pRemoteCandidate->connectionAddr, remote, sizeof(remote), true);
    printf("   %2d: %s %21s (%s %d) - %21s (%s %d) %s\n", pair->pairId,
           state2char4[pair->pairState],
           local,
           type2char2[pair->pLocalCandidate->type],
           pair->pLocalCandidate->componentid,
           remote,
           type2char2[pair->pRemoteCandidate->type],
           pair->pRemoteCandidate->componentid,
           msg);
}

static void debug(ICELIB_CHECKLIST *checkList,
                  uint32_t pairIdWithMsg,
                  const char *msg)
{
    uint32_t i;
    ICELIB_LIST_PAIR *pair;
    for (i = 0; i < checkList->numberOfPairs; i++) {
        pair = &checkList->checkListPairs[i];
        debug_pair(pair, pair->pairId == pairIdWithMsg ? msg : "");
    }
    printf("\n");
}


//
//----- The length of the destination string, "maxlength", includes
//      the terminating null character (i.e. maxlength=5 yields
//      4 characters and a '\0').
//
char *ICELIB_strncpy(char *dst, const char *src, int maxlength)
{
    if (maxlength-- == 0) return dst;
    strncpy(dst, src, maxlength);
    dst[ maxlength] = '\0';

    return dst;
}


//
//----- The length of the destination string, "maxlength", includes
//      the terminating null character (i.e. maxlength=5 yields
//      4 characters and a '\0').
//
char *ICELIB_strncat(char *dst, const char *src, int maxlength)
{
    int n;

    if (maxlength-- == 0) return dst;
    n = strlen(dst);
    if (n >= maxlength)   return dst;
    strncat(dst, src, maxlength - n);

    return dst;
}


uint64_t ICELIB_random64(void)
{
    uint64_t p2 = ((uint64_t)rand() << 62) & 0xc000000000000000LL;
    uint64_t p1 = ((uint64_t)rand() << 31) & 0x3fffffff80000000LL;
    uint64_t p0 = ((uint64_t)rand() <<  0) & 0x000000007fffffffLL;

    return p2 | p1 | p0;
}


ICELIB_uint128_t ICELIB_random128(void)
{
    ICELIB_uint128_t  result;

    result.ms64 = ICELIB_random64();
    result.ls64 = ICELIB_random64();

    return result;
}


StunMsgId ICELIB_generateTransactionId(void)
{
    StunMsgId           tId;
    ui128byteArray_t    ui128byteArray;

    ui128byteArray.ui128 = ICELIB_random128();

    memcpy(&tId.octet[ 0], &ui128byteArray.ba[ 0], STUN_MSG_ID_SIZE);

    return tId;
}

int ICELIB_compareTransactionId(const StunMsgId *pid1,
                                const StunMsgId *pid2)
{
    return memcmp(pid1, pid2,  STUN_MSG_ID_SIZE);
}



uint64_t ICELIB_makeTieBreaker(void)
{
    return ICELIB_random64();
}


//
//----- Create a 'username credential' string for a check list.
//      The length of the string, "maxlength", includes the terminating
//      null character (i.e. maxlength=5 yields 4 characters and a '\0').
//
char *ICELIB_makeUsernamePair(char       *dst,
                              int         maxlength,
                              const char *ufrag1,
                              const char *ufrag2)
{
    if ((ufrag1 != NULL) && (ufrag2 != NULL)) {
        ICELIB_strncpy(dst, ufrag1, maxlength);
        ICELIB_strncat(dst, ":", maxlength);
        ICELIB_strncat(dst, ufrag2, maxlength);
    } else {
        ICELIB_strncpy(dst, "--no_ufrags--", maxlength);
    }

    return dst;
}

const ICE_CANDIDATE *pICELIB_findCandidate(const ICE_MEDIA_STREAM *pMediaStream,
                                           const struct sockaddr         *address)
{
    uint32_t i;

    for (i=0; i < pMediaStream->numberOfCandidates; ++i) {
        if (sockaddr_alike((struct sockaddr *)&pMediaStream->candidate[ i].connectionAddr,
                           (struct sockaddr *)address)) {
            return &pMediaStream->candidate[ i];
        }
    }

    return NULL;
}


//
//----- "Split" an Ufrag pair around the ':'
//
//      Return: pointer to ufrag - pointer to the right part the pair
//              NULL             - no ':' found in the input string
//
//      Also: ColonIndex set to the index of the ':', which also happens
//      to be the length of the left part of the pair...
//
const char *pICELIB_splitUfragPair(const char *pUfragPair, size_t *pColonIndex)
{
    const char *pColon;

    pColon = strchr(pUfragPair, ':');

    if (pColon      == NULL) return NULL;
    if (pColonIndex != NULL) *pColonIndex = pColon - pUfragPair;

    return pColon + 1;
}


//
//----- Compare a username fragment pair to a set of local and remote fragments.
//      According to the ICE spec the pair must be constructed by a remote and
//      a local user name fragment separated by a ':'.
//
//      Return: true    - equal
//              false   - not equal
//
bool ICELIB_compareUfragPair(const char *pUfragPair,
                             const char *pUfragLeft,
                             const char *pUfragRight)
{
    const char *pSecondPart;
    size_t colonIndex;

    pSecondPart = pICELIB_splitUfragPair(pUfragPair, &colonIndex);
    if (pSecondPart == NULL) return false;

    if (colonIndex != strlen(pUfragLeft)) return false;
    if (strncmp(pUfragLeft,  pUfragPair, colonIndex)  != 0) return false;
    if (strcmp( pUfragRight, pSecondPart) != 0) return false;

    return true;
}



#define ICELIB_log(pCallbacks, level, message) ICELIB_log_(pCallbacks,  \
                                                           level,       \
                                                           __FUNCTION__, \
                                                           __FILE__,    \
                                                           __LINE__,    \
                                                           message)
#define ICELIB_log1(pCallbacks, level, message, arg1) ICELIB_log_(pCallbacks, \
                                                                  level, \
                                                                  __FUNCTION__, \
                                                                  __FILE__, \
                                                                  __LINE__, \
                                                                  message, \
                                                                  arg1)


static void ICELIB_changePairState(ICELIB_LIST_PAIR    *pPair,
                                   ICELIB_PAIR_STATE    newState,
                                   ICELIB_CALLBACK_LOG *pCallbackLog)
{
    ICELIB_logVaString(pCallbackLog,
                       ICELIB_logDebug,
                       "Pair 0x%p changing state old=%s new=%s\n", pPair,
                       ICELIB_toString_CheckListPairState(pPair->pairState),
                       ICELIB_toString_CheckListPairState(newState));
    pPair->pairState = newState;
}

void ICELIB_logStringBasic(const ICELIB_CALLBACK_LOG *pCallbackLog,
                           ICELIB_logLevel            logLevel,
                           const char                *str)
{
    ICELIB_logCallback LogFunction;

    if (pCallbackLog != NULL) {

        LogFunction = pCallbackLog->pICELIB_logCallback;

        if (LogFunction != NULL) {
            LogFunction(pCallbackLog->pLogUserData, logLevel, str);
        } else {
            printf("%s",str);
        }
    } else {
        printf("%s",str);
    }
}


void ICELIB_logString(const ICELIB_CALLBACK_LOG *pCallbackLog,
                      ICELIB_logLevel            logLevel,
                      const char                *str)
{
    ICELIB_logLevel logLevelConfig = ICELIB_logDebug;

    if (pCallbackLog != NULL) {
        if (pCallbackLog->pInstance != NULL) {
            logLevelConfig = pCallbackLog->pInstance->iceConfiguration.logLevel;
        }
    }

    if (logLevel >= logLevelConfig) {
        ICELIB_logStringBasic(pCallbackLog, logLevel, str);
    }
}


void FORMAT_CHECK(3,4) ICELIB_logVaString(const ICELIB_CALLBACK_LOG *pCallbackLog,
                                          ICELIB_logLevel            logLevel,
                                          const char                *fmt,
                                          ...)
{
    va_list  args;
    char     str[ ICE_MAX_DEBUG_STRING + 1];

    va_start(args, fmt);
    vsnprintf(str, ICE_MAX_DEBUG_STRING, fmt, args);
    str[ ICE_MAX_DEBUG_STRING] = '\0';
    va_end(args);

    ICELIB_logString(pCallbackLog, logLevel, str);
}


void FORMAT_CHECK(6,7) ICELIB_log_(const ICELIB_CALLBACK_LOG *pCallbackLog,
                                   ICELIB_logLevel            logLevel,
                                   const char                *function,
                                   const char                *file,
                                   unsigned int               line,
                                   const char                *fmt,
                                   ...)
{
    va_list  args;
    char     str1[ ICE_MAX_DEBUG_STRING + 1];
    char     str2[ ICE_MAX_DEBUG_STRING + 1];

    switch(logLevel) {
        case ICELIB_logDebug  : strncpy(str1, "-D- ", 5);
            break;
        case ICELIB_logInfo   : strncpy(str1, "-I- ", 5);
            break;
        case ICELIB_logWarning: strncpy(str1, "-W- ", 5);
            break;
        case ICELIB_logError  : strncpy(str1, "-E- ", 5);
            break;
        default:                strncpy(str1, "-?- ", 5);
    }

    va_start(args, fmt);

    snprintf(str2, ICE_MAX_DEBUG_STRING, "%s - '%s' (%u): ", function, file, line);
    ICELIB_strncat(str1, str2, ICE_MAX_DEBUG_STRING);

    vsnprintf(str2, ICE_MAX_DEBUG_STRING, fmt, args);
    str2[ ICE_MAX_DEBUG_STRING] = '\0';

    ICELIB_strncat(str1, str2, ICE_MAX_DEBUG_STRING);
    ICELIB_strncat(str1, "\n", ICE_MAX_DEBUG_STRING);

    va_end(args);

    ICELIB_logString(pCallbackLog, logLevel, str1);
}


void ICELIB_timerConstructor(ICELIB_TIMER *pTimer,
                             unsigned int tickIntervalMS)
{
    memset(pTimer, 0, sizeof(*pTimer));
    pTimer->tickIntervalMS = tickIntervalMS;
    pTimer->countUpMS      = 0;
    pTimer->timerState     = ICELIB_timerStopped;
}


void ICELIB_timerStart(ICELIB_TIMER *pTimer,
                       unsigned int timeoutMS)
{
    pTimer->timeoutValueMS = timeoutMS;
    pTimer->countUpMS      = 0;
    pTimer->timerState     = ICELIB_timerRunning;
}


void ICELIB_timerStop(ICELIB_TIMER *pTimer)
{
    pTimer->timerState = ICELIB_timerStopped;
}


void ICELIB_timerTick(ICELIB_TIMER *pTimer)
{
    if (pTimer->timerState == ICELIB_timerRunning) {

        pTimer->countUpMS += pTimer->tickIntervalMS;

        if (pTimer->countUpMS >= pTimer->timeoutValueMS) {
            pTimer->timerState = ICELIB_timerTimeout;
        }
    }
}


bool ICELIB_timerIsRunning(ICELIB_TIMER *pTimer)
{
    return pTimer->timerState == ICELIB_timerRunning;
}


bool ICELIB_timerIsTimedOut(ICELIB_TIMER *pTimer)
{
    return pTimer->timerState == ICELIB_timerTimeout;
}


bool ICELIB_veryfyICESupportOnStream(ICELIB_INSTANCE *pInstance,
                                     const ICE_MEDIA_STREAM *stream) {

    uint32_t i;

    for (i=0;i<stream->numberOfCandidates;i++) {
        ICE_CANDIDATE const *candidate = &stream->candidate[i];

        if (sockaddr_sameAddr((struct sockaddr *)&candidate->connectionAddr, 
                              (struct sockaddr *)&stream->defaultAddr)) {
            return true;
        }
    }
    ICELIB_log(&pInstance->callbacks.callbackLog,
               ICELIB_logDebug, "Verify ICE support returned false\n");

    char tmp[256];
    sockaddr_toString( (struct sockaddr *) &stream->defaultAddr,
                       tmp,
                       256,
                       true );
    
    printf("Default addr: %s, \n",tmp);

    return false;

}



bool ICELIB_verifyICESupport(ICELIB_INSTANCE *pInstance,
                             const ICE_MEDIA *iceRemoteMedia)
{
    uint32_t i;

    //Need to see if someone mangled the default address.

    for (i=0;i<iceRemoteMedia->numberOfICEMediaLines;i++) {
        ICE_MEDIA_STREAM const *mediaStream = &iceRemoteMedia->mediaStream[i];

        if (!ICELIB_veryfyICESupportOnStream(pInstance, mediaStream)) {
            ICELIB_log1(&pInstance->callbacks.callbackLog,
                         ICELIB_logDebug, "Verify ICE Support failed. Medialine: %i\n", i);
            return false;
        }

    }

    return true;
}


//
//----- Convert a random number to a segment of ice-chars
//      Number of characters: limit to maxlength, maxlength must be in the
//      range 0..5.
//
void ICELIB_longToIcechar(long data, char *b64, int maxLength)
{
    const char    *enc = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                         "0123456789+/";
    int           chpos;
    int           bpos;
    unsigned char udat;

    for (chpos = 0, bpos = 0; (chpos < maxLength) && (bpos < 32); ++chpos, bpos += 6) {

        udat = (data >> bpos) & 0x3f;
        b64[chpos] = enc[udat];
    }
}


//
//----- Create a string of random "ice-chars"
//      The length of the string, "maxlength", includes the terminating
//      null character (i.e. maxlength=5 yields 4 characters and a '\0').
//      A maxlength value of 0 leaves dst untouched.
//
void ICELIB_createRandomString(char *dst, int maxlength)
{
    int offset;
    int remaining;

    if (maxlength-- == 0) return;

    for (offset = 0; offset < maxlength; offset += ICELIB_RANDOM_SEGMENT_LENGTH) {
        remaining = min(maxlength - offset, ICELIB_RANDOM_SEGMENT_LENGTH);
        ICELIB_longToIcechar(rand(), dst + offset, remaining);
    }
    dst[ maxlength] = '\0';
}


void ICELIB_createUfrag(char *dst, int maxlength)
{
    maxlength = min(maxlength, ICE_MAX_UFRAG_LENGTH);
    ICELIB_createRandomString(dst, maxlength);
}


void ICELIB_createPasswd(char *dst, int maxlength)
{
    maxlength = min(maxlength, ICE_MAX_PASSWD_LENGTH);
    ICELIB_createRandomString(dst, maxlength);
}




uint32_t ICELIB_calculatePriority(ICE_CANDIDATE_TYPE type, uint16_t compid)
{
    uint32_t typepref = 0;
    uint32_t typeprefs[] = { 0,
                             ICELIB_LOCAL_TYPEPREF,
                             ICELIB_REFLEX_TYPEREF,
                             ICELIB_RELAY_TYPEREF,
                             ICELIB_PEERREF_TYPEREF
                           };

    typepref = (0xff000000 & (typeprefs[type] << 24));

    return (typepref | 0x00ffff00 | ((256 - compid) & 0xff));
}


//
//----- Create a 'foundation' string
//      The length of the string, "maxlength", includes the terminating
//      null character (i.e. maxlength=5 yields 4 characters and a '\0').
//
void ICELIB_createFoundation(char *dst, ICE_CANDIDATE_TYPE type, int maxlength)
{
    char const * foundation;

    if (maxlength-- == 0) return;

    switch(type) {
        case ICE_CAND_TYPE_HOST:
            foundation = "1";
            break;
        case ICE_CAND_TYPE_SRFLX:
            foundation = "3";
            break;
        case ICE_CAND_TYPE_RELAY:
            foundation = "4";
            break;
        case ICE_CAND_TYPE_PRFLX:
            foundation = "2";
            break;
        default:
            foundation = "unknowntype";
    }

    strncpy(dst, foundation, maxlength);
    dst[ maxlength] = '\0';
}


bool ICELIB_isEmptyCandidate(const ICE_CANDIDATE *pCandidate)
{
    return strlen(pCandidate->foundation) == 0;
}


//
//----- Candidate considered non-valid if reset or set to 'any'
//
bool ICELIB_isNonValidCandidate(const ICE_CANDIDATE *pCandidate)
{
    return !sockaddr_isSet(    (struct sockaddr *)&pCandidate->connectionAddr) ||
        sockaddr_isAddrAny((struct sockaddr *)&pCandidate->connectionAddr);
}


bool ICELIB_isEmptyOrNonValidCandidate(const ICE_CANDIDATE *pCandidate)
{
    return ICELIB_isEmptyCandidate(   pCandidate) ||
           ICELIB_isNonValidCandidate(pCandidate);
}


//
//----- Eliminate (zero out) redundant candidates.
//      It is assumed that the table is allready sorted in order
//      of priority!
//
void ICELIB_clearRedundantCandidates(ICE_CANDIDATE candidates[])
{
    int i;
    int j;

    for (i=0; i < ICE_MAX_CANDIDATES; ++i) {

        if (!ICELIB_isEmptyCandidate(&candidates[ i])) {

            for (j=i+1; j < ICE_MAX_CANDIDATES; ++j) {

                if (sockaddr_alike((struct sockaddr *)&candidates[ i].connectionAddr,
                                   (struct sockaddr *)&candidates[ j].connectionAddr)) {
                    // Found redundant, eliminate
                    ICELIB_resetCandidate(&candidates[ j]);
                }
            }
        }
    }
}


//
//----- Eliminate empty and non-valid entries.
//      Compact the table so that empty and non-valid entries are moved to the
//      end of the table.
//      A non-valid entry is a non-empty entry where the connection address is
//      not valid (it is reset or set to "Any").
//
void ICELIB_compactTable(ICE_CANDIDATE candidates[])
{
    int i;
    int j;

    for (i=0; i < ICE_MAX_CANDIDATES; ++i) {

        if (ICELIB_isEmptyOrNonValidCandidate(&candidates[ i])) {

            for (j=i+1; j < ICE_MAX_CANDIDATES; ++j) {

                if (!ICELIB_isEmptyOrNonValidCandidate(&candidates[ j])) {
                    // Move current entry up to empty location
                    candidates[ i] = candidates[ j];
                    ICELIB_resetCandidate(&candidates[ j]);
                    break;
                }
            }
        }
    }
}


int ICELIB_countCandidates(ICE_CANDIDATE candidates[])
{
    int i;
    int n = 0;

    for (i=0; i < ICE_MAX_CANDIDATES; ++i) {
        if (!ICELIB_isEmptyOrNonValidCandidate(&candidates[ i])) ++n;
    }

    return n;
}


//
//----- Eliminate redundant candidates and empty entries.
//      It is assumed that the table is already sorted in order
//      of priority!
//
//      ICE-19: 4.1.3. Eliminating Redundant Candidates
//
int ICELIB_eliminateRedundantCandidates(ICE_CANDIDATE candidates[])
{
    ICELIB_clearRedundantCandidates(candidates);
    ICELIB_compactTable(candidates);

    return ICELIB_countCandidates(candidates);
}

const char *ICELIB_toString_CheckListState(ICELIB_CHECKLIST_STATE state)
{
    switch (state) {
        case ICELIB_CHECKLIST_IDLE:      return "ICELIB_CHECKLIST_IDLE";
        case ICELIB_CHECKLIST_RUNNING:   return "ICELIB_CHECKLIST_RUNNING";
        case ICELIB_CHECKLIST_COMPLETED: return "ICELIB_CHECKLIST_COMPLETED";
        case ICELIB_CHECKLIST_FAILED:    return "ICELIB_CHECKLIST_FAILED";
        default: return "--unknown--";
    }
}


const char *ICELIB_toString_CheckListPairState(ICELIB_PAIR_STATE state)
{
    switch (state) {
        case ICELIB_PAIR_IDLE:       return "IDLE";
        case ICELIB_PAIR_PAIRED:     return "PAIRED";
        case ICELIB_PAIR_FROZEN:     return "FROZEN";
        case ICELIB_PAIR_WAITING:    return "WAITING";
        case ICELIB_PAIR_INPROGRESS: return "INPROGRESS";
        case ICELIB_PAIR_SUCCEEDED:  return "SUCCEEDED";
        case ICELIB_PAIR_FAILED:     return "FAILED";
        default: return "--unknown--";
    }
}


void ICELIB_removChecksFromCheckList(ICELIB_CHECKLIST *pCheckList)
{
    pCheckList->numberOfPairs = 0;
    memset(&pCheckList->checkListPairs, 0, sizeof(ICELIB_LIST_PAIR));
    memset(&pCheckList->componentList, 0, sizeof(ICELIB_COMPONENTLIST));
    pCheckList->nextPairId = 0;
}

void ICELIB_resetCheckList(ICELIB_CHECKLIST *pCheckList, unsigned int checkListId)
{
    memset(pCheckList, 0, sizeof(*pCheckList));
    pCheckList->id = checkListId;
}


void ICELIB_resetPair(ICELIB_LIST_PAIR *pPair)
{
    memset(pPair, 0, sizeof(*pPair));
}


void ICELIB_resetCandidate(ICE_CANDIDATE *pCandidate)
{
    memset(pCandidate, 0, sizeof(*pCandidate));
}


int ICELIB_comparePairsCL(const void *cp1, const void *cp2)
{
    const ICELIB_LIST_PAIR *pPair1 = (const ICELIB_LIST_PAIR *)cp1;
    const ICELIB_LIST_PAIR *pPair2 = (const ICELIB_LIST_PAIR *)cp2;

    if (pPair1->pairPriority > pPair2->pairPriority) return -1;
    if (pPair1->pairPriority < pPair2->pairPriority) return  1;

    return 0;
}


void ICELIB_saveUfragPasswd(ICELIB_CHECKLIST *pCheckList,
                            const ICE_MEDIA_STREAM *pLocalMediaStream,
                            const ICE_MEDIA_STREAM *pRemoteMediaStream)
{
    pCheckList->ufragLocal   = pLocalMediaStream->ufrag;
    pCheckList->ufragRemote  = pRemoteMediaStream->ufrag;
    pCheckList->passwdLocal  = pLocalMediaStream->passwd;
    pCheckList->passwdRemote = pRemoteMediaStream->passwd;

}


//
//----- Form candidate pairs
//
//      ICE-19: 5.7.1 Forming Candidate Pairs:
//
//      A local candidate is paired with a remote candidate if and only if
//      the two candidates have the same component ID and have the same IP
//      address version.

void ICELIB_formPairs(ICELIB_CHECKLIST       *pCheckList,
                      ICELIB_CALLBACK_LOG    *pCallbackLog,
                      const ICE_MEDIA_STREAM *pLocalMediaStream,
                      const ICE_MEDIA_STREAM *pRemoteMediaStream,
                      unsigned int           maxPairs)
{
    uint32_t           iLocal;
    uint32_t           iRemote;
    unsigned int  iPairs = 0;

    for (iLocal=0; iLocal < pLocalMediaStream->numberOfCandidates; ++iLocal) {

        const ICE_CANDIDATE *pLocalCand = &pLocalMediaStream->candidate [ iLocal];
        if (iPairs >= maxPairs) break;

        for (iRemote=0; iRemote < pRemoteMediaStream->numberOfCandidates; ++iRemote)
        {
            const ICE_CANDIDATE *pRemoteCand =
                &pRemoteMediaStream->candidate[ iRemote];

            if (iPairs >= maxPairs) break;

            if (pLocalCand->componentid == pRemoteCand->componentid) {
                if (pLocalCand->connectionAddr.ss_family ==
                    pRemoteCand->connectionAddr.ss_family) {

                    // Local and remote matches in component ID and address type,
                    // make a pair!
                    ICELIB_LIST_PAIR *pCheckListPair = &pCheckList->checkListPairs[ iPairs];
                    ICELIB_resetPair(pCheckListPair);

                    ICELIB_changePairState(pCheckListPair, ICELIB_PAIR_PAIRED, pCallbackLog);
                    pCheckListPair->pairId           = ++pCheckList->nextPairId+100*pCheckList->id;
                    pCheckListPair->pLocalCandidate  = pLocalCand;
                    pCheckListPair->pRemoteCandidate = pRemoteCand;

                    ++iPairs;

                    ICELIB_log1(pCallbackLog, ICELIB_logDebug,
                                "Pair Created, pair count: %d",
                                iPairs);
                }
            }
        }
    }
    pCheckList->numberOfPairs = iPairs;
}


uint64_t ICELIB_pairPriority(uint32_t G, uint32_t D)
{
    
    uint64_t  f1 = min(G, D);
    uint64_t  f2 = max(G, D);
    uint64_t  f3 = G > D ? 1 : 0;
    
    return (f1 << 32) | (f2 * 2) | f3;
    
}


//
//----- ICE-19: 5.7.2 Computing Pair Priority
//
void ICELIB_computePairPriority(ICELIB_LIST_PAIR *pCheckListPair,
                                 bool              iceControlling)
{
    uint32_t  G;
    uint32_t  D;

    if (iceControlling) {
        G = pCheckListPair->pLocalCandidate->priority;
        D = pCheckListPair->pRemoteCandidate->priority;
    } else {
        G = pCheckListPair->pRemoteCandidate->priority;
        D = pCheckListPair->pLocalCandidate->priority;
    }

    pCheckListPair->pairPriority = ICELIB_pairPriority(G, D);
}


void ICELIB_computeListPairPriority(ICELIB_CHECKLIST *pCheckList,
                                     bool              iceControlling)
{
    unsigned int i;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        ICELIB_computePairPriority(&pCheckList->checkListPairs[ i],
                                    iceControlling);
    }
}


void ICELIB_sortPairsCL(ICELIB_CHECKLIST *pCheckList)
{
    qsort(pCheckList->checkListPairs,
           pCheckList->numberOfPairs,
           sizeof(ICELIB_LIST_PAIR),
           ICELIB_comparePairsCL);
}


//
//----- Find base addresses
//
//      The base addresses is assumed to be equal to the Rtp/Rtcp HOST addresses.
//
//      Return: true  - found both
//              false - did not find both
//
bool ICELIB_findReflexiveBaseAddresses(const ICE_CANDIDATE * * ppBaseServerReflexiveRtp,
                                       const ICE_CANDIDATE * * ppBaseServerReflexiveRtcp,
                                       const ICE_MEDIA_STREAM *pLocalMediaStream)
{
    uint32_t i;
    bool    foundRtp  = false;
    bool    foundRtcp = false;

    *ppBaseServerReflexiveRtp  = NULL;
    *ppBaseServerReflexiveRtcp = NULL;

    for (i=0; i < pLocalMediaStream->numberOfCandidates; ++i) {
        if (pLocalMediaStream->candidate[i].type == ICE_CAND_TYPE_HOST) {
            if (pLocalMediaStream->candidate[i].componentid == ICELIB_RTP_COMPONENT_ID) {
                *ppBaseServerReflexiveRtp = &pLocalMediaStream->candidate[i];
                foundRtp = true;
            }
            if (pLocalMediaStream->candidate[i].componentid == ICELIB_RTCP_COMPONENT_ID) {
                *ppBaseServerReflexiveRtcp = &pLocalMediaStream->candidate[i];
                foundRtcp = true;
            }
        }
    }
    if (foundRtp || foundRtcp) return true;
    return false;
}


bool ICELIB_isComponentIdPresent(const ICELIB_COMPONENTLIST *pComponentList,
                                 uint32_t                   componentId)
{
    unsigned int i;

    for (i=0; i < pComponentList->numberOfComponents; ++i) {
        if (pComponentList->componentIds[ i] == componentId) return true;
    }

    return false;
}


//
//----- Return: true  - list full
//              false - OK
//
bool ICELIB_addComponentId(ICELIB_COMPONENTLIST *pComponentList,
                           uint32_t              componentId)
{
    if (pComponentList->numberOfComponents >= ICELIB_MAX_COMPONENTS) return true;

    pComponentList->componentIds[ pComponentList->numberOfComponents++] =
                                                                     componentId;
    return false;
}


bool ICELIB_addComponentIdIfUnique(ICELIB_COMPONENTLIST *pComponentList,
                                   uint32_t              componentId)
{
    bool listFull;

    listFull = false;
    if (! ICELIB_isComponentIdPresent(pComponentList, componentId)) {
        listFull = ICELIB_addComponentId(pComponentList, componentId);
    }
    return listFull;
}


//
//----- Collect all (different) component IDs in a check list
//
//      The component IDs are placed in a list for later reference.
//      Checking only IDs for local candidates in each pair (in a pair
//      the local and remote candidate always have equal IDs).
//
//      Return: true  - list full
//              false - OK
//
bool ICELIB_collectEffectiveCompontIds(ICELIB_CHECKLIST *pCheckList)
{
    unsigned int i;
    bool         listFull;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        listFull =  ICELIB_addComponentIdIfUnique(
                    &pCheckList->componentList,
                    pCheckList->checkListPairs[ i].pLocalCandidate->componentid);
        if (listFull) {
            printf("<icelib> Problem adding componentId\n");
            return true;
        }
    }

    return false;
}


void ICELIB_prunePairsReplaceWithBase(ICELIB_CHECKLIST    *pCheckList,
                                      const ICE_CANDIDATE *pBbaseServerReflexiveRtp,
                                      const ICE_CANDIDATE *pBaseServerReflexiveRtcp)
{
    unsigned int i;

    for (i = 0; i < pCheckList->numberOfPairs; ++i) {
        if (pCheckList->checkListPairs[ i].pLocalCandidate->type == ICE_CAND_TYPE_SRFLX) {

            if (pCheckList->checkListPairs[ i].pLocalCandidate->componentid == ICELIB_RTP_COMPONENT_ID) {
                pCheckList->checkListPairs[ i].pLocalCandidate = pBbaseServerReflexiveRtp;
            }
            if (pCheckList->checkListPairs[ i].pLocalCandidate->componentid == ICELIB_RTCP_COMPONENT_ID) {
                pCheckList->checkListPairs[ i].pLocalCandidate = pBaseServerReflexiveRtcp;
            }
        }
    }
}


bool ICELIB_prunePairsIsEqual(const ICELIB_LIST_PAIR *pPair1,
                              const ICELIB_LIST_PAIR *pPair2)
{
    if (pPair1->pLocalCandidate  != pPair2->pLocalCandidate) return false;
    if (pPair1->pRemoteCandidate != pPair2->pRemoteCandidate) return false;

    return true;
}


bool ICELIB_prunePairsIsClear(const ICELIB_LIST_PAIR *pPair)
{
    if (pPair->pLocalCandidate  == NULL) return true;
    if (pPair->pRemoteCandidate == NULL) return true;

    return false;
}


void ICELIB_prunePairsClear(ICELIB_LIST_PAIR *pPair)
{
    pPair->pLocalCandidate  = NULL;
    pPair->pRemoteCandidate = NULL;
}


void ICELIB_prunePairsClearDuplicates(ICELIB_CHECKLIST *pCheckList)
{
    unsigned int i;
    unsigned int j;

    for (i = 0; i < pCheckList->numberOfPairs; ++i) {
        for (j = i + 1; j < pCheckList->numberOfPairs; ++j) {
            if (ICELIB_prunePairsIsEqual(&pCheckList->checkListPairs[i],
                                          &pCheckList->checkListPairs[j])) {

                ICELIB_prunePairsClear(  &pCheckList->checkListPairs[j]);
            }
        }
    }
}


void ICELIB_prunePairsCompact(ICELIB_CHECKLIST * pCheckList)
{
    unsigned int i;
    unsigned int j;

    for (i = 0; i < pCheckList->numberOfPairs; ++i) {
        if (ICELIB_prunePairsIsClear(&pCheckList->checkListPairs[ i])) {

            for (j = i + 1; j < pCheckList->numberOfPairs; ++j) {
                if (! ICELIB_prunePairsIsClear(&pCheckList->checkListPairs[ j])) {
                    pCheckList->checkListPairs[ i] = pCheckList->checkListPairs[ j];
                    ICELIB_prunePairsClear(&pCheckList->checkListPairs[ j]);
                    break;
                }
            }
        }
    }
}


unsigned int ICELIB_prunePairsCountPairs(ICELIB_LIST_PAIR pairs[])
{
    unsigned int i;

    for (i=0; i < ICELIB_MAX_PAIRS; ++i) {
        if (ICELIB_prunePairsIsClear(&pairs[ i])) break;
    }

    return i;
}


//
//----- ICE-19: 5.7.3 Pruning the Pairs:
//
//      Since an agent cannot send requests directly from a reflexive
//      candidate, but only from its base, the agent next goes through the
//      sorted list of candidate pairs. For each pair where the local
//      candidate is server reflexive, the server reflexive candidate MUST be
//      replaced by its base. Once this has been done, the agent MUST prune
//      the list. This is done by removing a pair if its local and remote
//      candidates are identical to the local and remote candidates of a pair
//      higher up on the priority list. The result is a sequence of ordered
//      candidate pairs, called the check list for that media stream.

void ICELIB_prunePairs(ICELIB_CHECKLIST    *pCheckList,
                       const ICE_CANDIDATE *pBbaseServerReflexiveRtp,
                       const ICE_CANDIDATE *pBaseServerReflexiveRtcp)
{
    ICELIB_prunePairsReplaceWithBase(pCheckList,
                                      pBbaseServerReflexiveRtp,
                                      pBaseServerReflexiveRtcp);
    ICELIB_prunePairsClearDuplicates(pCheckList);
    ICELIB_prunePairsCompact(pCheckList);

    pCheckList->numberOfPairs =
        ICELIB_prunePairsCountPairs(pCheckList->checkListPairs);
}


//
//----- Set all pairs in the check list to a given state
//
void ICELIB_computeStatesSetState(ICELIB_CHECKLIST  *pCheckList,
                                  ICELIB_PAIR_STATE state,
                                  ICELIB_CALLBACK_LOG *pCallbackLog)
{
    unsigned int i;

    for (i = 0; i < pCheckList->numberOfPairs; ++i) {
        ICELIB_changePairState(&pCheckList->checkListPairs[i], state, pCallbackLog);
    }
}


//
//----- Set all pairs i a check list to an initial state
//
//      ICE-19: 5.7.4 Computing States:
//
//      *  The agent sets all of the pairs in the check list to the Frozen
//         state.
//      *  For all pairs with the same foundation, it sets the state of
//         the pair with the lowest component ID to Waiting.
//
//      Assumes the check list is sorted so that pairs with lower component ID
//      comes before pairs with higher componnet ID.
//      Also assumes that the initial pairState != ICELIB_PAIR_FROZEN.
//
void ICELIB_computeStatesSetWaitingFrozen(ICELIB_CHECKLIST *pCheckList,
                                          ICELIB_CALLBACK_LOG *pCallbackLog)
{
    unsigned int i;
    unsigned int j;
    char foundation1[ICE_MAX_FOUNDATION_PAIR_LENGTH];
    char foundation2[ICE_MAX_FOUNDATION_PAIR_LENGTH];

    for (i = 0; i < pCheckList->numberOfPairs; ++i) {
        if (pCheckList->checkListPairs[i].pairState != ICELIB_PAIR_FROZEN) {

            ICELIB_changePairState(&pCheckList->checkListPairs[i], ICELIB_PAIR_WAITING, pCallbackLog);
            ICELIB_getPairFoundation(foundation1,
                                      ICE_MAX_FOUNDATION_PAIR_LENGTH,
                                      &pCheckList->checkListPairs[i]);

            for (j = i + 1; j < pCheckList->numberOfPairs; ++j) {
                ICELIB_getPairFoundation(foundation2,
                                          ICE_MAX_FOUNDATION_PAIR_LENGTH,
                                          &pCheckList->checkListPairs[j]);
                if (strcmp(foundation1, foundation2) == 0) {
                    ICELIB_changePairState(&pCheckList->checkListPairs[j], ICELIB_PAIR_FROZEN, pCallbackLog);
                }
            }
        }
    }
}


bool ICELIB_makeCheckList(ICELIB_CHECKLIST       *pCheckList,
                          ICELIB_CALLBACK_LOG    *pCallbackLog,
                          const ICE_MEDIA_STREAM *pLocalMediaStream,
                          const ICE_MEDIA_STREAM *pRemoteMediaStream,
                          bool                   iceControlling,
                          unsigned int           maxPairs,
                          unsigned int           checkListId)
{
    const ICE_CANDIDATE *pBaseServerReflexiveRtp;
    const ICE_CANDIDATE *pBaseServerReflexiveRtcp;
    bool                foundBases;
    bool                componentListFull;


    if (pCheckList->ufragLocal != NULL && pLocalMediaStream->ufrag != NULL) {
        if (!strncmp(pCheckList->ufragLocal, pLocalMediaStream->ufrag, ICE_MAX_UFRAG_LENGTH)) {
            ICELIB_log(pCallbackLog,
                       ICELIB_logDebug, "Ufrag already present in checklist Ignoring (No restart detected)\n");

            ICELIB_removChecksFromCheckList(pCheckList);
            pCheckList->checkListState=ICELIB_CHECKLIST_COMPLETED;

            return false;;
        }
    }

    ICELIB_resetCheckList(pCheckList, checkListId);
    ICELIB_saveUfragPasswd(pCheckList, pLocalMediaStream, pRemoteMediaStream);
    ICELIB_formPairs(pCheckList,
                     pCallbackLog,
                     pLocalMediaStream,
                     pRemoteMediaStream,
                     maxPairs);

    ICELIB_computeListPairPriority(pCheckList, iceControlling);

    ICELIB_sortPairsCL(pCheckList);

    foundBases = ICELIB_findReflexiveBaseAddresses(&pBaseServerReflexiveRtp,
                                                   &pBaseServerReflexiveRtcp,
                                                   pLocalMediaStream);
    if (!foundBases) {
        ICELIB_log(pCallbackLog, ICELIB_logError, "Base addresses not found!");
        return false;
    }

    ICELIB_prunePairs(pCheckList,
                      pBaseServerReflexiveRtp,
                      pBaseServerReflexiveRtcp);

    componentListFull = ICELIB_collectEffectiveCompontIds(pCheckList);

    if (componentListFull) {
        ICELIB_log(pCallbackLog, ICELIB_logError, "Component list is full!");
        return false;
    }

    pCheckList->checkListState = ICELIB_CHECKLIST_RUNNING;

    ICELIB_log1(pCallbackLog, ICELIB_logDebug,
                 "Checklist generated, pair count: %d",
                 pCheckList->numberOfPairs);
    return true;
}


void ICELIB_makeAllCheckLists(ICELIB_INSTANCE *pInstance)
{
    unsigned int i;
    bool checkListWaitInited = false;

    pInstance->numberOfMediaStreams =
        min(pInstance->localIceMedia.numberOfICEMediaLines,
            pInstance->remoteIceMedia.numberOfICEMediaLines);

    //  Ooops: how do we associate local and remote media streams?
    //  For now just match up streams of equal index
    for (i = 0; i < pInstance->numberOfMediaStreams; ++i) {
        bool notIgnored;

        notIgnored = ICELIB_makeCheckList(&pInstance->streamControllers[ i].checkList,
                                          &pInstance->callbacks.callbackLog,
                                          &pInstance->localIceMedia.mediaStream[ i],
                                          &pInstance->remoteIceMedia.mediaStream[ i],
                                          pInstance->iceControlling,
                                          pInstance->iceConfiguration.maxCheckListPairs,
                                          i);

        if (!checkListWaitInited && notIgnored) {
            checkListWaitInited = true;
            ICELIB_computeStatesSetWaitingFrozen(&pInstance->streamControllers[ i].checkList,
                                                 &pInstance->callbacks.callbackLog);
        } else {
            ICELIB_computeStatesSetState(&pInstance->streamControllers[ i].checkList,
                                         ICELIB_PAIR_FROZEN,
                                         &pInstance->callbacks.callbackLog);
        }
    }
}


//
//----- Insert a new pair into the check list.
//      Pair inserted at its proper place based on its priority.
//
//      Return: true  - no more room, check list is full!!
//              false - pair was inserted into list
//
bool ICELIB_insertIntoCheckList(ICELIB_CHECKLIST  *pCheckList,
                                ICELIB_LIST_PAIR  *pPair)
{
    if (pCheckList->numberOfPairs >= ICELIB_MAX_PAIRS) return true;

    pPair->pairId = (++pCheckList->nextPairId+100*pCheckList->id);
    pCheckList->checkListPairs[ pCheckList->numberOfPairs++] = *pPair;
    ICELIB_sortPairsCL(pCheckList);

    return false;
}


ICELIB_LIST_PAIR *pICELIB_findPairByState(ICELIB_CHECKLIST *pCheckList,
                                          ICELIB_PAIR_STATE pairState)
{
    unsigned int i;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        if (pCheckList->checkListPairs[ i].pairState == pairState) {
            return &pCheckList->checkListPairs[ i];
        }
    }

    return NULL;
}

bool ICELIB_isPairAddressMatchInChecklist(ICELIB_CHECKLIST *pCheckList,
                                          ICELIB_LIST_PAIR *pair)
{
    unsigned int i;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        if (ICELIB_isPairAddressMatch(&pCheckList->checkListPairs[ i], pair)) {

            return true;
        }
    }

    return false;
}


//
//----- Find ordinary (non triggered) pair to schedule
//
//      ICE-19: 5.8 Scheduling checks
//
//      Find highest priority WAITING pair, or if no such pair exists,
//      find highest priority FROZEN pair.
//
ICELIB_LIST_PAIR *pICELIB_chooseOrdinaryPair(ICELIB_CHECKLIST *pCheckList)
{
    ICELIB_LIST_PAIR *pPairFound;

    pPairFound = pICELIB_findPairByState(pCheckList, ICELIB_PAIR_WAITING);
    if (pPairFound != NULL) return pPairFound;
    return pICELIB_findPairByState(pCheckList, ICELIB_PAIR_FROZEN);
}


//
//----- A check list with at least one pair that is Waiting is called an
//      active check list.
//
//      return: true  - at least one pair is in the waiting state
//              false - no pair is in the waiting state
//
bool ICELIB_isActiveCheckList(const ICELIB_CHECKLIST *pCheckList)
{
    unsigned int i;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        if (pCheckList->checkListPairs[ i].pairState == ICELIB_PAIR_WAITING) {
            return true;
        }
    }

    return false;
}


//
//----- A check list with all pairs frozen is called a frozen check list
//
//      return: true  - all pairs are in the frozen state
//              false - one or more pairs are NOT frozen
//
bool ICELIB_isFrozenCheckList(const ICELIB_CHECKLIST *pCheckList)
{
    unsigned int i;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        if (pCheckList->checkListPairs[ i].pairState != ICELIB_PAIR_FROZEN) {
            return false;
        }
    }

    return true;
}


//
//----- Unfreeze a frozen check list
//
//      Groups together all of the pairs with the same foundation,
//      For each group, sets the state of the pair with the lowest
//      component ID to Waiting. If there is more than one such pair,
//      the one with the highest priority is used.
//
//      Assumes the check list is sorted so that pairs with lower component ID
//      comes before pairs with higher componnet ID.
//
//      TODO: what is meant by 'groups together all of the pairs with the same
//            foundation' ....
//
void ICELIB_unfreezeFrozenCheckList(ICELIB_CHECKLIST *pCheckList, ICELIB_CALLBACK_LOG *pCallbackLog)
{
    ICELIB_computeStatesSetState(pCheckList, ICELIB_PAIR_PAIRED, pCallbackLog);
    ICELIB_computeStatesSetWaitingFrozen(pCheckList, pCallbackLog);
}


//
//----- Unfreeze pairs with specified foundation
//
void ICELIB_unfreezePairsByFoundation(ICELIB_CHECKLIST *pCheckList,
                                      const char *pPairFoundationToMatch,
                                      ICELIB_CALLBACK_LOG *pCallbackLog)
{
    unsigned int i;
    char pairFoundation[ ICE_MAX_FOUNDATION_PAIR_LENGTH];
    ICELIB_LIST_PAIR *pPair;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        pPair = &pCheckList->checkListPairs[ i];
        if (pPair->pairState == ICELIB_PAIR_FROZEN) {
            ICELIB_getPairFoundation(pairFoundation,
                                      ICE_MAX_FOUNDATION_PAIR_LENGTH,
                                      pPair);
            if (strcmp(pairFoundation, pPairFoundationToMatch) == 0) {
                ICELIB_changePairState(pPair, ICELIB_PAIR_WAITING, pCallbackLog);
            }
        }
    }
}


//-----------------------------------------------------------------------------
//
//===== Local Functions: FIFO
//

#define fifoIncrementToNext(index) (index = (index + 1) % ICELIB_MAX_FIFO_ELEMENTS)

void ICELIB_fifoClear(ICELIB_TRIGGERED_FIFO *pFifo)
{
    memset(pFifo, 0, sizeof(*pFifo));
}


unsigned int ICELIB_fifoCount(const ICELIB_TRIGGERED_FIFO *pFifo)
{
    if (pFifo->isFull) return ICELIB_MAX_FIFO_ELEMENTS;
    if (pFifo->inIndex >= pFifo->outIndex) {
        return pFifo->inIndex - pFifo->outIndex;
    } else {
        return ICELIB_MAX_FIFO_ELEMENTS - (pFifo->outIndex - pFifo->inIndex);
    }
}


bool ICELIB_fifoIsEmpty(const ICELIB_TRIGGERED_FIFO *pFifo)
{
    return (pFifo->inIndex == pFifo->outIndex) && !pFifo->isFull;
}


bool ICELIB_fifoIsFull(const ICELIB_TRIGGERED_FIFO *pFifo)
{
    return pFifo->isFull;
}


//
//----- Return: true  - no more room, fifo is full!!
//              false - element was inserted into fifo
//
bool ICELIB_fifoPut(ICELIB_TRIGGERED_FIFO *pFifo, ICELIB_FIFO_ELEMENT element)
{
    if (ICELIB_fifoIsFull(pFifo)) return true;

    pFifo->elements[ pFifo->inIndex] = element;
    fifoIncrementToNext(pFifo->inIndex);
    if (pFifo->inIndex == pFifo->outIndex) pFifo->isFull = true;

    return false;
}


//
//----- Return: element               - fifo was not empty
//              ICELIB_FIFO_IS_EMPTY  - fifo is empty!!
//
ICELIB_FIFO_ELEMENT ICELIB_fifoGet(ICELIB_TRIGGERED_FIFO *pFifo)
{
    unsigned int outPreIndex;

    if (ICELIB_fifoIsEmpty(pFifo)) return ICELIB_FIFO_IS_EMPTY;

    pFifo->isFull = false;
    outPreIndex   = pFifo->outIndex;
    fifoIncrementToNext(pFifo->outIndex);

    return pFifo->elements[ outPreIndex];
}


void ICELIB_fifoIteratorConstructor(ICELIB_TRIGGERED_FIFO_ITERATOR *pIterator,
                                    ICELIB_TRIGGERED_FIFO          *pFifo)
{
    pIterator->pFifo = pFifo;
    pIterator->index = pFifo->outIndex;
    pIterator->atEnd = false;
}


ICELIB_FIFO_ELEMENT *pICELIB_fifoIteratorNext(ICELIB_TRIGGERED_FIFO_ITERATOR *pIterator)
{
    ICELIB_FIFO_ELEMENT *pElement = NULL;

    if (ICELIB_fifoIsEmpty(pIterator->pFifo)) return NULL;

    if (pIterator->atEnd) return NULL;

    if (ICELIB_fifoIsFull(pIterator->pFifo)) {
        pElement = &pIterator->pFifo->elements[ pIterator->index];
        fifoIncrementToNext(pIterator->index);
        if (pIterator->index == pIterator->pFifo->inIndex) {
            pIterator->atEnd = true;
        }
    } else {
        if (pIterator->index == pIterator->pFifo->inIndex) {
            pIterator->atEnd = true;
            return NULL;
        }

        pElement = &pIterator->pFifo->elements[ pIterator->index];
        fifoIncrementToNext(pIterator->index);
    }

    return pElement;
}


//-----------------------------------------------------------------------------
//
//===== Local Functions: Triggered checks queue
//
//      The queue (fifo) holds pairIds. All pairs in a check list must have
//      unique pairIds. The reason for not storing pointers to pairs is that
//      insert and sort operations will most likely move the pairs to new
//      locations.
//

//
//----- Look up a pair in the check list by its pairId
//
//      Return: pointer to pair  - pair found
//              NULL             - pair not found
//
ICELIB_LIST_PAIR *ICELIB_getPairById(ICELIB_CHECKLIST *pCheckList,
                                     uint32_t          pairId)
{
    unsigned int i;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        if (pCheckList->checkListPairs[ i].pairId == pairId) {
            return &pCheckList->checkListPairs[ i];
        }
    }

    return NULL;
}


void ICELIB_triggeredFifoClear(ICELIB_TRIGGERED_FIFO *pFifo)
{
    ICELIB_fifoClear(pFifo);
}


unsigned int ICELIB_triggeredFifoCount(const ICELIB_TRIGGERED_FIFO *pFifo)
{
    return ICELIB_fifoCount(pFifo);
}


bool ICELIB_triggeredFifoIsEmpty(const ICELIB_TRIGGERED_FIFO *pFifo)
{
    return ICELIB_fifoIsEmpty(pFifo);
}


bool ICELIB_triggeredFifoIsFull(const ICELIB_TRIGGERED_FIFO *pFifo)
{
    return ICELIB_fifoIsFull(pFifo);
}


//
//----- Return: true  - no more room, fifo is full!!
//              false - element was inserted into fifo
//
bool ICELIB_triggeredFifoPut(ICELIB_TRIGGERED_FIFO *pFifo,
                              ICELIB_LIST_PAIR      *pPair)
{
    return ICELIB_fifoPut(pFifo, pPair->pairId);
}

bool ICELIB_isTriggeredFifoPairPresent(ICELIB_TRIGGERED_FIFO *pFifo,
                                        ICELIB_LIST_PAIR      *pPair,
                                        ICELIB_CALLBACK_LOG   *pCallbackLog)
{
    uint32_t                        *pPairId;
    ICELIB_TRIGGERED_FIFO_ITERATOR  tfIterator;

    ICELIB_fifoIteratorConstructor(&tfIterator, pFifo);

    while ((pPairId = pICELIB_fifoIteratorNext(&tfIterator)) != NULL) {
        if (*pPairId == pPair->pairId) {
            ICELIB_log(pCallbackLog, ICELIB_logDebug, "pair already present in FIFO");
            return true;
        }
    }
    return false;
}


//
//----- Return: true  - no more room, fifo is full!!
//              false - element was inserted into fifo
//
bool ICELIB_triggeredFifoPutIfNotPresent(ICELIB_TRIGGERED_FIFO *pFifo,
                                         ICELIB_LIST_PAIR      *pPair,
                                         ICELIB_CALLBACK_LOG   *pCallbackLog)
{
    if (ICELIB_isTriggeredFifoPairPresent(pFifo, pPair, pCallbackLog)) {
        return ICELIB_fifoPut(pFifo, pPair->pairId);
    }
    return false;
}


//
//----- Get a pair from the triggered checks queue
//
//      Get a pointer to the pair.
//      Since the fifo holds pairId's, the address of the pair is found by
//      looking up the pair in the check list by its pairId.
//
//      Return: pointer to pair   - fifo was not empty
//              NULL              - fifo is empty!!
//
ICELIB_LIST_PAIR *pICELIB_triggeredFifoGet(ICELIB_CHECKLIST      *pCheckList,
                                           ICELIB_CALLBACK_LOG   *pCallbackLog,
                                           ICELIB_TRIGGERED_FIFO *pFifo)
{
    uint32_t          pairId;
    ICELIB_LIST_PAIR *pPair;

    do {
        if (ICELIB_fifoIsEmpty(pFifo)) {
            ICELIB_log(pCallbackLog, ICELIB_logDebug, "Triggered Check FIFO is empty!");
            return NULL;
        }
        pairId = ICELIB_fifoGet(pFifo);
    } while (pairId == ICELIB_FIFO_ELEMENT_REMOVED);

    pPair  = ICELIB_getPairById(pCheckList, pairId);

    if (pPair == NULL) {
        //printf("<icelib> TriggeredFifoGet: Could not find pair by Id: %u", pairId);
        ICELIB_log1(pCallbackLog, ICELIB_logDebug, "Could not find pair by Id: %u", pairId);
    }

    return pPair;
}




void ICELIB_triggeredFifoRemove(ICELIB_TRIGGERED_FIFO *pFifo,
                                ICELIB_LIST_PAIR      *pPair)
{
    uint32_t                        *pPairId;
    ICELIB_TRIGGERED_FIFO_ITERATOR  tfIterator;

    ICELIB_fifoIteratorConstructor(&tfIterator, pFifo);

    while ((pPairId = pICELIB_fifoIteratorNext(&tfIterator)) != NULL) {
        if (*pPairId == pPair->pairId) {
            *pPairId = ICELIB_FIFO_ELEMENT_REMOVED;
        }
    }
}


void ICELIB_triggeredfifoIteratorConstructor(ICELIB_TRIGGERED_FIFO_ITERATOR *pIterator,
                                             ICELIB_TRIGGERED_FIFO          *pFifo)
{
    ICELIB_fifoIteratorConstructor(pIterator, pFifo);
}


ICELIB_LIST_PAIR *pICELIB_triggeredfifoIteratorNext(ICELIB_CHECKLIST               *pCheckList,
                                                    ICELIB_CALLBACK_LOG            *pCallbackLog,
                                                    ICELIB_TRIGGERED_FIFO_ITERATOR *pIterator)
{
    uint32_t         *pPairId;
    ICELIB_LIST_PAIR *pPair = NULL;

    pPairId = pICELIB_fifoIteratorNext(pIterator);

    if (pPairId != NULL) {

        pPair  = ICELIB_getPairById(pCheckList, *pPairId);

        if (pPair == NULL) {
            ICELIB_log1(pCallbackLog, ICELIB_logDebug, "Could not find pair by Id: %u", *pPairId);
        }
    }

    return pPair;
}


//-----------------------------------------------------------------------------
//
//===== Local Functions: Stream controller
//

void ICELIB_resetStreamController(ICELIB_STREAM_CONTROLLER *pStreamController,
                                  unsigned int             tickIntervalMS)
{
    (void)tickIntervalMS;
    memset(pStreamController, 0, sizeof(*pStreamController));
}


//
//----- Pick a pair from the triggered checks queue, or from the
//      check list (if the triggered checks queue is empty).
//
//      ICE-19: 5.8 Scheduling checks
//
//      Return: pointer to pair - pair to schedule
//              NULL            - no pair in queue and no WAITIN/FROZEN pair
//                                in check list.
//
ICELIB_LIST_PAIR *pICELIB_findPairToScedule(ICELIB_STREAM_CONTROLLER *pController,
                                            ICELIB_CALLBACK_LOG      *pCallbackLog)
{
    ICELIB_LIST_PAIR *pPair;

    pPair = pICELIB_triggeredFifoGet(&pController->checkList,
                                      pCallbackLog,
                                      &pController->triggeredChecksFifo);
    if (pPair == NULL) {
        if (pController->checkList.stopChecks) {
            ICELIB_log(pCallbackLog, ICELIB_logDebug, "Checklist is stopped. No pair to schedule.");
            //printf("<icelib> Checklist is stopped. No pair to schedule\n");
            return NULL;
        }

        pPair = pICELIB_chooseOrdinaryPair(&pController->checkList);

        if (pPair != NULL) {
            // Unfreeze possible frozen pair
            ICELIB_changePairState(pPair, ICELIB_PAIR_WAITING, pCallbackLog);
            ICELIB_log(pCallbackLog, ICELIB_logDebug, "Schedueling pair form CHECKLIST.");
        }
    }else{
        ICELIB_log(pCallbackLog, ICELIB_logDebug, "\033[1;34m Schedueling Triggered Check\033[0m.");
        if (pPair->useCandidate)
            ICELIB_log(pCallbackLog, ICELIB_logDebug, "FIFO pair has USE_CANDIDATE");
    }

    return pPair;
}



//-----------------------------------------------------------------------------
//
//===== Local Functions: All stream controllers
//

bool ICELIB_insertTransactionId(ICELIB_LIST_PAIR *pair,  StunMsgId id)
{
    if (pair->numberOfTransactionIds>=ICELIB_MAX_NO_OF_TRANSID) {
        return false;
    }

    pair->transactionIdTable[pair->numberOfTransactionIds++] = id;
    return true;

}


void ICELIB_scheduleCheck(ICELIB_INSTANCE   *pInstance,
                          ICELIB_CHECKLIST  *pCheckList,
                          ICELIB_LIST_PAIR  *pPair)
{
    uint16_t                       componentid;
    ICELIB_outgoingBindingRequest  BindingRequest;
    char                           ufragPair[ ICE_MAX_UFRAG_PAIR_LENGTH ];
    char                           addr[ SOCKADDR_MAX_STRLEN ];
    StunMsgId                      transactionId;

    BindingRequest  = pInstance->callbacks.callbackRequest.pICELIB_sendBindingRequest;
    componentid     = pPair->pLocalCandidate->componentid;

    transactionId = ICELIB_generateTransactionId();
    if (!ICELIB_insertTransactionId(pPair,  transactionId)) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logWarning,
                   "To many transaction ID per pair");

    }

    if (BindingRequest != NULL) {

        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug,
                   "Scheduling check");

        if (pPair->useCandidate)
            ICELIB_log(&pInstance->callbacks.callbackLog,
                       ICELIB_logDebug,
                       "Pair has USE_CANDIDATE");

        if (pInstance->iceControlling && pInstance->iceControlled) {
            ICELIB_log(&pInstance->callbacks.callbackLog,
                       ICELIB_logDebug,
                       "Sending connectivitycheck with both controlling and controlled set");

        }

        BindingRequest(pInstance->callbacks.callbackRequest.pBindingRequestUserData,
                       (struct sockaddr *)&pPair->pRemoteCandidate->connectionAddr,
                       (struct sockaddr *)&pPair->pLocalCandidate->connectionAddr,
                       pPair->pLocalCandidate->userValue1,
                       pPair->pLocalCandidate->userValue2,
                       pPair->pLocalCandidate->componentid,
                       pPair->pLocalCandidate->type ==
                       ICE_CAND_TYPE_RELAY ? true : false,
                       ICELIB_getCheckListRemoteUsernamePair(ufragPair,
                                                             ICE_MAX_UFRAG_PAIR_LENGTH,
                                                             pCheckList),
                       ICELIB_getCheckListRemotePasswd(pCheckList),
                       ICELIB_calculatePriority(ICE_CAND_TYPE_PRFLX,
                                                componentid),
                       pPair->useCandidate,
                       pInstance->iceControlling,
                       pInstance->iceControlled,
                       pInstance->tieBreaker,
                       /*pPair->transactionId*/transactionId);

        if (pInstance->iceConfiguration.logLevel == ICELIB_logDebug) {
            debug(pCheckList, pPair->pairId, " --> sending binding request");
        }

        sockaddr_toString((const struct sockaddr *)&pPair->pRemoteCandidate->connectionAddr,
                          addr,
                          sizeof(addr),
                          true);
        ICELIB_log1(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug,
                    "Scheduling a check to: %s",
                    addr);

    }
}


//
//----- Schedule a connectivity check for a pair if check list is active and if
//      any WAITING/FROZEN or triggered pair is available.
//
//      ICE-19: 5.8 Scheduling checks
//
//      Return: true  - if a check was scheduled
//              false - if no more checks to scedule
//
bool ICELIB_scheduleSingle(ICELIB_INSTANCE          *pInstance,
                           ICELIB_STREAM_CONTROLLER *pController,
                           ICELIB_CALLBACK_LOG      *pCallbackLog)
{
    bool checkWasSceduled = false;
    ICELIB_LIST_PAIR *pPair;

    if (ICELIB_isActiveCheckList(&pController->checkList)) {
        pController->checkList.timerRunning = true;
    }

    if (pController-> checkList.timerRunning) {
        pPair = pICELIB_findPairToScedule(pController, &pInstance->callbacks.callbackLog);
        if (pPair != NULL) {
            if (pPair->pairState == ICELIB_PAIR_SUCCEEDED) {
                if (!pPair->useCandidate) {
                    ICELIB_log(&pInstance->callbacks.callbackLog,
                               ICELIB_logWarning,
                               "Scheduling SUCCEEDED check without USE_CANDIDATE flag set");

                }
            }else{
                ICELIB_changePairState(pPair, ICELIB_PAIR_INPROGRESS, pCallbackLog);
            }
            checkWasSceduled = true;
            if (pPair->useCandidate)
            {
                if (!pPair->sentUseCandidateAlready)
                {
                    ICELIB_scheduleCheck(pInstance, &pController->checkList, pPair);
                }
                else
                {
                    ICELIB_log(&pInstance->callbacks.callbackLog,
                               ICELIB_logWarning,
                               "Ignoring attempt to send USE_CANDIDATE check twice");
                }
                pPair->sentUseCandidateAlready = true;
            }
            else
            {
                ICELIB_scheduleCheck(pInstance, &pController->checkList, pPair);
            }
        } else {
            ICELIB_log(&pInstance->callbacks.callbackLog,
                       ICELIB_logDebug,
                       "Setting timerRunning to false");
            pController->checkList.timerRunning = false;
        }
    }


    return checkWasSceduled;
}


//
//----- Round-robin check list scheduler
//
//      For every tick, start on the next check list (so that each list gets
//      a try). Then try to scedule a check. If no success in this list, try
//      the next list. Stop when a list produces a check or all lists have
//      been tried. The idea is to produce a single check for each tick and
//      at the same time give all list a fair chance.
//
void ICELIB_tickStreamController(ICELIB_INSTANCE *pInstance)
{
    unsigned int i;
    unsigned int startIndex;
    unsigned int j;

    startIndex = pInstance->roundRobinStreamControllerIndex;

    for (i=0; i < pInstance->numberOfMediaStreams; ++i) {
        j = (i + startIndex) % pInstance->numberOfMediaStreams;
        if (ICELIB_scheduleSingle(pInstance,
                                  &pInstance->streamControllers[ j],
                                   &pInstance->callbacks.callbackLog)) {
            ICELIB_log1(&pInstance->callbacks.callbackLog,
                         ICELIB_logDebug,
                         "Check for stramcontroller[%i] was scheduled", j);

            break;
        }
    }

    startIndex = (startIndex + 1) % pInstance->numberOfMediaStreams;
    pInstance->roundRobinStreamControllerIndex = startIndex;
}


//
//----- Unfreeze all frozen check lists
//
void ICELIB_unfreezeAllFrozenCheckLists(ICELIB_STREAM_CONTROLLER streamControllers[],
                                        unsigned int             numberOfMediaStreams,
                                        ICELIB_CALLBACK_LOG      *pCallbackLog)
{
    unsigned int i;

    for (i=0; i < numberOfMediaStreams; ++i) {
        if (ICELIB_isFrozenCheckList(&streamControllers[i].checkList)) {
            ICELIB_unfreezeFrozenCheckList(&streamControllers[i].checkList, pCallbackLog);
        }
    }
}


void ICELIB_recomputeAllPairPriorities(ICELIB_STREAM_CONTROLLER streamControllers[],
                                       unsigned int             numberOfMediaStreams,
                                       bool                     iceControlling)
{
    unsigned int i;

    for (i=0; i < numberOfMediaStreams; ++i) {
        ICELIB_computeListPairPriority(&streamControllers[i].checkList,
                                       iceControlling);
        ICELIB_sortPairsCL(&streamControllers[i].checkList);
    }
}


#define ICELIB_STREAM_NOT_FOUND  0xFFFFFFFF

//
//----- Find which stream that is associated with the specified HOST address
//
//      Return:   0..n                      - the media stream index
//                ICELIB_STREAM_NOT_FOUND   - no match for specified address
//
unsigned int ICELIB_findStreamByAddress(ICELIB_STREAM_CONTROLLER streamControllers[],
                                        unsigned int             numberOfMediaStreams,
                                        const struct sockaddr           *pHostAddr)
{
    unsigned int       i;
    unsigned int       j;
    const struct sockaddr     *pAddr;
    ICELIB_CHECKLIST   *pCheckList;

    for (i=0; i < numberOfMediaStreams; ++i) {
        pCheckList = &streamControllers[i].checkList;
        for (j=0; j < pCheckList->numberOfPairs; ++j) {
            if (pCheckList->checkListPairs[j].pLocalCandidate->type == ICE_CAND_TYPE_HOST)
            {
                pAddr = (struct sockaddr *)&pCheckList->checkListPairs[j].pLocalCandidate->connectionAddr;
                if (sockaddr_alike(pAddr, pHostAddr))
                {
                    return i;
                }
            }
        }
    }

    return ICELIB_STREAM_NOT_FOUND;
}


//-----------------------------------------------------------------------------
//
//===== Local Functions: Basic Lists
//

//----- 1) Array for the Valid List (VL)

int ICELIB_listCompareVL(const void *cp1, const void *cp2)
{
    const ICELIB_VALIDLIST_ELEMENT *pPair1 = (const ICELIB_VALIDLIST_ELEMENT *)cp1;
    const ICELIB_VALIDLIST_ELEMENT *pPair2 = (const ICELIB_VALIDLIST_ELEMENT *)cp2;

    if (pPair1->pairPriority > pPair2->pairPriority) return -1;
    if (pPair1->pairPriority < pPair2->pairPriority) return  1;

    return 0;
}


void ICELIB_listSortVL(ICELIB_LIST_VL *pList)
{
    qsort(pList->elements,
           pList->numberOfElements,
           sizeof(ICELIB_VALIDLIST_ELEMENT),
           ICELIB_listCompareVL);
}


void ICELIB_listConstructorVL(ICELIB_LIST_VL *pList)
{
    memset(pList, 0, sizeof(*pList));
}


unsigned int ICELIB_listCountVL(const ICELIB_LIST_VL *pList)
{
    return(pList->numberOfElements);
}


//
//----- Return: pointer to element  - OK
//              NULL                - index out of range
//
ICELIB_VALIDLIST_ELEMENT *pICELIB_listGetElementVL(ICELIB_LIST_VL *pList,
                                                    unsigned int   index)
{
    if (index < pList->numberOfElements) {
        return(&pList->elements[ index]);
    }

    return NULL;
}


//
//----- Return: true  - list is full
//              false - element added
//
bool ICELIB_listAddBackVL(ICELIB_LIST_VL           *pList,
                           ICELIB_VALIDLIST_ELEMENT *pPair)
{
    if (pList->numberOfElements >= ICELIB_MAX_VALID_ELEMENTS) return true;
    pList->elements[ pList->numberOfElements++] = *pPair;
    return false;
}


//
//----- Insert into list in order of pair priority
//
//      Return: true  - list is full
//              false - element added
//
bool ICELIB_listInsertVL(ICELIB_LIST_VL            *pList,
                           ICELIB_VALIDLIST_ELEMENT  *pPair)
{
    if (ICELIB_listAddBackVL(pList, pPair)) return true;
    ICELIB_listSortVL(pList);
    return false;
}


//-----------------------------------------------------------------------------
//
//===== Local Functions: Valid List
//

void ICELIB_validListConstructor(ICELIB_VALIDLIST *pValidList)
{
    memset(pValidList, 0, sizeof(*pValidList));
}


unsigned int ICELIB_validListCount(const ICELIB_VALIDLIST *pValidList)
{
    return(ICELIB_listCountVL(&pValidList->pairs));
}


//
//----- Return: pointer to element  - OK
//              NULL                - index out of range
//
ICELIB_VALIDLIST_ELEMENT *pICELIB_validListGetElement(ICELIB_VALIDLIST *pValidList,
                                                       unsigned int      index)
{
    return(pICELIB_listGetElementVL(&pValidList->pairs, index));
}


//
//----- Get pairId of the pair last added or inserted
//
uint32_t ICELIB_validListLastPairId(const ICELIB_VALIDLIST *pValidList)
{
    return(pValidList->nextPairId);
}


void ICELIB_validListIteratorConstructor(ICELIB_VALIDLIST_ITERATOR *pIterator,
                                          ICELIB_VALIDLIST          *pValidList)
{
    pIterator->index      = 0;
    pIterator->pValidList = pValidList;
}


ICELIB_VALIDLIST_ELEMENT *pICELIB_validListIteratorNext(ICELIB_VALIDLIST_ITERATOR *pIterator)
{
    return(pICELIB_validListGetElement(pIterator->pValidList, pIterator->index++));
}


//
//----- Find pair in valid list be ID
//
//      Return: NULL            - could not find pair with specified pairId
//              pointer to pair - pair found
//
ICELIB_VALIDLIST_ELEMENT *pICELIB_validListFindPairById(ICELIB_VALIDLIST *pValidList,
                                                         uint32_t pairId)
{
    ICELIB_VALIDLIST_ELEMENT    *pValidPair;
    ICELIB_VALIDLIST_ITERATOR   vlIterator;

    ICELIB_validListIteratorConstructor(&vlIterator, pValidList);

    while ((pValidPair = pICELIB_validListIteratorNext(&vlIterator)) != NULL) {
        if (pValidPair->pairId == pairId) {
            return pValidPair;
        }
    }

    return NULL;
}



void ICELIB_storeRemoteCandidates(ICELIB_INSTANCE *pInstance) {


    ICELIB_VALIDLIST         *pValidList;
    unsigned int i,j;

    for (i=0;i<pInstance->numberOfMediaStreams;i++) {
        pValidList = &pInstance->streamControllers[i].validList;
        pInstance->streamControllers[i].remoteCandidates.numberOfComponents = 0;

        for (j=0;j<pValidList->pairs.numberOfElements;j++) {
            const ICE_CANDIDATE    *pRemoteCandidate = pValidList->pairs.elements[j].pRemoteCandidate;
            //RTP
            if (pValidList->pairs.elements[j].nominatedPair &&
                pRemoteCandidate->componentid == ICELIB_RTP_COMPONENT_ID) {

                pInstance->streamControllers[i].remoteCandidates.
                    remoteCandidate[ICELIB_RTP_COMPONENT_INDEX].componentId = ICELIB_RTP_COMPONENT_ID;
                sockaddr_copy((struct sockaddr *)&pInstance->streamControllers[i].remoteCandidates.
                              remoteCandidate[ICELIB_RTP_COMPONENT_INDEX].connectionAddr,
                              (struct sockaddr *)&pRemoteCandidate->connectionAddr);
                pInstance->streamControllers[i].remoteCandidates.
                    remoteCandidate[ICELIB_RTP_COMPONENT_INDEX].type = pRemoteCandidate->type;
                pInstance->streamControllers[i].remoteCandidates.numberOfComponents++;
            }
            //RTCP
            if (pValidList->pairs.elements[j].nominatedPair &&
                pRemoteCandidate->componentid == ICELIB_RTCP_COMPONENT_ID) {

                pInstance->streamControllers[i].remoteCandidates.
                    remoteCandidate[ICELIB_RTCP_COMPONENT_INDEX].componentId = ICELIB_RTCP_COMPONENT_ID;
                sockaddr_copy((struct sockaddr *)&pInstance->streamControllers[i].remoteCandidates.
                             remoteCandidate[ICELIB_RTCP_COMPONENT_INDEX].connectionAddr,
                              (struct sockaddr *)&pRemoteCandidate->connectionAddr);
                pInstance->streamControllers[i].remoteCandidates.
                    remoteCandidate[ICELIB_RTCP_COMPONENT_INDEX].type = pRemoteCandidate->type;
                pInstance->streamControllers[i].remoteCandidates.numberOfComponents++;
            }

        }
    }
}


//
//----- Nominate pair (set nominated flag)
//
//      Return: true  - Pair nominated
//              false - could not find pair in valid list.
//
bool ICELIB_validListNominatePair(ICELIB_VALIDLIST *pValidList,
                                  ICELIB_LIST_PAIR *pPair,
                                  const struct sockaddr   *pMappedAddress)
{
    unsigned int i;
    ICELIB_LIST_PAIR *pPairFromList;

    ICELIB_LIST_PAIR dummyPair;

    ICE_CANDIDATE    dummyCandidate;

    for (i=0; i < pValidList->pairs.numberOfElements; i++) {
        pPairFromList = &pValidList->pairs.elements[ i];
        if (ICELIB_isPairAddressMatch(pPairFromList,  pPair)) {
            pPairFromList->nominatedPair = true;
            return true;
        }
    }

    //Try with mapped addres innstead (Or maybee thats what we always should do?)

    sockaddr_copy((struct sockaddr *)&dummyCandidate.connectionAddr, 
                  (struct sockaddr *)pMappedAddress);
    dummyPair.pLocalCandidate = &dummyCandidate;
    dummyPair.pRemoteCandidate = pPair->pRemoteCandidate;


    for (i=0; i < pValidList->pairs.numberOfElements; i++) {
        pPairFromList = &pValidList->pairs.elements[ i];
        if (ICELIB_isPairAddressMatch(pPairFromList,  &dummyPair)) {
            pPairFromList->nominatedPair = true;
            return true;
        }
    }


    return false;
}


//
//----- Add to back of list
//
//      Return: true  - list is full
//              false - element added
//
bool ICELIB_validListAddBack(ICELIB_VALIDLIST         *pValidList,
                             ICELIB_VALIDLIST_ELEMENT *pPair)
{
    ICELIB_VALIDLIST_ELEMENT validListPair;

    validListPair        = *pPair;
    validListPair.pairId = pValidList->nextPairId + 1;

    if (ICELIB_listAddBackVL(&pValidList->pairs, &validListPair)) return true;

    ++pValidList->nextPairId;

    return false;
}


//
//----- Insert into list in order of priority
//
//      Return: true  - list is full
//              false - element added
//
bool ICELIB_validListInsert(ICELIB_VALIDLIST         *pValidList,
                            ICELIB_VALIDLIST_ELEMENT *pPair)
{
    if (ICELIB_validListAddBack(pValidList, pPair)) return true;
    ICELIB_listSortVL(&pValidList->pairs);
    return false;
}


//
//----- Count the number of nominated pairs in the Valid List
//
unsigned int ICELIB_countNominatedPairsInValidList(ICELIB_VALIDLIST *pValidList)
{
    unsigned int                    count;
    const ICELIB_VALIDLIST_ELEMENT  *pValidPair;
    ICELIB_VALIDLIST_ITERATOR       vlIterator;

    count = 0;

    ICELIB_validListIteratorConstructor(&vlIterator, pValidList);

    while ((pValidPair = pICELIB_validListIteratorNext(&vlIterator)) != NULL) {
        if (pValidPair->nominatedPair) ++count;
    }

    return count;
}

//
//----- Find pair by referd to by a checklist idx
//
ICELIB_VALIDLIST_ELEMENT *ICELIB_findElementInValidListByid(ICELIB_VALIDLIST *pValidList, uint32_t id)
{
    ICELIB_VALIDLIST_ELEMENT  *pValidPair;
    ICELIB_VALIDLIST_ITERATOR       vlIterator;

    ICELIB_validListIteratorConstructor(&vlIterator, pValidList);

    while ((pValidPair = pICELIB_validListIteratorNext(&vlIterator)) != NULL) {
        if (pValidPair->refersToPairId == id) {
            return pValidPair;
        }
    }

    return NULL;;
}

//
//----- Check if there is a pair in the Valid List for each component of the
//      media stream. Note: local and remote candidates in a pair always have
//      the same component ID!
//
//      Return: true  - each component has a pair in the Valid List
//              false - one or more components are missing
//
bool ICELIB_isPairForEachComponentInValidList(ICELIB_VALIDLIST           *pValidList,
                                              const ICELIB_COMPONENTLIST *pComponentList)
{
    bool                            foundComponent;
    unsigned int                    i;
    uint32_t                        componentId;
    const ICELIB_VALIDLIST_ELEMENT  *pValidPair;
    ICELIB_VALIDLIST_ITERATOR       vlIterator;

    for (i=0; i < pComponentList->numberOfComponents; ++i) {
        componentId    = pComponentList->componentIds[ i];
        foundComponent = false;

        ICELIB_validListIteratorConstructor(&vlIterator, pValidList);

        while ((pValidPair = pICELIB_validListIteratorNext(&vlIterator)) != NULL) {
            if (pValidPair->pLocalCandidate->componentid == componentId) {
                foundComponent = true;
                break;
            }
        }
        if (foundComponent == false) return false;
    }

    return true;
}


//
//----- Unfreeze pairs with foundations matching foundations in Valid List
//
//      Changes the state of all Frozen pairs in the check list whose foundation
//      matches a pair in the valid list under consideration, to Waiting.
//
void ICELIB_unfreezePairsByMatchingFoundation(ICELIB_VALIDLIST *pValidList,
                                              ICELIB_CHECKLIST *pCheckList,
                                              ICELIB_CALLBACK_LOG *pCallbackLog)
{
    char  foundationInValidList[ ICE_MAX_FOUNDATION_PAIR_LENGTH];
    const ICELIB_VALIDLIST_ELEMENT  *pValidPair;
    ICELIB_VALIDLIST_ITERATOR       vlIterator;

    ICELIB_validListIteratorConstructor(&vlIterator, pValidList);

    while ((pValidPair = pICELIB_validListIteratorNext(&vlIterator)) != NULL) {

        ICELIB_getPairFoundation(foundationInValidList,
                                  ICE_MAX_FOUNDATION_PAIR_LENGTH,
                                  pValidPair);

        ICELIB_unfreezePairsByFoundation(pCheckList, foundationInValidList, pCallbackLog);
    }
}


//
//----- Check if there is at least one foundation match
//
//      Check if there is at least one frozen pair in the check list whose
//      foundation matches a pair in the valid list under consideration.
//
//      Return: true  - at least one pair with matching foundation
//              false - no pairs with matching foundation
//
bool ICELIB_atLeastOneFoundationMatch(ICELIB_VALIDLIST *pValidList,
                                      ICELIB_CHECKLIST *pCheckList)
{
    bool         atLeastOneMatch;
    unsigned int j;
    char         foundationValid[  ICE_MAX_FOUNDATION_PAIR_LENGTH];
    char         foundationFrozen[ ICE_MAX_FOUNDATION_PAIR_LENGTH];
    const ICELIB_VALIDLIST_ELEMENT  *pValidPair;
    ICELIB_VALIDLIST_ITERATOR       vlIterator;

    atLeastOneMatch = false;

    ICELIB_validListIteratorConstructor(&vlIterator, pValidList);

    while ((pValidPair = pICELIB_validListIteratorNext(&vlIterator)) != NULL) {

        ICELIB_getPairFoundation(foundationValid,
                                 ICE_MAX_FOUNDATION_PAIR_LENGTH,
                                 pValidPair);

        for (j=0; j < pCheckList->numberOfPairs; ++j) {
            if (pCheckList->checkListPairs[ j].pairState == ICELIB_PAIR_FROZEN) {

                ICELIB_getPairFoundation(foundationFrozen,
                                         ICE_MAX_FOUNDATION_PAIR_LENGTH,
                                         &pCheckList->checkListPairs[ j]);

                if (strcmp(foundationValid, foundationFrozen) == 0) {
                    atLeastOneMatch = true;
                    break;
                }
            }
        }

        if (atLeastOneMatch) break;
    }

    return atLeastOneMatch;
}

void ICELIB_resetCallbacks(ICELIB_CALLBACKS *pCallbacks)
{
    memset(pCallbacks, 0, sizeof(*pCallbacks));
}


void ICELIB_callbackConstructor(ICELIB_CALLBACKS *pCallbacks,
                                ICELIB_INSTANCE  *pInstance)
{
    ICELIB_resetCallbacks(pCallbacks);
    pCallbacks->callbackRequest.pInstance   = pInstance;
    pCallbacks->callbackResponse.pInstance  = pInstance;
    pCallbacks->callbackKeepAlive.pInstance = pInstance;
    pCallbacks->callbackComplete.pInstance  = pInstance;
    pCallbacks->callbackLog.pInstance       = pInstance;
}


//
//      ICE-19: 7.1.2 Processing the response
//
//
//----- Correlate a Binding Response to its Binding Request
//
//      When a Binding Response is received, it is correlated to its Binding
//      Request using the transaction ID, as defined in
//      [I-D.ietf-behave-rfc3489bis], which then ties it to the candidate
//      pair for which the Binding Request was sent.
//
ICELIB_LIST_PAIR *pICELIB_correlateToRequest(unsigned int    *pStreamIndex,
                                             ICELIB_INSTANCE *pInstance,
                                             const StunMsgId *pTransactionId)
{
    unsigned int     i;
    unsigned int     j;
    unsigned int     k;
    ICELIB_CHECKLIST *pCheckList;
    ICELIB_LIST_PAIR *pair;

    for (i=0; i < pInstance->numberOfMediaStreams; ++i) {
        pCheckList = &pInstance->streamControllers[i].checkList;
        for (j=0; j < pCheckList->numberOfPairs; ++j) {
            pair = &pCheckList->checkListPairs[j];

            for (k=0;k < pair->numberOfTransactionIds; k++) {
                if (ICELIB_compareTransactionId(&pair->transactionIdTable[k],
                                                pTransactionId) == 0) {

                    if (pStreamIndex != NULL) *pStreamIndex = i;
                    return &pCheckList->checkListPairs[ j];
                }
            }
        }
    }

    return NULL;
}


//
//----- Check source and destination addresses against a pair
//
//      Return: true  - addresses match
//              false - addresses does not match
//
bool ICELIB_checkSourceAndDestinationAddr(const ICELIB_LIST_PAIR *pPair,
                                          const struct sockaddr         *source,
                                          const struct sockaddr         *destination)
{
    if (! sockaddr_alike((struct sockaddr *)&pPair->pLocalCandidate->connectionAddr,
                         (struct sockaddr *)destination))
        return false;
    if (! sockaddr_alike((struct sockaddr *)&pPair->pRemoteCandidate->connectionAddr, 
                         (struct sockaddr *)source))
        return false;
    return true;
}


//
//----- Check if the transport addresses if two pairs are equal
//
//      Return: true  - addresses equal
//              false - addresses not equal
//
bool ICELIB_isPairAddressMatch(const ICELIB_LIST_PAIR *pPair1,
                               const ICELIB_LIST_PAIR *pPair2)
{
    if (! sockaddr_alike((struct sockaddr *)&pPair1->pLocalCandidate->connectionAddr,
                         (struct sockaddr *)&pPair2->pLocalCandidate->connectionAddr)) return false;

    if (! sockaddr_alike((struct sockaddr *)&pPair1->pRemoteCandidate->connectionAddr,
                        (struct sockaddr *)&pPair2->pRemoteCandidate->connectionAddr)) return false;

    return true;
}


//
//----- Add a new candidate into a candidate list.
//
//      Return: NULL                  - no more room, list is full!!
//              pointer to candidate  - pointer to added candidate in the list
//
ICE_CANDIDATE *ICELIB_addDiscoveredCandidate(ICE_MEDIA_STREAM    *pMediaStream,
                                             const ICE_CANDIDATE *pPeerCandidate)
{
    ICE_CANDIDATE *pNewCandidate;

    if (pMediaStream->numberOfCandidates >= ICE_MAX_CANDIDATES) {
        return NULL;
    }

    pNewCandidate  = &pMediaStream->candidate[ pMediaStream->numberOfCandidates++];
    *pNewCandidate = *pPeerCandidate;

    return pNewCandidate;
}


//
//----- Make a peer local reflexive candidate according to ICE-19: 7.1.2.2.1
//
void ICELIB_makePeerLocalReflexiveCandidate(ICE_CANDIDATE       *pPeerCandidate,
                                            ICELIB_CALLBACK_LOG *pCallbackLog,
                                            const struct sockaddr      *pMappedAddress,
                                            uint16_t             componentId)
{
    ICELIB_resetCandidate(pPeerCandidate);
    sockaddr_copy((struct sockaddr *)&pPeerCandidate->connectionAddr, 
                  (struct sockaddr *)pMappedAddress);
    pPeerCandidate->type        = ICE_CAND_TYPE_PRFLX;
    pPeerCandidate->componentid = componentId;
    ICELIB_createFoundation(pPeerCandidate->foundation,
                             ICE_CAND_TYPE_PRFLX,
                             ICELIB_FOUNDATION_LENGTH);
    pPeerCandidate->priority = ICELIB_calculatePriority(ICE_CAND_TYPE_PRFLX,
                                                         componentId);

    ICELIB_log(pCallbackLog, ICELIB_logInfo,
                "Peer reflexive candidate generated:");
    ICELIB_candidateDumpLog(pCallbackLog, ICELIB_logInfo, pPeerCandidate);
}

//
//----- Make a peer remote reflexive candidate according to ICE-19: 7.2.1.3
//
void ICELIB_makePeerRemoteReflexiveCandidate(ICE_CANDIDATE       *pPeerCandidate,
                                             ICELIB_CALLBACK_LOG *pCallbackLog,
                                             const struct sockaddr      *sourceAddr,
                                             uint32_t             peerPriority,
                                             uint16_t             componentId)
{
    ICELIB_resetCandidate(pPeerCandidate);

    pPeerCandidate->priority = peerPriority;
    pPeerCandidate->type     = ICE_CAND_TYPE_PRFLX;


    sockaddr_copy((struct sockaddr *)&pPeerCandidate->connectionAddr, 
                  (struct sockaddr *)sourceAddr);
    pPeerCandidate->type        = ICE_CAND_TYPE_PRFLX;
    pPeerCandidate->componentid = componentId;
    strncpy(pPeerCandidate->foundation, "2345\0", ICELIB_FOUNDATION_LENGTH);
    pPeerCandidate->foundation[ICELIB_FOUNDATION_LENGTH - 1] = '\0';

    ICELIB_log(pCallbackLog, ICELIB_logInfo,
               "Peer reflexive candidate generated:");
    ICELIB_candidateDumpLog(pCallbackLog, ICELIB_logInfo, pPeerCandidate);
}


//
//----- See if a pair is already in the check list.
//      Using only local and remote addresses in the check.
//
//      Return: NULL             - pair not found in the check list
//              pointer to pair  - pointer to pair in check list
//
ICELIB_LIST_PAIR *pICELIB_findPairInCheckList(ICELIB_CHECKLIST       *pCheckList,
                                              const ICELIB_LIST_PAIR *pPair)
{
    unsigned int i;
    ICELIB_LIST_PAIR *pPairFromList;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        pPairFromList = &pCheckList->checkListPairs[i];
        if (ICELIB_isPairAddressMatch(pPairFromList,  pPair)) {
            return pPairFromList;
        }
    }

    return NULL;
}


bool ICELIB_isAllPairsFailedOrSucceded(const ICELIB_CHECKLIST *pCheckList)
{
    unsigned int i;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        if ((pCheckList->checkListPairs[ i].pairState != ICELIB_PAIR_SUCCEEDED) &&
           (pCheckList->checkListPairs[ i].pairState != ICELIB_PAIR_FAILED)) {
            return false;
        }
    }

    return true;
}


//
//----- ICE-19: 7.1.2.3.  Check List and Timer State Updates
//
//      Regardless of whether the check was successful or failed, the
//      completion of the transaction may require updating of check list and
//      timer states.
//
void ICELIB_updateCheckListState(ICELIB_CHECKLIST         *pCheckList,
                                 ICELIB_VALIDLIST         *pValidList,
                                 ICELIB_STREAM_CONTROLLER  streamControllers[],
                                 unsigned int              numberOfMediaStreams,
                                 ICELIB_CALLBACK_LOG      *pCallbackLog)
{
    //
    // If all of the pairs in the check list are now either in the Failed or
    // Succeeded state:
    //
    if (ICELIB_isAllPairsFailedOrSucceded(pCheckList)) {

        // If there is not a pair in the valid list for each component of the
        // media stream, the state of the check list is set to Failed.

        if (! ICELIB_isPairForEachComponentInValidList(pValidList, &pCheckList->componentList)) {
            pCheckList->checkListState = ICELIB_CHECKLIST_FAILED;
        }

        // For each frozen check list, the agent:
        // *  Groups together all of the pairs with the same foundation,
        // *  For each group, sets the state of the pair with the lowest
        //    component ID to Waiting. If there is more than one such pair,
        //    the one with the highest priority is used.

        ICELIB_unfreezeAllFrozenCheckLists(streamControllers,
                                           numberOfMediaStreams,
                                           pCallbackLog);
    }

}


void ICELIB_processSuccessResponse(ICELIB_INSTANCE         *pInstance,
                                   const ICE_MEDIA_STREAM  *pLocalMediaStream,
                                   ICE_MEDIA_STREAM        *pDiscoveredLocalCandidates,
                                   ICELIB_CHECKLIST        *pCurrentCheckList,
                                   ICELIB_VALIDLIST        *pValidList,
                                   ICELIB_LIST_PAIR        *pPair,
                                   const struct sockaddr   *pMappedAddress,
                                   bool                    iceControlling)
{
    bool                     listFull;
    uint16_t                 componentId;
    ICELIB_LIST_PAIR         candidatePair;
    ICELIB_LIST_PAIR         *pKnownPair;
    ICELIB_VALIDLIST_ELEMENT *pValidPair;
    uint32_t                 validPairId;
    const ICE_CANDIDATE      *pLocalCandidate;
    ICE_CANDIDATE            peerLocalCandidate;
    char                     pairFoundation[ICE_MAX_FOUNDATION_PAIR_LENGTH];


    ICELIB_log(&pInstance->callbacks.callbackLog,
                ICELIB_logDebug,
                "Got Binding Response!!! Sucsess Cases!");
    componentId = pPair->pLocalCandidate->componentid;
    validPairId = 0;

    //
    //----- Updating the Nominated Flag
    //
    //      ICE-19: 7.1.2.2.4 Updating the Nominated Flag
    //
    //      If the agent was a controlling agent, and it had included a USE-
    //      CANDIDATE attribute in the Binding Request, the valid pair generated
    //      from that check has its nominated flag set to true.
    //
    if (pInstance->iceControlling) {
        if (pPair->useCandidate) {
            ICELIB_log(&pInstance->callbacks.callbackLog,
                       ICELIB_logDebug,
                       "\033[1;31m Controlling UseCandidate\033[0m");
            //Must check mapped address as local in pair...

            if (!ICELIB_validListNominatePair(pValidList, pPair, pMappedAddress)) {
                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logError,
                           "Can't find generated pair in valid list!");
                ICELIB_pairDumpLog(&pInstance->callbacks.callbackLog,
                                   ICELIB_logError,
                                   &candidatePair);

                return;

            }

            ICELIB_log(&pInstance->callbacks.callbackLog,
                       ICELIB_logDebug,
                       "Pair Nominated!");

            ICELIB_updatingStates(pInstance);
            return;
        }
    } else {
        // See also ICE-19: Section 7.2.1.5
        if (pPair->useCandidate) {

            ICELIB_log(&pInstance->callbacks.callbackLog,
                       ICELIB_logDebug,
                       "\033[1;31m Controlled UseCandidate\033[0m");

            if (pPair->pairState == ICELIB_PAIR_SUCCEEDED) {
                if (!ICELIB_validListNominatePair(pValidList, pPair, pMappedAddress)) {

                    ICELIB_log(&pInstance->callbacks.callbackLog,
                               ICELIB_logError,
                               "Can't find generated pair in valid list (Controlled)!");
                    ICELIB_pairDumpLog(&pInstance->callbacks.callbackLog,
                                       ICELIB_logError,
                                       &candidatePair);
                }
                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           "Controlled Nominating!");
                ICELIB_updatingStates(pInstance);
                return;

            }
            if (pPair->pairState == ICELIB_PAIR_INPROGRESS) {
                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           "PAIR IN PROGRESS");
            }
        }
    }

    //
    //----- Discovering Peer Reflexive Local Candidates
    //
    //      ICE-19: 7.1.2.2.1 Discovering Peer Reflexive Candidates
    //
    //      The agent checks the mapped address from the STUN response. If the
    //      transport address does not match any of the local candidates that the
    //      agent knows about, the mapped address represents a new candidate - a
    //      peer reflexive candidate. Like other candidates, it has a type,
    //      base, priority and foundation.
    //
    pLocalCandidate = pICELIB_findCandidate(pLocalMediaStream, pMappedAddress);

    if (pLocalCandidate  == NULL) {
        pLocalCandidate = pICELIB_findCandidate(pDiscoveredLocalCandidates,
                                                pMappedAddress);
        if (pLocalCandidate == NULL) {

            // Unknown: must be a peer reflexive local address
            ICELIB_makePeerLocalReflexiveCandidate(&peerLocalCandidate,
                                                   &pInstance->callbacks.callbackLog,
                                                   pMappedAddress,
                                                   componentId);

            pLocalCandidate = ICELIB_addDiscoveredCandidate(pDiscoveredLocalCandidates,
                                                            &peerLocalCandidate);
            if (pLocalCandidate == NULL) {
                ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logError,
                           "Discovered local candidate list full!");
                return;
            }
        }
    }

    //----- Constructing a Valid Pair
    //
    //      ICE-19: 7.1.2.2.2 Constructing a Valid Pair
    //
    //      The agent constructs a candidate pair whose local candidate equals
    //      the mapped address of the response, and whose remote candidate equals
    //      the destination address to which the request was sent. This is
    //      called a valid pair, since it has been validated by a STUN
    //      connectivity check.
    //
    ICELIB_resetPair(&candidatePair);

    candidatePair.pairId           = 0;   // Updated when inserted into valid list
    candidatePair.refersToPairId   = pPair->pairId;
    candidatePair.pLocalCandidate  = pLocalCandidate;
    candidatePair.pRemoteCandidate = pPair->pRemoteCandidate;

    if (candidatePair.pLocalCandidate->componentid == candidatePair.pRemoteCandidate->componentid) {
        // Everyhting fine.
        ICELIB_changePairState(&candidatePair, ICELIB_PAIR_SUCCEEDED, &pInstance->callbacks.callbackLog);
    } else {
        // We have paired up an RTP candidate to an RTCP candidate!
        // Something wrong must have happenned in the port mappings on a firewall for example.
        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logError,
                   "Candidates in constructed pair from different component id!");
        ICELIB_changePairState(&candidatePair,
                               ICELIB_PAIR_FAILED,
                               &pInstance->callbacks.callbackLog);
    }

    ICELIB_computePairPriority(&candidatePair, iceControlling);
    //
    //  The valid pair may equal the pair that generated
    //  the check, may equal a different pair in the check list, or may be a
    //  pair not currently on any check list.
    //
    pKnownPair = pICELIB_findPairInCheckList(pCurrentCheckList,
                                             &candidatePair);
    if (pKnownPair != NULL) {

        //- Pair is known, add constructed pair to valid list

        ICELIB_log1(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug,
                    "\033[1;31m Known Pair. Adding to valid List (%i)\033[0m", pKnownPair->pairId);

        listFull = ICELIB_validListInsert(pValidList, &candidatePair);

        if (listFull) {
            ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logError,
                       "Valid list full!");
            return;
        }
        validPairId = ICELIB_validListLastPairId(pValidList);

        pPair->refersToPairId = validPairId;

        pValidPair = pICELIB_validListFindPairById(pValidList,validPairId);
        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
                   "Valid pair generated (known): ");
        ICELIB_pairDumpLog(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           pValidPair);
    } else {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug,
                    "\033[1;31m <ICELIB> UnKnown Pair. Adding constructed pair to valid List\033[0m");

        listFull = ICELIB_validListInsert(pValidList, &candidatePair);

        if (listFull) {
            ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logError,
                       "Valid list full!");
            return;
        }

        validPairId = ICELIB_validListLastPairId(pValidList);
        pPair->refersToPairId = validPairId;

        pValidPair = pICELIB_validListFindPairById(pValidList,validPairId);
        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
                   "Valid pair generated (not known): ");
        ICELIB_pairDumpLog(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           pValidPair);
    }

    //
    //----- Updating Pair States
    //
    //      ICE-19: 7.1.2.2.3 Updating Pair States
    //
    //      The agent sets the state of the pair that generated the check to
    //      Succeeded. The success of this check might also cause the state of
    //      other checks to change as well. The agent MUST perform the following
    //      two steps:
    //
    ICELIB_changePairState(pPair, ICELIB_PAIR_SUCCEEDED, &pInstance->callbacks.callbackLog);
    //
    //      1. The agent changes the states for all other Frozen pairs for the
    //         same media stream and same foundation to Waiting. Typically
    //         these other pairs will have different component IDs but not
    //         always.
    //
    ICELIB_getPairFoundation(pairFoundation,
                             ICE_MAX_FOUNDATION_PAIR_LENGTH,
                             pPair);
    ICELIB_unfreezePairsByFoundation(pCurrentCheckList, pairFoundation, &pInstance->callbacks.callbackLog);
    //
    //      2. If there is a pair in the valid list for every component of this
    //         media stream (where this is the actual number of components being
    //         used, in cases where the number of components signaled in the SDP
    //         differs from offerer to answerer), the success of this check may
    //         unfreeze checks for *other* media streams. Note that this step is
    //         followed not just the first time the valid list under
    //         consideration has a pair for every component, but every
    //         subsequent time a check succeeds and adds yet another pair to
    //         that valid list.
    //
    if (ICELIB_isPairForEachComponentInValidList(pValidList,
                                                 &pCurrentCheckList->componentList)) {

        bool             atLeastOneMatch;
        unsigned int     i;
        ICELIB_CHECKLIST *pOtherCheckList;
        //
        //- The agent examines the check list for each *other* media stream in turn:
        //
        for (i=0; i < pInstance->numberOfMediaStreams; ++i) {
            pOtherCheckList = &pInstance->streamControllers[i].checkList;

            if (pOtherCheckList != pCurrentCheckList) {
                // * If the check list is active, the agent changes the state of
                //   all Frozen pairs in that check list whose foundation matches
                //   a pair in the valid list under consideration, to Waiting.

                if (ICELIB_isActiveCheckList(pOtherCheckList)) {
                    ICELIB_unfreezePairsByMatchingFoundation(pValidList,
                                                              pOtherCheckList,
                                                              &pInstance->callbacks.callbackLog);
                }

                if (ICELIB_isFrozenCheckList(pOtherCheckList)) {
                    atLeastOneMatch = ICELIB_atLeastOneFoundationMatch(pValidList,
                                                                       pOtherCheckList);

                    if (atLeastOneMatch) {
                        // * If the check list is frozen, and there is at least
                        //   one pair in the check list whose foundation matches
                        //   a pair in the valid list under consideration, the
                        //   state of all pairs in the check list whose foundation
                        //   matches a pair in the valid list under consideration
                        //   are set to Waiting.

                        ICELIB_unfreezePairsByMatchingFoundation(pValidList,
                                                                  pOtherCheckList,
                                                                  &pInstance->callbacks.callbackLog);

                    } else {

                        // * If the check list is frozen, and there are no pairs
                        //   in the check list whose foundation matches a pair
                        //   in the valid list under consideration, the agent
                        //   +  Groups together all of the pairs with the same
                        //      foundation,
                        //   +  For each group, sets the state of the pair with
                        //      the lowest component ID to Waiting. If there is
                        //      more than one such pair, the one with the highest
                        //      priority is used.

                        ICELIB_unfreezeFrozenCheckList(pOtherCheckList, &pInstance->callbacks.callbackLog);
                    }
                }
            }
        }
    }
    ICELIB_updatingStates(pInstance);
}


void ICELIB_incommingBindingResponse(ICELIB_INSTANCE  *pInstance,
                                     uint16_t          errorResponse,
                                     StunMsgId         transactionId,
                                     const struct sockaddr   *source,
                                     const struct sockaddr   *destination,
                                     const struct sockaddr   *mappedAddress)
{
    unsigned int            streamIndex;
    bool                    iceControlling;
    ICELIB_LIST_PAIR        *pPair;
    ICELIB_TRIGGERED_FIFO   *pTriggeredChecksFifo;
    ICELIB_CHECKLIST        *pCheckList;
    ICELIB_VALIDLIST        *pValidList;
    const ICE_MEDIA_STREAM  *pLocalMediaStream;
    ICE_MEDIA_STREAM        *pDiscoveredLocalCandidates;

    //
    //----- Correlate to the Binding Request using the transaction ID
    //
    //      ICE-19: 7.1.2 Processing the Response
    //
    //      When a Binding Response is received, it is correlated to its Binding
    //      Request using the transaction ID, as defined in
    //      [I-D.ietf-behave-rfc3489bis], which then ties it to the candidate
    //      pair for which the Binding Request was sent.
    //
    pPair = pICELIB_correlateToRequest(&streamIndex, pInstance, &transactionId);

    if (pPair == NULL) {
        if (pInstance->iceState != ICELIB_COMPLETED) {

            ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                       "Can't correlate incomming Binding Response!");

            ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning,"Transaction ID was: ");
            ICELIB_transactionIdDumpLog(&pInstance->callbacks.callbackLog, ICELIB_logWarning, transactionId);
            ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "\n");
        }
        return;
    }

    pCheckList = &pInstance->streamControllers[streamIndex].checkList;

    if (!pPair->useCandidate) {
        if ((pPair->pairState != ICELIB_PAIR_INPROGRESS) &&
            (pPair->pairState != ICELIB_PAIR_WAITING)) {
            ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                        "Binding Response to not-in-progress pair!");

            if (pInstance->iceConfiguration.logLevel == ICELIB_logDebug) {
                debug(pCheckList, pPair->pairId, " <-- binding response to not-in-progress pair ignored");
            }
            return;
        }
    }else{
        //Do Nothing...(debuging)

        //printf("   <icelib> Got response to a use-candidate pair!!\n");
        //printf("Transaction ID: ");
        //STUNMSG_transactionIdDump(transactionId);
        //printf("\n");

        //ICELIB_checkListDumpAll(pInstance);

        //netaddr_dump(&pPair->pLocalCandidate->connectionAddr, true);
        //printf(" type: %i ", pPair->pLocalCandidate->type);
        //printf("  :  ");
        //netaddr_dump(&pPair->pRemoteCandidate->connectionAddr, true);
        //printf(" type: %i ", pPair->pRemoteCandidate->type);
        //printf("\n");
    }

    if (pInstance->iceConfiguration.logLevel == ICELIB_logDebug) {
        debug(pCheckList, pPair->pairId, " <-- Binding Response");
    }

    iceControlling    = pInstance->iceControlling;
    pLocalMediaStream = &pInstance->localIceMedia.mediaStream[ streamIndex];

    pValidList                 = &pInstance->streamControllers[streamIndex].validList;
    pTriggeredChecksFifo       = &pInstance->streamControllers[streamIndex].triggeredChecksFifo;
    pDiscoveredLocalCandidates = &pInstance->streamControllers[streamIndex].discoveredLocalCandidates;

    //
//----- Handle a 487 Role Conflict error response
//
//      ICE-19: 7.1.2.1 Failure Cases
//
//      Invert CONTROLLING state.
//      Bug in ICE-19: should also recompute priorities
//      Set pair state to Waiting
//      Enqueue the failed pair into the triggered check queue.
//
    if (errorResponse == 487) {

        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                   "Error response 487: Role Conflict!");

        pInstance->iceControlling = ! pInstance->iceControlling;
        pInstance->iceControlled  = ! pInstance->iceControlled;

        ICELIB_recomputeAllPairPriorities(pInstance->streamControllers,
                                          pInstance->numberOfMediaStreams,
                                          pInstance->iceControlling);

        ICELIB_log1(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                    "Changing role, iceControlling now: %d!",
                    pInstance->iceControlling);

        ICELIB_changePairState(pPair, ICELIB_PAIR_WAITING, &pInstance->callbacks.callbackLog);

        if (ICELIB_triggeredFifoPut(pTriggeredChecksFifo, pPair)) {
            ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logError,
                       "Triggered check queue full!");
        }
        return;
    }

    if (errorResponse < 200) {
        ICELIB_log1(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                    "Unexpected error response: %u!",
                    errorResponse);
        return;
    }

    //
    //----- Source and destination addresses of response must match request
    //
    //      ICE-19: 7.1.2.1
    //
    //      The agent MUST check that the source IP address and port of the
    //      response equals the destination IP address and port that the Binding
    //      Request was sent to, and that the destination IP address and port of
    //      the response match the source IP address and port that the Binding
    //      Request was sent from. In other words, the source and destination
    //      transport addresses in the request and responses are the symmetric.
    //      If they are not symmetric, the agent sets the state of the pair to
    //      Failed.
    //
    if (! ICELIB_checkSourceAndDestinationAddr(pPair, (struct sockaddr *)source, (struct sockaddr *)destination)) {

        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                    "Source and destination addresses don't match!");


        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "Pair local           : ");
        ICELIB_netAddrDumpLog(&pInstance->callbacks.callbackLog,
                              ICELIB_logWarning,
                              (struct sockaddr *)&pPair->pLocalCandidate->connectionAddr);
        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "\n");
        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "Pair remote          : ");
        ICELIB_netAddrDumpLog(&pInstance->callbacks.callbackLog,
                              ICELIB_logWarning,
                              (struct sockaddr *)&pPair->pRemoteCandidate->connectionAddr);
        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "\n");

        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "Incomming destination: ");
        ICELIB_netAddrDumpLog(&pInstance->callbacks.callbackLog, ICELIB_logWarning, destination);
        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "\n");
        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "Incomming source     : ");
        ICELIB_netAddrDumpLog(&pInstance->callbacks.callbackLog, ICELIB_logWarning, source);
        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "\n");


        ICELIB_changePairState(pPair, ICELIB_PAIR_FAILED, &pInstance->callbacks.callbackLog);

        ICELIB_updateCheckListState(pCheckList,
                                    pValidList,
                                    pInstance->streamControllers,
                                    pInstance->numberOfMediaStreams,
                                    &pInstance->callbacks.callbackLog);
        return;
    }

    if (errorResponse > 299) {
        ICELIB_log1(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                    "Bad error response: %d!",
                    errorResponse);

        ICELIB_changePairState(pPair, ICELIB_PAIR_FAILED, &pInstance->callbacks.callbackLog);

        ICELIB_updateCheckListState(pCheckList,
                                    pValidList,
                                    pInstance->streamControllers,
                                    pInstance->numberOfMediaStreams,
                                    &pInstance->callbacks.callbackLog);
        return;
    }

//
//----- ICE-19: 7.1.2.2.  Success Cases
//
    ICELIB_processSuccessResponse(pInstance,
                                  pLocalMediaStream,
                                  pDiscoveredLocalCandidates,
                                  pCheckList,
                                  pValidList,
                                  pPair,
                                  mappedAddress,
                                  iceControlling);

//
//----- ICE-19: 7.1.2.3.  Check List and Timer State Updates
//
//      Regardless of whether the check was successful or failed, the
//      completion of the transaction may require updating of check list and
//      timer states.
//
    ICELIB_updateCheckListState(pCheckList,
                                pValidList,
                                pInstance->streamControllers,
                                pInstance->numberOfMediaStreams,
                                &pInstance->callbacks.callbackLog);

    if (pInstance->iceConfiguration.logLevel == ICELIB_logDebug) {
        debug(pCheckList, 0, "");
    }
}


//
//----- Timeout occurred on the connectivity check response!
//
void ICELIB_incommingTimeout(ICELIB_INSTANCE  *pInstance,
                             StunMsgId         transactionId)
{
    unsigned int         streamIndex;
    ICELIB_LIST_PAIR    *pPair;
    ICELIB_CHECKLIST    *pCheckList;
    ICELIB_VALIDLIST    *pValidList;

    pPair = pICELIB_correlateToRequest(&streamIndex, pInstance, &transactionId);

    if (pPair == NULL) {

        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                    "Timeout: Can't correlate incomming Binding Response!");

        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "Transaction ID was: ");
        ICELIB_transactionIdDumpLog(&pInstance->callbacks.callbackLog, ICELIB_logWarning, transactionId);
        ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "\n");

        return;
    }

    pCheckList = &pInstance->streamControllers[streamIndex].checkList;
    pValidList = &pInstance->streamControllers[streamIndex].validList;

    if (pPair->pairState == ICELIB_PAIR_INPROGRESS) {

        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning, "Response timeout on pair:");
        ICELIB_pairDumpLog(&pInstance->callbacks.callbackLog, ICELIB_logWarning, pPair);

        ICELIB_changePairState(pPair, ICELIB_PAIR_FAILED, &pInstance->callbacks.callbackLog);

        ICELIB_updateCheckListState(pCheckList,
                                    pValidList,
                                    pInstance->streamControllers,
                                    pInstance->numberOfMediaStreams,
                                    &pInstance->callbacks.callbackLog);

    } else if (pPair->pairState == ICELIB_PAIR_WAITING) {

        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                   "Response timeout. Cancelled, ignore.");

    } else {
        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                   "Timeout on non-INPROGRESS pair:");
        ICELIB_pairDumpLog(&pInstance->callbacks.callbackLog,ICELIB_logWarning,  pPair);
    }
}


//-----------------------------------------------------------------------------
//
//===== Local Functions: Processing a connectivity check request
//
//      ICE-19: 7.2 STUN Server Procedures
//

void ICELIB_sendBindingResponse(ICELIB_INSTANCE   *pInstance,
                                const struct sockaddr    *source,
                                const struct sockaddr    *destination,
                                const struct sockaddr    *MappedAddress,
                                uint32_t           userValue1,
                                uint32_t           userValue2,
                                uint16_t           componentId,
                                uint16_t           errorResponse,
                                StunMsgId          transactionId,
                                bool               useRelay,
                                const char        *pUfragPair,
                                const char        *pPasswd)
{
    ICELIB_outgoingBindingResponse  BindingResponse;

    BindingResponse = pInstance->callbacks.callbackResponse.pICELIB_sendBindingResponse;

    if (BindingResponse != NULL) {
        BindingResponse(pInstance->callbacks.callbackResponse.pBindingResponseUserData,
                        userValue1,
                        userValue2,
                        componentId,
                        source,
                        destination,
                        MappedAddress,
                        errorResponse,
                        transactionId,
                        useRelay,
                        pUfragPair,
                        pPasswd);
    }
}


void ICELIB_processSuccessRequest(ICELIB_INSTANCE         *pInstance,
                                  StunMsgId                transactionId,
                                  const struct sockaddr          *source,
                                  const struct sockaddr          *destination,
                                  const struct sockaddr          *relayBaseAddr,
                                  uint32_t                 userValue1,
                                  uint32_t                 userValue2,
                                  uint32_t                 peerPriority,
                                  const ICE_MEDIA_STREAM  *pLocalMediaStream,
                                  const ICE_MEDIA_STREAM  *pRemoteMediaStream,
                                  ICE_MEDIA_STREAM        *pDiscoveredRemoteCandidates,
                                  ICE_MEDIA_STREAM        *pDiscoveredLocalCandidates,
                                  ICELIB_CHECKLIST        *pCurrentCheckList,
                                  ICELIB_VALIDLIST        *pCurrentValidList,
                                  ICELIB_TRIGGERED_FIFO   *pTriggeredFifo,
                                  bool                     iceControlling,
                                  bool                     useCandidate,
                                  bool                     fromRelay,
                                  uint16_t                 componentId)
{
    ICELIB_LIST_PAIR         candidatePair;
    ICELIB_LIST_PAIR         *pKnownPair;
    ICELIB_VALIDLIST_ELEMENT *pValidElement;
    ICE_CANDIDATE            peerRemoteCandidate;
    char                     ufragPair[ICE_MAX_UFRAG_PAIR_LENGTH];

    ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
                     "ICELIB_processSuccessRequest");

    ICELIB_sendBindingResponse(pInstance,          // pInstance
                               destination,        // source
                               source,             // destination
                               source,             // MappedAddress
                               userValue1,         // userValue1
                               userValue2,         // userValue2
                               componentId,
                               200,                // errorResponse
                               transactionId,
                               fromRelay,
                               ICELIB_getCheckListLocalUsernamePair(ufragPair,
                                                                    ICE_MAX_UFRAG_PAIR_LENGTH,
                                                                    pCurrentCheckList),
                               ICELIB_getCheckListLocalPasswd(pCurrentCheckList));

    //
    //----- Learning Peer Reflexive Candidates
    //
    //      ICE-19: 7.2.1.3. Learning Peer Reflexive Candidates
    //
    if (pICELIB_findCandidate(pRemoteMediaStream, source) == NULL &&
        pICELIB_findCandidate(pDiscoveredRemoteCandidates, source) == NULL) {

        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug,
                   "Making peer refelxive remote Candidate");

        ICELIB_makePeerRemoteReflexiveCandidate(&peerRemoteCandidate,
                                                &pInstance->callbacks.callbackLog,
                                                source,
                                                peerPriority,
                                                componentId);

        ICELIB_addDiscoveredCandidate(pDiscoveredRemoteCandidates, &peerRemoteCandidate);

    }

    //
    //----- Triggered Checks
    //
    //      ICE-19: 7.2.1.4. Triggered Checks
    //

    ICELIB_log(&pInstance->callbacks.callbackLog,
               ICELIB_logDebug,
               "Trigered Checks");

    ICELIB_resetPair(&candidatePair);

    ICELIB_changePairState(&candidatePair, ICELIB_PAIR_IDLE, &pInstance->callbacks.callbackLog);
    candidatePair.pairId = 0;   // Updated when inserted into valid list
    if (fromRelay) {
        candidatePair.pLocalCandidate  = pICELIB_findCandidate(pLocalMediaStream, relayBaseAddr);
        if (candidatePair.pLocalCandidate == NULL) {
            candidatePair.pLocalCandidate = pICELIB_findCandidate(pDiscoveredLocalCandidates, relayBaseAddr);
        }
    }else{
        candidatePair.pLocalCandidate  = pICELIB_findCandidate(pLocalMediaStream, destination);
        if (candidatePair.pLocalCandidate == NULL) {
            candidatePair.pLocalCandidate = pICELIB_findCandidate(pDiscoveredLocalCandidates, destination);
        }
    }
    candidatePair.pRemoteCandidate = pICELIB_findCandidate(pRemoteMediaStream, source);

    if (candidatePair.pRemoteCandidate == NULL) {
        candidatePair.pRemoteCandidate = pICELIB_findCandidate(pDiscoveredRemoteCandidates, source);
    }

    if (candidatePair.pLocalCandidate == NULL) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug,
                   "No Local Candidates found for trigered checks");
        return;
    }
    if (candidatePair.pRemoteCandidate == NULL) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug,
                   "No Remote Candidates found for trigered checks");
        return;
    }

    ICELIB_computePairPriority(&candidatePair, iceControlling);

    pKnownPair = pICELIB_findPairInCheckList(pCurrentCheckList,
                                             &candidatePair);
    if (pKnownPair != NULL) {
        if (!useCandidate) {
            if (pKnownPair->pairState == ICELIB_PAIR_WAITING ||
                pKnownPair->pairState == ICELIB_PAIR_FROZEN) {

                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           "7.2.1.4 Waiting or Frozen");
                ICELIB_triggeredFifoPutIfNotPresent(pTriggeredFifo, pKnownPair, &pInstance->callbacks.callbackLog);
            }
            if (pKnownPair->pairState == ICELIB_PAIR_SUCCEEDED) {
                //Nothing to do
                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           "7.2.1.4 Found Succeeded pair in checklist");
                return;
            }
            if (pKnownPair->pairState == ICELIB_PAIR_INPROGRESS) {
                ICELIB_outgoingCancelRequest CancelReq;

                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           "7.2.1.4 In Progress");

                CancelReq = pInstance->callbacks.callbackCancelRequest.pICELIB_sendCancelRequest;
                if (CancelReq != NULL) {
                    ICELIB_log1(&pInstance->callbacks.callbackLog,
                                ICELIB_logDebug,
                                "Canceling Transaction. Transaction Table Size(%d).", pKnownPair->numberOfTransactionIds);

                    CancelReq(pInstance->callbacks.callbackCancelRequest.pCancelRequestUserData,
                              pInstance->localIceMedia.mediaStream[0].userValue1,
                              pKnownPair->transactionIdTable[0]);
                }
                if (ICELIB_triggeredFifoPut(pTriggeredFifo, pKnownPair)) {
                    ICELIB_log(&pInstance->callbacks.callbackLog,
                               ICELIB_logError,
                               "Triggered Check queue full");
                }
                ICELIB_changePairState(pKnownPair, ICELIB_PAIR_WAITING, &pInstance->callbacks.callbackLog);
            }
            if (pKnownPair->pairState == ICELIB_PAIR_FAILED) {
                if (ICELIB_triggeredFifoPut(pTriggeredFifo, pKnownPair)) {
                    ICELIB_log(&pInstance->callbacks.callbackLog,
                               ICELIB_logError,
                               "Triggered Check queue full");
                }
                ICELIB_changePairState(pKnownPair, ICELIB_PAIR_WAITING, &pInstance->callbacks.callbackLog);
                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logError,
                           "7.2.1.4 Failed");
            }
        } else {
            //
            //----- Updating the Nominated Flag
            //
            //      ICE-19: 7.2.1.5. Updating the Nominated Flag
            //

            ICELIB_log1(&pInstance->callbacks.callbackLog,
                        ICELIB_logDebug,
                        "\033[1;31m <ICELIB> Use Candidate flag detected (ID: %i)\033[0m",
                        pKnownPair->pairId);

            if (pKnownPair->pairState == ICELIB_PAIR_SUCCEEDED) {
                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           "Setting Nominated");

                pValidElement = ICELIB_findElementInValidListByid(pCurrentValidList, pKnownPair->pairId);
                if (pValidElement != NULL) {
                    pValidElement->nominatedPair = true;
                }else{
                    ICELIB_log(&pInstance->callbacks.callbackLog,
                               ICELIB_logError,
                               "Could not find element in validlist");
                }
            }
            if (pKnownPair->pairState == ICELIB_PAIR_INPROGRESS) {
                //TODO: Implement
                printf("<ICELIB> TODO:7.2.1.5 Pair is in-progress\n");
            }
        }
    }else{

        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug,
                   "\033[1;32m<ICELIB> 7.2.1.4 PAIR NOT in checklist\n\033[0m");

        ICELIB_changePairState(&candidatePair, ICELIB_PAIR_WAITING, &pInstance->callbacks.callbackLog);
        if (ICELIB_insertIntoCheckList(pCurrentCheckList,
                                       &candidatePair)) {
            ICELIB_log(&pInstance->callbacks.callbackLog,
                       ICELIB_logError,
                       "Could not insert pair into checklist!");
        }

        if (ICELIB_triggeredFifoPut(pTriggeredFifo, &candidatePair)) {
            ICELIB_log(&pInstance->callbacks.callbackLog,
                       ICELIB_logError,
                       "Triggered Check queue full");
        }
    }
}



void ICELIB_processIncommingLite(ICELIB_INSTANCE  *pInstance,
                                 uint32_t          userValue1,
                                 uint32_t          userValue2,
                                 const char       *pUfragPair,
                                 uint32_t          peerPriority,
                                 bool              useCandidate,
                                 bool              iceControlling,
                                 bool              iceControlled,
                                 uint64_t          tieBreaker,
                                 StunMsgId         transactionId,
                                 const struct sockaddr   *source,
                                 const struct sockaddr   *destination,
                                 uint16_t          componentId)
{
    (void)userValue1;
    (void)userValue2;
    (void)pUfragPair;
    (void)peerPriority;
    (void)useCandidate;
    (void)iceControlling;
    (void)iceControlled;
    (void)tieBreaker;
    (void)transactionId;
    (void)source;
    (void)destination;
    (void)componentId;

    ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
               "Processing incomming request lite (NOT IMPLEMENTED)");
}


void ICELIB_processIncommingFull(ICELIB_INSTANCE  *pInstance,
                                 uint32_t          userValue1,
                                 uint32_t          userValue2,
                                 const char       *pUfragPair,
                                 uint32_t          peerPriority,
                                 bool              useCandidate,
                                 bool              iceControlling,
                                 bool              iceControlled,
                                 uint64_t          tieBreaker,
                                 StunMsgId         transactionId,
                                 const struct sockaddr   *source,
                                 const struct sockaddr   *destination,
                                 bool              fromRelay,
                                 const struct sockaddr   *relayBaseAddr,
                                 uint16_t          componentId)

{
    unsigned int             streamIndex;
    bool                     iceLocalControlling;
    ICELIB_TRIGGERED_FIFO    *pTriggeredChecksFifo;
    ICELIB_CHECKLIST         *pCheckList;
    ICELIB_VALIDLIST         *pValidList;
    ICELIB_TRIGGERED_FIFO    *pTriggeredFifo;
    const ICE_MEDIA_STREAM   *pLocalMediaStream;
    const ICE_MEDIA_STREAM   *pRemoteMediaStream;
    ICE_MEDIA_STREAM         *pDiscoveredRemoteCandidates;
    ICE_MEDIA_STREAM         *pDiscoveredLocalCandidates;
    char                      localUfragPair[ICE_MAX_UFRAG_PAIR_LENGTH];

    //
    //----- Detecting and repairing role Conflicts
    //
    //      ICE-19: 7.2.1.1. Detecting and Repairing Role Conflicts
    //
    if (iceControlling && iceControlled) {
        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logError,
                   "Both ICE-CONTROLLING and ICE-CONTROLLED set!");
    }

    //
    //----- Correlate to media stream using the our own address ("destination")
    //
    streamIndex = ICELIB_findStreamByAddress(pInstance->streamControllers,
                                             pInstance->numberOfMediaStreams,
                                             destination);

    if (streamIndex == ICELIB_STREAM_NOT_FOUND) {
        if (pInstance->iceState != ICELIB_COMPLETED) {
            ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logError,
                        "Can't find media stream index!");
            ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logError,
                              "Desination specified in request: ");
            ICELIB_netAddrDumpLog(&pInstance->callbacks.callbackLog, ICELIB_logError, destination);
            ICELIB_logString(&pInstance->callbacks.callbackLog, ICELIB_logError, "\n");

        }
        return;
    }

    iceLocalControlling = pInstance->iceControlling;
    pCheckList          = &pInstance->streamControllers[streamIndex].checkList;
    pValidList          = &pInstance->streamControllers[streamIndex].validList;
    pTriggeredFifo      = &pInstance->streamControllers[streamIndex].triggeredChecksFifo;
    pLocalMediaStream   = &pInstance->localIceMedia.mediaStream[streamIndex];
    pRemoteMediaStream   = &pInstance->remoteIceMedia.mediaStream[streamIndex];

    pTriggeredChecksFifo        = &pInstance->streamControllers[streamIndex].triggeredChecksFifo;
    pDiscoveredRemoteCandidates = &pInstance->streamControllers[streamIndex].discoveredRemoteCandidates;
    pDiscoveredLocalCandidates  = &pInstance->streamControllers[streamIndex].discoveredLocalCandidates;



    if (pInstance->iceControlling) {
        if (iceControlling) {
            ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                        "Both parties are controlling!");
            if (pInstance->tieBreaker >= tieBreaker) {
                ICELIB_sendBindingResponse(pInstance,
                                           destination,
                                           source,
                                           source,
                                           userValue1,
                                           userValue2,
                                           componentId,
                                           487,
                                           transactionId,
                                           fromRelay,
                                           ICELIB_getCheckListLocalUsernamePair(localUfragPair,
                                                                                ICE_MAX_UFRAG_PAIR_LENGTH,
                                                                                pCheckList),
                                           ICELIB_getCheckListLocalPasswd(pCheckList));
                return;
            }else{
                pInstance->iceControlling = false;
                pInstance->iceControlled = true;
                ICELIB_recomputeAllPairPriorities(pInstance->streamControllers,
                                                  pInstance->numberOfMediaStreams,
                                                  pInstance->iceControlling);
                ICELIB_log1(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
                            "Changing role, iceControlling now: %d!",
                            pInstance->iceControlling);
            }
        }
    } else {
        if (iceControlled) {
            ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                       "Both parties are controlled!");
            if (pInstance->tieBreaker >= tieBreaker) {
                pInstance->iceControlling = true;
                pInstance->iceControlled = false;
                ICELIB_recomputeAllPairPriorities(pInstance->streamControllers,
                                                  pInstance->numberOfMediaStreams,
                                                  pInstance->iceControlling);
                ICELIB_log1(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
                            "Changing role, iceControlling now: %d!",
                            pInstance->iceControlling);
            } else {
                ICELIB_sendBindingResponse(pInstance,
                                           destination,
                                           source,
                                           source,
                                           userValue1,
                                           userValue2,
                                           componentId,
                                           487,
                                           transactionId,
                                           fromRelay,
                                           ICELIB_getCheckListLocalUsernamePair(localUfragPair,
                                                                                ICE_MAX_UFRAG_PAIR_LENGTH,
                                                                                pCheckList),
                                           ICELIB_getCheckListLocalPasswd(pCheckList));
                return;
            }
        }
    }
    //
    //----- Make sure that the ufrag pair match our ufrag pair (TODO: see 7.2)
    //
    if (! ICELIB_compareUfragPair(pUfragPair,
                                  pCheckList->ufragLocal,
                                  pCheckList->ufragRemote)) {
        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                   "UfragPair mismatch!");
        ICELIB_log1(&pInstance->callbacks.callbackLog, ICELIB_logWarning,
                    "Received UfragPair was: '%s'",
                    pUfragPair);

        // TODO: MS seem to return the wrong pair in the response sometimes how should
        // we handle this? For now just ignore and continue
        //return;
    }
    //
    //----- ICE-19: 7.2.1.3 to 7.2.1.5  Success Cases
    //

    ICELIB_processSuccessRequest(pInstance,
                                 transactionId,
                                 source,
                                 destination,
                                 relayBaseAddr,
                                 userValue1,
                                 userValue2,
                                 peerPriority,
                                 pLocalMediaStream,
                                 pRemoteMediaStream,
                                 pDiscoveredRemoteCandidates,
                                 pDiscoveredLocalCandidates,
                                 pCheckList,
                                 pValidList,
                                 pTriggeredFifo,
                                 iceControlling,
                                 useCandidate,
                                 fromRelay,
                                 componentId);

    ICELIB_updatingStates(pInstance);
}


void ICELIB_incommingBindingRequest(ICELIB_INSTANCE   *pInstance,
                                    uint32_t           userValue1,
                                    uint32_t           userValue2,
                                    const char        *pUfragPair,
                                    uint32_t           peerPriority,
                                    bool               useCandidate,
                                    bool               iceControlling,
                                    bool               iceControlled,
                                    uint64_t           tieBreaker,
                                    StunMsgId          transactionId,
                                    const struct sockaddr    *source,
                                    const struct sockaddr    *destination,
                                    bool               fromRelay,
                                    const struct sockaddr    *relayBaseAddr,
                                    uint16_t           componentId)
{


    //
    //----- Have we received the request before having made the check lists?
    //      (Typically: we are controlling but have not received an Answer)
    //
    if (pInstance->iceState == ICELIB_IDLE) {

        // TODO - put into request queue + call callback

        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
                   "Not yet received the ansver: Should  buffering request!");
        //ICELIB_sendBindingResponse(pInstance,
        //                            destination,
        //                            source,
        //                            source,
        //                            userValue1,
        //                            userValue2,
        //                            componentId,
        //                            200,
        //                            transactionId,
        //                            fromRelay);
        return;
    }

    if (pInstance->iceConfiguration.iceLite) {
        ICELIB_processIncommingLite(pInstance,
                                    userValue1,
                                    userValue2,
                                    pUfragPair,
                                    peerPriority,
                                    useCandidate,
                                    iceControlling,
                                    iceControlled,
                                    tieBreaker,
                                    transactionId,
                                    source,
                                    destination,
                                    componentId);
    } else {
        ICELIB_processIncommingFull(pInstance,
                                    userValue1,
                                    userValue2,
                                    pUfragPair,
                                    peerPriority,
                                    useCandidate,
                                    iceControlling,
                                    iceControlled,
                                    tieBreaker,
                                    transactionId,
                                    source,
                                    destination,
                                    fromRelay,
                                    relayBaseAddr,
                                    componentId);
    }
}



//
//      ICE-19: 8. Concluding ICE Processing
//
//
//----- ICE-19: 8.1.  Procedures for Full Implementation
//
//      Concluding ICE involves nominating pairs by the controlling agent and
//      updating of state machinery.
//

bool ICELIB_isNominatingCriteriaMet(ICELIB_VALIDLIST *pValidList)
{
    if (pValidList->readyToNominateWeighting >= ICELIB_COMPLETE_WEIGHT) {
        return true;
    }
    return false;
}

bool ICELIB_isNominatingCriteriaMetForAllMediaStreams(ICELIB_INSTANCE *pInstance)
{
    unsigned int i;
    ICELIB_VALIDLIST        *pValidList;
    ICELIB_CHECKLIST        *pCheckList = NULL;
    uint32_t pathScore = 0;

    for (i=0; i < pInstance->numberOfMediaStreams; ++i) {
        pValidList   = &pInstance->streamControllers[ i].validList;
        if (!ICELIB_isNominatingCriteriaMet(pValidList)) {
            return false;
        }
    }
    //Do additional check that all paths for all media streams are the same;
    pValidList = &pInstance->streamControllers[0].validList;
    pathScore  = pValidList->nominatedPathScore;

    for (i=1; i < pInstance->numberOfMediaStreams; i++) {
        pValidList = &pInstance->streamControllers[i].validList;
        pCheckList = &pInstance->streamControllers[i].checkList;

        if (pathScore != pValidList->nominatedPathScore) {
            ICELIB_updateValidPairReadyToNominateWeightingMediaStream(pCheckList,
                                                                      pValidList,
                                                                      ICELIB_getWeightTimeMultiplier(pInstance));
            return false;
        }
    }
    return true;
}

void ICELIB_stopChecks(ICELIB_INSTANCE  *pInstance,
                       ICELIB_CHECKLIST *pCheckList,
                       ICELIB_TRIGGERED_FIFO *pTriggeredChecksFifo)
{
    unsigned int i;
    ICELIB_outgoingCancelRequest CancelReq;

    ICELIB_log1(&pInstance->callbacks.callbackLog,
                ICELIB_logDebug,
                "Stopping checks (%i)",
                pCheckList->numberOfPairs) ;



    pCheckList->stopChecks = true;

    CancelReq = pInstance->callbacks.callbackCancelRequest.pICELIB_sendCancelRequest;
    for (i=0; i<pCheckList->numberOfPairs; i++) {
        unsigned int j;
        ICELIB_LIST_PAIR *pair = &pCheckList->checkListPairs[i];

        if (pair->pairState == ICELIB_PAIR_INPROGRESS) {
            if (!pair->useCandidate) {
                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           "Pair in progress. Removing FIFO pair");

                ICELIB_triggeredFifoRemove(pTriggeredChecksFifo, pair);
            }else{
                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug,
                           "Won't remove pair from triggeredchcks when USE_CANDIDATE is set");
            }

            if (CancelReq != NULL) {
                for (j=0; j<pair->numberOfTransactionIds; j++) {
                    CancelReq(pInstance->callbacks.callbackCancelRequest.pCancelRequestUserData,
                              pInstance->localIceMedia.mediaStream[0].userValue1,
                              pair->transactionIdTable[j]);
                }
            }
            pair->numberOfTransactionIds = 0;
        }
    }
}



ICELIB_LIST_PAIR *pICELIB_pickValidPairForNominationNormalMode(ICELIB_VALIDLIST *pValidList,
                                                               uint32_t componentId)

{
    unsigned int i;

    for (i=0; i<pValidList->pairs.numberOfElements; i++) {

        const ICE_CANDIDATE    *pLocalCandidate  = pValidList->pairs.elements[i].pLocalCandidate;
        const ICE_CANDIDATE    *pRemoteCandidate = pValidList->pairs.elements[i].pRemoteCandidate;

        uint32_t pathScore = ICELIB_calculateReadyWeight(pLocalCandidate->type,
                                                         pRemoteCandidate->type,
                                                         1);

        if (pLocalCandidate->componentid == componentId) {
            if (pathScore == pValidList->nominatedPathScore) {

                if (pValidList->pairs.elements[i].nominatedPair) {
                    //Already nominated..
                    return NULL;
                }else{
                    return &pValidList->pairs.elements[i];
                }
            }
        }
    }

    return NULL;
}


ICELIB_LIST_PAIR *pICELIB_pickValidPairForNomination(ICELIB_INSTANCE  *pInstance,
                                                     ICELIB_VALIDLIST *pValidList,
                                                     ICELIB_CHECKLIST *pCheckList,
                                                     uint32_t componentId)
{
    (void)pInstance;
    (void)pCheckList;
    return pICELIB_pickValidPairForNominationNormalMode(pValidList, componentId);
}



void ICELIB_enqueueValidPair(ICELIB_TRIGGERED_FIFO *pTriggeredChecksFifo,
                             ICELIB_CHECKLIST      *pCheckList,
                             ICELIB_CALLBACK_LOG   *pCallbackLog,
                             ICELIB_LIST_PAIR      *pValidPair)
{
    ICELIB_LIST_PAIR *pPair;
    uint32_t         refersToPairId;

    //- Find pair the generated the check

    refersToPairId = pValidPair->refersToPairId;

    pPair = ICELIB_getPairById(pCheckList, refersToPairId);

    if (pPair == NULL) {
        ICELIB_log1(pCallbackLog, ICELIB_logError,
                    "Can't find pair in checklist, refersToPairId=%d",
                    refersToPairId);
        return;
    }

    pPair->useCandidate = true;
    pPair->numberOfTransactionIds = 0;
    pCheckList->timerRunning = true;
    if (ICELIB_triggeredFifoPut(pTriggeredChecksFifo, pPair)) {

        ICELIB_log(pCallbackLog, ICELIB_logError,
                   "Triggered check queue full!");
        ICELIB_logString(pCallbackLog, ICELIB_logError, "Pair: ");
        ICELIB_pairDumpLog(pCallbackLog, ICELIB_logError, pPair);
    }
}


void ICELIB_nominateAggressive(ICELIB_INSTANCE *pInstance)
{
    ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logInfo, "Aggressive nomination");
//
//----- ICE-19: 8.1.1.2  Aggressive Nomination
//
// TODO: Implement
}


void ICELIB_nominateRegularIfComplete(ICELIB_INSTANCE *pInstance)
{

    ICELIB_CHECKLIST        *pCheckList;
    ICELIB_VALIDLIST        *pValidList;
    ICELIB_TRIGGERED_FIFO   *pTriggeredChecksFifo;
    ICELIB_LIST_PAIR        *pValidPair;

    if (ICELIB_isNominatingCriteriaMetForAllMediaStreams(pInstance)) {
        unsigned int i;
        ICELIB_log1(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug,
                    "All media streams are ready to be nominated (%i)",
                    pInstance->numberOfMediaStreams);

        for (i=0; i < pInstance->numberOfMediaStreams; ++i) {
            unsigned int numberOfComp = 0;
            pCheckList           = &pInstance->streamControllers[ i].checkList;
            pValidList           = &pInstance->streamControllers[ i].validList;
            pTriggeredChecksFifo = &pInstance->streamControllers[ i].triggeredChecksFifo;
            numberOfComp         = pCheckList->componentList.numberOfComponents;

            if (pCheckList->stopChecks) {
                //do nothing
            }else{
                uint32_t j;
                ICELIB_stopChecks(pInstance, pCheckList, pTriggeredChecksFifo);
                for (j=0; j<numberOfComp; j++) {
                    uint32_t componentId = pCheckList->componentList.componentIds[j];

                    pValidPair = pICELIB_pickValidPairForNomination(pInstance, pValidList, pCheckList, componentId);
                    if (pValidPair != NULL) {
                        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
                                   "Enqueueing valid pair...");

                        ICELIB_enqueueValidPair(pTriggeredChecksFifo,
                                                pCheckList,
                                                &pInstance->callbacks.callbackLog,
                                                pValidPair);
                    }else{
                        ICELIB_log1(&pInstance->callbacks.callbackLog,
                                    ICELIB_logWarning,
                                    "Could not pick valid pair for nomination (CompId: %i)",
                                    componentId);

                        ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logError,
                                   "Could not pick a valid pair!");
                    }
                }
            }
        }
    }else{
        ICELIB_updatingStates(pInstance);
    }
}


void ICELIB_removePairFromCheckList(ICELIB_CHECKLIST    *pCheckList,
                                    ICELIB_LIST_PAIR    *pPair,
                                    ICELIB_CALLBACK_LOG *pCallbackLog)
{
    if (pCheckList->numberOfPairs > 0) {
        ICELIB_changePairState(pPair, ICELIB_PAIR_REMOVED, pCallbackLog);
        pPair->pairPriority = 0;
        ICELIB_sortPairsCL(pCheckList);
        --pCheckList->numberOfPairs;
    }
}


//
 //----- Remove all Waiting and Frozen pairs in the check list with the specified
 //      component ID.
 //
void ICELIB_removeWaitingAndFrozenByComponentFromCheckList(ICELIB_CHECKLIST  *pCheckList,
                                                           uint32_t          componentId,
                                                           ICELIB_CALLBACK_LOG *pCallbackLog)
{
    unsigned int     i;
    ICELIB_LIST_PAIR *pPair;

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        pPair = &pCheckList->checkListPairs[ i];
        if ((pPair->pairState == ICELIB_PAIR_WAITING) ||
            (pPair->pairState == ICELIB_PAIR_FROZEN)) {
            if (pPair->pLocalCandidate->componentid == componentId) {
                ICELIB_removePairFromCheckList(pCheckList, pPair, pCallbackLog);
            }
        }
    }
}


//
//----- Remove all Waiting and Frozen pairs in the triggered check queue with
//      the specified component ID.
//
void ICELIB_removeWaitingAndFrozenByComponentFromTriggeredChecksFifo(ICELIB_CHECKLIST       *pCheckList,
                                                                     ICELIB_TRIGGERED_FIFO  *pTriggeredChecksFifo,
                                                                     ICELIB_CALLBACK_LOG    *pCallbackLog,
                                                                     uint32_t               componentId)
{
    ICELIB_LIST_PAIR               *pPair;
    ICELIB_TRIGGERED_FIFO_ITERATOR tfIterator;

    ICELIB_triggeredfifoIteratorConstructor(&tfIterator, pTriggeredChecksFifo);

    while ((pPair = pICELIB_triggeredfifoIteratorNext(pCheckList,
                                                      pCallbackLog,
                                                      &tfIterator)) != NULL) {
        if ((pPair->pairState == ICELIB_PAIR_WAITING) ||
            (pPair->pairState == ICELIB_PAIR_FROZEN)) {

            if (pPair->pLocalCandidate->componentid == componentId) {
                ICELIB_triggeredFifoRemove(pTriggeredChecksFifo, pPair);
            }
        }
    }
}


//
//----- Remove all Waiting and Frozen pairs in the check list and triggered
//      checks queue for the same components as the nominated pairs in the
//      valid list.
//
void ICELIB_removeWaitingAndFrozen(ICELIB_CHECKLIST       *pCheckList,
                                   ICELIB_VALIDLIST       *pValidList,
                                   ICELIB_TRIGGERED_FIFO  *pTriggeredChecksFifo,
                                   ICELIB_CALLBACK_LOG    *pCallbackLog)
{
    const ICELIB_VALIDLIST_ELEMENT  *pValidPair;
    ICELIB_VALIDLIST_ITERATOR       vlIterator;

    ICELIB_validListIteratorConstructor(&vlIterator, pValidList);

    while ((pValidPair = pICELIB_validListIteratorNext(&vlIterator)) != NULL) {
        if (pValidPair->nominatedPair) {
            ICELIB_removeWaitingAndFrozenByComponentFromTriggeredChecksFifo(pCheckList,
                                                                            pTriggeredChecksFifo,
                                                                            pCallbackLog,
                                                                            pValidPair->pLocalCandidate->componentid);
            ICELIB_removeWaitingAndFrozenByComponentFromCheckList(pCheckList,
                                                                  pValidPair->pLocalCandidate->componentid,
                                                                  pCallbackLog);
        }
    }
}


 //
 //----- If an In-Progress pair in the check list is for the same component as a
 //      nominated pair, the agent SHOULD cease retransmission for its check if
 //      its pair priority is lower than the lowest priority nominated pair for
 //      that component.
 //
void ICELIB_ceaseRetransmissions(ICELIB_CHECKLIST       *pCheckList,
                                 ICELIB_VALIDLIST       *pValidList,
                                 ICELIB_TRIGGERED_FIFO  *pTriggeredChecksFifo)
{
     (void)pCheckList;
     (void)pValidList;
     (void)pTriggeredChecksFifo;
     // TODO: Implement 8.1.2 Cease retransmissions
}


//
//----- ICE-19: 8.1.2  Update State for a single check list
//
void  ICELIB_updateCheckListStateConcluding(ICELIB_CHECKLIST       *pCheckList,
                                            ICELIB_VALIDLIST       *pValidList,
                                            ICELIB_TRIGGERED_FIFO  *pTriggeredChecksFifo,
                                            ICELIB_CALLBACK_LOG    *pCallbackLog)
{
    if (pCheckList->checkListState == ICELIB_CHECKLIST_RUNNING) {
        unsigned int i,j;
        unsigned int numberOfComp = pCheckList->componentList.numberOfComponents;
        unsigned int foundComp = 0;

        if (ICELIB_countNominatedPairsInValidList(pValidList) == 0) {
            return;
        }else{
            ICELIB_removeWaitingAndFrozen(pCheckList, pValidList, pTriggeredChecksFifo, pCallbackLog);
            ICELIB_ceaseRetransmissions(pCheckList, pValidList, pTriggeredChecksFifo);
        }
        //Once there is at least one nominated pair in the valid list for
        //every component of at least one media stream and the state of the
        //check list is Running:

        //*  The agent MUST change the state of processing for its check
        //   list for that media stream to Completed.
        for (i=0; i<numberOfComp; i++) {
            uint32_t componentId = pCheckList->componentList.componentIds[i];
            for (j=0; j<pValidList->pairs.numberOfElements; j++) {
                const ICE_CANDIDATE *pLocalCandidate = pValidList->pairs.elements[j].pLocalCandidate;
                if (pValidList->pairs.elements[j].nominatedPair &&
                    pLocalCandidate->componentid == componentId) {

                    foundComp++;
                    if (foundComp == numberOfComp) {
                        ICELIB_log(pCallbackLog,
                                   ICELIB_logDebug,
                                    "8.1.2 Media stream COMPLETED");
                        pCheckList->checkListState = ICELIB_CHECKLIST_COMPLETED;
                    }
                }
            }
        }
    }
}

uint32_t ICELIB_getWeightTimeMultiplier(ICELIB_INSTANCE *pInstance)
{
    uint32_t timeMS;

    timeMS = pInstance->tickCount * pInstance->iceConfiguration.tickIntervalMS;
    //Don't want to return a multiplier less than 1
    return (timeMS/ICELIB_TIME_MULTIPLIER_INCREASE_MS)+1;
}


//
//----- ICE-19: 8.1.2  Updating States
//
void ICELIB_updatingStates(ICELIB_INSTANCE *pInstance)
{
    unsigned int            i;
    bool                    complete = true;
    ICELIB_CHECKLIST        *pCheckList;
    ICELIB_VALIDLIST        *pValidList;
    ICELIB_TRIGGERED_FIFO   *pTriggeredChecksFifo;
    ICELIB_CALLBACK_LOG     *pCallbackLog;

    if (pInstance->iceState == ICELIB_COMPLETED || pInstance->iceState == ICELIB_FAILED)
        return;

    pCallbackLog = &pInstance->callbacks.callbackLog;

    for (i=0; i < pInstance->numberOfMediaStreams; ++i) {
        pCheckList           = &pInstance->streamControllers[ i].checkList;
        pValidList           = &pInstance->streamControllers[ i].validList;
        pTriggeredChecksFifo = &pInstance->streamControllers[ i].triggeredChecksFifo;

        ICELIB_updateCheckListStateConcluding(pCheckList, pValidList, pTriggeredChecksFifo, pCallbackLog);
        if (pCheckList->checkListState != ICELIB_CHECKLIST_COMPLETED) {
            complete = false;
        }
    }
    if (complete) {
        ICELIB_connectivityChecksComplete ConnectivityCheckComplete;
        for (i=0; i < pInstance->numberOfMediaStreams; ++i) {
            pCheckList           = &pInstance->streamControllers[i].checkList;
            ICELIB_removChecksFromCheckList(pCheckList);
        }
        ICELIB_log(pCallbackLog,
                   ICELIB_logInfo,
                   "*** COMPLETE!!! ****");
        pInstance->iceState = ICELIB_COMPLETED;
        //Store remote candidates for easy accsess by user
        ICELIB_storeRemoteCandidates(pInstance);
        ConnectivityCheckComplete = pInstance->callbacks.callbackComplete.pICELIB_connectivityChecksComplete;
        if (ConnectivityCheckComplete != NULL) {
            ConnectivityCheckComplete(pInstance->callbacks.callbackComplete.pConnectivityChecksCompleteUserData,
                                      pInstance->localIceMedia.mediaStream[0].userValue1,
                                      pInstance->iceControlling,
                                      false);
        }
    }else{
        //Not complete BUT have we failed?
        uint32_t timeMS = pInstance->tickCount * pInstance->iceConfiguration.tickIntervalMS;

        if (timeMS > ICELIB_FAIL_AFTER_MS /*5 sec*/) {
            ICELIB_connectivityChecksComplete ConnectivityCheckComplete;
            ConnectivityCheckComplete = pInstance->callbacks.callbackComplete.pICELIB_connectivityChecksComplete;
            pInstance->iceState = ICELIB_FAILED;
            ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
                       "ICE failed (Timeout)");
            ConnectivityCheckComplete(pInstance->callbacks.callbackComplete.pConnectivityChecksCompleteUserData,
                                      pInstance->localIceMedia.mediaStream[0].userValue1,
                                      pInstance->iceControlling,
                                      true);
        }
    }
}


void ICE_concludeFullIfComplete(ICELIB_INSTANCE *pInstance)
{
    //ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
    //            "Concluding as ICE Full...");
    //
    //----- ICE-19: 8.1.1  Nominating Pairs
    //
    if (pInstance->iceConfiguration.aggressiveNomination) {
        ICELIB_nominateAggressive(pInstance);
    } else {
        if (pInstance->iceControlling) {
            ICELIB_nominateRegularIfComplete(pInstance);
        }
    }
    //
    //----- ICE-19: 8.1.2  Updating States
    //
    //ICELIB_updatingStates(pInstance);
}

//
//----- ICE-19: 8.2.  Procedures for Lite Implementation
//
void ICE_concludingLite(ICELIB_INSTANCE *pInstance)
{
    ICELIB_log(&pInstance->callbacks.callbackLog, ICELIB_logInfo,
               "Concluding as ICE Lite...");
}


void ICELIB_concludeICEProcessingIfComplete(ICELIB_INSTANCE *pInstance)
{
    if (pInstance->iceConfiguration.iceLite) {
        ICE_concludingLite(pInstance);
    } else{
        ICE_concludeFullIfComplete(pInstance);
    }
}


void ICELIB_resetIceInstance(ICELIB_INSTANCE *pInstance)
{
    memset(pInstance, 0, sizeof(*pInstance));
}


void ICELIB_resetAllStreamControllers(ICELIB_INSTANCE *pInstance)
{
    unsigned int i;
    for (i=0; i < ICE_MAX_MEDIALINES; ++i) {
        ICELIB_resetStreamController(&pInstance->streamControllers[ i],
                                      pInstance->iceConfiguration.tickIntervalMS);
    }
}

uint32_t ICELIB_getCandidateTypeWeight(ICE_CANDIDATE_TYPE type)
{
    switch (type) {
    case ICE_CAND_TYPE_NONE:
        return 0;
    case ICE_CAND_TYPE_HOST:
        return ICELIB_HOST_WEIGHT;
    case ICE_CAND_TYPE_SRFLX:
        return ICELIB_SRFLX_WEIGHT;
    case ICE_CAND_TYPE_RELAY:
        return ICELIB_RELAY_WEIGHT;
    case ICE_CAND_TYPE_PRFLX:
        return ICELIB_PRFLX_WEIGHT;
    }
    return 0;
}

uint32_t ICELIB_calculateReadyWeight(ICE_CANDIDATE_TYPE localType,
                                     ICE_CANDIDATE_TYPE remoteType,
                                     uint32_t timeMultiplier)
{
    uint32_t weight= 0;

    weight+= ICELIB_getCandidateTypeWeight(localType);
    weight+= (2*ICELIB_getCandidateTypeWeight(remoteType));

    weight*= timeMultiplier;
    return weight;
}

static uint32_t ICELIB_getBestWeight(ICELIB_VALIDLIST *pValidList,
                                     uint32_t componentId,
                                     uint32_t timeMultiplier)
{
    uint32_t i;
    uint32_t weight = 0;

    for (i=0; i < pValidList->pairs.numberOfElements; i++) {

        if (componentId == pValidList->pairs.elements[i].pLocalCandidate->componentid) {
            uint32_t tmpWeight = ICELIB_calculateReadyWeight(pValidList->pairs.elements[i].pLocalCandidate->type,
                                                             pValidList->pairs.elements[i].pRemoteCandidate->type,
                                                             timeMultiplier);
            if (tmpWeight > weight) {
                weight = tmpWeight;
            }
        }
    }

    return weight;
}

static uint32_t ICELIB_getBestScore(ICELIB_VALIDLIST *pValidList,
                                    uint32_t componentId)
{
    return ICELIB_getBestWeight(pValidList, componentId, 1);
}


void ICELIB_updateValidPairReadyToNominateWeightingMediaStream(ICELIB_CHECKLIST *pCheckList,
                                                               ICELIB_VALIDLIST *pValidList,
                                                               uint32_t timeMultiplier)
{
    uint32_t weight = 0;
    uint32_t numberOfComp = pCheckList->componentList.numberOfComponents;
    uint32_t i;

    for (i=0; i<numberOfComp; i++) {
        //Get the one with highest path score in the list.
        if (weight == 0) {
            weight = ICELIB_getBestWeight(pValidList,
                                          pCheckList->componentList.componentIds[i],
                                          timeMultiplier);
            //Special case with one component only
            if (numberOfComp== 1) {
                pValidList->readyToNominateWeighting = weight;
                pValidList->nominatedPathScore = ICELIB_getBestScore(pValidList,
                                                                     pCheckList->componentList.componentIds[i]);
                return;
            }

        }else{
            uint32_t tmpWeight = ICELIB_getBestWeight(pValidList,
                                                      pCheckList->componentList.componentIds[i],
                                                      timeMultiplier);
            if (tmpWeight == weight) {
                //We have same for all components
                pValidList->readyToNominateWeighting = weight;
                pValidList->nominatedPathScore = ICELIB_getBestScore(pValidList,
                                                                     pCheckList->componentList.componentIds[i]);

            } else {
                pValidList->readyToNominateWeighting = 0;

            }
        }
    }
}

void ICELIB_updateValidPairReadyToNominateWeighting(ICELIB_INSTANCE *pInstance)
{
    unsigned int i;
    for (i=0; i < pInstance->numberOfMediaStreams; i++) {
        ICELIB_updateValidPairReadyToNominateWeightingMediaStream(&pInstance->streamControllers[i].checkList,
                                                                  &pInstance->streamControllers[i].validList,
                                                                  ICELIB_getWeightTimeMultiplier(pInstance));
    }

}

//-----------------------------------------------------------------------------
//
//===== ICELIB API Functions
//

void ICELIB_Constructor(ICELIB_INSTANCE            *pInstance,
                        const ICELIB_CONFIGURATION *pConfiguration)
{
    ICELIB_resetIceInstance(pInstance);

    pInstance->iceState         = ICELIB_IDLE;
    pInstance->iceConfiguration = *pConfiguration;

    ICELIB_resetAllStreamControllers(pInstance);

    pInstance->iceConfiguration.maxCheckListPairs =
        min(pInstance->iceConfiguration.maxCheckListPairs, ICELIB_MAX_PAIRS);

    ICELIB_callbackConstructor(&pInstance->callbacks, pInstance);
}


void ICELIB_Destructor (ICELIB_INSTANCE *pInstance)
{
    ICELIB_resetIceInstance(pInstance);
}

void ICELIB_PasswordUpdate(ICELIB_INSTANCE *pInstance)
{
    unsigned int i;
    ICELIB_passwordUpdate  PasswordUpdate;


    PasswordUpdate  = pInstance->callbacks.callbackPasswordUpdate.pICELIB_passwordUpdate;
    if (PasswordUpdate != NULL) {
        for (i=0;i<pInstance->numberOfMediaStreams;i++) {

            PasswordUpdate(pInstance->callbacks.callbackPasswordUpdate.pUserDataPasswordUpdate,
                           pInstance->localIceMedia.mediaStream[i].userValue1,
                           pInstance->localIceMedia.mediaStream[i].userValue2,
                           &pInstance->localIceMedia.mediaStream[i].passwd[0]);

        }

    }

}


void ICELIB_EliminateRedundantCandidates(ICELIB_INSTANCE *pInstance)
{
    uint32_t i;
    ICE_MEDIA_STREAM *iceMediaStream;

    for (i=0; i < pInstance->localIceMedia.numberOfICEMediaLines;i++) {
        iceMediaStream = &pInstance->localIceMedia.mediaStream[i];

        iceMediaStream->numberOfCandidates =
            ICELIB_eliminateRedundantCandidates(iceMediaStream->candidate);
    }
}


bool ICELIB_Start(ICELIB_INSTANCE *pInstance, bool controlling)
{

    if (controlling)
        ICELIB_log(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug, "ICELIB_Start with controlling TRUE");

    else
        ICELIB_log(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug, "ICELIB_Start with controlling FALSE");


    if (!ICELIB_verifyICESupport(pInstance, &pInstance->remoteIceMedia)) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug, "Remote Media mangling detected");
        return false;
    }

    pInstance->iceSupportVerified = true;

    ICELIB_EliminateRedundantCandidates(pInstance);

    if (controlling) {
        pInstance->iceControlling = true;
        pInstance->iceControlled  = false;

    }else{
        pInstance->iceControlling = false;
        pInstance->iceControlled  = true;
    }

    ICELIB_makeAllCheckLists(pInstance);

    ICELIB_logVaString(&pInstance->callbacks.callbackLog,
                       ICELIB_logInfo,
                       "Start ICE check list processing ===== "
                       "Media streams: %d == Controlling: %d =====\n",
                       pInstance->numberOfMediaStreams,
                       pInstance->iceControlling);

    //Tell UA what passwords we are expecting in
    //connectivity checks against us.
    ICELIB_PasswordUpdate(pInstance);

    pInstance->tickCount = 0;
    pInstance->keepAliveTickCount = 0;
    pInstance->tieBreaker = ICELIB_makeTieBreaker();
    //ICELIB_startAllStoppingTimers(pInstance);
    pInstance->iceState   = ICELIB_RUNNING;

    return true;
}


void ICELIB_Stop(ICELIB_INSTANCE *pInstance)
{
    (void)pInstance;
    //unsigned int i;
    //- TODO
    //for (i=0;i<pInstance->numberOfMediaStreams;i++) {
    //    ICELIB_timerStop(&pInstance->streamControllers[i].stoppingTimer);
    //}

}

bool ICELIB_isRestart(ICELIB_INSTANCE *pInstance, unsigned int mediaIdx,
                      const char *ufrag, const char *passwd)
{

    if (pInstance->iceConfiguration.logLevel == ICELIB_logDebug) {
        printf("<ICELIB_isRestart> (%i,%i) ['%s' '%s'] ['%s' '%s']\n",
               mediaIdx,
               pInstance->numberOfMediaStreams,ufrag, passwd,
               pInstance->remoteIceMedia.mediaStream[mediaIdx].ufrag,
               pInstance->remoteIceMedia.mediaStream[mediaIdx].passwd);

        if (mediaIdx>=pInstance->numberOfMediaStreams) {
            printf("<ICELIB> Checking invalid medialine\n");
            return false;
        }
    }

    if (ufrag == NULL || passwd == NULL) {
        return false;
    }

    if (0 != strncmp(pInstance->remoteIceMedia.mediaStream[mediaIdx].ufrag,
                     ufrag,
                     ICE_MAX_UFRAG_LENGTH)) {
        return true;
    }

    if (0 != strncmp(pInstance->remoteIceMedia.mediaStream[mediaIdx].passwd,
                     passwd,
                     ICE_MAX_PASSWD_LENGTH)) {
        return true;
    }
    return false;

}


void ICELIB_doKeepAlive(ICELIB_INSTANCE *pInstance)
{
    uint32_t timeS;

    pInstance->keepAliveTickCount++;

    timeS = (pInstance->keepAliveTickCount * pInstance->iceConfiguration.tickIntervalMS)/1000;

    if (timeS>pInstance->iceConfiguration.keepAliveIntervalS) {
        ICELIB_sendKeepAlive SendKeepAlive;
        pInstance->keepAliveTickCount = 0;
        SendKeepAlive = pInstance->callbacks.callbackKeepAlive.pICELIB_sendKeepAlive;

        if (SendKeepAlive != NULL) {
            uint32_t    mediaIdx;
            for (mediaIdx=0; mediaIdx < pInstance->localIceMedia.numberOfICEMediaLines;mediaIdx++) {
                ICELIB_log(&pInstance->callbacks.callbackLog,
                           ICELIB_logDebug, "Sending KeepAlive");
                SendKeepAlive(pInstance->callbacks.callbackKeepAlive.pUserDataKeepAlive,
                              pInstance->localIceMedia.mediaStream[mediaIdx].userValue1,
                              pInstance->localIceMedia.mediaStream[mediaIdx].userValue2,
                              mediaIdx);
            }

        }else{
            ICELIB_log(&pInstance->callbacks.callbackLog,
                       ICELIB_logError, "Should send keep alive, but no callback is set");
        }

    }

}

void ICELIB_ReStart(ICELIB_INSTANCE *pInstance)
{
    //- TODO
    pInstance->tickCount = 0;
    pInstance->keepAliveTickCount = 0;
    pInstance->tieBreaker = ICELIB_makeTieBreaker();
    ICELIB_resetAllStreamControllers(pInstance);
}


void ICELIB_Tick(ICELIB_INSTANCE *pInstance)
{
    if (pInstance->iceSupportVerified) {
        pInstance->tickCount++;
        ICELIB_tickStreamController(pInstance);
        if (pInstance->iceState == ICELIB_RUNNING) {
            ICELIB_updateValidPairReadyToNominateWeighting(pInstance);
            ICELIB_concludeICEProcessingIfComplete(pInstance);
            ICELIB_updatingStates(pInstance);
        }else if (pInstance->iceState == ICELIB_COMPLETED) {
            ICELIB_doKeepAlive(pInstance);

        }
    }
    //- TODO    ICELIB_tickAllStoppingTimers(ICELIB_INSTANCE *pInstance);
}


void ICELIB_setCallbackOutgoingBindingRequest(ICELIB_INSTANCE               *pInstance,
                                              ICELIB_outgoingBindingRequest  pICELIB_sendBindingRequest,
                                              void                          *pBindingRequestUserData)
{
    pInstance->callbacks.callbackRequest.pICELIB_sendBindingRequest =
        pICELIB_sendBindingRequest;
    pInstance->callbacks.callbackRequest.pBindingRequestUserData    =
        pBindingRequestUserData;
    pInstance->callbacks.callbackRequest.pInstance = pInstance;
}


void ICELIB_setCallbackOutgoingBindingResponse(ICELIB_INSTANCE                *pInstance,
                                               ICELIB_outgoingBindingResponse  pICELIB_sendBindingResponse,
                                               void                           *pBindingResponseUserData)
{
    pInstance->callbacks.callbackResponse.pICELIB_sendBindingResponse =
        pICELIB_sendBindingResponse;
    pInstance->callbacks.callbackResponse.pBindingResponseUserData    =
        pBindingResponseUserData;
    pInstance->callbacks.callbackResponse.pInstance = pInstance;
}

void ICELIB_setCallbackOutgoingCancelRequest(ICELIB_INSTANCE                *pInstance,
                                             ICELIB_outgoingCancelRequest   pICELIB_sendBindingCancelRequest,
                                             void                           *pCancelRequesUserData)
{

    pInstance->callbacks.callbackCancelRequest.pICELIB_sendCancelRequest =
        pICELIB_sendBindingCancelRequest;

    pInstance->callbacks.callbackCancelRequest.pCancelRequestUserData    =
        pCancelRequesUserData;

    pInstance->callbacks.callbackCancelRequest.pInstance = pInstance;
}



void ICELIB_setCallbackConnecitivityChecksComplete(ICELIB_INSTANCE                    *pInstance,
                                                   ICELIB_connectivityChecksComplete  pICELIB_connectivityChecksComplete,
                                                   void                               *userData)
{
    pInstance->callbacks.callbackComplete.pICELIB_connectivityChecksComplete =
        pICELIB_connectivityChecksComplete;
    pInstance->callbacks.callbackComplete.pConnectivityChecksCompleteUserData    =
        userData;
    pInstance->callbacks.callbackComplete.pInstance = pInstance;
}

void ICELIB_setCallbackKeepAlive(ICELIB_INSTANCE      *pInstance,
                                 ICELIB_sendKeepAlive  pICELIB_sendKeepAlive,
                                 void                 *pUserDataKeepAlive)
{
    pInstance->callbacks.callbackKeepAlive.pICELIB_sendKeepAlive =
        pICELIB_sendKeepAlive;
    pInstance->callbacks.callbackKeepAlive.pUserDataKeepAlive    =
        pUserDataKeepAlive;
    pInstance->callbacks.callbackKeepAlive.pInstance = pInstance;
}


void ICELIB_setCallbackPasswordUpdate(ICELIB_INSTANCE                *pInstance,
                                      ICELIB_passwordUpdate           pICELIB_passwordUpdate,
                                      void                           *pUserDataPasswordUpdate) {
    pInstance->callbacks.callbackPasswordUpdate.pICELIB_passwordUpdate =
        pICELIB_passwordUpdate;
    pInstance->callbacks.callbackPasswordUpdate.pUserDataPasswordUpdate    =
        pUserDataPasswordUpdate;
    pInstance->callbacks.callbackPasswordUpdate.pInstance = pInstance;
}

void ICELIB_setCallbackLog(ICELIB_INSTANCE                *pInstance,
                           ICELIB_logCallback              pICELIB_logCallback,
                           void                           *pLogUserData,
                           ICELIB_logLevel                 logLevel)
{
    pInstance->callbacks.callbackLog.pICELIB_logCallback = pICELIB_logCallback;
    pInstance->callbacks.callbackLog.pLogUserData        = pLogUserData;
    pInstance->callbacks.callbackLog.pInstance           = pInstance;
    pInstance->iceConfiguration.logLevel                 = logLevel;
}

//
//----- Create a 'foundation' string for a candidate pair.
//      The length of the string, "maxlength", includes the terminating
//      null character (i.e. maxlength=5 yields 4 characters and a '\0').
//
char *ICELIB_getPairFoundation(char                    *dst,
                               int                      maxlength,
                               const ICELIB_LIST_PAIR  *pPair)
{
    ICELIB_strncpy(dst, pPair->pRemoteCandidate->foundation, maxlength);
    ICELIB_strncat(dst, pPair->pLocalCandidate->foundation, maxlength);

    return dst;
}


ICE_REMOTE_CANDIDATES const *ICELIB_getActiveRemoteCandidates(ICELIB_INSTANCE *pInstance,
                                                              int mediaLineId)
{


    if (pInstance->streamControllers[mediaLineId].checkList.checkListState == ICELIB_CHECKLIST_COMPLETED) {
        return &pInstance->streamControllers[mediaLineId].remoteCandidates;
    }else{
        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug, "No Remote Candidates available. Checklist not Complete\n");
        return NULL;
    }

}


ICE_CANDIDATE const *ICELIB_getActiveCandidate(ICELIB_INSTANCE *pInstance,
                                               int mediaLineId,
                                               uint32_t componentId)
{

    ICELIB_VALIDLIST         *pValidList;
    ICE_MEDIA_STREAM         *mediaStream;
    uint32_t i;

    pValidList = &pInstance->streamControllers[mediaLineId].validList;

    for (i=0;i<pValidList->pairs.numberOfElements;i++) {
        const ICE_CANDIDATE    *pLocalCandidate = pValidList->pairs.elements[i].pLocalCandidate;
        if (pValidList->pairs.elements[i].nominatedPair &&
            pLocalCandidate->componentid == componentId) {
            return pLocalCandidate;


        }
    }
    //We should look up the default candididate for the media stream and use that
    mediaStream =  &pInstance->localIceMedia.mediaStream[mediaLineId];
    for (i=0;i<mediaStream->numberOfCandidates; i++) {
        const ICE_CANDIDATE *defaultCandidate = &mediaStream->candidate[i];
        if (defaultCandidate->type == mediaStream->defaultCandType &&
           defaultCandidate->componentid == componentId) {
            return defaultCandidate;
        }


    }


    return NULL;

}

//
//----- Create a 'username credential' string for a check list.
//      The length of the string, "maxlength", includes the terminating
//      null character (i.e. maxlength=5 yields 4 characters and a '\0').
//
char *ICELIB_getCheckListRemoteUsernamePair(char                    *dst,
                                            int                      maxlength,
                                            const ICELIB_CHECKLIST  *pCheckList)
{
    ICELIB_makeUsernamePair(dst,
                            maxlength,
                            pCheckList->ufragRemote,
                            pCheckList->ufragLocal);
    return dst;
}

char *ICELIB_getCheckListLocalUsernamePair(char                    *dst,
                                           int                      maxlength,
                                           const ICELIB_CHECKLIST  *pCheckList)
{
    ICELIB_makeUsernamePair(dst,
                            maxlength,
                            pCheckList->ufragLocal,
                            pCheckList->ufragRemote);

    return dst;
}


const char *ICELIB_getCheckListLocalPasswd(const ICELIB_CHECKLIST *pCheckList)
{
    return pCheckList->passwdLocal;
}

const char *ICELIB_getCheckListRemotePasswd(const ICELIB_CHECKLIST *pCheckList)
{
    return pCheckList->passwdRemote;
}


bool ICELIB_isControlling(ICELIB_INSTANCE *pInstance)
{
    return pInstance->iceControlling;
}



void ICELIB_netAddrDumpLog(const ICELIB_CALLBACK_LOG *pCallbackLog,
                            ICELIB_logLevel            logLevel,
                            const struct sockaddr            *netAddr)
{
    char addr[SOCKADDR_MAX_STRLEN];

    if (sockaddr_toString((struct sockaddr *)netAddr, addr , sizeof(addr), true)) {
        ICELIB_logString(pCallbackLog, logLevel, addr);
    } else {
        ICELIB_logString(pCallbackLog, logLevel, "invalid");
    }
}


void ICELIB_transactionIdDumpLog(const ICELIB_CALLBACK_LOG *pCallbackLog,
                                 ICELIB_logLevel            logLevel,
                                 StunMsgId                  tId)
{
    unsigned int i;
    char         str[ 100];

    strcpy(str, "0x");

    for (i=0; i < STUN_MSG_ID_SIZE; ++i) {
        sprintf(str + 2 + (i * 2), "%02x", tId.octet[ i]);
    }

    ICELIB_logString(pCallbackLog, logLevel, str);
}


void ICELIB_candidateDumpLog(const ICELIB_CALLBACK_LOG *pCallbackLog,
                             ICELIB_logLevel            logLevel,
                             const ICE_CANDIDATE       *candidate)
{
    ICELIB_logVaString(pCallbackLog, logLevel, "   Fnd: '%s' ", candidate->foundation);
    ICELIB_logVaString(pCallbackLog, logLevel, "Comp: %i ", candidate->componentid);
    ICELIB_logVaString(pCallbackLog, logLevel, "Pri: %u ", candidate->priority);
    ICELIB_logVaString(pCallbackLog, logLevel, "Addr: ");
    ICELIB_netAddrDumpLog(pCallbackLog, logLevel, (struct sockaddr *)&candidate->connectionAddr);
    ICELIB_logVaString(pCallbackLog, logLevel, " Type: '%s' ", ICELIBTYPES_ICE_CANDIDATE_TYPE_toString(candidate->type));
    ICELIB_logVaString(pCallbackLog, logLevel, " UVal1: %u ", candidate->userValue1);
    ICELIB_logVaString(pCallbackLog, logLevel, " UVal2: %u\n", candidate->userValue2);
}


void ICELIB_componentIdsDumpLog(const ICELIB_CALLBACK_LOG  *pCallbackLog,
                                ICELIB_logLevel            logLevel,
                                const ICELIB_COMPONENTLIST *pComponentList)
{
    unsigned int i;

    ICELIB_logVaString(pCallbackLog, logLevel, "Number of components: %d - ", pComponentList->numberOfComponents);
    ICELIB_logVaString(pCallbackLog, logLevel, "[ ");
    for (i=0; i < pComponentList->numberOfComponents; ++i) {
        ICELIB_logVaString(pCallbackLog, logLevel, "%d, ", pComponentList->componentIds[ i]);
    }
    ICELIB_logVaString(pCallbackLog, logLevel, "]");
}


void ICELIB_pairDumpLog(const ICELIB_CALLBACK_LOG  *pCallbackLog,
                        ICELIB_logLevel             logLevel,
                        const ICELIB_LIST_PAIR     *pPair)
{
    char foundation[ ICE_MAX_FOUNDATION_PAIR_LENGTH];
    unsigned int i;

    if (pPair == NULL) {
        ICELIB_log(pCallbackLog, ICELIB_logError, "pPair == NULL");
        return;
    }

    if (ICELIB_prunePairsIsClear(pPair)) {
        ICELIB_logVaString(pCallbackLog, logLevel, "[empty]\n");
    } else {
        //unsigned int i;
        ICELIB_logVaString(pCallbackLog, logLevel, "Pair state: '%s'\n",
                           ICELIB_toString_CheckListPairState(pPair ->pairState));

        ICELIB_logVaString(pCallbackLog, logLevel, "Default=%d, ", pPair->defaultPair);
        ICELIB_logVaString(pCallbackLog, logLevel, "Use-Cand=%d, ", pPair->useCandidate);
        ICELIB_logVaString(pCallbackLog, logLevel, "Triggered-Use-Cand=%d, ", pPair->triggeredUseCandidate);
        ICELIB_logVaString(pCallbackLog, logLevel, "Nominated=%d, ", pPair->nominatedPair);
        ICELIB_logVaString(pCallbackLog, logLevel, "Id=%u ", pPair->pairId);
        ICELIB_logVaString(pCallbackLog, logLevel, "refersTo=%u\n", pPair->refersToPairId);

#if defined(__x86_64__)
        ICELIB_logVaString(pCallbackLog, logLevel, "Pair priority  : 0x%016lx\n", pPair->pairPriority);
#else
        ICELIB_logVaString(pCallbackLog, logLevel, "Pair priority  : 0x%016llx\n", pPair->pairPriority);
#endif // __X86_64__
        ICELIB_logVaString(pCallbackLog, logLevel, "Pair foundation: '%s'\n",
                           ICELIB_getPairFoundation(foundation,
                                                    ICE_MAX_FOUNDATION_PAIR_LENGTH,
                                                    pPair));

        ICELIB_logVaString(pCallbackLog, logLevel, "Transaction ID : ");
        for (i=0;i<pPair->numberOfTransactionIds;i++) {
            ICELIB_transactionIdDumpLog(pCallbackLog, logLevel, pPair->transactionIdTable[i]);
            ICELIB_logVaString(pCallbackLog, logLevel, ", ");
        }
        //ICELIB_transactionIdDumpLog(pCallbackLog, logLevel, pPair->transactionId);
        ICELIB_logVaString(pCallbackLog, logLevel, "\n");

        ICELIB_logVaString(pCallbackLog, logLevel, "<Local candidate>\n");
        ICELIB_candidateDumpLog(pCallbackLog, logLevel, pPair->pLocalCandidate);

        ICELIB_logVaString(pCallbackLog, logLevel, "<Remote candidate>\n");
        ICELIB_candidateDumpLog(pCallbackLog, logLevel, pPair->pRemoteCandidate);
    }
}


void ICELIB_checkListDumpLog(const ICELIB_CALLBACK_LOG *pCallbackLog,
                             ICELIB_logLevel            logLevel,
                             const ICELIB_CHECKLIST    *pCheckList)
{
    unsigned int i;
    char         ufrag[ ICE_MAX_UFRAG_PAIR_LENGTH];

    ICELIB_logVaString(pCallbackLog, logLevel, "Check list uname : '%s'\n",
                       ICELIB_getCheckListRemoteUsernamePair(ufrag,
                                                             ICE_MAX_UFRAG_PAIR_LENGTH,
                                                             pCheckList));
    ICELIB_logVaString(pCallbackLog, logLevel,"Check list Local passwd: '%s'\n",
                       ICELIB_getCheckListLocalPasswd(pCheckList));
    ICELIB_logVaString(pCallbackLog, logLevel,"Check list Remote passwd: '%s'\n",
                       ICELIB_getCheckListRemotePasswd(pCheckList));

    ICELIB_logVaString(pCallbackLog, logLevel,"Check list state : '%s'\n",
                       ICELIB_toString_CheckListState(pCheckList->checkListState));

    ICELIB_logVaString(pCallbackLog, logLevel, "List of component IDs: ");
    ICELIB_componentIdsDumpLog(pCallbackLog, logLevel, &pCheckList->componentList);
    ICELIB_logVaString(pCallbackLog, logLevel, "\n");

    ICELIB_logVaString(pCallbackLog, logLevel,
                       "Number of pairs in list: %u\n",
                       pCheckList->numberOfPairs);

    for (i=0; i < pCheckList->numberOfPairs; ++i) {
        ICELIB_logVaString(pCallbackLog, logLevel,
                           "Pair[ %u] ====================================================\n", i);
        ICELIB_pairDumpLog(pCallbackLog, logLevel, &pCheckList->checkListPairs[ i]);
    }
}


void ICELIB_checkListDumpAllLog(const ICELIB_CALLBACK_LOG *pCallbackLog,
                                ICELIB_logLevel            logLevel,
                                const ICELIB_INSTANCE     *pInstance)
{
    unsigned int i;

    ICELIB_logVaString(pCallbackLog, logLevel,
                       "\n\n--- Dump all check lists ------------------------------------\n");
    ICELIB_logVaString(pCallbackLog, logLevel,
                       "    Number of paired media streams: %d\n",
                       pInstance->numberOfMediaStreams);

    for (i = 0; i < pInstance->numberOfMediaStreams; ++i) {
        ICELIB_logVaString(pCallbackLog, logLevel,
                           "--- Dump check list[%u] ------------------------------------\n\n", i);
        ICELIB_checkListDumpLog(pCallbackLog, logLevel, &pInstance->streamControllers[ i].checkList);
        ICELIB_logVaString(pCallbackLog, logLevel, "\n\n");
    }
}


void ICELIB_validListDumpLog(const ICELIB_CALLBACK_LOG *pCallbackLog,
                             ICELIB_logLevel            logLevel,
                             ICELIB_VALIDLIST          *pValidList)
{
    int i = 0;
    ICELIB_VALIDLIST_ITERATOR vlIterator;
    ICELIB_VALIDLIST_ELEMENT  *pValidPair;

    ICELIB_validListIteratorConstructor(&vlIterator, pValidList);

    while ((pValidPair = pICELIB_validListIteratorNext(&vlIterator)) != NULL) {
        ICELIB_logVaString(pCallbackLog, logLevel,
                            "Valid Pair[ %u] ========================================\n", i++);
        ICELIB_pairDumpLog(pCallbackLog, logLevel, pValidPair);
    }
}


void ICELIB_netAddrDump(const struct sockaddr *netAddr)
{
    ICELIB_netAddrDumpLog(NULL, ICELIB_logDebug, netAddr);
}


void ICELIB_transactionIdDump(StunMsgId tId)
{
    ICELIB_transactionIdDumpLog(NULL, ICELIB_logDebug, tId);
}


void ICELIB_candidateDump(const ICE_CANDIDATE *candidate)
{
    ICELIB_candidateDumpLog(NULL, ICELIB_logDebug, candidate);
}


void ICELIB_componentIdsDump(const ICELIB_COMPONENTLIST *pComponentList)
{
    ICELIB_componentIdsDumpLog(NULL, ICELIB_logDebug, pComponentList);
}


void ICELIB_checkListDumpPair(const ICELIB_LIST_PAIR *pPair)
{
    ICELIB_pairDumpLog(NULL, ICELIB_logDebug, pPair);
}


void ICELIB_checkListDump(const ICELIB_CHECKLIST *pCheckList)
{
    ICELIB_checkListDumpLog(NULL, ICELIB_logDebug, pCheckList);
}


void ICELIB_checkListDumpAll(const ICELIB_INSTANCE *pInstance)
{
    ICELIB_checkListDumpAllLog(NULL, ICELIB_logDebug, pInstance);
}


void ICELIB_validListDump(ICELIB_VALIDLIST *pValidList)
{
    ICELIB_validListDumpLog(NULL, ICELIB_logDebug, pValidList);
}

int32_t ICELIB_getNumberOfLocalICEMediaLines(const ICELIB_INSTANCE *pInstance)
{
    return pInstance->localIceMedia.numberOfICEMediaLines;
}

int32_t ICELIB_getNumberOfRemoteICEMediaLines(const ICELIB_INSTANCE *pInstance)
{
    return pInstance->remoteIceMedia.numberOfICEMediaLines;
}


int32_t ICELIB_getNumberOfLocalCandidates(const ICELIB_INSTANCE *pInstance, uint32_t idx)
{
    if (idx > pInstance->localIceMedia.numberOfICEMediaLines) {
        return 0;
    }

    return pInstance->localIceMedia.mediaStream[idx].numberOfCandidates;

}


int32_t ICELIB_getNumberOfRemoteCandidates(const ICELIB_INSTANCE *pInstance, uint32_t idx)
{
    if (idx > pInstance->remoteIceMedia.numberOfICEMediaLines) {
        return 0;
    }

    return pInstance->remoteIceMedia.mediaStream[idx].numberOfCandidates;



}


uint32_t ICELIB_getLocalComponentId(const ICELIB_INSTANCE *pInstance, uint32_t mediaIdx, uint32_t candIdx)
{
    if (mediaIdx > pInstance->localIceMedia.numberOfICEMediaLines) {
        return 0;
    }

    if (candIdx > pInstance->localIceMedia.mediaStream[mediaIdx].numberOfCandidates) {
        return 0;
    }

    return pInstance->localIceMedia.mediaStream[mediaIdx].candidate[candIdx].componentid;
}


uint32_t ICELIB_getRemoteComponentId(const ICELIB_INSTANCE *pInstance, uint32_t mediaIdx, uint32_t candIdx)
{
    if (mediaIdx > pInstance->remoteIceMedia.numberOfICEMediaLines) {
        return 0;
    }

    if (candIdx > pInstance->remoteIceMedia.mediaStream[mediaIdx].numberOfCandidates) {
        return 0;
    }

    return pInstance->remoteIceMedia.mediaStream[mediaIdx].candidate[candIdx].componentid;
}


struct sockaddr const *ICELIB_getLocalConnectionAddr(const ICELIB_INSTANCE *pInstance, uint32_t mediaIdx, uint32_t candIdx)
{
    if (mediaIdx > pInstance->localIceMedia.numberOfICEMediaLines) {
        return NULL;
    }

    if (candIdx > pInstance->localIceMedia.mediaStream[mediaIdx].numberOfCandidates) {
        return NULL;
    }

    return (struct sockaddr *)&pInstance->localIceMedia.mediaStream[mediaIdx].candidate[candIdx].connectionAddr;
}

struct sockaddr const *ICELIB_getRemoteConnectionAddr(const ICELIB_INSTANCE *pInstance, uint32_t mediaIdx, uint32_t candIdx)
{
    if (mediaIdx > pInstance->localIceMedia.numberOfICEMediaLines) {
        return NULL;
    }

    if (candIdx > pInstance->remoteIceMedia.mediaStream[mediaIdx].numberOfCandidates) {
        return NULL;
    }

    return (struct sockaddr *)&pInstance->remoteIceMedia.mediaStream[mediaIdx].candidate[candIdx].connectionAddr;
}

ICE_CANDIDATE_TYPE ICELIB_getLocalCandidateType(const ICELIB_INSTANCE *pInstance, uint32_t mediaIdx, uint32_t candIdx)
{
    if (mediaIdx > pInstance->localIceMedia.numberOfICEMediaLines) {
        return ICE_CAND_TYPE_NONE;
    }

    if (candIdx > pInstance->localIceMedia.mediaStream[mediaIdx].numberOfCandidates) {
        return ICE_CAND_TYPE_NONE;
    }

    return pInstance->localIceMedia.mediaStream[mediaIdx].candidate[candIdx].type;


}

ICE_CANDIDATE_TYPE ICELIB_getRemoteCandidateType(const ICELIB_INSTANCE *pInstance, uint32_t mediaIdx, uint32_t candIdx)
{
    if (mediaIdx > pInstance->localIceMedia.numberOfICEMediaLines) {
        return ICE_CAND_TYPE_NONE;
    }

    if (candIdx > pInstance->remoteIceMedia.mediaStream[mediaIdx].numberOfCandidates) {
        return ICE_CAND_TYPE_NONE;
    }

    return pInstance->remoteIceMedia.mediaStream[mediaIdx].candidate[candIdx].type;


}


static int ICELIB_candidateSort(const void *x, const void *y) {
    ICE_CANDIDATE *a = (ICE_CANDIDATE*)x;
    ICE_CANDIDATE *b = (ICE_CANDIDATE*)y;

    return (b->priority - a->priority);
}


int32_t ICELIB_addLocalCandidate(ICELIB_INSTANCE *pInstance,
                                 uint32_t mediaIdx,
                                 uint32_t componentId,
                                 struct sockaddr *connectionAddr,
                                 struct sockaddr *relAddr,
                                 ICE_CANDIDATE_TYPE candType)
{
    ICE_MEDIA_STREAM *mediaStream;
    ICE_CANDIDATE *cand;
    uint32_t priority = ICELIB_calculatePriority(candType, componentId);


    if (connectionAddr == NULL) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug, "Failed to add candidate. Connection ADDR is NULL\n");
        return -1;

    }

    mediaStream = &(pInstance->localIceMedia.mediaStream[mediaIdx]);

    if (mediaStream->numberOfCandidates >= ICE_MAX_CANDIDATES) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug, "Failed to add candidate. MAX number of candidates reached\n");
        return -1;
    }



    cand = &mediaStream->candidate[mediaStream->numberOfCandidates];


    sockaddr_copy((struct sockaddr *)&cand->connectionAddr, 
                  (struct sockaddr *)connectionAddr);
    cand->type = candType;
    cand->componentid = componentId;

    ICELIB_createFoundation(cand->foundation,
                             candType,
                             ICELIB_FOUNDATION_LENGTH);

    cand->priority = priority;
    if (relAddr != NULL) {
        sockaddr_copy((struct sockaddr *)&cand->relAddr, 
                      (struct sockaddr *)relAddr);
    }

    //Once the candidates are paired and the checlist is created
    //the only way to send back this information in the callback
    //is to store it in the candidates
    cand->userValue1 = mediaStream->userValue1;
    cand->userValue2 = mediaStream->userValue2;

    mediaStream->numberOfCandidates++;

    qsort(mediaStream->candidate,
          mediaStream->numberOfCandidates,
          sizeof(ICE_CANDIDATE),
          ICELIB_candidateSort);

    return 1;

}

int32_t ICELIB_addRemoteCandidate(ICELIB_INSTANCE *pInstance,
                                  uint32_t mediaIdx,
                                  char* foundation,
                                  uint32_t foundationLen,
                                  uint32_t componentId,
                                  uint32_t priority,
                                  char *connectionAddr,
                                  uint16_t port,
                                  ICE_CANDIDATE_TYPE candType)
{

    ICE_MEDIA_STREAM *mediaStream;
    ICE_CANDIDATE *iceCand;

    if (mediaIdx >= pInstance->remoteIceMedia.numberOfICEMediaLines) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug, "Failed to add candidate. Wrong media idx\n");
        return -1;
    }
    mediaStream = &(pInstance->remoteIceMedia.mediaStream[mediaIdx]);

    if (mediaStream->numberOfCandidates >= ICE_MAX_CANDIDATES) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug, "Failed to add REMOTE candidate. MAX number of candidates reached");
        
        return -1;
    }
    iceCand = &mediaStream->candidate[mediaStream->numberOfCandidates];


    //connection address
    if (! sockaddr_initFromString((struct sockaddr *)&iceCand->connectionAddr,
                                 connectionAddr)) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug, "Failed to add candidate. Something wrong with IP address\n");
        return -1;

    }
    sockaddr_setPort((struct sockaddr *)&iceCand->connectionAddr, port);



    //foundation
    memset(iceCand->foundation, 0, sizeof(iceCand->foundation));
    strncpy(iceCand->foundation,
            foundation,
            min(sizeof(iceCand->foundation) , foundationLen));
    iceCand->foundation[sizeof(iceCand->foundation) - 1] = '\0';

    //componentid
    iceCand->componentid = componentId;

    //priority
    iceCand->priority = priority;

    //Candidate Type
    iceCand->type = candType;


    mediaStream->numberOfCandidates++;

    return mediaStream->numberOfCandidates;
}


int32_t ICELIB_addLocalMediaStream(ICELIB_INSTANCE *pInstance,
                                   uint32_t mediaIdx,
                                   uint32_t userValue1,
                                   uint32_t userValue2,
                                   ICE_CANDIDATE_TYPE defaultCandType)
{

    ICE_MEDIA_STREAM *mediaStream;

    if (mediaIdx >= ICE_MAX_MEDIALINES) {

        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug, "Failed to add local media stream. Index larger than MAX number of medialines\n");
        return -1;
    }


    mediaStream = &pInstance->localIceMedia.mediaStream[mediaIdx];

    mediaStream->userValue1 = userValue1;
    mediaStream->userValue2 = userValue2;

    ICELIB_createUfrag( mediaStream->ufrag,  ICELIB_UFRAG_LENGTH);
    ICELIB_createPasswd(mediaStream->passwd, ICELIB_PASSWD_LENGTH);

    mediaStream->defaultCandType = defaultCandType;

    //TODO: Can be possible holes in the array. Must fix that.
    pInstance->localIceMedia.numberOfICEMediaLines++;


    return 1;
}

int32_t ICELIB_addRemoteMediaStream(ICELIB_INSTANCE *pInstance,
                                    char* ufrag, char *pwd, struct sockaddr *defaultAddr)
{
    ICE_MEDIA_STREAM *mediaStream;

    if (pInstance->remoteIceMedia.numberOfICEMediaLines >= ICE_MAX_MEDIALINES) {
        ICELIB_log(&pInstance->callbacks.callbackLog,
                   ICELIB_logDebug, "Failed to add medialine. MAX number of medialines reached\n");
        return -1;
    }
    
    mediaStream = &pInstance->remoteIceMedia.mediaStream[pInstance->remoteIceMedia.numberOfICEMediaLines];

    if ((ufrag != NULL) && (pwd != NULL)) {
        memset(mediaStream->ufrag,0, sizeof(mediaStream->ufrag));
        strncpy(mediaStream->ufrag,
                ufrag,
                min(sizeof(mediaStream->ufrag)-1, strlen(ufrag)));

        memset(mediaStream->passwd,0 ,sizeof(mediaStream->passwd));
        strncpy(mediaStream->passwd,
                pwd,
                min(sizeof(mediaStream->passwd)-1, strlen(pwd)));
    }else{
        ICELIB_log(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug, "Failed to add medialine. No UFRAG or PASSWD\n");
        return -1;

    }

    if (defaultAddr != NULL) {
        sockaddr_copy((struct sockaddr *)&mediaStream->defaultAddr, 
                      (struct sockaddr *)defaultAddr);
    }else{
        ICELIB_log(&pInstance->callbacks.callbackLog,
                    ICELIB_logDebug, "Failed to add medialine. No default address\n");
        return -1;

    }

    pInstance->remoteIceMedia.numberOfICEMediaLines++;


    return pInstance->remoteIceMedia.numberOfICEMediaLines;
}


ICE_TURN_STATE ICELIB_getTurnState(const ICELIB_INSTANCE  *pInstance, uint32_t mediaIdx)
{
    return pInstance->localIceMedia.mediaStream[mediaIdx].turnState;
}

void ICELIB_setTurnState(ICELIB_INSTANCE  *pInstance, uint32_t mediaIdx, ICE_TURN_STATE turnState)
{
    pInstance->localIceMedia.mediaStream[mediaIdx].turnState = turnState;
}

ICE_MEDIA const *ICELIB_getLocalIceMedia(const ICELIB_INSTANCE *pInstance)
{
    return &pInstance->localIceMedia;
}

struct sockaddr const *ICELIB_getLocalRelayAddr(const ICELIB_INSTANCE *pInstance,
                                                uint32_t mediaIdx)
{
    uint32_t i;
    for (i=0;i<pInstance->localIceMedia.numberOfICEMediaLines;i++) {

        if (pInstance->localIceMedia.mediaStream[mediaIdx].candidate[i].type == ICE_CAND_TYPE_RELAY)
        {
            return (struct sockaddr *)&pInstance->localIceMedia.mediaStream[mediaIdx].candidate[i].connectionAddr;
        }
    }

    return NULL;
}


struct sockaddr const *ICELIB_getLocalRelayAddrFromHostAddr(const ICELIB_INSTANCE *pInstance,
                                                     const struct sockaddr *hostAddr)
{
    //TODO: Rewrite Not using MAX values (Use actual number of stred instead)
    int i,j,k = 0;

    for (i=0;i<ICE_MAX_MEDIALINES;i++) {
            for (j=0;j<ICE_MAX_CANDIDATES;j++) {

                if (sockaddr_alike((const struct sockaddr *)&pInstance->localIceMedia.mediaStream[i].candidate[j].connectionAddr,
                                   (const struct sockaddr *)hostAddr)) {
                    //Found a match. No find the relay candidate..
                    uint32_t compId = pInstance->localIceMedia.mediaStream[i].candidate[j].componentid;
                    for (k=0;k<ICE_MAX_CANDIDATES;k++) {
                        if (pInstance->localIceMedia.mediaStream[i].candidate[k].type == ICE_CAND_TYPE_RELAY &&
                             pInstance->localIceMedia.mediaStream[i].candidate[k].componentid == compId)

                            return (struct sockaddr *)&pInstance->localIceMedia.mediaStream[i].candidate[k].connectionAddr;
                    }
                }
            }
    }

    return NULL;

}
