CC = gcc
CFLAGS  = -g -Wall -I$(LDIR)
LDIR = libmill

default: pc-libmill

pc-libmill:  pc-libmill.o
	$(CC) $(CFLAGS) -o pc-libmill pc-libmill.o

pc-libmill.o:  pc-libmill.c
	$(CC) $(CFLAGS) -c pc-libmill.c

clean:
	$(RM) count *.o *~
