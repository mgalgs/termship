#include <ncurses.h>
#include "screen.h"
#include "connection.h"
#include "gamepieces.h"
#include "log.h"


Player *player;


int main() {
  write_to_log("termship is starting!!!\n");

  initscr();			/* Initialize screen for curses mode */
  cbreak();
  noecho();                       

  start_color();		/* start color */
  /* set up some color pairs: */
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);
  init_pair(4, COLOR_RED, COLOR_BLACK);
  init_pair(5, COLOR_BLACK, COLOR_WHITE);
  init_pair(6, COLOR_WHITE, COLOR_BLACK);
  init_pair(7, COLOR_GREEN, COLOR_BLACK);


  main_menu();

  endwin();			/* end curses mode */

  return 0;
}
