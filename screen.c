/**
 * This file contains all the ui routines.
 */


#include <ncurses.h>
#include "gamepieces.h"
#include "connection.h"
#include "screen.h"

#define MAX_NAME 100
#define CROSSHAIR '+'

extern Ship Shipset[];
extern Ship Their_Shipset[];
char name[MAX_NAME];
player_pos the_player_pos = {.x=0, .y=0};

bool their_shots[BOARD_SIZE][BOARD_SIZE];
bool my_shots[BOARD_SIZE][BOARD_SIZE];
int user_mode;

WINDOW *player_win = NULL;
WINDOW *opponent_win = NULL;
WINDOW *status_win = NULL;

void place_hit_or_mis(WINDOW * win,int mesg, int x, int y) {
  //-2game -1 hit sink 1hit  0miss
  //deal with hits first

  if ((mesg == -2) || (mesg == -1) || (mesg == 1)) {
    wattron(win,COLOR_PAIR(4));
    mvwprintw(win, y+2, x*2+3,"#");
    wattroff(win,COLOR_PAIR(4));
    wrefresh(win);
  }

  else { // miss
    wattron(win,COLOR_PAIR(3));
    mvwprintw(win, y+2, x*2+3,"@");
    wattroff(win,COLOR_PAIR(3));
    wrefresh(win);
  }

}


/**
 * Documentation here
 */
void main_menu()
{
  Player *player;
  /* some testing modes: */
#ifdef TEST_SHIPS
  initShips(-1);
  return;
#endif

  title_screen();
  printw("Enter your name: ");
  getstr(name);
  printw("\nEnter 0 for server mode, 1 for client mode: ");
  scanw("%d", &user_mode);
  player = create_player(name, user_mode);

  printw("\ninitializing the game...\n");
  refresh();
  init_game(user_mode);
}


void return_cords(int * x, int * y) {
  int ch;
  char msg[50];
  write_to_log("about to enter forever loop...\n");
  for(;;) {
    ch = getch();
    switch(ch) {
    case KEY_LEFT:
      if (the_player_pos.x > 0)
	the_player_pos.x--;
      break;
    case KEY_RIGHT:
      if (the_player_pos.x < BOARD_SIZE-1 )
	the_player_pos.x++;
      break;
    case KEY_UP:
      if (the_player_pos.y > 0)
	the_player_pos.y--;
      break;
    case KEY_DOWN:
      if (the_player_pos.y < BOARD_SIZE-1 )
	the_player_pos.y++;
      break;
    case 10:
    case KEY_ENTER:
      *x = the_player_pos.x;
      *y = the_player_pos.y;
      return;
    default:
      sprintf(msg, "got %d from getch...weird...\n", (int)ch);
      write_to_log(msg);
      break;
    } /* eo switch */
    display_boards(true);
    write_to_log("looping around forever getch loop...\n");
  }
}

