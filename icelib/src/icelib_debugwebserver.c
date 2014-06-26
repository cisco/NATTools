/*
Copyright 2014 Cisco. All rights reserved. 

Redistribution and use in source and binary forms, with or without modification, are 
permitted provided that the following conditions are met: 

   1. Redistributions of source code must retain the above copyright notice, this list of 
      conditions and the following disclaimer. 

   2. Redistributions in binary form must reproduce the above copyright notice, this list 
      of conditions and the following disclaimer in the documentation and/or other materials 
      provided with the distribution. 

THIS SOFTWARE IS PROVIDED BY CISCO ''AS IS'' AND ANY EXPRESS OR IMPLIED 
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH 
DAMAGE. 

The views and conclusions contained in the software and documentation are those of the 
authors and should not be interpreted as representing official policies, either expressed 
or implied, of Cisco.
*/

#include "icelib.h"
#include "icelib_intern.h"
#include "icelib_debugwebserver.h"

#include "netaddr.h"

#include <stdio.h>

static void  ICELIB_FormatHeaderOk(char  *resp_head, char *resp, const char  *content)
{
    sprintf(resp_head, "HTTP/1.0 200 OK\r\n");
    sprintf(&resp_head[strlen(resp_head)], "Content-type: %s\r\n", content);
    sprintf(&resp_head[strlen(resp_head)], "Content-length: %d\r\n", strlen(resp));
    sprintf(&resp_head[strlen(resp_head)], "\r\n");
}

static void ICELIB_RowStart(char *s, const char *bgColour, bool isExcel) {
    if (!isExcel) {
        sprintf(&s[strlen(s)], "<tr bgcolor=%s>", bgColour);
    }
}

static void ICELIB_RowEnd(char *s, bool isExcel) 
{
    if (!isExcel) {
        sprintf(&s[strlen(s)], "</tr>");
    }
}

static void ICELIB_FormatUint32_1(char *s, uint32_t val, bool isExcel, bool isTable, bool singleRow)
{
    if (isExcel)
        sprintf(&s[strlen(s)], "\t %d\n", val);  /* simple tab field separator */
    else if (isTable)
    {
        if (singleRow)
            sprintf(&s[strlen(s)], "<tr><td>%d</td></tr>\n", val);  /* single row */
        else
            sprintf(&s[strlen(s)], "<td>%d</td>\n", val);             /* continuation */
    }
    else
        sprintf(&s[strlen(s)], "%d<BR>\n", val);
}


static void ICELIB_FormatStr(char *s, const char *val, bool isExcel, bool isTable, bool singleRow)
{
    if (isExcel)
        sprintf(&s[strlen(s)], "\t %s\n", val);  /* simple tab field separator */
    else if (isTable)
    {
        if (singleRow)
            sprintf(&s[strlen(s)], "<tr><td>%s</td></tr>\n", val);  /* row */
        else
            sprintf(&s[strlen(s)], "<td>%s</td>\n", val);  /* row */
    }
    else
        sprintf(&s[strlen(s)], "%s<BR>\n", val);
}




static const char *ICELIB_IceComponentIdToStr(uint32_t componentId)
{
    switch (componentId)
    {
        case 1: return "RTP";
        case 2: return "RTCP";
        default: return "???";
    }
}



