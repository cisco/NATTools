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

#include <stdio.h>
#include <string.h>


#include "sockaddr_util.h"
#include "icelibtypes.h"


char const * ICELIBTYPES_ICE_CANDIDATE_TYPE_toString(const ICE_CANDIDATE_TYPE candidateType){
    switch(candidateType){
    case ICE_CAND_TYPE_NONE:
        return "NONE";
    case ICE_CAND_TYPE_HOST:
        return "HOST";
    case ICE_CAND_TYPE_SRFLX:
        return "SRFLX";
    case ICE_CAND_TYPE_RELAY:
        return "RELAY";
    case ICE_CAND_TYPE_PRFLX:
        return "PRFLX";
    }
    return "UNKNOWN";
}

void ICELIBTYPES_ICE_CANDIDATE_dump(FILE *stream, const ICE_CANDIDATE *candidate){
    char addr[SOCKADDR_MAX_STRLEN];
    fprintf(stream, "   Fnd: '%s' ", candidate->foundation);
    fprintf(stream, "Comp: %i ", candidate->componentid);
    fprintf(stream, "Pri: %u ", candidate->priority);
    fprintf(stream, "Addr: %s", sockaddr_toString((const struct sockaddr *)&candidate->connectionAddr, 
                                         addr, sizeof addr, true));
           //netaddr_dump(&candidate->connectionAddr, true);
    fprintf(stream, " Type: '%s' ", ICELIBTYPES_ICE_CANDIDATE_TYPE_toString(candidate->type));
    fprintf(stream, " UVal1: %u ", candidate->userValue1);
    fprintf(stream, " UVal2: %u\n", candidate->userValue2);
}

void ICELIBTYPES_ICE_MEDIA_STREAM_dump(FILE *stream, const ICE_MEDIA_STREAM *iceMediaStream)
{
    uint32_t i;
    fprintf(stream, " Number of Candidates: %i\n", iceMediaStream->numberOfCandidates);
    if(iceMediaStream->numberOfCandidates > 0){
        fprintf(stream, " Ufrag : '%s'\n", iceMediaStream->ufrag);
        fprintf(stream, " Passwd: '%s'\n", iceMediaStream->passwd);
    }
    for(i=0; i<iceMediaStream->numberOfCandidates; i++) {
        fprintf(stream, " Candidate[%i]\n", i);
        ICELIBTYPES_ICE_CANDIDATE_dump(stream, &iceMediaStream->candidate[i]);
    }

}


void ICELIBTYPES_ICE_MEDIA_dump(FILE *stream, const ICE_MEDIA *iceMedia)
{
    uint32_t i;
    uint32_t printed =0;

    fprintf(stream, "Number of Media Lines: %i\n",iceMedia->numberOfICEMediaLines);

    for (i=0; i< ICE_MAX_MEDIALINES; i++) {
        if ( ICELIBTYPES_ICE_MEDIA_STREAM_isEmpty( &iceMedia->mediaStream[i] ) == false ){
            if ( printed <= iceMedia->numberOfICEMediaLines) {
                fprintf(stream, "---  ICEMediaLine[%i] ---\n", i);
                ICELIBTYPES_ICE_MEDIA_STREAM_dump(stream, &iceMedia->mediaStream[i]);
                printed++;
            }
        }
        else {
            fprintf(stream, "---  ICEMediaLine[%i] ---\n", i);
            fprintf(stream, "[empty]\n");
        }
    }
}

bool ICELIBTYPES_ICE_MEDIA_STREAM_isEmpty(const ICE_MEDIA_STREAM *iceMediaStream)
{
    if (iceMediaStream->numberOfCandidates > 0) {
        return false;
    }
    return true;
}

bool ICELIBTYPES_ICE_MEDIA_isEmpty(const ICE_MEDIA *iceMedia)
{
    if (iceMedia->numberOfICEMediaLines > 0) {
        return false;
    }
    return true;
}

void ICELIBTYPES_ICE_MEDIA_STREAM_reset(ICE_MEDIA_STREAM *iceMediaStream)
{
    memset(iceMediaStream, 0, sizeof(*iceMediaStream));
}

void ICELIBTYPES_ICE_MEDIA_reset(ICE_MEDIA *iceMedia)
{
    memset(iceMedia, 0, sizeof(*iceMedia));
}
