#include <stdlib.h>
#include <stdio.h>


#include "icelib.h"
#include <check.h>
#include "../src/icelib_intern.h"

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

ICELIB_Result ICELIB_TEST_sendConnectivityCheck( void                    *pUserData,
                                                 const struct sockaddr   *destination,
                                                 const struct sockaddr   *source,
                                                 uint32_t                userValue1,
                                                 uint32_t                userValue2,
                                                 uint32_t                componentId,
                                                 bool                    useRelay,
                                                 const char              *pUfrag,
                                                 const char              *pPasswd,
                                                 uint32_t                peerPriority,
                                                 bool                    useCandidate,
                                                 bool                    iceControlling,
                                                 bool                    iceControlled,
                                                 uint64_t                tieBreaker,
                                                 StunMsgId               transactionId);


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


typedef struct{
    bool gotCB;
    const struct sockaddr   *destination;
    const struct sockaddr   *source;
    uint32_t                userValue1;
    uint32_t                userValue2;
    uint32_t                componentId;
    bool                    useRelay;
    const char              *pUfrag;
    const char              *pPasswd;
    uint32_t                peerPriority;
    bool                    useCandidate;
    bool                    iceControlling;
    bool                    iceControlled;
    StunMsgId               transactionId;
}ConncheckCB;

ConncheckCB connChkCB;

void icelib_setup (void);
void icelib_teardown (void);
Suite * icelib_suite (void);

ICELIB_Result ICELIB_TEST_sendConnectivityCheck( void                    *pUserData,
                                                 const struct sockaddr   *destination,
                                                 const struct sockaddr   *source,
                                                 uint32_t                userValue1,
                                                 uint32_t                userValue2,
                                                 uint32_t                componentId,
                                                 bool                    useRelay,
                                                 const char              *pUfrag,
                                                 const char              *pPasswd,
                                                 uint32_t                peerPriority,
                                                 bool                    useCandidate,
                                                 bool                    iceControlling,
                                                 bool                    iceControlled,
                                                 uint64_t                tieBreaker,
                                                 StunMsgId               transactionId){
        
    connChkCB.gotCB = true;
    connChkCB.destination = destination;
    connChkCB.source = source;
    connChkCB.userValue1 = userValue1;
    connChkCB.userValue2 = userValue2;
    connChkCB.componentId = componentId;
    connChkCB.useRelay = useRelay;
    connChkCB.pUfrag = pUfrag;
    connChkCB.pPasswd = pPasswd;
    connChkCB.peerPriority = peerPriority;
    connChkCB.useCandidate = useCandidate;
    connChkCB.iceControlling = iceControlling;
    connChkCB.iceControlled = iceControlled;
    connChkCB.transactionId = transactionId;
    return 0;
}


ICELIB_INSTANCE *icelib;

static char remoteHostRtpAddr[] = "10.47.2.246:47936";
static char remoteHostRtcpAddr[] = "10.47.2.246:47937";
static char remoteRflxRtpAddr[] = "67.70.2.252:3807";
static char remoteRflxRtcpAddr[] = "67.70.2.252:32629";
static char remoteRelayRtpAddr[] = "93.95.67.89:52948";
static char remoteRelayRtcpAddr[] = "93.95.67.89:49832";

static char remoteUfrag[] = "remUf";
static char remotePasswd[] = "remPa";
static char localUfrag[] = "locUf";
static char localPasswd[] = "locPa";


