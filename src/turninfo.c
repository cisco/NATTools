#include <sockaddr_util.h>

#include <curses.h>

#include "turninfo.h"



void initTurnInfo(struct turn_info *turnInfo)
{

    memset(turnInfo, 0, sizeof( struct turn_info));

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
           sockaddr_toString((struct sockaddr *)&result->relAddr,
                             addr,
                             sizeof(addr),
                             true));


}
#endif

int printPermissionsw(WINDOW *win, int startx, int starty, struct turn_permissions *perm)
{
    char addr[SOCKADDR_MAX_STRLEN];
    int i;

    init_pair(1, COLOR_CYAN, COLOR_BLACK);

    mvwprintw(win, starty, startx,"Permissions:");
    startx = startx +13;

    for (i=0;i<perm->numPermissions;i++){
        sockaddr_toString((struct sockaddr *)&perm->permissions[i],
                          addr,
                          sizeof(addr),
                          false);

        mvwprintw(win, starty, startx, "'%s'",
                  addr);

        startx = startx + strlen(addr) + 3;

    }






    if(perm->ok){
        mvwchgat(win, starty, 0, -1, A_BLINK, 1, NULL);
    }


    return ++starty;


}

int printAllocationResultw(WINDOW *win, int startx, int starty, struct turn_allocation_result *result)
{

    char addr[SOCKADDR_MAX_STRLEN];

    mvwprintw(win, starty++, startx,"Active TURN server: '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->activeTurnServerAddr,
                             addr,
                             sizeof(addr),
                             true));

    mvwprintw(win, starty++, startx,"HOST addr:          '%s'\n",
              sockaddr_toString((struct sockaddr *)&result->hostAddr,
                                addr,
                                sizeof(addr),
                                true));


    mvwprintw(win, starty++, startx,"RFLX addr:          '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->rflxAddr,
                             addr,
                             sizeof(addr),
                             true));

    mvwprintw(win, starty++, startx,"RELAY addr:         '%s'\n",
           sockaddr_toString((struct sockaddr *)&result->relAddr,
                             addr,
                             sizeof(addr),
                             true));

    return starty;
}

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

void wprintTurnInfow( WINDOW *win, struct turn_info *turnInfo )
{
    char addr[SOCKADDR_MAX_STRLEN];
    int starty = 1;
    int startx = 2;

    mvwprintw(win,
              starty++,
              startx,
              "TURN Server     : '%s'\n",
              turnInfo->fqdn);
    mvwprintw(win,
              starty++,
              startx,
              "TURN Server IPv4: '%s'\n",
              sockaddr_toString((struct sockaddr *)&turnInfo->remoteIp4,
                                addr,
                                sizeof(addr),
                                true));

    mvwprintw(win,
              starty++,
              startx,"TURN Server IPv6: '%s'\n",
              sockaddr_toString((struct sockaddr *)&turnInfo->remoteIp6,
                                addr,
                                sizeof(addr),
                                true));


    mvwprintw(win,
              starty++,
              startx,"Local IPv4      : '%s'\n",
              sockaddr_toString((struct sockaddr *)&turnInfo->localIp4,
                                addr,
                                sizeof(addr),
                                false));

    mvwprintw(win,
              starty++,
              startx,
              "Local IPv6      : '%s'\n",
              sockaddr_toString((struct sockaddr *)&turnInfo->localIp6,
                                addr,
                                sizeof(addr),
                                false));

    mvwprintw(win, starty++, startx,"TURN 44 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_44);
    starty = printPermissionsw(win, startx+5, starty, &turnInfo->turnAlloc_44.turnPerm);

    mvwprintw(win, starty++, startx,"TURN 46 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_46);
    starty = printPermissionsw(win, startx+5, starty, &turnInfo->turnAlloc_46.turnPerm);

    mvwprintw(win, starty++, startx,"TURN 64 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_64);
    starty = printPermissionsw(win, startx+5, starty, &turnInfo->turnAlloc_64.turnPerm);

    mvwprintw(win, starty++, startx,"TURN 66 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_66);
    starty = printPermissionsw(win, startx+5, starty, &turnInfo->turnAlloc_66.turnPerm);

}


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

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddr)){
        sockaddr_toString((struct sockaddr *)&turnInfo->turnAlloc_44.relAddr,
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

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddr)){
        sockaddr_toString((struct sockaddr *)&turnInfo->turnAlloc_66.relAddr,
                          addr,
                          sizeof(addr),
                          true);
        strcat(string, addr);
        strcat(string, " ");
    }



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
                if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddr)){
                    sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_44.turnPerm.permissions[turnInfo->turnAlloc_44.turnPerm.numPermissions++],
                                  (struct sockaddr *)&addr);
                }

                if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_64.relAddr)){
                    sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_64.turnPerm.permissions[turnInfo->turnAlloc_64.turnPerm.numPermissions++],
                                  (struct sockaddr *)&addr);
                }
            }
            if (addr.ss_family == AF_INET6){
                if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_46.relAddr)){
                    sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_46.turnPerm.permissions[turnInfo->turnAlloc_46.turnPerm.numPermissions++],
                                  (struct sockaddr *)&addr);
                }
                if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddr)){
                    sockaddr_copy((struct sockaddr *)&turnInfo->turnAlloc_66.turnPerm.permissions[turnInfo->turnAlloc_66.turnPerm.numPermissions++],
                                  (struct sockaddr *)&addr);
                }
            }
        }

        if(moreAddr)
            string = idx+1;

    }
}
