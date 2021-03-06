#include <stdlib.h>
#include <stdio.h>

#include <netinet/in.h>
#include <netdb.h>

#include <check.h>


#include "sockaddr_util.h"

const char ipv4_1_str[] = "192.168.2.1:4567\0";
const char ipv4_2_str[] = "10.47.1.58:4567\0";
const char ipv4_4_str[] = "158.38.48.10:80\0";
const char ipv6_1_str[] = "[2001:470:dc88:2:226:18ff:fe92:6d53]:3456\0";
const char ipv6_2_str[] = "[2001:470:fc88:1:226:1800:fe23:6d51]:3456\0";
const char ipv6_4_str[] = "[2001:470:fc88:1:226:1800:fe23:6d51]:4567\0";

const char ipv4_any_str[] = "0.0.0.0:0\0";
const char ipv6_any_str[] = "[::]:0\0";

const char ipv4_1_noport_str[] = "192.168.2.1\0";
const char ipv6_1_noport_str[] = "2001:470:dc88:2:226:18ff:fe92:6d53\0";

struct sockaddr_storage  *sockaddr_IPv4_1,
                         *sockaddr_IPv4_2,
                         *sockaddr_IPv4_3,
                         *sockaddr_IPv4_4;

struct sockaddr_storage  *sockaddr_IPv6_1,
                         *sockaddr_IPv6_2,
                         *sockaddr_IPv6_3,
                         *sockaddr_IPv6_4;

struct sockaddr *sockaddr_uninit;

struct sockaddr_storage *sockaddr_IPv4_any,
                        *sockaddr_IPv4_any_2,
                        *sockaddr_IPv6_any,
                        *sockaddr_IPv6_any_2;

void sockaddr_setup (void);
void sockaddr_teardown (void);
Suite * sockaddr_suite (void);

void
sockaddr_setup (void)
{


    //IPv4
    sockaddr_IPv4_1 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));

    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv4_1, ipv4_1_str);




    sockaddr_IPv4_2 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv4_2, ipv4_2_str);

    sockaddr_IPv4_3 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv4_3, ipv4_1_str);

    sockaddr_IPv4_4 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv4_4, ipv4_4_str);

//IPv6
    sockaddr_IPv6_1 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv6_1, ipv6_1_str);

    sockaddr_IPv6_2 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv6_2, ipv6_2_str);

    sockaddr_IPv6_3 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv6_3, ipv6_1_str);

    sockaddr_IPv6_4 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv6_4, ipv6_4_str);

    //unitialized
    sockaddr_uninit = (struct sockaddr *)malloc(sizeof(struct sockaddr_storage));


    //any
    sockaddr_IPv4_any = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv4_any, ipv4_any_str);

    sockaddr_IPv4_any_2 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initAsIPv4Any((struct sockaddr_in *)sockaddr_IPv4_any_2);

    sockaddr_IPv6_any = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initFromString((struct sockaddr *)sockaddr_IPv6_any, ipv6_any_str);

    sockaddr_IPv6_any_2 = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
    sockaddr_initAsIPv6Any((struct sockaddr_in6 *)sockaddr_IPv6_any_2);
}



void
sockaddr_teardown (void)
{
    free(sockaddr_IPv4_1);
    free(sockaddr_IPv4_2);
    free(sockaddr_IPv4_3);
    free(sockaddr_IPv4_4);
    free(sockaddr_IPv6_1);
    free(sockaddr_IPv6_2);
    free(sockaddr_IPv6_3);
    free(sockaddr_IPv6_4);
    free(sockaddr_IPv4_any);
    free(sockaddr_IPv4_any_2);
    free(sockaddr_IPv6_any);
    free(sockaddr_IPv6_any_2);
}



START_TEST (test_sockaddr_create )
{
    fail_unless( ((struct sockaddr *)sockaddr_IPv4_1)->sa_family == AF_INET,
                 "Addrinfo got wrong family");

    fail_unless( ((struct sockaddr *)sockaddr_IPv6_1)->sa_family == AF_INET6,
                 "Addrinfo got wrong family");

    fail_unless( ((struct sockaddr *)sockaddr_IPv6_2)->sa_family == AF_INET6,
                 "Addrinfo got wrong family");

}
END_TEST

