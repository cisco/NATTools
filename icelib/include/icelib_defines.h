#ifndef ICELIB_DEFINE_H
#define ICELIB_DEFINE_H

#define ICELIB_RANDOM_SEGMENT_LENGTH    (32/6)

#define ICE_MAX_DEBUG_STRING      200

#define ICELIB_UFRAG_LENGTH       (4+1)     // includes zero ('\0') termination
#define ICELIB_PASSWD_LENGTH      (22+1)    // includes zero ('\0') termination
#define ICELIB_FOUNDATION_LENGTH  (1+1)     // includes zero ('\0') termination

#define ICE_MAX_UFRAG_PAIR_LENGTH       ( ( ICE_MAX_UFRAG_LENGTH      * 2) + 1)
#define ICE_MAX_FOUNDATION_PAIR_LENGTH  ( ( ICE_MAX_FOUNDATION_LENGTH * 2))

#define ICELIB_MAX_PAIRS          40
#define ICELIB_MAX_FIFO_ELEMENTS  40
#define ICELIB_MAX_COMPONENTS     5

#define ICELIB_LOCAL_TYPEPREF   126
#define ICELIB_PEERREF_TYPEREF  110
#define ICELIB_REFLEX_TYPEREF   100
#define ICELIB_RELAY_TYPEREF    0



#define ICELIB_COMPLETE_WEIGHT              150
//Warning. No equal values are allowed. (Used to ensure that RTP and RTCP has the same path)
#define ICELIB_HOST_WEIGHT                   50
#define ICELIB_SRFLX_WEIGHT                  25
#define ICELIB_PRFLX_WEIGHT                  20
#define ICELIB_RELAY_WEIGHT                  10
#define ICELIB_TIME_MULTIPLIER_INCREASE_MS  250

#define ICELIB_RTP_COMPONENT_ID 1
#define ICELIB_RTCP_COMPONENT_ID 2

#define ICELIB_RTP_COMPONENT_INDEX 0
#define ICELIB_RTCP_COMPONENT_INDEX 1

#define ICELIB_FAIL_AFTER_MS                5000 /*5 sec*/


#endif
