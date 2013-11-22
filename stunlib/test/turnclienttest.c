
#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "turnclient.h"
#include "turn_intern.h"
#include "sockaddr_util.h"

Suite * turnclient_suite (void);


#define  MAX_INSTANCES  5
#define  TEST_THREAD_CTX 1

typedef  struct
{
    uint32_t a;
    uint8_t  b;
}
AppCtx_T;

static AppCtx_T           AppCtx[MAX_INSTANCES];
static TurnCallBackData_T TurnCbData[MAX_INSTANCES];

static AppCtx_T CurrAppCtx;

//static int            TestNo;
static uint32_t       StunDefaultTimeoutList[STUNCLIENT_MAX_RETRANSMITS] = {100, 0};
static StunMsgId      LastTransId;
static struct sockaddr_storage LastAddress;
static bool runningAsIPv6;

static  const uint8_t StunCookie[]   = STUN_MAGIC_COOKIE_ARRAY;

TurnResult_T turnResult;

struct sockaddr_storage turnServerAddr;
TURN_INSTANCE_DATA * pInst;


static void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData)
{

    turnResult = retData->turnResult;
    printf("Got TURN status callback (Result (%i)\n", retData->turnResult);

}


/* Callback for management info  */
static void  PrintTurnInfo(TurnInfoCategory_T category, char *ErrStr)
{
    fprintf(stderr, "%s\n", ErrStr);
}


static void SendRawStun(const uint8_t *buf, 
                       size_t len, 
                       const struct sockaddr *addr,
                       void *userdata)
{
    char addr_str[SOCKADDR_MAX_STRLEN];
    /* find the transaction id  so we can use this in the simulated resp */


    memcpy(&LastTransId, &buf[8], STUN_MSG_ID_SIZE); 

    sockaddr_copy((struct sockaddr *)&LastAddress, addr);
    
    sockaddr_toString(addr, addr_str, SOCKADDR_MAX_STRLEN, true);
                      
    printf("TurnClienttest sendto: '%s'\n", addr_str);

}

static int StartAllocateTransaction(int n)
{
    n = 0; // ntot sure we need n... TODO: fixme
    //struct sockaddr_storage addr;

    CurrAppCtx.a =  AppCtx[n].a = 100+n;
    CurrAppCtx.b =  AppCtx[n].b = 200+n;
    

    /* kick off turn */
    return TurnClient_StartAllocateTransaction(&pInst,
                                               50,
                                               NULL,
                                               "test",
                                               NULL,
                                               (struct sockaddr *)&turnServerAddr,
                                               "pem",
                                               "pem",
                                               0,
                                               SendRawStun,             /* send func */
                                               TurnStatusCallBack,
                                               false,
                                               0);

}

static void SimAllocResp(int ctx, bool relay, bool xorMappedAddr, bool lifetime, bool IPv6)
{
    StunMessage m;
    memset(&m, 0, sizeof(m));
    memcpy(&m.msgHdr.id, &LastTransId, STUN_MSG_ID_SIZE);  
    memcpy(&m.msgHdr.cookie, StunCookie, sizeof(m.msgHdr.cookie));
    m.msgHdr.msgType = STUN_MSG_AllocateResponseMsg;

    /* relay */
    if (relay)
    {
        if(IPv6){
            uint8_t addr[16] = {0x20, 0x1, 0x4, 0x70, 0xdc, 0x88, 0x0, 0x2, 0x2, 0x26, 0x18, 0xff, 0xfe, 0x92, 0x6d, 0x53};
            m.hasXorRelayAddress = true;
            stunlib_setIP6Address(&m.xorRelayAddress, addr, 0x4200);

            
        }else{
            m.hasXorRelayAddress = true;
            m.xorRelayAddress.familyType = STUN_ADDR_IPv4Family;
            m.xorRelayAddress.addr.v4.addr = 3251135384UL;// "193.200.99.152"
            m.xorRelayAddress.addr.v4.port = 42000;
        }
    }

    /* XOR mapped addr*/
    if (xorMappedAddr)
    {
        if(IPv6){
            uint8_t addr[16] = {0x20, 0x1, 0x4, 0x70, 0xdc, 0x88, 0x1, 0x22, 0x21, 0x26, 0x18, 0xff, 0xfe, 0x92, 0x6d, 0x53};
            m.hasXorMappedAddress = true;
            stunlib_setIP6Address(&m.xorMappedAddress, addr, 0x4200);

        }else{
            
            m.hasXorMappedAddress = true;
            m.xorMappedAddress.familyType = STUN_ADDR_IPv4Family;
            m.xorMappedAddress.addr.v4.addr = 1009527574UL;// "60.44.43.22");
            m.xorMappedAddress.addr.v4.port = 43000;
        }
    }

    /* lifetime */
    if (lifetime)
    {
        m.hasLifetime = true;
        m.lifetime.value = 60;
    }

    TurnClient_HandleIncResp(pInst, &m, NULL);

}


