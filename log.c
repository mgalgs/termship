#include <stdlib.h>
#include <stdio.h>
#include "log.h"

extern int user_mode;

void write_to_log(char *st)
{
  static FILE *thelogfile = NULL;
  if (thelogfile == NULL) {
    thelogfile = fopen(LOG_FILE, "a+");
  }
  fprintf(thelogfile, "[%d]: %s", user_mode, st);
  fflush(thelogfile);
}
