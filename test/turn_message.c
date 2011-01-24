#include <stdlib.h>
#include <stdio.h>

#include <check.h>

#include "stunlib.h"

Suite * turnmessage_suite (void);


static unsigned char allocate_resp[] = 
    "\x01\x03\x00\x5c" 
    "\x21\x12\xa4\x42" 
    "\x64\x3c\x98\x69" 
    "\x00\x01\x00\x00" 
    "\x07\x5d\xfe\x0c" 
    "\x00\x16\x00\x08" 
    "\x00\x01\xf3\x10" 
    "\x7c\x4f\xc4\x88" 
    "\x00\x0d\x00\x04" 
    "\x00\x00\x02\x58" 
    "\x00\x20\x00\x08" 
    "\x00\x01\x6d\x8a" 
    "\x74\xb4\x2c\xa0" 
    "\x80\x22\x00\x1d" 
    "\x72\x65\x73\x74" 
    "\x75\x6e\x64\x20" 
    "\x76\x30\x2e\x31" 
    "\x2e\x30\x20\x28" 
    "\x78\x38\x36\x5f" 
    "\x36\x34\x2f\x6c" 
    "\x69\x6e\x75\x78" 
    "\x29\x00\x00\x00" 
    "\x00\x08\x00\x14" 
    "\x9d\x89\xb4\x21" 
    "\x26\x5c\xe2\x20" 
    "\xd0\x45\xc1\x2c" 
    "\x98\xbd\xcd\x2f" 
    "\xce\xb4\x8f\x50";

/*
static unsigned char allocate_error_resp[] = 

01 13 00 68 
21 12 a4 42 
32 7b 23 c6 
00 00 00 00 
51 1a 45 a3 
00 09 00 10 
00 00 04 01 
55 6e 61 75 
74 68 6f 72 
69 7a 65 64 
00 14 00 18 
6d 65 64 69 
61 6e 65 74 
77 6f 72 6b 
73 65 72 76 
69 63 65 73 
2e 63 6f 6d 
00 15 00 10 
30 39 61 36 
30 33 36 30 
31 36 33 61 
36 62 63 32 
80 22 00 1d 
72 65 73 74 
75 6e 64 20 
76 30 2e 31 
2e 30 20 28 
78 38 36 5f 
36 34 2f 6c 
69 6e 75 78 
29 00 00 00                                             
*/


static char password[] = "pem:medianetworkservices.com:pem\0";
static const unsigned char idOctet[] = 
    "\x64\x3c\x98\x69" 
    "\x00\x01\x00\x00" 
    "\x07\x5d\xfe\x0c" 
    "\x00\x16";

const char *software_restund= "restund v0.1.0 (x86_64/linux)\0";

START_TEST(encode_integrity)
{
    StunMessage stunMsg, decodeStunMsg;

    unsigned char stunBuf[120];
    struct sockaddr_storage a,b;

    sockaddr_initFromString((struct sockaddr*)&a, 
                            "85.166.136.226:19608");
    

    sockaddr_initFromString((struct sockaddr*)&b, 
                            "93.93.96.202:53762");
    


    memset(&stunMsg, 0, sizeof(StunMessage));


    stunMsg.msgHdr.msgType = STUN_MSG_AllocateResponseMsg;
    memcpy(&stunMsg.msgHdr.id.octet,&idOctet,12);

    /*Relay Address*/
    stunMsg.hasXorRelayAddress = true;
    stunlib_setIP4Address(&stunMsg.xorRelayAddress, 
                          htonl(((struct sockaddr_in *)&b)->sin_addr.s_addr), 
                          htons(((struct sockaddr_in *)&b)->sin_port));

    /*Lifetime*/
    stunMsg.hasLifetime = true;
    stunMsg.lifetime.value = 600;


    /*Mapped Address*/
    stunMsg.hasXorMappedAddress = true;
    stunlib_setIP4Address(&stunMsg.xorMappedAddress, 
                          htonl(((struct sockaddr_in *)&a)->sin_addr.s_addr), 
                          htons(((struct sockaddr_in *)&a)->sin_port));



    
    fail_unless( stunlib_addSoftware(&stunMsg, software_restund, '\x20') );


    fail_unless( stunlib_encodeMessage(&stunMsg,
                                       stunBuf,
                                       120,
                                       (unsigned char*)password,
                                       strlen(password),
                                       false, /*verbose */
                                       false)  /* msice2 */);



    fail_unless( stunlib_DecodeMessage( stunBuf,
                                        120,
                                        &decodeStunMsg,
                                        NULL,
                                        password,
                                        false,
                                        false ));


}
END_TEST

START_TEST(decode_integrity)
{

    StunMessage stunMsg;

    fail_unless( stunlib_DecodeMessage( allocate_resp,
                                        sizeof(allocate_resp),
                                        &stunMsg,
                                        NULL,
                                        password,
                                        false,
                                        false ));

}
END_TEST

Suite * turnmessage_suite (void){

    Suite *s = suite_create ("TURN message");

    {/* INTEGRITY */
        
        TCase *tc_integrity = tcase_create ("Alloctate Sucsess Integrity");
        
        tcase_add_test (tc_integrity, encode_integrity);
        //tcase_add_test (tc_integrity, decode_integrity);
        
        suite_add_tcase (s, tc_integrity);
      
    }
    return s;
}
