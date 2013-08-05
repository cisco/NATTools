/*
Copyright 2011 Cisco. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY CISCO ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of Cisco.
*/
#ifndef STUNMSG_H
#define STUNMSG_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#ifndef WIN32
#include <netinet/in.h>
#else
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <ws2tcpip.h>
#endif

#ifndef socklen_t
#define socklen_t int
#endif


#ifdef __cplusplus
extern "C" {
#endif
#if 0
} /*Emacs indent hack */
#endif

/*
 * Avoid disrupting std::min() and std::max()
 * when compiling .cpp files that include this
*/
#if !defined(max)&&!defined( __cplusplus)
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min)&&!defined( __cplusplus)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif


/*** STUN message classes ***/
#define STUN_CLASS_MASK                            0x0110  /* Bits C0,C1 */
#define STUN_CLASS_REQUEST                         0x0000
#define STUN_CLASS_INDICATION                      0x0010
#define STUN_CLASS_SUCCESS_RESP                    0x0100
#define STUN_CLASS_ERROR_RESP                      0x0110

/*** STUN message methods, including class ***/
#define STUN_MSG_BindRequestMsg                    0x0001
#define STUN_MSG_BindResponseMsg                   0x0101
#define STUN_MSG_BindErrorResponseMsg              0x0111
#define STUN_MSG_BindIndicationMsg                 0x0011

/*** STUN message methods (TURN extensions), including class ***/
#define STUN_MSG_AllocateRequestMsg                0x0003
#define STUN_MSG_AllocateResponseMsg               0x0103
#define STUN_MSG_AllocateErrorResponseMsg          0x0113

#define STUN_MSG_RefreshRequestMsg                 0x0004
#define STUN_MSG_RefreshResponseMsg                0x0104
#define STUN_MSG_RefreshErrorResponseMsg           0x0114

#define STUN_MSG_CreatePermissionRequestMsg        0x0008
#define STUN_MSG_CreatePermissionResponseMsg       0x0108
#define STUN_MSG_CreatePermissionErrorResponseMsg  0x0118

#define STUN_MSG_ChannelBindRequestMsg             0x0009
#define STUN_MSG_ChannelBindResponseMsg            0x0109
#define STUN_MSG_ChannelBindErrorResponseMsg       0x0119

#define STUN_MSG_SendIndicationMsg                 0x0016
#define STUN_MSG_DataIndicationMsg                 0x0017

/*** STUN message methods MS-ICE2 specific, including class ***/
#define STUN_MSG_MS2_SendRequestMsg                0x004  /* equiv to STUN_MSG_SendIndication */
#define STUN_MSG_MS2_DataIndicationMsg             0x115  /* equiv to STUN_MSG_DataIndicationMsg */
#define STUN_MSG_MS2_SetActiveDestReqMsg           0x006
#define STUN_MSG_MS2_SetActiveDestResponseMsg      0x106
#define STUN_MSG_MS2_SetActiveDestErrorResponseMsg 0x116

/*** STUN attributes ***/
#define STUN_ATTR_MappedAddress      0x0001  /* MSICE2 equiv of STUN_ATTR_XorRelayAddress */
#define STUN_ATTR_XorMappedAddress   0x0020  /* =reflexive */
#define STUN_ATTR_Username           0x0006
#define STUN_ATTR_MessageIntegrity   0x0008
#define STUN_ATTR_FingerPrint        0x8028
#define STUN_ATTR_ErrorCode          0x0009
#define STUN_ATTR_Realm              0x0014
#define STUN_ATTR_Nonce              0x0015
#define STUN_ATTR_UnknownAttribute   0x000A
#define STUN_ATTR_Software           0x8022
#define STUN_ATTR_AlternateServer    0x8023

/* STUN attributes (TURN extensions) */
#define STUN_ATTR_ChannelNumber       0x000c
#define STUN_ATTR_Lifetime            0x000d
#define STUN_ATTR_XorPeerAddress      0x0012
#define STUN_ATTR_Data                0x0013
#define STUN_ATTR_XorRelayAddress     0x0016   /* relay address */
#define STUN_ATTR_RequestedAddrFamily 0x0017
#define STUN_ATTR_EvenPort            0x0018
#define STUN_ATTR_RequestedTransport  0x0019
#define STUN_ATTR_DontFragment        0x001a
#define STUN_ATTR_ReservationToken    0x0022

