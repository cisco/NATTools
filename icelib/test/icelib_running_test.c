#include <stdlib.h>
#include <stdio.h>
#include <time.h>


#include <check.h>

#include "icelib.h"


ICELIB_INSTANCE *m_icelib;

static char m0_remoteHostRtpAddr[] = "10.47.2.246:47936";
static char m0_remoteHostRtcpAddr[] = "10.47.2.246:47937";
static char m0_remoteRflxRtpAddr[] = "67.70.2.252:3807";
static char m0_remoteRflxRtcpAddr[] = "67.70.2.252:32629";
static char m0_remoteRelayRtpAddr[] = "93.95.67.89:52948";
static char m0_remoteRelayRtcpAddr[] = "93.95.67.89:49832";
static char m0_remoteUfrag[] = "rm0Uf";
static char m0_remotePasswd[] = "rm0Pa";

static char m1_remoteHostRtpAddr[] = "10.47.2.246:47938";
static char m1_remoteHostRtcpAddr[] = "10.47.2.246:47339";
static char m1_remoteRflxRtpAddr[] = "67.70.2.252:38071";
static char m1_remoteRflxRtcpAddr[] = "67.70.2.252:32529";
static char m1_remoteRelayRtpAddr[] = "93.95.67.89:52348";
static char m1_remoteRelayRtcpAddr[] = "93.95.67.89:12832";
static char m1_remoteUfrag[] = "rm1Uf";
static char m1_remotePasswd[] = "rm1Pa";

static char m2_remoteHostRtpAddr[] = "10.47.2.246:47940";
static char m2_remoteHostRtcpAddr[] = "10.47.2.246:47941";
static char m2_remoteRflxRtpAddr[] = "67.70.2.252:38076";
static char m2_remoteRflxRtcpAddr[] = "67.70.2.252:32839";
static char m2_remoteRelayRtpAddr[] = "93.95.67.89:52128";
static char m2_remoteRelayRtcpAddr[] = "93.95.67.89:41232";
static char m2_remoteUfrag[] = "rm2Uf";
static char m2_remotePasswd[] = "rm2Pa";




typedef struct{
    bool gotCB;
    const struct sockaddr   *destination;
    const struct sockaddr   *source;
    uint32_t                userValue1;
    uint32_t                userValue2;
    uint32_t                componentId;
    bool                    useRelay;
    char ufrag[ ICE_MAX_UFRAG_LENGTH];
    //const char              *pUfrag;
    const char              *pPasswd;
    uint32_t                peerPriority;
    bool                    useCandidate;
    bool                    iceControlling;
    bool                    iceControlled;
    uint64_t                tieBreaker;
    StunMsgId               transactionId;
}m_ConncheckCB;

m_ConncheckCB m_connChkCB;

ICELIB_RUNNING_TEST_printLog( void                    *pUserData,
                              int                     loglevel,
                              char                    *str){
    printf("%s\n", str);

}

ICELIB_Result ICELIB_RUNNING_TEST_sendConnectivityCheck( void                    *pUserData,
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
    
    m_connChkCB.gotCB = true;
    m_connChkCB.destination = destination;
    m_connChkCB.source = source;
    m_connChkCB.userValue1 = userValue1;
    m_connChkCB.userValue2 = userValue2;
    m_connChkCB.componentId = componentId;
    m_connChkCB.useRelay = useRelay;
    strncpy(m_connChkCB.ufrag, pUfrag, ICE_MAX_UFRAG_LENGTH);
    m_connChkCB.pPasswd = pPasswd;
    m_connChkCB.peerPriority = peerPriority;
    m_connChkCB.useCandidate = useCandidate;
    m_connChkCB.iceControlling = iceControlling;
    m_connChkCB.iceControlled = iceControlled;
    m_connChkCB.transactionId = transactionId;
    m_connChkCB.tieBreaker = tieBreaker;
    return 0;
}



