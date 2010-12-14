
#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "stunlib.h"

static unsigned char req[] =    
    "\x00\x01\x00\x58"  //   Request type and message length
    "\x21\x12\xa4\x42"  //   Magic cookie
    "\xb7\xe7\xa7\x01"  //}
    "\xbc\x34\xd6\x86"  //}  Transaction ID
    "\xfa\x87\xdf\xae"  //}
    "\x80\x22\x00\x10"  //     SOFTWARE attribute header
    "\x53\x54\x55\x4e"  //}
    "\x20\x74\x65\x73"  //}  User-agent...
    "\x74\x20\x63\x6c"  //}  ...name
    "\x69\x65\x6e\x74"  //}
    "\x00\x24\x00\x04"  // PRIORITY attribute header
    "\x6e\x00\x01\xff"  //    ICE priority value
    "\x80\x29\x00\x08"  //     ICE-CONTROLLED attribute header
    "\x93\x2f\xf9\xb1"  //  }  Pseudo-random tie breaker...
    "\x51\x26\x3b\x36"  //  }   ...for ICE control
    "\x00\x06\x00\x09"  //     USERNAME attribute header
    "\x65\x76\x74\x6a"  //  }
    "\x3a\x68\x36\x76"  //  }  Username (9 bytes) and padding (3 bytes)
    "\x59\x20\x20\x20"  //  }
    "\x00\x08\x00\x14"  //   MESSAGE-INTEGRITY attribute header
    "\x9a\xea\xa7\x0c"  //}
    "\xbf\xd8\xcb\x56"  //}
    "\x78\x1e\xf2\xb5"  //}  HMAC-SHA1 fingerprint
    "\xb2\xd3\xf2\x49"  //}
    "\xc1\xb5\x71\xa2"  //}
    "\x80\x28\x00\x04"  //   FINGERPRINT attribute header
    "\xe5\x7a\x3b\xcf"; //  CRC32 fingerprint

     

static unsigned char respv4[] =
    "\x01\x01\x00\x3c"  //     Response type and message length
    "\x21\x12\xa4\x42"  //     Magic cookie
    "\xb7\xe7\xa7\x01"  //  }
    "\xbc\x34\xd6\x86"  //  }  Transaction ID
    "\xfa\x87\xdf\xae"  //  }
    "\x80\x22\x00\x0b"  //     SOFTWARE attribute header
    "\x74\x65\x73\x74"  //  }
    "\x20\x76\x65\x63"  //  }  UTF-8 server name
    "\x74\x6f\x72\x20"  //  }
    "\x00\x20\x00\x08"  //     XOR-MAPPED-ADDRESS attribute header
    "\x00\x01\xa1\x47"  //     Address family (IPv4) and xor'd mapped port number
    "\xe1\x12\xa6\x43"  //     Xor'd mapped IPv4 address
    "\x00\x08\x00\x14"  //     MESSAGE-INTEGRITY attribute header
    "\x2b\x91\xf5\x99"  //  }
    "\xfd\x9e\x90\xc3"  //  }
    "\x8c\x74\x89\xf9"  //  }  HMAC-SHA1 fingerprint
    "\x2a\xf9\xba\x53"  //  }
    "\xf0\x6b\xe7\xd7"  //  }
    "\x80\x28\x00\x04"  //     FINGERPRINT attribute header
    "\xc0\x7d\x4c\x96"; //     CRC32 fingerprint


static unsigned char respv6[] =
    "\x01\x01\x00\x48"  //     Response type and message length
    "\x21\x12\xa4\x42"  //     Magic cookie
    "\xb7\xe7\xa7\x01"  //  }
    "\xbc\x34\xd6\x86"  //  }  Transaction ID
    "\xfa\x87\xdf\xae"  //  }
    "\x80\x22\x00\x0b"  //     SOFTWARE attribute header
    "\x74\x65\x73\x74"  //  }
    "\x20\x76\x65\x63"  //  }  UTF-8 server name
    "\x74\x6f\x72\x20"  //  }
    "\x00\x20\x00\x14"  //     XOR-MAPPED-ADDRESS attribute header
    "\x00\x02\xa1\x47"  //     Address family (IPv6) and xor'd mapped port number
    "\x01\x13\xa9\xfa"  //  }
    "\xa5\xd3\xf1\x79"  //  }  Xor'd mapped IPv6 address
    "\xbc\x25\xf4\xb5"  //  }
    "\xbe\xd2\xb9\xd9"  //  }
    "\x00\x08\x00\x14"  //     MESSAGE-INTEGRITY attribute header
    "\xa3\x82\x95\x4e"  //  }
    "\x4b\xe6\x7b\xf1"  //  }
    "\x17\x84\xc9\x7c"  //  }  HMAC-SHA1 fingerprint
    "\x82\x92\xc2\x75"  //  }
    "\xbf\xe3\xed\x41"  //  }
    "\x80\x28\x00\x04"  //     FINGERPRINT attribute header
    "\xc8\xfb\x0b\x4c"; //     CRC32 fingerprint



                                
