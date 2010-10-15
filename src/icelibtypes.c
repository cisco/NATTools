#include <stdio.h>
#include <string.h>

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

void ICELIBTYPES_ICE_CANDIDATE_dump(const ICE_CANDIDATE *candidate){
    char addr[MAX_NET_ADDR_STRING_SIZE];
    printf("   Fnd: '%s' ", candidate->foundation);
    printf("Comp: %i ", candidate->componentid);
    printf("Pri: %u ", candidate->priority);
    printf("Addr: %s", netaddr_toStr(&candidate->connectionAddr, addr, sizeof addr, true));
           //netaddr_dump(&candidate->connectionAddr, true);
    printf(" Type: '%s' ", ICELIBTYPES_ICE_CANDIDATE_TYPE_toString(candidate->type));
    printf(" UVal1: %u ", candidate->userValue1);
    printf(" UVal2: %u\n", candidate->userValue2);
}

void ICELIBTYPES_ICE_MEDIA_STREAM_dump(const ICE_MEDIA_STREAM *iceMediaStream)
{
    uint32_t i;
    printf(" Number of Candidates: %i\n", iceMediaStream->numberOfCandidates);
    if(iceMediaStream->numberOfCandidates > 0){
        printf(" Ufrag : '%s'\n", iceMediaStream->ufrag);
        printf(" Passwd: '%s'\n", iceMediaStream->passwd);
    }
    for(i=0; i<iceMediaStream->numberOfCandidates; i++) {
        printf(" Candidate[%i]\n", i);
        ICELIBTYPES_ICE_CANDIDATE_dump(&iceMediaStream->candidate[i]);
    }

}


void ICELIBTYPES_ICE_MEDIA_dump(const ICE_MEDIA *iceMedia)
{
    uint32_t i;
    uint32_t printed =0;

    printf("Number of Media Lines: %i\n",iceMedia->numberOfICEMediaLines);

    for (i=0; i< ICE_MAX_MEDIALINES; i++) {
        if ( ICELIBTYPES_ICE_MEDIA_STREAM_isEmpty( &iceMedia->mediaStream[i] ) == false ){
            if ( printed <= iceMedia->numberOfICEMediaLines) {
                printf("---  ICEMediaLine[%i] ---\n", i);
                ICELIBTYPES_ICE_MEDIA_STREAM_dump(&iceMedia->mediaStream[i]);
                printed++;
            } else {
                printf("--- Number of ICEMedialines exceeded (Got %i, excpected %i)\n",
                       printed, iceMedia->numberOfICEMediaLines);
            }
        }
        else {
            printf("---  ICEMediaLine[%i] ---\n", i);
            printf("[empty]\n");
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
