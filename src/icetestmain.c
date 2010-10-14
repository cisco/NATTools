#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <error.h>
#include <errno.h>

#include "stdbool.h"

#include "icelib.h"

#define PRIORITY_HOST_1     ( ( 126 << 24) | (65535 << 8) | ( 256 - 1))
#define PRIORITY_HOST_2     ( ( 126 << 24) | (65535 << 8) | ( 256 - 2))
#define PRIORITY_SRFLX_1    ( ( 100 << 24) | (65535 << 8) | ( 256 - 1))
#define PRIORITY_SRFLX_2    ( ( 100 << 24) | (65535 << 8) | ( 256 - 2))
#define PRIORITY_RELAY_1    ( (   0 << 24) | (65535 << 8) | ( 256 - 1))
#define PRIORITY_RELAY_2    ( (   0 << 24) | (65535 << 8) | ( 256 - 2))
#define PRIORITY_PRFLX_1    ( ( 110 << 24) | (65535 << 8) | ( 256 - 1))
#define PRIORITY_PRFLX_2    ( ( 110 << 24) | (65535 << 8) | ( 256 - 2))

#define FOUNDATION_HOST     "1"
#define FOUNDATION_SRFLX    "3"
#define FOUNDATION_RELAY    "4"
#define FOUNDATION_PRFLX    "2"


/*

#define unittest_assert( test) _unittest_assert( __FILE__, __LINE__, test)

void _unittest_assert( char *file, int line, bool test)
{
    if( !test) {
        printf( "****FAIL! %s (%d)\n", file, line);
    }
}

*/



#define subTestBegin() printf( "### %s ###########################################\n\n", __FUNCTION__);


typedef struct {
    bool                        callbackCalled;
    const NET_ADDR             *destination;
    const NET_ADDR             *source;
    uint32_t                    userValue1;
    uint32_t                    userValue2;
    uint32_t                    componentId;
    bool                        useRelay;
    const char                 *pUfragPair;
    const char                 *pPasswd;
    uint32_t                    peerPriority;
    bool                        useCandidate;
    bool                        iceControlling;
    uint64_t                    tieBreaker;
} brCallbackData;


void tt_error(int t, char *msg)
{
    printf( "TT_ERROR: %s\n", msg);
}


unsigned int errorCount = 0;
unsigned int count = 0;


//----- Create a 'username credential' string for a check list.
//      The length of the string, "maxlength", includes the terminating
//      null character (i.e. maxlength=5 yields 4 characters and a '\0').
//
static char *ICELIB_getCheckListUsernamePair( char                    *dst,
                                       int                      maxlength,
                                       const ICELIB_CHECKLIST  *pCheckList)
{
    ICELIB_makeUsernamePair( dst,
                             maxlength,
                             pCheckList->ufragRemote,
                             pCheckList->ufragLocal);
    return dst;
}


static const char *ICELIB_getCheckListPasswd( const ICELIB_CHECKLIST *pCheckList)
{
    return pCheckList->passwdRemote;
}




//----- Duplicates from unittest.c (to make it possible to run standalone):

void unittest_assert_(const char *f, unsigned int l, const char *x, int condition)
{
    if( !condition) {
        printf( "****FAIL! %s (%d) - %s\n", f, l, x);
        ++errorCount;
    }

    ++count;
}


void unittest_context( char *msg)
{
    printf( "Unit test started: %s\n", msg);
}


void unittest_ice(void);
void ICELIB_clearRedundantCandidates( ICE_CANDIDATE Candidates[]);
// void ICELIB_compactTable( ICE_CANDIDATE Candidates[]);
int  ICELIB_countCandidates( ICE_CANDIDATE Candidates[]);
int  ICELIB_eliminateRedundantCandidates( ICE_CANDIDATE Candidates[]);



void fillCandidate( ICE_CANDIDATE *pCandidate, ICE_CANDIDATE_TYPE type, NET_ADDR *pNa, int componentId)
{
    ICELIB_createFoundation( pCandidate->foundation, type, ICELIB_FOUNDATION_LENGTH);
    pCandidate->componentid    = componentId;
    pCandidate->priority       = ICELIB_calculatePriority( ICE_CAND_TYPE_HOST, componentId);
    pCandidate->connectionAddr = *pNa;
    pCandidate->type           = type;
    pCandidate->userValue1     = 1;
    pCandidate->userValue2     = 2;
}


