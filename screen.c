/* -*- c-basic-offset: 2 -*- */
/**
 * This file contains all the ui routines.
 */


#include <ncurses.h>
#include <menu.h>
#include <panel.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "gamepieces.h"
#include "connection.h"
#include "screen.h"
#include "log.h"
#include "common.h"

#define MAX_NAME 100

extern Ship Shipset[];
extern Ship PeerShipset[];
char global_user_name[MAX_NAME];
char peer_user_name[MAX_NAME];
int user_mode;
typedef enum SHOT_SPOT {
  UNTOUCHED=0,
  MISS,
  HIT
} SHOT_SPOT;
SHOT_SPOT player_shots[BOARD_SIZE][BOARD_SIZE]; /* 1=hit, 2=miss */
SHOT_SPOT peer_shots[BOARD_SIZE][BOARD_SIZE]; /* 1=hit, 2=miss */


WINDOW *player_win;
WINDOW *opponent_win;
WINDOW *status_win;

void place_hit_or_mis(WINDOW * win,int mesg, int x, int y, bool was_peer_shot)
{
  //-2game -1 hit sink 1hit  0miss
  //deal with hits first

  if ((mesg == -2) || (mesg == -1) || (mesg == 1)) {
    wattron(win,COLOR_PAIR(4));
    mvwprintw(win, y+2, x*2+3,"#");
    wattroff(win,COLOR_PAIR(4));
    wrefresh(win);
    if (was_peer_shot)
      peer_shots[x][y] = HIT;
    else
      player_shots[x][y] = HIT;
  } else { // miss
    wattron(win,COLOR_PAIR(3));
    mvwprintw(win, y+2, x*2+3,"@");
    wattroff(win,COLOR_PAIR(3));
    wrefresh(win);
    if (was_peer_shot)
      peer_shots[x][y] = MISS;
    else
      player_shots[x][y] = MISS;
  }
}

/**
 * Display battlefields after exchanging boards.
 */
void show_battlefields()
{
  /* dump battlefields: */
  if (user_mode == SERVER_MODE) {
    write_to_log("player_shots:\n");
    for (int i=0; i<BOARD_SIZE; ++i) {
      for (int j=0; j<BOARD_SIZE; ++j) {
    char msg[10];
    sprintf(msg, "%c", player_shots[i][j] == HIT ? 'h'
        : (player_shots[i][j] == MISS ? 'm' : 'u') );
    write_to_log(msg);
      }
      write_to_log("\n");
    }
    write_to_log("peer_shots:\n");
    for (int i=0; i<BOARD_SIZE; ++i) {
      for (int j=0; j<BOARD_SIZE; ++j) {
    char msg[10];
    sprintf(msg, "%c", peer_shots[i][j] == HIT ? 'h'
        : (peer_shots[i][j] == MISS ? 'm' : 'u') );
    write_to_log(msg);
      }
      write_to_log("\n");
    }
  }

  noecho();
  bool checking_opponent = true;
  bool cont = true;
  while (cont) {
    char field[BOARD_SIZE*BOARD_SIZE + 1000]; /* TODO: give me a break... */
    int ind=0;
    for (int y=0; y<BOARD_SIZE; ++y) {
      for (int x=0; x<BOARD_SIZE; ++x) {
    if (checking_opponent) { /* checking opponent */
      if (is_there_a_ship_here(PeerShipset, x, y)) {
        field[ind++] = player_shots[x][y] == HIT ? '#' : '*';
      } else {
        field[ind++] = player_shots[x][y] == MISS ? '@' : '_';
      }
    } else {        /* checking user */
      if (is_there_a_ship_here(Shipset, x, y)) {
        field[ind++] = peer_shots[x][y] == HIT ? '#' : '*';
      } else {
        field[ind++] = peer_shots[x][y] == MISS ? '@' : '_';
      }
    } /* eo checking_opponent */
    field[ind++] = '|';
      }
      field[ind++] = '\n';
    }
    sprintf(&(field[ind]), "Currently displaying %s%s territory.\n"
        "Press any key to see %s%s.\n"
        "Press enter to continue",
        checking_opponent ? peer_user_name : "your",
        checking_opponent ? "'s" : "",
        checking_opponent ? "yours" : peer_user_name,
        checking_opponent ? "" : "'s");
    show_message_box(field);

    int ch = getch();
    switch(ch) {
    case 10:
    case KEY_ENTER:
      cont = false;
      break;
    } /* eo switch */
    checking_opponent = !checking_opponent;
  }   /* eo while cont */
  hide_message_box();
}

