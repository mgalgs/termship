#include <ncurses.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "screen.h"
#include "connection.h"
#include "gamepieces.h"
#include "log.h"


Player *player;

void sigsegv_handler(int sig)
{
  cleanup_ncurses();
  printf("Whoa, we just overwrote The Grid. Cora's dumping the core of whatever's left...\n");
  fflush(stdout);
  signal(sig, SIG_DFL);
  kill(getpid(), sig);
}

int main() {
  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = sigsegv_handler;
  if (sigaction(SIGSEGV, &sa, NULL) == -1) {
    perror("Couldn't set up signal handler...");
    exit(EXIT_FAILURE);
  }


  /* seed the random number generator */
  srand(time(NULL));

  write_to_log("termship is starting!!!\n");

  initscr();			/* Initialize screen for curses mode */
  cbreak();
  noecho();                       

  start_color();		/* start color */
  /* set up some color pairs: */
  /* some helpful macros to use these are defined in screen.h */
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);
  init_pair(4, COLOR_RED, COLOR_BLACK);
  init_pair(5, COLOR_BLACK, COLOR_WHITE);
  init_pair(6, COLOR_WHITE, COLOR_BLACK);
  init_pair(7, COLOR_GREEN, COLOR_BLACK);
  init_pair(8, COLOR_WHITE, COLOR_RED);


  main_menu();

  cleanup_ncurses();

  return 0;
}