void showFoundation( ICE_CANDIDATE candidates[])
{
    int i;

    for( i=0; i < ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES; ++i) {
        printf( "Foundation [%d]: '%s'\n", i, candidates[ i].foundation);
    }
}


void _makeTransportAddresses(NET_ADDR na[],
                             char const * addr[],
                             unsigned int numberOfAddresses,
                             NET_PROTO eProto)
{
    unsigned int i;

    for (i = 0; i < numberOfAddresses; ++i) {
        NetAddr_initFromIPv4StringWithPort( &na[ i], addr[ i], eProto);
    }
}


void _makeMediaStream( ICE_MEDIA_STREAM *pMediaStream, char const * addr[])
{
    NET_ADDR                na[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];

    _makeTransportAddresses( na,
                             addr,
                             ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES,
                             NET_PROTO_UDP);

    ICELIB_createIceMediaStream( pMediaStream,
                                 0, 1,
                                 &na[ 0],
                                 &na[ 1],
                                 &na[ 2],
                                 &na[ 3],
                                 &na[ 4],
                                 &na[ 5]);
}


void test_ufrag_passwd( void)
{
    char ufrag     [ ICE_MAX_UFRAG_LENGTH     ];
    char passwd    [ ICE_MAX_PASSWD_LENGTH    ];
    char foundation[ ICE_MAX_FOUNDATION_LENGTH];

    subTestBegin();

    memset( ufrag,      35, sizeof(ufrag     ));
    memset( passwd,     35, sizeof(passwd    ));
    memset( foundation, 35, sizeof(foundation));

    ICELIB_createUfrag ( ufrag, ICELIB_UFRAG_LENGTH);

    printf( "\n");

    ICELIB_createPasswd( passwd, ICELIB_PASSWD_LENGTH);

    printf( "Ufrag : '%s' - length: %d\n", ufrag,  strlen( ufrag));
    printf( "Passwd: '%s' - length: %d\n", passwd, strlen( passwd));

    printf( "\n");

    ICELIB_createUfrag( ufrag, ICE_MAX_UFRAG_LENGTH + 999);

    printf( "Ufrag (max): '%s' - length: %d\n", ufrag,  strlen( ufrag));

    printf( "\n");

    printf( "Priority (host/compid=1 ): %u\n", ICELIB_calculatePriority( ICE_CAND_TYPE_HOST,  1));
    printf( "Priority (host/compid=2 ): %u\n", ICELIB_calculatePriority( ICE_CAND_TYPE_HOST,  2));
    printf( "Priority (srflx/compid=1): %u\n", ICELIB_calculatePriority( ICE_CAND_TYPE_SRFLX, 1));
    printf( "Priority (srflx/compid=2): %u\n", ICELIB_calculatePriority( ICE_CAND_TYPE_SRFLX, 2));
    printf( "Priority (relay/compid=1): %u\n", ICELIB_calculatePriority( ICE_CAND_TYPE_RELAY, 1));
    printf( "Priority (relay/compid=2): %u\n", ICELIB_calculatePriority( ICE_CAND_TYPE_RELAY, 2));
    printf( "Priority (prflx/compid=1): %u\n", ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1));
    printf( "Priority (prflx/compid=2): %u\n", ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2));

    printf( "\n");

    ICELIB_createFoundation( foundation, ICE_CAND_TYPE_HOST, ICELIB_FOUNDATION_LENGTH);
    printf( "Foundation (host ) : '%s' - length: %d\n", foundation,  strlen( foundation));

    ICELIB_createFoundation( foundation, ICE_CAND_TYPE_SRFLX, ICELIB_FOUNDATION_LENGTH);
    printf( "Foundation (srflx) : '%s' - length: %d\n", foundation,  strlen( foundation));

    ICELIB_createFoundation( foundation, ICE_CAND_TYPE_RELAY, ICELIB_FOUNDATION_LENGTH);
    printf( "Foundation (relay) : '%s' - length: %d\n", foundation,  strlen( foundation));

    ICELIB_createFoundation( foundation, ICE_CAND_TYPE_PRFLX, ICELIB_FOUNDATION_LENGTH);
    printf( "Foundation (prflx) : '%s' - length: %d\n", foundation,  strlen( foundation));

    printf( "\n");
}


