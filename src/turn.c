#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>

#include <curses.h>
#include <form.h>

#include "sockaddr_util.h"
#include "gather.h"
#include "ipaddress_helper.h"



/* Callback for management info  */
static void  PrintTurnInfo(TurnInfoCategory_T category, char *ErrStr)
{
    //fprintf(stderr, "%s\n", ErrStr);
}

static void *tickTurn(void *ptr){
    struct timespec timer;
    struct timespec remaining;
    uint32_t  *ctx = (uint32_t *)ptr;

    timer.tv_sec = 0;
    timer.tv_nsec = 50000000;

    for(;;){
        nanosleep(&timer, &remaining);
        TurnClient_HandleTick(*ctx);
    }
}


static struct turn_info turnInfo;
static WINDOW *turnInfoWin;

WINDOW *create_newwin(int height, int width, int starty, int startx)
{	
    WINDOW *local_win;
    
    local_win = newwin(height, width, starty, startx);
    box(local_win, 0 , 0);		/* 0, 0 gives default characters 
					 * for the vertical and horizontal
					 * lines			*/
    wrefresh(local_win);		/* Show that box 		*/
    
    return local_win;
}

void update_turnInfo(){
        
    wprintTurnInfow(turnInfoWin, &turnInfo);
    wborder(turnInfoWin, '|', '|', '-', '-', '+', '+', '+', '+');
    wrefresh(turnInfoWin);

}


int main(int argc, char *argv[])
{
    pthread_t turnTickThread;
    pthread_t turnListenThread;

    struct listenConfig listenConfig;
    
    int ch;
    char perm_ip[200];
    static WINDOW *menuWin;

    if (argc != 5) {
        fprintf(stderr,"usage: testice  iface turnserver user pass\n");
        exit(1);
    }
    memset(&turnInfo, 0, sizeof turnInfo);
    
    strncpy(turnInfo.fqdn, argv[2], FQDN_MAX_LEN);
    strncpy(turnInfo.user, argv[3], STUN_MSG_MAX_USERNAME_LENGTH);
    strncpy(turnInfo.pass, argv[4], STUN_MSG_MAX_PASSWORD_LENGTH);
    
    initscr();			/* Start curses mode 		*/
    clear();
    noecho();
    cbreak();
    
    
    

    turnInfoWin = create_newwin(28,90,0,0);
    menuWin = create_newwin(10,70,28,0);

   
    mvprintw(30,0,"Gather all (a), Send Permission (p), Send Message (m), Quit (q)");
    
    refresh();

        
    getRemoteTurnServerIp(&turnInfo, argv[2]);

    getLocalIPaddresses(&turnInfo, argv[1]);
    
    //Turn setup
    TurnClient_Init(TEST_THREAD_CTX, 50, 50, PrintTurnInfo, false, "TestIce");
    pthread_create( &turnTickThread, NULL, tickTurn, (void*) &TEST_THREAD_CTX);

    update_turnInfo();
    
    while((ch = getch()) != 'q'){
        switch(ch){
        case 'q':
            mvprintw(29,0 ,"Bye bye!! ");
            endwin();
            //Close socets as well?
            return 0;
        case 'a':
            gatherAll(&turnInfo, &listenConfig, &update_turnInfo);
            pthread_create( &turnListenThread, NULL, stunListen, (void*)&listenConfig);
            break;
        case 'p':
            mvprintw(29,0 ,"Enter ip: ");
            refresh();
            echo();
            scanw("%s", perm_ip);
            noecho();
            mvprintw(30,0,"got %s:", perm_ip);
            break;
        }
    } 
    
}
