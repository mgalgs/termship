/**
 * This file contains all the networking routines.
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "connection.h"
#include "gamepieces.h"
#include "screen.h"
#include "Btypes.h"

#define PORT 4740
#define MAX_MSG 100
#define MAX_IP 20
#define MAX_USER_NAME 50
#define MAX_NAME 100

extern Ship Shipset[];
extern Ship Their_Shipset[];
extern WINDOW *opponent_win;
extern char name[];
extern bool their_shots[BOARD_SIZE][BOARD_SIZE];
extern bool my_shots[BOARD_SIZE][BOARD_SIZE];
extern int user_mode;

char peer_user_name[50];

void init_game(const int user_mode)
{
  int sock, accept_sock, cnt;
  struct sockaddr_in addr;
  int cont = 1;

  //printw("using usermode %d\n", user_mode);
  refresh();

  //open socket:
  if ( (sock = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) {
    graceful_exit("Error creating socket", -1);
  }


  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);




  if (user_mode == CLIENT_MODE) {
    char ip[MAX_IP];
    printw("Enter IP address of server (enter nothing for localhost): ");
    getstr(ip);
    if (strcmp(ip,"") == 0)
      strcpy(ip, "127.0.0.1");
    printw("using IP: %s\n",ip);

    //set up addr with IP address:
    if ( inet_pton(AF_INET, ip, &addr.sin_addr) == 0 ) {
      graceful_exit("That doesn't look like a valid IP address", sock);
    }
    clear();
    for (cnt=0; cnt<MAX_RETRIES; cnt++) {
      //connect
      if ( (connect(sock, (struct sockaddr *) &addr, sizeof(addr))) == -1 ) {
	mvprintw(1,2,"Connecting to server... %s", cnt%2==0 ? "|" : "-");
	refresh();
	sleep(1);
      } else {
	exchange_names(sock,1);
	mvprintw(2,2,"Connected to server!");
	mvprintw(3,2,"You will be battling against %s\n\nPress any key to continue...", peer_user_name);
	break;
      }
    }
    if (cnt == MAX_RETRIES) {
      graceful_exit("Error connecting to server", sock);
    }

    refresh();
    getch();
    do_gameplay(sock, 0);
  } else {
    int i = 0;
    //bind
    if ( bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {/*bind error*/
      graceful_exit("bind error", sock);
    }

    //listen
    if ( listen(sock, 100) == -1 ) {/*listen error*/
      graceful_exit("listen error", sock);
    }

    while(cont) {
      socklen_t len; //for accept
      //accept
      clear();
      printw("Waiting for a client to connect.%s", i ? "." : "..");
      refresh();
      i = ~i;
      accept_sock = accept(sock, (struct sockaddr *) &addr, &len);
      exchange_names(accept_sock,0);
      printw("Connected to client!\nYou will be battling against %s\n\nPress any key to continue...", peer_user_name);
      refresh();
      getch();
      do_gameplay(accept_sock, 1);
    }
  }
}

/**
 * Return -1 on sink, 1 on hit (no sink), -2 on game over, 0 on miss
 */
int do_fire(const int sock, const int x, const int y)
{
  BMesg *buf;
  int nbytes;
  buf = CreateBMesg(BFIRE, x, y);
  if ( nbytes = send(sock, buf, sizeof(BMesg), 0) == -1) { /*send error*/
    graceful_exit("send error", sock);
  }
  //we successfully sent
  //printw("Sent data...\n");
  refresh();

  //receive response
  get_response(sock, buf);
  if (buf->msg == BHIT) {
    if ( strcmp(buf->code,SUNK) == 0 ) {
      //printw("You sunk them!");
      return -1;
    } else if ( strcmp(buf->code,GAME_OVER) == 0){
      return -2;
    } else {
      //printw("You hit them!");
      return 1;
    }
    refresh();
  } else {
    return 0;
    //printw("You missed!");
  }
  //WE SHOULD NEVER GET HERE!
  return -100;
  //TODO: get rid of memory leaks (deallocate everything allocated with CreateBMesg)
}