void test_createIceMediaStream( void)
{
    int n;
    ICE_CANDIDATE    candidates[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];
    ICE_MEDIA_STREAM iceMediaStream;
    NET_ADDR         na1;
    NET_ADDR         na2;
    NET_ADDR         na3;
    NET_ADDR         na4;
    NET_ADDR         na5;
    NET_ADDR         na6;

    subTestBegin();

    memset( candidates, 0, sizeof( candidates));

    NetAddr_initFromIPv4StringWithPort( &na1, "10.47.1.58:1234", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na2, "10.47.1.59:1235", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na3, "10.47.1.58:1236", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na4, "10.47.1.60:1237", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na5, "10.47.1.61:1238", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na6, "10.47.1.62:1239", NET_PROTO_UDP);

    showFoundation( candidates);
    printf( "Number of candidates [empty          ]: %d\n", ICELIB_countCandidates( candidates));

    fillCandidate( &candidates[ 1], ICE_CAND_TYPE_HOST,  &na1, 1);
    fillCandidate( &candidates[ 2], ICE_CAND_TYPE_RELAY, &na1, 1);

    showFoundation( candidates);
    printf( "Number of candidates [fill 2 equal   ]: %d\n", ICELIB_countCandidates( candidates));

    ICELIB_clearRedundantCandidates( candidates);

    showFoundation( candidates);
    printf( "Number of candidates [clear redundant]: %d\n", ICELIB_countCandidates( candidates));

    fillCandidate( &candidates[ 4], ICE_CAND_TYPE_SRFLX, &na3, 1);

    showFoundation( candidates);
    printf( "Number of candidates [one more       ]: %d\n", ICELIB_countCandidates( candidates));

/*
    ICELIB_clearRedundantCandidates( candidates);
    ICELIB_compactTable( candidates);
*/

    n = ICELIB_eliminateRedundantCandidates( candidates);

    printf( "ICELIB_eliminateRedundantCandidates: %d\n", n);

    showFoundation( candidates);
    printf( "Number of candidates [compact        ]: %d\n", ICELIB_countCandidates( candidates));

    memset( candidates, 0, sizeof( candidates));

    fillCandidate( &candidates[ 0], ICE_CAND_TYPE_HOST,  &na1, 1);
    fillCandidate( &candidates[ 2], ICE_CAND_TYPE_RELAY, &na1, 1);
    fillCandidate( &candidates[ 4], ICE_CAND_TYPE_SRFLX, &na3, 1);
    fillCandidate( &candidates[ 5], ICE_CAND_TYPE_PRFLX, &na4, 1);

    showFoundation( candidates);
    printf( "Number of candidates [4 candidates   ]: %d\n", ICELIB_countCandidates( candidates));

    n = ICELIB_eliminateRedundantCandidates( candidates);

    printf( "ICELIB_eliminateRedundantCandidates: %d\n", n);

    showFoundation( candidates);
    printf( "Number of candidates [eliminate      ]: %d\n", ICELIB_countCandidates( candidates));

    memset( &iceMediaStream, 0, sizeof( iceMediaStream));

    ICELIB_createIceMediaStream( &iceMediaStream, 1, 2, &na1, &na2, &na3, &na4, &na5, &na6); 

    ICELIBTYPES_ICE_MEDIA_STREAM_dump( &iceMediaStream);

    printf( "\n");
}


void test_pairPriority( void)
{
    uint32_t G;
    uint32_t D;
    uint64_t priority;

    subTestBegin();

    G = 0x12345678;
    D = 0x76543210;

    priority = ICELIB_pairPriority( G, D);
    printf( "G=0x%08x, D=0x%08x, priority=0x%016llx\n", G, D, priority);

    G = 0x76543210;
    D = 0x12345678;

    priority = ICELIB_pairPriority( G, D);
    printf( "G=0x%08x, D=0x%08x, priority=0x%016llx\n", G, D, priority);

    G = 0x11111111;
    D = 0x22222222;

    priority = ICELIB_pairPriority( G, D);
    printf( "G=0x%08x, D=0x%08x, priority=0x%016llx\n", G, D, priority);

    G = 0x22222222;
    D = 0x11111111;

    priority = ICELIB_pairPriority( G, D);
    printf( "G=0x%08x, D=0x%08x, priority=0x%016llx\n", G, D, priority);

    printf( "\n");
}