static void Sim_ChanBindOrPermissionResp(int ctx, uint16_t msgType, uint32_t errClass, uint32_t errNumber)
{
    StunMessage m;
    memset(&m, 0, sizeof(m));
    memcpy(&m.msgHdr.id, &LastTransId, STUN_MSG_ID_SIZE);  
    memcpy(&m.msgHdr.cookie, StunCookie, sizeof(m.msgHdr.cookie));
    m.msgHdr.msgType = msgType;

    if ((msgType == STUN_MSG_ChannelBindErrorResponseMsg)
    || (msgType == STUN_MSG_CreatePermissionErrorResponseMsg))
    {
        m.hasErrorCode          = true;
        m.errorCode.errorClass  = errClass;
        m.errorCode.number      = errNumber;
    }
    TurnClient_HandleIncResp(pInst, &m, NULL);
}


static void SimInitialAllocRespErr(int ctx, bool hasErrCode, uint32_t errClass, uint32_t errNumber, 
                                   bool hasRealm, bool hasNonce, bool hasAltServer)
{
    StunMessage m;

    memset(&m, 0, sizeof(m));
    memcpy(&m.msgHdr.id, &LastTransId, STUN_MSG_ID_SIZE);  
    memcpy(&m.msgHdr.cookie, StunCookie, sizeof(m.msgHdr.cookie));
    m.msgHdr.msgType = STUN_MSG_AllocateErrorResponseMsg;

    /* error code */
    if (hasErrCode)
    {
        m.hasErrorCode          = hasErrCode;
        m.errorCode.errorClass  = errClass;
        m.errorCode.number      = errNumber;
    }

    /* realm */
    if (hasRealm)
    {
        m.hasRealm = hasRealm;
        strncpy(m.realm.value, "united.no", strlen("united.no"));
        m.realm.sizeValue = strlen(m.realm.value);
    }

    /* nonce */
    if (hasNonce)
    {
        m.hasNonce = hasNonce;
        strncpy(m.nonce.value, "mydaftnonce", strlen("mydaftnonce"));
        m.nonce.sizeValue = strlen(m.nonce.value);
    }


    /* alternate server */
    if (hasAltServer) 
    {
        m.hasAlternateServer = true;
        if(runningAsIPv6){
            uint8_t addr[16] = {0x20, 0x1, 0x4, 0x70, 0xdc, 0x88, 0x0, 0x2, 0x2, 0x26, 0x18, 0xff, 0xfe, 0x92, 0x6d, 0x53};
            stunlib_setIP6Address(&m.alternateServer, addr, 0x4200);
            

        }else{

        m.alternateServer.familyType = STUN_ADDR_IPv4Family;
        m.alternateServer.addr.v4.addr = 0x12345678;
        m.alternateServer.addr.v4.port = 3478;
        }
    }


    TurnClient_HandleIncResp(pInst, &m, NULL);
}



