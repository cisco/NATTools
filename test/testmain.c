
#include <stdlib.h>
#include <stdio.h>


#include "icelib.h"
#include <check.h>


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


static bool isLegalString( const char *str)
{
    while( *str) {
        if( !isLegalCharacter( *str++)) {
            return false;
        }
    }

    return true;
}

/*
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

    
    for (i = 0; i < numberOfMediaCandidates; ++i) {
        NetAddr_initFromIPv4StringWithPort(&na[i], addr[i], eProto);
    }
}
*/


ICELIB_INSTANCE icelib;


void
setup (void)
{

    struct sockaddr_storage defaultAddr;
    
    struct sockaddr_storage localHostRtp;
    struct sockaddr_storage localHostRtcp;
    struct sockaddr_storage localRflxRtp;
    struct sockaddr_storage localRflxRtcp;
    struct sockaddr_storage localRelayRtp;
    struct sockaddr_storage localRelayRtcp;

    struct sockaddr_storage remoteHostRtp;
    struct sockaddr_storage remoteHostRtcp;
    struct sockaddr_storage remoteRflxRtp;
    struct sockaddr_storage remoteRflxRtcp;
    struct sockaddr_storage remoteRelayRtp;
    struct sockaddr_storage remoteRelayRtcp;


    sockaddr_initFromString( (struct sockaddr *)&localHostRtp,  "192.168.2.10:3456");
    sockaddr_initFromString( (struct sockaddr *)&localHostRtcp, "192.168.2.10:3457");
    sockaddr_initFromString( (struct sockaddr *)&localRflxRtp,  "67.45.4.6:1045");
    sockaddr_initFromString( (struct sockaddr *)&localRflxRtcp, "67.45.4.6:3451");
    sockaddr_initFromString( (struct sockaddr *)&localRelayRtp, "158.38.46.10:2312");
    sockaddr_initFromString( (struct sockaddr *)&localRelayRtcp,"158.38.46.10:4567");

    sockaddr_initFromString( (struct sockaddr *)&remoteHostRtp,  "192.168.2.10:3456");
    sockaddr_initFromString( (struct sockaddr *)&remoteHostRtcp, "192.168.2.10:3457");
    sockaddr_initFromString( (struct sockaddr *)&remoteRflxRtp,  "147.45.4.6:1045");
    sockaddr_initFromString( (struct sockaddr *)&remoteRflxRtcp, "147.45.4.6:3451");
    sockaddr_initFromString( (struct sockaddr *)&remoteRelayRtp, "67.38.46.10:2312");
    sockaddr_initFromString( (struct sockaddr *)&remoteRelayRtcp,"78.38.46.10:4567");



    ICELIB_CONFIGURATION iceConfig;     
    iceConfig.tickIntervalMS = 20;
    iceConfig.keepAliveIntervalS = 15;
    iceConfig.maxCheckListPairs = ICELIB_MAX_PAIRS;
    iceConfig.aggressiveNomination = false;
    iceConfig.iceLite = false;


    ICELIB_Constructor (&icelib,
                        &iceConfig);
    
    //Local side
    ICELIB_addLocalMediaStream(&icelib,
                               0,
                               42,
                               42,
                               ICE_CAND_TYPE_HOST);

    
    ICELIB_addLocalCandidate( &icelib,
                              0,
                              1,
                              (struct sockaddr *)&localHostRtp,
                              NULL,
                              ICE_CAND_TYPE_HOST );
    
    ICELIB_addLocalCandidate( &icelib,
                              0,
                              2,
                              (struct sockaddr *)&localHostRtcp,
                              NULL,
                              ICE_CAND_TYPE_HOST );
    
    
    //add rflx candidates
    
    
    ICELIB_addLocalCandidate( &icelib,
                              0,
                              1,
                              (struct sockaddr *)&localRflxRtp,
                              (struct sockaddr *)&localHostRtp,
                              ICE_CAND_TYPE_SRFLX );
        
    ICELIB_addLocalCandidate( &icelib,
                              0,
                              2,
                              (struct sockaddr *)&localRflxRtcp,
                              (struct sockaddr *)&localHostRtp,
                              ICE_CAND_TYPE_SRFLX );
    
        
    
    
    //add relay candidates
    ICELIB_addLocalCandidate( &icelib,
                              0,
                              1,
                              (struct sockaddr *)&localRelayRtp,
                              (struct sockaddr *)&localRflxRtp,
                              ICE_CAND_TYPE_RELAY );
    
    ICELIB_addLocalCandidate( &icelib,
                              0,
                              2,
                              (struct sockaddr *)&localRelayRtcp,
                              (struct sockaddr *)&localRflxRtcp,
                              ICE_CAND_TYPE_RELAY );
    
        
    ICELIB_setTurnState(&icelib, 0, ICE_TURN_ALLOCATED);

    //Remote side
    sockaddr_initFromString( (struct sockaddr *)&defaultAddr,
                              "192.168.2.10:3456");

    ICELIB_addRemoteMediaStream(&icelib,
                                "dert",
                                "gtre",
                                (struct sockaddr *)&defaultAddr);

    ICELIB_addRemoteCandidate(&pSession->iceLibInstance, 
                              0,
                              pCand->szFoundation, pCand->foundationLen, 
                              pCand->componentid, pCand->priority,
                              pCand->szConnectionAddr, pCand->port,  
                              candType);
}



