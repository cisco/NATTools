#include "icelib.h"
#include <check.h>
#include "../src/icelib_intern.h"

static ICELIB_INSTANCE *m_icelib;

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

static m_ConncheckCB m_connChkCB;


static ICELIB_RUNNING_TEST_printLog( void                    *pUserData,
                              int                     loglevel,
                              char                    *str){
    printf("%s\n", str);

}

static ICELIB_Result ICELIB_RUNNING_TEST_sendConnectivityCheck( void                    *pUserData,
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


static void
icelib_medialines_setup_trickle (void)
{

    ICELIB_CONFIGURATION iceConfig;     

    
    srand(time(NULL));

    m_icelib = (ICELIB_INSTANCE *)malloc(sizeof(ICELIB_INSTANCE));


    iceConfig.tickIntervalMS = 20;
    iceConfig.keepAliveIntervalS = 15;
    iceConfig.maxCheckListPairs = ICELIB_MAX_PAIRS;
    iceConfig.aggressiveNomination = false;
    iceConfig.iceLite = false;
    iceConfig.logLevel = ICELIB_logDebug;
    iceConfig.trickleIce = true;

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
    ICELIB_addLocalMediaStream( m_icelib, 42, 42, ICE_CAND_TYPE_HOST);

    //Medialine: 1
    ICELIB_addLocalMediaStream( m_icelib, 42, 42, ICE_CAND_TYPE_HOST);

    //Medialine: 2
    ICELIB_addLocalMediaStream( m_icelib, 42, 42, ICE_CAND_TYPE_HOST);
        
    ICELIB_setTurnState( m_icelib, 0, ICE_TURN_ALLOCATED);

    
    //Remote side
    //Medialine: 0
    ICELIB_addRemoteMediaStream( m_icelib, m0_remoteUfrag, m0_remotePasswd, NULL);        
    
    //Medialine: 1
    ICELIB_addRemoteMediaStream( m_icelib, m1_remoteUfrag, m1_remotePasswd, NULL);        
    
    //Medialine: 2
    ICELIB_addRemoteMediaStream( m_icelib, m2_remoteUfrag, m2_remotePasswd, NULL);        
}


static void
icelib_medialines_teardown_trickle (void)
{
    ICELIB_Destructor (m_icelib);
    free(m_icelib);
}


START_TEST (startTrickle_zeroCandidates)
{
    // Since we now have zero candidates registered. Starting Ice should only succeed if one is using trickle.
    fail_unless ( ICELIB_Start(m_icelib, true) );
    fail_unless ( ICELIB_isRunning(m_icelib) );
    fail_if( ICELIB_isMangled( m_icelib ) );

}
END_TEST

START_TEST (startTrickle_oneCandidate)
{
    
    // Starting Ice should succeed if one is using trickle.
    fail_unless ( ICELIB_Start(m_icelib, true) );
    fail_unless ( ICELIB_isRunning(m_icelib) );
    fail_if( ICELIB_isMangled( m_icelib ) );

}
END_TEST

START_TEST (addCandidateAfterStart_nonRedundant)
{
    struct sockaddr_storage m0_defaultAddr;
    struct sockaddr_storage m0_localHostRtp;
    struct sockaddr_storage m0_localHostRtcp;
    struct sockaddr_storage m0_localRflxRtp;
    struct sockaddr_storage m0_localRflxRtcp;
    struct sockaddr_storage m0_localRelayRtp;
    struct sockaddr_storage m0_localRelayRtcp;
    
    sockaddr_initFromString( (struct sockaddr *)&m0_localHostRtp,  "192.168.2.10:3456");
    sockaddr_initFromString( (struct sockaddr *)&m0_localHostRtcp, "192.168.2.10:3457");
    sockaddr_initFromString( (struct sockaddr *)&m0_localRflxRtp,  "67.45.4.6:1045");
    sockaddr_initFromString( (struct sockaddr *)&m0_localRflxRtcp, "67.45.4.6:3451");
    sockaddr_initFromString( (struct sockaddr *)&m0_localRelayRtp, "158.38.46.10:2312");
    sockaddr_initFromString( (struct sockaddr *)&m0_localRelayRtcp,"158.38.46.10:4567");
    
    // New candidatePair should be formed when adding nonredundant candidates.
    ICELIB_Start(m_icelib, true);

    ICELIB_addLocalCandidate( m_icelib, 0, 1, (struct sockaddr *)&m0_localHostRtp, NULL, ICE_CAND_TYPE_HOST, 0xffff );
    ICELIB_addRemoteCandidate( m_icelib, 0, "1", 1, 1, 2130706431, m0_remoteHostRtpAddr, 47936, ICE_CAND_TYPE_HOST );

    fail_unless (m_icelib->streamControllers[0].checkList.numberOfPairs == 1);
    int i;
    for (i = 1; i < m_icelib->numberOfMediaStreams; ++i) {
        fail_unless (m_icelib->streamControllers[i].checkList.numberOfPairs == 0);
    }

    ICELIB_addLocalCandidate( m_icelib, 0, 2, (struct sockaddr *)&m0_localHostRtcp, NULL, ICE_CAND_TYPE_HOST, 0xffff );
    ICELIB_addLocalCandidate( m_icelib, 0, 1, (struct sockaddr *)&m0_localRflxRtp, (struct sockaddr *)&m0_localHostRtp, ICE_CAND_TYPE_SRFLX, 0xffff);        
    ICELIB_addLocalCandidate( m_icelib, 0, 2, (struct sockaddr *)&m0_localRflxRtcp, (struct sockaddr *)&m0_localHostRtp, ICE_CAND_TYPE_SRFLX, 0xffff );
    ICELIB_addLocalCandidate( m_icelib, 0, 1, (struct sockaddr *)&m0_localRelayRtp, (struct sockaddr *)&m0_localRflxRtp, ICE_CAND_TYPE_RELAY, 0xffff );    
    ICELIB_addLocalCandidate( m_icelib, 0, 2, (struct sockaddr *)&m0_localRelayRtcp, (struct sockaddr *)&m0_localRflxRtcp, ICE_CAND_TYPE_RELAY, 0xffff );
    
    ICELIB_addRemoteCandidate( m_icelib, 0, "1", 1, 2, 2130706430, m0_remoteHostRtcpAddr, 47937, ICE_CAND_TYPE_HOST );
    ICELIB_addRemoteCandidate( m_icelib, 0, "3", 1, 1, 1694498815, m0_remoteRflxRtpAddr, 3807, ICE_CAND_TYPE_SRFLX );
    ICELIB_addRemoteCandidate( m_icelib, 0, "3", 1, 2, 1694498814, m0_remoteRflxRtcpAddr, 32629, ICE_CAND_TYPE_SRFLX );
    ICELIB_addRemoteCandidate( m_icelib, 0, "4", 1, 1, 16777215, m0_remoteRelayRtpAddr, 52948, ICE_CAND_TYPE_RELAY );
    ICELIB_addRemoteCandidate( m_icelib, 0, "4", 1, 2, 16777214, m0_remoteRelayRtcpAddr, 49832, ICE_CAND_TYPE_RELAY );
    
    fail_unless (m_icelib->localIceMedia.mediaStream[0].numberOfCandidates == 6);
    //TODO: I admint I have not yet checked if 21 pairs is the correct number. It doesn't feel right, I would actually guess 20.
    fail_unless (m_icelib->streamControllers[0].checkList.numberOfPairs == 21);
}
END_TEST


START_TEST (addCandidateAfterStart_redundant)
{
    //TODO: Add test for when serverReflexive candidate is added and make sure redundant pair doesn't show.
}
END_TEST


START_TEST (runningTrickle_successfulCandidates)
{
    // This test might succeed in a flash, with some minor check if endOfCandidates bool is true.
    //TODO: Before end of candidates is called, trickle should not finish.
    //TODO: After end of local and remote candidates have bin called, trickle should succed.
}
END_TEST


START_TEST (running_trickle_failureCandidates)
{
    // This test might succeed in a flash, with some minor check if endOfCandidates bool is true.
    //TODO: Before end of candidates is called, trickle should not fail.
    //TODO: After end of local and remote candidates have bin called, trickle should fail.
}
END_TEST


Suite * icelib_trickle_suite (void)
{
  Suite *s = suite_create ("ICElib Trickle");

  {/*Restart */ 
     TCase *tc_startup = tcase_create ("Startup");
     tcase_add_checked_fixture (tc_startup, icelib_medialines_setup_trickle, icelib_medialines_teardown_trickle);
     tcase_add_test (tc_startup, startTrickle_zeroCandidates);
     tcase_add_test (tc_startup, startTrickle_oneCandidate);
     tcase_add_test (tc_startup, addCandidateAfterStart_nonRedundant);
     tcase_add_test (tc_startup, addCandidateAfterStart_redundant);
     tcase_add_test (tc_startup, runningTrickle_successfulCandidates);
     tcase_add_test (tc_startup, running_trickle_failureCandidates);
     suite_add_tcase (s, tc_startup);
  }
  
  return s;
}