void test_findBases()
{
    ICE_MEDIA_STREAM    mediaStreamLocal;
    NET_ADDR            na1;
    NET_ADDR            na2;
    NET_ADDR            na3;
    NET_ADDR            na4;
    NET_ADDR            na5;
    NET_ADDR            na6;
    bool                foundBases;
    const ICE_CANDIDATE *pBaseSrvRflxRtp;
    const ICE_CANDIDATE *pBaseSrvRflxRtcp;

    subTestBegin();

    NetAddr_initFromIPv4StringWithPort( &na1, "10.47.1.58:1234", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na2, "10.47.1.59:1235", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na3, "10.47.1.58:1236", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na4, "10.47.1.60:1237", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na5, "10.47.1.61:1238", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na6, "10.47.1.62:1239", NET_PROTO_UDP);

    ICELIB_createIceMediaStream( &mediaStreamLocal,
                                 1, 2,
                                 &na1,
                                 &na2,
                                 &na3,
                                 &na4,
                                 &na5,
                                 &na6);

    printf( "------ Local media stream -----------------\n");
    ICELIBTYPES_ICE_MEDIA_STREAM_dump( &mediaStreamLocal);

    printf( "\n");

    foundBases = ICELIB_findReflexiveBaseAddresses( &pBaseSrvRflxRtp,
                                                    &pBaseSrvRflxRtcp,
                                                    &mediaStreamLocal);

    printf( "Found bases: %d\n", foundBases);

    printf("BaseSrvRflxRtp : ");
    ICELIBTYPES_ICE_CANDIDATE_dump( pBaseSrvRflxRtp);
    printf( "\n");
    printf("BaseSrvRflxRtcp: ");
    ICELIBTYPES_ICE_CANDIDATE_dump( pBaseSrvRflxRtcp);
    printf( "\n");
    printf( "\n");

    printf( "\n");

}


