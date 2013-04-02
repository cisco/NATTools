#ifdef UNITTEST


#include <string.h>
#include <stdio.h>

#include "unittest.h"
#include "icelib.h"

#ifdef ICE_DEBUG
  #define UNITTEST_LOG_LEVEL ICELIB_logDebug
#else
  #define UNITTEST_LOG_LEVEL ICELIB_logDisable
#endif

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
} BindingRequestCallbackData;


typedef struct {
    bool                callbackCalled;
    uint32_t            userValue1;
    uint32_t            userValue2;
    const NET_ADDR     *source;
    const NET_ADDR     *destination;
    const NET_ADDR     *MappedAddress;
    uint16_t            errorResponse;
    StunMsgId           transactionId;
} BindingResponseCallbackData;


typedef struct {
    bool                callbackCalled;
    const char          *message;
} LogFunctionCallbackData;


static bool isLegalCharacter( char ch)
{
    const char *set = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    const char *ptr = set;

    while( *ptr) {
        if( ch == *ptr++) {
            return true;
        }
    }

    return false;
}


static void noPrintf( void *pUserData, ICELIB_logLevel logLevel, const char *str)
{
#if 0
#ifdef ICE_DEBUG
    printf( str);
#endif
#endif
}


static bool isLegalString( const char *str)
{
    while( *str) {
        if( !isLegalCharacter( *str++)) {
            return false;
        }
    }

    return true;
}


static void fillNetAddrPointerArray( NET_ADDR *netAddr[], NET_ADDR *pValue, int n)
{
    while( n--) {
        *netAddr++ = pValue;
    }
}


bool testCandidateType( ICE_CANDIDATE_TYPE type, int index)
{
    ICE_CANDIDATE_TYPE expectedType;

    switch( index) {
        case 0:
        case 1: expectedType = ICE_CAND_TYPE_HOST;
            break;
        case 2:
        case 3: expectedType = ICE_CAND_TYPE_SRFLX;
            break;
        case 4:
        case 5: expectedType = ICE_CAND_TYPE_RELAY;
            break;
        default: return false;
    }

    return expectedType == type;
}


bool testCandidateFoundation(const char * foundation, int index)
{
    const char * expectedFoundation;

    switch( index) {
        case 0:
        case 1: expectedFoundation = FOUNDATION_HOST;
            break;
        case 2:
        case 3: expectedFoundation = FOUNDATION_SRFLX;
            break;
        case 4:
        case 5: expectedFoundation = FOUNDATION_RELAY;
            break;
        default: return false;
    }

    return strcmp( expectedFoundation, foundation) == 0;
}


bool testCandidatePriority( uint32_t priority, int index)
{
    uint32_t expectedPriority;

    switch( index) {
        case 0: expectedPriority = PRIORITY_HOST_1;
            break;
        case 1: expectedPriority = PRIORITY_HOST_2;
            break;
        case 2: expectedPriority = PRIORITY_SRFLX_1;
            break;
        case 3: expectedPriority = PRIORITY_SRFLX_2;
            break;
        case 4: expectedPriority = PRIORITY_RELAY_1;
            break;
        case 5: expectedPriority = PRIORITY_RELAY_2;
            break;
        default: return false;
    }

    return expectedPriority == priority;
}


static void assertMediaStream(ICE_MEDIA_STREAM * pMediaStream,
                              unsigned int streamCount,
                              NET_ADDR * pNa[],
                              int candidateIndex[])
{
    unsigned int i;

    unittest_assert(
        ( strlen( pMediaStream->ufrag) + 1) == ICELIB_UFRAG_LENGTH);
    unittest_assert(
        isLegalString( pMediaStream->ufrag));

    unittest_assert(
        ( strlen( pMediaStream->passwd) + 1) == ICELIB_PASSWD_LENGTH);
    unittest_assert(
        isLegalString( pMediaStream->passwd));

    unittest_assert( pMediaStream->numberOfEntries == (int)streamCount);

    for (i = 0; i < streamCount; ++i) {

        unittest_assert(
            NetAddr_alike( &pMediaStream->candidate[ i].connectionAddr,
                           pNa[ candidateIndex[ i]])
            );

        unittest_assert(
                        pMediaStream->candidate[ i].componentid == (unsigned int)((candidateIndex[i] % 2) + 1)
            );

        unittest_assert(
            testCandidateType( pMediaStream->candidate[ i].type, candidateIndex[ i])
            );

        unittest_assert(
            testCandidateFoundation( pMediaStream->candidate[ i].foundation,
                                     candidateIndex[ i])
            );

        unittest_assert(
            testCandidatePriority( pMediaStream->candidate[ i].priority,
                                   candidateIndex[ i])
            );
    }


}


void makeSomeTransportAddresses(NET_ADDR na[],
                                unsigned int numberOfMediaCandidates,
                                NET_PROTO eProto)
{
    unsigned int i;
    const char * addr[] = {
        "10.47.1.58:1234",
        "10.47.1.59:1235",
        "10.47.1.58:1236",
        "10.47.1.60:1237",
        "10.47.1.61:1238",
        "10.47.1.62:1239",
    };

    unittest_assert(
        numberOfMediaCandidates <= (sizeof(addr)/sizeof(addr[0])));

    for (i = 0; i < numberOfMediaCandidates; ++i) {
        NetAddr_initFromIPv4StringWithPort(&na[i], addr[i], eProto);
    }
}


void makeTransportAddresses(NET_ADDR na[],
                            const char * addr[],
                            unsigned int numberOfAddresses,
                            NET_PROTO eProto)
{
    unsigned int i;

    for (i = 0; i < numberOfAddresses; ++i) {
        NetAddr_initFromIPv4StringWithPort( &na[ i], addr[ i], eProto);
    }
}


void makeMediaStream( ICE_MEDIA_STREAM *pMediaStream, const char * addr[])
{
    NET_ADDR na[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];

    makeTransportAddresses( na,
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


void createTwoCheckLists( ICELIB_INSTANCE *pInstance)
{
    ICE_MEDIA               mediaLocal;
    ICE_MEDIA               mediaRemote;
    ICE_MEDIA_STREAM        mediaStreamLocal_0;
    ICE_MEDIA_STREAM        mediaStreamLocal_1;
    ICE_MEDIA_STREAM        mediaStreamRemote_0;
    ICE_MEDIA_STREAM        mediaStreamRemote_1;
    ICELIB_CONFIGURATION    config;

    const char * addrLocal_0[] = {
        "10.47.1.58:1234",
        "10.47.1.59:1235",
        "10.47.1.58:1236",
        "10.47.1.60:1237",
        "10.47.1.61:1238",
        "10.47.1.62:1239",
    };

    const char * addrLocal_1[] = {
        "10.47.2.58:1234",
        "10.47.2.59:1235",
        "10.47.2.58:1236",
        "10.47.2.60:1237",
        "10.47.2.61:1238",
        "10.47.2.62:1239",
    };

    const char * addrRemote_0[] = {
        "10.47.1.100:2000",
        "10.47.1.101:2001",
        "10.47.1.102:2002",
        "10.47.1.103:2003",
        "10.47.1.104:2004",
        "10.47.1.105:2005",
    };

    const char * addrRemote_1[] = {
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
    config.iceLite              = false;
    config.stoppingTimeoutS     = 5;
    config.logLevel             = UNITTEST_LOG_LEVEL;

    ICELIB_Constructor( pInstance, &config);
//
//- Create 4 media streams
//
    makeMediaStream( &mediaStreamLocal_0,  addrLocal_0);
    makeMediaStream( &mediaStreamLocal_1,  addrLocal_1);
    makeMediaStream( &mediaStreamRemote_0, addrRemote_0);
    makeMediaStream( &mediaStreamRemote_1, addrRemote_1);
//
//- Create local and remote ICE_MEDIA_STREAMs
//
    ICELIBTYPES_ICE_MEDIA_reset( &mediaLocal);
    ICELIBTYPES_ICE_MEDIA_reset( &mediaRemote);

    mediaLocal.mediaStream[ 0]        = mediaStreamLocal_0;
    mediaLocal.mediaStream[ 1]        = mediaStreamLocal_1;
    mediaLocal.numberOfICEMediaLines  = 2;

    mediaRemote.mediaStream[ 0]       = mediaStreamRemote_0;
    mediaRemote.mediaStream[ 1]       = mediaStreamRemote_1;
    mediaRemote.numberOfICEMediaLines = 2;
//
//- Add local and remote candidates
//
    // ICELIB_addLocalCandidates ( pInstance, &mediaLocal);
    // ICELIB_addRemoteCandidates( pInstance, &mediaRemote);

    ICELIB_addRemoteCandidates( pInstance, &mediaRemote, false, false);
    ICELIB_addLocalCandidates ( pInstance, &mediaLocal, false);

//  ICELIB_checkListDumpAll( pInstance);

}


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



//-----------------------------------------------------------------------------
//
// ICE Unit Tests
//

void unittest_iceufrag( void)
{
    char tmp1[ ICE_MAX_UFRAG_LENGTH];
    char tmp2[ ICE_MAX_UFRAG_LENGTH];

    unittest_context( "unittest_iceufrag");

    memset( tmp1, '#', sizeof( tmp1));
    memset( tmp2, '#', sizeof( tmp2));

    ICELIB_createUfrag( tmp1, ICELIB_UFRAG_LENGTH);
    ICELIB_createUfrag( tmp2, ICELIB_UFRAG_LENGTH);

    unittest_assert( isLegalString( tmp1));
    unittest_assert( isLegalString( tmp2));

    unittest_assert( ( strlen( tmp1) + 1) == ICELIB_UFRAG_LENGTH);
    unittest_assert( ( strlen( tmp2) + 1) == ICELIB_UFRAG_LENGTH);
    unittest_assert( strcmp( tmp1, tmp2));

    ICELIB_createUfrag( tmp1, ICE_MAX_UFRAG_LENGTH + 999);
    unittest_assert( ( strlen( tmp1) + 1) == ICE_MAX_UFRAG_LENGTH);

    unittest_assert( isLegalString( tmp1));
}


void unittest_icepasswd( void)
{
    char tmp1[ ICE_MAX_PASSWD_LENGTH];
    char tmp2[ ICE_MAX_PASSWD_LENGTH];
    unsigned int i;

    unittest_context( "unittest_icepasswd");

    memset( tmp1, '#', sizeof( tmp1));
    memset( tmp2, '#', sizeof( tmp2));

    ICELIB_createPasswd( tmp1, ICELIB_PASSWD_LENGTH);
    ICELIB_createPasswd( tmp2, ICELIB_PASSWD_LENGTH);

    unittest_assert( isLegalString( tmp1));
    unittest_assert( isLegalString( tmp2));

    unittest_assert( ( strlen( tmp1) + 1) == ICELIB_PASSWD_LENGTH);
    unittest_assert( ( strlen( tmp2) + 1) == ICELIB_PASSWD_LENGTH);
    unittest_assert( strcmp( tmp1, tmp2));

    memset( tmp2, '#', sizeof( tmp2));
    tmp2[ ICE_MAX_PASSWD_LENGTH-1] = '\0';

    for (i = 0; i < ICE_MAX_PASSWD_LENGTH + 10; ++i) {
        memset( tmp1, '#', sizeof( tmp1));
        tmp1[ICE_MAX_PASSWD_LENGTH - 1] = '\0';

        ICELIB_createPasswd(tmp1, i);

        if (i == 0) {
            unittest_assert( strcmp( tmp1, tmp2) == 0);
        }
        else if (i > ICE_MAX_PASSWD_LENGTH) {
            unittest_assert(isLegalString(tmp1));
            unittest_assert((strlen(tmp1) + 1) == ICE_MAX_PASSWD_LENGTH);
        } else {
            unittest_assert(isLegalString(tmp1));
            unittest_assert((strlen(tmp1) + 1) == i);
        }
    }
}


void unittest_icepriority( void)
{
    uint32_t priority1;
    uint32_t priority2;
    uint32_t priority3;
    uint32_t priority4;
    uint32_t priority5;
    uint32_t priority6;
    uint32_t priority7;
    uint32_t priority8;

    unittest_context( "unittest_icepriority");

    priority1 = ICELIB_calculatePriority( ICE_CAND_TYPE_HOST,  1);
    priority2 = ICELIB_calculatePriority( ICE_CAND_TYPE_HOST,  2);
    priority3 = ICELIB_calculatePriority( ICE_CAND_TYPE_SRFLX, 1);
    priority4 = ICELIB_calculatePriority( ICE_CAND_TYPE_SRFLX, 2);
    priority5 = ICELIB_calculatePriority( ICE_CAND_TYPE_RELAY, 1);
    priority6 = ICELIB_calculatePriority( ICE_CAND_TYPE_RELAY, 2);
    priority7 = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1);
    priority8 = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2);

    unittest_assert( priority1 == PRIORITY_HOST_1);
    unittest_assert( priority2 == PRIORITY_HOST_2);
    unittest_assert( priority3 == PRIORITY_SRFLX_1);
    unittest_assert( priority4 == PRIORITY_SRFLX_2);
    unittest_assert( priority5 == PRIORITY_RELAY_1);
    unittest_assert( priority6 == PRIORITY_RELAY_2);
    unittest_assert( priority7 == PRIORITY_PRFLX_1);
    unittest_assert( priority8 == PRIORITY_PRFLX_2);
}


void unittest_icefoundation( void)
{
    char tmp[ ICE_MAX_FOUNDATION_LENGTH];

    unittest_context( "unittest_icefoundation");

    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_HOST, ICE_MAX_FOUNDATION_LENGTH);

    unittest_assert( ( strlen( tmp) + 1) == ICELIB_FOUNDATION_LENGTH);
    unittest_assert( strcmp( tmp, FOUNDATION_HOST) == 0);

    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_SRFLX, ICE_MAX_FOUNDATION_LENGTH);

    unittest_assert( ( strlen( tmp) + 1) == ICELIB_FOUNDATION_LENGTH);
    unittest_assert( strcmp( tmp, FOUNDATION_SRFLX) == 0);

    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_RELAY, ICE_MAX_FOUNDATION_LENGTH);

    unittest_assert( ( strlen( tmp) + 1) == ICELIB_FOUNDATION_LENGTH);
    unittest_assert( strcmp( tmp, FOUNDATION_RELAY) == 0);

    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_PRFLX, ICE_MAX_FOUNDATION_LENGTH);

    unittest_assert( ( strlen( tmp) + 1) == ICELIB_FOUNDATION_LENGTH);
    unittest_assert( strcmp( tmp, FOUNDATION_PRFLX) == 0);

    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_HOST+999, ICE_MAX_FOUNDATION_LENGTH);

    unittest_assert( strlen( tmp) == strlen( "unknowntype"));
    unittest_assert( strcmp( tmp, "unknowntype") == 0);
}