void
teardown (void)
{
    ICELIB_Destructor (&icelib);
}


START_TEST (create_ufrag)
{
    char tmp1[ ICE_MAX_UFRAG_LENGTH];
    char tmp2[ ICE_MAX_UFRAG_LENGTH];

    memset( tmp1, '#', sizeof( tmp1));
    memset( tmp2, '#', sizeof( tmp2));

    ICELIB_createUfrag( tmp1, ICELIB_UFRAG_LENGTH);
    ICELIB_createUfrag( tmp2, ICELIB_UFRAG_LENGTH);

    fail_unless( isLegalString( tmp1),
                 "ufrag failed" );

    fail_unless( isLegalString( tmp2),
                 "ufrag failed" );

    fail_unless( ( strlen( tmp1) + 1) == ICELIB_UFRAG_LENGTH,
                 "ufrag length faild");

    fail_unless( ( strlen( tmp2) + 1) == ICELIB_UFRAG_LENGTH,
                 "ufrag length faild");

    fail_unless( strcmp( tmp1, tmp2), 
                 "ufrag failed");

    ICELIB_createUfrag( tmp1, ICE_MAX_UFRAG_LENGTH + 999);

    fail_unless( ( strlen( tmp1) + 1) == ICE_MAX_UFRAG_LENGTH,
                 "ufrag failed");

    fail_unless( isLegalString( tmp1));


}
END_TEST


START_TEST (create_passwd)
{
    char tmp1[ ICE_MAX_PASSWD_LENGTH];
    char tmp2[ ICE_MAX_PASSWD_LENGTH];
    unsigned int i;
        
    memset( tmp1, '#', sizeof( tmp1));
    memset( tmp2, '#', sizeof( tmp2));

    ICELIB_createPasswd( tmp1, ICELIB_PASSWD_LENGTH);
    ICELIB_createPasswd( tmp2, ICELIB_PASSWD_LENGTH);

    fail_unless( isLegalString( tmp1));
    fail_unless( isLegalString( tmp2));

    fail_unless( ( strlen( tmp1) + 1) == ICELIB_PASSWD_LENGTH);
    fail_unless( ( strlen( tmp2) + 1) == ICELIB_PASSWD_LENGTH);
    fail_unless( strcmp( tmp1, tmp2));

    memset( tmp2, '#', sizeof( tmp2));
    tmp2[ ICE_MAX_PASSWD_LENGTH-1] = '\0';

    for (i = 0; i < ICE_MAX_PASSWD_LENGTH + 10; ++i) {
        memset( tmp1, '#', sizeof( tmp1));
        tmp1[ICE_MAX_PASSWD_LENGTH - 1] = '\0';

        ICELIB_createPasswd(tmp1, i);

        if (i == 0) {
            fail_unless( strcmp( tmp1, tmp2) == 0);
        }
        else if (i > ICE_MAX_PASSWD_LENGTH) {
            fail_unless(isLegalString(tmp1));
            fail_unless((strlen(tmp1) + 1) == ICE_MAX_PASSWD_LENGTH);
        } else {
            fail_unless(isLegalString(tmp1));
            fail_unless((strlen(tmp1) + 1) == i);
        }
    }

}
END_TEST


