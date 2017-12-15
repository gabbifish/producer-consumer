/*
Usage: pc-pthreads [-h] [-b BUFFER_SIZE] [-p NUM_PRODUCERS] [-c NUM_CONSUMERS]
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
-j    have consumer threads sleep for 1ms after reading int from buffer
      (default: no sleep)
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h> // for usleep()

typedef struct {
  int *buffer;
  int buffer_elems;
  int next_write;
  int next_read;
  int producers_done;
  int total_producers;
  int buffer_cap;
  pthread_mutex_t *buf_mutex;
  pthread_cond_t *can_produce;
  pthread_cond_t *can_consume;
} buf_info_t;

typedef struct {
  int elems_per_producer;
  int id;
  buf_info_t *buf_info;
  bool do_sleep;
} producer_t;

typedef struct {
  int id;
  buf_info_t *buf_info;
  int *num_consumed;
  bool do_sleep;
} consumer_t;

void *producer(void *pc) {
  producer_t *producer_config = (producer_t *)pc;
  buf_info_t *buf_info = producer_config->buf_info;

  for (int i = 0; i < producer_config->elems_per_producer; i++){
    if (producer_config->do_sleep) {
      // simulate a blocking disk I/O read
      // sleep for 1000 microseconds = 1 millisecond
      usleep(1000);
    }

    pthread_mutex_lock(buf_info->buf_mutex);
    // if can_produce, write an element in. Otherwise block.
    while (buf_info->buffer_elems == buf_info->buffer_cap) {
      pthread_cond_wait(buf_info->can_produce, buf_info->buf_mutex);
    }

    // ADD TO BUFFER APPROPRIATELY
    int val = producer_config->id * 1000 + i;
    buf_info->buffer[buf_info->next_write] = val; // producer_config->id;
    // if (val % buf_info->buffer_cap >
    //   buf_info->buffer_cap - buf_info->total_producers) {
    //   printf("Producer %d wrote value %d\n", producer_config->id, val); // producer_config->id);
    // }

    // In case last index is taken, do wraparound.
    buf_info->next_write = (buf_info->next_write + 1) % buf_info->buffer_cap;
    buf_info->buffer_elems += 1;

    // Signify this thread is done if all elems have been produced.
    if (i == producer_config->elems_per_producer - 1) {
      buf_info->producers_done += 1;
    }

    /* We tell ALL consumers via pthread_cond_broadcast() that there are items
     * in the buffer. This is because if this is the last producer on its last
     * item, there may be multiple consumers waiting on can_consume.
     * If we call pthread_cond_signal() which only signals one consumer, then
     * only that one consumer will read from buffer, realize that all producers
     * are done, and exit. The other consumers would just hang on can_consume.
     */
    pthread_cond_broadcast(buf_info->can_consume);
    pthread_mutex_unlock(buf_info->buf_mutex);
  }
  // done producing, no more elements to write
  // printf("Producer id %d finished writing to buffer.\n", producer_config->id);

  pthread_exit(NULL);
}

