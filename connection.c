/**
 * This file contains all the networking routines.
 */

#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>
#include "connection.h"
#include "gamepieces.h"
#include "screen.h"
#include "Btypes.h"


#define START_PORT 4740
#define END_PORT 5160
#define PORT_INTERVAL 42
#define MAX_MSG 100
#define MAX_IP 20
#define MAX_USER_NAME 50
#define MAX_NAME 100

extern Ship Shipset[];
extern Ship PeerShipset[];
extern WINDOW *opponent_win;
extern char global_user_name[];
extern char peer_user_name[];
extern int user_mode;


void init_game()
{
  int sock, accept_sock;
  struct sockaddr_in addr;
  int cont = 1;
  char msg[70];

  //printw("using usermode %d\n", user_mode);
  refresh();

  //open socket:
  if ( (sock = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) {
    cleanup_ncurses();
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }


  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;


  if (user_mode == CLIENT_MODE) {
    char ip[50];
    bool found_server = false;
    get_text_string_from_centered_panel("Enter IP address of server (enter nothing for localhost): ",
					ip, 50);
    if (ip[0] == '\0')
      strcpy(ip, "127.0.0.1");
    /* char msg[50]; */
    /* sprintf(msg, "using IP: %s", ip); */
    /* show_message_box(msg); */
    /* getch(); */
    /* cleanup_ncurses(); */
    /* exit(EXIT_SUCCESS); */

    //set up addr with IP address:
    if ( inet_pton(AF_INET, ip, &addr.sin_addr) == 0 ) {
      cleanup_ncurses();
      perror("That doesn't look like a valid IP address");
      exit(EXIT_FAILURE);
    }
    clear();


    //connect
    bool keep_trying_to_find_a_game = true;
    while(keep_trying_to_find_a_game) {
      for (uint16_t i=START_PORT; i<END_PORT; i+=PORT_INTERVAL) {
	sprintf(msg, "Searching for battleship server on port %u", i);
	show_message_box(msg);
	addr.sin_port = htons(i);
	if ( (connect(sock, (struct sockaddr *) &addr, sizeof(addr))) == 0 ) {
	  if (verify_server(sock)) {
	    found_server = true;
	    keep_trying_to_find_a_game = false;
	    break;
	  }
	}
	usleep(200000);		/* 200 ms pause between scans */
      }
      if (found_server) {
	show_message_box("Connected to server!");
	usleep(100000);		/* 1s */
	exchange_names(sock);
	sprintf(msg, "You will be battling against %s.\nPress any key to continue...", peer_user_name);
	show_message_box(msg);
	getch();
	do_gameplay(sock, 0);
      } else {
	show_message_box("We're having trouble finding a game...\nWe'll keep trying though.");
	sleep(2);
	/* cleanup_ncurses(); */
	/* printf("Couldn't find a game!\n"); /\* TODO: return to main menu *\/ */
	/* exit(EXIT_FAILURE); */
      }
    }
  } else {
    int i = 0;
    addr.sin_port = htons(get_battleship_port());

    //bind
    if ( bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {/*bind error*/
      cleanup_ncurses();
      perror("bind error");
      exit(EXIT_FAILURE);
    }

    //listen
    if ( listen(sock, 100) == -1 ) {/*listen error*/
      cleanup_ncurses();
      perror("listen error");
      exit(EXIT_FAILURE);
    }

    while(cont) {
      socklen_t len; //for accept
      //accept
      clear();
      show_message_box("Waiting for a client to connect...");
      i = !i;
      accept_sock = accept(sock, (struct sockaddr *) &addr, &len);
      if (verify_client(accept_sock)) {
	exchange_names(accept_sock);
	sprintf(msg, "You will be battling against %s.\nPress any key to continue...", peer_user_name);
	/* sprintf(msg, "Connected to client!\nYou will be battling against %s\nPress any key to continue...", peer_user_name); */
	show_message_box(msg);
	getch();
	do_gameplay(accept_sock, 1);
      }
    }
  }
}

/**
 * Gets a random battleship port. A battleship port is any multiple of
 * 42 in the range [4740..5160] (that's 10 possible ports, 1 for
 * each day of the year -- not really).
 */
uint16_t get_battleship_port()
{
  return 4740 + ((rand() % 10) * 42);
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
    cleanup_ncurses();
    perror("send error");
    exit(EXIT_FAILURE);
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
    cleanup_ncurses();
    perror("recv error");
    exit(EXIT_FAILURE);
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
	cleanup_ncurses();
	perror("Unexpected case in do_receive");
	exit(EXIT_FAILURE);
      }
    }
  } else {
    send_miss(sock);
    return res; //res==0
  }
  //At this point we should have sent exactly 1 response to the other user *and returned*
  //WE SHOULD NEVER GET HERE
  cleanup_ncurses();
  perror("Reached unexpected worlds....");
  exit(EXIT_FAILURE);
  return -1; //just to make the compiler happy
}