void display_boards(bool draw_crosshair)
{
  int startx, starty, width, height, i, j;
  int stat_width, stat_height;
  int xoffset = 3;
  int yoffset = 2;

  char players_grid[BOARD_SIZE][BOARD_SIZE];

  int x,y,res;
  int f, h = 0;
  char t;
  char msg[50];
  stat_height= 5;
  stat_width=50;

  write_to_log("refreshing board (display_boards)\n");

  cbreak();
  noecho();                       

  keypad(stdscr, TRUE);            
  height = 3+BOARD_SIZE; 
  width = 14+BOARD_SIZE; 
  starty = (LINES - height) / 2;  
  startx = (COLS - width) / 2;    
  clear();
  refresh(); 

  if (player_win == NULL)
    player_win = newwin(height, width, starty, startx+20); 
  box(player_win, 0, 0);
  wrefresh(player_win);

  if (opponent_win == NULL)
    opponent_win = newwin(height, width, starty, startx-20);
  box(opponent_win, 0, 0);
  wrefresh(opponent_win);

  if (status_win == NULL)
    status_win = newwin(stat_height, stat_width, starty+13, startx-20);

  create_grid(players_grid);

  start_color();
  init_pair(2, COLOR_YELLOW, COLOR_BLACK);
  init_pair(3, COLOR_BLUE, COLOR_BLACK);
  init_pair(4, COLOR_RED, COLOR_BLACK);
  clear();
  refresh();

  mvprintw(starty-1, startx-15, "Your ships");
  mvwprintw(opponent_win, 1,1,"  A B C D E F G H I P");
  wattron(opponent_win,COLOR_PAIR(2));
  mvwprintw(opponent_win, 1, 1, " ");
  wattroff(opponent_win,COLOR_PAIR(2));

  for (h = 0; h<BOARD_SIZE; h++) {
    mvwprintw(opponent_win, 2+h, 1,"%d|", h);
    for (f =0; f<BOARD_SIZE; f++) {
      t = players_grid[h][f];
      if (t == '*') {
      	wattron(opponent_win,COLOR_PAIR(2));
      	mvwprintw(opponent_win, 2+h,3+f*2, "%c", t);
      	wattroff(opponent_win,COLOR_PAIR(2));
      }
      else {
      	mvwprintw(opponent_win, 2+h,3+f*2, "%c", t);
      }
      /* mvwprintw(opponent_win, 2+h, 3+f*2, "_|"); */
      mvwprintw(opponent_win, 2+h,4+f*2, "|");
    }
  }
  wrefresh(opponent_win);

  mvprintw(starty-1, startx+21, "Hit or miss ships");
  mvwprintw(player_win,1,1,"  A B C D E F G H I J");

  for (i=0;i<BOARD_SIZE;i++) {
    mvwprintw(player_win,i+2,1,"%d|_|_|_|_|_|_|_|_|_|_|",i+0);
  }

  /* now that both boards are drawn, paint on the shots: */
  for (x=0; x<BOARD_SIZE; ++x) {
    for (y=0; y<BOARD_SIZE; ++y) {
      if (my_shots[x][y]) {
	wattron(player_win,COLOR_PAIR(3));
	mvwprintw(player_win, yoffset + y, xoffset + x*2,"^");
	wattroff(player_win,COLOR_PAIR(3));
      }
      if (their_shots[x][y]) {
	wattron(opponent_win,COLOR_PAIR(3));
	mvwprintw(opponent_win, yoffset + y, xoffset + x*2,"^");
	wattroff(opponent_win,COLOR_PAIR(3));
      }
    }
  }

  /* for (i=0; i<BOARD_SIZE; ++i) { */
  /*   for (j=0; j<BOARD_SIZE; ++j) { */
  /*     if (my_shots[i][j]) { */
  /* 	sprintf(msg, "my shots at: (%d,%d)\n", i, j); */
  /* 	write_to_log(msg); */
  /*     } */
  /*   } */
  /* } */
  /* for (i=0; i<BOARD_SIZE; ++i) { */
  /*   for (j=0; j<BOARD_SIZE; ++j) { */
  /*     if (their_shots[i][j]) { */
  /* 	sprintf(msg, "their shots at: (%d,%d)\n", i, j); */
  /* 	write_to_log(msg); */
  /*     } */
  /*   } */
  /* } */


  /* draw the crosshair: */
  if (draw_crosshair) {
    mvwprintw(player_win, yoffset + the_player_pos.y, xoffset + (the_player_pos.x*2), "%c", CROSSHAIR);
    /* curs_set(1); // Set cursor visible */
    /* sprintf(msg, "moving cursors to %d,%d in player_win\n", yoffset + the_player_pos.y, xoffset + (the_player_pos.x*2)); */
    /* write_to_log(msg); */
    /* wmove(player_win, yoffset + the_player_pos.y, xoffset + (the_player_pos.x*2)); */
    /* wrefresh(player_win); */
  } else {
    curs_set(0); // Set cursor invisible
  }

  /* now look at our set of ships, if they have a shot on a ship, mark it as hit */
  for (i=0; i<NUM_SHIPS; i++) {
    x = Shipset[i].x;
    y = Shipset[i].y;
    for (j=0; j<Shipset[i].size; ++j) {
      if (Shipset[i].direction == 0) { /* horizontal */
	if (their_shots[x+j][y]) {
	  wattron(opponent_win,COLOR_PAIR(4));
	  mvwprintw(opponent_win, yoffset + y, xoffset + (x+j)*2, "!"); /* stupid axes names mixing... */
	  wattroff(opponent_win,COLOR_PAIR(4));
	}
      } else { /* vertical */
	if (their_shots[x][y+j]) {
	  wattron(opponent_win,COLOR_PAIR(4));
	  mvwprintw(opponent_win, yoffset + y+j, xoffset + x*2, "!"); /* stupid axes names mixing... */
	  wattroff(opponent_win,COLOR_PAIR(4));
	}
      }	/* eo if direction */
    } /* eo foreach slot */
  }   /* eo foreach ship */

  /* now look at their set of ships, if we have a shot on a ship, mark it as hit */
  for (i=0; i<NUM_SHIPS; i++) {
    x = Their_Shipset[i].x;
    y = Their_Shipset[i].y;
    for (j=0; j<Their_Shipset[i].size; ++j) {
      if (Their_Shipset[i].direction == 0) { /* horizontal */
	if (my_shots[x+j][y]) {
	  wattron(player_win,COLOR_PAIR(4));
	  mvwprintw(player_win, yoffset + y, xoffset + (x+j)*2, "!"); /* stupid axes names mixing... */
	  wattroff(player_win,COLOR_PAIR(4));
	}
      } else { /* vertical */
	if (my_shots[x][y+j]) {
	  wattron(player_win,COLOR_PAIR(4));
	  mvwprintw(player_win, yoffset + y+j, xoffset + x*2, "!"); /* stupid axes names mixing... */
	  wattroff(player_win,COLOR_PAIR(4));
	}
      }	/* eo if direction */
    } /* eo foreach slot */
  }   /* eo foreach ship */



  refresh();
  wrefresh(player_win);
  wrefresh(opponent_win);
  wrefresh(status_win); 

  attroff(A_UNDERLINE);

}