void
icelib_medialines_setup (void)
{

    struct sockaddr_storage m0_defaultAddr;
    struct sockaddr_storage m0_localHostRtp;
    struct sockaddr_storage m0_localHostRtcp;
    struct sockaddr_storage m0_localRflxRtp;
    struct sockaddr_storage m0_localRflxRtcp;
    struct sockaddr_storage m0_localRelayRtp;
    struct sockaddr_storage m0_localRelayRtcp;

    struct sockaddr_storage m1_defaultAddr;
    struct sockaddr_storage m1_localHostRtp;
    struct sockaddr_storage m1_localHostRtcp;
    struct sockaddr_storage m1_localRflxRtp;
    struct sockaddr_storage m1_localRflxRtcp;
    struct sockaddr_storage m1_localRelayRtp;
    struct sockaddr_storage m1_localRelayRtcp;

    struct sockaddr_storage m2_defaultAddr;
    struct sockaddr_storage m2_localHostRtp;
    struct sockaddr_storage m2_localHostRtcp;
    struct sockaddr_storage m2_localRflxRtp;
    struct sockaddr_storage m2_localRflxRtcp;
    struct sockaddr_storage m2_localRelayRtp;
    struct sockaddr_storage m2_localRelayRtcp;

    
    ICELIB_CONFIGURATION iceConfig;     

    uint32_t mediaIdx;
    
    srand(time(NULL));

    m_icelib = (ICELIB_INSTANCE *)malloc(sizeof(ICELIB_INSTANCE));

    //------------ Media Line: 0
    sockaddr_initFromString( (struct sockaddr *)&m0_localHostRtp,  "192.168.2.10:3456");
    sockaddr_initFromString( (struct sockaddr *)&m0_localHostRtcp, "192.168.2.10:3457");
    sockaddr_initFromString( (struct sockaddr *)&m0_localRflxRtp,  "67.45.4.6:1045");
    sockaddr_initFromString( (struct sockaddr *)&m0_localRflxRtcp, "67.45.4.6:3451");
    sockaddr_initFromString( (struct sockaddr *)&m0_localRelayRtp, "158.38.46.10:2312");
    sockaddr_initFromString( (struct sockaddr *)&m0_localRelayRtcp,"158.38.46.10:4567");

    //------------ Media Line: 1
    sockaddr_initFromString( (struct sockaddr *)&m1_localHostRtp,  "192.168.2.10:3458");
    sockaddr_initFromString( (struct sockaddr *)&m1_localHostRtcp, "192.168.2.10:3459");
    sockaddr_initFromString( (struct sockaddr *)&m1_localRflxRtp,  "67.45.4.6:6123");
    sockaddr_initFromString( (struct sockaddr *)&m1_localRflxRtcp, "67.45.4.6:5831");
    sockaddr_initFromString( (struct sockaddr *)&m1_localRelayRtp, "158.38.46.10:2156");
    sockaddr_initFromString( (struct sockaddr *)&m1_localRelayRtcp,"158.38.46.10:6734");

    //------------ Media Line: 2
    sockaddr_initFromString( (struct sockaddr *)&m2_localHostRtp,  "192.168.2.10:3460");
    sockaddr_initFromString( (struct sockaddr *)&m2_localHostRtcp, "192.168.2.10:3461");
    sockaddr_initFromString( (struct sockaddr *)&m2_localRflxRtp,  "67.45.4.6:4532");
    sockaddr_initFromString( (struct sockaddr *)&m2_localRflxRtcp, "67.45.4.6:45678");
    sockaddr_initFromString( (struct sockaddr *)&m2_localRelayRtp, "158.38.46.10:56783");
    sockaddr_initFromString( (struct sockaddr *)&m2_localRelayRtcp,"158.38.46.10:23451");


    
    iceConfig.tickIntervalMS = 20;
    iceConfig.keepAliveIntervalS = 15;
    iceConfig.maxCheckListPairs = ICELIB_MAX_PAIRS;
    iceConfig.aggressiveNomination = false;
    iceConfig.iceLite = false;
    iceConfig.logLevel = ICELIB_logDebug;
    //iceConfig.logLevel = ICELIB_logDisable;


    ICELIB_Constructor (m_icelib,
                        &iceConfig);
    
    ICELIB_setCallbackOutgoingBindingRequest(m_icelib,
                                             ICELIB_RUNNING_TEST_sendConnectivityCheck,
                                             NULL);

    ICELIB_setCallbackLog(m_icelib,
                          ICELIB_RUNNING_TEST_printLog,
                          NULL,
                          ICELIB_logDebug);

    //Local side
    //Medialine: 0
    mediaIdx = ICELIB_addLocalMediaStream( m_icelib, 42, 42, ICE_CAND_TYPE_HOST);
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 1, (struct sockaddr *)&m0_localHostRtp, NULL, ICE_CAND_TYPE_HOST, 0xffff ); 
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 2, (struct sockaddr *)&m0_localHostRtcp, NULL, ICE_CAND_TYPE_HOST, 0xffff );
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 1, (struct sockaddr *)&m0_localRflxRtp, (struct sockaddr *)&m0_localHostRtp, ICE_CAND_TYPE_SRFLX, 0xffff);        
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 2, (struct sockaddr *)&m0_localRflxRtcp, (struct sockaddr *)&m0_localHostRtp, ICE_CAND_TYPE_SRFLX, 0xffff );
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 1, (struct sockaddr *)&m0_localRelayRtp, (struct sockaddr *)&m0_localRflxRtp, ICE_CAND_TYPE_RELAY, 0xffff );    
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 2, (struct sockaddr *)&m0_localRelayRtcp, (struct sockaddr *)&m0_localRflxRtcp, ICE_CAND_TYPE_RELAY, 0xffff );
    
    //Medialine: 1
    mediaIdx = ICELIB_addLocalMediaStream( m_icelib, 42, 42, ICE_CAND_TYPE_HOST);
    
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 1, (struct sockaddr *)&m1_localHostRtp, NULL, ICE_CAND_TYPE_HOST, 0xffff ); 
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 2, (struct sockaddr *)&m1_localHostRtcp, NULL, ICE_CAND_TYPE_HOST, 0xffff );
    
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 1, (struct sockaddr *)&m1_localRflxRtp, (struct sockaddr *)&m1_localHostRtp, ICE_CAND_TYPE_SRFLX, 0xffff);        
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 2, (struct sockaddr *)&m1_localRflxRtcp, (struct sockaddr *)&m1_localHostRtp, ICE_CAND_TYPE_SRFLX, 0xffff );
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 1, (struct sockaddr *)&m1_localRelayRtp, (struct sockaddr *)&m1_localRflxRtp, ICE_CAND_TYPE_RELAY, 0xffff );    
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 2, (struct sockaddr *)&m1_localRelayRtcp, (struct sockaddr *)&m1_localRflxRtcp, ICE_CAND_TYPE_RELAY, 0xffff );
    
    //Medialine: 2
    mediaIdx = ICELIB_addLocalMediaStream( m_icelib, 42, 42, ICE_CAND_TYPE_HOST);
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 1, (struct sockaddr *)&m2_localHostRtp, NULL, ICE_CAND_TYPE_HOST, 0xffff ); 
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 2, (struct sockaddr *)&m2_localHostRtcp, NULL, ICE_CAND_TYPE_HOST, 0xffff );
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 1, (struct sockaddr *)&m2_localRflxRtp, (struct sockaddr *)&m2_localHostRtp, ICE_CAND_TYPE_SRFLX, 0xffff);        
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 2, (struct sockaddr *)&m2_localRflxRtcp, (struct sockaddr *)&m2_localHostRtp, ICE_CAND_TYPE_SRFLX, 0xffff );
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 1, (struct sockaddr *)&m2_localRelayRtp, (struct sockaddr *)&m2_localRflxRtp, ICE_CAND_TYPE_RELAY, 0xffff );    
    ICELIB_addLocalCandidate( m_icelib, mediaIdx, 2, (struct sockaddr *)&m2_localRelayRtcp, (struct sockaddr *)&m2_localRflxRtcp, ICE_CAND_TYPE_RELAY, 0xffff );
    
        
    ICELIB_setTurnState( m_icelib, 0, ICE_TURN_ALLOCATED);

    
    //Remote side
    //Medialine: 0
    sockaddr_initFromString( (struct sockaddr *)&m0_defaultAddr,
                              "10.47.2.246:47936");

    ICELIB_addRemoteMediaStream( m_icelib, m0_remoteUfrag, m0_remotePasswd, (struct sockaddr *)&m0_defaultAddr);        
    ICELIB_addRemoteCandidate( m_icelib, 0, "1", 1, 1, 2130706431, m0_remoteHostRtpAddr, 47936, ICE_CAND_TYPE_HOST );
    ICELIB_addRemoteCandidate( m_icelib, 0, "1", 1, 2, 2130706430, m0_remoteHostRtcpAddr, 47937, ICE_CAND_TYPE_HOST );
    
    ICELIB_addRemoteCandidate( m_icelib, 0, "3", 1, 1, 1694498815, m0_remoteRflxRtpAddr, 3807, ICE_CAND_TYPE_SRFLX );
    ICELIB_addRemoteCandidate( m_icelib, 0, "3", 1, 2, 1694498814, m0_remoteRflxRtcpAddr, 32629, ICE_CAND_TYPE_SRFLX );

    ICELIB_addRemoteCandidate( m_icelib, 0, "4", 1, 1, 16777215, m0_remoteRelayRtpAddr, 52948, ICE_CAND_TYPE_RELAY );
    ICELIB_addRemoteCandidate( m_icelib, 0, "4", 1, 2, 16777214, m0_remoteRelayRtcpAddr, 49832, ICE_CAND_TYPE_RELAY );

    //Medialine: 1
    sockaddr_initFromString( (struct sockaddr *)&m1_defaultAddr,
                              "10.47.2.246:47938");

    ICELIB_addRemoteMediaStream( m_icelib, m1_remoteUfrag, m1_remotePasswd, (struct sockaddr *)&m1_defaultAddr);        
    ICELIB_addRemoteCandidate( m_icelib, 1, "1", 1, 1, 2130706431, m1_remoteHostRtpAddr, 47938, ICE_CAND_TYPE_HOST );
    ICELIB_addRemoteCandidate( m_icelib, 1, "1", 1, 2, 2130706430, m1_remoteHostRtcpAddr, 47939, ICE_CAND_TYPE_HOST );
    
    ICELIB_addRemoteCandidate( m_icelib, 1, "3", 1, 1, 1694498815, m1_remoteRflxRtpAddr, 38071, ICE_CAND_TYPE_SRFLX );
    ICELIB_addRemoteCandidate( m_icelib, 1, "3", 1, 2, 1694498814, m1_remoteRflxRtcpAddr, 32529, ICE_CAND_TYPE_SRFLX );

    ICELIB_addRemoteCandidate( m_icelib, 1, "4", 1, 1, 16777215, m1_remoteRelayRtpAddr, 52348, ICE_CAND_TYPE_RELAY );
    ICELIB_addRemoteCandidate( m_icelib, 1, "4", 1, 2, 16777214, m1_remoteRelayRtcpAddr, 12832, ICE_CAND_TYPE_RELAY );

    //Medialine: 2
    sockaddr_initFromString( (struct sockaddr *)&m2_defaultAddr,
                              "10.47.2.246:47940");

    ICELIB_addRemoteMediaStream( m_icelib, m2_remoteUfrag, m2_remotePasswd, (struct sockaddr *)&m2_defaultAddr);        
    ICELIB_addRemoteCandidate( m_icelib, 2, "1", 1, 1, 2130706431, m2_remoteHostRtpAddr, 47940, ICE_CAND_TYPE_HOST );
    ICELIB_addRemoteCandidate( m_icelib, 2, "1", 1, 2, 2130706430, m2_remoteHostRtcpAddr, 47941, ICE_CAND_TYPE_HOST );
    
    ICELIB_addRemoteCandidate( m_icelib, 2, "3", 1, 1, 1694498815, m2_remoteRflxRtpAddr, 38076, ICE_CAND_TYPE_SRFLX );
    ICELIB_addRemoteCandidate( m_icelib, 2, "3", 1, 2, 1694498814, m2_remoteRflxRtcpAddr, 32839, ICE_CAND_TYPE_SRFLX );

    ICELIB_addRemoteCandidate( m_icelib, 2, "4", 1, 1, 16777215, m2_remoteRelayRtpAddr, 52128, ICE_CAND_TYPE_RELAY );
    ICELIB_addRemoteCandidate( m_icelib, 2, "4", 1, 2, 16777214, m2_remoteRelayRtcpAddr, 41232, ICE_CAND_TYPE_RELAY );


}



