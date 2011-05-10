/**
 * This file contains all the ui routines.
 */


#include <ncurses.h>
#include <menu.h>
#include <panel.h>
#include <string.h>
#include <stdlib.h>
#include "gamepieces.h"
#include "connection.h"
#include "screen.h"
#include "log.h"
#include "common.h"

#define MAX_NAME 100

extern Ship Shipset[];
char global_user_name[MAX_NAME];

WINDOW *player_win;
WINDOW *opponent_win;
WINDOW *status_win;

void place_hit_or_mis(WINDOW * win,int mesg, int x, int y) {
  //-2game -1 hit sink 1hit  0miss
  //deal with hits first

  if ((mesg == -2) || (mesg == -1) || (mesg == 1)) {
    wattron(win,COLOR_PAIR(4));
    mvwprintw(win, y+2, x*2+3,"#");
    wattroff(win,COLOR_PAIR(4));
    wrefresh(win);
  } else { // miss
    wattron(win,COLOR_PAIR(3));
    mvwprintw(win, y+2, x*2+3,"@");
    wattroff(win,COLOR_PAIR(3));
    wrefresh(win);
  }
}

/**
 * Documentation here
 */
void show_battlefield(const Board *board)
{
  printw("Hello, world!");
  refresh();
  getch();
  return;
}

/**
 * Documentation here
 */
void main_menu()
{
  int user_mode;
  Player *player;
  WINDOW *panel_win;
  PANEL *the_panel;
  int panel_height=6,panel_width=50;
  char msg[200];


  MENU *my_menu;
  /* Order here doesn't matter. Name (the first item in each of these
     "pairs") does. If you change the name, change the selection
     handling code towards the end of this function. */
  char *my_choices[] = {
    "Create","(Create new termship game)",
    "Join", "(Join existing termship game on network)",
    "Exit", "",
    (char *)NULL, (char *)NULL
  };
  ITEM **my_items;
  int n_choices = ARRAY_SIZE(my_choices)/2;

  /* some testing modes: */
#ifdef TEST_SHIPS
  initShips();
  return;
#endif

  keypad(stdscr, TRUE);
  curs_set(0); // make cursor invisible
  noecho();
  title_screen();

  /* create the panel: */
  panel_win = newwin(panel_height,
                     panel_width,
                     (LINES-panel_height)/2,
                     (COLS-panel_width)/2);
  box(panel_win, 0, 0);
  print_in_middle(panel_win, 2, 0, panel_width,
                  "Welcome to termship!", COLOR_PAIR(7));
  print_in_middle(panel_win, 3, 0, panel_width,
                  "(Press any key)", COLOR_PAIR(7));
  /* mvwprintw(panel_win, 1, 1, "Test me"); */
  refresh();
  the_panel = new_panel(panel_win);
  top_panel(the_panel);
  update_panels();
  doupdate();
  getch();

  del_panel(the_panel);
  update_panels();
  delwin(panel_win);
  doupdate();

  clear();

  my_items = (ITEM **)calloc(n_choices,sizeof(ITEM *));
  for (int i=0; my_choices[i*2]!=NULL; ++i) {
    /* i*2 since we're iterating in pairs */
    my_items[i] = new_item(my_choices[(i*2)], my_choices[(i*2)+1]);
  }

  my_menu = new_menu(my_items);
  set_menu_mark(my_menu, "   * ");
  post_menu(my_menu);
  refresh();

  int c;
  while ((c = getch()) != 10) {
    switch (c) {
    case KEY_DOWN:
      menu_driver(my_menu, REQ_DOWN_ITEM);
      break;
    case KEY_UP:
      menu_driver(my_menu, REQ_UP_ITEM);
      break;
    }
  }
  ITEM *cur = current_item(my_menu);
  const char *selected_name = item_name(cur);
  
  /* delete the menu, free resources */
  for (int i=0; i<n_choices-1; ++i) {
    free_item(my_items[i]);
  }
  unpost_menu(my_menu);
  free_menu(my_menu);

  if (0==strcmp(selected_name, "Create")) {
    user_mode = SERVER_MODE;
    strncpy(global_user_name, get_text_string_from_centered_panel("Enter your name"), MAX_NAME);
    player = create_player(global_user_name, user_mode);
    init_game(user_mode);
  } else if (0==strcmp(selected_name, "Join")) {
    user_mode = CLIENT_MODE;
    strncpy(global_user_name, get_text_string_from_centered_panel("Enter your name"), MAX_NAME);
    player = create_player(global_user_name, user_mode);
    init_game(user_mode);
  }

  return;
}