static void  Sim_RefreshResp(int ctx)
{
    TurnClientSimulateSig(pInst, TURN_SIGNAL_RefreshResp);
}


/* allocation refresh error */
static void Sim_RefreshError(int ctx, uint32_t errClass, uint32_t errNumber, bool hasRealm, bool hasNonce)
{
    StunMessage m;
    memset(&m, 0, sizeof(m));
    memcpy(&m.msgHdr.id, &LastTransId, STUN_MSG_ID_SIZE);  
    memcpy(&m.msgHdr.cookie, StunCookie, sizeof(m.msgHdr.cookie));
    m.msgHdr.msgType = STUN_MSG_RefreshErrorResponseMsg;
    m.hasErrorCode          = true;
    m.errorCode.errorClass  = errClass;
    m.errorCode.number      = errNumber;

    /* realm */
    if (hasRealm)
    {
        m.hasRealm = hasRealm;
        strncpy(m.realm.value, "united.no", strlen("united.no"));
        m.realm.sizeValue = strlen(m.realm.value);
    }

    /* nonce */
    if (hasNonce)
    {
        m.hasNonce = hasNonce;
        strncpy(m.nonce.value, "mydaftnonce", strlen("mydaftnonce"));
        m.nonce.sizeValue = strlen(m.nonce.value);
    }

    TurnClient_HandleIncResp(pInst, &m, NULL);
}


static int GotoAllocatedState(int appCtx)
{
    int ctx = StartAllocateTransaction(appCtx);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(appCtx, true, 4, 1, true, true, false); /* 401, has realm has nonce */
    TurnClient_HandleTick(pInst);
    SimAllocResp(ctx, true, true, true, runningAsIPv6);
    return ctx;
}


static void  Sim_TimerRefreshAlloc(int ctx)
{
    TurnClientSimulateSig(pInst, TURN_SIGNAL_TimerRefreshAlloc);
}




static void setup (void)
{
    runningAsIPv6 = false;
    sockaddr_initFromString((struct sockaddr*)&turnServerAddr, "193.200.93.152:3478");
    pInst = NULL;
}

static void teardown (void)
{
    turnResult = TurnResult_Empty; 

    TurnClient_free(pInst);
    pInst = NULL;
}


static void setupIPv6 (void)
{
    runningAsIPv6 = true;
    sockaddr_initFromString((struct sockaddr*)&turnServerAddr, "[2001:470:dc88:2:226:18ff:fe92:6d53]:3478");
    pInst = NULL;
}

static void teardownIPv6 (void)
{
    turnResult = TurnResult_Empty; 
    TurnClient_free(pInst);
    pInst = NULL;
}




#if 0
START_TEST (turnclient_init)
{

    int ret;
    ret = TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "UnitTestSofware");
    fail_unless(ret);

}
END_TEST
#endif


START_TEST (WaitAllocRespNotAut_Timeout)
{
    int ctx;

    ctx = StartAllocateTransaction(0);
    printf ("WaitAllocRespNotAut_Timeout: %p\n", pInst);

    /* 1 Tick */
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_Empty);
    fail_unless (sockaddr_alike((struct sockaddr *)&LastAddress, 
                                    (struct sockaddr *)&turnServerAddr));
    /* 2 Tick */
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_Empty);

    /* 3 Tick */
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_Empty);

    /* 4 Tick */
    {
        int i;
        for (i = 0; i < 100; i++)
            TurnClient_HandleTick(pInst);
    }
    fail_unless (turnResult == TurnResult_AllocFailNoAnswer);

}
END_TEST