START_TEST (test_sockaddr_int_IPv4_init)
{

    char addr_str[256];
    uint32_t a = 2653302794UL;
    uint16_t port = 4567;
    struct sockaddr_storage a_addr;
    struct sockaddr_storage b_addr;


    sockaddr_initFromIPv4Int((struct sockaddr_in *)&a_addr, htonl(a), htons(port));
    sockaddr_initFromIPv4String((struct sockaddr_in *)&b_addr, "158.38.48.10:4567");

    sockaddr_toString( (struct sockaddr *)&a_addr,
                       addr_str,
                       256,
                       true);

    fail_unless( sockaddr_alike( (struct sockaddr *)&a_addr, (struct sockaddr *)&b_addr) );

    fail_if(sockaddr_initFromIPv4String((struct sockaddr_in *)&b_addr, "158.38.48.10.45"));

    fail_if(sockaddr_initFromIPv4String((struct sockaddr_in *)&b_addr, "158.38.48.10.45.45.67.87:4556"));



}
END_TEST


START_TEST (test_sockaddr_int_IPv6_init)
{

    char addr_str[256];
    char addr_str_small[4];
    uint8_t a[16] = {0x20, 0x1, 0x4, 0x70, 0xdc, 0x88, 0x0, 0x2, 0x2, 0x26, 0x18, 0xff, 0xfe, 0x92, 0x6d, 0x53};
    uint16_t port = 4567;
    struct sockaddr_storage a_addr;
    struct sockaddr_storage b_addr;

    sockaddr_initFromIPv6Int((struct sockaddr_in6 *)&a_addr, a, htons(port));

    fail_unless(sockaddr_initFromIPv6String((struct sockaddr_in6 *)&b_addr, "[2001:470:dc88:2:226:18ff:fe92:6d53]:4567"));

    sockaddr_toString( (struct sockaddr *)&a_addr,
                       addr_str,
                       256,
                       true);


    fail_unless( sockaddr_alike( (struct sockaddr *)&a_addr, (struct sockaddr *)&b_addr) );


    fail_if( sockaddr_initFromIPv6String((struct sockaddr_in6 *)&b_addr, "[2001:470:dc88:2:226:18ff:fe92:6d53"));

    fail_if( sockaddr_initFromIPv6String((struct sockaddr_in6 *)&b_addr, "[2001:470:dc88:2:226:18ff:fe92:6d53:456]"));


    fail_if( sockaddr_initFromIPv6String((struct sockaddr_in6 *)&b_addr, "2001:470:dc88:2:226:18ff:fe92:6d53:4567]:4567"));

}
END_TEST

START_TEST (sockaddr_IPv4_sameaddr)
{

    fail_unless( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv4_1,
                                   (struct sockaddr *)sockaddr_IPv4_1),
                 "samaddr failed");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv4_1,
                                   (struct sockaddr *)sockaddr_IPv4_3),
                 "samaddr failed");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv4_any,
                                   (struct sockaddr *)sockaddr_IPv4_any_2),
                 "samaddr failed");


    fail_if   ( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv4_1,
                                  (struct sockaddr *)sockaddr_IPv4_2),
                  "samaddr failed");

    fail_if   ( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv4_1,
                                  (struct sockaddr *)sockaddr_IPv6_1 ),
                "sameaddr failed");
}
END_TEST

START_TEST (sockaddr_IPv6_sameaddr)
{

    fail_unless( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv6_1,
                                   (struct sockaddr *)sockaddr_IPv6_1),
                 "samaddr failed");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv6_3,
                                   (struct sockaddr *)sockaddr_IPv6_3),
                 "samaddr failed");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv6_1,
                                   (struct sockaddr *)sockaddr_IPv6_3),
                 "samaddr failed");

    fail_unless( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv6_any,
                                   (struct sockaddr *)sockaddr_IPv6_any_2),
                 "samaddr failed");


    fail_if   ( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv6_1,
                                  (struct sockaddr *)sockaddr_IPv6_2),
                  "samaddr failed");

    fail_if   ( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv6_1,
                                  (struct sockaddr *)sockaddr_IPv6_2),
                  "samaddr failed");

    fail_if   ( sockaddr_sameAddr((struct sockaddr *)sockaddr_IPv6_1,
                                  (struct sockaddr *)sockaddr_IPv4_1 ),
                "sameaddr failed");
}
END_TEST

