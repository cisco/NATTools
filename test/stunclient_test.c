
#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "turnclient.h"
#include "turn_intern.h"
#include "sockaddr_util.h"

Suite * stunclient_suite (void);


#define  MAX_INSTANCES  50
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


static  const uint8_t StunCookie[]   = STUN_MAGIC_COOKIE_ARRAY;

TurnResult_T turnResult;

struct sockaddr_storage turnServerAddr;





static void TurnStatusCallBack(void *ctx, TurnCallBackData_T *retData)
{

    turnResult = retData->turnResult;
    //printf("Got TURN status callback\n");

}


/* Callback for management info  */
static void  PrintTurnInfo(TurnInfoCategory_T category, char *ErrStr)
{
    //fprintf(stderr, "%s\n", ErrStr);
}


static int SendRawStun(int              globalSocketId, 
                       uint8_t         *buffer, 
                       int              bufLen, 
                       struct sockaddr *dstAddr,
                       socklen_t        t,
                       void            *userData)
{
    char addr_str[SOCKADDR_MAX_STRLEN];
    /* find the transaction id  so we can use this in the simulated resp */
    memcpy(&LastTransId, &buffer[8], STUN_MSG_ID_SIZE);
    sockaddr_copy((struct sockaddr *)&LastAddress, dstAddr);
    
    sockaddr_toString(dstAddr, addr_str, SOCKADDR_MAX_STRLEN, true);
                      
    //printf("Sendto: '%s'\n", addr_str);

    return 1;
}


static int StartAllocateTransaction(int n)
{
    //struct sockaddr_storage addr;

    CurrAppCtx.a =  AppCtx[n].a = 100+n;
    CurrAppCtx.b =  AppCtx[n].b = 200+n;
    


    /* kick off turn */
    return TurnClient_startAllocateTransaction(TEST_THREAD_CTX,
                                               &AppCtx[n],
                                               (struct sockaddr *)&turnServerAddr,
                                               "pem",
                                               "pem",
                                               0,                       /* socket */
                                               SendRawStun,             /* send func */
                                               StunDefaultTimeoutList,  /* timeout list */
                                               TurnStatusCallBack,
                                               &TurnCbData[n],
                                               false);
}



static void SimAllocResp(int ctx, bool relay, bool xorMappedAddr, bool lifetime)
{
    StunMessage m;
    memset(&m, 0, sizeof(m));
    memcpy(&m.msgHdr.id, &LastTransId, STUN_MSG_ID_SIZE);  
    memcpy(&m.msgHdr.cookie, StunCookie, sizeof(m.msgHdr.cookie));
    m.msgHdr.msgType = STUN_MSG_AllocateResponseMsg;

    /* relay */
    if (relay)
    {
        m.hasXorRelayAddress = true,
            m.xorRelayAddress.addr.v4.addr = 3251135384UL;// "193.200.99.152"
        m.xorRelayAddress.addr.v4.port = 42000;
    }

    /* XOR mapped addr*/
    if (xorMappedAddr)
    {
        m.hasXorMappedAddress = true;
        m.xorMappedAddress.addr.v4.addr = 1009527574UL;// "60.44.43.22");
        m.xorMappedAddress.addr.v4.port = 43000;
    }

    /* lifetime */
    if (lifetime)
    {
        m.hasLifetime = true;
        m.lifetime.value = 60;
    }

    TurnClient_HandleIncResp(TEST_THREAD_CTX, ctx, &m);

}


static void  Sim_ChanBindResp(int ctx, uint16_t msgType, uint32_t errClass, uint32_t errNumber)
{
    StunMessage m;
    memset(&m, 0, sizeof(m));
    memcpy(&m.msgHdr.id, &LastTransId, STUN_MSG_ID_SIZE);  
    memcpy(&m.msgHdr.cookie, StunCookie, sizeof(m.msgHdr.cookie));
    m.msgHdr.msgType = msgType;

    if (msgType == STUN_MSG_ChannelBindErrorResponseMsg)
    {
        m.hasErrorCode          = true;
        m.errorCode.errorClass  = errClass;
        m.errorCode.number      = errNumber;
    }
    TurnClient_HandleIncResp(TEST_THREAD_CTX, ctx, &m);
}



static void SimInitialAllocRespErr(int ctx, bool hasErrCode, uint32_t errClass, uint32_t errNumber, 
                                   bool hasRealm, bool hasNonce, bool hasAltServer, const char *ip4Str)
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
    if (hasAltServer && ip4Str) 
    {
        m.hasAlternateServer = true;
        m.alternateServer.familyType = STUN_ADDR_IPv4Family;


    //    inet_pton(AF_INET, ip4Str, &m.alternateServer.addr.v4.addr);
    //    m.alternateServer.addr.v4.addr = ntohl(m.alternateServer.addr.v4.addr);
        m.alternateServer.addr.v4.addr = 0x12345678;
        m.alternateServer.addr.v4.port = 3478;
    }


    TurnClient_HandleIncResp(TEST_THREAD_CTX, ctx, &m);
}