/**
 * This function gets called from within init_game
 * It is called repeatedly if we are the server, else
 * it is just called once.
 */
void do_gameplay(const int sock, int fire)
{
  int x,y,height,width,res,win_status=0;

  initShips(sock);
  display_boards(true);


  Ship sh;
  do {
    if (fire == 1) { /*you're the attacker*/
      mvwprintw(status_win,1,1,"It's your turn!                    ");
      wrefresh(status_win);

      fire = 0;
      return_cords(&x, &y);
      res = do_fire(sock, x, y);
      my_shots[x][y] = true;
      /* place_hit_or_mis(player_win,res, x, y); */
      switch (res) {
      case 0:
	mvwprintw(status_win,2,1,"Missed!                                    ");
	wrefresh(status_win);
	break;
      case 1:
	mvwprintw(status_win,2,1,"You hit them!                              ");
	wrefresh(status_win);
	break;
      case -1:
	mvwprintw(status_win,2,1,"You sunk them!                             ");
	wrefresh(status_win);
	break;
      case -2:
	win_status = 1;
	mvwprintw(status_win,2,1,"Game over!                        ");
	wrefresh(status_win);
	fire = -1;
	break;
      }

    } else { /*you're the defender*/
      keypad(stdscr, FALSE);
      /* curs_set(0); // Set cursor invisible */
      mvwprintw(status_win,1,1,"Waiting for other player to fire...");
      wrefresh(status_win);
      res = do_receive(sock);
      refresh();
      if (res == 0) {
	//wclear(status_win);
	mvwprintw(status_win,2,1,"They missed!                                ");
	//mvwprintw(status_win,5,1,"It's your turn!");
	wrefresh(status_win);
      } else if (res < 0) { //negative res indicates sunken ship
	sh = getShipById(-1*res);
	//wclear(status_win);
	mvwprintw(status_win,2,1,"They sunk your %s!               ", sh.name);
	//mvwprintw(status_win,5,1,"It's your turn!");
	wrefresh(status_win);
      } else if (res==100);//do nothing...the game is over
      else {
	sh = getShipById(res);
	//wclear(status_win);
	mvwprintw(status_win,2,1,"They hit your %s!                ", sh.name);
	//mvwprintw(status_win,5,1,"It's your turn!");
	wrefresh(status_win);
      }
      mvwprintw(status_win,1,1,"It's your turn!                               ");
      fire = (check_game_over() == 1) ? -1 : 1;
      refresh();
    }
  } while(fire > -1);

  show_end_screen(win_status);
}


void title_screen()
{
  printw(\
	 "                                     # #  ( )\n" \
	 "                                  ___#_#___|__\n" \
	 "                              _  |____________|  _\n" \
	 "                       _=====| | |            | | |==== _\n" \
	 "                 =====| |.---------------------------. | |====\n" \
	 "   <--------------------'   .  .  .  .  .  .  .  .   '--------------/\n" \
	 "     \\                                                             /\n" \
	 "      \\_______________________________________________WWS_________/\n" \
	 "  wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n" \
	 "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww\n" \
	 "   wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww \n");
  printw("\n\nPress any key to continue...\n");
  getch();
}

void show_end_screen(int win_status)
{
  clear();
  refresh();
  printw("Game over, %s, you %s!\nthanks for playing!!!\n\npress any key to continue...\n", name, (win_status) ? "win" : "lose");
  refresh();
  getch();
}