void ICELIB_FormatValidCheckListsBodyTable(ICELIB_INSTANCE *icelib, char *s, char *content) {
    int32_t j = 0;
    uint32_t i;
    sprintf(&s[strlen(s)], "<H2> Session %i </H2>\n", j);

    sprintf(&s[strlen(s)],  "<table border cellspacing=6>\n");

    /* for all media streams */
    for (i = 0; i < icelib->numberOfMediaStreams; ++i) {
        ICELIB_VALIDLIST *pValidList;
        ICELIB_VALIDLIST_ITERATOR vlIterator;
        ICELIB_VALIDLIST_ELEMENT *pValidPair;
        int j;

        pValidList = &icelib->streamControllers[i].validList;
        ICELIB_validListIteratorConstructor(&vlIterator, pValidList);

        /* for all valid pairs */
        for (j=0; ( ( pValidPair = pICELIB_validListIteratorNext( &vlIterator)) != NULL); j++)
        {
            char addr[MAX_NET_ADDR_STRING_SIZE];

            if ((i == 0) && (j == 0))
            {
                /* table header row */
                ICELIB_RowStart(&s[strlen(s)], "wheat", false);
                sprintf(&s[strlen(s)], "<th>MediaStream</th>\n");
                sprintf(&s[strlen(s)], "<th>PairId</th>\n");
                sprintf(&s[strlen(s)], "<th>State</th>\n");
                sprintf(&s[strlen(s)], "<th>Nominated</th>\n");
                sprintf(&s[strlen(s)], "<th>L-Comp</th>\n");
                sprintf(&s[strlen(s)], "<th>L-Type</th>\n");
                sprintf(&s[strlen(s)], "<th>L-Addr</th>\n");
                sprintf(&s[strlen(s)], "<th>R-Comp</th>\n");
                sprintf(&s[strlen(s)], "<th>R-Type</th>\n");
                sprintf(&s[strlen(s)], "<th>R-Addr</th>\n");
                ICELIB_RowEnd(&s[strlen(s)], false);
            }

            ICELIB_RowStart(&s[strlen(s)], pValidPair->nominatedPair ? "chartreuse" : "white", false);
            ICELIB_FormatUint32_1(&s[strlen(s)], i, false, true, false);
            ICELIB_FormatUint32_1(&s[strlen(s)], pValidPair->pairId, false, true, false);
            ICELIB_FormatStr(&s[strlen(s)], ICELIB_toString_CheckListPairState(pValidPair->pairState), false, true, false);
            ICELIB_FormatUint32_1(&s[strlen(s)], pValidPair->nominatedPair ? 1 : 0, false, true, false);

            /* local cand */
            ICELIB_FormatStr(&s[strlen(s)], 
                             ICELIB_IceComponentIdToStr(pValidPair->pLocalCandidate->componentid), false, true, false);
            ICELIB_FormatStr(&s[strlen(s)], 
                             ICELIBTYPES_ICE_CANDIDATE_TYPE_toString(pValidPair->pLocalCandidate->type), false, true, false);
            netaddr_toString(&pValidPair->pLocalCandidate->connectionAddr, addr , sizeof( addr), true);
            ICELIB_FormatStr(&s[strlen(s)], addr, false, true, false);

            /* remote cand */
            ICELIB_FormatStr(&s[strlen(s)], 
                             ICELIB_IceComponentIdToStr(pValidPair->pRemoteCandidate->componentid), false, true, false);
            ICELIB_FormatStr(&s[strlen(s)], 
                             ICELIBTYPES_ICE_CANDIDATE_TYPE_toString(pValidPair->pRemoteCandidate->type), false, true, false);
            netaddr_toString(&pValidPair->pRemoteCandidate->connectionAddr, addr , sizeof( addr), true);
            ICELIB_FormatStr(&s[strlen(s)], addr, false, true, false);

            ICELIB_RowEnd(&s[strlen(s)], false);

        } /* for all valid pairs */

    } /* for all media streams */

    sprintf(&s[strlen(s)],  "</table>\n");
}

void ICELIB_FormatIcePage(char *resp, char *resp_head)
{
    /* body */
    sprintf(resp, "<H1>TANDBERG ICE DEBUGGER</H1>\n");
    sprintf(&resp[strlen(resp)], "<H2>ICE</H2>\n");

    sprintf(&resp[strlen(resp)], "<BR><A HREF=\"/DumpAllCheckLists\">DumpAllCheckLists</A>\n");
    sprintf(&resp[strlen(resp)], "<BR><A HREF=\"/DumpValidCheckLists\">DumpValidCheckLists</A>\n");

    /* Create response header */
    ICELIB_FormatHeaderOk(resp_head, resp, "text/html");
}
