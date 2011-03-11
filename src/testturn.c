
#include <stdlib.h>
#include <stdio.h>
#include <ncurses.h>
#include <string.h>
#include <time.h>


#include "iphelper.h"
#include "gather.h"

#define WIDTH 60
#define HEIGHT 5 


static int keep_ticks; //Used to determine if it is time to send keepalives
static void *tickTurn(void *ptr);


char *choices[] = { 
    "Gather All",
    "Send Permissions",
    "Send Message",
    "Release",
    "Exit",
};

int n_choices = sizeof(choices) / sizeof(char *);
void print_menu(WINDOW *menu_win, int highlight);
void print_input(WINDOW *input_win);
void print_input_status(char *message);
void print_status(WINDOW *status_win, struct turn_info *turnInfo);
void releaseAll(struct turn_info *turnInfo);


static struct turn_info turnInfo;
static WINDOW *status_win;
static WINDOW *input_win;

void update_turnInfo(){
    print_status(status_win, &turnInfo);
}


int main(int argc, char *argv[])
{	
    WINDOW *menu_win;

    int highlight = 1;
    int choice = 0;
    int c;
    int max_x, max_y;
    int start_menu_x = 0;
    int start_menu_y = 0;
    
    int start_input_x = 0;
    int start_input_y = 0;
    
    char perm_ip[800];
    char message[100];
    char message_dst[50];
    
    
    
    pthread_t turnTickThread;
    pthread_t turnListenThread;
    
    struct listenConfig listenConfig;
    
    
    if (argc != 5) {
        fprintf(stderr,"usage: testice  iface turnserver user pass\n");
        exit(1);
    }
    memset(&turnInfo, 0, sizeof turnInfo);
    
    strncpy(turnInfo.fqdn, argv[2], FQDN_MAX_LEN);
    strncpy(turnInfo.user, argv[3], STUN_MSG_MAX_USERNAME_LENGTH);
    strncpy(turnInfo.pass, argv[4], STUN_MSG_MAX_PASSWORD_LENGTH);
    
    
    listenConfig.update_inc_status = print_input_status;

    getRemoteTurnServerIp(&turnInfo, argv[2]);
    
    getLocalIPaddresses(&turnInfo, argv[1]);
    
    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, NULL, false, "TestIce");
    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);
    
    
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
    
    print_menu(menu_win, highlight);
    print_input(input_win);
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
            mvprintw(24, 0, "Charcter pressed is = %3d Hopefully it can be printed as '%c'", c, c);
            refresh();
            break;
        }
        print_menu(menu_win, highlight);
        print_input(input_win);
        if(choice != 0){	/* User did a choice come out of the infinite loop */
            if(choice == 5){
                break;
            }
            switch(choice){
            case 1:
                gatherAll(&turnInfo, &listenConfig, &update_turnInfo);
                pthread_create( &turnListenThread, NULL, stunListen, (void*)&listenConfig);
                break;
                
            case 2:
                mvwprintw(input_win,1, 1, "Enter Ip adresses: ");
                wrefresh(input_win);
                echo();
                wgetstr(input_win, perm_ip);
                noecho();
                fillPermissions(&turnInfo, perm_ip);
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
            case 4:
                releaseAll(&turnInfo);
                
            }
        }
        choice = 0;
        
	
    }	
    mvprintw(23, 0, "You chose choice %d with choice string %s\n", choice, choices[choice - 1]);



    clrtoeol();
    refresh();
    endwin();

    close(turnInfo.turnAlloc_44.sockfd);
    close(turnInfo.turnAlloc_46.sockfd);
    close(turnInfo.turnAlloc_64.sockfd);
    close(turnInfo.turnAlloc_66.sockfd);
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

void print_input_status(char *message)
{
    werase(input_win);
    box(input_win, 0, 0);
    mvwprintw(input_win, 1, 1, "%s", message);
    wrefresh(input_win);

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