void unittest_iceCreateIceMediaStream1( void)
{
    ICE_MEDIA_STREAM iceMediaStream;
    int    candidateIndex[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];
    NET_ADDR         *pNa[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];
    NET_ADDR           na[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];

    unittest_context( "unittest_iceCreateIceMediaStream1");

    makeSomeTransportAddresses( na, ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES, NET_PROTO_UDP);

    memset( &iceMediaStream, 0, sizeof( iceMediaStream));

    ICELIB_createIceMediaStream( &iceMediaStream,
                                 0, 1,
                                 &na[ 0],
                                 &na[ 1],
                                 &na[ 2],
                                 &na[ 3],
                                 &na[ 4],
                                 &na[ 5]);

    pNa[ 0] = &na[ 0];
    pNa[ 1] = &na[ 1];
    pNa[ 2] = &na[ 2];
    pNa[ 3] = &na[ 3];
    pNa[ 4] = &na[ 4];
    pNa[ 5] = &na[ 5];

    candidateIndex[ 0] = 0;
    candidateIndex[ 1] = 1;
    candidateIndex[ 2] = 2;
    candidateIndex[ 3] = 3;
    candidateIndex[ 4] = 4;
    candidateIndex[ 5] = 5;

    assertMediaStream( &iceMediaStream, 6, pNa, candidateIndex);
}


void unittest_iceCreateIceMediaStream2( void)
{
    int first;
    int second;
    int dupl;
    int nulltype;
    ICE_MEDIA_STREAM iceMediaStream;
    int    candidateIndex[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];
    NET_ADDR         *pNa[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];
    NET_ADDR           na[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];
    NET_ADDR         naReset;
    NET_ADDR         naAny;

    unittest_context( "unittest_iceCreateIceMediaStream2");

    makeSomeTransportAddresses( na, ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES, NET_PROTO_UDP);

    NetAddr_reset( &naReset);
    NetAddr_initAsAddrAny( &naAny, NET_ADDR_TYPE_IPV4);

//
//- Test with two different moving addresses, insert moving duplicate also
//
    for( first=0; first < ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES; ++first) {
        for( second=first+1; second < ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES; ++second) {
            for( dupl=0; dupl < ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES; ++dupl) {

                if( ( dupl == first) || ( dupl == second)) continue;

                for( nulltype=0; nulltype < 3; ++nulltype) {

                    switch( nulltype) {
                        case 0:
                            fillNetAddrPointerArray(
                                pNa,
                                NULL,
                                ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES);
                            break;
                        case 1:
                            fillNetAddrPointerArray(
                                pNa,
                                &naReset,
                                ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES);
                            break;
                        case 2:
                            fillNetAddrPointerArray(
                                pNa,
                                &naAny,
                                ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES);
                            break;
                    }

                    pNa[  first] = &na[  first];
                    pNa[ second] = &na[ second];
                    pNa[   dupl] = &na[  first];

                    memset( &iceMediaStream, 0, sizeof( iceMediaStream));
                    ICELIB_createIceMediaStream( &iceMediaStream,
                                                 0, 1,
                                                 pNa[ 0],
                                                 pNa[ 1],
                                                 pNa[ 2],
                                                 pNa[ 3],
                                                 pNa[ 4],
                                                 pNa[ 5]);

// printf( "[%d] [%d] [%d] ----------------------------\n", first, second, dupl);
// ICELIBTYPES_ICE_MEDIA_STREAM_dump( &iceMediaStream);


                    if( dupl > first) {
                        candidateIndex[ 0] = first;
                    } else {
                        candidateIndex[ 0] = dupl;
                    }

                    candidateIndex[ 1] = second;

                    assertMediaStream( &iceMediaStream, 2, pNa, candidateIndex);

                }
            }
        }
    }
}


void unittest_icePairPriority( void)
{
    uint32_t G;
    uint32_t D;
    uint64_t priority;

    unittest_context( "unittest_icePairPriority");

    G = 0x12345678;
    D = 0x76543210;

    priority = ICELIB_pairPriority( G, D);
    unittest_assert( priority == 0x12345678eca86420LL);

    G = 0x76543210;
    D = 0x12345678;

    priority = ICELIB_pairPriority( G, D);
    unittest_assert( priority == 0x12345678eca86421LL);

    G = 0x11111111;
    D = 0x22222222;

    priority = ICELIB_pairPriority( G, D);
    unittest_assert( priority == 0x1111111144444444LL);

    G = 0x22222222;
    D = 0x11111111;

    priority = ICELIB_pairPriority( G, D);
    unittest_assert( priority == 0x1111111144444445LL);
}


void unittest_iceFindBases( void)
{
    ICE_MEDIA_STREAM     mediaStreamLocal;
    NET_ADDR             na[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];
    const ICE_CANDIDATE *pBaseSrvRflxRtp;
    const ICE_CANDIDATE *pBaseSrvRflxRtcp;
    bool                 foundBases;

    const char * addrLocal[] = {
        "10.47.1.58:1234",
        "10.47.1.59:1235",
        "10.47.1.58:1236",
        "10.47.1.60:1237",
        "10.47.1.61:1238",
        "10.47.1.62:1239",
    };

    unittest_context( "unittest_iceFindBases");

    makeTransportAddresses( na,
                            addrLocal,
                            ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES,
                            NET_PROTO_UDP);

    ICELIB_createIceMediaStream( &mediaStreamLocal,
                                 0, 1,
                                 &na[ 0],
                                 &na[ 1],
                                 &na[ 2],
                                 &na[ 3],
                                 &na[ 4],
                                 &na[ 5]);

    foundBases = ICELIB_findReflexiveBaseAddresses( &pBaseSrvRflxRtp,
                                                    &pBaseSrvRflxRtcp,
                                                    &mediaStreamLocal);

    unittest_assert( foundBases == true);
    unittest_assert( pBaseSrvRflxRtp  == &mediaStreamLocal.candidate[ 0]);
    unittest_assert( pBaseSrvRflxRtcp == &mediaStreamLocal.candidate[ 1]);
}


