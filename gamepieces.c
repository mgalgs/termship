/* -*- c-basic-offset: 2 -*- */
#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <time.h>
#include <unistd.h>
#include "gamepieces.h"
#include "screen.h"
#include "Btypes.h"
#include "log.h"


extern  WINDOW *player_win;
extern  WINDOW *opponent_win; 
extern  WINDOW *status_win;

//all "slots" 0:
#define HEALTHY {0,0,0,0,0}

//the ships: (this is globaly visible)
Ship Shipset[] = {
  {"Aircraft Carrier",5,0,0,-1,0,HEALTHY},
  {"Battleship",4,0,0,-1,0,HEALTHY},
  {"Cruiser",3,0,0,-1,0,HEALTHY},
  {"Submarine",3,0,0,-1,0,HEALTHY},
  {"Destroyer",2,0,0,-1,0,HEALTHY}
};


Ship PeerShipset[] = {
  {"Aircraft Carrier",5,0,0,-1,0,HEALTHY},
  {"Battleship",4,0,0,-1,0,HEALTHY},
  {"Cruiser",3,0,0,-1,0,HEALTHY},
  {"Submarine",3,0,0,-1,0,HEALTHY},
  {"Destroyer",2,0,0,-1,0,HEALTHY}
};


/**
 * This function creats a grid of ships from Shipset
 */
void create_grid(char grid[BOARD_SIZE][BOARD_SIZE], Ship ships[])
{
  int x, y, dir, size, i,j;
  for (i = 0; i< BOARD_SIZE; i++) {
    for(j = 0; j< BOARD_SIZE; j++) {
      grid[i][j]='_';
    }
  }

  for (i=0; i<NUM_SHIPS; i++) {
    x = ships[i].x;
    y = ships[i].y;
    dir = ships[i].direction;
    size = ships[i].size;
    if(dir) { /*vertical*/
      for (j=0;j<size; j++) {
	if (grid[y+j][x] != '_')
	  grid[y+j][x]='x';
	else
	  grid[y+j][x]='*';
      }
    } else {
      for (j=0;j<size; j++) {
	if (grid[y][x+j] != '_')
	  grid[y][x+j]='x';
	else
	  grid[y][x+j]='*';
      }
    } /* eo if(dir) */
  }   /* eo for each ship */
}



/**
 * Function to generate random numbers from low to high
 */
int randNum(const int high, const int low) {
  srand((unsigned)time(0));
  return ((rand() % (high - low +1)) + low);
}


/**
 * Set given ship as hit on given slot.
 * Also takes care of setting the ship as sunk.
 */
void setAsHit(int ship, int slot)
{
  int i;
  int sunk = 1;
  int x, y,dir;
    
  x = Shipset[ship].x;
  y = Shipset[ship].y;
  dir = Shipset[ship].direction;
  if (dir) y += slot;
  else x += slot;
 
  Shipset[ship].slots[slot] = 1; //set slot as hit
  place_hit_or_mis(opponent_win, -1, x, y, true); 
  //set as sunk if relevant
  for (i=0; i<Shipset[ship].size; i++) {
    if (Shipset[ship].slots[i] == 0) {/*not hit*/
      sunk = 0;
      break; //we found 1 good slot, so the ship isn't sunk
    }
  }
  Shipset[ship].sunk = sunk;
}

/**
 * Sets all ships to being healthy and place them on a random map.
 */
