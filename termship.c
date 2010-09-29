#include <ncurses.h>
#include "screen.h"
#include "connection.h"
#include "gamepieces.h"
#include "log.h"


Player *player;


int main() {
  write_to_log("termship is starting!!!\n");

  initscr();//Initialize screen for curses mode

  main_menu();

  endwin(); //end curses mode

  return 0;
}
