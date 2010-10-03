/**
  * connection routines.
  */
#ifndef CONNECTION_H
#define CONNECTION_H

#include "Btypes.h"

#define MAX_RETRIES 100

void init_game(const int);
int do_fire(const int, const int, const int);
int do_receive(const int);
int check_hit(const BMesg*);
int check_game_over();
void send_hit(const int, const char*);
void send_miss(const int);
void get_response(const int, BMesg*);
void exchange_names(const int, const int);
void get_user_name(const int);
void send_user_name(const int);
void exchange_maps(const int);
void get_their_map(const int);
void send_my_map(const int);

#endif