void initShips()
{
  int i,j;

  //choose a "map"
  int ch;
  int current_editing_ship = 0;
  int origx, origy, origdir;
  Ship *curship;
  bool done = false;
  int scratch=0;
  char msg[50];
  char extramsg[50] = "";
  char players_grid[BOARD_SIZE][BOARD_SIZE];

  /* get rid of cursor: */
  curs_set(0);

  while (!done) {
    scratch++;
    curship = &(Shipset[current_editing_ship]);
    origx = curship->x;
    origy = curship->y;
    origdir = curship->direction;

    display_boards();
    mvprintw(0,1,"Use arrows to place ship, 'r' to rotate, space to select another ship.");
    mvprintw(1,1,"Press enter to finish. %s", extramsg);
    mvprintw(2,8, "Placing your \"%s\"", curship->name);
    echo();
    ch = getch();

    strcpy(extramsg, "");
    switch (ch) {
    case KEY_DOWN:
      curship->y++;
      break;
    case KEY_UP:
      curship->y--;
      break;
    case KEY_LEFT:
      curship->x--;
      break;
    case KEY_RIGHT:
      curship->x++;
      break;
    case ' ':
      current_editing_ship = current_editing_ship == NUM_SHIPS-1 ? 0 : current_editing_ship+1;
      break;
    case 'r':
      curship->direction = !curship->direction;
      break;
    case 10:
    case KEY_ENTER:
      write_to_log("maybe continuing...\n");
      done = true;
      /* check to see that all ships have been place: */
      for (i=0; i<NUM_SHIPS; i++) {
	if (!valid_placement(&(Shipset[i]))) {
	  sprintf(msg, "%s hasn't been placed yet", Shipset[i].name);
	  show_message_box(msg);
	  sleep(2);
	  done = false;
	  break;
	}
      }
      if (!done) break;
      create_grid(players_grid, Shipset);
      /* check for overlaps */
      for (i=0; i<BOARD_SIZE; ++i) {
	for (j=0; j<BOARD_SIZE; ++j) {
	  if (players_grid[i][j] != '*' && players_grid[i][j] != '_' ) {
	    sprintf(msg, "Can't continue, there's an overlap at: %d,%d\n", i, j);
	    write_to_log(msg);
	    strcpy(msg, "There are overlapping pieces");
	    done = false;
	    break;
	  }
	}
      }
      if (!done) {
	sprintf(extramsg,"[Can't continue: %s]", msg);
	show_message_box(extramsg);
	sleep(2);
	write_to_log("Can't continue! Not all ships in valid locations.\n");
      }
      break;
    } /* eo switch */
    if (!valid_placement(curship)) {
      /* undo changes: */
      write_to_log("Undoing changes because of bad placement...\n");
      curship->x = origx;
      curship->y = origy;
      curship->direction = origdir;
    }
  } /* eo while(!done) */

}

bool is_there_a_ship_here(Ship ships[], int x, int y)
{
  //check for a hit:
  for (int i=0; i<NUM_SHIPS; i++) { //for each ship
    for (int j=0; j<ships[i].size; j++) { //for each "slot" on the ship
      if (ships[i].direction == 1) { /*ship placed vertically*/
	if ( (ships[i].x == x) && (ships[i].y+j == y) ) {
	  return true;
	}
      } else { /*ship placed horizontally*/
	if ( (ships[i].x+j == x) && (ships[i].y == y) ) {
	  return true;
	}
      }
    }
  }
  return false;
}

bool valid_placement(Ship *ship)
{
  char msg[50];
  write_to_log("ships ahoy! Checking ship placement...");
  if (ship->x < 0 || ship->y < 0 || ship->x > BOARD_SIZE-1 || ship->y > BOARD_SIZE-1) {
    sprintf(msg, "Bad placement: x: %d, y: %d\n", ship->x, ship->y);
    write_to_log(msg);
    return false;
  }
  if (ship->direction) {	/* vertical */
    if (ship->y + ship->size > BOARD_SIZE) return false;
  } else {		/* horizontal */
    if (ship->x + ship->size > BOARD_SIZE) return false;
  }

  /* TODO: check for overlapping ships! */

  sprintf(msg, "Good placement: x: %d, y: %d\n", ship->x, ship->y);
  write_to_log(msg);
  return true;
}

/**
 * returns ship with passed in id
 */
Ship getShipById(const int id)
{
  return Shipset[id-1];
}


/**
 * Debugging routine just to print the ships
 */
void printShips()
{
  int i,j;
  scr_dump(".Bstatesave");
  clear();
  refresh();
  for (i=0; i<NUM_SHIPS; i++) {
    printw("%s: ", Shipset[i].name);
    for (j=0; j<Shipset[i].size; j++)
      printw("%d",Shipset[i].slots[j]);
    printw("\n");
  }
  printw("\nPress any key to continue\n");
  refresh();
  getch();
  scr_restore(".Bstatesave");
}