START_TEST (calculate_priority)
{
    uint32_t priority1;
    uint32_t priority2;
    uint32_t priority3;
    uint32_t priority4;
    uint32_t priority5;
    uint32_t priority6;
    uint32_t priority7;
    uint32_t priority8;

    priority1 = ICELIB_calculatePriority( ICE_CAND_TYPE_HOST,  1);
    priority2 = ICELIB_calculatePriority( ICE_CAND_TYPE_HOST,  2);
    priority3 = ICELIB_calculatePriority( ICE_CAND_TYPE_SRFLX, 1);
    priority4 = ICELIB_calculatePriority( ICE_CAND_TYPE_SRFLX, 2);
    priority5 = ICELIB_calculatePriority( ICE_CAND_TYPE_RELAY, 1);
    priority6 = ICELIB_calculatePriority( ICE_CAND_TYPE_RELAY, 2);
    priority7 = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 1);
    priority8 = ICELIB_calculatePriority( ICE_CAND_TYPE_PRFLX, 2);

    fail_unless( priority1 == PRIORITY_HOST_1);
    fail_unless( priority2 == PRIORITY_HOST_2);
    fail_unless( priority3 == PRIORITY_SRFLX_1);
    fail_unless( priority4 == PRIORITY_SRFLX_2);
    fail_unless( priority5 == PRIORITY_RELAY_1);
    fail_unless( priority6 == PRIORITY_RELAY_2);
    fail_unless( priority7 == PRIORITY_PRFLX_1);
    fail_unless( priority8 == PRIORITY_PRFLX_2);


}
END_TEST


START_TEST (create_foundation)
{

    char tmp[ ICE_MAX_FOUNDATION_LENGTH];
 
    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_HOST, ICE_MAX_FOUNDATION_LENGTH);

    fail_unless( ( strlen( tmp) + 1) == ICELIB_FOUNDATION_LENGTH);
    fail_unless( strcmp( tmp, FOUNDATION_HOST) == 0);

    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_SRFLX, ICE_MAX_FOUNDATION_LENGTH);

    fail_unless( ( strlen( tmp) + 1) == ICELIB_FOUNDATION_LENGTH);
    fail_unless( strcmp( tmp, FOUNDATION_SRFLX) == 0);

    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_RELAY, ICE_MAX_FOUNDATION_LENGTH);

    fail_unless( ( strlen( tmp) + 1) == ICELIB_FOUNDATION_LENGTH);
    fail_unless( strcmp( tmp, FOUNDATION_RELAY) == 0);

    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_PRFLX, ICE_MAX_FOUNDATION_LENGTH);

    fail_unless( ( strlen( tmp) + 1) == ICELIB_FOUNDATION_LENGTH);
    fail_unless( strcmp( tmp, FOUNDATION_PRFLX) == 0);

    memset( tmp, '#', sizeof( tmp));

    ICELIB_createFoundation( tmp, ICE_CAND_TYPE_HOST+999, ICE_MAX_FOUNDATION_LENGTH);

    fail_unless( strlen( tmp) == strlen( "unknowntype"));
    fail_unless( strcmp( tmp, "unknowntype") == 0);

}
END_TEST


START_TEST (create_localMediaStream)
{
    ICELIB_INSTANCE localIcelib;
    ICELIB_CONFIGURATION localIceConfig; 
    ICE_MEDIA const *localIceMedia;

    int32_t result,i;


    localIceConfig.logLevel = ICELIB_logDisable;

    ICELIB_Constructor (&localIcelib,
                        &localIceConfig);
     
    
    for (i=0; i < ICE_MAX_MEDIALINES; i++){
        result = ICELIB_addLocalMediaStream(&localIcelib,
                                            i,
                                            45,
                                            34,
                                            ICE_CAND_TYPE_HOST);
        fail_unless( result == 1 );
        
        localIceMedia = ICELIB_getLocalIceMedia(&localIcelib);
        fail_unless( isLegalString( localIceMedia->mediaStream[i].ufrag ) );
        fail_unless( isLegalString( localIceMedia->mediaStream[i].passwd ) );
    }
            
    i++;
    result = ICELIB_addLocalMediaStream(&localIcelib,
                                        i,
                                        45,
                                        34,
                                        ICE_CAND_TYPE_HOST);
    fail_unless( result == -1 );

}
END_TEST