void
icelib_medialines_teardown (void)
{
    ICELIB_Destructor (m_icelib);
    free(m_icelib);
}


START_TEST (multiple_medialines)
{
    int i;
    
    memset(&m_connChkCB, 0, sizeof(m_ConncheckCB));

    fail_unless( ICELIB_Start(m_icelib, true) );
    

    for(i=0;i<10000;i++){ 
        ICELIB_Tick( m_icelib );
        if(m_connChkCB.gotCB) {
            //We pretend to be the perfect network. Responses arrive imediately!
            
            ICELIB_incomingBindingResponse(m_icelib,
                                           200,
                                           m_connChkCB.transactionId,
                                           m_connChkCB.destination,
                                           m_connChkCB.source,
                                           m_connChkCB.source);
            
            memset(&m_connChkCB, 0, sizeof(m_ConncheckCB));
        }
    }
        
    //ICELIB_validListDump(&m_icelib->streamControllers[0].validList);
    
    fail_unless(m_icelib->iceState == ICELIB_COMPLETED);
}
END_TEST


START_TEST (medialines_local_inactive)
{
    
    int i;
    
    memset(&m_connChkCB, 0, sizeof(m_ConncheckCB));
        
    ICELIB_setLocalMediaStream( m_icelib, 1, 42, 42, ICE_CAND_TYPE_HOST);

    fail_unless( ICELIB_Start(m_icelib, true) );
    fail_unless( m_icelib->streamControllers[1].checkList.numberOfPairs == 0);

    //ICELIB_checkListDumpAll(m_icelib);
    
    for(i=0;i<300;i++){
        //if(i==30) ICELIB_checkListDumpAll(m_icelib);
        
        ICELIB_Tick( m_icelib );
        if(m_connChkCB.gotCB) {
            //We pretend to be the perfect network. Responses arrive imediately!
           
            ICELIB_incomingBindingResponse(m_icelib,
                                           200,
                                           m_connChkCB.transactionId,
                                           m_connChkCB.destination,
                                           m_connChkCB.source,
                                           m_connChkCB.source);
            
            memset(&m_connChkCB, 0, sizeof(m_ConncheckCB));
        }
    }
        
    //ICELIB_validListDump(&m_icelib->streamControllers[0].validList);
    //ICELIB_validListDump(&m_icelib->streamControllers[1].validList);
    //ICELIB_validListDump(&m_icelib->streamControllers[2].validList);
     
    
    fail_unless( m_icelib->iceState == ICELIB_COMPLETED );
    
    
}
END_TEST