void test_formPairsBasic( void)
{
    ICELIB_CHECKLIST    checkList;
    ICE_MEDIA_STREAM    mediaStreamLocal;
    ICE_MEDIA_STREAM    mediaStreamRemote;
    NET_ADDR            na1;
    NET_ADDR            na2;
    NET_ADDR            na3;
    NET_ADDR            na4;
    NET_ADDR            na5;
    NET_ADDR            na6;
    const ICE_CANDIDATE *pBaseSrvRflxRtp;
    const ICE_CANDIDATE *pBaseSrvRflxRtcp;
    bool                foundBases;
    ICELIB_LIST_PAIR    Pair;
    ICELIB_CALLBACK_LOG CallbackLog;

    subTestBegin();

    NetAddr_initFromIPv4StringWithPort( &na1, "10.47.1.58:1234", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na2, "10.47.1.59:1235", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na3, "10.47.1.58:1236", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na4, "10.47.1.60:1237", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na5, "10.47.1.61:1238", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na6, "10.47.1.62:1239", NET_PROTO_UDP);

    ICELIB_createIceMediaStream( &mediaStreamLocal, 1, 2, &na1, &na2, &na3, &na4, &na5, &na6);

    printf( "------ Local media stream -----------------\n");
    ICELIBTYPES_ICE_MEDIA_STREAM_dump( &mediaStreamLocal);

    printf( "\n");

    NetAddr_initFromIPv4StringWithPort( &na1, "10.47.1.100:2000", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na2, "10.47.1.101:2001", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na3, "10.47.1.102:2002", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na4, "10.47.1.103:2003", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na5, "10.47.1.104:2004", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &na6, "10.47.1.105:2005", NET_PROTO_UDP);

    ICELIB_createIceMediaStream( &mediaStreamRemote, 1, 2, &na1, &na2, &na3, &na4, &na5, &na6);
//    ICELIB_createIceMediaStream( &mediaStreamRemote, &na1, &na2, NULL, &na4, &na5, NULL);

    printf( "------ Remote media stream -----------------\n");
    ICELIBTYPES_ICE_MEDIA_STREAM_dump( &mediaStreamRemote);

    printf( "\n");

    ICELIB_resetCheckList( &checkList, 0);
    ICELIB_saveUfragPasswd( &checkList, &mediaStreamLocal, &mediaStreamRemote);

    ICELIB_formPairs( &checkList, &CallbackLog, &mediaStreamLocal, &mediaStreamRemote, 50);
    ICELIB_computeListPairPriority( &checkList, true);

    printf( "\n");
    printf( "------ Dumping check list (pre-sort)-------\n\n");
//    ICELIB_checkListDump( &checkList);

    ICELIB_sortPairsCL( &checkList);

    printf( "\n\n\n");
    printf( "------ Dumping check list (post sort)------\n\n\n");
    ICELIB_checkListDump( &checkList);

    foundBases = ICELIB_findReflexiveBaseAddresses( &pBaseSrvRflxRtp,
                                                    &pBaseSrvRflxRtcp,
                                                    &mediaStreamLocal);
    if( !foundBases) {
        printf( "### Base addresses not found!\n");
    }

    ICELIB_prunePairs( &checkList, pBaseSrvRflxRtp, pBaseSrvRflxRtcp);

    printf( "\n\n\n");
    printf( "------ Dumping check list (post ICELIB_prunePairs)------\n\n\n");
    ICELIB_checkListDump( &checkList);

    ICELIB_computeStatesSetWaitingFrozen( &checkList);

    printf( "\n\n\n");
    printf( "------ Dumping check list (after set FROZEN/WAITING)------\n\n\n");
    ICELIB_checkListDump( &checkList);

    Pair.pairState        = ICELIB_PAIR_PAIRED;
    Pair.defaultPair      = 0;
    Pair.nominatedPair    = 0;
    Pair.pairPriority     = 0x7efffffffdfffffdLL;
    Pair.pLocalCandidate  = checkList.checkListPairs[ 11].pLocalCandidate;
    Pair.pRemoteCandidate = checkList.checkListPairs[ 11].pRemoteCandidate;

    printf( "ICELIB_insertIntoCheckList: %d\n",
            ICELIB_insertIntoCheckList( &checkList, &Pair));

    printf( "\n\n\n");
    printf( "------ Dumping check list (after inserted one)------\n\n\n");
    ICELIB_checkListDump( &checkList);


    printf( "\n");

}


ICELIB_Result _sendBRequest( void               *pUserData,
                             const NET_ADDR     *destination,
                             const NET_ADDR     *source,
                             uint32_t           userValue1,
                             uint32_t           userValue2,
                             uint32_t           componentId,
                             bool               useRelay,
                             const char         *pUfrag,
                             const char         *pPasswd,
                             uint32_t           peerPriority,
                             bool               useCandidate,
                             bool               iceControlling,
                             uint64_t           tieBreaker,
                             StunMsgId          transactionId)
{
    printf( "CALLBACK CALLED! ----------\n");

    printf( "destination    : ");
    NetAddr_dump( destination, true);
    printf( "\n");

    printf( "source         : ");
    NetAddr_dump( source, true);
    printf( "\n");

    printf( "userValue1     : %u\n",   userValue1);
    printf( "userValue2     : %u\n",   userValue2);
    printf( "componentId    : %u\n",   componentId);
    printf( "useRelay       : %d\n",   useRelay);
    printf( "pUfrag         : '%s'\n", pUfrag);
    printf( "pPasswd        : '%s'\n", pPasswd);
    printf( "peerPriority   : %u\n",   peerPriority);
    printf( "useCandidate   : %d\n",   useCandidate);
    printf( "iceControlling : %d\n",   iceControlling);
    printf( "tieBreaker     : 0x%016llxd\n", tieBreaker);

    printf( "transactionId  : ");
    ICELIB_transactionIdDump( transactionId);
    printf( "\n");
    printf( "\n");

    return ICELIB_Result_OK;
}


