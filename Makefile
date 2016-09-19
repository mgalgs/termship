#termship

CC=gcc
CFLAGS=-g -std=gnu99 -DTERMSHIP_PATH=`pwd` -Wall
OBJS=termship.o connection.o screen.o gamepieces.o Btypes.o log.o
NMS=no-more-secrets/src/nms.c
# CFLAGS += -DTEST_SHIPS

### Implicit Rules ###

.c.o:
	$(CC) $(CFLAGS) -c $<

######################


all: termship

termship: $(OBJS) Btypes.h
	$(CC) $(CFLAGS) $(NMS) -o termship $(OBJS) -lpanel -lmenu -lncurses -lncursesw

clean:
	rm -f *.o

emacs_clean:
	rm -f *~

purge: clean emacs_clean
	rm -f termship

rebuild: clean all