START_TEST (WaitAllocRespNotAut_AllocRspOk)
{
    int ctx;
    ctx = StartAllocateTransaction(5);
    TurnClient_HandleTick(pInst);
    SimAllocResp(ctx, true, true, true, runningAsIPv6);
    fail_unless (turnResult == TurnResult_AllocOk);

    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRspErr_AltServer)
{
    int ctx;
    ctx = StartAllocateTransaction(11);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 3, 0, false, false, true);  /* 300, alt server */
    fail_unless (turnResult == TurnResult_Empty);

    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false); /* 401, has realm and nonce */
    fail_unless (turnResult == TurnResult_Empty);

    TurnClient_HandleTick(pInst);
    SimAllocResp(ctx, true, true, true, runningAsIPv6);
    fail_unless (turnResult == TurnResult_AllocOk);
    
    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRsp_Malf1)
{
    int ctx;
    ctx = StartAllocateTransaction(5);
    TurnClient_HandleTick(pInst);
    SimAllocResp(ctx, false, true, true, runningAsIPv6);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(pInst);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRsp_Malf2)
{
    int ctx;
    ctx = StartAllocateTransaction(5);
    TurnClient_HandleTick(pInst);
    SimAllocResp(ctx, true, false, true, runningAsIPv6);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(pInst);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRsp_Malf3)
{
    int ctx;
    ctx = StartAllocateTransaction(5);
    TurnClient_HandleTick(pInst);
    SimAllocResp(ctx, true, true, false, runningAsIPv6);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(pInst);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRspErr_Ok)
{
    int ctx;
    ctx = StartAllocateTransaction(9);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false); /* 401, has realm and nonce */
    fail_unless (turnResult == TurnResult_Empty);

    TurnClient_HandleTick(pInst);
    SimAllocResp(ctx, true, true, true, runningAsIPv6);
    fail_unless (turnResult == TurnResult_AllocOk);

    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
}
END_TEST

START_TEST (WaitAllocRespNotAut_AllocRspErr_ErrNot401)
{
    int ctx;
    ctx = StartAllocateTransaction(15);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 4, 4, false, false, false); /* 404, no realm, no nonce */
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(pInst);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRspErr_Err_malf1)
{
    int ctx;
    ctx = StartAllocateTransaction(15);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 4, 1, false, true, false); /* 401, no realm, nonce */
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(pInst);
}
END_TEST

START_TEST (WaitAllocRespNotAut_AllocRspErr_Err_malf2)
{
    int ctx;
    ctx = StartAllocateTransaction(15);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, false, false); /* 401, realm, no nonce */
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(pInst);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRspErr_Err_malf3)
{
    int ctx;
    ctx = StartAllocateTransaction(11);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 3, 0, false, false, false);  /* 300, missing alt server */
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(pInst);

}
END_TEST

START_TEST (WaitAllocResp_AllocRespOk)
{
    int ctx;
    ctx = StartAllocateTransaction(9);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false); /* 401, has realm and nonce */
    TurnClient_HandleTick(pInst);
    SimAllocResp(ctx, true, true, true, runningAsIPv6);
    fail_unless (turnResult == TurnResult_AllocOk);
    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
    
}
END_TEST


START_TEST (WaitAllocResp_AllocRespErr)
{
    int ctx;
    ctx = StartAllocateTransaction(9);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false);    /* 401, has realm and nonce */
    fail_unless (turnResult == TurnResult_Empty);
    

    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 4, 4, false, false, false);  /* 404, no realm and no nonce */


    fail_unless (turnResult == TurnResult_AllocUnauthorised);
    
    TurnClient_Deallocate(pInst);

}
END_TEST

START_TEST (WaitAllocResp_Retry)
{
    int ctx, i;
    ctx = StartAllocateTransaction(9);
    TurnClient_HandleTick(pInst);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false);    /* 401, has realm and nonce */
    fail_unless (turnResult == TurnResult_Empty);
    for (i=0; i < 100; i++)
        TurnClient_HandleTick(pInst);

    fail_unless (turnResult == TurnResult_AllocFailNoAnswer);

    TurnClient_Deallocate(pInst);
}
END_TEST

START_TEST (Allocated_RefreshOk)
{
    int ctx, i;

    ctx = GotoAllocatedState(9);

    for (i=0; i < 2; i++)
    {
        Sim_TimerRefreshAlloc(ctx);
        TurnClient_HandleTick(pInst);
        Sim_RefreshResp(ctx);
    }
    fail_unless (turnResult == TurnResult_AllocOk);

    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);


}
END_TEST