void return_cords(int * x, int * y) {
  struct player_pos_ player_pos;
  int startx, starty,height, width;
  
  player_pos.x = 0;
  player_pos.y = 0;

  keypad(stdscr, TRUE);
  curs_set(1); // make cursor visible
  height = 3+BOARD_SIZE; 
  width = 14+BOARD_SIZE; 
  starty = (LINES - height) / 2;  
  startx = (COLS - width) / 2;   

  int playerx = 3+startx+20;
  int playery = 2+starty;
  move(playery,playerx);

  int ch;
  while((ch = getch())) 
    {      
      switch(ch) 
        {       case KEY_LEFT:
            if (playerx > 3+startx+20) {
              playerx -=2;
              player_pos.x--;
              move(playery, playerx);
              break;
            }
            break;
        case KEY_RIGHT:
          if (playerx < -3+startx+20+width) {
            playerx +=2;
            player_pos.x++;
            move(playery, playerx);
            break; 
          }
          break;
        case KEY_UP:
          if (playery > 2+starty) {
            --playery;
            --player_pos.y;
            move(playery, playerx);
            break;
          }
          break;
        case KEY_DOWN:
          if (playery < starty+height-2) {
            ++playery;
            ++player_pos.y;
            move(playery, playerx);
            break;  
          }
          break;
        case 10:
          *x = player_pos.x;
          *y = player_pos.y;
          return;
          break;
          break;  
                          
        case KEY_ENTER:
          *x = player_pos.x;
          *y = player_pos.y;
          return;
          break;
          break;
                
        }
    } 
}

