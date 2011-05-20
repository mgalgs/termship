#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "common.h"
#include "screen.h"

void write_to_log(char *st)
{
  static FILE *thelogfile = NULL;
  static char *full_path_to_log_file = NULL;
  if (full_path_to_log_file == NULL) {
      full_path_to_log_file = (char *) malloc(sizeof(char) * MAX_FILE_FULL_PATH);
      strcpy(full_path_to_log_file, TERMSHIP_PATH_STR);
      strcat(full_path_to_log_file, LOG_FILE_NAME);
      printf("log file is at %s\n", full_path_to_log_file);
  }
  if (thelogfile == NULL) {
    thelogfile = fopen(full_path_to_log_file, "a+");
  }
  fprintf(thelogfile, "%s", st);
  fflush(thelogfile);
}