START_TEST (Allocated_RefreshError)
{
    int ctx;
    ctx = GotoAllocatedState(4);

    Sim_TimerRefreshAlloc(ctx);
    TurnClient_HandleTick(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_AllocOk);

    Sim_TimerRefreshAlloc(ctx);
    TurnClient_HandleTick(pInst);
    Sim_RefreshError(ctx, 4, 1, false, false);
    fail_unless (turnResult == TurnResult_RefreshFail);

}
END_TEST


START_TEST (Allocated_StaleNonce)
{
    int ctx;
    ctx = GotoAllocatedState(4);

    Sim_TimerRefreshAlloc(ctx);
    TurnClient_HandleTick(pInst);
    Sim_RefreshResp(ctx);

    Sim_TimerRefreshAlloc(ctx);
    TurnClient_HandleTick(pInst);
    Sim_RefreshError(ctx, 4, 38, true, true); /* stale nonce */

    TurnClient_HandleTick(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_AllocOk);

    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);

}
END_TEST

START_TEST (Allocated_ChanBindReqOk)
{
    struct sockaddr_storage peerIp;
    int ctx;
    sockaddr_initFromString((struct sockaddr *)&peerIp,"192.168.5.22:1234");

    ctx = GotoAllocatedState(12);
    TurnClient_StartChannelBindReq(pInst, 0x4001, (struct sockaddr *)&peerIp);
    TurnClient_HandleTick(pInst);
    Sim_ChanBindOrPermissionResp(ctx, STUN_MSG_ChannelBindResponseMsg, 0, 0);
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_ChanBindOk);

    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);

}
END_TEST

START_TEST (Allocated_ChanBindErr)
{
    struct sockaddr_storage peerIp;
    int ctx;
    sockaddr_initFromString((struct sockaddr *)&peerIp,"192.168.5.22:1234");

    ctx = GotoAllocatedState(12);
    TurnClient_StartChannelBindReq(pInst, 0x4001, (struct sockaddr *)&peerIp);
    TurnClient_HandleTick(pInst);
    Sim_ChanBindOrPermissionResp(ctx, STUN_MSG_ChannelBindErrorResponseMsg, 4, 4);
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_ChanBindFail);

    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
}
END_TEST

START_TEST (Allocated_CreatePermissionReqOk)
{
    struct sockaddr_storage peerIp[6];
    struct sockaddr_storage *p_peerIp[6];
    int ctx;
    uint32_t i;

    for (i=0; i < sizeof(peerIp)/sizeof(peerIp[0]); i++)
    {
        sockaddr_initFromString((struct sockaddr *)&peerIp[i],"192.168.5.22:1234");
        p_peerIp[i] = &peerIp[i];
    }

    ctx = GotoAllocatedState(12);
    TurnClient_StartCreatePermissionReq(pInst, sizeof(peerIp)/sizeof(peerIp[0]), (const struct sockaddr**)p_peerIp);
    TurnClient_HandleTick(pInst);
    Sim_ChanBindOrPermissionResp(ctx, STUN_MSG_CreatePermissionResponseMsg, 0, 0);
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_CreatePermissionOk);
    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
}
END_TEST

START_TEST (Allocated_CreatePermissionErr)
{
    struct sockaddr_storage peerIp[3];
    struct sockaddr_storage *p_peerIp[3];
    int ctx;
    uint32_t i;

    for (i=0; i < sizeof(peerIp)/sizeof(peerIp[0]); i++)
    {
        sockaddr_initFromString((struct sockaddr *)&peerIp[i],"192.168.5.22:1234");
        p_peerIp[i] = &peerIp[i];
    }

    ctx = GotoAllocatedState(12);
    TurnClient_StartCreatePermissionReq(pInst, sizeof(peerIp)/sizeof(peerIp[0]), (const struct sockaddr**)p_peerIp);
    TurnClient_HandleTick(pInst);
    Sim_ChanBindOrPermissionResp(ctx, STUN_MSG_CreatePermissionErrorResponseMsg, 4, 4);
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_PermissionRefreshFail);
    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
}
END_TEST

