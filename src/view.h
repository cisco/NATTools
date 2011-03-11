#ifndef VIEW_H
#define VIEW_H


#include <ncurses.h>

#include "turninfo.h"

#define WIDTH 60
#define HEIGHT 5 



static char *choices[] = { 
    "Gather All",
    "Send Permissions",
    "Send Message",
    "Release",
    "Exit",
};

static int n_choices = sizeof(choices) / sizeof(char *);



void print_menu(WINDOW *menu_win, int highlight);
void print_input(WINDOW *input_win);
void print_input_status(char *message);
void print_status(WINDOW *status_win, struct turn_info *turnInfo);
void releaseAll(struct turn_info *turnInfo);



static WINDOW *status_win;
static WINDOW *input_win;
static WINDOW *menu_win;

#endif
