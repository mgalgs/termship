/**
 * This file contains the data structures for the game board
 */

#ifndef GAMEPIECES_H
#define GAMEPIECES_H

#include <stdint.h>

#define CLIENT_MODE 1
#define SERVER_MODE 0

#define NUM_SHIPS 5
#define MAX_SHIP_SIZE 5

#define BOARD_SIZE 10


typedef struct Ship_
{
  char *name;
  uint8_t size;
  uint8_t sunk; //0 for afloat, 1 for sunk
  uint8_t x;
  uint8_t y;
  uint8_t direction; //0 for horizontal, 1 for vertical
  uint8_t slots[MAX_SHIP_SIZE]; //0 for intact, 1 for hit
} Ship;

typedef struct Board_
{
  Ship *ships;
  int unsunk_cnt; //number of unsunk ships
  int hits[BOARD_SIZE][BOARD_SIZE];
  int mis[BOARD_SIZE][BOARD_SIZE];
    
} Board;


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

void create_grid(char grid[BOARD_SIZE][BOARD_SIZE], Ship[]);

/**
 * Sets a slot on a ship as hit
 */
void setAsHit(const int, const int);
bool is_there_a_ship_here(Ship [], int, int);
Ship getShipById(const int);
void printShips();
void initShips();
int randNum(const int, const int);

/* checks the placement of a ship */
bool valid_placement(Ship *);

#endif

