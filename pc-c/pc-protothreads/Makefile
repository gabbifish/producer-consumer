CC = gcc
LDIR = pt-1.4
CFLAGS = -g -Wall -I$(LDIR)

default: pc-protothreads

pc-protothreads:  pc-pthreads.o
	$(CC) $(CFLAGS) -o pc-protothreads pc-protothreads.o $(LDFLAGS)

pc-protothreads.o:  pc-protothreads.c
	$(CC) $(CFLAGS) -c pc-protothreads.c

.PHONY : clean

clean:
	$(RM) count *.o *~

