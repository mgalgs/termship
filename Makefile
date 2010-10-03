#termship

CC=gcc
CFLAGS=-g
OBJS=termship.o connection.o screen.o gamepieces.o Btypes.o log.o error.o
# CFLAGS += -DTEST_SHIPS

### Implicit Rules ###

.c.o:
	$(CC) $(CFLAGS) -c $<

######################


all: termship

termship: $(OBJS) Btypes.h
	$(CC) $(CFLAGS) -o termship $(OBJS) -lncurses

clean:
	rm -f *.o

emacs_clean:
	rm -f *~

purge: clean emacs_clean
	rm -f termship

rebuild: clean all