START_TEST (sockaddr_IPv4_sameport)
{
    fail_unless ( sockaddr_samePort( (struct sockaddr *)sockaddr_IPv4_1,
                                     (struct sockaddr *)sockaddr_IPv4_1),
                  "sockaddr samePort failed");

    fail_unless ( sockaddr_samePort( (struct sockaddr *)sockaddr_IPv4_1,
                                     (struct sockaddr *)sockaddr_IPv4_2),
                  "sockaddr samePort failed");

    fail_if ( sockaddr_samePort( (struct sockaddr *)sockaddr_IPv4_1,
                                 (struct sockaddr *)sockaddr_IPv4_4),
              "sockaddr samePort failed");

    fail_if ( sockaddr_samePort( (struct sockaddr *)sockaddr_IPv4_1,
                                 (struct sockaddr *)sockaddr_IPv6_4),
              "sockaddr samePort failed");
}
END_TEST


START_TEST (sockaddr_IPv6_sameport)
{
        fail_unless ( sockaddr_samePort( (struct sockaddr *)sockaddr_IPv6_1,
                                         (struct sockaddr *)sockaddr_IPv6_1),
                  "sockaddr samePort failed");

        fail_unless ( sockaddr_samePort( (struct sockaddr *)sockaddr_IPv6_1,
                                         (struct sockaddr *)sockaddr_IPv6_2),
                      "sockaddr samePort failed");
        fail_if ( sockaddr_samePort( (struct sockaddr *)sockaddr_IPv6_1,
                                     (struct sockaddr *)sockaddr_IPv6_4),
              "sockaddr samePort failed");


}
END_TEST

START_TEST (sockaddr_IPv4_alike)
{
    fail_unless( sockaddr_alike((struct sockaddr *)sockaddr_IPv4_1,
                                (struct sockaddr *)sockaddr_IPv4_1),
                 "alike failed");

    fail_if( sockaddr_alike((struct sockaddr *)sockaddr_IPv4_1,
                            (struct sockaddr *)sockaddr_IPv4_2),
                 "alike failed");


}
END_TEST


START_TEST (sockaddr_IPv6_alike)
{
    fail_unless( sockaddr_alike((struct sockaddr *)sockaddr_IPv6_1,
                                (struct sockaddr *)sockaddr_IPv6_1),
                 "alike failed");

    fail_if( sockaddr_alike((struct sockaddr *)sockaddr_IPv6_1,
                            (struct sockaddr *)sockaddr_IPv6_2),
                 "alike failed");

}
END_TEST


START_TEST (sockaddr_IPv4_isSet)
{
    fail_unless( sockaddr_isSet((struct sockaddr *)sockaddr_IPv4_1),
                 "isSet failed");


}
END_TEST



START_TEST (sockaddr_IPv6_isSet)
{
    fail_unless( sockaddr_isSet((struct sockaddr *)sockaddr_IPv6_1),
                 "isSet failed");

    fail_if( sockaddr_isSet(sockaddr_uninit),
             "isSet failed");

}
END_TEST

START_TEST (sockaddr_IPv4_isAny)
{

    struct sockaddr_storage addr;

    addr.ss_family = 12;

    fail_unless( sockaddr_isAddrAny((struct sockaddr *)sockaddr_IPv4_any),
                 "isAny failed");

    fail_if( sockaddr_isAddrAny((struct sockaddr *)sockaddr_IPv4_1),
             "isAny failed");

    fail_if( sockaddr_isAddrAny( (struct sockaddr *)&addr ));

    


}
END_TEST


START_TEST (sockaddr_IPv6_isAny)
{
    fail_if(  sockaddr_isAddrAny((struct sockaddr *)sockaddr_IPv6_1),
             "isAny failed");

    fail_unless( sockaddr_isAddrAny((struct sockaddr *)sockaddr_IPv6_any),
                 "isAny failed");

    fail_unless( sockaddr_isAddrAny((struct sockaddr *)sockaddr_IPv6_any_2),
                 "isAny failed");

}
END_TEST