void *consumer(void *cc) {
  consumer_t *consumer_config = (consumer_t *)cc;
  buf_info_t *buf_info = consumer_config->buf_info;

  // int num_elems_consumed = 0;
  bool done_consuming = false; // only true when all producers finish and buffer is empty

  while(true) {
    pthread_mutex_lock(buf_info->buf_mutex);
    while (buf_info->buffer_elems == 0) {
      if (buf_info->producers_done == buf_info->total_producers) {
        done_consuming = true;
        pthread_mutex_unlock(buf_info->buf_mutex); // still need to unlock mutex
        break;
      } else {
        pthread_cond_wait(buf_info->can_consume, buf_info->buf_mutex);
      }
    }
    if (done_consuming) {
      break;
    }

    // read from buffer
    int read_val = buf_info->buffer[buf_info->next_read];
    // printf("Consumer %d read value %d\n", consumer_config->id, read_val);

    buf_info->buffer[buf_info->next_read] = -1;
    // In case last index is already read, do wraparound.
    buf_info->next_read = (buf_info->next_read + 1) % buf_info->buffer_cap;
    buf_info->buffer_elems -= 1;

    // Tell at least one producer that there is now space in the buffer.
    // We don't have to tell all producers since consumers always outlive
    // producers.
    pthread_cond_signal(buf_info->can_produce);
    pthread_mutex_unlock(buf_info->buf_mutex);

    if (consumer_config->do_sleep) {
      // simulate an expensive computation
      // sleep for 1000 microseconds = 1 millisecond
      usleep(1000);
    }

    // num_elems_consumed += 1;
    read_val *= 2; // "data computation"
  }
  // done consuming
  // printf("Consumer id %d finished reading %d items from buffer.\n",
  //     consumer_config->id, num_elems_consumed);
  // *(consumer_config->num_consumed) = num_elems_consumed;

  pthread_exit(NULL);
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

  // Buffer. We can keep this on the stack since the main
  // thread waits for the producers and consumers to finish.
  int buffer[buffer_size];

  // Mutexes necessary
  pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;

  // Condition variables necessary
  pthread_cond_t can_produce = PTHREAD_COND_INITIALIZER;
  pthread_cond_t can_consume = PTHREAD_COND_INITIALIZER;

  // Lists of producer and consumer threads
  pthread_t producer_threads[producer_count];
  pthread_t consumer_threads[consumer_count];

  // lists of producer and consumer configs
  producer_t producer_configs[producer_count];
  consumer_t consumer_configs[consumer_count];

  // Result array to store return value of consumer threads
  int consumer_results[consumer_count];

  buf_info_t buf_info = {
    .buffer = buffer,
    .buffer_elems = 0,
    .buffer_cap = buffer_size,
    .next_write = 0,
    .next_read = 0,
    .producers_done = 0,
    .total_producers = producer_count,
    .buf_mutex = &buf_mutex,
    .can_produce = &can_produce,
    .can_consume = &can_consume
  };

  for(int p = 0; p < producer_count; p++) {
    producer_t *producer_config = &producer_configs[p];
    producer_config->id = p;
    producer_config->elems_per_producer = elems_per_producer;
    producer_config->buf_info = &buf_info;
    producer_config->do_sleep = producer_do_sleep;

    int producer_thread_err = pthread_create(&producer_threads[p], NULL,
      producer, (void *) producer_config);
    if (producer_thread_err) {
      printf("ERROR; return code from creating producer is %d\n",
       producer_thread_err);
      exit(-1);
    }
  }

  for(int c = 0; c < consumer_count; c++) {
    consumer_t *consumer_config = &consumer_configs[c];
    consumer_config->id = c;
    consumer_config->buf_info = &buf_info;
    consumer_config->num_consumed = &consumer_results[c];
    consumer_config->do_sleep = consumer_do_sleep;

    int consumer_thread_err = pthread_create(&consumer_threads[c], NULL,
      consumer, (void *) consumer_config);
    if (consumer_thread_err) {
      printf("ERROR; return code from creating consumer is %d\n",
       consumer_thread_err);
      exit(-1);
    }
  }

  // Join all producer threads
  for (int p = 0; p < producer_count; p++) {
    pthread_join(producer_threads[p], NULL);
    // printf("Joined producer id %d\n", p);
  }
  // printf("Finished joining all producers\n");

  // Join all consumer threads
  // int total_consumed = 0;
  for (int c = 0; c < consumer_count; c++) {
    pthread_join(consumer_threads[c], NULL);
    // total_consumed += consumer_results[c];
    // printf("Joined consumer id %d\n", c);
  }
  // printf("Finished joining all consumers\n");

  // check that we actually consumed all items produced
  // const int total_produced = producer_count * elems_per_producer;
  // if (total_consumed == total_produced) {
  //   // printf("Consumed all %d elements produced by producers\n", total_produced);
  // } else {
  //   printf("ERROR: Only consumed %d out of %d elements produced\n",
  //       total_consumed, total_produced);
  // }

  // memory clean up
  pthread_mutex_destroy(&buf_mutex);
  pthread_cond_destroy(&can_produce);
  pthread_cond_destroy(&can_consume);
}