void test_iceCheckList( void)
{
    bool              checkWasSceduled;
    ICELIB_INSTANCE   Instance;
    ICE_MEDIA         mediaLocal;
    ICE_MEDIA         mediaRemote;
    ICE_MEDIA_STREAM  mediaStreamLocal_0;
    ICE_MEDIA_STREAM  mediaStreamLocal_1;
    ICE_MEDIA_STREAM  mediaStreamRemote_0;
    ICE_MEDIA_STREAM  mediaStreamRemote_1;
    char              ufragPair[ ICE_MAX_UFRAG_PAIR_LENGTH];
    brCallbackData    brData;

    ICELIB_CONFIGURATION config;

    char const * addrLocal_0[] = {
        "10.47.1.58:1234",
        "10.47.1.59:1235",
        "10.47.1.58:1236",
        "10.47.1.60:1237",
        "10.47.1.61:1238",
        "10.47.1.62:1239",
    };

    char const * addrLocal_1[] = {
        "10.47.2.58:1234",
        "10.47.2.59:1235",
        "10.47.2.58:1236",
        "10.47.2.60:1237",
        "10.47.2.61:1238",
        "10.47.2.62:1239",
    };

    char const * addrRemote_0[] = {
        "10.47.1.100:2000",
        "10.47.1.101:2001",
        "10.47.1.102:2002",
        "10.47.1.103:2003",
        "10.47.1.104:2004",
        "10.47.1.105:2005",
    };

    char const * addrRemote_1[] = {
        "10.47.2.100:2000",
        "10.47.2.101:2001",
        "10.47.2.102:2002",
        "10.47.2.103:2003",
        "10.47.2.104:2004",
        "10.47.2.105:2005",
    };

    memset( &config, 0, sizeof( config));

    config.tickIntervalMS       = 20;
    config.keepAliveIntervalS   = 15;
    config.maxCheckListPairs    = ICELIB_MAX_PAIRS;
    config.aggressiveNomination = false;

    ICELIB_Constructor( &Instance, &config);
//
//- Create 4 media streams
//
    _makeMediaStream( &mediaStreamLocal_0,  addrLocal_0);
    _makeMediaStream( &mediaStreamLocal_1,  addrLocal_1);
    _makeMediaStream( &mediaStreamRemote_0, addrRemote_0);
    _makeMediaStream( &mediaStreamRemote_1, addrRemote_1);
//
//- Create local and remote ICE_MEDIA_STREAMs
//
    memset( &mediaLocal,  0, sizeof( mediaLocal));
    memset( &mediaRemote, 0, sizeof( mediaRemote));

    mediaLocal.mediaStream[ 0]        = mediaStreamLocal_0;
    mediaLocal.mediaStream[ 1]        = mediaStreamLocal_1;
    mediaLocal.numberOfICEMediaLines  = 2;

    mediaRemote.mediaStream[ 0]       = mediaStreamRemote_0;
    mediaRemote.mediaStream[ 1]       = mediaStreamRemote_1;
    mediaRemote.numberOfICEMediaLines = 2;
//
//- Add local and remote candidates
//
    ICELIB_addLocalCandidates ( &Instance, &mediaLocal, false);
    ICELIB_addRemoteCandidates( &Instance, &mediaRemote, false, false);

    printf( "---Local ICE Media--------------------------------------------------\n");
    ICELIBTYPES_ICE_MEDIA_dump( &Instance.localIceMedia);
    printf( "---Remore ICE Media-------------------------------------------------\n");
    ICELIBTYPES_ICE_MEDIA_dump( &Instance.remoteIceMedia);

    ICELIB_checkListDumpAll( &Instance);

    brData.callbackCalled = false;
    brData.destination    = &Instance.streamControllers[ 0].checkList.checkListPairs[ 0].pRemoteCandidate->connectionAddr;
    brData.source         = &Instance.streamControllers[ 0].checkList.checkListPairs[ 0].pLocalCandidate->connectionAddr;
    brData.userValue1     = 1;
    brData.userValue1     = 2;
    brData.componentId    = Instance.streamControllers[ 0].checkList.checkListPairs[ 0].pLocalCandidate->componentid;
    brData.useRelay       =  Instance.streamControllers[ 0].checkList.checkListPairs[ 0].pLocalCandidate->type == ICE_CAND_TYPE_RELAY ? 1 : 0;
    brData.pUfragPair     = ICELIB_getCheckListUsernamePair( ufragPair, ICE_MAX_UFRAG_PAIR_LENGTH, &Instance.streamControllers[ 0].checkList);
    brData.pPasswd        = ICELIB_getCheckListPasswd( &Instance.streamControllers[ 0].checkList);
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1);
    brData.useCandidate   = false;
    brData.iceControlling = Instance.iceControlling;
    brData.tieBreaker     = Instance.tieBreaker;

    ICELIB_setCallbackOutgoingBindingRequest( &Instance, _sendBRequest, &brData);

    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);

    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);

    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);

    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamControllers[ 0]);
    printf( "checkWasSceduled: %d\n", checkWasSceduled);

