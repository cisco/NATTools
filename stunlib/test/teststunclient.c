#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "stunclient.h"
#include "sockaddr_util.h"

Suite * stunclient_suite (void);


#define  MAX_INSTANCES  50
#define  TEST_THREAD_CTX 1

#define  TEST_IPv4_ADDR 
#define  TEST_IPv4_PORT
#define  TEST_IPv6_ADDR 

typedef  struct
{
    uint32_t a;
    uint8_t  b;
}
AppCtx_T;

static AppCtx_T           AppCtx[MAX_INSTANCES];
static StunCallBackData_T StunCbData[MAX_INSTANCES];

static AppCtx_T CurrAppCtx;

//static int            TestNo;
static uint32_t       StunDefaultTimeoutList[STUNCLIENT_MAX_RETRANSMITS] = {100, 0};
static StunMsgId      LastTransId;
static struct sockaddr_storage LastAddress;
static bool runningAsIPv6;

const uint8_t test_addr_ipv6[16] = {0x20, 0x1, 0x4, 0x70, 0xdc, 0x88, 0x1, 0x22, 0x21, 0x26, 0x18, 0xff, 0xfe, 0x92, 0x6d, 0x53};
const uint64_t test_addr_ipv4 = 1009527574;// "60.44.43.22");
const uint32_t test_port_ipv4 = 43000;

static  const uint8_t StunCookie[]   = STUN_MAGIC_COOKIE_ARRAY;

StunResult_T stunResult;

struct sockaddr_storage stunServerAddr;

static void StunStatusCallBack(void *ctx, StunCallBackData_T *retData)
{
    stunResult = retData->stunResult;
    //printf("Got STUN status callback (Result (%i)\n", retData->stunResult);

}

/* Callback for management info  */
static void  PrintStunInfo(StunInfoCategory_T category, char *InfoStr)
{
    //fprintf(stderr, "%s\n", ErrStr);
}

static int SendRawStun(int sockfd, 
                       uint8_t *buf, 
                       int len, 
                       struct sockaddr *addr,
                       socklen_t t,
                       void *userdata)
{
    char addr_str[SOCKADDR_MAX_STRLEN];
    /* find the transaction id  so we can use this in the simulated resp */

    memcpy(&LastTransId, &buf[8], STUN_MSG_ID_SIZE); 

    sockaddr_copy((struct sockaddr *)&LastAddress, addr);
    
    sockaddr_toString(addr, addr_str, SOCKADDR_MAX_STRLEN, true);
                      
    //printf("Sendto: '%s'\n", addr_str);


    return 1;
}

static int StartBindTransaction(int n)
{
    //struct sockaddr_storage addr;

    CurrAppCtx.a =  AppCtx[n].a = 100+n;
    CurrAppCtx.b =  AppCtx[n].b = 200+n;
    

    /* kick off stun */
    return StunClient_startBindTransaction(TEST_THREAD_CTX,
                                           &AppCtx[n],
                                           (struct sockaddr *)&stunServerAddr,
                                           NULL,
                                           false,
                                           "pem",
                                           "pem",
                                           0, // uint32_t 1845494271 (priority)
                                           false,
                                           false,
                                           0, // uint64_t 0x932FF9B151263B36LL (tieBreaker)
                                           LastTransId,
                                           0,                       /* socket */
                                           SendRawStun,             /* send func */
                                           StunDefaultTimeoutList,  /* timeout list */
                                           StunStatusCallBack,
                                           &StunCbData[n],
                                           -1);
}

static void SimBindSuccessResp(int ctx, bool IPv6)
{
    StunMessage m;
    memset(&m, 0, sizeof(m));
    memcpy(&m.msgHdr.id, &LastTransId, STUN_MSG_ID_SIZE);  
    memcpy(&m.msgHdr.cookie, StunCookie, sizeof(m.msgHdr.cookie));
    m.msgHdr.msgType = STUN_MSG_BindResponseMsg;
    m.hasXorMappedAddress = true;

    if(IPv6){
        stunlib_setIP6Address(&m.xorMappedAddress, test_addr_ipv6, 0x4200);

    }else{
        m.xorMappedAddress.familyType = STUN_ADDR_IPv4Family;
        m.xorMappedAddress.addr.v4.addr = test_addr_ipv4;
        m.xorMappedAddress.addr.v4.port = test_port_ipv4;
    }

    StunClient_HandleIncResp(TEST_THREAD_CTX, &m, NULL);

}

