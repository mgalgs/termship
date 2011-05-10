#ifndef SCREEN_H
#define SCREEN_H

struct player_pos_
{
  int x;
  int y;
}; 


void place_hit_or_mis(WINDOW *,int, int, int);

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
void print_in_middle(WINDOW *, int, int, int, char const *const, chtype);
/* Picture _must_ be NULL terminated */
int get_picture_width(char *[]);
/* print picture to window */
void print_picture(WINDOW *, char *[]);
/* get a string from a little pop-up dialog */
char *get_text_string_from_centered_panel(char const * const);


#endif