void
icelib_setup (void)
{

    struct sockaddr_storage defaultAddr;
    struct sockaddr_storage localHostRtp;
    struct sockaddr_storage localHostRtcp;
    struct sockaddr_storage localRflxRtp;
    struct sockaddr_storage localRflxRtcp;
    struct sockaddr_storage localRelayRtp;
    struct sockaddr_storage localRelayRtcp;


    
    ICELIB_CONFIGURATION iceConfig;     

    icelib = (ICELIB_INSTANCE *)malloc(sizeof(ICELIB_INSTANCE));

    sockaddr_initFromString( (struct sockaddr *)&localHostRtp,  "192.168.2.10:3456");
    sockaddr_initFromString( (struct sockaddr *)&localHostRtcp, "192.168.2.10:3457");
    sockaddr_initFromString( (struct sockaddr *)&localRflxRtp,  "67.45.4.6:1045");
    sockaddr_initFromString( (struct sockaddr *)&localRflxRtcp, "67.45.4.6:3451");
    sockaddr_initFromString( (struct sockaddr *)&localRelayRtp, "158.38.46.10:2312");
    sockaddr_initFromString( (struct sockaddr *)&localRelayRtcp,"158.38.46.10:4567");

    
    iceConfig.tickIntervalMS = 20;
    iceConfig.keepAliveIntervalS = 15;
    iceConfig.maxCheckListPairs = ICELIB_MAX_PAIRS;
    iceConfig.aggressiveNomination = false;
    iceConfig.iceLite = false;
    //iceConfig.logLevel = ICELIB_logDebug;
    iceConfig.logLevel = ICELIB_logDisable;


    ICELIB_Constructor (icelib,
                        &iceConfig);
    
    ICELIB_setCallbackOutgoingBindingRequest(icelib,
                                             ICELIB_TEST_sendConnectivityCheck,
                                             NULL);

    //Local side
    ICELIB_addLocalMediaStream(icelib,
                               0,
                               42,
                               42,
                               ICE_CAND_TYPE_HOST);

    
    ICELIB_addLocalCandidate( icelib,
                              0,
                              1,
                              (struct sockaddr *)&localHostRtp,
                              NULL,
                              ICE_CAND_TYPE_HOST );
    
    ICELIB_addLocalCandidate( icelib,
                              0,
                              2,
                              (struct sockaddr *)&localHostRtcp,
                              NULL,
                              ICE_CAND_TYPE_HOST );
    
    
    //add rflx candidates
    
    
    ICELIB_addLocalCandidate( icelib,
                              0,
                              1,
                              (struct sockaddr *)&localRflxRtp,
                              (struct sockaddr *)&localHostRtp,
                              ICE_CAND_TYPE_SRFLX );
        
    ICELIB_addLocalCandidate( icelib,
                              0,
                              2,
                              (struct sockaddr *)&localRflxRtcp,
                              (struct sockaddr *)&localHostRtp,
                              ICE_CAND_TYPE_SRFLX );
    
        
    
    
    //add relay candidates
    ICELIB_addLocalCandidate( icelib,
                              0,
                              1,
                              (struct sockaddr *)&localRelayRtp,
                              (struct sockaddr *)&localRflxRtp,
                              ICE_CAND_TYPE_RELAY );
    
    ICELIB_addLocalCandidate( icelib,
                              0,
                              2,
                              (struct sockaddr *)&localRelayRtcp,
                              (struct sockaddr *)&localRflxRtcp,
                              ICE_CAND_TYPE_RELAY );
    
        
    ICELIB_setTurnState(icelib, 0, ICE_TURN_ALLOCATED);

    
    //Remote side
    sockaddr_initFromString( (struct sockaddr *)&defaultAddr,
                              "10.47.2.246:47936");


             

    ICELIB_addRemoteMediaStream(icelib,
                                remoteUfrag,
                                remotePasswd,
                                (struct sockaddr *)&defaultAddr);

    
    
    
    sockaddr_initFromString( (struct sockaddr *)&defaultAddr,
                             "0.0.0.0:0"); 

    
    
    
    ICELIB_addRemoteCandidate(icelib, 
                              0,
                              "1", 
                              1, 
                              1, 
                              2130706431,
                              remoteHostRtpAddr, 
                              47936,  
                              ICE_CAND_TYPE_HOST );

        
    ICELIB_addRemoteCandidate(icelib, 
                              0,
                              "1", 
                              1, 
                              2, 
                              2130706430,
                              remoteHostRtcpAddr, 
                              47937,  
                              ICE_CAND_TYPE_HOST );
    
    ICELIB_addRemoteCandidate(icelib, 
                              0,
                              "1", 
                              1, 
                              1, 
                              1694498815,
                              remoteRflxRtpAddr, 
                              3807,  
                              ICE_CAND_TYPE_SRFLX );

    ICELIB_addRemoteCandidate(icelib, 
                              0,
                              "3", 
                              1, 
                              2, 
                              1694498814,
                              remoteRflxRtcpAddr, 
                              32629,  
                              ICE_CAND_TYPE_SRFLX );

   
    
    ICELIB_addRemoteCandidate(icelib, 
                              0,
                              "4", 
                              1, 
                              1, 
                              16777215,
                              remoteRelayRtpAddr, 
                              52948,  
                              ICE_CAND_TYPE_RELAY );

    
    ICELIB_addRemoteCandidate(icelib, 
                              0,
                              "4", 
                              1, 
                              2, 
                              16777214,
                              remoteRelayRtcpAddr, 
                              49832,  
                              ICE_CAND_TYPE_RELAY );

    

}



