#include <libmill.h>
// #include "libmill/libmill.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <getopt.h>

extern char* optarg;

coroutine void producer(chan buffer, chan complete, int id, int elements) {
	for (int i = 0; i < elements; i++) {
		chs(buffer, int, id);
	  printf("Producer id %d added to buffer.\n", id);
	}
  printf("Producer id %d finished writing to buffer.\n", id);
  chs(complete, int, id);
}

coroutine void consumer(chan buffer, chan complete, int id) {
  int elem = chr(buffer, int);
  while (elem >= 0) {
    printf("Consumer id %d read %d from buffer.\n", id, elem);
    elem = chr(buffer, int);
  }
  printf("producer id %d finished reading from buffer.\n", id);
  chs(complete, int, id);
}

int prog(chan chan_done, int complete_count) {
  int done = 0;
  while (1) {
    int id = chr(chan_done, int);
    if (id >= 0) done++;
    if (done == complete_count) return complete_count;
  }
}

int main (int argc, char *argv[]) {
  // Establish default values
  int buffer_size = 100;
  int producer_count = 5;
  int consumer_count = 5;
  int elements_produced = 100;

  int c;
  while ((c = getopt(argc, argv, "hb:p:c:e:")) != EOF)
  {
    switch (c)
    {
      case 'h':
        // usage(argv[0]);
        exit(0);
        break;
      case 'b':
        buffer_size = atoi((char *) optarg);
        break;
      case 'p':
        producer_count = atoi((char *) optarg);
        break;
      case 'c':
        consumer_count = atoi((char *) optarg);
        break;
      case 'e':
        elements_produced = atoi((char *) optarg);
        break;
    } /* switch */
  } /* -- while -- */
  chan producer_done = chmake(int, producer_count);
  chan consumer_done = chmake(int, consumer_count);
  chan buffer = chmake(int, buffer_size);

  for (int i = 0; i < producer_count; i++) {
    go(producer(buffer, producer_done, i, elements_produced));
  }

  for (int i = 0; i < consumer_count; i++) {
    go(consumer(buffer, consumer_done, i));
  }

  // Wait for all producers to finish. Send -1 sentinel to signify producers are
  // done.
  prog(producer_done, producer_count);
  chdone(buffer, int, -1);
  chclose(producer_done);

  // Wait for all consumers to finish before closing channel.
  prog(consumer_done, producer_count);
  chclose(buffer);
  chclose(consumer_done);
  return 0;
}
