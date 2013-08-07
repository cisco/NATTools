#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>

#include <stdbool.h>

#include <stunlib.h>
#include <stunclient.h>
#include <turnclient.h>
#include <icelib.h>

#include "utils.h"

#define MY_PORT "33333"
#define MY_PORT_INT 33333

#define TURN_SERVER "turn.qa.rd.tandberg.com"
#define TURN_PORT "3478"
#define TURN_USER "turnuser"
#define TURN_PASS "turnpass"

#define SOFTWARE "ICE test client\0"

struct sockaddr_storage
    lRflxAddr,
    lRelAddr;

int sockfd;
int turnInst;

static const uint32_t TEST_THREAD_CTX = 1;

struct addrinfo *turnservinfo;

ICELIB_INSTANCE iceInstance;


void StunStatusCallBack(void *ctx, StunCallBackData_T *retData)
{
    if (retData->stunResult == StunResult_BindOk) {
        struct sockaddr_storage rflxAddr;
        struct sockaddr *pRflxAddr = &rflxAddr;

        char addr[SOCKADDR_MAX_STRLEN];

        sockaddr_initFromString(pRflxAddr, retData->bindResp.rflxAddr);

        printf("\n\nmappedAddress: %s actual: %s\n\n",
            sockaddr_toString(pRflxAddr, addr, sizeof(addr), true),
            retData->bindResp.rflxAddr
        );

        if (pRflxAddr->sa_family == AF_INET)
            ((struct sockaddr_in *) pRflxAddr)->sin_port = retData->bindResp.rflxPort;
        else if (pRflxAddr->sa_family == AF_INET6)
            ((struct sockaddr_in6 *) pRflxAddr)->sin6_port = retData->bindResp.rflxPort;

        ICELIB_incommingBindingResponse(&iceInstance,
                                        200,
                                        retData->msgId,
                                        (struct sockaddr *) &retData->srcAddrStr,
                                        (struct sockaddr *) &retData->dstBaseAddrStr,
                                        (struct sockaddr *) &retData->bindResp.rflxAddr);

        printf("StunResult OK\n");
    }
    else {
        printf("StunResult not OK\n");
    }
}


static void *tickTurn(void *ptr)
{
    struct timespec timer;
    struct timespec remaining;
    uint32_t  *ctx = (uint32_t *)ptr;

    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    for(;;) {
        nanosleep(&timer, &remaining);
        TurnClient_HandleTick(*ctx);
    }

}


void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData)
{
    //ctx points to whatever you initialized the library with. (Not used in this simple example.)

    if ( retData->turnResult == TurnResult_AllocOk ){
        char addr[SOCKADDR_MAX_STRLEN];

        lRflxAddr = retData->TurnResultData.AllocResp.rflxAddr;
        lRelAddr = retData->TurnResultData.AllocResp.relAddr;

        startIce();

    } else if (retData->turnResult == TurnResult_AllocUnauthorised) {
        printf("Unable to authorize. Wrong user/pass?\n");
    }
}

