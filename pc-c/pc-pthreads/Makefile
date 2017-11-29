CC = gcc
CFLAGS = -g -Wall

default: pc-pthreads

pc-pthreads:  pc-pthreads.o
	$(CC) $(CFLAGS) -o pc-pthreads pc-pthreads.o $(LDFLAGS)

pc-pthreads.o:  pc-pthreads.c
	$(CC) $(CFLAGS) -c pc-pthreads.c

.PHONY : clean

clean:
	$(RM) count *.o *~