void unittest_iceCheckListBasic( void)
{
    ICELIB_CHECKLIST    checkList;
    ICE_MEDIA_STREAM    mediaStreamLocal;
    ICE_MEDIA_STREAM    mediaStreamRemote;
    NET_ADDR            na[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];
    const ICE_CANDIDATE *pBaseSrvRflxRtp;
    const ICE_CANDIDATE *pBaseSrvRflxRtcp;
    bool                foundBases;
    ICELIB_CALLBACK_LOG     CallbackLog;

    const char * addrLocal[] = {
        "10.47.1.58:1234",
        "10.47.1.59:1235",
        "10.47.1.58:1236",
        "10.47.1.60:1237",
        "10.47.1.61:1238",
        "10.47.1.62:1239",
    };

    const char * addrRemote[] = {
        "10.47.1.100:2000",
        "10.47.1.101:2001",
        "10.47.1.102:2002",
        "10.47.1.103:2003",
        "10.47.1.104:2004",
        "10.47.1.105:2005",
    };

    unittest_context( "unittest_iceCheckListBasic");

//
//- Create 2 media streams
//
    makeTransportAddresses( na,
                            addrLocal,
                            ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES,
                            NET_PROTO_UDP);

    ICELIB_createIceMediaStream( &mediaStreamLocal,
                                 0, 1,
                                 &na[ 0],
                                 &na[ 1],
                                 &na[ 2],
                                 &na[ 3],
                                 &na[ 4],
                                 &na[ 5]);

    makeTransportAddresses( na,
                            addrRemote,
                            ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES,
                            NET_PROTO_UDP);

    ICELIB_createIceMediaStream( &mediaStreamRemote,
                                 0, 1,
                                 &na[ 0],
                                 &na[ 1],
                                 &na[ 2],
                                 &na[ 3],
                                 &na[ 4],
                                 &na[ 5]);
//
//- Create a basic check list (with pair priorities)
//
    ICELIB_resetCheckList( &checkList, 0);

    memset(&CallbackLog, 0, sizeof( CallbackLog));
    CallbackLog.pICELIB_logCallback = noPrintf;

    ICELIB_formPairs( &checkList, &CallbackLog, &mediaStreamLocal, &mediaStreamRemote, 50);
    ICELIB_computeListPairPriority( &checkList, true);

    unittest_assert( checkList.checkListState == ICELIB_CHECKLIST_IDLE);
    unittest_assert( checkList.numberOfPairs  == 18);

    unittest_assert( checkList.checkListPairs[ 0].pairState     == ICELIB_PAIR_PAIRED);
    unittest_assert( checkList.checkListPairs[ 0].pairPriority  == 0x7efffffffdfffffeLL);

    unittest_assert( checkList.checkListPairs[ 1].pairState     == ICELIB_PAIR_PAIRED);
    unittest_assert( checkList.checkListPairs[ 1].pairPriority  == 0x64fffffffdffffffLL);

    unittest_assert( checkList.checkListPairs[ 15].pairState    == ICELIB_PAIR_PAIRED);
    unittest_assert( checkList.checkListPairs[ 15].pairPriority == 0x00fffffefdfffffcLL);

    unittest_assert( checkList.checkListPairs[ 17].pairState    == ICELIB_PAIR_PAIRED);
    unittest_assert( checkList.checkListPairs[ 17].pairPriority == 0x00fffffe01fffffcLL);
//
//- Sort check list
//
    ICELIB_sortPairsCL( &checkList);

    unittest_assert( checkList.checkListState == ICELIB_CHECKLIST_IDLE);
    unittest_assert( checkList.numberOfPairs  == 18);

    unittest_assert( checkList.checkListPairs[ 0].pairState    == ICELIB_PAIR_PAIRED);
    unittest_assert( checkList.checkListPairs[ 0].pairPriority == 0x7efffffffdfffffeLL);

    unittest_assert( checkList.checkListPairs[ 1].pairState    == ICELIB_PAIR_PAIRED);
    unittest_assert( checkList.checkListPairs[ 1].pairPriority == 0x7efffffefdfffffcLL);

    unittest_assert( checkList.checkListPairs[ 15].pairState    == ICELIB_PAIR_PAIRED);
    unittest_assert( checkList.checkListPairs[ 15].pairPriority == 0x00fffffec9fffffdLL);

    unittest_assert( checkList.checkListPairs[ 17].pairState    == ICELIB_PAIR_PAIRED);
    unittest_assert( checkList.checkListPairs[ 17].pairPriority == 0x00fffffe01fffffcLL);
//
//- Prune check list
//
    foundBases = ICELIB_findReflexiveBaseAddresses( &pBaseSrvRflxRtp,
                                                    &pBaseSrvRflxRtcp,
                                                    &mediaStreamLocal);
    unittest_assert( foundBases == true);

    ICELIB_prunePairs( &checkList, pBaseSrvRflxRtp, pBaseSrvRflxRtcp);

    unittest_assert( checkList.numberOfPairs  == 12);

    unittest_assert( checkList.checkListPairs[  3].pairPriority == 0x64fffffefdfffffdLL);
    unittest_assert( checkList.checkListPairs[ 11].pairPriority == 0x00fffffe01fffffcLL);

}




void unittest_iceCheckListSingle( void)
{
    ICELIB_CHECKLIST        checkList;
    char                    foundation[ ICE_MAX_FOUNDATION_LENGTH];
    char                   *pFoundation;
    ICE_MEDIA_STREAM        mediaStreamLocal;
    ICE_MEDIA_STREAM        mediaStreamRemote;
    NET_ADDR                na[ ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES];
    ICELIB_LIST_PAIR        Pair;
    ICELIB_LIST_PAIR        *pPair;
    ICELIB_CALLBACK_LOG     CallbackLog;
    bool                    result;

    const char * addrLocal[] = {
        "10.47.1.58:1234",
        "10.47.1.59:1235",
        "10.47.1.58:1236",
        "10.47.1.60:1237",
        "10.47.1.61:1238",
        "10.47.1.62:1239",
    };

    const char * addrRemote[] = {
        "10.47.1.100:2000",
        "10.47.1.101:2001",
        "10.47.1.102:2002",
        "10.47.1.103:2003",
        "10.47.1.104:2004",
        "10.47.1.105:2005",
    };

    unittest_context( "unittest_iceCheckListSingle");

//
//- Create 2 media streams
//
    makeTransportAddresses( na,
                            addrLocal,
                            ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES,
                            NET_PROTO_UDP);

    ICELIB_createIceMediaStream( &mediaStreamLocal,
                                 0, 1,
                                 &na[ 0],
                                 &na[ 1],
                                 &na[ 2],
                                 &na[ 3],
                                 &na[ 4],
                                 &na[ 5]);

    makeTransportAddresses( na,
                            addrRemote,
                            ICE_MAX_NUMBER_OF_MEDIA_CANDIDATES,
                            NET_PROTO_UDP);

    ICELIB_createIceMediaStream( &mediaStreamRemote,
                                 0, 1,
                                 &na[ 0],
                                 &na[ 1],
                                 &na[ 2],
                                 &na[ 3],
                                 &na[ 4],
                                 &na[ 5]);
//
//- Create a single check list (sorted and pruned)
//
    memset( &CallbackLog, 0, sizeof( CallbackLog));
    CallbackLog.pICELIB_logCallback = noPrintf;
    ICELIB_makeCheckList( &checkList, &CallbackLog, &mediaStreamLocal, &mediaStreamRemote, true, 50, 0);

    unittest_assert( checkList.numberOfPairs  == 12);
    unittest_assert( checkList.checkListState == ICELIB_CHECKLIST_RUNNING);
    unittest_assert( ICELIB_isActiveCheckList( &checkList) == false);

    unittest_assert( checkList.checkListPairs[  0].pairPriority == 0x7efffffffdfffffeLL);
    unittest_assert( checkList.checkListPairs[  1].pairPriority == 0x7efffffefdfffffcLL);
    unittest_assert( checkList.checkListPairs[  2].pairPriority == 0x64fffffffdffffffLL);
    unittest_assert( checkList.checkListPairs[  3].pairPriority == 0x64fffffefdfffffdLL);

    unittest_assert( checkList.checkListPairs[  8].pairPriority == 0x00fffffefdfffffdLL);
    unittest_assert( checkList.checkListPairs[  9].pairPriority == 0x00fffffefdfffffcLL);
    unittest_assert( checkList.checkListPairs[ 10].pairPriority == 0x00fffffec9fffffcLL);
    unittest_assert( checkList.checkListPairs[ 11].pairPriority == 0x00fffffe01fffffcLL);

    pFoundation = ICELIB_getPairFoundation( foundation,
                                            ICE_MAX_FOUNDATION_LENGTH,
                                            &checkList.checkListPairs[  0]);
    unittest_assert( strcmp( pFoundation, "11") == 0);

    pFoundation = ICELIB_getPairFoundation( foundation,
                                            ICE_MAX_FOUNDATION_LENGTH,
                                            &checkList.checkListPairs[  2]);
    unittest_assert( strcmp( pFoundation, "31") == 0);

    pFoundation = ICELIB_getPairFoundation( foundation,
                                            ICE_MAX_FOUNDATION_LENGTH,
                                            &checkList.checkListPairs[  5]);
    unittest_assert( strcmp( pFoundation, "14") == 0);

    pFoundation = ICELIB_getPairFoundation( foundation,
                                            ICE_MAX_FOUNDATION_LENGTH,
                                            &checkList.checkListPairs[  10]);
    unittest_assert( strcmp( pFoundation, "34") == 0);
//
//- Set inital states as a first media stream (WAITING and FROZEN)
//
    ICELIB_computeStatesSetWaitingFrozen( &checkList);

    unittest_assert( ICELIB_isActiveCheckList( &checkList) == true);

    unittest_assert( checkList.checkListPairs[  0].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  1].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[  2].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  3].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[  4].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  5].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  6].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  7].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  8].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[  9].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[ 10].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[ 11].pairState == ICELIB_PAIR_FROZEN);
//
//- Insert a new pair, use a priority that makes it end up at index 1
//
    Pair.pairState        = ICELIB_PAIR_PAIRED;
    Pair.defaultPair      = 0;
    Pair.nominatedPair    = 0;
    Pair.pairPriority     = 0x7efffffffdfffffdLL;
    Pair.pLocalCandidate  = checkList.checkListPairs[ 11].pLocalCandidate;
    Pair.pRemoteCandidate = checkList.checkListPairs[ 11].pRemoteCandidate;

    result = ICELIB_insertIntoCheckList( &checkList, &Pair);
    unittest_assert( result == false);

    unittest_assert( checkList.numberOfPairs  == 13);

    unittest_assert( checkList.checkListPairs[  0].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  1].pairState == ICELIB_PAIR_PAIRED);
    unittest_assert( checkList.checkListPairs[  2].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[  3].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  4].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[  5].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  6].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  7].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  8].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( checkList.checkListPairs[  9].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[ 10].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[ 11].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( checkList.checkListPairs[ 12].pairState == ICELIB_PAIR_FROZEN);  
//
//- Choose/find pair
//
    pPair = pICELIB_findPairByState( &checkList, ICELIB_PAIR_WAITING);
    unittest_assert( pPair == &checkList.checkListPairs[  0]);
    pPair = pICELIB_findPairByState( &checkList, ICELIB_PAIR_PAIRED);
    unittest_assert( pPair == &checkList.checkListPairs[  1]);
    pPair = pICELIB_findPairByState( &checkList, ICELIB_PAIR_FROZEN);
    unittest_assert( pPair == &checkList.checkListPairs[  2]);

    pPair = pICELIB_chooseOrdinaryPair( &checkList);
    unittest_assert( pPair == &checkList.checkListPairs[  0]);

}


ICELIB_Result sendBindingRequest_( void                *pUserData,
                                   const NET_ADDR      *destination,
                                   const NET_ADDR      *source,
                                   uint32_t             userValue1,
                                   uint32_t             userValue2,
                                   uint32_t             componentId,
                                   bool                 useRelay,
                                   const char          *pUfragPair,
                                   const char          *pPasswd,
                                   uint32_t             peerPriority,
                                   bool                 useCandidate,
                                   bool                 iceControlling,
                                   uint64_t             tieBreaker,
                                   StunMsgId            transactionId)
{
    BindingRequestCallbackData *pbrData;

    pbrData = ( BindingRequestCallbackData *)pUserData;

    unittest_assert( destination == pbrData->destination);
    unittest_assert( source == pbrData->source);
    unittest_assert( userValue1 == pbrData->userValue1);
    unittest_assert( userValue2 == pbrData->userValue2);
    unittest_assert( componentId == pbrData->componentId);
    unittest_assert( useRelay == pbrData->useRelay);
    unittest_assert( strcmp( pUfragPair, pbrData->pUfragPair) == 0);
    unittest_assert( strcmp( pPasswd, pbrData->pPasswd) == 0);
    unittest_assert( peerPriority == pbrData->peerPriority);
    unittest_assert( useCandidate == pbrData->useCandidate);
    unittest_assert( iceControlling == pbrData->iceControlling);
    unittest_assert( tieBreaker == pbrData->tieBreaker);

    pbrData->callbackCalled = true;

    return ICELIB_Result_OK;
}




ICELIB_Result sendBindingResponse_( void               *pUserData,
                                    uint32_t            userValue1,
                                    uint32_t            userValue2,
                                    const NET_ADDR     *source,
                                    const NET_ADDR     *destination,
                                    const NET_ADDR     *MappedAddress,
                                    uint16_t            errorResponse,
                                    StunMsgId           transactionId,
                                    bool                useRelay,
                                    const char         *pUfragPair,
                                    const char         *pPasswd)

