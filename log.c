#include <stdlib.h>
#include <stdio.h>
#include "log.h"

void write_to_log(char *st)
{
  static FILE *thelogfile = NULL;
  if (thelogfile == NULL) {
    thelogfile = fopen(LOG_FILE, "a+");
  }
  fprintf(thelogfile, "%s", st);
  fflush(thelogfile);
}
