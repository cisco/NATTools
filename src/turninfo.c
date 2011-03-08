

#include <sockaddr_util.h>

#include <curses.h>

#include "turninfo.h"



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
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    
    

    mvwprintw(win, starty++, startx,"Permissions: '%s'\n",
           sockaddr_toString((struct sockaddr *)&perm->permissions[0],
                             addr,
                             sizeof(addr),
                             false));
    
    
    
    if(perm->ok){
        mvwchgat(win, starty-1, startx, -1, A_BLINK, 1, NULL);
    }

    return starty;


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


void fillPermissions(struct turn_info *turnInfo, char *string)
{
    int n;
    int i;

    struct sockaddr_storage addr;
    
    mvprintw(24, 2, "Got '%s'", string);
    refresh();
    
    
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
}
