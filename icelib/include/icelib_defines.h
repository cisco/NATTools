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

#ifndef ICELIB_DEFINE_H
#define ICELIB_DEFINE_H

#define ICELIB_RANDOM_SEGMENT_LENGTH    (32/6)

#define ICE_MAX_DEBUG_STRING      200

#define ICELIB_UFRAG_LENGTH       (4+1)     /* includes zero ('\0') termination */
#define ICELIB_PASSWD_LENGTH      (22+1)    /* includes zero ('\0') termination */
#define ICELIB_FOUNDATION_LENGTH  (1+1)     /* includes zero ('\0') termination */

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
/*Warning. No equal values are allowed. (Used to ensure that RTP and RTCP has the same path)*/
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