static const char username[] = "evtj:h6vY";
static char password[] = "VOkJxbRl1RmTxUk/WvJxBt";
static const unsigned char idOctet[] = "\xb7\xe7\xa7\x01"  
    "\xbc\x34\xd6\x86"
    "\xfa\x87\xdf\xae";

static const uint32_t priority = 1845494271;
    
static const uint64_t tieBreaker = 0x932FF9B151263B36LL;
static const uint32_t xorMapped = 3221225985U; /*192.0.2.1*/
static const uint16_t port = 32853;

static uint32_t xorMapped_v6[] = {536939960, 305419896, 1122867, 1146447479};

const char *software= "STUN test client\0";
const char *software_resp= "test vector\0";











Suite * stunlib_suite (void);






START_TEST (request_decode)
{
    StunMessage stunMsg;
    
    fail_unless( stunlib_DecodeMessage( req,
                                        108,
                                        &stunMsg,
                                        NULL,
                                        password,
                                        false,
                                        false ));
     
    fail_unless( stunMsg.msgHdr.msgType == STUN_MSG_BindRequestMsg );
    
    fail_unless ( 0 == memcmp(&stunMsg.msgHdr.id.octet,&idOctet,12));

    fail_unless( stunMsg.hasUsername );
    fail_unless( 0 == memcmp(&stunMsg.username.value, username, stunMsg.username.sizeValue ));

    fail_unless( stunMsg.hasSoftware );
    fail_unless( 0 == memcmp(&stunMsg.software.value, software, stunMsg.software.sizeValue ));
        
    fail_unless( stunMsg.hasPriority );
    fail_unless( stunMsg.priority.value == priority );
           
    fail_unless( stunMsg.hasControlled);
    fail_unless( stunMsg.controlled.value == tieBreaker );
       
}
END_TEST

START_TEST (request_encode)
{
    StunMessage stunMsg;
        
    unsigned char stunBuf[120];
        

    memset(&stunMsg, 0, sizeof(StunMessage));
    

    stunMsg.msgHdr.msgType = STUN_MSG_BindRequestMsg;
    memcpy(&stunMsg.msgHdr.id.octet,&idOctet,12);

    fail_unless( stunlib_addUserName(&stunMsg, username, '\x20') );
    
    fail_unless( stunlib_addSoftware(&stunMsg, software, '\x20') );




    stunMsg.hasPriority = true;
    stunMsg.priority.value = priority;

    stunMsg.hasControlled = true;
    stunMsg.controlled.value = tieBreaker;

    
    

    fail_unless( stunlib_encodeMessage(&stunMsg,
                                       stunBuf,
                                       120,
                                       (unsigned char*)password,
                                       strlen(password),
                                       false, /*verbose */
                                       false)  /* msice2 */);

    
    fail_unless( memcmp(stunBuf, req, 108 ) == 0 );

}
END_TEST


START_TEST (response_decode)
{
    StunMessage stunMsg;
    
    fail_unless( stunlib_DecodeMessage( respv4,
                                        80,
                                        &stunMsg,
                                        NULL,
                                        password,
                                        false,
                                        false ));
    
    fail_unless ( 0 == memcmp(&stunMsg.msgHdr.id.octet,&idOctet,12));
        
    fail_unless( stunMsg.msgHdr.msgType == STUN_MSG_BindResponseMsg );
    
    fail_unless( stunMsg.hasXorMappedAddress );
    fail_unless( stunMsg.xorMappedAddress.addr.v4.addr == xorMapped );
    fail_unless( stunMsg.xorMappedAddress.addr.v4.port == port );
            
    fail_unless( stunMsg.hasSoftware);
    fail_unless( strncmp( stunMsg.software.value, 
                          software_resp, 
                          max(stunMsg.software.sizeValue,sizeof(software)) )==0 );

}
END_TEST