static void  Sim_RefreshResp(int ctx)
{
    TurnClientSimulateSig(TEST_THREAD_CTX, ctx, TURN_SIGNAL_RefreshResp);
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

    TurnClient_HandleIncResp(TEST_THREAD_CTX, ctx, &m);
}



static int GotoAllocatedState(int appCtx)
{
    int ctx = StartAllocateTransaction(appCtx);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false, NULL); /* 401, has realm has nonce */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimAllocResp(ctx, true, true, true);
    return ctx;
}

static void  Sim_TimerRefreshAlloc(int ctx)
{
    TurnClientSimulateSig(TEST_THREAD_CTX, ctx, TURN_SIGNAL_TimerRefreshAlloc);
}




static void setup (void)
{
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "UnitTestSofware");
    sockaddr_initFromString((struct sockaddr*)&turnServerAddr, "193.200.93.152:3478");

}

static void teardown (void)
{
    turnResult = TurnResult_Empty; 
}


static void setupIPv6 (void)
{
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "UnitTestSofware");
    sockaddr_initFromString((struct sockaddr*)&turnServerAddr, "[2001:470:dc88:2:226:18ff:fe92:6d53]:3478");

}

static void teardownIPv6 (void)
{
    turnResult = TurnResult_Empty; 
}




START_TEST (turnclient_init)
{

    int ret;
    ret = TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "UnitTestSofware");
    fail_unless(ret);

}
END_TEST

START_TEST (WaitAllocRespNotAut_Timeout)
{
    int ctx;

    ctx = StartAllocateTransaction(0);
    /* 1 Tick */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_Empty);
    fail_unless (sockaddr_alike((struct sockaddr *)&LastAddress, 
                                (struct sockaddr *)&turnServerAddr));
    /* 2 Tick */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_Empty);

    /* 3 Tick */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_Empty);

    /* 4 Tick */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_AllocFailNoAnswer);

    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRspOk)
{
    int ctx;
    ctx = StartAllocateTransaction(5);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimAllocResp(ctx, true, true, true);
    fail_unless (turnResult == TurnResult_AllocOk);

    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);

}
END_TEST

START_TEST (WaitAllocRespNotAut_AllocRspErr_AltServer)
{
    int ctx;
    ctx = StartAllocateTransaction(11);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 3, 0, false, false, true, "192.168.6.7");  /* 300, alt server */
    fail_unless (turnResult == TurnResult_Empty);

    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false, NULL); /* 401, has realm and nonce */
    fail_unless (turnResult == TurnResult_Empty);

    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimAllocResp(ctx, true, true, true);
    fail_unless (turnResult == TurnResult_AllocOk);
    
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRsp_Malf1)
{
    int ctx;
    ctx = StartAllocateTransaction(5);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimAllocResp(ctx, false, true, true);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRsp_Malf2)
{
    int ctx;
    ctx = StartAllocateTransaction(5);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimAllocResp(ctx, true, false, true);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRsp_Malf3)
{
    int ctx;
    ctx = StartAllocateTransaction(5);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimAllocResp(ctx, true, true, false);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRspErr_Ok)
{
    int ctx;
    ctx = StartAllocateTransaction(9);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false, NULL); /* 401, has realm and nonce */
    fail_unless (turnResult == TurnResult_Empty);

    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimAllocResp(ctx, true, true, true);
    fail_unless (turnResult == TurnResult_AllocOk);

    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
}
END_TEST

START_TEST (WaitAllocRespNotAut_AllocRspErr_ErrNot401)
{
    int ctx;
    ctx = StartAllocateTransaction(15);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 4, false, false, false, NULL); /* 404, no realm, no nonce */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRspErr_Err_malf1)
{
    int ctx;
    ctx = StartAllocateTransaction(15);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 1, false, true, false, NULL); /* 401, no realm, nonce */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
}
END_TEST

START_TEST (WaitAllocRespNotAut_AllocRspErr_Err_malf2)
{
    int ctx;
    ctx = StartAllocateTransaction(15);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, false, false, NULL); /* 401, realm, no nonce */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST


START_TEST (WaitAllocRespNotAut_AllocRspErr_Err_malf3)
{
    int ctx;
    ctx = StartAllocateTransaction(11);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 3, 0, false, false, false, NULL);  /* 300, missing alt server */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_MalformedRespWaitAlloc);
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST

START_TEST (WaitAllocResp_AllocRespOk)
{
    int ctx;
    ctx = StartAllocateTransaction(9);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false, NULL); /* 401, has realm and nonce */
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimAllocResp(ctx, true, true, true);
    fail_unless (turnResult == TurnResult_AllocOk);
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
    
}
END_TEST