START_TEST (sockaddr_IPv4_toString)
{
    char ipaddr[SOCKADDR_MAX_STRLEN];
    char ipaddr_small[10];
    struct sockaddr_storage ss;

    fail_unless( strncmp(ipv4_1_str,
                         sockaddr_toString((const struct sockaddr *)sockaddr_IPv4_1,
                                           ipaddr,
                                           SOCKADDR_MAX_STRLEN,
                                           true),
                         INET_ADDRSTRLEN) == 0 ,
                 "sockaddr toString failed ('%s'", ipaddr);

    fail_unless( strncmp(ipv4_1_noport_str,
                         sockaddr_toString((const struct sockaddr *)sockaddr_IPv4_1,
                                           ipaddr,
                                           SOCKADDR_MAX_STRLEN,
                                           false),
                         SOCKADDR_MAX_STRLEN) == 0 ,
                 "sockaddr toString failed ('%s'", ipaddr);


    fail_unless( strncmp(ipv4_any_str,
                         sockaddr_toString((const struct sockaddr *)sockaddr_IPv4_any_2,
                                           ipaddr,
                                           SOCKADDR_MAX_STRLEN,
                                           true),
                         SOCKADDR_MAX_STRLEN ) == 0 ,
                 "sockaddr toString failed ('%s'", ipaddr);

    fail_if( strncmp(ipv4_any_str,
                     sockaddr_toString((const struct sockaddr *)sockaddr_IPv4_any_2,
                                       ipaddr_small,
                                       10,
                                       true),
                     SOCKADDR_MAX_STRLEN ) == 0 ,
             "sockaddr toString failed ('%s'", ipaddr);


    ss.ss_family = 12;

    fail_unless( NULL ==  sockaddr_toString((const struct sockaddr *)&ss,
                                            ipaddr,
                                            SOCKADDR_MAX_STRLEN,
                                            true) );


}
END_TEST



START_TEST (sockaddr_IPv6_toString)
{

    char ipaddr[SOCKADDR_MAX_STRLEN];

    fail_unless( strncmp(ipv6_1_str,
                         sockaddr_toString((const struct sockaddr *)sockaddr_IPv6_1,
                                           ipaddr,
                                           SOCKADDR_MAX_STRLEN,
                                           true),
                         SOCKADDR_MAX_STRLEN) == 0 ,
                 "sockaddr toString failed ('%s'", ipaddr);

    fail_unless( strncmp(ipv6_1_noport_str,
                         sockaddr_toString((const struct sockaddr *)sockaddr_IPv6_1,
                                           ipaddr,
                                           SOCKADDR_MAX_STRLEN,
                                           false),
                         SOCKADDR_MAX_STRLEN) == 0 ,
                 "sockaddr toString failed ('%s'", ipaddr);


}
END_TEST

START_TEST (sockaddr_IPv4_copy)
{
    struct sockaddr_storage copy;

    sockaddr_copy((struct sockaddr *)&copy,
                  (const struct sockaddr *)sockaddr_IPv4_1);

    fail_unless( sockaddr_alike((struct sockaddr *)&copy,
                                (struct sockaddr *)sockaddr_IPv4_1),
                 "copy failed");

    fail_if( sockaddr_alike((struct sockaddr *)&copy,
                            (struct sockaddr *)sockaddr_IPv4_2),
                 "copy failed");


}
END_TEST


START_TEST (sockaddr_IPv6_copy)
{
    struct sockaddr_storage copy;

    sockaddr_copy((struct sockaddr *)&copy,
                  (const struct sockaddr *)sockaddr_IPv6_1);

    fail_unless( sockaddr_alike((struct sockaddr *)&copy,
                                (struct sockaddr *)sockaddr_IPv6_1),
                 "copy failed");

    fail_if( sockaddr_alike((struct sockaddr *)&copy,
                            (struct sockaddr *)sockaddr_IPv6_2),
                 "copy failed");



}
END_TEST


START_TEST (test_initFromString)
{
    struct sockaddr_storage ip;
    
    fail_if( sockaddr_initFromString( (struct sockaddr *)&ip, "" ));

    fail_if( sockaddr_initFromString( (struct sockaddr *)&ip, "1.2.3.4.5" ));




}
END_TEST

START_TEST (sockaddr_IPv4_setPort)
{
    struct sockaddr_storage ip;
    sockaddr_initFromString( (struct sockaddr *)&ip, ipv4_1_noport_str );

    sockaddr_setPort( (struct sockaddr *)&ip, 4567 );

    fail_unless( sockaddr_alike((struct sockaddr *)&ip,
                                (struct sockaddr *)sockaddr_IPv4_1),
                 "setPort failed");

    sockaddr_setPort( (struct sockaddr *)&ip, 1111 );

    fail_if( sockaddr_alike((struct sockaddr *)&ip,
                            (struct sockaddr *)sockaddr_IPv4_1),
                 "setPort failed");

    


}
END_TEST