static void *recieveMsgs(void *ptr){
    int numbytes;
    struct sockaddr_storage their_addr, my_addr;
    unsigned char buf[MAXBUFLEN];
    StunMessage stunMsg;
    int keyLen = 16;
    char md5[keyLen];
    char realm[STUN_MAX_STRING];

    while (1) {
        if((numbytes = recvStunMsg(sockfd, &their_addr, &stunMsg, buf)) != -1) {
            if (stunMsg.msgHdr.msgType == STUN_MSG_DataIndicationMsg) {
                if (stunMsg.hasData) {
                    //Decode and do something with the data?
                }
            }

            if (stunMsg.hasRealm) {
               memcpy(realm, stunMsg.realm.value, STUN_MAX_STRING);
            }


            if (stunMsg.hasMessageIntegrity) {
                printf("   Integrity attribute present.\n");
                stunlib_createMD5Key((unsigned char *)md5, TURN_USER, realm, TURN_PASS);

                if (stunlib_checkIntegrity(buf, numbytes, &stunMsg, md5, keyLen)) {
                    printf("     - Integrity check OK\n");
                } else {
                    printf("     - Integrity check NOT OK\n");
                }
            }


            switch (stunMsg.msgHdr.msgType) {
                case STUN_MSG_BindResponseMsg:
                    printf("STUN bind response rcvd.\n");
                    StunClient_HandleIncResp(TEST_THREAD_CTX, &stunMsg, &their_addr);
                    break;
                case STUN_MSG_BindErrorResponseMsg:
                    printf("STUN error response msg rcvd.\n");
                    break;
                case STUN_MSG_BindRequestMsg:
                    printf("STUN bind request rcvd.\n");
                    STUN_INCOMING_REQ_DATA incomingReq;
                    memset(&incomingReq, 0, sizeof(incomingReq));

                    if (StunServer_HandleStunIncomingBindReqMsg(TEST_THREAD_CTX, &incomingReq, &stunMsg, false/*TODO: fromRelay*/)) {
                        uint64_t tieBreaker = stunMsg.hasControlling ? stunMsg.controlling.value :
                            stunMsg.hasControlled ? stunMsg.controlled.value : 0;

                        bool fromRelay =
                            sockaddr_sameAddr(&their_addr, &turnservinfo->ai_addr) &&
                            sockaddr_samePort(&their_addr, &turnservinfo->ai_addr);

                        ICELIB_incommingBindingRequest(&iceInstance,
                            0, // userValue1
                            0, // userValue2
                            stunMsg.username.value,
                            stunMsg.priority.value,
                            stunMsg.hasUseCandidate,
                            stunMsg.hasControlling,
                            stunMsg.hasControlled,
                            tieBreaker,
                            stunMsg.msgHdr.id,
                            &their_addr,
                            ICELIB_getLocalConnectionAddr(&iceInstance, 0, 0),
                            fromRelay,
                            turnservinfo->ai_addr,
                            ICELIB_RTP_COMPONENT_ID);
                    }
                    break;

                case STUN_MSG_AllocateRequestMsg:
                case STUN_MSG_AllocateResponseMsg:
                case STUN_MSG_AllocateErrorResponseMsg:
                case STUN_MSG_RefreshRequestMsg:
                case STUN_MSG_RefreshResponseMsg:
                case STUN_MSG_RefreshErrorResponseMsg:
                case STUN_MSG_CreatePermissionRequestMsg:
                case STUN_MSG_CreatePermissionResponseMsg:
                case STUN_MSG_CreatePermissionErrorResponseMsg:
                case STUN_MSG_ChannelBindRequestMsg:
                case STUN_MSG_ChannelBindResponseMsg:
                case STUN_MSG_ChannelBindErrorResponseMsg:
                case STUN_MSG_SendIndicationMsg:
                case STUN_MSG_DataIndicationMsg:
                    printf("Some TURN message received.\n");
                    TurnClient_HandleIncResp(TEST_THREAD_CTX,
                                             TURNCLIENT_CTX_UNKNOWN,
                                             &stunMsg,
                                             buf);
                    break;

                default:
                    printf("Unknown STUN-message rcvd.\n");
            }
        }
    }
}

void turnSetup() {
    pthread_t turnTickThread;
    TurnCallBackData_T TurnCbData;
    struct addrinfo hints;
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;

    TurnClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, "TestTurn");
    pthread_create(&turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);

    if ((rv = getaddrinfo(TURN_SERVER, TURN_PORT, &hints, &turnservinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    turnInst = TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
                                              NULL,
                                              turnservinfo->ai_addr,
                                              TURN_USER,
                                              TURN_PASS,
                                              sockfd,                       /* socket */
                                              AF_INET,
                                              sendRawStun,             /* send func */
                                              NULL,  /* timeout list */
                                              TurnStatusCallBack,
                                              &TurnCbData,
                                              false,
                                              false,
                                              0);

}


static void createSocketAndStartRecieving() {
    struct addrinfo *servinfo;
    pthread_t recieverThread;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    printf("starting turn...");
    struct addrinfo hints;
    int rv;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, MY_PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }

    // loop through all the results and bind to the first we can
    for(; servinfo != NULL; servinfo = servinfo->ai_next) {

        if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
            close(sockfd);
            perror("createSocket: bind");
            continue;
        }

        break;
    }

    if (servinfo == NULL) {
        fprintf(stderr, "%s: failed to bind socket\n");
        return -2;
    } else {
        fprintf(stderr, "%s: created socket\n");
    }

    pthread_create(&recieverThread, NULL, recieveMsgs, (void*) &TEST_THREAD_CTX);
}