void
icelib_teardown (void)
{
    ICELIB_Destructor (icelib);
    free(icelib);
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


    //localIceConfig.logLevel = ICELIB_logDebug;
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

    sockaddr_initFromString( (struct sockaddr *)&defaultAddr,
                              "10.47.2.246:47936");


    remoteIceConfig.logLevel = ICELIB_logDisable;
    //remoteIceConfig.logLevel = ICELIB_logDebug;

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
    
    expected = 0x12345678eca86420LL;
    fail_unless( priority == expected);

    G = 0x76543210;
    D = 0x12345678;
    priority = ICELIB_pairPriority( G, D);
    expected = 0x12345678eca86421LL;
    fail_unless( priority == expected);

    G = 0x11111111;
    D = 0x22222222;
    priority = ICELIB_pairPriority( G, D);
    expected = 0x1111111144444444LL;
    fail_unless( priority == expected);

    G = 0x22222222;
    D = 0x11111111;
    priority = ICELIB_pairPriority( G, D);
    expected = 0x1111111144444445LL;
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
    //config.logLevel             = 3;
    config.logLevel = ICELIB_logDebug;
    
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


START_TEST (controlling)
{

    fail_unless( ICELIB_Start(icelib, true) );
    fail_unless( icelib->numberOfMediaStreams == 1 );
    fail_unless( icelib->iceControlling == true );
    fail_unless( icelib->iceControlled == false );
}
END_TEST

START_TEST (initialState)
{
    fail_unless( icelib->iceState == ICELIB_IDLE );

    fail_unless( ICELIB_Start(icelib, true) );

    fail_unless( icelib->iceState == ICELIB_RUNNING );
}
END_TEST

START_TEST (iceSupportVerified)
{
    fail_if( icelib->iceSupportVerified );

    fail_unless( ICELIB_Start(icelib, true) );

    fail_unless( icelib->iceSupportVerified );
}
END_TEST


START_TEST (simpleTick)
{

    fail_unless( ICELIB_Start(icelib, true) );

    fail_unless( icelib->tickCount == 0 );

    ICELIB_Tick( icelib );

    fail_unless( icelib->tickCount == 1 );

    ICELIB_Tick( icelib );

    fail_unless( icelib->tickCount == 2 );

    


}
END_TEST


START_TEST (checklistInitialState)
{
    fail_unless( ICELIB_Start(icelib, true) );
    
    fail_unless( icelib->streamControllers[0].checkList.checkListState == ICELIB_CHECKLIST_RUNNING );

    fail_unless( icelib->streamControllers[0].checkList.numberOfPairs == 12 );

    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[0].pairState == ICELIB_PAIR_WAITING );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[1].pairState == ICELIB_PAIR_FROZEN );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[2].pairState == ICELIB_PAIR_FROZEN );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[3].pairState == ICELIB_PAIR_WAITING );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[4].pairState == ICELIB_PAIR_WAITING );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[5].pairState == ICELIB_PAIR_WAITING );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[6].pairState == ICELIB_PAIR_FROZEN );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[7].pairState == ICELIB_PAIR_WAITING );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[8].pairState == ICELIB_PAIR_FROZEN );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[9].pairState == ICELIB_PAIR_FROZEN );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[10].pairState == ICELIB_PAIR_WAITING );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[11].pairState == ICELIB_PAIR_FROZEN );

}
END_TEST


START_TEST (checklistTick)
{
    fail_unless( ICELIB_Start(icelib, true) );
    
    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[0].pairState == ICELIB_PAIR_INPROGRESS );

    
    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[3].pairState == ICELIB_PAIR_INPROGRESS );

    
    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[4].pairState == ICELIB_PAIR_INPROGRESS );

    
    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[5].pairState == ICELIB_PAIR_INPROGRESS );

    
    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[7].pairState == ICELIB_PAIR_INPROGRESS );

    ICELIB_Tick( icelib );    
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[10].pairState == ICELIB_PAIR_INPROGRESS );

    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[1].pairState == ICELIB_PAIR_INPROGRESS );
    
    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[2].pairState == ICELIB_PAIR_INPROGRESS );

    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[6].pairState == ICELIB_PAIR_INPROGRESS );

    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[8].pairState == ICELIB_PAIR_INPROGRESS );

    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[9].pairState == ICELIB_PAIR_INPROGRESS );

    ICELIB_Tick( icelib );
    fail_unless( icelib->streamControllers[0].checkList.checkListPairs[11].pairState == ICELIB_PAIR_INPROGRESS );

}
END_TEST


