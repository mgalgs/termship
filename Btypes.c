#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "Btypes.h"
#include "screen.h"

/**
 * Returns 1 if passed in int is a valid message (BMesg.msg), else returns 0
 */
int validMesg(int MESS)
{
  return ((MESS == BFIRE) || (MESS == BHIT) || (MESS == BMISS));
}

/**
 * Returns 1 if passed in char* is a valid code (BMesg.code), else returns 0
 */
int validCode(char *code)
{
  return 1;
}


/**
 * Creates and initializes a BMesg with the passed-in parameters.
 */
BMesg *CreateBMesg(int MESS, int x, int y)
{
  BMesg *newBMesg;
  //allocate memory:
  newBMesg = CreateEmptyBMesg();
  //do some validation:
  if (!validMesg(MESS)) {
    cleanup_ncurses();
    perror("Invalide message passed to CreateBMesg");
    exit(EXIT_FAILURE);
  }
  newBMesg->msg = MESS; //should be one of BFIRE, BHIT, BMISS (defined in Btypes.h)
  sprintf(newBMesg->code, "%d,%d\0", x, y);
  return newBMesg;
}

/**
 * Creates and returns an empty BMesg
 */
BMesg *CreateEmptyBMesg()
{
  BMesg *newBMesg;
  int i;
  if ((newBMesg = (BMesg *) malloc(sizeof(BMesg))) == NULL) { /*malloc error*/
    perror("malloc error");
    exit(EXIT_FAILURE);
  }
  //initialize the code:
  for (i=0; i<MAX_CODE; i++)
    newBMesg->code[i]=0;
  return newBMesg;
}