START_TEST (medialines_remote_inactive)
{
    
    int i;
    
    memset(&m_connChkCB, 0, sizeof(m_ConncheckCB));
        
    
    ICELIB_setRemoteMediaStream( m_icelib, 
                                 1, 
                                 m0_remoteUfrag, 
                                 m0_remotePasswd, 
                                 NULL);        

    
    fail_unless( ICELIB_Start(m_icelib, true) );
    fail_unless( m_icelib->streamControllers[1].checkList.numberOfPairs == 0);

    //ICELIB_checkListDumpAll(m_icelib);
    
    for(i=0;i<300;i++){
        //if(i==30) ICELIB_checkListDumpAll(m_icelib);
        
        ICELIB_Tick( m_icelib );
        if(m_connChkCB.gotCB) {
            //We pretend to be the perfect network. Responses arrive imediately!
           
            ICELIB_incomingBindingResponse(m_icelib,
                                           200,
                                           m_connChkCB.transactionId,
                                           m_connChkCB.destination,
                                           m_connChkCB.source,
                                           m_connChkCB.source);
            
            memset(&m_connChkCB, 0, sizeof(m_ConncheckCB));
        }
    }
        
    //ICELIB_validListDump(&m_icelib->streamControllers[0].validList);
    //ICELIB_validListDump(&m_icelib->streamControllers[1].validList);
    //ICELIB_validListDump(&m_icelib->streamControllers[2].validList);
     
    ICELIB_checkListDumpAll(m_icelib);

    fail_unless( m_icelib->iceState == ICELIB_COMPLETED );
    
    
}
END_TEST