START_TEST (conncheck)
{
    /*
    connChkCB.destination = destination;
    connChkCB.source = source;;
    connChkCB.userValue1 = userValue1;
    connChkCB.userValue2 = userValue2;
    connChkCB.componentId = componentId;
    connChkCB.useRelay = useRelay;
    connChkCB.pUfrag = pUfrag;
    connChkCB.pPasswd = pPasswd;
    connChkCB.peerPriority = peerPriority;
    connChkCB.useCandidate = useCandidate;
    connChkCB.iceControlling = iceControlling;
    connChkCB.iceControlled = iceControlled;
    */
    char ipaddr[SOCKADDR_MAX_STRLEN];
    
    memset(&connChkCB, 0, sizeof(ConncheckCB));

    fail_unless( ICELIB_Start(icelib, true) );
    
    /* 1. Tick */
    
    ICELIB_Tick( icelib );
    fail_unless( strncmp(remoteHostRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed ('%s'", ipaddr); 

    fail_if( connChkCB.useRelay );
    
    /* 2. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRflxRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_if( connChkCB.useRelay );
    
    /* 3. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRelayRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteHostRtpAddr); 

    fail_if( connChkCB.useRelay );
    fail_if( connChkCB.useCandidate );
    
    /* 4. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteHostRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_unless( connChkCB.useRelay );
    

    /* 5. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRelayRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_unless( connChkCB.useRelay );
    

    /* 6. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRflxRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_unless( connChkCB.useRelay );

    /* 7. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteHostRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_if( connChkCB.useRelay );
    

    /* 8. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRflxRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_if( connChkCB.useRelay );
    

    /* 9. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRflxRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_unless( connChkCB.useRelay );


    /* 10. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRelayRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_if( connChkCB.useRelay );
    
    
    /* 11. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteHostRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_unless( connChkCB.useRelay );
    
    /* 12. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRelayRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_unless( connChkCB.useRelay );

    /* 13. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    fail_if( connChkCB.gotCB );

    /* 14. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    fail_if( connChkCB.gotCB );

}
END_TEST



START_TEST (conncheck_withIncomming)
{
    /*
    connChkCB.destination = destination;
    connChkCB.source = source;;
    connChkCB.userValue1 = userValue1;
    connChkCB.userValue2 = userValue2;
    connChkCB.componentId = componentId;
    connChkCB.useRelay = useRelay;
    connChkCB.pUfrag = pUfrag;
    connChkCB.pPasswd = pPasswd;
    connChkCB.peerPriority = peerPriority;
    connChkCB.useCandidate = useCandidate;
    connChkCB.iceControlling = iceControlling;
    connChkCB.iceControlled = iceControlled;
    */
    char ipaddr[SOCKADDR_MAX_STRLEN];
    char ufragPair[ICE_MAX_UFRAG_PAIR_LENGTH];
    StunMsgId stunId;
    char srcAddrStr[] = "10.47.1.23:52456";
    char respRflxAddrStr[] = "158.38.48.10:52423";
    
    struct sockaddr_storage srcAddr;
    struct sockaddr_storage dstAddr;

    struct sockaddr_storage respRflxAddr;

    memset(&connChkCB, 0, sizeof(ConncheckCB));

    fail_unless( ICELIB_Start(icelib, true) );
    
    /* 1. Tick */
    
    ICELIB_Tick( icelib );
    fail_unless( strncmp(remoteHostRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed ('%s'", ipaddr); 

    fail_if( connChkCB.useRelay );
    
    /* 2. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRflxRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_if( connChkCB.useRelay );
    
    /* 3. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRelayRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_if( connChkCB.useRelay );
    
    stunlib_createId(&stunId, 34, 3);
    sockaddr_initFromString( (struct sockaddr *)&srcAddr,  srcAddrStr);
    sockaddr_initFromString( (struct sockaddr *)&dstAddr,  "192.168.2.10:3456");
    ICELIB_getCheckListRemoteUsernamePair(ufragPair,
                                          ICE_MAX_UFRAG_PAIR_LENGTH,
                                          &icelib->streamControllers[0].checkList);
    ICELIB_incommingBindingRequest(icelib,
                                   1,
                                   2,
                                   ufragPair,
                                   connChkCB.peerPriority,
                                   false,
                                   false,
                                   true,
                                   45678,
                                   stunId,
                                   (struct sockaddr *)&srcAddr,
                                   (const struct sockaddr *)&dstAddr,
                                   false,
                                   NULL,
                                   0);
    


    /* 4. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(srcAddrStr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_if( connChkCB.useRelay );
    
    
    

    /* 5. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteHostRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteHostRtpAddr); 

    fail_unless( connChkCB.useRelay );
    fail_if( connChkCB.useCandidate );
    

    /* 6. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRelayRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRelayRtpAddr); 

    fail_unless( connChkCB.useRelay );

    /* 7. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRflxRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtcpAddr); 

    fail_unless( connChkCB.useRelay );
    

    /* 8. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteHostRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteHostRtcpAddr); 

    fail_if( connChkCB.useRelay );
    

    /* 9. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRflxRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_if( connChkCB.useRelay );
    sockaddr_initFromString( (struct sockaddr *)&respRflxAddr,  respRflxAddrStr);

    ICELIB_incommingBindingResponse(icelib,
                                    200,
                                    connChkCB.transactionId,
                                    connChkCB.destination,
                                    connChkCB.source,
                                    (struct sockaddr *)&respRflxAddr);

    




    /* 10. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRflxRtpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRflxRtpAddr); 

    fail_unless( connChkCB.useRelay );
    
    
    /* 11. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteRelayRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteRelayRtcpAddr); 

    fail_if( connChkCB.useRelay );
    
    /* 12. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    
    fail_unless( strncmp(remoteHostRtcpAddr, 
                         sockaddr_toString((const struct sockaddr *)connChkCB.destination, 
                                           ipaddr, 
                                           SOCKADDR_MAX_STRLEN, 
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed (Got:'%s' Expected: '%s')", ipaddr, remoteHostRtcpAddr); 

    fail_unless( connChkCB.useRelay );

    /* 13. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    //fail_if( connChkCB.gotCB );

    /* 14. Tick */
    memset(&connChkCB, 0, sizeof(ConncheckCB));
    ICELIB_Tick( icelib );
    fail_if( connChkCB.gotCB );

}
END_TEST





START_TEST( compareTransactionId )
{
    StunMsgId id1;
    StunMsgId id2;
    StunMsgId id3;
    
    stunlib_createId(&id1, 34, 3);


    memcpy(&id3, &id1, STUN_MSG_ID_SIZE);

    fail_unless( 0 == ICELIB_compareTransactionId(&id1,
                                                  &id3) );

    stunlib_createId(&id2, 43, 2);
    fail_if( 0 == ICELIB_compareTransactionId(&id1,
                                              &id2) );

}
END_TEST

START_TEST( makeTieBreaker )
{
    uint64_t tie1 = ICELIB_makeTieBreaker();
    uint64_t tie2 = ICELIB_makeTieBreaker();
    
    fail_if( tie1 == tie2 );
    

}
END_TEST

START_TEST( makeUserNamePair )
{
    char ufrag1[] = "ufr1";
    char ufrag2[] = "ufr2";
    
    char pair[ICE_MAX_UFRAG_PAIR_LENGTH];

    ICELIB_makeUsernamePair(pair,
                            ICE_MAX_UFRAG_PAIR_LENGTH,
                            ufrag1,
                            ufrag2);

    fail_unless( 0 == strncmp(pair, "ufr1:ufr2", ICE_MAX_UFRAG_PAIR_LENGTH ) );
    fail_if( 0 == strncmp(pair, "ufr2:ufr1", ICE_MAX_UFRAG_PAIR_LENGTH ) );

    ICELIB_makeUsernamePair(pair,
                            ICE_MAX_UFRAG_PAIR_LENGTH,
                            NULL,
                            ufrag2);
    fail_if( 0 == strncmp(pair, "ufr1:ufr2", ICE_MAX_UFRAG_PAIR_LENGTH ) );
    fail_unless( 0 == strncmp(pair, "--no_ufrags--", ICE_MAX_UFRAG_PAIR_LENGTH ) );


    ICELIB_makeUsernamePair(pair,
                            ICE_MAX_UFRAG_PAIR_LENGTH,
                            ufrag1,
                            NULL);
    fail_if( 0 == strncmp(pair, "ufr1:ufr2", ICE_MAX_UFRAG_PAIR_LENGTH ) );
    fail_unless( 0 == strncmp(pair, "--no_ufrags--", ICE_MAX_UFRAG_PAIR_LENGTH ) );

    ICELIB_makeUsernamePair(pair,
                            ICE_MAX_UFRAG_PAIR_LENGTH,
                            NULL,
                            NULL);
    fail_if( 0 == strncmp(pair, "ufr1:ufr2", ICE_MAX_UFRAG_PAIR_LENGTH ) );
    fail_unless( 0 == strncmp(pair, "--no_ufrags--", ICE_MAX_UFRAG_PAIR_LENGTH ) );




}
END_TEST


START_TEST( findCandidates )
{
    
    ICE_MEDIA_STREAM mediaStream;

    struct sockaddr_storage addr1;
    
    sockaddr_initFromString((struct sockaddr *)&addr1,
                            "10.47.1.34");

    ICELIBTYPES_ICE_MEDIA_STREAM_reset(&mediaStream);

    fail_unless( NULL ==pICELIB_findCandidate(&mediaStream,
                                              (struct sockaddr *)&addr1 ) );

    mediaStream.numberOfCandidates = 1;

    sockaddr_copy((struct sockaddr *)&mediaStream.candidate[0].connectionAddr,
                  (struct sockaddr *)&addr1 );


    fail_if( NULL ==pICELIB_findCandidate(&mediaStream,
                                          (struct sockaddr *)&addr1 ) );
    

}
END_TEST

START_TEST( splitUfragPair )
{
    char ufragPair[] = "ufr1:ufr2";
    size_t idx;

    fail_unless( 0 == strcmp(pICELIB_splitUfragPair(ufragPair, &idx), "ufr2"));

}
END_TEST


START_TEST( compareUfragPair )
{

    char pair[] = "ufrag1:ufrag2";
    char ufrag1[] = "ufrag1";
    char ufrag2[] = "ufrag2";

    fail_unless( ICELIB_compareUfragPair(pair,
                                         ufrag1,
                                         ufrag2) );

    fail_if( ICELIB_compareUfragPair("hei",
                                     ufrag1,
                                     ufrag2) );

    fail_if( ICELIB_compareUfragPair(pair,
                                     "hei",
                                     ufrag2) );

    fail_if( ICELIB_compareUfragPair(pair,
                                     ufrag1,
                                     "hei") );

    fail_if( ICELIB_compareUfragPair(NULL,
                                     ufrag1,
                                     ufrag2) );

    fail_if( ICELIB_compareUfragPair(pair,
                                     NULL,
                                     NULL) );
    

}
END_TEST


START_TEST( timerStop )
{
    ICELIB_TIMER timer;

    ICELIB_timerStop(&timer);

    fail_unless( timer.timerState == ICELIB_timerStopped );

}
END_TEST

START_TEST( createRandomString )
{
    char randomStr[128];

    ICELIB_createRandomString(randomStr, 127);    

    fail_unless( randomStr[127] == '\0' );

    
}
END_TEST

START_TEST( longToIceChar )
{
    char b64[6];
    long data = 45678944;
    ICELIB_longToIcechar(data, b64, 5);

    fail_unless( b64[0] == 'g');
    fail_unless( b64[1] == 'F');
    fail_unless( b64[2] == 'Q');
    fail_unless( b64[3] == 'u');
    fail_unless( b64[4] == 'C');

}
END_TEST

START_TEST( isEmptyCandidate )
{

    ICE_CANDIDATE candidate;


    candidate.foundation[0] = '\0';

    fail_unless( ICELIB_isEmptyCandidate(&candidate) );

    strcpy(candidate.foundation, "ert");

    fail_if( ICELIB_isEmptyCandidate(&candidate) );

    ICELIB_resetCandidate(&candidate);
    fail_unless( ICELIB_isEmptyCandidate(&candidate) );
    
    
}
END_TEST


START_TEST( toStringCheckListState )
{

    fail_unless( 0 == strcmp(ICELIB_toString_CheckListState(ICELIB_CHECKLIST_IDLE),
                             "ICELIB_CHECKLIST_IDLE") );

    fail_unless( 0 == strcmp(ICELIB_toString_CheckListState(ICELIB_CHECKLIST_RUNNING),
                             "ICELIB_CHECKLIST_RUNNING") );

    fail_unless( 0 == strcmp(ICELIB_toString_CheckListState(ICELIB_CHECKLIST_COMPLETED),
                             "ICELIB_CHECKLIST_COMPLETED") );

    fail_unless( 0 == strcmp(ICELIB_toString_CheckListState(ICELIB_CHECKLIST_FAILED),
                             "ICELIB_CHECKLIST_FAILED") );

    fail_unless( 0 == strcmp(ICELIB_toString_CheckListState(7),
                             "--unknown--") );
    
}
END_TEST


START_TEST( toStringCheckListPairState )
{
    fail_unless( 0 == strcmp(ICELIB_toString_CheckListPairState(ICELIB_PAIR_IDLE),
                             "IDLE") );
    
    fail_unless( 0 == strcmp(ICELIB_toString_CheckListPairState(ICELIB_PAIR_PAIRED),
                             "PAIRED") );
    
    fail_unless( 0 == strcmp(ICELIB_toString_CheckListPairState(ICELIB_PAIR_FROZEN),
                             "FROZEN") );
    
    fail_unless( 0 == strcmp(ICELIB_toString_CheckListPairState(ICELIB_PAIR_WAITING),
                             "WAITING") );
    
    fail_unless( 0 == strcmp(ICELIB_toString_CheckListPairState(ICELIB_PAIR_INPROGRESS),
                             "INPROGRESS") );
    
    fail_unless( 0 == strcmp(ICELIB_toString_CheckListPairState(ICELIB_PAIR_SUCCEEDED),
                             "SUCCEEDED" ) );
    
    fail_unless( 0 == strcmp(ICELIB_toString_CheckListPairState(ICELIB_PAIR_FAILED),
                             "FAILED") );
    
    fail_unless( 0 == strcmp(ICELIB_toString_CheckListPairState(10),
                             "--unknown--") );
    
}
END_TEST


START_TEST( removeChecksFromCheckList )
{
    ICELIB_CHECKLIST checkList;

    ICELIB_removChecksFromCheckList(&checkList);
    
    fail_unless( checkList.numberOfPairs == 0 );
    
    fail_unless( checkList.nextPairId == 0 );
}
END_TEST

START_TEST( resetCandidate )
{
    ICE_CANDIDATE candidate;
    ICELIB_resetCandidate(&candidate);
    fail_unless( ICELIB_isEmptyCandidate(&candidate) );
}
END_TEST

START_TEST( formPairs_IPv4 )
{
    
    ICELIB_CHECKLIST CheckList;
    ICELIB_CALLBACK_LOG CallbackLog;
    ICE_MEDIA_STREAM LocalMediaStream;
    ICE_MEDIA_STREAM RemoteMediaStream;
    unsigned int     maxPairs;
    
    ICE_CANDIDATE *cand;

    /* set up addresses */
    struct sockaddr_storage localHost;
    struct sockaddr_storage localRelay;
    struct sockaddr_storage remoteHost;
    struct sockaddr_storage remoteRelay;

    sockaddr_initFromString( (struct sockaddr *)&localHost,  "192.168.2.10:3456");
    sockaddr_initFromString( (struct sockaddr *)&localRelay,  "158.38.48.10:4534");
    sockaddr_initFromString( (struct sockaddr *)&remoteHost,  "192.168.2.10:3459");
    sockaddr_initFromString( (struct sockaddr *)&remoteRelay,  "8.8.8.8:4444");
    
    ICELIBTYPES_ICE_MEDIA_STREAM_reset(&LocalMediaStream);
    ICELIBTYPES_ICE_MEDIA_STREAM_reset(&RemoteMediaStream);

    cand = &LocalMediaStream.candidate[0];

 
    //Local 
    ICELIB_fillLocalCandidate(cand,
                              1,
                              (struct sockaddr *)&localHost,
                              NULL,
                              ICE_CAND_TYPE_HOST);
    LocalMediaStream.numberOfCandidates++;
    
    cand = &LocalMediaStream.candidate[1];

    fflush(stdout);

    ICELIB_fillLocalCandidate(cand,
                              1,
                              (struct sockaddr *)&localRelay,
                              NULL,
                              ICE_CAND_TYPE_RELAY);
    LocalMediaStream.numberOfCandidates++;
    
    

    qsort(LocalMediaStream.candidate,
          LocalMediaStream.numberOfCandidates,
          sizeof(ICE_CANDIDATE),
          ICELIB_candidateSort);
    //Remote

    fflush(stdout);

    cand = &RemoteMediaStream.candidate[0];
    
    ICELIB_fillRemoteCandidate(cand,
                               1,
                               "TJA",
                               3,
                               2130706431,
                               (struct sockaddr *)&remoteHost,
                               ICE_CAND_TYPE_HOST);
    RemoteMediaStream.numberOfCandidates++;
    
    cand = &RemoteMediaStream.candidate[1];


    fflush(stdout);


    ICELIB_fillRemoteCandidate(cand,
                               1,
                               "TJA",
                               3,
                               30706431,
                               (struct sockaddr *)&remoteRelay,
                               ICE_CAND_TYPE_RELAY);
    RemoteMediaStream.numberOfCandidates++;


    ICELIB_resetCheckList(&CheckList, 0);

    ICELIB_formPairs(&CheckList,
                     NULL,
                     &LocalMediaStream,
                     &RemoteMediaStream,
                     10);

    fflush(stdout);


    ICELIB_computeListPairPriority(&CheckList, true);
    ICELIB_sortPairsCL(&CheckList);

    
    fflush(stdout);


    fail_unless( CheckList.numberOfPairs  == 4 );

    //Check pair 0 
    fail_unless( CheckList.checkListPairs[0].pairState == ICELIB_PAIR_PAIRED );
    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[0].pLocalCandidate->connectionAddr,
                                   (struct sockaddr *)&localHost),
                 "wrong pair");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[0].pRemoteCandidate->connectionAddr,
                                   (struct sockaddr *)&remoteHost),
                 "wrong pair");


    //Check pair 1 
    fail_unless( CheckList.checkListPairs[1].pairState == ICELIB_PAIR_PAIRED );
    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[1].pLocalCandidate->connectionAddr,
                                   (struct sockaddr *)&localHost),
                 "wrong pair");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[1].pRemoteCandidate->connectionAddr,
                                   (struct sockaddr *)&remoteRelay),
                 "wrong pair");

    //Check pair 2 
    fail_unless( CheckList.checkListPairs[2].pairState == ICELIB_PAIR_PAIRED );
    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[2].pLocalCandidate->connectionAddr,
                                   (struct sockaddr *)&localRelay),
                 "wrong pair");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[2].pRemoteCandidate->connectionAddr,
                                   (struct sockaddr *)&remoteHost),
                 "wrong pair");

    //Check pair 3 
    fail_unless( CheckList.checkListPairs[3].pairState == ICELIB_PAIR_PAIRED );
    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[3].pLocalCandidate->connectionAddr,
                                   (struct sockaddr *)&localRelay),
                 "wrong pair");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[3].pRemoteCandidate->connectionAddr,
                                   (struct sockaddr *)&remoteRelay),
                 "wrong pair");

    //ICELIB_checkListDump(&CheckList);
    //fflush(stdout);
     
}
END_TEST