//    ICELIB_checkListDumpAll( &Instance);

}


void test_fifo_dump( ICELIB_TRIGGERED_FIFO *pf)
{
    int i;

    printf( "count: %u\n", ICELIB_fifoCount( pf));
    printf( "is empty: %d\n", ICELIB_fifoIsEmpty( pf));
    printf( "is full : %d\n", ICELIB_fifoIsFull( pf));
    printf( "inIndex: %d - outIndex: %d\n", pf->inIndex, pf->outIndex);
    for( i=0; i < ICELIB_MAX_FIFO_ELEMENTS; ++i) {
        printf( "  element[%d]: %u\n", i, pf->elements[ i]);
    }
}


void test_fifo( void)
{
    ICELIB_TRIGGERED_FIFO f;

    ICELIB_fifoClear( &f);
    test_fifo_dump( &f);

    ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT)1);
    ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT)2);
    ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT)3);
    ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT)4);
    test_fifo_dump( &f);

    ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT)5);
    test_fifo_dump( &f);

    ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT)6);
    test_fifo_dump( &f);

    printf( "--get: %u\n", ICELIB_fifoGet( &f));
    printf( "--get: %u\n", ICELIB_fifoGet( &f));
    test_fifo_dump( &f);

    ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT)100);
    ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT)101);
    ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT)102);
    test_fifo_dump( &f);

    printf( "--get: %u\n", ICELIB_fifoGet( &f));
    printf( "--get: %u\n", ICELIB_fifoGet( &f));
    printf( "--get: %u\n", ICELIB_fifoGet( &f));
    test_fifo_dump( &f);
    printf( "--get: %u\n", ICELIB_fifoGet( &f));
    printf( "--get: %u\n", ICELIB_fifoGet( &f));
    printf( "--get: %u\n", ICELIB_fifoGet( &f));
    test_fifo_dump( &f);
}


/*-----------------------------------------------------------------------------
 * Do the test...
 */
int main(int argc, char *argv[])
{
    printf( "\nStarting...\n");

    srandom( time( NULL));

    printf( "\n");

//    test_ufrag_passwd();
//    test_createIceMediaStream();
//    test_pairPriority();
//    test_findBases();
//    test_formPairsBasic();
//    test_fifo();
//    test_iceCheckList();

    printf( "Tie breaker: 0x%016llx\n\n", ICELIB_makeTieBreaker());
    printf( "sizeof( ICELIB_INSTANCE)         : %d\n", sizeof( ICELIB_INSTANCE));
    printf( "sizeof( ICE_MEDIA)               : %d\n", sizeof( ICE_MEDIA));
    printf( "sizeof( ICE_MEDIA_STREAM)        : %d\n", sizeof( ICE_MEDIA_STREAM));
    printf( "sizeof( ICE_CANDIDATE)           : %d\n", sizeof( ICE_CANDIDATE));
    printf( "sizeof( ICELIB_STREAM_CONTROLLER): %d\n", sizeof( ICELIB_STREAM_CONTROLLER));
    printf( "sizeof( ICELIB_CHECKLIST)        : %d\n", sizeof( ICELIB_CHECKLIST));
    printf( "sizeof( ICELIB_TRIGGERED_FIFO)   : %d\n", sizeof( ICELIB_TRIGGERED_FIFO));
    printf( "\n");

    printf( "Transaction ID: ");
    ICELIB_transactionIdDump( ICELIB_generateTransactionId());
    printf( "\n\n");

    printf( "### unittest_ice #################################\n\n");

    unittest_ice();             //----- Run unit tests from unittest_ice.c

    printf( "Number of asserts        : %d\n", count);
    printf( "Number of FAILED asserts : %d\n", errorCount);

    printf( "\n");

    return 0;
}
