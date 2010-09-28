/**
  * This file contains all the networking routines.
  */

#include <string.h>
#include <stdlib.h>
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
extern WINDOW *opponent_win;
extern char name[];

char peer_user_name[50];

void init_game(const int user_mode)
{
    int sock, accept_sock;
    struct sockaddr_in addr;
    int cont = 1;

    //printw("using usermode %d\n", user_mode);
    refresh();

    //open socket:
    if ( (sock = socket(PF_INET, SOCK_STREAM, 0)) == -1 ) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
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
            perror("That doesn't look like a valid IP address");
            exit(EXIT_FAILURE);
        }
        clear();
        //connect
        if ( (connect(sock, (struct sockaddr *) &addr, sizeof(addr))) == -1 ) {
            perror("Error connecting to server");
            exit(EXIT_FAILURE);
        } else {
            exchange_names(sock,1);
            printw("Connected to server!\nYou will be battling against %s\n\nPress any key to continue...", peer_user_name);
        }
        refresh();
        getch();
        do_gameplay(sock, 0);
    } else {
        int i = 0;
        //bind
        if ( bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {/*bind error*/
            perror("bind error");
            exit(EXIT_FAILURE);
        }

        //listen
        if ( listen(sock, 100) == -1 ) {/*listen error*/
            perror("listen error");
            exit(EXIT_FAILURE);
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

    place_hit_or_mis(opponent_win,0,x,y);

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
        perror("recv error");
        exit(EXIT_FAILURE);
    }
}


/**
 * Get the other user's name.
 * mode=0 means server, mode=1 means client
 */
void exchange_names(const int sock, const int mode)
{
    int nbytes;
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
        perror("Error getting other user's name...");
        exit(EXIT_FAILURE);
    }
}

//sends user name
void send_user_name(const int sock)
{
    int nbytes;
    if ( (nbytes = send(sock, name, strlen(name)+1/*for '\0'*/, 0)) == -1) { /*send error*/
        perror("send error");
        exit(EXIT_FAILURE);
    }
}