START_TEST (Allocated_CreatePermissionReqAndChannelBind)
{
    struct sockaddr_storage peerIp[6];
    struct sockaddr_storage *p_peerIp[6];
    int ctx;
    uint32_t i;

    for (i=0; i < sizeof(peerIp)/sizeof(peerIp[0]); i++)
    {
        sockaddr_initFromString((struct sockaddr *)&peerIp[i],"192.168.5.22:1234");
        p_peerIp[i] = &peerIp[i];
    }

    ctx = GotoAllocatedState(12);
    TurnClient_StartCreatePermissionReq(pInst, sizeof(peerIp)/sizeof(peerIp[0]), (const struct sockaddr**)p_peerIp);
    TurnClient_StartChannelBindReq(pInst, 0x4001, (struct sockaddr *)&peerIp[0]);

    TurnClient_HandleTick(pInst);
    Sim_ChanBindOrPermissionResp(ctx, STUN_MSG_CreatePermissionResponseMsg, 0, 0);
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_CreatePermissionOk);

    Sim_ChanBindOrPermissionResp(ctx, STUN_MSG_ChannelBindResponseMsg, 0, 0);
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_ChanBindOk);

    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
}
END_TEST

START_TEST (Allocated_CreatePermissionErrorAndChannelBind)
{
    struct sockaddr_storage peerIp[6];
    struct sockaddr_storage *p_peerIp[6];
    int ctx;
    uint32_t i;

    for (i=0; i < sizeof(peerIp)/sizeof(peerIp[0]); i++)
    {
        sockaddr_initFromString((struct sockaddr *)&peerIp[i],"192.168.5.22:1234");
        p_peerIp[i] = &peerIp[i];
    }

    ctx = GotoAllocatedState(12);
    TurnClient_StartCreatePermissionReq(pInst, sizeof(peerIp)/sizeof(peerIp[0]), (const struct sockaddr**)p_peerIp);
    TurnClient_StartChannelBindReq(pInst, 0x4001, (struct sockaddr *)&peerIp[0]);

    TurnClient_HandleTick(pInst);
    Sim_ChanBindOrPermissionResp(ctx, STUN_MSG_CreatePermissionErrorResponseMsg, 4, 4);
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_PermissionRefreshFail);

    Sim_ChanBindOrPermissionResp(ctx, STUN_MSG_ChannelBindResponseMsg, 0, 0);
    TurnClient_HandleTick(pInst);
    fail_unless (turnResult == TurnResult_ChanBindOk);

    TurnClient_Deallocate(pInst);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
}
END_TEST


#if 0
START_TEST( SendIndication )
{
    struct sockaddr_storage addr;
    unsigned char stunBuf[200];
    char message[] = "Some useful data\0";
    int msg_len;
    StunMessage msg;
    
    
    sockaddr_initFromString((struct sockaddr *)&addr,
                            "1.2.3.4:2345");
    
    msg_len = TurnClient_createSendIndication(stunBuf,
                                              message,
                                              sizeof(stunBuf),
                                              strlen(message),
                                              (struct sockaddr *)&addr,
                                              false,
                                              0,
                                              0);
    fail_unless(msg_len == 52);
    
        
    fail_unless( stunlib_DecodeMessage(stunBuf,
                                       msg_len,
                                       &msg,
                                       NULL,
                                       NULL));

    
    fail_unless( msg.msgHdr.msgType == STUN_MSG_SendIndicationMsg );
        
    fail_unless( msg.hasData );
    
    
    fail_unless( 0 == strncmp(&stunBuf[msg.data.offset], message, strlen(message)) );
          
           
    
}
END_TEST
#endif