/* STUN attributes (ICE extensions) */
#define STUN_ATTR_Priority           0x0024
#define STUN_ATTR_UseCandidate       0x0025
#define STUN_ATTR_ICEControlled      0x8029
#define STUN_ATTR_ICEControlling     0x802A

/* STUN attributes (MS-ICE2) */
#define STUN_ATTR_MS2_AlternateServer  0x000E  /* equiv to STUN_ATTR_AlternateServer */
#define STUN_ATTR_MS2_MagicCookie      0x000F  /* no equiv */
#define STUN_ATTR_MS2_Bandwidth        0x0010  /* no equiv */
#define STUN_ATTR_MS2_DestAddr         0x0011  /* equiv to STUN_ATTR_XorPeerAddress  */
#define STUN_ATTR_MS2_RemoteAddr       0x0012  /* equiv to STUN_ATTR_XorPeerAddress ?? */
#define STUN_ATTR_MS2_MsVersion        0x8008  /* no equiv */
#define STUN_ATTR_MS2_XorMappedAddress 0x8020  /* equiv to STUN_ATTR_XorMappedAddress = Reflexive */
#define STUN_ATTR_MS2_SequenceNumber   0x8050  /* no equiv */
#define STUN_ATTR_MS2_CandidateId      0x8054  /* no equiv */
#define STUN_ATTR_MS2_ServiceQuality   0x8055  /* no equiv */

/* MSICE2 coding of Service Quality Attribute  */
#define STUN_MS2_ServiceQuality_BestEffort  0
#define STUN_MS2_Servicequality_Reliable    1
#define STUN_MS2_StreamType_Audio           1
#define STUN_MS2_StreamType_Video           2
#define STUN_MS2_StreamType_Duo             3
#define STUN_MS2_StreamType_Data            4

/* MSVERSION attribute */
#define STUN_MS2_VERSION                    2


/** IP Addr family **/
#define STUN_ADDR_IPv4Family         0x01
#define STUN_ADDR_IPv6Family         0x02


/*** STUN Error attribute codes ***/

/* STUN RFC5389 */
#define STUN_ERROR_TRY_ALTERNATE             300
#define STUN_ERROR_BAD_REQUEST               400
#define STUN_ERROR_UNAUTHORIZED              401
#define STUN_ERROR_UNKNOWN_ATTR              420
#define STUN_ERROR_STALE_NONCE               438

#define STUN_ERROR_SERVER_ERROR              500

/* TURN extensions */
#define STUN_ERROR_NO_BINDING                437
#define STUN_ERROR_ADDR_FAMILY_NOT_SUPPORTED 440
#define STUN_ERROR_WRONG_USERNAME            441
#define STUN_ERROR_UNSUPPORTED_PROTO         442
#define STUN_ERROR_PEER_ADDR_MISMATCH        443
#define STUN_ERROR_QUOTA_REACHED             486
#define STUN_ERROR_INSUFFICIENT_CAPACITY     508

/* ICE extensions */
#define STUN_ERROR_ROLE_CONFLICT             487

/* (old) STUN RFC3489 */
#define STUN_ERROR_STALE_CREDS               430
#define STUN_ERROR_INTEG_CHECK_FAIL          431
#define STUN_ERROR_MISSING_USERNAME          432
#define STUN_ERROR_GLOBAL_FAIL               600


/*** STUN decode helpers ***/

#define STUN_MSG_ID_SIZE                 12

#define STUN_MSG_MAX_REALM_LENGTH        128
#define STUN_MSG_MAX_NONCE_LENGTH        128
#define STUN_MSG_MAX_USERNAME_LENGTH     81
#define STUN_MSG_MAX_PASSWORD_LENGTH     61
#define STUN_MIN_CHANNEL_ID              0x4000
#define STUN_MAX_CHANNEL_ID              0x7FFF