ICELIB_Result callbackRequest(void                    *pUserData,
                              const struct sockaddr *destination,
                              const struct sockaddr *source,
                              uint32_t              userValue1,
                              uint32_t              userValue2,
                              uint32_t              componentId,
                              bool                  useRelay,
                              const char            *pUfragPair,
                              const char            *pPasswd,
                              uint32_t              peerPriority,
                              bool                  useCandidate,
                              bool                  iceControlling,
                              bool                  iceControlled,
                              uint64_t              tieBreaker,
                              StunMsgId             transactionId)
{

    StunCallBackData_T StunCbData;
    char addr[SOCKADDR_MAX_STRLEN];

    printf("\nRequest callback...\n");

    StunClient_startBindTransaction (TEST_THREAD_CTX,
                                     pUserData,
                                     destination,
                                     source,
                                     useRelay,
                                     pUfragPair,
                                     pPasswd,
                                     peerPriority,
                                     useCandidate,
                                     iceControlling,
                                     tieBreaker,
                                     transactionId,
                                     sockfd,
                                     sendRawStun,
                                     NULL, // timeoutlist
                                     StunStatusCallBack,
                                     &StunCbData,
                                     turnInst,
                                     NULL);

    printf("source: %s\n",
        sockaddr_toString(
            source,
            addr,
            sizeof(addr),
            true
        )
    );

    printf("destination: %s\n",
        sockaddr_toString(
            destination,
            addr,
            sizeof(addr),
            true
        )
    );

    return ICELIB_Result_OK;
}

ICELIB_Result callbackResponse(void *pUserData,
                            uint32_t userValue1,
                            uint32_t userValue2,
                            uint32_t componentId,
                            const struct sockaddr *source,
                            const struct sockaddr *destination,
                            const struct sockaddr *mappedAddress,
                            uint16_t errorResponse,
                            StunMsgId transactionId,
                            bool useRelay,
                            const char *pUfragPair,
                            const char *pPasswd)
{
    char addr[SOCKADDR_MAX_STRLEN];

    printf("\nResponse callback...\n");

    StunServer_SendConnectivityBindingResp(TEST_THREAD_CTX,
                                           sockfd,
                                           transactionId,
                                           pUfragPair,
                                           pPasswd,
                                           mappedAddress,
                                           sizeof(*mappedAddress),
                                           destination,
                                           sizeof(*destination),
                                           pUserData,
                                           sendRawStun,
                                           useRelay,
                                           turnInst,
                                           NULL); // no MALICE

    return ICELIB_Result_OK;
}

ICELIB_Result callbackKeepAlive(void
                                *pUserData,
                                uint32_t    userValue1,
                                uint32_t    userValue2,
                                uint32_t    mediaIdx)
{

    printf("\nKeep alive callback...\n");

    uint8_t buf [STUN_MAX_PACKET_SIZE];
    int encLen;
    StunMsgId transId = {{0},};

    transId.octet[0] = 'K';
    transId.octet[1] = 'e';
    transId.octet[2] = 'e';
    transId.octet[3] = 'p';
    transId.octet[4] = 'a';
    transId.octet[5] = ' ';
    transId.octet[6] = 'R';
    transId.octet[7] = 'T';
    transId.octet[8] = 'P';

    encLen = stunlib_encodeStunKeepAliveReq(StunKeepAliveUsage_Ice, &transId, buf, STUN_MAX_PACKET_SIZE);
    ICE_REMOTE_CANDIDATES const * candidates = ICELIB_getActiveRemoteCandidates(&iceInstance, mediaIdx);
    if (candidates)
    {
        const struct sockaddr * conn = (const struct sockaddr *) &candidates->remoteCandidate[0].connectionAddr;
        if (sockaddr_isSet(conn))
            sendRawStun(sockfd, buf, encLen, conn, sizeof(*conn), NULL);

        /*
        conn = (const struct sockaddr *) &candidates->remoteCandidate[1].connectionAddr;
        if ((candidates->numberOfComponents > 1) && userValue2 && sockaddr_isSet(conn))
        {
            sendRawStun_cb (userValue2, buf, encLen, conn, 0);
        }
        */
    }

    return ICELIB_Result_OK;
}

