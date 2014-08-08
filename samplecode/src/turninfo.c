#include <sockaddr_util.h>

#include <curses.h>

#include "turninfo.h"



void initTurnInfo(struct turn_info *turnInfo)
{

    memset(turnInfo, 0, sizeof( struct turn_info));

}

void turnInfoFunc(void *userCtx, TurnInfoCategory_T category, char *ErrStr) {
    printf("%s", ErrStr);
}


void addCredentials(struct turn_info *turnInfo, char *fqdn, char *user, char *pass)
{

    strncpy(turnInfo->fqdn, fqdn, FQDN_MAX_LEN);
    strncpy(turnInfo->user, user, STUN_MSG_MAX_USERNAME_LENGTH);
    strncpy(turnInfo->pass, pass, STUN_MSG_MAX_PASSWORD_LENGTH);


}

#if 0
void printAllocationResult(struct turn_allocation_result *result)
{

    char addr[SOCKADDR_MAX_STRLEN];

    printf("   Active TURN server: '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->activeTurnServerAddr,
                             addr,
                             sizeof(addr),
                             true));

    printf("   RFLX addr: '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->rflxAddr,
                             addr,
                             sizeof(addr),
                             true));

    printf("   RELAY addr: '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->relAddrIPv4,
                             addr,
                             sizeof(addr),
                             true));

    printf("   RELAY addr: '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->relAddrIPv6,
                             addr,
                             sizeof(addr),
                             true));


}
#endif


#if 0
void printTurnInfo( struct turn_info *turnInfo )
{
    char addr[SOCKADDR_MAX_STRLEN];

    printf("--- TURN Info (start) -----\n");
    printf("TURN Server     : '%s'\n", turnInfo->fqdn);
    printf("TURN Server IPv4: '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->remoteIp4,
                                                         addr,
                                                         sizeof(addr),
                                                         true));

    printf("TURN Server IPv6: '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->remoteIp6,
                                                         addr,
                                                         sizeof(addr),
                                                         true));
    printf("\n");

    printf("Loval IPv4      : '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->localIp4,
                                                         addr,
                                                         sizeof(addr),
                                                         true));

    printf("Loval IPv6      : '%s'\n", sockaddr_toString((struct sockaddr *)&turnInfo->localIp6,
                                                         addr,
                                                         sizeof(addr),
                                                         true));

    printf("TURN 44 Result\n");
    printAllocationResult(&turnInfo->turnAlloc_44);

    printf("TURN 46 Result\n");
    printAllocationResult(&turnInfo->turnAlloc_46);

    printf("TURN 64 Result\n");
    printAllocationResult(&turnInfo->turnAlloc_64);

    printf("TURN 66 Result\n");
    printAllocationResult(&turnInfo->turnAlloc_66);

    printf("--- TURN Info (end) -----\n");

}
#endif



char * getDefaultAddrAsString(char *string, struct turn_info *turnInfo)
{
    //string must be big enough. Not our problem.. :-)
    char addr[SOCKADDR_MAX_STRLEN];

    string[0] = '\0';


    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.hostAddr)){
        sockaddr_toString((struct sockaddr *)&turnInfo->turnAlloc_44.hostAddr,
                          addr,
                          sizeof(addr),
                          true);
        strcat(string, addr);
        strcat(string, " ");
    }


    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.rflxAddr)){
        sockaddr_toString((struct sockaddr *)&turnInfo->turnAlloc_44.rflxAddr,
                          addr,
                          sizeof(addr),
                          true);
        strcat(string, addr);
        strcat(string, " ");
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddrIPv4)){
        sockaddr_toString((struct sockaddr *)&turnInfo->turnAlloc_44.relAddrIPv4,
                          addr,
                          sizeof(addr),
                          true);
        strcat(string, addr);
        strcat(string, " ");
    }


    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddrIPv6)){
        sockaddr_toString((struct sockaddr *)&turnInfo->turnAlloc_44.relAddrIPv6,
                          addr,
                          sizeof(addr),
                          true);
        strcat(string, addr);
        strcat(string, " ");
    }



    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.hostAddr)){
        sockaddr_toString((struct sockaddr *)&turnInfo->turnAlloc_66.hostAddr,
                          addr,
                          sizeof(addr),
                          true);
        strcat(string, addr);
        strcat(string, " ");
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddrIPv4)){
        sockaddr_toString((struct sockaddr *)&turnInfo->turnAlloc_66.relAddrIPv4,
                          addr,
                          sizeof(addr),
                          true);
        strcat(string, addr);
        strcat(string, " ");
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddrIPv6)){
        sockaddr_toString((struct sockaddr *)&turnInfo->turnAlloc_66.relAddrIPv6,
                          addr,
                          sizeof(addr),
                          true);
        strcat(string, addr);
        strcat(string, " ");
    }

    strcat(string, "\0");
    //printf("Addrs: %s\n", string);
    return string;
}