START_TEST (WaitAllocResp_AllocRespErr)
{
    int ctx;
    ctx = StartAllocateTransaction(9);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false, NULL);    /* 401, has realm and nonce */
    fail_unless (turnResult == TurnResult_Empty);
    

    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 4, false, false, false, NULL);  /* 404, no realm and no nonce */


    fail_unless (turnResult == TurnResult_AllocUnauthorised);
    
    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST

START_TEST (WaitAllocResp_Retry)
{
    int ctx, i;
    ctx = StartAllocateTransaction(9);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    SimInitialAllocRespErr(ctx, true, 4, 1, true, true, false, NULL);    /* 401, has realm and nonce */
    fail_unless (turnResult == TurnResult_Empty);
    for (i=0; i < 4; i++)
        TurnClient_HandleTick(TEST_THREAD_CTX);

    fail_unless (turnResult == TurnResult_AllocFailNoAnswer);

    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
}
END_TEST

START_TEST (Allocated_RefreshOk)
{
    int ctx, i;

    ctx = GotoAllocatedState(9);

    for (i=0; i < 2; i++)
    {
        Sim_TimerRefreshAlloc(ctx);
        TurnClient_HandleTick(TEST_THREAD_CTX);
        Sim_RefreshResp(ctx);
    }
    fail_unless (turnResult == TurnResult_AllocOk);

    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);


}
END_TEST

START_TEST (Allocated_RefreshError)
{
    int ctx;
    ctx = GotoAllocatedState(4);

    Sim_TimerRefreshAlloc(ctx);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_AllocOk);

    Sim_TimerRefreshAlloc(ctx);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    Sim_RefreshError(ctx, 4, 1, false, false);
    fail_unless (turnResult == TurnResult_RefreshFail);

}
END_TEST


START_TEST (Allocated_StaleNonce)
{
    int ctx;
    ctx = GotoAllocatedState(4);

    Sim_TimerRefreshAlloc(ctx);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    Sim_RefreshResp(ctx);

    Sim_TimerRefreshAlloc(ctx);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    Sim_RefreshError(ctx, 4, 38, true, true); /* stale nonce */

    TurnClient_HandleTick(TEST_THREAD_CTX);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_AllocOk);

    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
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
    TurnClient_StartChannelBindReq(TEST_THREAD_CTX, ctx, 0x4001, (struct sockaddr *)&peerIp);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    Sim_ChanBindResp(ctx, STUN_MSG_ChannelBindResponseMsg, 0, 0);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_ChanBindOk);

    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
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
    TurnClient_StartChannelBindReq(TEST_THREAD_CTX, ctx, 0x4001, (struct sockaddr *)&peerIp);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    Sim_ChanBindResp(ctx, STUN_MSG_ChannelBindErrorResponseMsg, 4, 4);
    TurnClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (turnResult == TurnResult_ChanBindFail);

    TurnClient_Deallocate(TEST_THREAD_CTX, ctx);
    Sim_RefreshResp(ctx);
    fail_unless (turnResult == TurnResult_RelayReleaseComplete);
}
END_TEST



Suite * stunclient_suite (void)
{
  Suite *s = suite_create ("TURN Client");


  {/* Init */

      TCase *tc_init = tcase_create ("Turnclient Init");
      tcase_add_test (tc_init, turnclient_init);
      
      suite_add_tcase (s, tc_init);

  }


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
      suite_add_tcase (s, tc_allocate);

  }


  {/* allocate IPv6 */

      TCase *tc_allocateIPv6 = tcase_create ("Turnclient Allocate IPv6");

      tcase_add_checked_fixture (tc_allocateIPv6, setupIPv6, teardownIPv6);

      tcase_add_test (tc_allocateIPv6, WaitAllocRespNotAut_Timeout);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspOk);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_AltServer);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRsp_Malf1);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRsp_Malf2);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRsp_Malf3);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_Ok);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_ErrNot401);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_Err_malf1);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_Err_malf2);
      //tcase_add_test (tc_allocate, WaitAllocRespNotAut_AllocRspErr_Err_malf3);
      //tcase_add_test (tc_allocate, WaitAllocResp_AllocRespOk);
      //tcase_add_test (tc_allocate, WaitAllocResp_AllocRespErr); 
      //tcase_add_test (tc_allocate, WaitAllocResp_Retry);
      //tcase_add_test (tc_allocate, Allocated_RefreshOk);
      //tcase_add_test (tc_allocate, Allocated_RefreshError);
      //tcase_add_test (tc_allocate, Allocated_StaleNonce);
      //tcase_add_test (tc_allocate, Allocated_ChanBindReqOk); 
      //tcase_add_test (tc_allocate, Allocated_ChanBindErr);
      suite_add_tcase (s, tc_allocateIPv6);

  }




  return s;
}