START_TEST (GetMessageName)
{
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_BindRequestMsg), "BindRequest") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_BindResponseMsg), "BindResponse") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_BindIndicationMsg), "BindInd") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_BindErrorResponseMsg), "BindErrorResponse") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_AllocateRequestMsg), "AllocateRequest") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_AllocateResponseMsg), "AllocateResponse") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_AllocateErrorResponseMsg), "AllocateErrorResponse") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_CreatePermissionRequestMsg), "CreatePermissionReq") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_CreatePermissionResponseMsg), "CreatePermissionResp") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_CreatePermissionErrorResponseMsg), "CreatePermissionError") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_ChannelBindRequestMsg),  "ChannelBindRequest") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_ChannelBindResponseMsg),  "ChannelBindResponse") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_ChannelBindErrorResponseMsg),   "ChannelBindErrorResponse") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_RefreshRequestMsg),   "RefreshRequest") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_RefreshResponseMsg),   "RefreshResponse") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_RefreshErrorResponseMsg),   "RefreshErrorResponse") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_DataIndicationMsg),   "DataIndication") );
    fail_unless( 0 == strcmp(stunlib_getMessageName(STUN_MSG_SendIndicationMsg),   "STUN_MSG_SendInd") );



    fail_unless( 0 == strcmp(stunlib_getMessageName(123),   "???") );

}
END_TEST


START_TEST( SuccessResp )
{
    StunMessage msg;

    msg.msgHdr.msgType = STUN_MSG_BindResponseMsg;
    fail_unless( stunlib_isSuccessResponse(&msg) );

}
END_TEST

START_TEST( ErrorResp )
{
    StunMessage msg;

    msg.msgHdr.msgType = STUN_MSG_BindErrorResponseMsg;
    fail_unless( stunlib_isErrorResponse(&msg) );

}
END_TEST

START_TEST( Resp )
{
    StunMessage msg;

    msg.msgHdr.msgType = STUN_MSG_BindErrorResponseMsg;
    fail_unless( stunlib_isResponse(&msg) );

}
END_TEST

START_TEST( Ind )
{
    StunMessage msg;

    msg.msgHdr.msgType = STUN_MSG_SendIndicationMsg;
    fail_unless( stunlib_isIndication(&msg) );

}
END_TEST


START_TEST( Req )
{
    StunMessage msg;

    msg.msgHdr.msgType = STUN_MSG_BindRequestMsg;
    fail_unless( stunlib_isRequest(&msg) );

}
END_TEST


START_TEST( isTurnChan )
{
    unsigned char req[] =
        "Kinda buffer ready to be overwritten";

    /* We overwrite some of the data, but in this case who cares.. */
    stunlib_encodeTurnChannelNumber(0x4001,
                                    sizeof(req),
                                    (uint8_t*)req);

    fail_unless( stunlib_isTurnChannelData(req) );

}
END_TEST


START_TEST ( GetErrorReason )
{
    fail_unless( 0 == strcmp(stunlib_getErrorReason(3, 0),"Try Alternate" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 0),"Bad Request" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 1),"Unauthorized" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 20),"Unknown Attribute" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 30),"Stale Credentials" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 31),"Integrity Check Failure" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 32),"Missing Username" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 37),"No Binding" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 38),"Stale Nonce" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 41),"Wrong Username" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 42),"Unsupported Transport Protocol" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(5, 00),"Server Error" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(6, 00),"Global Failure" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 86),"Allocation Quota Reached" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(5, 8),"Insufficient Capacity" ) );
    fail_unless( 0 == strcmp(stunlib_getErrorReason(4, 87),"Role Conflict" ) );

    fail_unless( 0 == strcmp(stunlib_getErrorReason(2, 43),"???" ) );
    
}
END_TEST


