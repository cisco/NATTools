#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <string.h>
#include <time.h>
#include <pthread.h>

#include "iphelper.h"
#include "gather.h"
#include "permissions.h"



static int keep_ticks; //Used to determine if it is time to send keepalives
static void *tickTurn(void *ptr);

static struct turn_info turnInfo;
static struct listenConfig listenConfig;
static pthread_t turnListenThread;
static pthread_t turnTickThread;

static char permission_ip[1000];
static char message[100];
static char rcv_message[100];
static char message_dst[50];
static int highlight = 1;


pthread_mutex_t turnInfo_mutex = PTHREAD_MUTEX_INITIALIZER;

void update_turnInfo(){
    pthread_mutex_lock( &turnInfo_mutex );
    print_status(status_win, &turnInfo);
    pthread_mutex_unlock( &turnInfo_mutex );
    print_menu(menu_win, highlight);
    print_input(input_win);
}


void init_view();
void cleanup();
void doChoice(int choice);
void messageCB(char *message);


static void *tickTurn(void *ptr){
    struct timespec timer;
    struct timespec remaining;
    uint32_t  *ctx = (uint32_t *)ptr;
    struct turn_info *tInfo = (struct turn_info*)ptr;

    
    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    for(;;){
        
        nanosleep(&timer, &remaining);
        
        if(tInfo->turnAlloc_44.tInst != NULL){
            TurnClient_HandleTick(tInfo->turnAlloc_44.tInst);
        }
        if(tInfo->turnAlloc_46.tInst != NULL){
            TurnClient_HandleTick(tInfo->turnAlloc_46.tInst);
        }
        if(tInfo->turnAlloc_64.tInst != NULL){
            TurnClient_HandleTick(tInfo->turnAlloc_64.tInst);
        }
        if(tInfo->turnAlloc_66.tInst != NULL){
            TurnClient_HandleTick(tInfo->turnAlloc_66.tInst);
        }
    }

}

int main(int argc, char *argv[])
{

    int choice = 0;
    int c;

    setbuf(stdout, NULL);


    if (argc != 5) {
        fprintf(stderr,"usage: testturn  iface turnserver user pass\n");
        exit(1);
    }

    //Our local struct where we store stuff
    initTurnInfo(&turnInfo);
    addCredentials(&turnInfo, argv[2], argv[3], argv[4]);
    
    getRemoteTurnServerIp(&turnInfo, argv[2]);

    getLocalIPaddresses(&turnInfo, SOCK_DGRAM, argv[1]);

    //Turn setup
    //TurnClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, "TestTurn");
    
    //gatherAll(&turnInfo, &listenConfig);
    //pthread_create( &turnTickThread, NULL, tickTurn, (void*) &turnInfo);
    //pthread_create( &turnListenThread, NULL, stunListen, (void*)&listenConfig);
    
    //for(;;){
    //    sleep(2);
    //}
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
        if(choice != 0){	// User did a choice come out of the infinite loop 
            doChoice(choice);
        }
        choice = 0;
    }
    
    return 0;
}





void init_view()
{
    int max_x, max_y;
    int start_menu_x = 0;
    int start_menu_y = 0;

    int start_input_x = 0;
    int start_input_y = 0;

    int start_message_x = 0;
    int start_message_y = 0;




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

    start_message_x = 0;
    start_message_y = start_input_y - 3;
    message_win = newwin(3, max_x, start_message_y, start_message_x);



    status_win = newwin(start_message_y-1, max_x, 0, 0);

    print_menu(menu_win, 1);
    print_input(input_win);
    print_message(message_win, " ");
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
        pthread_create( &turnTickThread, NULL, tickTurn, (void*) &turnInfo);
        pthread_create( &turnListenThread, NULL, stunListen, (void*)&listenConfig);
        break;

    case 2:
        mvwprintw(input_win,1, 1, "Enter Ip adresses (enter for rflx): ");
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

void messageCB(char *message)
{
    int n = sizeof(rcv_message);
    strncpy(rcv_message, message, n);
    if (n > 0)
        rcv_message[n - 1]= '\0';

    print_message(message_win, rcv_message);

}