#define IPV4_ADDR_LEN                    16
#define IPV4_ADDR_LEN_WITH_PORT          (IPV4_ADDR_LEN+6) /* Need extra space for :port */
#define IPV6_ADDR_LEN                    100

#define STUN_REQ_TRANSPORT_UDP         (uint32_t)17 /* IANA protocol number */

#define STUN_MIN_PACKET_SIZE             20
#define STUN_HEADER_SIZE                 20
#define STUN_MAX_PACKET_SIZE            512
#define STUN_MAX_STRING                 256
#define STUN_MS2_MAX_FOUNDATION_STR      32 /* misice2 */
#define STUN_MAX_UNKNOWN_ATTRIBUTES       8
#define STUN_MAGIC_COOKIE_ARRAY         {0x21, 0x12, 0xA4, 0x42}
#define STUN_MS2_MAGIC_COOKIE_ARRAY     {0x72, 0xC6, 0x4B, 0xC6}
#define STUN_MAGIC_COOKIE_SIZE          4
#define STUN_PACKET_MASK                0xC0
#define STUN_PACKET                     0x00
#define TURN_CHANNEL_PACKET             0x40
#define STUN_STRING_ALLIGNMENT          4
#define STUN_MS2_STRING_ALLIGNMENT      1

#define STUNCLIENT_MAX_RETRANSMITS          9
#define STUNCLIENT_RETRANSMIT_TIMEOUT_LIST      100, 200, 400, 800, 1600, 1600, 1600, 1600, 1600  /* msec */
#define STUNCLIENT_RETRANSMIT_TIMEOUT_LIST_MS2  650, 650, 650, 650,  650,  650,  650,  650,  650
#define STUNCLIENT_DFLT_TICK_TIMER_MS            50

#define STUN_KEEPALIVE_TIMER_SEC    15

#define TURN_SEND_IND_HDR_SIZE      36  /* fixed overhead when using  TURN send indication  */
#define TURN_SEND_REQ_HDR_SIZE_MS2  52  /* min. overhead when using  Microsoft TURN send Request, excl. integrity  */
                                        /* hdr(20)+Cookie(8)+Vers(8)+Data(4) */
#define TURN_SEQ_LEN_MS2            28  /* size of MS Sequence Number attribute */
#define TURN_INTEG_LEN              24  /* size of integrity attribute */
#define TURN_CHANNEL_DATA_HDR_SIZE   4  /* overhead when using  TURN channel data     */

#define STUN_MAX_PEER_ADDR          10 /* max no. of peer addresses supported  (e.g. when encoding  CreatePermission) */

#define STUN_DFLT_PAD ' '             /* default padding char */


typedef enum
{
  StunKeepAliveUsage_Outbound,
  StunKeepAliveUsage_Ice
}
StunKeepAliveUsage;

typedef struct{
    uint8_t octet[STUN_MAGIC_COOKIE_SIZE];
} StunMsgCookie;

typedef struct
{
    uint8_t octet[STUN_MSG_ID_SIZE];
} StunMsgId;

/* Stun message header.
 */
typedef struct
{
    uint16_t      msgType;
    uint16_t      msgLength;
    StunMsgId     id;
    StunMsgCookie cookie;
} StunMsgHdr;


typedef struct
{
    uint16_t type;
    uint16_t length;
} StunAtrHdr;


typedef struct
{
    uint16_t port;
    uint32_t addr;
} StunAddress4;

typedef struct
{
    uint16_t port;
    uint8_t addr[16];
} StunAddress6;

typedef struct
{
    uint8_t familyType;   /* IP 4 or 6, se above for define */
    union
    {
        StunAddress4  v4;
        StunAddress6  v6;
    } addr;
} StunIPAddress;

typedef struct
{
    uint16_t reserved;
    uint8_t errorClass;
    uint8_t number;
    char reason[STUN_MAX_STRING];
    char padChar;
    uint16_t sizeReason;
} StunAtrError;

typedef struct
{
    uint16_t attrType[STUN_MAX_UNKNOWN_ATTRIBUTES];
    uint16_t numAttributes;
} StunAtrUnknown;