{
    BindingResponseCallbackData *pbrData;

    pbrData = ( BindingResponseCallbackData *)pUserData;

    unittest_assert( userValue1 == pbrData->userValue1);
    unittest_assert( userValue2 == pbrData->userValue2);

    unittest_assert( NetAddr_alike( source, pbrData->source) == true);
    unittest_assert( NetAddr_alike( destination, pbrData->destination) == true);
    unittest_assert( NetAddr_alike( MappedAddress, pbrData->MappedAddress) == true);

    unittest_assert( errorResponse == pbrData->errorResponse);
    unittest_assert( memcmp( &transactionId, &pbrData->transactionId, STUN_MSG_ID_SIZE) == 0);

/*
    printf( "\nsendBindingResponse_:\n");
    printf( "source: ");
    NetAddr_dump( source, true);
    printf( "\n");
    printf( "destination: ");
    NetAddr_dump( destination, true);
    printf( "\n");
    printf( "MappedAddress: ");
    NetAddr_dump( MappedAddress, true);
    printf( "\n");
    printf( "errorResponse: %u\n", errorResponse);
    printf( "transactionId: ");
    ICELIB_transactionIdDump( transactionId);
    printf( "\n");
*/

    pbrData->callbackCalled = true;

    return ICELIB_Result_OK;
}


void unittest_iceTriggeredCheckQueue1( void)
{
    bool                    result;
    unsigned int            i;
    unsigned int            j;
    unsigned int            k;
    ICELIB_FIFO_ELEMENT     element;
    ICELIB_TRIGGERED_FIFO   f;

    unittest_context( "unittest_iceTriggeredCheckQueue1");

    ICELIB_fifoClear( &f);
    unittest_assert( ICELIB_fifoCount( &f) == 0);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == true);
    unittest_assert( ICELIB_fifoIsFull( &f) == false);

    unittest_assert( ICELIB_fifoGet( &f) == ICELIB_FIFO_IS_EMPTY);
//
//----- Run FIFO full
// 
    for( i=0; i < ICELIB_MAX_FIFO_ELEMENTS; ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i);
        unittest_assert( result == false);
        unittest_assert( ICELIB_fifoCount( &f) == i + 1);
    }

    unittest_assert( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f) == true);

    unittest_assert( ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) 999) == true);
//
//----- Remove some elements
// 
    for( i=0; i < 10; ++i) {
        element = ICELIB_fifoGet( &f);
        unittest_assert( element == ( ICELIB_FIFO_ELEMENT) i);
        unittest_assert( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS - i - 1);
    }

    unittest_assert( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS - 10);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f) == false);
//
//----- Remove more elements
// 
    for( i=10; i < ICELIB_MAX_FIFO_ELEMENTS - 5; ++i) {
        element = ICELIB_fifoGet( &f);
        unittest_assert( element == ( ICELIB_FIFO_ELEMENT) i);
        unittest_assert( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS - i - 1);
    }

    unittest_assert( ICELIB_fifoCount( &f) == 5);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f) == false);
//
//----- Remove the rest of the elements
// 
    for( i = ICELIB_MAX_FIFO_ELEMENTS - 5; i < ICELIB_MAX_FIFO_ELEMENTS; ++i) {
        element = ICELIB_fifoGet( &f);
        unittest_assert( element == ( ICELIB_FIFO_ELEMENT) i);
        unittest_assert( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS - i - 1);
    }

    unittest_assert( ICELIB_fifoCount( &f) == 0);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == true);
    unittest_assert( ICELIB_fifoIsFull( &f) == false);
//
//----- Fill 3/4 of the FIFO
// 
    for( i=0; i < ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4; ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        unittest_assert( result == false);
        unittest_assert( ICELIB_fifoCount( &f) == i + 1);
    }

    unittest_assert( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f) == false);
//
//----- Remove 1/2 FIFO
// 
    for( j=0; j < ICELIB_MAX_FIFO_ELEMENTS / 2; ++j) {
        element = ICELIB_fifoGet( &f);
        unittest_assert( element == ( ICELIB_FIFO_ELEMENT) j + 1000);
        unittest_assert( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4 - j - 1);
    }

    unittest_assert( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS / 4);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f) == false);
//
//----- Fill another 1/2 FIFO
// 
    for( k=0; k < ICELIB_MAX_FIFO_ELEMENTS / 2; ++k, ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        unittest_assert( result == false);
        unittest_assert( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS) / 4 + k + 1);
    }

    unittest_assert( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f) == false);
//
//----- Remove 3/4 FIFO
// 
    for( k=0; k < ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4; ++k, ++j) {
        element = ICELIB_fifoGet( &f);
        unittest_assert( element == ( ICELIB_FIFO_ELEMENT) j + 1000);
        unittest_assert( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4 - k - 1);
    }

    unittest_assert( ICELIB_fifoCount( &f) == 0);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == true);
    unittest_assert( ICELIB_fifoIsFull( &f) == false);
}


void unittest_iceTriggeredCheckQueue2( void)
{
    bool                            result;
    unsigned int                    i;
    unsigned int                    j;
    unsigned int                    k;
    unsigned int                    l;
    ICELIB_FIFO_ELEMENT             element;
    ICELIB_FIFO_ELEMENT             *pElement;
    ICELIB_TRIGGERED_FIFO           f;
    ICELIB_TRIGGERED_FIFO_ITERATOR  tfIterator;

    unittest_context( "unittest_iceTriggeredCheckQueue2");

    ICELIB_fifoClear( &f);
    unittest_assert( ICELIB_fifoCount( &f) == 0);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == true);
    unittest_assert( ICELIB_fifoIsFull( &f) == false);
//
//----- Fill 3/4 of the FIFO
// 
    for( i=0; i < ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4; ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        unittest_assert( result == false);
        unittest_assert( ICELIB_fifoCount( &f) == i + 1);
    }

    unittest_assert( ICELIB_fifoCount( &f)   == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f)  == false);
//
//----- Iterate through elements in FIFO
// 
    ICELIB_fifoIteratorConstructor( &tfIterator, &f);

    j = 0;
    while( ( pElement = pICELIB_fifoIteratorNext( &tfIterator)) != NULL) {
        unittest_assert( *pElement == ( ICELIB_FIFO_ELEMENT) j + 1000);
        ++j;
    }

    unittest_assert( j == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
//
//----- Remove 1/2 FIFO
// 
    for( j=0; j < ICELIB_MAX_FIFO_ELEMENTS / 2; ++j) {
        element = ICELIB_fifoGet( &f);
        unittest_assert( element == ( ICELIB_FIFO_ELEMENT) j + 1000);
        unittest_assert( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4 - j - 1);
    }

    unittest_assert( ICELIB_fifoCount( &f)   == ICELIB_MAX_FIFO_ELEMENTS / 4);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f)  == false);
//
//----- Fill another 1/2 FIFO
// 
    for( k=0; k < ICELIB_MAX_FIFO_ELEMENTS / 2; ++k, ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        unittest_assert( result == false);
        unittest_assert( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS) / 4 + k + 1);
    }

    unittest_assert( ICELIB_fifoCount( &f)   == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f)  == false);
//
//----- Iterate through elements in FIFO
// 
    ICELIB_fifoIteratorConstructor( &tfIterator, &f);

    k = 0;
    l = j;
    while( ( pElement = pICELIB_fifoIteratorNext( &tfIterator)) != NULL) {
        unittest_assert( *pElement == ( ICELIB_FIFO_ELEMENT) l + 1000);
        ++k;
        ++l;
    }

    unittest_assert( k == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
//
//----- Fill another 1/4 FIFO (should then be full)
// 
    for( k=0; k < ICELIB_MAX_FIFO_ELEMENTS / 4; ++k, ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        unittest_assert( result == false);
        unittest_assert( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4 + k + 1);
    }

    unittest_assert( ICELIB_fifoCount( &f)   == ICELIB_MAX_FIFO_ELEMENTS);
    unittest_assert( ICELIB_fifoIsEmpty( &f) == false);
    unittest_assert( ICELIB_fifoIsFull( &f)  == true);
//
//----- Iterate through elements in FIFO
// 
    ICELIB_fifoIteratorConstructor( &tfIterator, &f);

    k = 0;
    l = j;
    while( ( pElement = pICELIB_fifoIteratorNext( &tfIterator)) != NULL) {
        unittest_assert( *pElement == ( ICELIB_FIFO_ELEMENT) l + 1000);
        ++k;
        ++l;
    }

    unittest_assert( k == ICELIB_MAX_FIFO_ELEMENTS);
}


void unittest_iceCheckList( void)
{
    bool                    checkWasSceduled;
    bool                    pairNotInFifo;
    ICELIB_INSTANCE         Instance;
    char                    ufragPair[ ICE_MAX_UFRAG_PAIR_LENGTH];
    ICELIB_CHECKLIST        *pCheckList;
    ICELIB_TRIGGERED_FIFO   *pFifo;
    ICELIB_STREAM_CONTROLLER   *pStreamController0;
    ICELIB_STREAM_CONTROLLER   *pStreamController1;
    BindingRequestCallbackData brData;

    unittest_context( "unittest_iceCheckList");

    createTwoCheckLists( &Instance);

    unittest_assert( Instance.iceControlling == false);

    pStreamController0 = &Instance.streamControllers[ 0];
    pStreamController1 = &Instance.streamControllers[ 1];

//
//- First list should be active, the second should be frozen
// 
    unittest_assert( ICELIB_isActiveCheckList( &pStreamController0->checkList) == true);
    unittest_assert( ICELIB_isFrozenCheckList( &pStreamController1->checkList) == true);

//
//- Trigger connectivity callbacks and check transported parameters for stream 0 check list
//
    brData.pUfragPair     = ICELIB_getCheckListUsernamePair( ufragPair, ICE_MAX_UFRAG_PAIR_LENGTH, &pStreamController0->checkList);
    brData.pPasswd        = ICELIB_getCheckListPasswd( &pStreamController0->checkList);
    brData.useCandidate   = false;
    brData.iceControlling = Instance.iceControlling;
    brData.tieBreaker     = Instance.tieBreaker;

    ICELIB_setCallbackOutgoingBindingRequest( &Instance, sendBindingRequest_, &brData);

    pCheckList = &pStreamController0->checkList;
    pFifo      = &pStreamController0->triggeredChecksFifo;

    fprintf(stderr, "-- pCheckList->checkListPairs[ 0].pLocalCandidate %08x\n", (uint32_t)pCheckList->checkListPairs[ 0].pLocalCandidate);

    brData.destination    = &pCheckList->checkListPairs[ 0].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 0].pLocalCandidate->connectionAddr;
    brData.userValue1     = 0;
    brData.userValue2     = 1;
    brData.componentId    = pCheckList->checkListPairs[ 0].pLocalCandidate->componentid;
    brData.useRelay       = false;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);

    brData.destination    = &pCheckList->checkListPairs[ 2].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 2].pLocalCandidate->connectionAddr;
    brData.userValue1     = 0;
    brData.userValue2     = 1;
    brData.componentId    = pCheckList->checkListPairs[ 2].pLocalCandidate->componentid;
    brData.useRelay       = false;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);

    brData.destination    = &pCheckList->checkListPairs[ 4].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 4].pLocalCandidate->connectionAddr;
    brData.userValue1     = 4;
    brData.userValue2     = 5;
    brData.componentId    = pCheckList->checkListPairs[ 4].pLocalCandidate->componentid;
    brData.useRelay       = true;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);

    brData.destination    = &pCheckList->checkListPairs[ 5].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 5].pLocalCandidate->connectionAddr;
    brData.userValue1     = 0;
    brData.userValue2     = 1;
    brData.componentId    = pCheckList->checkListPairs[ 5].pLocalCandidate->componentid;
    brData.useRelay       = false;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);

    brData.destination    = &pCheckList->checkListPairs[ 6].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 6].pLocalCandidate->connectionAddr;
    brData.userValue1     = 4;
    brData.userValue2     = 5;
    brData.componentId    = pCheckList->checkListPairs[ 6].pLocalCandidate->componentid;
    brData.useRelay       = true;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);

    brData.destination    = &pCheckList->checkListPairs[ 7].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 7].pLocalCandidate->connectionAddr;
    brData.userValue1     = 4;
    brData.userValue2     = 5;
    brData.componentId    = pCheckList->checkListPairs[ 7].pLocalCandidate->componentid;
    brData.useRelay       = true;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);

    brData.destination    = &pCheckList->checkListPairs[ 1].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 1].pLocalCandidate->connectionAddr;
    brData.userValue1     = 1;
    brData.userValue2     = 2;
    brData.componentId    = pCheckList->checkListPairs[ 1].pLocalCandidate->componentid;
    brData.useRelay       = false;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);

    brData.destination    = &pCheckList->checkListPairs[ 3].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 3].pLocalCandidate->connectionAddr;
    brData.userValue1     = 1;
    brData.userValue2     = 2;
    brData.componentId    = pCheckList->checkListPairs[ 3].pLocalCandidate->componentid;
    brData.useRelay       = false;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);
