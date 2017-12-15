/*
Usage: pc-libmill [-h] [-b BUFFER_SIZE] [-p NUM_PRODUCERS] [-c NUM_CONSUMERS]
          [-e NUM_ELEMS_PER_PRODUCER] [-i] [-j]

-h    help
-b BUFFER_SIZE
      use a shared buffer that can hold BUFFER_SIZE integers
      (default: 100)
-p NUM_PRODUCERS
      number of producer threads (default: 5)
-c NUM_CONSUMERS
      number of consumer threads (default: 5)
-e NUM_ELEMS_PER_PRODUCER
      each producer adds NUM_ELEMS_PER_PRODUCER ints to the buffer
      (default: 100)
-i    have producer threads sleep for 1ms before adding int to buffer
      (default: no sleep)
-j    have consumer threads sleep for 1ms after reading each int from
      buffer (default: no sleep)
*/

#include <libmill.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h> // for usleep()

extern char* optarg;

coroutine void producer(chan buffer, chan complete, int id, int elements, bool sleep) {
	for (int i = 0; i < elements; i++) {
    if (sleep) {
      // simulate a blocking disk I/O read
      // sleep for 1000 microseconds = 1 millisecond
      usleep(1000);
    }
		chs(buffer, int, id);
	  // printf("Producer id %d added to buffer.\n", id);
	}
  // printf("Producer id %d finished writing to buffer.\n", id);
  chs(complete, int, id);
}

coroutine void consumer(chan buffer, chan complete, int id, bool sleep) {
  int elem = chr(buffer, int);
  while (elem >= 0) {
    // printf("Consumer id %d read %d from buffer.\n", id, elem);
    elem = chr(buffer, int);

    if (sleep) {
      // simulate an expensive computation
      // sleep for 1000 microseconds = 1 millisecond
      usleep(1000);
    }

    elem *= 2; // "data computation"
  }
  // printf("producer id %d finished reading from buffer.\n", id);
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
  int elems_per_producer = 100;

  bool producer_do_sleep = false;
  bool consumer_do_sleep = false;

  int c;
  while ((c = getopt(argc, argv, "hb:p:c:e:ij")) != EOF) {
    switch (c) {
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
        elems_per_producer = atoi((char *) optarg);
        break;
      case 'i':
        producer_do_sleep = true;
        break;
      case 'j':
        consumer_do_sleep = true;
        break;
    } /* switch */
  } /* -- while -- */

  chan producer_done = chmake(int, producer_count);
  chan consumer_done = chmake(int, consumer_count);
  chan buffer = chmake(int, buffer_size);

  for (int i = 0; i < producer_count; i++) {
    go(producer(buffer, producer_done, i, elems_per_producer, producer_do_sleep));
  }

  for (int i = 0; i < consumer_count; i++) {
    go(consumer(buffer, consumer_done, i, consumer_do_sleep));
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