/**
 * Documentation here
 */
void main_menu()
{


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

  show_message_box("Welcome to termship!\n(Press any key)");
  getch();
  hide_message_box();

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
  free(my_items);
  unpost_menu(my_menu);
  free_menu(my_menu);

  if (0==strcmp(selected_name, "Create")) {
    user_mode = SERVER_MODE;
    get_text_string_from_centered_panel("Enter your name",
                    global_user_name,
                    MAX_NAME);
    init_game(user_mode);
  } else if (0==strcmp(selected_name, "Join")) {
    user_mode = CLIENT_MODE;
    get_text_string_from_centered_panel("Enter your name",
                    global_user_name,
                    MAX_NAME);
    init_game();
  }

  return;
}

void return_cords(int * x, int * y)
{
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
        {
    case KEY_LEFT:
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
        case 10:
        case KEY_ENTER:
      if (player_shots[player_pos.x][player_pos.y] == UNTOUCHED) {
        *x = player_pos.x;
        *y = player_pos.y;
        return;
      } else {
            move(playery, playerx);
      }
          break;
                
        }
    } 
}

void display_boards(void)
{
  int startx, starty, width, height; 
  int stat_width, stat_height;

  char players_grid[BOARD_SIZE][BOARD_SIZE];

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

  create_grid(players_grid, Shipset);

  clear();
  refresh();

  mvprintw(starty-1, startx-15, global_user_name);
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
      } else {
    mvwprintw(opponent_win, 2+h,3+f*2, "%c", t);
      }
      mvwprintw(opponent_win, 2+h,4+f*2, "|");
    }
  }
  wrefresh(opponent_win);

  mvprintw(starty-1, startx+21, peer_user_name);
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
  char msg[100];

  for (int i=0; i<BOARD_SIZE; ++i) {
    for (int j=0; j<BOARD_SIZE; ++j) {
      player_shots[i][j] = UNTOUCHED;
      peer_shots[i][j] = UNTOUCHED;
    }
  }


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


      place_hit_or_mis(player_win,res, x, y, false);
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
        sh = getShipById(-1*res); /* what a hack... */
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

  sprintf(msg, "Game over! You %s!\nPress any key to view battlefields.", win_status ? "won" : "lost");
  show_message_box(msg);
  getch();
  exchange_shipsets(sock);
  show_battlefields();
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

  /* int numsquiggles = 8; */
  /* int numreps = 2; */
  /* int framespeed = 60000; */
  /* do a little "animation" */
  /* for (int i=0; i<numreps; ++i) { */
  /*   /\* pushing out *\/ */
  /*   for (int j=0; j<numsquiggles; ++j) { */
  /*     char msg[100]; */
  /*     int s=0; */
  /*     for (int k=0; k<j+1; ++k) { */
  /*    msg[s++] = '~'; */
  /*     } */
  /*     msg[s++]=' '; */
  /*     msg[s++]='t'; */
  /*     msg[s++]='e'; */
  /*     msg[s++]='r'; */
  /*     msg[s++]='m'; */
  /*     msg[s++]='s'; */
  /*     msg[s++]='h'; */
  /*     msg[s++]='i'; */
  /*     msg[s++]='p'; */
  /*     msg[s++]=' '; */
  /*     for (int k=0; k<j+1; ++k) { */
  /*    msg[s++] = '~'; */
  /*     } */
  /*     msg[s++] = '\0'; */
  /*     show_message_box(msg); */
  /*     usleep(framespeed); */
  /*   } */
  /*   /\* pulling in *\/ */
  /*   for (int j=numsquiggles; j>0; --j) { */
  /*     char msg[100]; */
  /*     int s=0; */
  /*     for (int k=0; k<j+1; ++k) { */
  /*    msg[s++] = '~'; */
  /*     } */
  /*     msg[s++]=' '; */
  /*     msg[s++]='t'; */
  /*     msg[s++]='e'; */
  /*     msg[s++]='r'; */
  /*     msg[s++]='m'; */
  /*     msg[s++]='s'; */
  /*     msg[s++]='h'; */
  /*     msg[s++]='i'; */
  /*     msg[s++]='p'; */
  /*     msg[s++]=' '; */
  /*     for (int k=0; k<j+1; ++k) { */
  /*    msg[s++] = '~'; */
  /*     } */
  /*     msg[s++] = '\0'; */
  /*     show_message_box(msg); */
  /*     usleep(framespeed); */
  /*   } */
  /* } */



  /* Test an animation: */
  /* Animation *anim1 = create_animation("test1.txt"); */
  /* play_animation(anim1, true); */
  Animation *underwater_explosion = create_animation("underwater_explosion.txt");
  play_animation(underwater_explosion, true);
  destroy_animation(underwater_explosion);
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