typedef struct
{
    char value[STUN_MAX_STRING];
    char padChar;
    uint16_t sizeValue;
} StunAtrString;

typedef struct
{
    uint16_t offset;
    unsigned char hash[20];
} StunAtrIntegrity;


typedef struct
{
    uint32_t value;
} StunAtrValue;

typedef struct
{
    uint64_t value;
} StunAtrDoubleValue;


typedef struct
{
    uint32_t  dataLen;
    uint32_t  offset;
    uint8_t   *pData;
} StunData;

typedef struct
{
    uint16_t  channelNumber;
    uint16_t  rffu;         /* reserved */
}
StunAtrChannelNumber;

typedef struct
{
    uint8_t  protocol;
    uint8_t  rffu[3];       /* reserved */
}
StunAtrRequestedTransport;


typedef struct
{
    //first 4 bytes for type, 
    //rest is for padding (future use)
    uint8_t  family;
    uint8_t  rffu[3];       /* reserved */
}StunAttrRequestedAddrFamily;


typedef struct
{
    uint8_t  evenPort;
    uint8_t  pad[3];
}
StunAtrEvenPort;


/* MS-ICE2 specific  */
typedef struct
{
    uint8_t  connectionId[20];
    uint32_t sequenceNumber;
} StunAttrSequenceNum;

/* MS-ICE2 specific  */
typedef struct
{
    uint16_t streamType;
    uint16_t serviceQuality;
} StunAtrServiceQuality;

/* MS-ICE2 specific  */
typedef  StunAtrString StunAtrCandidateId;


typedef struct
{
    char stunUserName[STUN_MSG_MAX_USERNAME_LENGTH];
    char stunPassword[STUN_MSG_MAX_PASSWORD_LENGTH];
    char realm[STUN_MSG_MAX_REALM_LENGTH];
    char nonce[STUN_MSG_MAX_NONCE_LENGTH];
    unsigned char key[20];   /* for long  term cred: key= md5 hash of  username:realm:pass  */
                             /* for short term cred: key=password  */
} STUN_USER_CREDENTIALS;

/****************************************************************************************************************/
/****************************************** start MALICE specific ***********************************************/
/****************************************************************************************************************/

typedef struct
{
    uint8_t type;
    uint16_t length;
} MaliceIEHdr;

typedef struct
{
    uint8_t DT;
    uint8_t LT;
    uint8_t JT;

    uint32_t minBW;
    uint32_t maxBW;
} MaliceFlowdata;

typedef struct
{
    MaliceFlowdata flowdataUP;
    MaliceFlowdata flowdataDN;
} MaliceFlowdataReq;

typedef struct
{
    bool hasFlowdataReq;
    MaliceFlowdataReq flowdataReq;
} MaliceAttrAgent;

typedef struct
{
    bool hasFlowdataResp;
    MaliceFlowdata flowdataResp;
} MaliceAttrResp;

typedef struct
{
    uint16_t peerMaliceCheckResult;
} MaliceAttrPeerCheck;

typedef struct
{
    bool                    hasMDAgent;
    MaliceAttrAgent         mdAgent;
    bool                    hasMDRespUP;
    MaliceAttrResp          mdRespUP;
    bool                    hasMDRespDN;
    MaliceAttrResp          mdRespDN;
    bool                    hasMDPeerCheck;
    MaliceAttrPeerCheck     mdPeerCheck;
} MaliceMetadata;

/****************************************************************************************************************/
/****************************************** end MALICE specific *************************************************/
/****************************************************************************************************************/

