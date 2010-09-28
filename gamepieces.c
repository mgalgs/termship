#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include "gamepieces.h"
#include "screen.h"


extern  WINDOW *player_win;
extern  WINDOW *opponent_win; 
extern  WINDOW *status_win;

//all "slots" 0:
#define HEALTHY {0,0,0,0,0}

#define NUM_MAPS 6

//defined the "maps"
Mapping Mapset[NUM_MAPS][NUM_SHIPS] = {
    {
        3,1,1,
        4,3,0,
        2,7,0,
        6,5,1,
        0,2,1
    },{
        8,1,1,
        4,2,0,
        1,8,0,
        1,1,1,
        6,6,0
    },{
        5,4,1,
        3,4,1,
        0,4,1,
        6,9,0,
        0,9,0
    },{
        7,3,1,
        0,7,0,
        2,0,0,
        1,2,0,
        0,5,1
    },{
        1,9,0,
        2,4,1,
        4,7,0,
        9,0,1,
        3,1,0
    },{
        9,5,1,
        1,5,1,
        3,1,0,
        7,0,1,
        5,4,0
    }
};

//the ships: (this is globaly visible)
Ship Shipset[] = {
    "Aircraft Carrier",5,0,0,0,0,HEALTHY,
    "Battleship",4,0,0,0,0,HEALTHY,
    "Cruiser",3,0,0,0,0,HEALTHY,
    "Submarine",3,0,0,0,0,HEALTHY,
    "Destroyer",2,0,0,0,0,HEALTHY
};

/* Ship Shipset[] = { */
/*     "Aircraft Carrier",5,0,3,1,1,{0,0,0,0,0}, */
/*     "Battleship",4,0,4,3,0,{0,0,0,0,0}, */
/*     "Cruiser",3,0,2,7,0,{0,0,0,0,0}, */
/*     "Submarine",3,0,6,5,1,{0,0,0,0,0}, */
/*     "Destroyer",2,0,0,2,1,{0,0,0,0,0} */
/* }; */




/**
 * This function creats a grid of ships from Shipset
 */


void create_grid(char grid[BOARD_SIZE][BOARD_SIZE]) {
  int x, y, dir, size, i,j;
  for (i = 0; i< BOARD_SIZE; i++) {
    for(j = 0; j< BOARD_SIZE; j++) {
      grid[i][j]='_';
    }
  }
  
   for (i=0;i< NUM_SHIPS; i++) {
    x = Shipset[i].x;
    y = Shipset[i].y;
    dir = Shipset[i].direction;
    size = Shipset[i].size;
    //grid[x][y] = '*';
    if(dir) { /*vertical*/
      for (j=0;j<size; j++) {
	grid[y+j][x]='*';
      }
    }
    else {
      for (j=0;j<size; j++) {
	grid[y][x+j]='*';
      }
    }
  }
}



/**
 * Function to generate random numbers from low to high
 */
int randNum(const int high, const int low) {
  srand((unsigned)time(0));
  return ((rand() % (high - low +1)) + low);
}

void place_ship(){
  int direction, x, y, i;
  for (i=0;i<5;i++) {
    direction = randNum(0,1);
    x = randNum(0,BOARD_SIZE);
    y = randNum(0,BOARD_SIZE);
    Shipset[i].direction = direction;
    Shipset[i].x = x;
    Shipset[i].y = y;
  }
}

/**
 * Helper function to create a ship by the name of name
 */
Ship *create_ship(const char *name, const int size)
{
    Ship *new_ship;
    if ( (new_ship = (Ship *) malloc(sizeof(Ship))) == NULL ) {/*malloc error*/
        perror("malloc error");
        exit(EXIT_FAILURE);
    }
    if ( (new_ship->name = (char *) malloc(strlen(name))) == NULL ) {
        perror("malloc error");
        exit(EXIT_FAILURE);
    }
    strcpy(new_ship->name, name); 
    //Randomly place the ships
    srand((unsigned)time(0));
    int direction = randNum(0,1); //Random number between 0 and 1
    int x = randNum(0,BOARD_SIZE); //x location of ship
    int y = randNum(0,BOARD_SIZE); //y location 
    new_ship->x = new_ship->y = new_ship->sunk = new_ship->direction = 0;
    new_ship->size = size;
    return new_ship;
}

/**
 * Helper function to create and return a board with ships initialized.
 */
Board *create_board()
{
    Board *new_board;
    if ( (new_board = (Board *) malloc(sizeof(Board))) == NULL ) {/*malloc error*/
        perror("malloc error");
        exit(EXIT_FAILURE);
    }
    new_board->unsunk_cnt = 5;
    if ( (new_board->ships = (Ship *) malloc(sizeof(Shipset))) == NULL ) {
        perror("malloc error");
        exit(EXIT_FAILURE);
    }
    new_board->ships = Shipset;
}

/**
 * Creates and returns an initialized player
 */
Player *create_player(const char *name, const int user_mode)
{
    Player *new_player;
    if ( (new_player = (Player *) malloc(sizeof(Player))) == NULL ) { /*malloc error*/
        perror("malloc error");
        exit(EXIT_FAILURE);
    }
    new_player->board = create_board();
    new_player->name = "John";
    new_player->user_mode = user_mode;
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
    place_hit_or_mis(opponent_win, -1, x, y); 
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
    //    int MAP = randNum(0,NUM_MAPS-1);
    int MAP;

    for(;;) { //they must choose a valid map
        printw("\nWhich map would you like to use? (1-%d) ", NUM_MAPS);
        scanw("%d", &MAP);
        if (MAP < 1 || MAP > NUM_MAPS) {
            printw("Invalid map choice. Press a key to try again\n");
            refresh();
            getch();
            clear();
            continue;
        }
        break;
    }

    MAP--; //make it an index

    for (i=0; i<NUM_SHIPS; i++) {
        //place according to randomly chosen map:
        Shipset[i].x = Mapset[MAP][i].x;
        Shipset[i].y = Mapset[MAP][i].y;
        Shipset[i].direction = Mapset[MAP][i].direction;
        printw("\nSetting up %s:\tx: %d\ty: %d\tdir: %d\n", Shipset[i].name, Mapset[MAP][i].x, Mapset[MAP][i].y, Mapset[MAP][i].direction);

        //set as healthy:
        Shipset[i].sunk = 0;
        for (j=0; j<Shipset[i].size; j++)
            Shipset[i].slots[j] = 0;
    }

    printw("press key to continue...\n");
    refresh();
    getch();
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

