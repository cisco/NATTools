
#include <sockaddr_util.h>


#include "view.h"


int printPermissionsw(WINDOW *win, int startx, int starty, struct turn_permissions *perm)
{
    char addr[SOCKADDR_MAX_STRLEN];
    int i;
    
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    
    mvwprintw(win, starty, startx,"Permissions:");
    startx = startx + 13;
    mvwprintw(win, starty, startx, "'%i'",
                      perm->numPermissions);
            
                
    /*
    for (i=0;i<=perm->numPermissions;i++){
        
        
        if( sockaddr_toString((struct sockaddr *)&perm->permissions[i],
                              addr,
                              sizeof(addr),
                              false) ) {    
            
            //mvwprintw(win, starty, startx, "'%s'",
            //          addr);
            startx = startx + strlen(addr) + 3;

            mvwprintw(win, starty, startx, "'%s'",
                      test);
            
            startx = startx + strlen(test) + 3;
        }
        
        }
    */
    
    if(perm->ok){
        //mvwchgat(win, starty, 0, -1, A_BLINK, 1, NULL);
    }
        
    return ++starty;


}



int printAllocationResultw(WINDOW *win, int startx, int starty, struct turn_allocation *alloc)
{

    char addr[SOCKADDR_MAX_STRLEN];

    mvwprintw(win, starty++, startx,"Active TURN server: '%s'\n",
           sockaddr_toString((struct sockaddr *)&alloc->activeTurnServerAddr,
                             addr,
                             sizeof(addr),
                             true));

    mvwprintw(win, starty++, startx,"HOST addr:          '%s'\n",
              sockaddr_toString((struct sockaddr *)&alloc->hostAddr,
                                addr,
                                sizeof(addr),
                                true));


    mvwprintw(win, starty++, startx,"RFLX addr:          '%s'\n",
           sockaddr_toString((struct sockaddr *)&alloc->rflxAddr,
                             addr,
                             sizeof(addr),
                             true));

    mvwprintw(win, starty++, startx,"RELAY addr:         '%s'\n",
              sockaddr_toString((struct sockaddr *)&alloc->relAddrIPv4,
                                addr,
                                sizeof(addr),
                                true));
    
    mvwprintw(win, starty++, startx,"RELAY addr:         '%s'\n",
              sockaddr_toString((struct sockaddr *)&alloc->relAddrIPv6,
                                addr,
                                sizeof(addr),
                             true));
    
    mvwprintw(win, starty++, startx,"Permissions: %i", alloc->turnPerm.numPermissions);
        
    return starty;
}


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

    mvwprintw(win,
              starty++,
              startx,
              "Local IPv6 (ULA): '%s'\n",
              sockaddr_toString((struct sockaddr *)&turnInfo->localIp6_ula,
                                addr,
                                sizeof(addr),
                                false));
    
    mvwprintw(win, starty++, startx,"TURN 44 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_44);
    //starty = printPermissionsw(win, startx+5, starty, &turnInfo->turnAlloc_44.turnPerm);

    mvwprintw(win, starty++, startx,"TURN 46 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_46);
    //starty = printPermissionsw(win, startx+5, starty, &turnInfo->turnAlloc_46.turnPerm);

    mvwprintw(win, starty++, startx,"TURN 64 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_64);
    //starty = printPermissionsw(win, startx+5, starty, &turnInfo->turnAlloc_64.turnPerm);

    mvwprintw(win, starty++, startx,"TURN 66 Result");
    starty = printAllocationResultw(win, startx+5, starty, &turnInfo->turnAlloc_66);
    //starty = printPermissionsw(win, startx+5, starty, &turnInfo->turnAlloc_66.turnPerm);

}


void print_status(WINDOW *status_win, struct turn_info *turnInfo)
{
    werase(status_win);
    box(status_win, 0, 0);

    //
    wprintTurnInfow(status_win, turnInfo);
    box(status_win, 0, 0);

    wrefresh(status_win);
}


void print_input(WINDOW *input_win)
{
    werase(input_win);
    box(input_win, 0, 0);
    mvwprintw(input_win, 0, 2, "Input");

    wrefresh(input_win);
}

void print_menu(WINDOW *menu_win, int highlight)
{
	int x, y, i;	

	x = 2;
	y = 2;
	box(menu_win, 0, 0);
        mvwprintw(menu_win, 0, 2, "Menu (Use left/right arrow keys)");
	for(i = 0; i < n_choices; ++i)
	{	
            if(highlight == i + 1) /* High light the present choice */
            {	
                wattron(menu_win, A_REVERSE); 
                mvwprintw(menu_win, y, x, "%s", choices[i]);
                wattroff(menu_win, A_REVERSE);
            }
            else
                mvwprintw(menu_win, y, x, "%s", choices[i]);
            //++y;
            x = x+strlen(choices[i])+1;
            
	}
	wrefresh(menu_win);
}

void print_message(WINDOW *message_win, char *message)
{
    
    box(message_win, 0, 0);
    mvwprintw(message_win, 0, 2, "Incomming Messages");
    mvwprintw(message_win, 1, 1, "%s", message);
    wrefresh(message_win);
    
    

}