/* Decoded  STUN message */
typedef struct
{
    StunMsgHdr msgHdr;

    bool hasMappedAddress;
    StunIPAddress  mappedAddress;

    bool hasUsername;
    StunAtrString username;

    bool hasIntegirtyKey;
    StunAtrString integrityKey;

    bool hasMessageIntegrity;
    StunAtrIntegrity messageIntegrity;

    bool hasErrorCode;
    StunAtrError errorCode;

    bool hasUnknownAttributes;
    StunAtrUnknown unknownAttributes;

    bool hasXorMappedAddress;
    StunIPAddress  xorMappedAddress;

    bool hasSoftware;
    StunAtrString software;

    bool hasLifetime;
    StunAtrValue   lifetime;

    bool hasAlternateServer;
    StunIPAddress alternateServer;

    uint32_t xorPeerAddrEntries;
    StunIPAddress xorPeerAddress[STUN_MAX_PEER_ADDR];

    bool hasData;
    StunData data;

    bool hasNonce;
    StunAtrString nonce;

    bool hasRealm;
    StunAtrString realm;

    bool hasXorRelayAddress;
    StunIPAddress  xorRelayAddress;

    bool hasRequestedAddrFamily;
    StunAttrRequestedAddrFamily requestedAddrFamily;

    bool hasChannelNumber;
    StunAtrChannelNumber channelNumber;

    bool hasPriority;
    StunAtrValue   priority;

    bool hasControlling;
    StunAtrDoubleValue controlling;

    bool hasControlled;
    StunAtrDoubleValue controlled;

    bool hasRequestedTransport;
    StunAtrRequestedTransport requestedTransport;

    bool hasEvenPort;
    StunAtrEvenPort evenPort;

    bool hasReservationToken;
    StunAtrDoubleValue reservationToken;

    /* No value, only flaged */
    bool hasUseCandidate;
    bool hasDontFragment;

    /******* Start MS-ICE2 specific *****/
    /* only put the attributes which have no turn-12 equivalnts here */

    bool hasMagicCookie;
    uint8_t magicCookie[STUN_MAGIC_COOKIE_SIZE];

    bool hasBandwidth;
    StunAtrValue bandwidth;

    bool hasMsVersion;
    StunAtrValue msVersion;

    bool hasSequenceNumber;
    StunAttrSequenceNum SequenceNum;

    bool hasCandidateId;
    StunAtrCandidateId candidateId;

    bool hasDestinationAddress;
    StunIPAddress  destinationAddress;

    bool hasServiceQuality;
    StunAtrServiceQuality ServiceQuality;

    /******* End MS-ICE2 specific *****/

    /****************************************************************************************************************/
    /****************************************** start MALICE specific *************************************************/
    /****************************************************************************************************************/

    bool hasMaliceMetadata;
    MaliceMetadata maliceMetadata;

    /****************************************************************************************************************/
    /****************************************** end MALICE specific *************************************************/
    /****************************************************************************************************************/
} StunMessage;

/* Defines how a user of stun sends data on e.g. socket */
typedef int (*STUN_SENDFUNC)(int              sockHandle,    /* context - e.g. socket handle */
                             uint8_t         *buffer,        /* ptr to buffer to send */
                             int              bufLen,        /* length of send buffer */
                             struct sockaddr *dstAddr,       /* Optional, if connected to socket */
                             socklen_t       t,             /* length of dstAddr */
                             void            *userCtxData);  /* User context data. Optional */


/* Defines how errors are reported */
typedef void  (*STUN_ERR_FUNC)(const char *fmt, va_list ap);


/**********************
 ***** API Funcs ******
 **********************/

/***********************************************/
/*************  Decode functions ***************/
/***********************************************/

/*!
 * STUNLIB_isStunMsg() - use this function to demux STUN messages from a media stream such as RTP or RTCP
 * \param payload        payload  to check (e.g. as received in RTP stream)
 * \param length         payload length
 * \param isMsStun       sets to TRUE is if this is a Microsoft MS-ICE stun
 * \return               TRUE if message is a STUN message
 */
uint16_t stunlib_isStunMsg(uint8_t *payload, uint16_t length, bool *isMsStun);

/*!
 * stunlib_isTurnChannelData - use this function to demux STUN messages from a media stream such as RTP or RTCP
 * \param payload        payload  to check (e.g. as received in RTP stream)
 * \return               TRUE if message is TURN Channel Data
 */
bool stunlib_isTurnChannelData(uint8_t *payload);


/*!
 * stunDecodeMessage() -  Decode and parse a STUN serialised message
 * \param buf             serialised buffer, network order
 * \param buflen          Length of buffer
 * \param message         Store parsed mesage here
 * \param integrityKey    Integrity key, password used to calculate integrity.
 * \param verbose         Turn on off verbose output (Warning can be very verbose!)
 * \param isMsStun        TRUE=MS-ICE2 stun/turn message, FALSE=standard stun/turn
 * \return                True if parsing OK.
 */