START_TEST (response_encode)
{
    StunMessage stunMsg;
    
    unsigned char stunBuf[120];
        
    memset(&stunMsg, 0, sizeof(StunMessage));
    stunMsg.msgHdr.msgType = STUN_MSG_BindResponseMsg;
    
    
    /*id*/
    memcpy(&stunMsg.msgHdr.id.octet,&idOctet,12);

    /*Server*/
    stunMsg.hasSoftware = true;
    memcpy( stunMsg.software.value, software, strlen(software_resp));
    
    stunMsg.software.sizeValue = strlen(software_resp);
    

    /*Mapped Address*/
    stunMsg.hasXorMappedAddress = true;        
    stunMsg.xorMappedAddress.familyType = STUN_ADDR_IPv4Family;
    stunMsg.xorMappedAddress.addr.v4.addr = xorMapped;
    stunMsg.xorMappedAddress.addr.v4.port = port;


    fail_unless( stunlib_addSoftware(&stunMsg, software_resp, '\x20') );
    
    
        
    fail_unless( stunlib_encodeMessage(&stunMsg,
                                       stunBuf,
                                       80,
                                       (unsigned char*)password,
                                       strlen(password),
                                       false, /*verbose */
                                       false)  /* msice2 */ );
                          

    fail_unless( memcmp(stunBuf, respv4, 80)==0 );

}
END_TEST


START_TEST (response_decode_IPv6)
{
    StunMessage stunMsg;
    
    fail_unless( stunlib_DecodeMessage( respv6,
                                        96,
                                        &stunMsg,
                                        NULL,
                                        password,
                                        false,
                                        false ));

    fail_unless ( 0 == memcmp(&stunMsg.msgHdr.id.octet,&idOctet,12));
        
    fail_unless( stunMsg.msgHdr.msgType == STUN_MSG_BindResponseMsg );
    
    fail_unless( stunMsg.hasXorMappedAddress );
    fail_unless( memcmp(&stunMsg.xorMappedAddress.addr.v6.addr, &xorMapped_v6, 4) == 0 );
    fail_unless( stunMsg.xorMappedAddress.addr.v6.port == port );
            
    fail_unless( stunMsg.hasSoftware);
    fail_unless( strncmp( stunMsg.software.value, 
                          software_resp, 
                          max(stunMsg.software.sizeValue,sizeof(software)) )==0 );
}
END_TEST


START_TEST (response_encode_IPv6)
{
    StunMessage stunMsg;
    
    unsigned char stunBuf[120];
        
    memset(&stunMsg, 0, sizeof(StunMessage));
    stunMsg.msgHdr.msgType = STUN_MSG_BindResponseMsg;
    
    
    /*id*/
    memcpy(&stunMsg.msgHdr.id.octet,&idOctet,12);

    /*Server*/
    stunMsg.hasSoftware = true;
    memcpy( stunMsg.software.value, software, strlen(software_resp));
    
    stunMsg.software.sizeValue = strlen(software_resp);
    

    /*Mapped Address*/
    stunMsg.hasXorMappedAddress = true;
    stunlib_setIP6Address(&stunMsg.xorMappedAddress, xorMapped_v6, port);

    
    fail_unless( stunlib_addSoftware(&stunMsg, software_resp, '\x20') );
    
    
        
    fail_unless( stunlib_encodeMessage(&stunMsg,
                                       stunBuf,
                                       96,
                                       (unsigned char*)password,
                                       strlen(password),
                                       false, /*verbose */
                                       false)  /* msice2 */ );
                          

    fail_unless( memcmp(stunBuf, respv6, 92) == 0 );

}
END_TEST


Suite * stunlib_suite (void)
{
  Suite *s = suite_create ("STUNlib");

  
  {/* Test Vector */
      
      TCase *tc_vector = tcase_create ("Test Vector (RFC 5769");
      tcase_add_test (tc_vector, request_decode);
      tcase_add_test (tc_vector, request_encode);
      tcase_add_test (tc_vector, response_decode);
      tcase_add_test (tc_vector, response_encode);
      tcase_add_test (tc_vector, response_decode_IPv6);
      tcase_add_test (tc_vector, response_encode_IPv6);
      suite_add_tcase (s, tc_vector);
      
  }

  return s;
}



int main(void){
    
    int number_failed;
    Suite *s = stunlib_suite ();
    SRunner *sr = srunner_create (s);
    //srunner_add_suite (sr, sockaddr_suite ());
    srunner_run_all (sr, CK_NORMAL);
    number_failed = srunner_ntests_failed (sr);
    srunner_free (sr);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