START_TEST (create_remoteMediaStream)
{
    ICELIB_INSTANCE remoteIcelib;
    ICELIB_CONFIGURATION remoteIceConfig; 
    struct sockaddr defaultAddr;

    int32_t result,i;


    //remoteIceConfig.logLevel = ICELIB_logDisable;
    remoteIceConfig.logLevel = 2;

    ICELIB_Constructor (&remoteIcelib,
                        &remoteIceConfig);
     
    
    for (i=0; i < ICE_MAX_MEDIALINES; i++){
        result = ICELIB_addRemoteMediaStream(&remoteIcelib,
                                            "ufrag",
                                            "pass",
                                             &defaultAddr);

        

        fail_unless( result != -1,
                     "Failed to add Remote mediastream (%i)", result);
    }

    i++;
    result = ICELIB_addRemoteMediaStream(&remoteIcelib,
                                            "ufrag",
                                            "pass",
                                             &defaultAddr);
    fail_unless( result == -1 );

}
END_TEST


START_TEST (pairPriority)
{
    uint32_t G;
    uint32_t D;
    uint64_t priority;
    uint64_t expected;

    G = 0x12345678;
    D = 0x76543210;
    priority = ICELIB_pairPriority( G, D);
    expected = 0xFFFFFFFFECA86420LL;
    fail_unless( priority == expected);

   

    G = 0x76543210;
    D = 0x12345678;
    priority = ICELIB_pairPriority( G, D);
    expected = 0xFFFFFFFFECA86421LL;
    fail_unless( priority == expected);

    G = 0x11111111;
    D = 0x22222222;
    priority = ICELIB_pairPriority( G, D);
    expected = 0x44444444LL;
    fail_unless( priority == expected);

    G = 0x22222222;
    D = 0x11111111;
    priority = ICELIB_pairPriority( G, D);
    expected = 0x44444445LL;
    fail_unless( priority == expected);

}
END_TEST


START_TEST (triggereedcheck_queue)
{
    bool                    result;
    unsigned int            i;
    unsigned int            j;
    unsigned int            k;
    ICELIB_FIFO_ELEMENT     element;
    ICELIB_TRIGGERED_FIFO   f;

    
    ICELIB_fifoClear( &f);
    fail_unless( ICELIB_fifoCount( &f) == 0);
    fail_unless( ICELIB_fifoIsEmpty( &f) == true);
    fail_unless( ICELIB_fifoIsFull( &f) == false);

    fail_unless( ICELIB_fifoGet( &f) == ICELIB_FIFO_IS_EMPTY);
//
//----- Run FIFO full
// 
    for( i=0; i < ICELIB_MAX_FIFO_ELEMENTS; ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i);
        fail_unless( result == false);
        fail_unless( ICELIB_fifoCount( &f) == i + 1);
    }

    fail_unless( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f) == true);

    fail_unless( ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) 999) == true);
//
//----- Remove some elements
// 
    for( i=0; i < 10; ++i) {
        element = ICELIB_fifoGet( &f);
        fail_unless( element == ( ICELIB_FIFO_ELEMENT) i);
        fail_unless( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS - i - 1);
    }

    fail_unless( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS - 10);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f) == false);
//
//----- Remove more elements
// 
    for( i=10; i < ICELIB_MAX_FIFO_ELEMENTS - 5; ++i) {
        element = ICELIB_fifoGet( &f);
        fail_unless( element == ( ICELIB_FIFO_ELEMENT) i);
        fail_unless( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS - i - 1);
    }

    fail_unless( ICELIB_fifoCount( &f) == 5);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f) == false);
//
//----- Remove the rest of the elements
// 
    for( i = ICELIB_MAX_FIFO_ELEMENTS - 5; i < ICELIB_MAX_FIFO_ELEMENTS; ++i) {
        element = ICELIB_fifoGet( &f);
        fail_unless( element == ( ICELIB_FIFO_ELEMENT) i);
        fail_unless( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS - i - 1);
    }

    fail_unless( ICELIB_fifoCount( &f) == 0);
    fail_unless( ICELIB_fifoIsEmpty( &f) == true);
    fail_unless( ICELIB_fifoIsFull( &f) == false);
//
//----- Fill 3/4 of the FIFO
// 
    for( i=0; i < ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4; ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        fail_unless( result == false);
        fail_unless( ICELIB_fifoCount( &f) == i + 1);
    }

    fail_unless( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f) == false);
