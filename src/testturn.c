
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <time.h>


#include "iphelper.h"
#include "gather.h"



static int keep_ticks; //Used to determine if it is time to send keepalives
static void *tickTurn(void *ptr);

static struct turn_info turnInfo;
static struct listenConfig listenConfig;
static pthread_t turnListenThread;

static char permission_ip[800];
static char message[100];
static char message_dst[50];
static int highlight = 1;    


void update_turnInfo(){
    print_status(status_win, &turnInfo);
    print_menu(menu_win, highlight);
    print_input(input_win);
}


void init_view();
void cleanup();
void doChoice(int choice);


int main(int argc, char *argv[])
{	

    int choice = 0;
    int c;
    
    pthread_t turnTickThread;
    
    
    if (argc != 5) {
        fprintf(stderr,"usage: testice  iface turnserver user pass\n");
        exit(1);
    }
    
    //Our local struct where we store stuff
    initTurnInfo(&turnInfo);
    addCredentials(&turnInfo, argv[2], argv[3], argv[4]);
    
    listenConfig.update_inc_status = print_input_status;

    getRemoteTurnServerIp(&turnInfo, argv[2]);
    
    getLocalIPaddresses(&turnInfo, argv[1]);
    
    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, "TestIce");
    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);
    
    //Ncurses view
    init_view();
    print_status(status_win, &turnInfo);
    
    
    while(1)
    {	c = wgetch(menu_win);
        switch(c)
        {	case KEY_LEFT:
                if(highlight == 1)
                    highlight = n_choices;
                else
                    --highlight;
                break;
        case KEY_RIGHT:
            if(highlight == n_choices)
                highlight = 1;
            else 
                ++highlight;
            break;
        case 10:
            choice = highlight;
            break;
        default:
            refresh();
            break;
        }
        print_menu(menu_win, highlight);
        print_input(input_win);
        if(choice != 0){	/* User did a choice come out of the infinite loop */
            doChoice(choice);
        }
        choice = 0;
    }	
    return 0;
}


void releaseAll(struct turn_info *turnInfo)
{
    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_44.relAddr)){

        TurnClient_Deallocate(TEST_THREAD_CTX, turnInfo->turnAlloc_44.stunCtx);
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_46.relAddr)){

        TurnClient_Deallocate(TEST_THREAD_CTX, turnInfo->turnAlloc_46.stunCtx);
    }

    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_64.relAddr)){

        TurnClient_Deallocate(TEST_THREAD_CTX, turnInfo->turnAlloc_64.stunCtx);
    }
    
    if (sockaddr_isSet((struct sockaddr *)&turnInfo->turnAlloc_66.relAddr)){

        TurnClient_Deallocate(TEST_THREAD_CTX, turnInfo->turnAlloc_66.stunCtx);
    }


}


static void *tickTurn(void *ptr){
    struct timespec timer;
    struct timespec remaining;
    uint32_t  *ctx = (uint32_t *)ptr;

    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    for(;;){
        keep_ticks++;
        nanosleep(&timer, &remaining);
        TurnClient_HandleTick(*ctx);

        if(keep_ticks>100){
            sendKeepalive(&turnInfo);
            keep_ticks = 0;
        }
    }
    
}

void init_view()
{
    int max_x, max_y;
    int start_menu_x = 0;
    int start_menu_y = 0;
    
    int start_input_x = 0;
    int start_input_y = 0;
    



    initscr();
    start_color();
    clear();
    noecho();
    cbreak();	/* Line buffering disabled. pass on everything */

    getmaxyx(stdscr,max_y,max_x);

    start_menu_x = 0;
    start_menu_y = max_y - HEIGHT;	
    menu_win = newwin(HEIGHT, max_x, start_menu_y, start_menu_x);
    keypad(menu_win, TRUE);
    
    start_input_x = 0;
    start_input_y = start_menu_y - 3;
    input_win = newwin(3, max_x, start_input_y, start_input_x);
    
    
    status_win = newwin(start_input_y-1, max_x, 0, 0);
    
    print_menu(menu_win, 1);
    print_input(input_win);

    //print_status(status_win, &turnInfo);
}


void cleanup(){
    clrtoeol();
    refresh();
    endwin();
    
    close(turnInfo.turnAlloc_44.sockfd);
    close(turnInfo.turnAlloc_46.sockfd);
    close(turnInfo.turnAlloc_64.sockfd);
    close(turnInfo.turnAlloc_66.sockfd);
}


void doChoice(int choice)
{
    switch(choice){
    case 1:
        gatherAll(&turnInfo, &listenConfig, &update_turnInfo);
        pthread_create( &turnListenThread, NULL, stunListen, (void*)&listenConfig);
        break;
        
    case 2:
        mvwprintw(input_win,1, 1, "Enter Ip adresses: ");
        wrefresh(input_win);
        echo();
        wgetstr(input_win, permission_ip);
        noecho();
        fillPermissions(&turnInfo, permission_ip);
        sendPermissionsAll(&turnInfo);
        print_status(status_win, &turnInfo);
        break;
    case 3:
        mvwprintw(input_win,1, 1, "Enter Message: ");
        wrefresh(input_win);
        echo();
        wgetstr(input_win, message);
        mvwprintw(input_win,1, 1, "Enter Ip: ");
        wrefresh(input_win);
        wgetstr(input_win, message_dst);
        noecho();
        sendMessage(&turnInfo, message_dst, message);
        break;
    case 4:
        releaseAll(&turnInfo);
        break;
    case 5:
        releaseAll(&turnInfo);
        cleanup();
        exit(0);
        break;
    }   
}