/**
 * Returns 0 if miss, -id if sunk, +id if hit (not sunk)
 */
int do_receive(const int sock)
{
  int nbytes,res;
  BMesg *buf;
  char *scratch;
  buf = CreateEmptyBMesg();
  if ( (nbytes = recv(sock, buf, MAX_MSG, 0) == -1) ) { /*recv error*/
    graceful_exit("recv error", sock);
  }
  //we successfully returned from recv
  //printw("Received %d bytes of data...Such as %s\n", nbytes, buf->code);
  refresh();

  //parse buf to see if it was a hit and respond
  if ( (res = check_hit(buf)) != 0 ) { /*it was a hit*/
    if (check_game_over()) { /*the game is over*/
      send_hit(sock, GAME_OVER);
      return 100; //magic number...i know it's bad but i'm ready to be done.
    } else {
      if (res < 0) {
	send_hit(sock, SUNK);
	return res;
      } else if (res > 0) {
	send_hit(sock, NULL);
	return res;
      }
      else {
	graceful_exit("Unexpected case in do_receive", sock);
      }
    }
  } else {
    send_miss(sock);
    return res; //res==0
  }
  //At this point we should have sent exactly 1 response to the other user *and returned*
  //WE SHOULD NEVER GET HERE
  graceful_exit("Reached unexpected worlds....", sock);
  return -1; //just to make the compiler happy
}

/**
 * Checks to see if buf causes a hit and adjust the gamepieces accordingly.
 * Returns 0 on a miss, -1*(id) of ship on a hit and sink, or the id of the ship on a hit without a sink.
 */
int check_hit(const BMesg *buf)
{
  char *scratch, dest[MAX_CODE], msg[50];
  int x,y,lenx,leny,i,j;
  refresh();
  if ( (scratch = index(buf->code, ',')) == NULL) { /*comma not found*/
    sprintf(msg, "Invalid BFIRE message detected in check_hit: %s", buf->code);
    graceful_exit(msg, -1);
  } else { /*this is an "x,y" code*/
    lenx = scratch - buf->code;
    leny = (strlen(buf->code)-1) - lenx; //strlen - 1 for null terminator
    strncpy(dest, buf->code, lenx);
    x = atoi(dest);
    strncpy(dest, buf->code+lenx+1, leny);
    y = atoi(dest);
    refresh();
  }
  their_shots[x][y] = true;
  //check for a hit:
  for (i=0; i<NUM_SHIPS; i++) { //for each ship
    if (Shipset[i].sunk) //it's sunk
      continue; //go to the next ship
    for (j=0; j<Shipset[i].size; j++) { //for each "slot" on the ship
      if (Shipset[i].slots[j] == 1) { //this slot has already been hit
	//printw("%s has already been hit on slot %d", Shipset[i].name, j);
	continue; //go to the next slot
      }
      if (Shipset[i].direction == 1) { /*ship placed vertically*/
	if ( (Shipset[i].x == x) && (Shipset[i].y+j == y) ) {
	  //printw("hit ship %d on slot %d\n",i,j);
	  refresh();
	  setAsHit(i, j); //hit ship i on slot j (also takes care if setting as sunk)
	  return ((Shipset[i].sunk) ? -1*(i+1) : i+1); //return -id if it's a sink, else +id
	}
      } else { /*ship placed horizontally*/
	if ( (Shipset[i].x+j == x) && (Shipset[i].y == y) ) {
	  //printw("hit ship %d on slot %d\n",i,j);
	  refresh();
	  setAsHit(i, j); //hit ship i on slot j (also takes care if setting as sunk)
	  return ((Shipset[i].sunk) ? -1*(i+1) : i+1); //return -id if it's a sink, else +id
	}
      }
    }
  }

  /* place_hit_or_mis(opponent_win,0,x,y); */

  //if we haven't returned until now then there was no hit
  return 0;
}

/**
 * Checks to see if the game is over (i.e. all your ships are sunk).
 * Returns 1 if the game is over, 0 otherwise.
 */
