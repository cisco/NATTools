
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

     
/*
static unsigned char respv4[] =
     "\x01\x01\x00\x3c"
     "\x21\x12\xa4\x42"
     "\xb7\xe7\xa7\x01\xbc\x34\xd6\x86\xfa\x87\xdf\xae"
     "\x80\x22\x00\x0b"
       "\x74\x65\x73\x74\x20\x76\x65\x63\x74\x6f\x72\x20"
     "\x00\x20\x00\x08"
       "\x00\x01\xa1\x47\xe1\x12\xa6\x43"
     "\x00\x08\x00\x14"
       "\x2b\x91\xf5\x99\xfd\x9e\x90\xc3\x8c\x74\x89\xf9"
       "\x2a\xf9\xba\x53\xf0\x6b\xe7\xd7"
     "\x80\x28\x00\x04"
       "\xc0\x7d\x4c\x96";
*/
                                
static const char username[] = "evtj:h6vY";
static char password[] = "VOkJxbRl1RmTxUk/WvJxBt";
static const unsigned char idOctet[] = "\xb7\xe7\xa7\x01"  
    "\xbc\x34\xd6\x86"
    "\xfa\x87\xdf\xae";

static const uint32_t priority = 1845494271;
    
static const uint64_t tieBreaker = 0x932FF9B151263B36LL;
static const uint32_t xorMapped = 3221225985U; /*192.0.2.1*/
static const uint16_t port = 32853;

const char *software= "STUN test client\0";











Suite * stunlib_suite (void);






START_TEST (request_decode)
{
    StunMessage stunMsg;
    
    fail_unless( stunlib_DecodeMessage( req,
                                        108,
                                        &stunMsg,
                                        NULL,
                                        password,
                                        true,
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
                                       true, /*verbose */
                                       false)  /* msice2 */);

    
    fail_unless( memcmp(stunBuf, req, 108 ) == 0 );

}
END_TEST


Suite * stunlib_suite (void)
{
  Suite *s = suite_create ("STUNlib");

  
  {/* Test Vector */
      
      TCase *tc_vector = tcase_create ("Test Vector");
      tcase_add_test (tc_vector, request_decode);
      tcase_add_test (tc_vector, request_encode);
      //tcase_add_test (tc_core, calculate_priority);
      //tcase_add_test (tc_core, create_foundation);
      //tcase_add_test (tc_core, pairPriority);
      //tcase_add_test (tc_core, ice_timer);
  
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
