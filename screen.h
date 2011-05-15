#ifndef SCREEN_H
#define SCREEN_H

#include <ncurses.h>

struct player_pos_
{
  int x;
  int y;
}; 


void place_hit_or_mis(WINDOW *, int, int, int);

/**
 * Displays the current battlefield to the user.
 */
void display_boards(void);


/**
 * Displays both battlefields. (Should only be used after using exchange_shipsets)
 */
void show_battlefields();

/**
 * Shows the main menu
 */
void main_menu();

/**
 * Does the gameplay
 */
void do_gameplay(const int, int);
void title_screen();



/**
 * Utility functions and macros
 */

/* Show global message box in middle of the screen */
void show_message_box(char const *const string);
/* Hide the global message box */
void hide_message_box();

/**
 * get a string from a little pop-up dialog.
 * dest should have space.
 */
void get_text_string_from_centered_panel(char const * const, char *, int);

/* Print in the middle of the given window */
void print_in_middle(WINDOW *, int, int, int, char const *const, chtype);

/* Picture _must_ be NULL terminated */
int get_picture_width(char *[]);
/* print picture to window */
void print_picture(WINDOW *, char *[]);

/* clean up the screen for exiting: */
void cleanup_ncurses();

/* COLORS */
#define YELLOW_ON_BLACK COLOR_PAIR(2)
#define BLUE_ON_BLACK COLOR_PAIR(3)
#define RED_ON_BLACK COLOR_PAIR(4)
#define BLACK_ON_WHITE COLOR_PAIR(5)
#define WHITE_ON_BLACK COLOR_PAIR(6)
#define GREEN_ON_BLACK COLOR_PAIR(7)
#define WHITE_ON_RED COLOR_PAIR(8)


#endif