Suite * turnclient_suite (void)
{
  Suite *s = suite_create ("TURN Client");


  {/* allocate */

      TCase *tc_allocate = tcase_create ("Turnclient Allocate");

      tcase_add_checked_fixture (tc_allocate, setup, teardown);
      
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_Timeout);
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspOk);     
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_AltServer);
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRsp_Malf1);
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRsp_Malf2);
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRsp_Malf3);
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_Ok);
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_ErrNot401);
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_Err_malf1);
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_Err_malf2);
      tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_Err_malf3);
      tcase_add_test (tc_allocate, WaitAllocResp_AllocRespOk);
      tcase_add_test (tc_allocate, WaitAllocResp_AllocRespErr); 
      tcase_add_test (tc_allocate, WaitAllocResp_Retry);
      tcase_add_test (tc_allocate, Allocated_RefreshOk);
      tcase_add_test (tc_allocate, Allocated_RefreshError);
      tcase_add_test (tc_allocate, Allocated_StaleNonce);
      tcase_add_test (tc_allocate, Allocated_ChanBindReqOk); 
      tcase_add_test (tc_allocate, Allocated_ChanBindErr);
      tcase_add_test (tc_allocate, Allocated_CreatePermissionReqOk);
      tcase_add_test (tc_allocate, Allocated_CreatePermissionErr);
      tcase_add_test (tc_allocate, Allocated_CreatePermissionReqAndChannelBind);
      tcase_add_test (tc_allocate, Allocated_CreatePermissionErrorAndChannelBind);


      
      
      
      
      suite_add_tcase (s, tc_allocate);

  }

  {/* allocate IPv6 */
      
      TCase *tc_allocateIPv6 = tcase_create ("Turnclient Allocate IPv6");

      tcase_add_checked_fixture (tc_allocateIPv6, setupIPv6, teardownIPv6);

      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_Timeout);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRspOk);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRspErr_AltServer);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRsp_Malf1);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRsp_Malf2);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRsp_Malf3);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRspErr_Ok);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRspErr_ErrNot401);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRspErr_Err_malf1);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRspErr_Err_malf2);
      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_AllocRspErr_Err_malf3);
      tcase_add_test (tc_allocateIPv6, WaitAllocResp_AllocRespOk);
      tcase_add_test (tc_allocateIPv6, WaitAllocResp_AllocRespErr); 
      tcase_add_test (tc_allocateIPv6, WaitAllocResp_Retry);
      tcase_add_test (tc_allocateIPv6, Allocated_RefreshOk);
      tcase_add_test (tc_allocateIPv6, Allocated_RefreshError);
      tcase_add_test (tc_allocateIPv6, Allocated_StaleNonce);
      tcase_add_test (tc_allocateIPv6, Allocated_ChanBindReqOk); 
      tcase_add_test (tc_allocateIPv6, Allocated_ChanBindErr);
      tcase_add_test (tc_allocateIPv6, Allocated_CreatePermissionReqOk);
      tcase_add_test (tc_allocateIPv6, Allocated_CreatePermissionErr);
      tcase_add_test (tc_allocateIPv6, Allocated_CreatePermissionReqAndChannelBind);
      tcase_add_test (tc_allocateIPv6, Allocated_CreatePermissionErrorAndChannelBind);

      

      suite_add_tcase (s, tc_allocateIPv6);

  }

  {/* Misc */

      TCase *tc_misc = tcase_create ("TURN Misc");
      
      //tcase_add_test ( tc_misc, SendIndication );
       tcase_add_test ( tc_misc, GetMessageName );
      tcase_add_test ( tc_misc, SuccessResp );
      tcase_add_test ( tc_misc, ErrorResp );
      tcase_add_test ( tc_misc, Resp );
      tcase_add_test ( tc_misc, Ind );
      tcase_add_test ( tc_misc, Req );
      tcase_add_test ( tc_misc, isTurnChan );
      tcase_add_test ( tc_misc, GetErrorReason );
      //tcase_add_test ( tc_misc, print );
      suite_add_tcase (s, tc_misc);

  }

  return s;
}
