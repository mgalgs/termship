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
void show_board();

/**
 * Shows the main menu
 */
void main_menu();

/**
 * Does the gameplay
 */
void do_gameplay(const int, int);
void title_screen();

#endif
