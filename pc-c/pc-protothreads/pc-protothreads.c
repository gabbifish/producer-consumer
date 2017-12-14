#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

#include <unistd.h>
#include "pt-sem.h"
#include "pt.h"

extern char* optarg;

typedef struct pt thread;
typedef struct pt_sem sem;

typedef struct {
  int *buffer;
  int buffer_elems;
  int next_write;
  int next_read;
  int producers_done;
  int total_producers;
  int buffer_cap;
  sem *not_full;
  sem *not_empty;
} buf_info_t;

typedef struct {
  thread t;
  int elems_per_producer;
  int num_produced;
  int id;
  buf_info_t *buf_info;
} producer_t;

typedef struct {
  thread t;
  int id;
  buf_info_t *buf_info;
  int num_consumed;
} consumer_t;


// static void add_to_buffer(int item) {
//   printf("Item %d added to buffer at place %d\n", item, bufptr);  
//   buffer[bufptr] = item;
//   bufptr = (bufptr + 1) % BUFSIZE;
// }
// static int get_from_buffer(void) {
//   int item;
//   item = buffer[bufptr];
//   printf("Item %d retrieved from buffer at place %d\n",
//    item, bufptr);
//   bufptr = (bufptr + 1) % BUFSIZE;
//   return item;
// }

// static int produce_item(void) {
//   static int item = 0;
//   printf("Item %d produced\n", item);
//   return item++;
// }

// static void consume_item(int item) {
//   printf("Item %d consumed\n", item);
// }
 
static PT_THREAD(producer(producer_t *p)) {
  static int val;
  static buf_info_t *buf_info;
 
  buf_info = p->buf_info;

  PT_BEGIN(&p->t);

  while (p->num_produced < p->elems_per_producer) {
    PT_SEM_WAIT(&p->t, buf_info->not_full);

    // ADD TO BUFFER APPROPRIATELY
    val = p->id * 1000 + p->num_produced;
    buf_info->buffer[buf_info->next_write] = val; // p->id;
    p->num_produced += 1;
    printf("Producer %d wrote value %d\n", p->id, val); // p->id);

    // In case last index is taken, do wraparound.
    buf_info->next_write = (buf_info->next_write + 1) % buf_info->buffer_cap;
    buf_info->buffer_elems += 1;

    // Signify this thread is done if all elems have been produced.
    if (p->num_produced == p->elems_per_producer) {
      buf_info->producers_done += 1;
    }

    PT_SEM_SIGNAL(&p->t, buf_info->not_empty);
  }

  PT_END(&p->t);
}
 
static PT_THREAD(consumer(consumer_t *c)) {
  static int read_val;
  static buf_info_t *buf_info;
 
  buf_info = c->buf_info;
  printf("Consumer thread %d\n", c->id);
  printf("%d\n", &c->t);

  PT_BEGIN(&c->t);

  while(true) {
    printf("Consumer %d running\n", c->id);
    if (buf_info->buffer_elems == 0 && 
        buf_info->producers_done == buf_info->total_producers) {
      break;
    }

    PT_SEM_WAIT(&c->t, buf_info->not_empty);

    // read from buffer
    read_val = buf_info->buffer[buf_info->next_read];
    printf("Consumer %d read value %d\n", c->id, read_val);

    buf_info->buffer[buf_info->next_read] = -1;
    // In case last index is already read, do wraparound.
    buf_info->next_read = (buf_info->next_read + 1) % buf_info->buffer_cap;
    buf_info->buffer_elems -= 1;

    // Tell at least one producer that there is now space in the buffer.
    // We don't have to tell all producers since consumers always outlive
    // producers.
    PT_SEM_SIGNAL(&c->t, buf_info->not_full);

    c->num_consumed += 1;
  }

  // done consuming
  printf("Consumer id %d finished reading %d items from buffer.\n",
      c->id, c->num_consumed);

  PT_END(&c->t);
}


int main(int argc, char *argv[]) {
  // Establish default values
  static int buffer_size = 17;
  static int producer_count = 5;
  static int consumer_count = 5;
  static int elems_per_producer = 30;

  static int p;
  static int c;

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
        elems_per_producer = atoi((char *) optarg);
        break;
    } /* switch */
  } /* -- while -- */

  // lists of producer and consumer configs
  static producer_t *producer_configs;
  static consumer_t *consumer_configs;
  producer_configs = calloc(producer_count, sizeof(producer_t));
  consumer_configs = calloc(consumer_count, sizeof(consumer_t));

  static int *buffer;
  buffer = calloc(buffer_size, sizeof(int));

  static sem not_full, not_empty;

  buf_info_t buf_info = {
    .buffer = buffer,
    .buffer_elems = 0,
    .buffer_cap = buffer_size,
    .next_write = 0,
    .next_read = 0,
    .producers_done = 0,
    .total_producers = producer_count,
    .not_full = &not_full,
    .not_empty = &not_empty
  };

  PT_SEM_INIT(buf_info.not_empty, 0);
  PT_SEM_INIT(buf_info.not_full, buffer_size);

  for(p = 0; p < producer_count; p++) {
    producer_t *producer_config = &producer_configs[p];
    producer_config->id = p;
    producer_config->elems_per_producer = elems_per_producer;
    producer_config->num_produced = 0;
    producer_config->buf_info = &buf_info;

    PT_INIT(&producer_config->t);
  }

  for(c = 0; c < consumer_count; c++) {
    consumer_t *consumer_config = &consumer_configs[c];
    consumer_config->id = c;
    consumer_config->buf_info = &buf_info;
    consumer_config->num_consumed = 0;

    PT_INIT(&consumer_config->t);
  }

  static bool is_running = true;
  static int result;

  while (is_running) {
    is_running = false;
    for (p = 0; p < producer_count; p++) {
      result = PT_SCHEDULE(producer(&producer_configs[p]));
      if (result != 0) {
        is_running = true;
      }
    }
    for (c = 0; c < consumer_count; c++) {
      result = PT_SCHEDULE(consumer(&consumer_configs[c]));
      if (result != 0) {
        is_running = true;
      }
    }
  }

  // Join all consumer threads
  static int total_consumed = 0;
  for (c = 0; c < consumer_count; c++) {
    total_consumed += consumer_configs[c].num_consumed;
    printf("Joined consumer id %d\n", c);
  }
  printf("Finished joining all consumers\n");

  // check that we actually consumed all items produced
  static int total_produced;
  total_produced = producer_count * elems_per_producer;
  if (total_consumed == total_produced) {
    printf("Consumed all %d elements produced by producers\n", total_produced);
  } else {
    printf("ERROR: Only consumed %d out of %d elements produced\n",
        total_consumed, total_produced);
  }

  free(producer_configs);
  free(consumer_configs);

  free(buffer);

  return 0;
}