static void setup (void)
{
    runningAsIPv6 = false;
    StunClient_Init(TEST_THREAD_CTX, 50, 50, PrintStunInfo, false, "UnitTestSofware");
    sockaddr_initFromString((struct sockaddr*)&stunServerAddr, "193.200.93.152:3478");
}

static void teardown (void)
{
    stunResult = StunResult_Empty; 
}


static void setupIPv6 (void)
{
    runningAsIPv6 = true;
    StunClient_Init(TEST_THREAD_CTX, 50, 50, PrintStunInfo, false, "UnitTestSofware");
    sockaddr_initFromString((struct sockaddr*)&stunServerAddr, "[2001:470:dc88:2:226:18ff:fe92:6d53]:3478");

}

static void teardownIPv6 (void)
{
    stunResult = StunResult_Empty; 
}

START_TEST (stunclient_init)
{

    int ret;
    ret = StunClient_Init(TEST_THREAD_CTX, 50, 50, PrintStunInfo, false, "UnitTestSofware");
    fail_unless(ret);

}
END_TEST

START_TEST (WaitBindRespNotAut_Timeout)
{
    fail_unless (stunResult == StunResult_Empty);
    int ctx;

    ctx = StartBindTransaction(0);
    /* 1 Tick */
    StunClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (stunResult == StunResult_Empty);
    fail_unless (sockaddr_alike((struct sockaddr *)&LastAddress, 
                                (struct sockaddr *)&stunServerAddr));
    /* 2 Tick */
    StunClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (stunResult == StunResult_Empty);

    /* 3 Tick */
    StunClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (stunResult == StunResult_Empty);

    /* 4 Tick */
    StunClient_HandleTick(TEST_THREAD_CTX);
    fail_unless (stunResult == StunResult_BindFailNoAnswer);

    StunClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST


START_TEST (WaitBindRespNotAut_BindSuccess)
{
    printf("IPv6: %s\n", runningAsIPv6? "True" : "False");
    int ctx;
    ctx = StartBindTransaction(5);
    StunClient_HandleTick(TEST_THREAD_CTX);
    SimBindSuccessResp(ctx, runningAsIPv6);
    fail_unless (stunResult == StunResult_BindOk);

    StunClient_Deallocate(TEST_THREAD_CTX, ctx);

}
END_TEST

Suite * stunclient_suite (void)
{
  Suite *s = suite_create ("Stunclient");


  {/* Init */

      TCase *sc_init = tcase_create ("Stunclient Init");
      
      tcase_add_test (sc_init, stunclient_init);
      
      suite_add_tcase (s, sc_init);

  }


  {/* bind */

      TCase *sc_allocate = tcase_create ("Stunclient Bind");

      tcase_add_checked_fixture (sc_allocate, setup, teardown);
      
      tcase_add_test (sc_allocate, WaitBindRespNotAut_Timeout);
      tcase_add_test (sc_allocate, WaitBindRespNotAut_BindSuccess);
      
      suite_add_tcase (s, sc_allocate);

  }


  {/* bind IPv6 */

      TCase *sc_allocateIPv6 = tcase_create ("Stunclient Bind IPv6");

      tcase_add_checked_fixture (sc_allocateIPv6, setupIPv6, teardownIPv6);

      tcase_add_test (sc_allocateIPv6, WaitBindRespNotAut_Timeout);
      tcase_add_test (sc_allocateIPv6, WaitBindRespNotAut_BindSuccess);

      suite_add_tcase (s, sc_allocateIPv6);

  }

  {/* Misc */

      TCase *sc_misc = tcase_create ("Stun Misc");
      
      // tcase_add_test ( sc_misc, SendIndication );

      suite_add_tcase (s, sc_misc);

  }
  

  return s;
}