void display_boards(void)
{
  int startx, starty, width, height; 
  int stat_width, stat_height;

  char players_grid[BOARD_SIZE][BOARD_SIZE];

  int x,y,res;
  int f, h = 0;
  char t;
  int i;
  stat_height= 5;
  stat_width=50;

  keypad(stdscr, TRUE);            
  height = 3+BOARD_SIZE; 
  width = 14+BOARD_SIZE; 
  starty = (LINES - height) / 2;  
  startx = (COLS - width) / 2;    
  clear();
  refresh(); 

  player_win = newwin(height, width, starty, startx+20); 
  box(player_win, 0, 0);
  wrefresh(player_win);

  opponent_win = newwin(height, width, starty, startx-20);
  box(opponent_win, 0, 0);
  wrefresh(opponent_win);

  status_win = newwin(stat_height, stat_width, starty+13, startx-20);

  create_grid(players_grid);

  clear();
  refresh();

  mvprintw(starty-1, startx-15, "Your ships");
  mvwprintw(opponent_win, 1,1,"  A B C D E F G H I J");
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
      else {mvwprintw(opponent_win, 2+h,3+f*2, "%c", t);}
      mvwprintw(opponent_win, 2+h,4+f*2, "|");
    }
  }
  wrefresh(opponent_win);

  mvprintw(starty-1, startx+21, "Hit or miss ships");
  mvwprintw(player_win,1,1,"  A B C D E F G H I J");

  for (i=0;i<BOARD_SIZE;i++) {
    mvwprintw(player_win,i+2,1,"%d|_|_|_|_|_|_|_|_|_|_|",i+0);
  }
  refresh();
  wrefresh(player_win);
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
  int x,y,res,win_status=0;

  initShips();
  display_boards();

  Ship sh;
  do {
    if (fire == 1) { /*you're the attacker*/
      mvwprintw(status_win,1,1,"It's your turn!                    ");
      wrefresh(status_win);
            
      fire = 0;
      return_cords(&x, &y);
      res = do_fire(sock, x, y);
      place_hit_or_mis(player_win,res, x, y);
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
      curs_set(0); // Set cursor invisible
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
  clear();
  refresh();
  printw("Game over, %s, you %s!\nthanks for playing!!!\n\npress any key to continue...\n", global_user_name, (win_status) ? "win" : "lose");
  refresh();
  getch();
}


void title_screen()
{
  char *picture[] = {
    "                                     # #  ( )",
    "                                  ___#_#___|__",
    "                              _  |____________|  _",
    "                       _=====| | |            | | |==== _",
    "                 =====| |.---------------------------. | |====",
    "   <--------------------'   .  .  .  .  .  .  .  .   '--------------/",
    "     \\                                                             /",
    "      \\_______________________________________________WWS_________/",
    "  wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww",
    "   wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww",
    NULL
  };

  print_picture(stdscr, picture);
}



/*** Some utility functions ***/

/* from http://www.linuxdoc.org/HOWTO/NCURSES-Programming-HOWTO/panels.html */
void print_in_middle(WINDOW *win, int starty,
                     int startx, int width,
                     char const *const string, chtype color)
{
  int length, x, y;
  float temp;

  if(win == NULL)
    win = stdscr;
  getyx(win, y, x);
  if(startx != 0)
    x = startx;
  if(starty != 0)
    y = starty;
  if(width == 0)
    width = 80;

  length = strlen(string);
  temp = (width - length)/ 2;
  x = startx + (int)temp;
  wattron(win, color);
  mvwprintw(win, y, x, "%s", string);
  wattroff(win, color);
  refresh();
}


int get_picture_width(char *picture[])
{
  int maxlen=0;
  char msg[50];
  for (int i=0; picture[i]!=NULL; ++i) {
    int len = strlen(picture[i]);
    maxlen = len > maxlen ? len : maxlen;
  }
  return maxlen;
}

void print_picture(WINDOW *win, char *picture[])
{
  /* get width of picture */
  int picwidth = get_picture_width(picture);
  int leftoffset = (COLS - picwidth)/2;
  int topoffset = 2;
  for (int i=0; picture[i] != NULL; ++i) {
    mvwprintw(win, topoffset+i, leftoffset, picture[i]);
  }
}

char *get_text_string_from_centered_panel(char const *const prompt)
{
  WINDOW *panel_win;
  PANEL *the_panel;
  int panel_height=6,panel_width;
  char *dest = malloc(100);

  int promptlen = strlen(prompt);
  panel_width = MAX(30, promptlen+5);

  /* Create the window to hold the panel */
  panel_win = newwin(panel_height,
                     panel_width,
                     (LINES-panel_height)/2,
                     (COLS-panel_width)/2);
  box(panel_win, 0, 0);
  print_in_middle(panel_win, 1,
		  0, panel_width,
		  prompt, COLOR_PAIR(6));
  wattron(panel_win, COLOR_PAIR(5));
  mvwhline(panel_win, 3, 2, ' ', panel_width-4);
  curs_set(1); // make cursor visible
  echo();
  mvwgetnstr(panel_win, 3, 2, dest, panel_width-6);
  noecho();
  curs_set(0); // make cursor invisible
  wattroff(panel_win, COLOR_PAIR(5));
  
  /* create the panel from our window */
  the_panel = new_panel(panel_win);
  top_panel(the_panel);
  update_panels();
  doupdate();

  del_panel(the_panel);
  update_panels();
  delwin(panel_win);
  doupdate();

  return dest;
}