bool stunlib_DecodeMessage(unsigned char* buf,
                           unsigned int bufLen,
                           StunMessage* message,
                           StunAtrUnknown* unknowns,
                           FILE *stream,
                           bool isMsStun);


/*!
 * stunlib_checkIntegrity -  Checks the integrity attribute. Be sure to send in the
                             correct key. simple Simple password for short term. 
                             MD5(user:realm:pass) for long term.
 * \param buf             serialised buffer, network order 
 * \param buflen          Length of buffer
 * \param message         STUN/TURN message to check integrity on.
 * \param integrityKey    Integrity key, password used to calculate integrity.
 */

bool stunlib_checkIntegrity(unsigned char* buf,
                            unsigned int bufLen,
                            StunMessage* message,
                            unsigned char *integrityKey,
                            int integrityKeyLen);

/*!
 * STUNLIB_isRequest() - Test if  decoded stun message is a request
 * \param msg            STUN message to check
 * \return               TRUE if message is a request
 */
bool stunlib_isRequest(StunMessage *msg);

/*!
 * STUNLIB_isSuccessResponse() - Test if decoded stun message is a STUN success response
 * \param msg            decoded STUN message to check
 * \return               TRUE if message is a success response
 */
bool stunlib_isSuccessResponse(StunMessage *msg);

/*!
 * STUNLIB_isErrorResponse() - Test if decoded stun message is a STUN error response
 * \param msg            decoded STUN message to check
 * \return               TRUE if message is an error response
 */
bool stunlib_isErrorResponse(StunMessage *msg);


/*!
 * STUNLIB_isResponse() - Test if decoded stun message is a STUN (success or error) response
 * \param msg            decoded STUN message to check
 * \return               TRUE if message is a response
 */
bool stunlib_isResponse(StunMessage *msg);

/*!
 * STUNLIB_isIndication() - Test if decoded stun message is a STUN indication
 * \param msg            STUN message to check
 * \return               TRUE if message is an indication
 */
bool stunlib_isIndication(StunMessage *msg);


uint16_t
stunlib_StunMsgLen (uint8_t *payload);

unsigned int stunlib_decodeTurnChannelNumber(uint16_t      *channelNumber,
                                             uint16_t      *length,
                                             unsigned char *buf);


/***********************************************/
/*************  Encode functions ***************/
/***********************************************/


/*!
 * stunEncodeMessage() - Encode/serialise a STUN message into buffer from StunMessage struct
 * \param message        STUN message to encode
 * \param buf            Buffer to store encoded STUN message in
 * \param bufLen         Length of encoded buffer
 * \param key            Used to calculate message integrity attribute, key in HMAC-SHA1 alg.
 *                       For Long term credentials = 128 bit MD5 hash of username+nonce+password. (created using STUNMSG_createMD5Key())
 *                       For short term credantials = password
 * \param keyLen         length of key
 * \param verbose        Verbose
 * \param isMsStun       true=encode in msice2 format, false=standard stun
 * \return               0 if msg encode fails, else the length of the encoded message (including any padding)
 */
unsigned int stunlib_encodeMessage(StunMessage *message,
                               unsigned char   *buf,
                               unsigned int     bufLen,
                               unsigned char   *key,
                               unsigned int     keyLen,
                               FILE            *stream,
                               bool             isMsStun);


/* encode a stun keepalive, return the length or 0 if it fails
 * \param usage     - Ice results in  BindingIndication, Outbound results in BindingRequest
 * \param transId   - ptr to transaction id
 * \param buf       - Buffer to store encoded STUN message in
 * \param bufLen    - Length of encoded buffer
 */
uint32_t stunlib_encodeStunKeepAliveReq(StunKeepAliveUsage usage,
                                        StunMsgId *transId,
                                        uint8_t   *buf,
                                        int        bufLen);


