#ifndef SCREEN_H
#define SCREEN_H


#include <ncurses.h>
#include <panel.h>
#include "common.h"

struct player_pos_
{
  int x;
  int y;
}; 


typedef struct Animation {
  int numFrames;
  int fps;
  char **frames;
  int width;
  int height;

  /* We load the frames from here. This should be present in the
     ANIMATIONS_PATH. */
  char *loadFile;
  bool isLoaded;
} Animation;

/* This doesn't load it. Lazy loading happens when the animation is
   played, or you can manually load it with load_animation. */
Animation *create_animation(char *);

/**
 * The animation file should have the following headers before the
 * actual frames begin:
 *
 * size (in lines) of each frame
 * total number of frames
 * desired frame rate (in fps)
 *
 * There should be one blank line between every frame. There should
 * not be a blank line between the headers and the first frame.
 *
 *
 # file format:
 # =============
 # height of each frame
 # total frames
 # frame rate
 # <first frame data...>
 # <one blank line>
 # <second frame data...>
 # ...
 # <last frame data...>
 # <newline on it's own line>

 # Note, there's not a newline between the header lines and the first
 # frame
 */
void load_animation(Animation *);
void play_animation(Animation *, bool);
void destroy_animation(Animation *);


void place_hit_or_mis(WINDOW *, int, int, int, bool);

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
void show_message_box(char const *const);
/* Hide the global message box */
void hide_message_box();

/* These are the "manual" message box routines, in case you want to
   pass in your own window */
void hide_message_box_win(WINDOW **, PANEL **);
void show_message_box_win(WINDOW **, PANEL **, char const *const, int *, int *);


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