START_TEST( formPairs_IPv6 )
{
    
    ICELIB_CHECKLIST CheckList;
    ICELIB_CALLBACK_LOG CallbackLog;
    ICE_MEDIA_STREAM LocalMediaStream;
    ICE_MEDIA_STREAM RemoteMediaStream;
    unsigned int     maxPairs;
    
    ICE_CANDIDATE *cand;

    /* set up addresses */
    struct sockaddr_storage localHost_6;
    struct sockaddr_storage localRelay_6;
    struct sockaddr_storage remoteHost_6;
    struct sockaddr_storage remoteRelay_6;

    
    sockaddr_initFromString( (struct sockaddr *)&localHost_6,  "[2001:420:4:eb66:119a:ddff:fe3a:27d1]:2345");
    sockaddr_initFromString( (struct sockaddr *)&localRelay_6, "[2001:420:2:ea63:119a:ddff:fe3a:27d1]:6789");
    sockaddr_initFromString( (struct sockaddr *)&remoteHost_6, "[2001:421:4:eb46:119a:ddff:fe1a:27d4]:4381");
    sockaddr_initFromString( (struct sockaddr *)&remoteRelay_6, "[2001:420:2:eb66:119a:ddff:fe3a:27d0]:2176");

    
    ICELIBTYPES_ICE_MEDIA_STREAM_reset(&LocalMediaStream);
    ICELIBTYPES_ICE_MEDIA_STREAM_reset(&RemoteMediaStream);

    cand = &LocalMediaStream.candidate[0];

 
    //Local 
    ICELIB_fillLocalCandidate(cand,
                              1,
                              (struct sockaddr *)&localHost_6,
                              NULL,
                              ICE_CAND_TYPE_HOST);
    LocalMediaStream.numberOfCandidates++;
    
    cand = &LocalMediaStream.candidate[1];

    ICELIB_fillLocalCandidate(cand,
                              1,
                              (struct sockaddr *)&localRelay_6,
                              NULL,
                              ICE_CAND_TYPE_RELAY);
    LocalMediaStream.numberOfCandidates++;
    


    qsort(LocalMediaStream.candidate,
          LocalMediaStream.numberOfCandidates,
          sizeof(ICE_CANDIDATE),
          ICELIB_candidateSort);
    //Remote
    cand = &RemoteMediaStream.candidate[0];
    
    ICELIB_fillRemoteCandidate(cand,
                               1,
                               "TJA",
                               3,
                               2130706431,
                               (struct sockaddr *)&remoteHost_6,
                               ICE_CAND_TYPE_HOST);
    RemoteMediaStream.numberOfCandidates++;
    
    cand = &RemoteMediaStream.candidate[1];

    ICELIB_fillRemoteCandidate(cand,
                               1,
                               "TJA",
                               3,
                               30706431,
                               (struct sockaddr *)&remoteRelay_6,
                               ICE_CAND_TYPE_RELAY);
    RemoteMediaStream.numberOfCandidates++;
    

    ICELIB_resetCheckList(&CheckList, 0);

    ICELIB_formPairs(&CheckList,
                     NULL,
                     &LocalMediaStream,
                     &RemoteMediaStream,
                     10);

    ICELIB_computeListPairPriority(&CheckList, true);
    ICELIB_sortPairsCL(&CheckList);

    fail_unless( CheckList.numberOfPairs  == 4 );

    //Check pair 0 
    fail_unless( CheckList.checkListPairs[0].pairState == ICELIB_PAIR_PAIRED );
    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[0].pLocalCandidate->connectionAddr,
                                   (struct sockaddr *)&localHost_6),
                 "wrong pair");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[0].pRemoteCandidate->connectionAddr,
                                   (struct sockaddr *)&remoteHost_6),
                 "wrong pair");


    //Check pair 1 
    fail_unless( CheckList.checkListPairs[1].pairState == ICELIB_PAIR_PAIRED );
    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[1].pLocalCandidate->connectionAddr,
                                   (struct sockaddr *)&localHost_6),
                 "wrong pair");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[1].pRemoteCandidate->connectionAddr,
                                   (struct sockaddr *)&remoteRelay_6),
                 "wrong pair");

    //Check pair 2 
    fail_unless( CheckList.checkListPairs[2].pairState == ICELIB_PAIR_PAIRED );
    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[2].pLocalCandidate->connectionAddr,
                                   (struct sockaddr *)&localRelay_6),
                 "wrong pair");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[2].pRemoteCandidate->connectionAddr,
                                   (struct sockaddr *)&remoteHost_6),
                 "wrong pair");

    //Check pair 3 
    fail_unless( CheckList.checkListPairs[3].pairState == ICELIB_PAIR_PAIRED );
    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[3].pLocalCandidate->connectionAddr,
                                   (struct sockaddr *)&localRelay_6),
                 "wrong pair");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)&CheckList.checkListPairs[3].pRemoteCandidate->connectionAddr,
                                   (struct sockaddr *)&remoteRelay_6),
                 "wrong pair");

    //ICELIB_checkListDump(&CheckList);
    //fflush(stdout);
     
}
END_TEST