ICELIB_Result callbackComplete(void *pUserData,
                                uint32_t userValue1,
                                bool isControlling,
                                bool iceFailed)
{

    printf("\nCheck complete callback...\n");

    if (iceFailed){
        printf("Ice failed.");
    }
    else {
        printf("Ice succeded.");


    }

    return ICELIB_Result_OK;
}

ICELIB_Result callbackLog(void     *pUserData,
                        ICELIB_logLevel logLevel,
                        const char         *str)
{

    printf("Log callback: %s\n", str);
    return ICELIB_Result_OK;
}

static void *tickIce(void *ptr)
{
    struct timespec timer;
    struct timespec remaining;
    ICELIB_INSTANCE *iceInstance = (ICELIB_INSTANCE *)ptr;

    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    for(;;) {
        nanosleep(&timer, &remaining);

        StunClient_HandleTick(TEST_THREAD_CTX);
        ICELIB_Tick(iceInstance);
    }
}

void startIce () {

    int controlling;

    struct sockaddr_in rHostAddr;

    struct ifaddrs *hostAddrs;

    char
        ufrag[] = "evtj:h6vY",
        pwd[] = "VOkJxbRl1RmTxUk";

    char
        rHostFoundation[ICELIB_FOUNDATION_LENGTH],
        rRflxFoundation[ICELIB_FOUNDATION_LENGTH],
        rRelFoundation[ICELIB_FOUNDATION_LENGTH];
    char
        rHostAddrStr[INET_ADDRSTRLEN],
        rRflxAddrStr[INET_ADDRSTRLEN],
        rRelAddrStr[INET_ADDRSTRLEN];

    unsigned short
        rHostPort,
        rRflxPort,
        rRelPort;

    char addr[SOCKADDR_MAX_STRLEN];

    pthread_t iceTickThread;
    ICELIB_CONFIGURATION iceConfig =
    {
        30,
        1,
        50,
        false,
        false,
        20,
        0 //ICELIB_logDebug
    };

    printf(">>> controlling\n");
    scanf("%d", &controlling);

    //if (controlling)
    //    ufrag[9] = '2';

    ICELIB_Constructor(&iceInstance, &iceConfig);

    StunClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, SOFTWARE);

    ICELIB_setCallbackOutgoingBindingRequest(
        &iceInstance,
        callbackRequest,
        NULL
    );

    ICELIB_setCallbackOutgoingBindingResponse(
        &iceInstance,
        callbackResponse,
        NULL
    );


    ICELIB_setCallbackConnecitivityChecksComplete(
        &iceInstance,
        callbackComplete,
        NULL
    );

    ICELIB_setCallbackKeepAlive(
        &iceInstance,
        callbackKeepAlive,
        NULL
    );

    ICELIB_setCallbackLog(
        &iceInstance,
        callbackLog,
        NULL,
        0 // ICELIB_logDebug
    );

    ICELIB_addLocalMediaStream(
        &iceInstance,
        0,
        17, // random value
        37, // random value
        ICE_CAND_TYPE_HOST
    );

    // Get local host addr and add the candidates...
    getifaddrs(&hostAddrs);

    while (hostAddrs != NULL) {
        if (hostAddrs->ifa_addr->sa_family == AF_INET && strcmp(hostAddrs->ifa_name, "eth0") == 0) {
            ((struct sockaddr_in *)hostAddrs->ifa_addr)->sin_port = htons(MY_PORT_INT);
            ICELIB_addLocalCandidate(
                &iceInstance,
                0,
                ICELIB_RTP_COMPONENT_ID,
                hostAddrs->ifa_addr,
                NULL,
                ICE_CAND_TYPE_HOST
            );
        }
        hostAddrs = hostAddrs->ifa_next;
    }

    ICELIB_addLocalCandidate(
        &iceInstance,
        0,
        ICELIB_RTP_COMPONENT_ID,
        &lRflxAddr,
        NULL,
        ICE_CAND_TYPE_SRFLX
    );

    ICELIB_addLocalCandidate(
        &iceInstance,
        0,
        ICELIB_RTP_COMPONENT_ID,
        &lRelAddr,
        NULL,
        ICE_CAND_TYPE_RELAY
    );

    printf("local candidates ->\n");
    uint32_t i;
    uint32_t n;
    for (i = 0, n = ICELIB_getNumberOfLocalCandidates(&iceInstance, 0); i < n; i++) {
        printf("    %s - compID: %d, candType: %d, foundation: %s\n",
            sockaddr_toString(
                ICELIB_getLocalConnectionAddr(&iceInstance, 0, i),
                addr,
                sizeof(addr),
                true
            ),
            ICELIB_getLocalComponentId(&iceInstance, 0, i),
            ICELIB_getLocalCandidateType(&iceInstance, 0, i),
            iceInstance.localIceMedia.mediaStream[0].candidate[i].foundation
        );
    }


    // Listen for host candidate...
    printf(">>> hostAddr\n");
    scanf("%s", &rHostAddrStr);

    sockaddr_initFromString(&rHostAddr, rHostAddrStr);

    ICELIB_addRemoteMediaStream(
        &iceInstance,
        ufrag,
        pwd,
        &rHostAddr
    );

    ICELIB_addRemoteCandidate(
        &iceInstance,
        0,
        "1", // foundation.
        sizeof(rHostFoundation),
        ICELIB_RTP_COMPONENT_ID,
        1,
        rHostAddrStr,
        MY_PORT_INT,
        ICE_CAND_TYPE_HOST
    );

    // Listen for rflx candidate.
    printf(">>> rlfxAddr : rlfxPort\n");
    scanf("%s : %hu", &rRflxAddrStr, &rRflxPort);

    ICELIB_addRemoteCandidate(
        &iceInstance,
        0,
        "3",
        sizeof(rRflxFoundation),
        ICELIB_RTP_COMPONENT_ID,
        0,
        rRflxAddrStr,
        rRflxPort,
        ICE_CAND_TYPE_SRFLX
    );

    // Listen for rel candidate
    printf(">>> relAddr : relPort\n");
    scanf("%s : %hu", &rRelAddrStr, &rRelPort);

    ICELIB_addRemoteCandidate(
        &iceInstance,
        0,
        "4",
        sizeof(rRelFoundation),
        ICELIB_RTP_COMPONENT_ID,
        0,
        rRelAddrStr,
        rRelPort,
        ICE_CAND_TYPE_RELAY
    );

    // Print remote candidates for comparing
    printf("remote candidates ->\n");
    for (i = 0, n = ICELIB_getNumberOfRemoteCandidates(&iceInstance, 0); i < n; i++) {
        printf("    %s - compID: %d, candType: %d, foundation: %s\n",
            sockaddr_toString(
                ICELIB_getRemoteConnectionAddr(&iceInstance, 0, i),
                addr,
                sizeof(addr),
                true
            ),
            ICELIB_getRemoteComponentId(&iceInstance, 0, i),
            ICELIB_getRemoteCandidateType(&iceInstance, 0, i),
            iceInstance.remoteIceMedia.mediaStream[0].candidate[i].foundation
        );
    }


    // Start ice...


    ICELIB_Start(&iceInstance, controlling);

    pthread_create(&iceTickThread, NULL, tickIce, (void*) &iceInstance);
}

int main(int argc, char *argv[])
{
    createSocketAndStartRecieving();

    turnSetup();

    while (1) sleep(10);
}