START_TEST (medialines_both_inactive)
{
    
    int i;
    
    memset(&m_connChkCB, 0, sizeof(m_ConncheckCB));
        
    ICELIB_setLocalMediaStream( m_icelib, 1, 42, 42, ICE_CAND_TYPE_HOST);
    
    ICELIB_setRemoteMediaStream( m_icelib, 
                                 1, 
                                 m0_remoteUfrag, 
                                 m0_remotePasswd, 
                                 NULL);        

    
    fail_unless( ICELIB_Start(m_icelib, true) );
    fail_unless( m_icelib->streamControllers[1].checkList.numberOfPairs == 0);

    //ICELIB_checkListDumpAll(m_icelib);
    
    for(i=0;i<300;i++){
        //if(i==30) ICELIB_checkListDumpAll(m_icelib);
        
        ICELIB_Tick( m_icelib );
        if(m_connChkCB.gotCB) {
            //We pretend to be the perfect network. Responses arrive imediately!
           
            ICELIB_incomingBindingResponse(m_icelib,
                                           200,
                                           m_connChkCB.transactionId,
                                           m_connChkCB.destination,
                                           m_connChkCB.source,
                                           m_connChkCB.source);
            
            memset(&m_connChkCB, 0, sizeof(m_ConncheckCB));
        }
    }
        
    //ICELIB_validListDump(&m_icelib->streamControllers[0].validList);
    //ICELIB_validListDump(&m_icelib->streamControllers[1].validList);
    //ICELIB_validListDump(&m_icelib->streamControllers[2].validList);
     
    ICELIB_checkListDumpAll(m_icelib);

    fail_unless( m_icelib->iceState == ICELIB_COMPLETED );
    
    
}
END_TEST




Suite * icelib_running_suite (void)
{
  Suite *s = suite_create ("ICElib Running");

  {/* Core test case */
      TCase *tc_medialines = tcase_create ("Multiple Medialines");
      tcase_add_checked_fixture (tc_medialines, icelib_medialines_setup, icelib_medialines_teardown);
      tcase_add_test (tc_medialines, multiple_medialines);
      tcase_add_test (tc_medialines, medialines_local_inactive);
      tcase_add_test (tc_medialines, medialines_remote_inactive);
      tcase_add_test (tc_medialines, medialines_both_inactive);
      suite_add_tcase (s, tc_medialines);
  }

  
  return s;
}