void fillPermissions(struct turn_info *turnInfo, char *string)
{
    int n;
    int i;

    struct sockaddr_storage addr;
    bool moreAddr = true;
    char *idx;

    if( string[0] == '\0') {
        //We fill it with reflexive adresses
        //Makes it easy to test with netcat from the same machine
        getDefaultAddrAsString(string, turnInfo);
    }


    while(moreAddr)
    {
        idx = strchr(string, ' ');

        if( idx != NULL ){

            *idx = '\0';
        }else{
            moreAddr = false;
        }

        if( sockaddr_initFromString((struct sockaddr *)&addr,
                                    string) ){

            if(addr.ss_family == AF_INET){
                
                if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddrIPv4)){
                    sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_44.turnPerm.permissions[turnInfo->turnAlloc_44.turnPerm.numPermissions++],
                                  (struct sockaddr *)&addr);
                }
                
                if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_64.relAddrIPv4)){
                    sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_64.turnPerm.permissions[turnInfo->turnAlloc_64.turnPerm.numPermissions++],
                                  (struct sockaddr *)&addr);
                }
                
            }
            if (addr.ss_family == AF_INET6){
                if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_46.relAddrIPv6)){
                    sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_46.turnPerm.permissions[turnInfo->turnAlloc_46.turnPerm.numPermissions++],
                                  (struct sockaddr *)&addr);
                }
                if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddrIPv6)){
                    sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_66.turnPerm.permissions[turnInfo->turnAlloc_66.turnPerm.numPermissions++],
                                  (struct sockaddr *)&addr);
                }
            }
        }

        if(moreAddr){
            string = idx+1;
        }
    }
}

void fillRflxPermissions(struct turn_info *turnInfo)
{
    int n;
    int i;

    struct sockaddr_storage addr;
    bool moreAddr = true;
    char *idx;
        
    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddrIPv4)){
        sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_44.turnPerm.permissions[turnInfo->turnAlloc_44.turnPerm.numPermissions++],
                                  (struct sockaddr *)&turnInfo->turnAlloc_44.rflxAddr);
        sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_44.turnPerm.permissions[turnInfo->turnAlloc_44.turnPerm.numPermissions++],
                      (struct sockaddr *)&turnInfo->turnAlloc_44.hostAddr);
    
    }
    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_64.relAddrIPv4)){
        sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_64.turnPerm.permissions[turnInfo->turnAlloc_64.turnPerm.numPermissions++],
                                  (struct sockaddr *)&turnInfo->turnAlloc_44.rflxAddr);
        sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_64.turnPerm.permissions[turnInfo->turnAlloc_64.turnPerm.numPermissions++],
                      (struct sockaddr *)&turnInfo->turnAlloc_44.hostAddr);
    
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_46.relAddrIPv6)){
        sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_46.turnPerm.permissions[turnInfo->turnAlloc_46.turnPerm.numPermissions++],
                                  (struct sockaddr *)&turnInfo->turnAlloc_66.hostAddr);
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddrIPv6)){
        sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_66.turnPerm.permissions[turnInfo->turnAlloc_66.turnPerm.numPermissions++],
                                  (struct sockaddr *)&turnInfo->turnAlloc_66.hostAddr);
    }
              
}
