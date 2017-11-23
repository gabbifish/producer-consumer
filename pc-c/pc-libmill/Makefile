CC = gcc
LDIR = libmill
CFLAGS = -g -Wall -I$(LDIR)
LDFLAGS = -lmill

default: pc-libmill

pc-libmill:  pc-libmill.o
	$(CC) $(CFLAGS) -o pc-libmill pc-libmill.o $(LDFLAGS)

pc-libmill.o:  pc-libmill.c
	$(CC) $(CFLAGS) -c pc-libmill.c

.PHONY : clean

clean:
	$(RM) count *.o *~
