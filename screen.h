#ifndef SCREEN_H
#define SCREEN_H

struct player_pos_
{
  int x;
  int y;
}; 


void place_hit_or_mis(WINDOW * win,int mesg, int x, int y);

/**
 * Displays the current battlefield to the user.
 */
void display_boards(void);


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
void print_in_middle(WINDOW *, int, int, int, char *, chtype);
/* Picture _must_ be NULL terminated */
int get_picture_width(char *[]);

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))


#endif
