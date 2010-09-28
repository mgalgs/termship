/**
 * These are the types needed for our Battleship protocol
 */

#ifndef BTYPES_H
#define BTYPES_H

#define MAX_CODE 10


//The messages:
typedef enum
{
    BFIRE,
    BHIT,
    BMISS
} MESSAGE;

//The HIT_CODEs:
#define SUNK "0"
#define GAME_OVER "1"

typedef struct BMesg_
{
    MESSAGE msg;
    /**
     * code:
     * Can be one of the following:
     *   if msg == BHIT
     *     - NULL
     *         Indicates a hit but not a sink nor game over.
     *     - SUNK (i.e. "0")
     *         Indicates that one of your ships was sunk.
     *     - GAME_OVER (i.e. "1")
     *         Indicates that all of your ships are sunk and the game is over.
     *
     *   if msg == BFIRE
     *     - "x,y"
     *         Indicates the x and y coordinates of where you are firing.
     */
    char code[MAX_CODE];
} BMesg;


BMesg *CreateBMesg(int, int, int);
BMesg *CreateEmptyBMesg();

#endif