//
//----- Now insert a pair (use number 11) in the triggered check queue/fifo !
// 
    pairNotInFifo = ICELIB_triggeredFifoPut( pFifo, &pCheckList->checkListPairs[ 11]);
    unittest_assert( pairNotInFifo == false);
//
//----- The next scheduled pair must now be number 11
// 
    brData.destination    = &pCheckList->checkListPairs[ 11].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 11].pLocalCandidate->connectionAddr;
    brData.userValue1     = 5;
    brData.userValue2     = 6;
    brData.componentId    = pCheckList->checkListPairs[ 11].pLocalCandidate->componentid;
    brData.useRelay       = true;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);
//
//----- Continue with ordinary pairs
// 
    brData.destination    = &pCheckList->checkListPairs[ 8].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 8].pLocalCandidate->connectionAddr;
    brData.userValue1     = 5;
    brData.userValue2     = 6;
    brData.componentId    = pCheckList->checkListPairs[ 8].pLocalCandidate->componentid;
    brData.useRelay       = true;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);

    brData.destination    = &pCheckList->checkListPairs[ 9].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 9].pLocalCandidate->connectionAddr;
    brData.userValue1     = 1;
    brData.userValue2     = 2;
    brData.componentId    = pCheckList->checkListPairs[ 9].pLocalCandidate->componentid;
    brData.useRelay       = false;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);

    brData.destination    = &pCheckList->checkListPairs[ 10].pRemoteCandidate->connectionAddr;
    brData.source         = &pCheckList->checkListPairs[ 10].pLocalCandidate->connectionAddr;
    brData.userValue1     = 5;
    brData.userValue2     = 6;
    brData.componentId    = pCheckList->checkListPairs[ 10].pLocalCandidate->componentid;
    brData.useRelay       = true;
    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2);
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == true);
    unittest_assert( brData.callbackCalled == true);
//
//----- The last pair (number 11) has already been scheduled (via the triggered
//      checks queue. Do not try to schedule it again.
// 
//    brData.destination    = &pCheckList->checkListPairs[ 11].pRemoteCandidate->connectionAddr;
//    brData.useRelay       = true;
//    brData.peerPriority   = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2);
//    brData.callbackCalled = false;
//    checkWasSceduled = ICELIB_scheduleSingle( &Instance, &Instance.streamController[ 0]);
//    unittest_assert( checkWasSceduled == true);
//    unittest_assert( brData.callbackCalled == true);
//
//- The check list should now have all pairs in the ICELIB_PAIR_INPROGRESS state
//  Nothing more to do...
// 
    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == false);
    unittest_assert( brData.callbackCalled == false);

    brData.callbackCalled = false;
    checkWasSceduled = ICELIB_scheduleSingle( &Instance, pStreamController0);
    unittest_assert( checkWasSceduled == false);
    unittest_assert( brData.callbackCalled == false);

//    ICELIB_checkListDumpAll( &Instance);