//
//----- Remove 1/2 FIFO
// 
    for( j=0; j < ICELIB_MAX_FIFO_ELEMENTS / 2; ++j) {
        element = ICELIB_fifoGet( &f);
        fail_unless( element == ( ICELIB_FIFO_ELEMENT) j + 1000);
        fail_unless( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4 - j - 1);
    }

    fail_unless( ICELIB_fifoCount( &f) == ICELIB_MAX_FIFO_ELEMENTS / 4);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f) == false);
//
//----- Fill another 1/2 FIFO
// 
    for( k=0; k < ICELIB_MAX_FIFO_ELEMENTS / 2; ++k, ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        fail_unless( result == false);
        fail_unless( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS) / 4 + k + 1);
    }

    fail_unless( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f) == false);
//
//----- Remove 3/4 FIFO
// 
    for( k=0; k < ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4; ++k, ++j) {
        element = ICELIB_fifoGet( &f);
        fail_unless( element == ( ICELIB_FIFO_ELEMENT) j + 1000);
        fail_unless( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4 - k - 1);
    }

    fail_unless( ICELIB_fifoCount( &f) == 0);
    fail_unless( ICELIB_fifoIsEmpty( &f) == true);
    fail_unless( ICELIB_fifoIsFull( &f) == false);

}
END_TEST

START_TEST (triggereedcheck_queue_ekstra)
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


    ICELIB_fifoClear( &f);
    fail_unless( ICELIB_fifoCount( &f) == 0);
    fail_unless( ICELIB_fifoIsEmpty( &f) == true);
    fail_unless( ICELIB_fifoIsFull( &f) == false);
//
//----- Fill 3/4 of the FIFO
// 
    for( i=0; i < ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4; ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        fail_unless( result == false);
        fail_unless( ICELIB_fifoCount( &f) == i + 1);
    }

    fail_unless( ICELIB_fifoCount( &f)   == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f)  == false);
//
//----- Iterate through elements in FIFO
// 
    ICELIB_fifoIteratorConstructor( &tfIterator, &f);

    j = 0;
    while( ( pElement = (ICELIB_FIFO_ELEMENT *)pICELIB_fifoIteratorNext( &tfIterator)) != NULL) {
        fail_unless( *pElement == ( ICELIB_FIFO_ELEMENT) j + 1000);
        ++j;
    }

    fail_unless( j == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
//
//----- Remove 1/2 FIFO
// 
    for( j=0; j < ICELIB_MAX_FIFO_ELEMENTS / 2; ++j) {
        element = ICELIB_fifoGet( &f);
        fail_unless( element == ( ICELIB_FIFO_ELEMENT) j + 1000);
        fail_unless( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4 - j - 1);
    }

    fail_unless( ICELIB_fifoCount( &f)   == ICELIB_MAX_FIFO_ELEMENTS / 4);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f)  == false);
//
//----- Fill another 1/2 FIFO
// 
    for( k=0; k < ICELIB_MAX_FIFO_ELEMENTS / 2; ++k, ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        fail_unless( result == false);
        fail_unless( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS) / 4 + k + 1);
    }

    fail_unless( ICELIB_fifoCount( &f)   == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f)  == false);
//
//----- Iterate through elements in FIFO
// 
    ICELIB_fifoIteratorConstructor( &tfIterator, &f);

    k = 0;
    l = j;
    while( ( pElement = (ICELIB_FIFO_ELEMENT *)pICELIB_fifoIteratorNext( &tfIterator)) != NULL) {
        fail_unless( *pElement == ( ICELIB_FIFO_ELEMENT) l + 1000);
        ++k;
        ++l;
    }

    fail_unless( k == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4);
//
//----- Fill another 1/4 FIFO (should then be full)
// 
    for( k=0; k < ICELIB_MAX_FIFO_ELEMENTS / 4; ++k, ++i) {
        result = ICELIB_fifoPut( &f, ( ICELIB_FIFO_ELEMENT) i + 1000);
        fail_unless( result == false);
        fail_unless( ICELIB_fifoCount( &f) == ( ICELIB_MAX_FIFO_ELEMENTS * 3) / 4 + k + 1);
    }

    fail_unless( ICELIB_fifoCount( &f)   == ICELIB_MAX_FIFO_ELEMENTS);
    fail_unless( ICELIB_fifoIsEmpty( &f) == false);
    fail_unless( ICELIB_fifoIsFull( &f)  == true);
