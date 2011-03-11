
#include "view.h"




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
