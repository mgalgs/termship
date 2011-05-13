#termship

CC=gcc
CFLAGS=-g -std=gnu99
OBJS=termship.o connection.o screen.o gamepieces.o Btypes.o log.o
# CFLAGS += -DTEST_SHIPS

### Implicit Rules ###

.c.o:
	$(CC) $(CFLAGS) -c $<

######################


all: termship

termship: $(OBJS) Btypes.h
	$(CC) $(CFLAGS) -o termship $(OBJS) -lpanel -lmenu -lncursesw

clean:
	rm -f *.o

emacs_clean:
	rm -f *~

purge: clean emacs_clean
	rm -f termship

rebuild: clean all