/**
 * Checks to see if buf causes a hit and adjust the gamepieces accordingly.
 * Returns 0 on a miss, -1*(id) of ship on a hit and sink, or the id of the ship on a hit without a sink.
 */
int check_hit(const BMesg *buf)
{
  char *scratch, dest[MAX_CODE];
  int x,y,lenx,leny,i,j;
  refresh();
  if ( (scratch = index(buf->code, ',')) == NULL) { /*comma not found*/
    cleanup_ncurses();
    perror("Invalid BFIRE message detected in check_hit");
    exit(EXIT_FAILURE);
  } else { /*this is an "x,y" code*/
    lenx = scratch - buf->code;
    leny = (strlen(buf->code)-1) - lenx; //strlen - 1 for null terminator
    strncpy(dest, buf->code, lenx);
    x = atoi(dest);
    strncpy(dest, buf->code+lenx+1, leny);
    y = atoi(dest);
    refresh();
  }
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

  place_hit_or_mis(opponent_win,0,x,y,true);

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
    cleanup_ncurses();
    perror("send error");
    exit(EXIT_FAILURE);
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
    cleanup_ncurses();
    perror("send error");
    exit(EXIT_FAILURE);
  }
}

/**
 * Get a response from client and store it in buf
 */
void get_response(const int sock, BMesg *buf)
{
  int nbytes;
  if ( (nbytes = recv(sock, buf, MAX_MSG, 0) == -1) ) { /*recv error*/
    cleanup_ncurses();
    perror("recv error");
    exit(EXIT_FAILURE);
  }
}


/**
 * Get the other user's name.
 */
void exchange_names(const int sock)
{
  if (user_mode == SERVER_MODE) {
    get_peer_user_name(sock);
    send_user_name(sock);
  } else { //reverse the order
    send_user_name(sock);
    get_peer_user_name(sock);
  }
}

//gets user name
void get_peer_user_name(const int sock)
{
  int nbytes;
  if ( (nbytes = recv(sock, peer_user_name, MAX_USER_NAME, 0)) == -1) {
    cleanup_ncurses();
    perror("Error getting other user's name...");
    exit(EXIT_FAILURE);
  }
}

//sends user name
void send_user_name(const int sock)
{
  int nbytes;
  if ( (nbytes = send(sock, global_user_name, strlen(global_user_name)+1/*for '\0'*/, 0)) == -1) { /*send error*/
    cleanup_ncurses();
    perror("send error");
    exit(EXIT_FAILURE);
  }
}

/**
 * Get the other user's name.
 */
void exchange_shipsets(const int sock)
{
  show_message_box("Waiting to exchange ships from opponent...");
  if (user_mode == SERVER_MODE) {
    get_peer_shipset(sock);
    send_shipset(sock);
  } else { //reverse the order
    send_shipset(sock);
    get_peer_shipset(sock);
  }
  hide_message_box();
}

void get_peer_shipset(const int sock)
{
  for (int i=0; i<NUM_SHIPS; ++i) {
    PeerShipset[i].x = recv_byte(sock);
    PeerShipset[i].y = recv_byte(sock);
    PeerShipset[i].direction = recv_byte(sock);
  }
}

void send_shipset(const int sock)
{
  for (int i=0; i<NUM_SHIPS; ++i) {
    send_byte(sock, Shipset[i].x);
    send_byte(sock, Shipset[i].y);
    send_byte(sock, Shipset[i].direction);
  }
}



void send_byte(const int sock, uint8_t byte)
{
  if (-1 == send(sock,
		 &byte,
		 1, 0)) { /*send error*/
    cleanup_ncurses();
    perror("Error sending a byte...");
    exit(EXIT_FAILURE);
  }
}

uint8_t recv_byte(const int sock)
{
  uint8_t byte;
  if ( -1 == recv(sock,
		  &byte,
		  1, 0)) {
    cleanup_ncurses();
    perror("Error recv'ing a byte...");
    exit(EXIT_FAILURE);
  }
  return byte;
}

bool verify_server(const int sock)
{
  /* Handshake: send a 42, recv a 47 in return, send a 49 back. */
  send_byte(sock, 42);
  if (recv_byte(sock) != 47) return false;
  send_byte(sock, 49);

  return true;
}

bool verify_client(const int sock)
{
  /* Handshake: recv a 42, send a 47 back, recv a 49 in return. */
  if (recv_byte(sock) != 42) return false;
  send_byte(sock, 47);
  if (recv_byte(sock) != 49) return false;

  return true;
}