START_TEST (sockaddr_IPv6_setPort)
{
    struct sockaddr_storage ip;
    sockaddr_initFromString( (struct sockaddr *)&ip, ipv6_1_noport_str );

    sockaddr_setPort( (struct sockaddr *)&ip, 3456 );

    fail_unless( sockaddr_alike((struct sockaddr *)&ip,
                                (struct sockaddr *)sockaddr_IPv6_1),
                 "setPort failed");

    sockaddr_setPort( (struct sockaddr *)&ip, 1111 );

    fail_if( sockaddr_alike((struct sockaddr *)&ip,
                            (struct sockaddr *)sockaddr_IPv6_1),
                 "setPort failed");


}
END_TEST


START_TEST (sockaddr_IPv4_loopback)
{
    struct sockaddr_storage ip;
    struct sockaddr_storage addr;
    addr.ss_family = 12;

    sockaddr_initFromString( (struct sockaddr *)&ip, "127.0.0.1" );

    fail_unless( sockaddr_isAddrLoopBack( (struct sockaddr *)&ip ));

    fail_if( sockaddr_isAddrLoopBack(  (struct sockaddr *)&addr ));


}
END_TEST


START_TEST (sockaddr_IPv6_loopback)
{
    struct sockaddr_storage ip;
    sockaddr_initFromString( (struct sockaddr *)&ip, "::1" );

    fail_unless( sockaddr_isAddrLoopBack( (struct sockaddr *)&ip ));



}
END_TEST


START_TEST (sockaddr_IPv6_linklocal)
{
    struct sockaddr_storage ip_link;
    struct sockaddr_storage ip_4;
    struct sockaddr_storage ip_6;
    struct sockaddr_storage ip_wrong;

    sockaddr_initFromString( (struct sockaddr *)&ip_link, "fe80::226:18ff:fe92:6d53" );

    sockaddr_initFromString( (struct sockaddr *)&ip_4, "1.2.3.4" );
    sockaddr_initFromString( (struct sockaddr *)&ip_6, "2001:470:dc88:2:226:18ff:fe92:6d53" );

    fail_unless( sockaddr_isAddrLinkLocal( (struct sockaddr *)&ip_link ));


    fail_if( sockaddr_isAddrLinkLocal( (struct sockaddr *)&ip_4 ));
    
    fail_if( sockaddr_isAddrLinkLocal( (struct sockaddr *)&ip_6 ));


    ip_wrong.ss_family = 12;
    fail_if( sockaddr_isAddrLinkLocal( (struct sockaddr *)&ip_wrong ));
}
END_TEST

START_TEST (sockaddr_IPv6_sitelocal)
{
    struct sockaddr_storage ip_link;
    struct sockaddr_storage ip_4;
    struct sockaddr_storage ip_6;
    struct sockaddr_storage ip_wrong;

    sockaddr_initFromString( (struct sockaddr *)&ip_link, "fec0::836:da94:3910:d502" );

    sockaddr_initFromString( (struct sockaddr *)&ip_4, "1.2.3.4" );
    sockaddr_initFromString( (struct sockaddr *)&ip_6, "2001:470:dc88:2:226:18ff:fe92:6d53" );

    fail_unless( sockaddr_isAddrSiteLocal( (struct sockaddr *)&ip_link ));


    fail_if( sockaddr_isAddrSiteLocal( (struct sockaddr *)&ip_4 ));
    
    fail_if( sockaddr_isAddrSiteLocal( (struct sockaddr *)&ip_6 ));


    ip_wrong.ss_family = 12;
    fail_if( sockaddr_isAddrSiteLocal( (struct sockaddr *)&ip_wrong ));
}
END_TEST

START_TEST (sockaddr_IPv6_ula)
{
    struct sockaddr_storage ip_link;
    struct sockaddr_storage ip_4;
    struct sockaddr_storage ip_6;
    struct sockaddr_storage ip_wrong;

    sockaddr_initFromString( (struct sockaddr *)&ip_link, "fc00::836:da94:3910:d502" );

    sockaddr_initFromString( (struct sockaddr *)&ip_4, "1.2.3.4" );
    sockaddr_initFromString( (struct sockaddr *)&ip_6, "2001:470:dc88:2:226:18ff:fe92:6d53" );

    fail_unless( sockaddr_isAddrULA( (struct sockaddr *)&ip_link ));


    fail_if( sockaddr_isAddrULA( (struct sockaddr *)&ip_4 ));
    
    fail_if( sockaddr_isAddrULA( (struct sockaddr *)&ip_6 ));


    ip_wrong.ss_family = 12;
    fail_if( sockaddr_isAddrULA( (struct sockaddr *)&ip_wrong ));
}
END_TEST



