
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <time.h>
#include <pthread.h>


#include <icelib.h>

#include "iphelper.h"
#include "gather.h"
    

static struct turn_info turnInfo;
static struct listenConfig listenConfig;
static pthread_t turnListenThread;



ICELIB_Result sendConnectivityCheck( void                    *pUserData,
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
                                     StunMsgId               transactionId)
{

}


ICELIB_Result sendConnectivityCheckResponse(void                    *pUserData,
                                            uint32_t                userValue1,
                                            uint32_t                userValue2,
                                            uint32_t                componentId,
                                            const struct sockaddr  *source,
                                            const struct sockaddr  *destination,
                                            const struct sockaddr  *MappedAddress,
                                            uint16_t                errorResponse,
                                            StunMsgId               transactionId,
                                            bool                    useRelay,
                                            const char              *pUfrag,
                                            const char              *pPasswd)
{

}


ICELIB_Result connectivityChecksComplete(void                    *pUserData,
                                         uint32_t                userValue1,
                                         bool                    isControlling,
                                         bool                    iceFailed)
{

}


ICELIB_Result cancelBindRequest( void      *pUserData,
                                 uint32_t   userValue1,
                                 StunMsgId  transactionId)
{


}

ICELIB_Result icePasswordUpdate( void *pUserData,
                                 uint32_t userValue1,
                                 uint32_t userValue2,
                                 char * password)
{

}


ICELIB_Result iceKeepAlive( void        *pUserData,
                            uint32_t    userValue1,
                            uint32_t    userValue2,
                            uint32_t    mediaIdx)
{

}


void ice_messageCB(char *message)
{
    
}



int main(int argc, char *argv[])
{
    ICELIB_INSTANCE iceLib;

    ICELIB_CONFIGURATION iceConfig;

    int choice = 0;
    int c;

    pthread_t turnTickThread;


    if (argc != 5) {
        fprintf(stderr,"usage: testice  iface turnserver user pass\n");
        exit(1);
    }

    //Our local struct where we store stuff
    initTurnInfo(&turnInfo);
    addCredentials(&turnInfo, argv[2], argv[3], argv[4]);

    listenConfig.update_inc_status = ice_messageCB;

    getRemoteTurnServerIp(&turnInfo, argv[2]);

    getLocalIPaddresses(&turnInfo, SOCK_DGRAM, argv[1]);

    //Turn setup
    //TurnClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, "TestIce");
    //pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);
    


    
    iceConfig.tickIntervalMS = 20;
    iceConfig.keepAliveIntervalS = 15;
    iceConfig.maxCheckListPairs = ICELIB_MAX_PAIRS;
    iceConfig.aggressiveNomination = false;
    iceConfig.iceLite = false;


    ICELIB_Constructor(&iceLib,
                       &iceConfig);


    ICELIB_setCallbackOutgoingBindingRequest(&iceLib,
                                             sendConnectivityCheck,
                                             NULL);
    
    ICELIB_setCallbackOutgoingBindingResponse(&iceLib,
                                              sendConnectivityCheckResponse,
                                              NULL);

    ICELIB_setCallbackConnecitivityChecksComplete(&iceLib,
                                                  connectivityChecksComplete,
                                                  NULL);

    ICELIB_setCallbackOutgoingCancelRequest(&iceLib,
                                            cancelBindRequest,
                                            NULL);

    ICELIB_setCallbackPasswordUpdate(&iceLib,
                                     icePasswordUpdate,
                                     NULL);

    ICELIB_setCallbackKeepAlive(&iceLib,
                                iceKeepAlive,
                                NULL);




    ICELIB_addLocalMediaStream(&iceLib,
                               0,/*media idx*/
                               1,/*userValue1*/
                               1,/*userValue2*/
                               ICE_CAND_TYPE_HOST );


}