//
//- Unfreeze stream 1 check list 
// 
    pCheckList = &pStreamController1->checkList;

    ICELIB_unfreezeFrozenCheckList( pCheckList);
    unittest_assert( ICELIB_isActiveCheckList( pCheckList) == true);

    unittest_assert( pCheckList->checkListPairs[  0].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( pCheckList->checkListPairs[  1].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( pCheckList->checkListPairs[  2].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( pCheckList->checkListPairs[  3].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( pCheckList->checkListPairs[  4].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( pCheckList->checkListPairs[  5].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( pCheckList->checkListPairs[  6].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( pCheckList->checkListPairs[  7].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( pCheckList->checkListPairs[  8].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( pCheckList->checkListPairs[  9].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( pCheckList->checkListPairs[ 10].pairState == ICELIB_PAIR_FROZEN);
    unittest_assert( pCheckList->checkListPairs[ 11].pairState == ICELIB_PAIR_FROZEN);
//
//- Unfreeze one more pair
// 
    ICELIB_unfreezePairsByFoundation( pCheckList, "44");
    unittest_assert( pCheckList->checkListPairs[ 11].pairState == ICELIB_PAIR_WAITING);

//    ICELIB_checkListDumpAll( &Instance);

}


void unittest_iceCorrelate( void)
{
    unsigned int                StreamIndex;
    ICELIB_INSTANCE             Instance;
    ICELIB_LIST_PAIR           *pPair;
    ICELIB_CHECKLIST           *pCheckList;
    ICELIB_TRIGGERED_FIFO      *pFifo;
    ICELIB_STREAM_CONTROLLER   *pStreamController0;
    ICELIB_STREAM_CONTROLLER   *pStreamController1;
    StunMsgId                   TransactionId;

    unittest_context( "unittest_iceCorrelate");

    createTwoCheckLists( &Instance);

    unittest_assert( Instance.iceControlling == false);

    pStreamController0 = &Instance.streamControllers[ 0];
    pStreamController1 = &Instance.streamControllers[ 1];
    pCheckList         = &pStreamController0->checkList;
    pFifo              = &pStreamController0->triggeredChecksFifo;

//
//- First list should be active, the second should be frozen
// 
    unittest_assert( ICELIB_isActiveCheckList( &pStreamController0->checkList) == true);
    unittest_assert( ICELIB_isFrozenCheckList( &pStreamController1->checkList) == true);
//
//- Schedule a couple of checks (pair 0 and 2 will go to IN PROGRESS)
//
    ICELIB_Tick( &Instance);
    ICELIB_Tick( &Instance);

    unittest_assert( pCheckList->checkListPairs[ 0].pairState == ICELIB_PAIR_INPROGRESS);
    unittest_assert( pCheckList->checkListPairs[ 2].pairState == ICELIB_PAIR_INPROGRESS);
//
//- Find scheduled pair by Transaction ID
// 
    TransactionId = pCheckList->checkListPairs[ 0].transactionIdTable[0];  // TBD PTM 
    pPair = pICELIB_correlateToRequest( &StreamIndex, &Instance, &TransactionId);

    unittest_assert( StreamIndex == 0);
    unittest_assert( &pCheckList->checkListPairs[ 0] == pPair);

    TransactionId = pCheckList->checkListPairs[ 2].transactionIdTable[0];  // TBD PTM
    pPair = pICELIB_correlateToRequest( &StreamIndex, &Instance, &TransactionId);

    unittest_assert( StreamIndex == 0);
    unittest_assert( &pCheckList->checkListPairs[ 2] == pPair);
//
//- Try an unknown transaction ID also
// 
    ++TransactionId.octet[ 0];
    pPair = pICELIB_correlateToRequest( &StreamIndex, &Instance, &TransactionId);

    unittest_assert( NULL == pPair);
}


void unittest_iceResponseError( void)
{
    bool                        iceControlling;
    unsigned int                StreamIndex;
    ICELIB_INSTANCE             Instance;
    ICELIB_LIST_PAIR           *pPair;
    ICELIB_CHECKLIST           *pCheckList0;
    ICELIB_CHECKLIST           *pCheckList1;
    ICELIB_TRIGGERED_FIFO      *pFifo;
    ICELIB_STREAM_CONTROLLER   *pStreamController0;
    ICELIB_STREAM_CONTROLLER   *pStreamController1;
    StunMsgId                   TransactionId;
    NET_ADDR                    sourceAddr;
    NET_ADDR                    destinationAddr;
    NET_ADDR                    mappedAddr;

    unittest_context( "unittest_iceResponseError");

    createTwoCheckLists( &Instance);

    unittest_assert( Instance.iceControlling == false);

    pStreamController0 = &Instance.streamControllers[ 0];
    pStreamController1 = &Instance.streamControllers[ 1];
    pCheckList0        = &pStreamController0->checkList;
    pCheckList1        = &pStreamController1->checkList;
    pFifo              = &pStreamController0->triggeredChecksFifo;

//
//- First list should be active, the second should be frozen
// 
    unittest_assert( ICELIB_isActiveCheckList( &pStreamController0->checkList) == true);
    unittest_assert( ICELIB_isFrozenCheckList( &pStreamController1->checkList) == true);
//
//- Schedule a couple of checks (pair 0 and 2 will go to IN PROGRESS)
//
    ICELIB_Tick( &Instance);
    ICELIB_Tick( &Instance);

    unittest_assert( pCheckList0->checkListPairs[ 0].pairState == ICELIB_PAIR_INPROGRESS);
    unittest_assert( pCheckList0->checkListPairs[ 2].pairState == ICELIB_PAIR_INPROGRESS);
//
//- Verify inital order in the check lists
// 
    unittest_assert( pCheckList0->checkListPairs[  0].pairId ==  1);
    unittest_assert( pCheckList0->checkListPairs[  1].pairId ==  4);
    unittest_assert( pCheckList0->checkListPairs[  2].pairId ==  2);
    unittest_assert( pCheckList0->checkListPairs[  3].pairId ==  5);
    unittest_assert( pCheckList0->checkListPairs[  4].pairId == 13);
    unittest_assert( pCheckList0->checkListPairs[  5].pairId ==  3);
    unittest_assert( pCheckList0->checkListPairs[  6].pairId == 14);
    unittest_assert( pCheckList0->checkListPairs[  7].pairId == 15);
    unittest_assert( pCheckList0->checkListPairs[  8].pairId == 16);
    unittest_assert( pCheckList0->checkListPairs[  9].pairId ==  6);
    unittest_assert( pCheckList0->checkListPairs[ 10].pairId == 17);
    unittest_assert( pCheckList0->checkListPairs[ 11].pairId == 18);

    unittest_assert( pCheckList1->checkListPairs[  0].pairId ==  1);
    unittest_assert( pCheckList1->checkListPairs[  1].pairId ==  4);
    unittest_assert( pCheckList1->checkListPairs[  2].pairId ==  2);
    unittest_assert( pCheckList1->checkListPairs[  3].pairId ==  5);
    unittest_assert( pCheckList1->checkListPairs[  4].pairId == 13);
    unittest_assert( pCheckList1->checkListPairs[  5].pairId ==  3);
    unittest_assert( pCheckList1->checkListPairs[  6].pairId == 14);
    unittest_assert( pCheckList1->checkListPairs[  7].pairId == 15);
    unittest_assert( pCheckList1->checkListPairs[  8].pairId == 16);
    unittest_assert( pCheckList1->checkListPairs[  9].pairId ==  6);
    unittest_assert( pCheckList1->checkListPairs[ 10].pairId == 17);
    unittest_assert( pCheckList1->checkListPairs[ 11].pairId == 18);
//
//- Simulate an incomming Binding Response with 487 (Role Confict) error response
//
    TransactionId = pCheckList0->checkListPairs[ 0].transactionIdTable[0];  // TBD PTM
    pPair = pICELIB_correlateToRequest( &StreamIndex, &Instance, &TransactionId);

    unittest_assert( &pCheckList0->checkListPairs[ 0] == pPair);

    NetAddr_initFromIPv4StringWithPort( &sourceAddr,      "10.47.3.58:5678", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &destinationAddr, "10.47.4.58:5678", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &mappedAddr,      "10.47.5.58:5678", NET_PROTO_UDP);

    iceControlling = Instance.iceControlling;

//    TransactionId.octet[ 1] = 0;

    ICELIB_incommingBindingResponse( &Instance,
                                     487,
                                     TransactionId,
                                     &sourceAddr,
                                     &destinationAddr,
                                     &mappedAddr);

    unittest_assert( Instance.iceControlling == ! iceControlling);
    unittest_assert( pPair->pairState == ICELIB_PAIR_WAITING);
    unittest_assert( ICELIB_triggeredFifoCount( pFifo) == 1);

//
//- Verify that a change of priorities has rearanged the check lists a little
//
    unittest_assert( pCheckList0->checkListPairs[  0].pairId ==  1);
    unittest_assert( pCheckList0->checkListPairs[  1].pairId ==  4);
    unittest_assert( pCheckList0->checkListPairs[  2].pairId ==  2);
    unittest_assert( pCheckList0->checkListPairs[  3].pairId ==  5);
    unittest_assert( pCheckList0->checkListPairs[  4].pairId ==  3); // swapped!
    unittest_assert( pCheckList0->checkListPairs[  5].pairId == 13); // swapped!
    unittest_assert( pCheckList0->checkListPairs[  6].pairId == 14);
    unittest_assert( pCheckList0->checkListPairs[  7].pairId == 15);
    unittest_assert( pCheckList0->checkListPairs[  8].pairId ==  6); // swapped!
    unittest_assert( pCheckList0->checkListPairs[  9].pairId == 16); // swapped!
    unittest_assert( pCheckList0->checkListPairs[ 10].pairId == 17);
    unittest_assert( pCheckList0->checkListPairs[ 11].pairId == 18);
   
    unittest_assert( pCheckList1->checkListPairs[  0].pairId ==  1);
    unittest_assert( pCheckList1->checkListPairs[  1].pairId ==  4);
    unittest_assert( pCheckList1->checkListPairs[  2].pairId ==  2);
    unittest_assert( pCheckList1->checkListPairs[  3].pairId ==  5);
    unittest_assert( pCheckList1->checkListPairs[  4].pairId ==  3); // swapped!
    unittest_assert( pCheckList1->checkListPairs[  5].pairId == 13); // swapped!
    unittest_assert( pCheckList1->checkListPairs[  6].pairId == 14);
    unittest_assert( pCheckList1->checkListPairs[  7].pairId == 15);
    unittest_assert( pCheckList1->checkListPairs[  8].pairId ==  6); // swapped!
    unittest_assert( pCheckList1->checkListPairs[  9].pairId == 16); // swapped!
    unittest_assert( pCheckList1->checkListPairs[ 10].pairId == 17);
    unittest_assert( pCheckList1->checkListPairs[ 11].pairId == 18);

    //- Repair state
    Instance.iceControlling = true;
    pPair->pairState        = ICELIB_PAIR_INPROGRESS;
    ICELIB_triggeredFifoClear( pFifo);

//
//- Simulate an incomming Binding Response address error
//
    ICELIB_incommingBindingResponse( &Instance,
                                     200,
                                     TransactionId,
                                     &sourceAddr,
                                     &destinationAddr,
                                     &mappedAddr);

    unittest_assert( pPair->pairState == ICELIB_PAIR_FAILED);

    //- Repair state
    pPair->pairState = ICELIB_PAIR_INPROGRESS;

}


void unittest_iceResponseOK( void)
{
    unsigned int                StreamIndex;
    ICE_CANDIDATE              *pCandidate;
    ICELIB_INSTANCE             Instance;
    ICELIB_LIST_PAIR           *pPair;
    ICELIB_CHECKLIST           *pCheckList0;
    ICELIB_CHECKLIST           *pCheckList1;
    ICELIB_VALIDLIST           *pValidList0;
    ICELIB_VALIDLIST           *pValidList1;
    ICELIB_TRIGGERED_FIFO      *pFifo;
    ICELIB_STREAM_CONTROLLER   *pStreamController0;
    ICELIB_STREAM_CONTROLLER   *pStreamController1;
    StunMsgId                   TransactionId;
    NET_ADDR                    mappedAddr;

    unittest_context( "unittest_iceResponseOK");

    createTwoCheckLists( &Instance);

    unittest_assert( Instance.iceControlling == false);

    pStreamController0 = &Instance.streamControllers[ 0];
    pStreamController1 = &Instance.streamControllers[ 1];
    pCheckList0        = &pStreamController0->checkList;
    pCheckList1        = &pStreamController1->checkList;
    pValidList0        = &pStreamController0->validList;
    pValidList1        = &pStreamController1->validList;
    pFifo              = &pStreamController0->triggeredChecksFifo;

//
//- First list should be active, the second should be frozen
// 
    unittest_assert( ICELIB_isActiveCheckList( &pStreamController0->checkList) == true);
    unittest_assert( ICELIB_isFrozenCheckList( &pStreamController1->checkList) == true);
//
//- Schedule a couple of checks (pair with index 0 and 2 will go to IN PROGRESS)
//
    ICELIB_Tick( &Instance);
    ICELIB_Tick( &Instance);

    unittest_assert( pCheckList0->checkListPairs[ 0].pairState == ICELIB_PAIR_INPROGRESS);
    unittest_assert( pCheckList0->checkListPairs[ 2].pairState == ICELIB_PAIR_INPROGRESS);

    TransactionId = pCheckList0->checkListPairs[ 0].transactionIdTable[0];  // TBD PTM
    pPair = pICELIB_correlateToRequest( &StreamIndex, &Instance, &TransactionId);

    unittest_assert( &pCheckList0->checkListPairs[ 0] == pPair);

    NetAddr_initFromIPv4StringWithPort( &mappedAddr, "10.47.5.58:5678", NET_PROTO_UDP);

//
//- Simulate an incomming Binding Response with success response.
//  The mapped address should result in a new peer reflexive candidate 
//  which should:
// 
//  1. End up in the list of discovered local candidates
//  2. Have a transport address equal to the mapped address
//
//  The new candidate should result in a new pair which should:
// 
//  1. Be inserted into the valid list
//  2. Refers back to the pair that generated it (refersToPairId)
// 
//  The pair that was hit by the response should:
// 
//  1. Have its state set to SUCCEEDED
//  2. Refers to the pair in the valid list (refersToPairId)
//
//  The pair with a foundation equal to the pair that went SUCCEEDED should
//  end up as WAITING
// 

//    printf( "\nScheduled pair:\n");
//    ICELIB_checkListDumpPair( pPair);
//    printf( "\n");

    ICELIB_incommingBindingResponse( &Instance,
                                     200,
                                     TransactionId,
                                     &pPair->pRemoteCandidate->connectionAddr,
                                     &pPair->pLocalCandidate->connectionAddr,
                                     &mappedAddr);

//    ICELIB_checkListDumpAll( &Instance);
//    ICELIB_checkListDump( pCheckList0);

//    printf( "\nDiscovered local candidate:\n");
//    ICELIBTYPES_ICE_MEDIA_STREAM_dump( &pStreamController0->discoveredLocalCandidates);

    pCandidate = &pStreamController0->discoveredLocalCandidates.candidate[ 0];

    unittest_assert( pStreamController0->discoveredLocalCandidates.numberOfEntries == 1);
    unittest_assert( NetAddr_alike( &pCandidate->connectionAddr, &mappedAddr));

    unittest_assert( pCheckList0->numberOfPairs  == 12);
    unittest_assert( pCheckList0->checkListState == ICELIB_CHECKLIST_RUNNING);
    unittest_assert( ICELIB_isActiveCheckList( pCheckList0) == true);

    unittest_assert( pPair->pairState == ICELIB_PAIR_SUCCEEDED);
    unittest_assert( ICELIB_validListCount( pValidList0) == 1);

    unittest_assert( pCheckList0->checkListPairs[ 1].pairState == ICELIB_PAIR_WAITING);

//
//- Schedule one more check (pair with index 1 will go to IN PROGRESS)
//
    ICELIB_Tick( &Instance);
    unittest_assert( pCheckList0->checkListPairs[ 1].pairState == ICELIB_PAIR_INPROGRESS);

    TransactionId = pCheckList0->checkListPairs[ 1].transactionIdTable[0];  // TBD PTM
    pPair = pICELIB_correlateToRequest( &StreamIndex, &Instance, &TransactionId);

    unittest_assert( &pCheckList0->checkListPairs[ 1] == pPair);

//
//- Simulate an incomming Binding Response to this pair (pair 1).
//  The mapped address is now set equal to the HOST address.
//  This should result in this pair going from IN PROGRESS to SUCCEEDED and also
//  going into the Valid List.
// 
//  Since there are now valid pairs for all components of this media stream, the
//  check list for the other media stream will also change.
// 

    ICELIB_incommingBindingResponse( &Instance,
                                     200,
                                     TransactionId,
                                     &pPair->pRemoteCandidate->connectionAddr,
                                     &pPair->pLocalCandidate->connectionAddr,
                                     &pPair->pLocalCandidate->connectionAddr);

    unittest_assert( pCheckList0->checkListPairs[ 1].pairState == ICELIB_PAIR_SUCCEEDED);
    unittest_assert( ICELIB_validListCount( pValidList0) == 2);

//
//  Since there are now valid pairs for all components of this media stream, the
//  check list for the other media stream will also change.
// 
//  ICE 7.1.2.2.3:
// 
//  If the check list is frozen, and there is at least one pair in
//  the check list whose foundation matches a pair in the valid
//  list under consideration, the state of all pairs in the check
//  list whose foundation matches a pair in the valid list under
//  consideration are set to Waiting. This will cause the check
//  list to become active

// 
    unittest_assert( ICELIB_isActiveCheckList( pCheckList1) == true);
    unittest_assert( pCheckList1->checkListPairs[ 0].pairState == ICELIB_PAIR_WAITING);
    unittest_assert( pCheckList1->checkListPairs[ 1].pairState == ICELIB_PAIR_WAITING);
    
//    ICELIB_checkListDumpAll( &Instance);
//    printf( "\n\n------------- CHECKLIST -------------------------------------------\n\n");
//    ICELIB_checkListDump( pCheckList0);
//    ICELIB_validListDumpLog( &Instance.callbacks.callbackLog , pCheckList0);
//    printf( "\n\n------------- VALIDLIST -------------------------------------------\n\n");
//    ICELIB_validListDump( pValidList0);
//    printf( "\n\n");
}


void unittest_iceResponseTimeout( void)
{
    ICELIB_INSTANCE            Instance;
    ICELIB_CHECKLIST           *pCheckList0;
    ICELIB_STREAM_CONTROLLER   *pStreamController0;
    StunMsgId                   TransactionId;

    unittest_context( "unittest_iceResponseTimeout");

    createTwoCheckLists( &Instance);

    unittest_assert( Instance.iceControlling == false);

    pStreamController0 = &Instance.streamControllers[ 0];
    pCheckList0        = &pStreamController0->checkList;
//
//- Schedule a couple of checks (pair with index 0 and 2 will go to IN PROGRESS)
//
    ICELIB_Tick( &Instance);
    ICELIB_Tick( &Instance);

    unittest_assert( pCheckList0->checkListPairs[ 0].pairState == ICELIB_PAIR_INPROGRESS);
    unittest_assert( pCheckList0->checkListPairs[ 2].pairState == ICELIB_PAIR_INPROGRESS);

    TransactionId = pCheckList0->checkListPairs[ 0].transactionIdTable[0];  // TBD PTM 
//
//- Simulate an incomming Binding Response timeout.
// 
    ICELIB_incommingTimeout( &Instance, TransactionId);

    unittest_assert( pCheckList0->checkListPairs[ 0].pairState == ICELIB_PAIR_FAILED);
}


void unittest_iceRequestError1( void)
{
    ICELIB_INSTANCE             Instance;
    ICELIB_CHECKLIST           *pCheckList;
    ICELIB_TRIGGERED_FIFO      *pFifo;
    ICELIB_STREAM_CONTROLLER   *pStreamController0;
    ICELIB_STREAM_CONTROLLER   *pStreamController1;
    StunMsgId                   transactionId;
    NET_ADDR                    sourceAddr;
    NET_ADDR                    destinationAddr;
    NET_ADDR                    mappedAddr;
    BindingResponseCallbackData brData;

    unittest_context( "unittest_iceRequestError1");
    createTwoCheckLists( &Instance);

    unittest_assert( Instance.iceControlling == false);

    pStreamController0 = &Instance.streamControllers[ 0];
    pStreamController1 = &Instance.streamControllers[ 1];
    pCheckList         = &pStreamController0->checkList;
    pFifo              = &pStreamController0->triggeredChecksFifo;

//
//- First list should be active, the second should be frozen
// 
    unittest_assert( ICELIB_isActiveCheckList( &pStreamController0->checkList) == true);
    unittest_assert( ICELIB_isFrozenCheckList( &pStreamController1->checkList) == true);
//
//- Schedule a couple of checks (pair 0 and 2 will go to IN PROGRESS)
//
    ICELIB_Tick( &Instance);
    ICELIB_Tick( &Instance);

    unittest_assert( pCheckList->checkListPairs[ 0].pairState == ICELIB_PAIR_INPROGRESS);
    unittest_assert( pCheckList->checkListPairs[ 2].pairState == ICELIB_PAIR_INPROGRESS);
//
//- Register a callback for the response, set up some addresses etc.
// 
    NetAddr_initFromIPv4StringWithPort( &destinationAddr, "10.47.4.58:6789", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &sourceAddr,      "10.47.3.58:5678", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &mappedAddr,      "10.47.3.58:5678", NET_PROTO_UDP);
    transactionId = ICELIB_generateTransactionId();
    ICELIB_setCallbackOutgoingBindingResponse( &Instance, sendBindingResponse_, &brData);
//
//- Force role conflict errors
// 
    brData.userValue1     = 1234;
    brData.userValue2     = 5678;
    brData.source         = &destinationAddr;
    brData.destination    = &sourceAddr;
    brData.MappedAddress  = &mappedAddr;
    brData.errorResponse  = 487;
    brData.transactionId  = transactionId;
    brData.callbackCalled = false;

    ICELIB_incommingBindingRequest( &Instance,
                                    1234,                       // userValue1
                                    5678,                       // userValue2
                                    "aaaa:bbbb",                // pUfragPair
                                    1234,                       // priority
                                    false,                      // useCandidate
                                    false,                      // iceControlling
                                    true,                       // iceControlled
                                    Instance.tieBreaker + 1,    // tieBreaker
                                    transactionId,              // transactionId
                                    &sourceAddr,                // source
                                    &destinationAddr,           // destination
                                    false,                      // fromRelay     TBD PTM
                                    NULL,                       // relayBaseAddr TBD PTM
                                    0);                         // componentId   TBD PTM

    unittest_assert( brData.callbackCalled == true);

    Instance.iceControlling = true;
    brData.callbackCalled   = false;

    ICELIB_incommingBindingRequest( &Instance,
                                    1234,                       // userValue1
                                    5678,                       // userValue2
                                    "aaaa:bbbb",                // pUfragPair
                                    1234,                       // priority
                                    false,                      // useCandidate
                                    true,                       // iceControlling
                                    false,                      // iceControlled
                                    Instance.tieBreaker - 1,    // tieBreaker
                                    transactionId,              // transactionId
                                    &sourceAddr,                // source
                                    &destinationAddr,           // destination
                                    false,                      // fromRelay     TBD PTM
                                    NULL,                       // relayBaseAddr TBD PTM
                                    0);                         // componentId   TBD PTM

    unittest_assert( brData.callbackCalled == true);
}


void unittest_iceRequestError2( void)
{
    ICELIB_INSTANCE             Instance;
    ICELIB_CHECKLIST           *pCheckList0;
    ICELIB_CHECKLIST           *pCheckList1;
    ICELIB_TRIGGERED_FIFO      *pFifo;
    ICELIB_STREAM_CONTROLLER   *pStreamController0;
    ICELIB_STREAM_CONTROLLER   *pStreamController1;
    StunMsgId                   transactionId;
    NET_ADDR                    sourceAddr;
    NET_ADDR                    destinationAddr;
    NET_ADDR                    mappedAddr;
    char                        uFragPair[ ICE_MAX_UFRAG_PAIR_LENGTH];
    BindingResponseCallbackData brData;

    unittest_context( "unittest_iceRequestError2");

    createTwoCheckLists( &Instance);
    unittest_assert( Instance.iceControlling == false);

    pStreamController0 = &Instance.streamControllers[ 0];
    pStreamController1 = &Instance.streamControllers[ 1];
    pCheckList0        = &pStreamController0->checkList;
    pCheckList1        = &pStreamController1->checkList;
    pFifo              = &pStreamController0->triggeredChecksFifo;

//
//- First list should be active, the second should be frozen
// 
    unittest_assert( ICELIB_isActiveCheckList( &pStreamController0->checkList) == true);
    unittest_assert( ICELIB_isFrozenCheckList( &pStreamController1->checkList) == true);
//
//- Schedule a couple of checks (pair 0 and 2 will go to IN PROGRESS)
//
    ICELIB_Tick( &Instance);
    ICELIB_Tick( &Instance);

    unittest_assert( pCheckList0->checkListPairs[ 0].pairState == ICELIB_PAIR_INPROGRESS);
    unittest_assert( pCheckList0->checkListPairs[ 2].pairState == ICELIB_PAIR_INPROGRESS);
//
//- Register a callback for the response, set up some addresses etc.
// 
    NetAddr_initFromIPv4StringWithPort( &sourceAddr, "10.47.3.58:5678", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &mappedAddr, "10.47.3.58:5678", NET_PROTO_UDP);
    transactionId = ICELIB_generateTransactionId();
    ICELIB_setCallbackOutgoingBindingResponse( &Instance, sendBindingResponse_, &brData);
//
//- Lets target the HOST address found here (stream index 1):
// 
    destinationAddr = pCheckList1->checkListPairs[ 1].pLocalCandidate->connectionAddr;
//
//- Verify inital order in the check lists
// 
    unittest_assert( pCheckList0->checkListPairs[  0].pairId ==  1);
    unittest_assert( pCheckList0->checkListPairs[  1].pairId ==  4);
    unittest_assert( pCheckList0->checkListPairs[  2].pairId ==  2);
    unittest_assert( pCheckList0->checkListPairs[  3].pairId ==  5);
    unittest_assert( pCheckList0->checkListPairs[  4].pairId == 13);
    unittest_assert( pCheckList0->checkListPairs[  5].pairId ==  3);
    unittest_assert( pCheckList0->checkListPairs[  6].pairId == 14);
    unittest_assert( pCheckList0->checkListPairs[  7].pairId == 15);
    unittest_assert( pCheckList0->checkListPairs[  8].pairId == 16);
    unittest_assert( pCheckList0->checkListPairs[  9].pairId ==  6);
    unittest_assert( pCheckList0->checkListPairs[ 10].pairId == 17);
    unittest_assert( pCheckList0->checkListPairs[ 11].pairId == 18);

    unittest_assert( pCheckList1->checkListPairs[  0].pairId ==  1);
    unittest_assert( pCheckList1->checkListPairs[  1].pairId ==  4);
    unittest_assert( pCheckList1->checkListPairs[  2].pairId ==  2);
    unittest_assert( pCheckList1->checkListPairs[  3].pairId ==  5);
    unittest_assert( pCheckList1->checkListPairs[  4].pairId == 13);
    unittest_assert( pCheckList1->checkListPairs[  5].pairId ==  3);
    unittest_assert( pCheckList1->checkListPairs[  6].pairId == 14);
    unittest_assert( pCheckList1->checkListPairs[  7].pairId == 15);
    unittest_assert( pCheckList1->checkListPairs[  8].pairId == 16);
    unittest_assert( pCheckList1->checkListPairs[  9].pairId ==  6);
    unittest_assert( pCheckList1->checkListPairs[ 10].pairId == 17);
    unittest_assert( pCheckList1->checkListPairs[ 11].pairId == 18);
//
//- Force a change of role
// 
    brData.callbackCalled = false;
    unittest_assert( Instance.iceControlling == false);

    ICELIB_getCheckListUsernamePair( uFragPair, ICE_MAX_UFRAG_PAIR_LENGTH, pCheckList1);

    uFragPair[ 4] = '*';    // Force a username error to stop processing early

    ICELIB_incommingBindingRequest( &Instance,
                                    1234,                       // userValue1
                                    5678,                       // userValue2
                                    uFragPair,                  // pUfragPair
                                    1234,                       // priority
                                    false,                      // useCandidate
                                    false,                      // iceControlling
                                    true,                       // iceControlled
                                    Instance.tieBreaker - 1,    // tieBreaker
                                    transactionId,              // transactionId
                                    &sourceAddr,                // source
                                    &destinationAddr,           // destination
                                    false,                      // fromRelay     TBD PTM
                                    NULL,                       // relayBaseAddr TBD PTM
                                    0);                         // componentId   TBD PTM


    unittest_assert( brData.callbackCalled   == false);
    unittest_assert( Instance.iceControlling == true);
//
//- Verify that a change of priorities has rearanged the check lists a little
//
    unittest_assert( pCheckList0->checkListPairs[  0].pairId ==  1);
    unittest_assert( pCheckList0->checkListPairs[  1].pairId ==  4);
    unittest_assert( pCheckList0->checkListPairs[  2].pairId ==  2);
    unittest_assert( pCheckList0->checkListPairs[  3].pairId ==  5);
    unittest_assert( pCheckList0->checkListPairs[  4].pairId ==  3); // swapped!
    unittest_assert( pCheckList0->checkListPairs[  5].pairId == 13); // swapped!
    unittest_assert( pCheckList0->checkListPairs[  6].pairId == 14);
    unittest_assert( pCheckList0->checkListPairs[  7].pairId == 15);
    unittest_assert( pCheckList0->checkListPairs[  8].pairId ==  6); // swapped!
    unittest_assert( pCheckList0->checkListPairs[  9].pairId == 16); // swapped!
    unittest_assert( pCheckList0->checkListPairs[ 10].pairId == 17);
    unittest_assert( pCheckList0->checkListPairs[ 11].pairId == 18);
   
    unittest_assert( pCheckList1->checkListPairs[  0].pairId ==  1);
    unittest_assert( pCheckList1->checkListPairs[  1].pairId ==  4);
    unittest_assert( pCheckList1->checkListPairs[  2].pairId ==  2);
    unittest_assert( pCheckList1->checkListPairs[  3].pairId ==  5);
    unittest_assert( pCheckList1->checkListPairs[  4].pairId ==  3); // swapped!
    unittest_assert( pCheckList1->checkListPairs[  5].pairId == 13); // swapped!
    unittest_assert( pCheckList1->checkListPairs[  6].pairId == 14);
    unittest_assert( pCheckList1->checkListPairs[  7].pairId == 15);
    unittest_assert( pCheckList1->checkListPairs[  8].pairId ==  6); // swapped!
    unittest_assert( pCheckList1->checkListPairs[  9].pairId == 16); // swapped!
    unittest_assert( pCheckList1->checkListPairs[ 10].pairId == 17);
    unittest_assert( pCheckList1->checkListPairs[ 11].pairId == 18);
}


void unittest_iceRequestOK( void)
{
    ICELIB_INSTANCE             Instance;
    ICELIB_CHECKLIST           *pCheckList0;
    ICELIB_CHECKLIST           *pCheckList1;
    ICELIB_TRIGGERED_FIFO      *pFifo;
    ICELIB_STREAM_CONTROLLER   *pStreamController0;
    ICELIB_STREAM_CONTROLLER   *pStreamController1;
    StunMsgId                   transactionId;
    NET_ADDR                    sourceAddr;
    NET_ADDR                    destinationAddr;
    NET_ADDR                    mappedAddr;
    char                        uFragPair[ ICE_MAX_UFRAG_PAIR_LENGTH];
    BindingResponseCallbackData brData;

    unittest_context( "unittest_iceRequestOK");

    createTwoCheckLists( &Instance);
    unittest_assert( Instance.iceControlling == false);

    pStreamController0 = &Instance.streamControllers[ 0];
    pStreamController1 = &Instance.streamControllers[ 1];
    pCheckList0        = &pStreamController0->checkList;
    pCheckList1        = &pStreamController1->checkList;
    pFifo              = &pStreamController0->triggeredChecksFifo;

//
//- First list should be active, the second should be frozen
// 
    unittest_assert( ICELIB_isActiveCheckList( &pStreamController0->checkList) == true);
    unittest_assert( ICELIB_isFrozenCheckList( &pStreamController1->checkList) == true);
//
//- Schedule a couple of checks (pair 0 and 2 will go to IN PROGRESS)
//
    ICELIB_Tick( &Instance);
    ICELIB_Tick( &Instance);

    unittest_assert( pCheckList0->checkListPairs[ 0].pairState == ICELIB_PAIR_INPROGRESS);
    unittest_assert( pCheckList0->checkListPairs[ 2].pairState == ICELIB_PAIR_INPROGRESS);
//
//- Register a callback for the response, set up some addresses etc.
// 
    NetAddr_initFromIPv4StringWithPort( &sourceAddr, "10.47.3.58:5678", NET_PROTO_UDP);
    NetAddr_initFromIPv4StringWithPort( &mappedAddr, "10.47.3.58:5678", NET_PROTO_UDP);
    transactionId = ICELIB_generateTransactionId();
    ICELIB_setCallbackOutgoingBindingResponse( &Instance, sendBindingResponse_, &brData);
//
//- Lets target the HOST address found here (stream index 0):
// 
    brData.userValue1     = 1234;
    brData.userValue2     = 5678;
    brData.source         = &destinationAddr;
    brData.destination    = &sourceAddr;
    brData.MappedAddress  = &mappedAddr;
    brData.errorResponse  = 200;
    brData.transactionId  = transactionId;
    brData.callbackCalled = false;

    destinationAddr = pCheckList0->checkListPairs[ 1].pLocalCandidate->connectionAddr;

    //- Remember to switch LFRAG and RFRAG for incomming request:

    ICELIB_makeUsernamePair( uFragPair,
                                   ICE_MAX_UFRAG_PAIR_LENGTH,
                                   pCheckList0->ufragLocal,
                                   pCheckList0->ufragRemote);


    ICELIB_incommingBindingRequest( &Instance,
                                    1234,                       // userValue1
                                    5678,                       // userValue2
                                    uFragPair,                  // pUfragPair
                                    1234,                       // priority
                                    false,                      // useCandidate
                                    true,                       // iceControlling
                                    false,                      // iceControlled
                                    Instance.tieBreaker - 1,    // tieBreaker
                                    transactionId,              // transactionId
                                    &sourceAddr,                // source
                                    &destinationAddr,           // destination
                                    false,                      // fromRelay     TBD PTM
                                    NULL,                       // relayBaseAddr TBD PTM
                                    0);                         // componentId   TBD PTM

    unittest_assert( brData.callbackCalled   == true);
    unittest_assert( Instance.iceControlling == false);
   
}


void unittest_iceTimer( void)
{
    unsigned int            i;
    ICELIB_INSTANCE         Instance;
    ICELIB_CONFIGURATION    config;
    ICELIB_TIMER            timer0;
    ICELIB_TIMER            *pTimer0 = &timer0;

    unittest_context( "unittest_iceTimer");

    memset( &config, 0, sizeof( config));

    config.tickIntervalMS       = 20;       // Number of ms between timer ticks
    config.keepAliveIntervalS   = 15;
    config.maxCheckListPairs    = ICELIB_MAX_PAIRS;
    config.aggressiveNomination = false;
    config.iceLite              = false;
    config.stoppingTimeoutS     = 5;
    config.logLevel             = UNITTEST_LOG_LEVEL;

    ICELIB_Constructor( &Instance, &config);
    ICELIB_timerConstructor(pTimer0, config.tickIntervalMS);

    unittest_assert( ICELIB_timerIsTimedOut( pTimer0) == false);
    unittest_assert( ICELIB_timerIsRunning( pTimer0)  == false);
//
//- First run
// 
    ICELIB_timerStart( pTimer0, 5 * 1000);  // Timeout in 5 seconds
    unittest_assert( ICELIB_timerIsRunning( pTimer0) == true);

    for( i=0; i < ( ( 5 * 1000 / 20) - 1); ++i) {
        ICELIB_timerTick( pTimer0);
    }

    unittest_assert( ICELIB_timerIsTimedOut( pTimer0) == false);
    ICELIB_timerTick( pTimer0);
    unittest_assert( ICELIB_timerIsTimedOut( pTimer0) == true);
//
//- Second run
// 
    unittest_assert( ICELIB_timerIsRunning( pTimer0) == false);
    ICELIB_timerStart( pTimer0, 3 * 1000);  // Timeout in 5 seconds
    unittest_assert( ICELIB_timerIsRunning( pTimer0) == true);

    for( i=0; i < ( ( 3 * 1000 / 20) - 1); ++i) {
        ICELIB_timerTick( pTimer0);
    }

    unittest_assert( ICELIB_timerIsTimedOut( pTimer0) == false);
    ICELIB_timerTick( pTimer0);
    unittest_assert( ICELIB_timerIsTimedOut( pTimer0) == true);
}


void LogFunction_( void *pUserData, ICELIB_logLevel logLevel, const char *str)
{
    LogFunctionCallbackData *pCallbackData;

    pCallbackData = ( LogFunctionCallbackData *)pUserData;
    pCallbackData->callbackCalled = true;
    unittest_assert( strcmp( str, pCallbackData->message) == 0);
}


void unittest_iceLogging( void)
{
    ICELIB_INSTANCE         Instance;
    ICELIB_CONFIGURATION    config;
    ICELIB_CALLBACK_LOG     *pCallbackLog;
    LogFunctionCallbackData callbackData;

    unittest_context( "unittest_iceLogging");

    memset( &config, 0, sizeof( config));

    config.tickIntervalMS       = 20;
    config.keepAliveIntervalS   = 15;
    config.maxCheckListPairs    = ICELIB_MAX_PAIRS;
    config.aggressiveNomination = false;
    config.iceLite              = false;
    config.stoppingTimeoutS     = 5;
    config.logLevel             = ICELIB_logDebug;

    ICELIB_Constructor( &Instance, &config);

    pCallbackLog = &Instance.callbacks.callbackLog;

    callbackData.callbackCalled = false;
    callbackData.message        = "Just testing...";

    ICELIB_setCallbackLog( &Instance, LogFunction_, &callbackData, ICELIB_logDebug);
    ICELIB_logString( pCallbackLog, ICELIB_logDebug, "Just testing...");
    unittest_assert( callbackData.callbackCalled == true);

    callbackData.callbackCalled = false;
    callbackData.message        = "Just testing 2...";

    ICELIB_setCallbackLog( &Instance, LogFunction_, &callbackData, ICELIB_logInfo);
    ICELIB_logString( pCallbackLog, ICELIB_logDebug, "Callback not called...");
    unittest_assert( callbackData.callbackCalled == false);

    ICELIB_setCallbackLog( &Instance, LogFunction_, &callbackData, ICELIB_logInfo);
    ICELIB_logString( pCallbackLog, ICELIB_logInfo, "Just testing 2...");
    unittest_assert( callbackData.callbackCalled == true);

    callbackData.callbackCalled = false;
    callbackData.message        = "Just testing 1-2-3";

    ICELIB_setCallbackLog( &Instance, LogFunction_, &callbackData, ICELIB_logWarning);
    ICELIB_logVaString( pCallbackLog, ICELIB_logWarning, "Just testing %d-%d-%d", 1, 2, 3);
    unittest_assert( callbackData.callbackCalled == true);

}


void unittest_ice( void)
{
    unittest_context( "unittest_ice");

    unittest_iceufrag(); /* ok */
    unittest_icepasswd(); /* ok */
    unittest_icepriority(); /* ok */
    unittest_icefoundation(); /* ok */
    unittest_iceCreateIceMediaStream1(); /* ok */
    unittest_iceCreateIceMediaStream2(); /* ok */
    unittest_icePairPriority(); /* ok */
    unittest_iceFindBases();   /* Todo in larger context */
    unittest_iceCheckListBasic(); /* Todo in larger context */
    unittest_iceCheckListSingle(); /* Todo in larger context */
    unittest_iceTriggeredCheckQueue1(); /* ok */
    unittest_iceTriggeredCheckQueue2(); /* ok */
 //   unittest_iceCheckList(); TBD PTM  fails - crash
 //   unittest_iceCorrelate(); TBD PTM fails 
//    unittest_iceResponseError();  TBD PTM  fails - crash
//    unittest_iceResponseOK();   TBD PTM  fails - crash
//    unittest_iceResponseTimeout(); TBD PTM fails
//    unittest_iceRequestError1();
//    unittest_iceRequestError2(); TBD PTM  fails - crash
//    unittest_iceRequestOK(); TBD PTM  fails - crash
    unittest_iceTimer(); /* ok */
    unittest_iceLogging();
}



#endif // UNITTEST