START_TEST (sockaddr_IPv4_ipPort)
{
    fail_unless( sockaddr_ipPort( (struct sockaddr *)sockaddr_IPv4_1 ) == 4567 );
    fail_if( sockaddr_ipPort( (struct sockaddr *)sockaddr_IPv4_1 ) == 1234 );
}
END_TEST

START_TEST (sockaddr_IPv6_ipPort)
{
    fail_unless( sockaddr_ipPort( (struct sockaddr *)sockaddr_IPv6_1 ) == 3456 );
    fail_if( sockaddr_ipPort( (struct sockaddr *)sockaddr_IPv6_1 ) == 1234 );
}
END_TEST

START_TEST (sockaddr_IPv4_private)
{
    /* set up addresses */
    struct sockaddr_storage addr_192, addr_192_l,addr_192_h;
    struct sockaddr_storage addr_10, addr_10_l, addr_10_h;
    struct sockaddr_storage addr_172, addr_172_l, addr_172_h;;
    struct sockaddr_storage addr_public;

    
    sockaddr_initFromString( (struct sockaddr *)&addr_192,  "192.168.2.10:3456");
    sockaddr_initFromString( (struct sockaddr *)&addr_192_l,  "192.168.0.0:3456");
    sockaddr_initFromString( (struct sockaddr *)&addr_192_h,  "192.168.255.255:3456");
    
    sockaddr_initFromString( (struct sockaddr *)&addr_172,  "172.16.31.5:4534");
    sockaddr_initFromString( (struct sockaddr *)&addr_172_l,  "172.16.0.0:4534");
    sockaddr_initFromString( (struct sockaddr *)&addr_172_h,  "172.31.255.255:4534");

    sockaddr_initFromString( (struct sockaddr *)&addr_10,  "10.47.4.5:3459");
    sockaddr_initFromString( (struct sockaddr *)&addr_10_l,  "10.0.0.0:3459");
    sockaddr_initFromString( (struct sockaddr *)&addr_10_h,  "10.255.255.255:3459");

    sockaddr_initFromString( (struct sockaddr *)&addr_public,  "8.8.8.8:4444");
    

    fail_unless( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_192 ) );
    fail_unless( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_192_l ) );
    fail_unless( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_192_h ) );

    fail_unless( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_172 ) );
    fail_unless( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_172_l ) );
    fail_unless( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_172_h ) );
    
    fail_unless( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_10 ) );
    fail_unless( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_10_l ) );
    fail_unless( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_10_h ) );

    fail_if( sockaddr_isAddrPrivate( (struct sockaddr *)&addr_public ) );
    fail_if( sockaddr_isAddrPrivate( (struct sockaddr *)sockaddr_IPv6_1 ) );
    

    //fail_if( sockaddr_ipPort( (struct sockaddr *)sockaddr_IPv6_1 ) == 1234 );
}
END_TEST



