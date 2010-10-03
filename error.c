#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void graceful_exit(char *msg, int sock)
{
  endwin(); //end curses mode
  if (sock > -1) {
    printf("Cleaning up...\n");
    close(sock);
  }
  perror(msg);
  exit(EXIT_FAILURE);
}