static WINDOW *message_box_win;
static PANEL *message_box_panel;
static int last_message_box_width=0;
static int last_message_box_height=0;
/**
 * Shows a global message box in the middle of the screen
 */
void show_message_box(char const *const string)
{
  show_message_box_win(&message_box_win, &message_box_panel,
               string, &last_message_box_width, &last_message_box_height);
}

/**
 * Shows a message box in specific window the middle of the
 * screen. You should make sure you're always passing in the same
 * reference to last_box_width and last_box_height associated with
 * this window.
 */
void show_message_box_win(WINDOW **win, PANEL **pan, char const *const string,
              int *last_box_width, int *last_box_height)
{
  int width;
  int height = 5;
  int leftoffset = 1;
  int largest_line_length = 0, prev_newline_index = 0;
  char msg[500];
  (void)msg; /*compiler warnings*/

  /* Count the newlines to determine any extra height we'll need to add */
  for (int i=0; i<strlen(string)+1; ++i) {
    if (string[i] == '\n' || string[i] == '\0') {
      largest_line_length = i-prev_newline_index > largest_line_length
    ? i-prev_newline_index
    : largest_line_length;
      prev_newline_index = i;
      if (string[i] == '\n')
    height++;
      /* sprintf(msg, "found newline or null at %d. now height is %d largest line is %d\n", i, height, largest_line_length); */
      /* write_to_log(msg); */
    }
  }
  largest_line_length = largest_line_length == 0
    ? strlen(string)
    : largest_line_length;
  /* sprintf(msg, "At the end, now height is %d largest line is %d\n", height, largest_line_length); */
  /* write_to_log(msg); */

  width = largest_line_length + 6;


  /* if there's an existing message box up and this string is a
     different length than the last one, we need to recompute the
     width, so we just hide the message box. */
  if (*last_box_width != width || *last_box_height != height) {
    sprintf(msg, "implicit hide of the message box because %d != %d || %d != %d\n",
        *last_box_width, width, *last_box_height, height);
    write_to_log(msg);
    hide_message_box_win(win, pan);
  }

  if (*win == NULL) {
    *win = newwin(height, width,
                 (LINES-height)/2,
                 (COLS-width)/2);
    sprintf(msg, "created new win at *win %p\n", *win);
    write_to_log(msg);
  }
  if (*pan == NULL) {
    *pan = new_panel(*win);
    sprintf(msg, "created new *pan at %p\n", *pan);
    write_to_log(msg);
  }
  wattron(*win, BLUE_ON_BLACK);
  box(*win, 0, 0);
  wattroff(*win, BLUE_ON_BLACK);
  /* border(186, 186, 205, 205, */
  /*     201, 187, 200, 188); */
  /* border(ls, rs, chtype ts, chtype bs, */
  /*     chtype tl, chtype tr, chtype bl, chtype br); */

  int current_y = 1;
  wattron(*win, WHITE_ON_RED);
  for (int i=0; i<width-2; ++i)
    mvwprintw(*win, current_y, i+1, "-"); /* fill above the text */
  current_y++;


  /* Now for the printing. We need to split the string on newlines (if
     any) and print each of those separately */
  char *our_string = (char *) malloc(strlen(string)+1);
  strcpy(our_string, string);
  if (strchr(string, '\n') != NULL) { /* we have newlines */
    for (char *next_tok = strtok(our_string, "\n");
     next_tok != NULL;
     next_tok = strtok(NULL, "\n")) {
      wattron(*win, WHITE_ON_RED);
      mvwhline(*win, current_y, 1, ' ', width-2);
      print_in_middle(*win, current_y,
              leftoffset, width-1, next_tok,
              WHITE_ON_RED);
      current_y++;
    }
  } else {
    wattron(*win, WHITE_ON_RED);
    mvwhline(*win, current_y, 1, ' ', width-2);
    print_in_middle(*win, current_y, leftoffset, width-1, string, WHITE_ON_RED);
    current_y++;
  }

  wattron(*win, WHITE_ON_RED);
  for (int i=0; i<width-2; ++i)
    mvwprintw(*win, current_y, i+1, "-"); /* fill below the text */

  wattroff(*win, WHITE_ON_RED);

  free(our_string);

  top_panel(*pan);
  update_panels();
  doupdate();

  *last_box_width = width;
  *last_box_height = height;
}