Suite * sockaddr_suite (void)
{
  Suite *s = suite_create ("sockaddr");

  {/* Core test case */
      TCase *tc_core = tcase_create ("Core");
      tcase_add_checked_fixture (tc_core, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_core, test_sockaddr_create);
      tcase_add_test (tc_core, test_sockaddr_int_IPv4_init);
      tcase_add_test (tc_core, test_sockaddr_int_IPv6_init);
      tcase_add_test (tc_core, test_initFromString);
      suite_add_tcase (s, tc_core);
  }

  {/* Sameaddr test case */
      TCase *tc_sameaddr = tcase_create ("SameAddr");
      tcase_add_checked_fixture (tc_sameaddr, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_sameaddr, sockaddr_IPv4_sameaddr);
      tcase_add_test (tc_sameaddr, sockaddr_IPv6_sameaddr);
      suite_add_tcase (s, tc_sameaddr);
  }
  {/* Sameport test case */
      TCase *tc_sameport = tcase_create ("SamePort");
      tcase_add_checked_fixture (tc_sameport, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_sameport, sockaddr_IPv4_sameport);
      tcase_add_test (tc_sameport, sockaddr_IPv6_sameport);
      suite_add_tcase (s, tc_sameport);
  }

  {/* Alike test case */
      TCase *tc_alike = tcase_create ("Alike");
      tcase_add_checked_fixture (tc_alike, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_alike, sockaddr_IPv4_alike);
      tcase_add_test (tc_alike, sockaddr_IPv6_alike);
      suite_add_tcase (s, tc_alike);
  }
  {/* isSet test case */
      TCase *tc_isSet = tcase_create ("isSet");
      tcase_add_checked_fixture (tc_isSet, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_isSet, sockaddr_IPv4_isSet);
      tcase_add_test (tc_isSet, sockaddr_IPv6_isSet);
      suite_add_tcase (s, tc_isSet);
  }
  {/* isAny test case */
      TCase *tc_isAny = tcase_create ("isAny");
      tcase_add_checked_fixture (tc_isAny, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_isAny, sockaddr_IPv4_isAny);
      tcase_add_test (tc_isAny, sockaddr_IPv6_isAny);
      suite_add_tcase (s, tc_isAny);
  }
  {/* toString test case */
      TCase *tc_sockaddr_toString = tcase_create ("sockaddr_toString");
      tcase_add_checked_fixture (tc_sockaddr_toString, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_sockaddr_toString, sockaddr_IPv4_toString);
      tcase_add_test (tc_sockaddr_toString, sockaddr_IPv6_toString);
      suite_add_tcase (s, tc_sockaddr_toString);
  }
  {/* copy test case */
      TCase *tc_sockaddr_copy = tcase_create ("sockaddr_copy");
      tcase_add_checked_fixture (tc_sockaddr_copy, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_sockaddr_copy, sockaddr_IPv4_copy);
      tcase_add_test (tc_sockaddr_copy, sockaddr_IPv6_copy);
      suite_add_tcase (s, tc_sockaddr_copy);

  }

  {/* setPort test case */
      TCase *tc_sockaddr_setPort = tcase_create ("sockaddr_setPort");
      tcase_add_checked_fixture (tc_sockaddr_setPort, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_sockaddr_setPort, sockaddr_IPv4_setPort);
      tcase_add_test (tc_sockaddr_setPort, sockaddr_IPv6_setPort);
      suite_add_tcase (s, tc_sockaddr_setPort);
  }

  {/* loopback test case */
      TCase *tc_sockaddr_loopback = tcase_create ("sockaddr_loopback");
      tcase_add_test (tc_sockaddr_loopback, sockaddr_IPv4_loopback);
      tcase_add_test (tc_sockaddr_loopback, sockaddr_IPv6_loopback);
      suite_add_tcase (s, tc_sockaddr_loopback);
  }
  {/* linklocal test case */
      TCase *tc_sockaddr_linklocal = tcase_create ("sockaddr_linklocal");
      tcase_add_test (tc_sockaddr_linklocal, sockaddr_IPv6_linklocal);
      suite_add_tcase (s, tc_sockaddr_linklocal);
  }

  {/* sitelocal test case */
      TCase *tc_sockaddr_sitelocal = tcase_create ("sockaddr_sitelocal");
      tcase_add_test (tc_sockaddr_sitelocal, sockaddr_IPv6_sitelocal);
      suite_add_tcase (s, tc_sockaddr_sitelocal);
  }

  {/* ULA test case */
      TCase *tc_sockaddr_ula = tcase_create ("sockaddr_ula");
      tcase_add_test (tc_sockaddr_ula, sockaddr_IPv6_ula);
      suite_add_tcase (s, tc_sockaddr_ula);
  }


  {/* ipPort test case */
      TCase *tc_sockaddr_ipPort = tcase_create ("sockaddr_ipPort");
      tcase_add_checked_fixture (tc_sockaddr_ipPort, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_sockaddr_ipPort, sockaddr_IPv4_ipPort);
      tcase_add_test (tc_sockaddr_ipPort, sockaddr_IPv6_ipPort);
      suite_add_tcase (s, tc_sockaddr_ipPort);
  }

    {/* private addr test case */
      TCase *tc_sockaddr_private = tcase_create ("sockaddr_private");
      tcase_add_checked_fixture (tc_sockaddr_private, sockaddr_setup, sockaddr_teardown);
      tcase_add_test (tc_sockaddr_private, sockaddr_IPv4_private);
      //tcase_add_test (tc_sockaddr_private, sockaddr_IPv6_ipPort);
      suite_add_tcase (s, tc_sockaddr_private);
  }
    

  return s;
}