Suite * icelib_suite (void)
{
  Suite *s = suite_create ("ICElib");

  {/* Core test case */
      TCase *tc_core = tcase_create ("Core");
      tcase_add_test (tc_core, create_ufrag);
      tcase_add_test (tc_core, create_passwd);
      tcase_add_test (tc_core, calculate_priority);
      tcase_add_test (tc_core, create_foundation);
      tcase_add_test (tc_core, pairPriority);
      tcase_add_test (tc_core, ice_timer);
      tcase_add_test (tc_core, compareTransactionId );
      tcase_add_test (tc_core, makeTieBreaker );
      tcase_add_test (tc_core, makeUserNamePair );
      tcase_add_test (tc_core, findCandidates );
      tcase_add_test (tc_core, splitUfragPair );
      tcase_add_test (tc_core, compareUfragPair );
      tcase_add_test (tc_core, timerStop );
      tcase_add_test (tc_core, createRandomString );
      tcase_add_test (tc_core, longToIceChar );
      tcase_add_test (tc_core, isEmptyCandidate );
      tcase_add_test (tc_core, toStringCheckListState );
      tcase_add_test (tc_core, toStringCheckListPairState );
      tcase_add_test (tc_core, removeChecksFromCheckList );
      tcase_add_test (tc_core, resetCandidate );
      tcase_add_test (tc_core, formPairs_IPv4 );
      tcase_add_test (tc_core, formPairs_IPv6 );
      suite_add_tcase (s, tc_core);
  }

  {/* MediaStream test case */
      TCase *tc_mediaStream = tcase_create ("MediaStream");
      tcase_add_test (tc_mediaStream, create_localMediaStream);
      tcase_add_test (tc_mediaStream, create_remoteMediaStream);
      suite_add_tcase (s, tc_mediaStream);
  }

  {/* TriggeredCheck Queue test case */
      TCase *tc_trigcheck = tcase_create ("TriggeredCheck Queue");
      tcase_add_test (tc_trigcheck, triggereedcheck_queue);
      tcase_add_test (tc_trigcheck, triggereedcheck_queue_ekstra);
      suite_add_tcase (s, tc_trigcheck);
  }

  {/* Run ICELib test case */
      TCase *tc_runIcelib = tcase_create ("Run ICELib");
      tcase_add_checked_fixture (tc_runIcelib, icelib_setup, icelib_teardown);
      
      tcase_add_test (tc_runIcelib, controlling);
      tcase_add_test (tc_runIcelib, initialState);
      tcase_add_test (tc_runIcelib, iceSupportVerified);
      tcase_add_test (tc_runIcelib, simpleTick);
      tcase_add_test (tc_runIcelib, checklistInitialState);
      tcase_add_test (tc_runIcelib, checklistTick);
      tcase_add_test (tc_runIcelib, conncheck);
      tcase_add_test (tc_runIcelib, conncheck_withIncomming );
      
      suite_add_tcase (s, tc_runIcelib);
  }

  return s;
}