//
//----- Iterate through elements in FIFO
// 
    ICELIB_fifoIteratorConstructor( &tfIterator, &f);

    k = 0;
    l = j;
    while( ( pElement = (ICELIB_FIFO_ELEMENT *)pICELIB_fifoIteratorNext( &tfIterator)) != NULL) {
        fail_unless( *pElement == ( ICELIB_FIFO_ELEMENT) l + 1000);
        ++k;
        ++l;
    }

    fail_unless( k == ICELIB_MAX_FIFO_ELEMENTS);

}
END_TEST


START_TEST (ice_timer)
{
    unsigned int            i;
    ICELIB_INSTANCE         Instance;
    ICELIB_CONFIGURATION    config;
    ICELIB_TIMER            timer0;
    ICELIB_TIMER            *pTimer0 = &timer0;

    
    memset( &config, 0, sizeof( config));

    config.tickIntervalMS       = 20;       // Number of ms between timer ticks
    config.keepAliveIntervalS   = 15;
    config.maxCheckListPairs    = ICELIB_MAX_PAIRS;
    config.aggressiveNomination = false;
    config.iceLite              = false;
    config.stoppingTimeoutS     = 5;
    config.logLevel             = 3;

    ICELIB_Constructor( &Instance, &config);
    ICELIB_timerConstructor(pTimer0, config.tickIntervalMS);

    fail_unless( ICELIB_timerIsTimedOut( pTimer0) == false);
    fail_unless( ICELIB_timerIsRunning( pTimer0)  == false);
//
//- First run
// 
    ICELIB_timerStart( pTimer0, 5 * 1000);  // Timeout in 5 seconds
    fail_unless( ICELIB_timerIsRunning( pTimer0) == true);

    for( i=0; i < ( ( 5 * 1000 / 20) - 1); ++i) {
        ICELIB_timerTick( pTimer0);
    }

    fail_unless( ICELIB_timerIsTimedOut( pTimer0) == false);
    ICELIB_timerTick( pTimer0);
    fail_unless( ICELIB_timerIsTimedOut( pTimer0) == true);
//
//- Second run
// 
    fail_unless( ICELIB_timerIsRunning( pTimer0) == false);
    ICELIB_timerStart( pTimer0, 3 * 1000);  // Timeout in 5 seconds
    fail_unless( ICELIB_timerIsRunning( pTimer0) == true);

    for( i=0; i < ( ( 3 * 1000 / 20) - 1); ++i) {
        ICELIB_timerTick( pTimer0);
    }

    fail_unless( ICELIB_timerIsTimedOut( pTimer0) == false);
    ICELIB_timerTick( pTimer0);
    fail_unless( ICELIB_timerIsTimedOut( pTimer0) == true);

}
END_TEST


START_TEST (checklist)
{



}
END_TEST


Suite * icelib_suite (void)
{
  Suite *s = suite_create ("ICElib");

  /* Core test case */
  TCase *tc_core = tcase_create ("Core");
  //tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, create_ufrag);
  tcase_add_test (tc_core, create_passwd);
  tcase_add_test (tc_core, calculate_priority);
  tcase_add_test (tc_core, create_foundation);
  tcase_add_test (tc_core, pairPriority);
  tcase_add_test (tc_core, ice_timer);
  
  suite_add_tcase (s, tc_core);
  

  /* MediaStream test case */
  TCase *tc_mediaStream = tcase_create ("MediaStream");
  tcase_add_test (tc_mediaStream, create_localMediaStream);
  tcase_add_test (tc_mediaStream, create_remoteMediaStream);
  suite_add_tcase (s, tc_mediaStream);

  /* TriggeredCheck Queue test case */
  TCase *tc_trigcheck = tcase_create ("TriggeredCheck Queue");
  tcase_add_test (tc_trigcheck, triggereedcheck_queue);
  tcase_add_test (tc_trigcheck, triggereedcheck_queue_ekstra);
  suite_add_tcase (s, tc_trigcheck);


  /* Run ICELib test case */
  TCase *tc_runIcelib = tcase_create ("Run ICELib");
  tcase_add_unchecked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_runIcelib, checklist);

  suite_add_tcase (s, tc_runIcelib);


  return s;
}




int main(void){
    
    int number_failed;
    Suite *s = icelib_suite ();
    SRunner *sr = srunner_create (s);
    //srunner_set_fork_status(sr,CK_NOFORK); 
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    

}