/**
 * Hides the message box
 */
void hide_message_box()
{
  hide_message_box_win(&message_box_win, &message_box_panel);
}

/**
 * Hides a specific message box
 */
void hide_message_box_win(WINDOW **win, PANEL **pan)
{
  char msg[500];
  wclear(*win);
  if (*pan != NULL) {
    sprintf(msg, "deleting *pan at %p\n", *pan);
    write_to_log(msg);
    del_panel(*pan);
    *pan = NULL;
    update_panels();
  }
  wrefresh(*win);
  if (*win != NULL) {
    sprintf(msg, "deleting *win at %p\n", *win);
    write_to_log(msg);
    delwin(*win);
    *win = NULL;
    /* doupdate(); */
  }
  refresh();
}


int get_picture_width(char *picture[])
{
  int maxlen=0;
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

/**
 * dest should have enough space (at least len) to hold the string.
 */
void get_text_string_from_centered_panel(char const *const prompt, char *dest, int len)
{
  WINDOW *panel_win;
  PANEL *the_panel;
  int panel_height=6,panel_width;
  /* char *dest = malloc(100); */

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
  mvwgetnstr(panel_win, 3, 2, dest, len);
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
}


/**
 * Constructs and returns a new Animation object. This doesn't load
 * the animation. That will be done at play time or you can call
 * load_animation to do it manually. The animation should be free'd by
 * the user.
 */
Animation *create_animation(char *loadFile)
{
  Animation *anim = (Animation *)malloc(sizeof(Animation));
  anim->isLoaded = false;
  anim->loadFile = (char *) malloc(sizeof(char) * MAX_FILE_LEAF_NAME);
  strcpy(anim->loadFile, loadFile);
  return anim;
}

void destroy_animation(Animation *anim)
{
  for(int i=0; i < anim->numFrames; ++i) {
    free(anim->frames[i]);
  }
  free(anim->frames);
  free(anim->loadFile);
  free(anim);
}


/**
 * Loads up the animation. Make sure you set the `loadFile` attribute
 * of the Animation before calling this function.
 */
void load_animation(Animation *anim)
{
  FILE *fp;
  char msg[500], loadFileFullPath[MAX_FILE_FULL_PATH];
  char *line, *thisFrame;
  size_t len=0;
  ssize_t read;

  sprintf(loadFileFullPath, "%s/animations/%s", xstr(TERMSHIP_PATH), anim->loadFile);
  sprintf(msg, "Loading animation file from %s...\n", loadFileFullPath);
  write_to_log(msg);

  fp = fopen(loadFileFullPath, "r");
  if (fp == NULL) {
    cleanup_ncurses();
    printf("couldn't open %s for reading...\n", loadFileFullPath);
    exit(EXIT_FAILURE);
  }

  /*** read the header lines: ***/
  /* First is the size (in lines) of each of the frames we're about to
     read */
  line = NULL;
  read = getline(&line, &len, fp);
  sscanf(line, "%d\n", &(anim->height));
  free(line);
  sprintf(msg, "%s has height %d\n", loadFileFullPath,
      anim->height);
  write_to_log(msg);
  /* Next is the total number of frames */
  line = NULL;
  read = getline(&line, &len, fp);
  sscanf(line, "%d\n", &(anim->numFrames));
  free(line);
  sprintf(msg, "%s has %d total frames\n", loadFileFullPath, anim->numFrames);
  write_to_log(msg);
  /* Next is the desired frame rate: */
  line = NULL;
  read = getline(&line, &len, fp);
  sscanf(line, "%d\n", &(anim->fps));
  free(line);
  sprintf(msg, "%s will run at %d fps\n", loadFileFullPath, anim->fps);
  write_to_log(msg);

  (void)read; /*compiler warnings*/

  /* Allocate space for the animation (the frames, not the actual lines quite yet): */
  anim->frames = (char **) malloc(sizeof(char *) * anim->numFrames);
  thisFrame = (char *) malloc(sizeof(char) * anim->height * MAX_FRAME_WIDTH);

  int max_width = 0;
  for (int i=0; i < anim->numFrames; ++i) {
    bool last_char_was_newline = false;
    int chars_read;
    for (chars_read=0; ; ++chars_read) {
      int ch = fgetc(fp);
      /* sprintf(msg, "[%d] => %c\n", chars_read, (char)ch); */
      /* write_to_log(msg); */
      thisFrame[chars_read] = ch;
      if (ch == '\n') {
        /* two newlines in a row. next frame. */
        if (last_char_was_newline)
          break;
        last_char_was_newline = true;
      } else {
        last_char_was_newline = false;
      }
    }
    thisFrame[chars_read-1] = '\0'; /* overwriting the final newline */
    anim->frames[i] = (char *) malloc((sizeof(char) * chars_read)); /* don't need +1 because we truncated the last newline */
    strcpy(anim->frames[i], thisFrame);

    max_width = MAX(chars_read, max_width);

  } /*eo for each line*/

  free(thisFrame);

  anim->width = max_width;

  anim->isLoaded = true;
}

/**
 * Plays the specified animation. Loads it if necessary.
 */
void play_animation(Animation *anim, bool hold_at_end)
{
  static WINDOW *animation_window = NULL;
  static PANEL  *animation_panel = NULL;
  static int    anim_width;
  static int    anim_height;
  char msg[500];
  (void)msg;
  char *hold_message = "\n(Press any key to continue)";

  if (!anim->isLoaded) load_animation(anim);

  anim_width = anim->width;
  anim_height = anim->height;

  for (int i=0; i < anim->numFrames; ++i) {
    show_message_box_win(&animation_window, &animation_panel,
             anim->frames[i], &anim_width, &anim_height);

    /* we assume the show_message_box_win takes 0 time */
    usleep( (1/(float)anim->fps) * 1000000 );
  }

  if (hold_at_end) {
    char *hold_frame = (char *) malloc(strlen(anim->frames[anim->numFrames-1]) + strlen(hold_message) + 1);
    strcpy(hold_frame, anim->frames[anim->numFrames-1]);
    strcat(hold_frame, hold_message);
    show_message_box_win(&animation_window, &animation_panel,
             hold_frame, &anim_width, &anim_height);
    getch();
    free(hold_frame);
  }

  hide_message_box_win(&animation_window, &animation_panel);
}


void cleanup_ncurses()
{
  endwin();         /* end curses mode */
}