int check_game_over()
{
  int i;
  for (i=0; i<NUM_SHIPS; i++) {
    if (!Shipset[i].sunk) {
      //printw("%s is not sunk..game is not over...\n",Shipset[i].name);
      return 0; //we found an unsunk ship, so the game's not over
    } else {
      //printw("%s is sunk..still checking for game over...\n",Shipset[i].name);
    }
  }
  return 1;
}



/**
 * Sends a HIT message along with a HIT_CODE.
 * Pass NULL in as code if there isn't a code.
 */
void send_hit(const int sock, const char *code)
{
  BMesg *buf;
  int nbytes;
  buf = CreateEmptyBMesg();
  buf->msg=BHIT;
  if (code)
    strcpy(buf->code, code);
  if ( nbytes = send(sock, buf, sizeof(BMesg), 0) == -1) { /*send error*/
    graceful_exit("send error", sock);
  }
}

/**
 * Sends a MISS message
 */
void send_miss(const int sock)
{
  BMesg *buf;
  int nbytes;
  buf = CreateEmptyBMesg();
  buf->msg=BMISS;
  if ( nbytes = send(sock, buf, sizeof(BMesg), 0) == -1) { /*send error*/
    graceful_exit("send error", sock);
  }
}

/**
 * Get a response from client and store it in buf
 */
void get_response(const int sock, BMesg *buf)
{
  int nbytes;
  if ( (nbytes = recv(sock, buf, MAX_MSG, 0) == -1) ) { /*recv error*/
    graceful_exit("recv error", sock);
  }
}

void exchange_maps(const int sock)
{
  if (user_mode == CLIENT_MODE) {
    get_their_map(sock);
    send_my_map(sock);
  } else {
    send_my_map(sock);
    get_their_map(sock);
  }
}

void get_their_map(const int sock)
{
  int nbytes,i;
  char msg[50];
  /* 3 bytes for each ship: x,y,direction (we assume they send them in
     order defined in gamepieces.c) */
  char map_buffer[3*NUM_SHIPS];
  write_to_log("Getting their map...\n");
  if ( (nbytes = recv(sock, map_buffer, sizeof(map_buffer), MSG_WAITALL)) == -1 )
    graceful_exit("Error getting their map...", sock);

  write_to_log("Got their map!\n");
  /* fill up their map: */
  for (i=0; i<NUM_SHIPS; ++i) {
    Their_Shipset[i].x = map_buffer[i*3];
    Their_Shipset[i].y = map_buffer[i*3+1];
    Their_Shipset[i].direction = map_buffer[i*3+2];
    sprintf(msg, "ship #%d: (x,y,dir)=(%d,%d,%d)\n", i, Their_Shipset[i].x, Their_Shipset[i].y, Their_Shipset[i].direction);
    write_to_log(msg);
  }
}

void send_my_map(const int sock)
{
  int nbytes, i;
  char map_buffer[3*NUM_SHIPS];

  /* fill up the buffer with our map: */
  for (i=0; i<NUM_SHIPS; ++i) {
    map_buffer[i*3] = (char)Shipset[i].x;
    map_buffer[i*3+1] = (char)Shipset[i].y;
    map_buffer[i*3+2] = (char)Shipset[i].direction;
  }

  /* send our map */
  if ( -1 == (nbytes = send(sock, map_buffer, sizeof(map_buffer), 0)) )
    graceful_exit("error while sending the map...", sock);
}

/**
 * Get the other user's name.
 * mode=0 means server, mode=1 means client
 */
void exchange_names(const int sock, const int mode)
{
  if (mode) {
    get_user_name(sock);
    send_user_name(sock);
  } else { //reverse the order
    send_user_name(sock);
    get_user_name(sock);
  }
}


//gets user name
void get_user_name(const int sock)
{
  int nbytes;
  if ( (nbytes = recv(sock, peer_user_name, MAX_USER_NAME, 0)) == -1) {
    graceful_exit("Error getting other user's name...", sock);
  }
}

//sends user name
void send_user_name(const int sock)
{
  int nbytes;
  if ( (nbytes = send(sock, name, strlen(name)+1/*for '\0'*/, 0)) == -1) { /*send error*/
    graceful_exit("send error", sock);
  }
}
