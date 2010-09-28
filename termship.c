#include <ncurses.h>
#include "screen.h"
#include "connection.h"
#include "gamepieces.h"



Player *player;


int main() {

  initscr();//Initialize screen for curses mode

  main_menu();

  endwin(); //end curses mode

  return 0;
}