/* encode a stun Outbound Keepalive response, return the length or 0 if it fails
 * \param transId   - ptr to transaction id
 * \param srvrRflxAddr - ptr to server reflexive address
 * \param buf       - Buffer to store encoded STUN message in
 * \param bufLen    - Length of encoded buffer
 */
uint32_t stunlib_encodeStunKeepAliveResp(StunMsgId     *transId,
                                         StunIPAddress *srvrRflxAddr,
                                         uint8_t       *buf,
                                         int            bufLen);


/*
 *  stunlib_encodeTurnChannelNumber() - Encode/serialise channel numbetr and  length fields of Turn Channel Data
 *  \param channelNumber    TURN Channel Number
 *  \param length           Length of application data. (=no. of bytes following ChannelNumber/Length).
 *  \param buf              Destination
 */
unsigned int stunlib_encodeTurnChannelNumber(uint16_t       channelNumber,
                                             uint16_t       length,
                                             unsigned char *buf);



/***********************************************/
/***********      utilities       **************/
/***********************************************/

/* \return random turn channel number in range STUN_MIN_CHANNEL_ID.. STUN_MAX_CHANNEL_ID */
uint16_t stunlib_createRandomTurnChanNum(void);

/*
 * Convert decoded msgType to string
*/
char const *stunlib_getMessageName(uint16_t msgType);

/*!
 * stunGetErrorReason() - Given an errorClass and number, returns standard reason text
 * \param errorClass
 * \param errorNumber
 */
char const *stunlib_getErrorReason(uint16_t errorClass, uint16_t errorNumber);


void stunlib_setIP4Address(StunIPAddress* pIpAdr, uint32_t addr, uint16_t port);
/* Addr is 4 long. With most significant DWORD in pos 0 */
void stunlib_setIP6Address(StunIPAddress *pIpAdr, uint8_t addr[16], uint16_t port);

void stunlib_printBuffer(FILE *stream, uint8_t *pBuf, int len, char const * szHead);


uint32_t crc32(uint32_t crc, const void *buf, uint32_t size);

void     stunlib_createId(StunMsgId *pId, long randval, unsigned char retries);
bool     stunlib_addRealm(StunMessage *stunMsg, const char *realm, char padChar);
bool     stunlib_addRequestedTransport(StunMessage *stunMsg, uint8_t protocol);
bool     stunlib_addRequestedAddrFamily(StunMessage *stunMsg, int sa_family);
bool     stunlib_addUserName(StunMessage *stunMsg, const char *userName, char padChar);
bool     stunlib_addNonce(StunMessage *stunMsg, const char *nonce, char padChar);
bool     stunlib_addSoftware(StunMessage *stunMsg, const char *software, char padChar);
bool     stunlib_addError(StunMessage *stunMsg, const char *reasonStr, uint16_t classAndNumber,  char padChar);
bool     stunlib_addChannelNumber(StunMessage *stunMsg,  uint16_t channelNumber);
uint32_t stunlib_calculateFingerprint(const uint8_t *buf, size_t len);
bool     stunlib_checkFingerPrint(uint8_t *buf, uint32_t fpOffset);
void     stunlib_addMsCookie(StunMessage *stunMsg);
void     stunlib_addMsVersion(StunMessage *stunMsg, uint32_t msVers);
void     stunlib_addMsSeqNum(StunMessage *stunMsg, StunAttrSequenceNum *seqNr);


/* Concat  username+realm+passwd into string "<username>:<realm>:<password>" then run the MD5 alg.
 * on this string to create a 128but MD5 hash in md5key.
 *
 * \param   md5Key    calculated md5key, must be min 16 bytes.
 * \param   userName  C string
 * \param   realm     C string
 * \param   password  C string
*/
void stunlib_createMD5Key(unsigned char *md5key, 
                          const char *userName, 
                          const char *realm, 
                          const char *password);


/* helpers */
//char *stunlib_transactionIdtoStr(char* s, StunMsgId tId);
//void  stunlib_transactionIdDump(FILE *stream, StunMsgId tId);


#if 0
{/*EMacs indent hack */
#endif
#ifdef __cplusplus
}
#endif

#endif /* STUNMSG_H*/
