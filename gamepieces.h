/**
 * This file contains the data structures for the game board
 */

#ifndef GAMEPIECES_H
#define GAMEPIECES_H


#define CLIENT_MODE 1
#define SERVER_MODE 0

#define NUM_SHIPS 5
#define MAX_SHIP_SIZE 5

#define BOARD_SIZE 10


typedef struct Ship_
{
  char *name;
  int size;
  int sunk; //0 for afloat, 1 for sunk
  int x;
  int y;
  int direction; //0 for horizontal, 1 for vertical
  int slots[MAX_SHIP_SIZE]; //0 for intact, 1 for hit
} Ship;

typedef struct Board_
{
  Ship *ships;
  int unsunk_cnt; //number of unsunk ships
  int hits[BOARD_SIZE][BOARD_SIZE];
  int mis[BOARD_SIZE][BOARD_SIZE];
    
} Board;

typedef struct Mapping_
{
  int x;
  int y;
  int direction;
} Mapping;


typedef struct Player_
{
  Board *board;
  char *name;
  int user_mode; //either SERVER_MODE or CLIENT_MODE
} Player;


/**
 * Creates and returns an initialized player
 */
Player *create_player(const char *, const int);

void create_grid(char grid[BOARD_SIZE][BOARD_SIZE]);

/**
 * Sets a slot on a ship as hit
 */
void setAsHit(const int, const int);
Ship getShipById(const int);
void printShips();
void initShips();
int randNum(const int, const int);

/* checks the placement of a ship */
bool valid_placement(Ship *);

#endif

