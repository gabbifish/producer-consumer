#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

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
  int elems_produced;
  int id;
  buf_info_t *buf_info;
} producer_t;

typedef struct {
  int id;
  buf_info_t *buf_info;
} consumer_t;

void *producer(void *pc) {
  producer_t *producer_config = (producer_t *)pc;
  buf_info_t *buf_info = producer_config->buf_info;
  for (int i = 0; i < producer_config->elems_produced; i++){
    pthread_mutex_lock(buf_info->buf_mutex);
    // if can_produce, write an element in. otherwise block.
    while (buf_info->buffer_elems == buf_info->buffer_cap) {
      pthread_cond_wait(buf_info->can_produce, buf_info->buf_mutex);
    }
    // ADD TO BUFFER APPROPRIATELY
    buf_info->buffer[buf_info->next_write] = producer_config->id;
    printf("Producer %d wrote value %d\n", producer_config->id,
      producer_config->id);
    // In case last index is taken, do wraparound.
    buf_info->next_write = (buf_info->next_write + 1) %
      buf_info->buffer_cap;
    buf_info->buffer_elems += 1;

    // signifify this thread is done if all elems have been produced.
    if (i == producer_config->elems_produced - 1) {
      buf_info->producers_done += 1;
    }
    pthread_cond_signal(buf_info->can_consume);
    pthread_mutex_unlock(buf_info->buf_mutex);
  }
  // when you know you're done producing (no more elements to write)
  printf("Producer id %d finished writing to buffer.\n", producer_config->id);
  pthread_exit(NULL);
  // need to free producer_configs once we know threads are done
  free(producer_config);
}

void *consumer(void *cc) {
  consumer_t *consumer_config = (consumer_t *)cc;
  buf_info_t *buf_info = consumer_config->buf_info;

  while(1) {
    pthread_mutex_lock(buf_info->buf_mutex);
    // REALLY HARD TO FIGURE OUT HOW TO SIGNIFY BUFFER IS CLOSED?
    if (buf_info->producers_done == buf_info->total_producers &&
        buf_info->buffer_elems == 0) {
      pthread_mutex_unlock(buf_info->buf_mutex); // still need to unlock mutex
      break;
    }
    while (buf_info->buffer_elems == 0) {
      pthread_cond_wait(buf_info->can_consume, buf_info->buf_mutex);
    }
    // READ FROM BUFFER APPROPRIATELY
    int read_val = buf_info->buffer[buf_info->next_read];
    printf("Consumer %d read value %d\n", consumer_config->id, read_val);

    buf_info->buffer[buf_info->next_read] = -1;
    // In case last index is already read, do wraparound.
    buf_info->next_read = (buf_info->next_read + 1) %
      buf_info->buffer_cap;
    buf_info->buffer_elems -= 1;

    pthread_cond_signal(buf_info->can_produce);
    pthread_mutex_unlock(buf_info->buf_mutex);
  }
  // when you know you're done consuming
  printf("Consumer id %d finished writing to buffer.\n", consumer_config->id);
  pthread_exit(NULL);
  // need to free consumer_configs once we know threads are done
  free(consumer_config);
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

  // Buffer variables
  int buffer[buffer_size];

  // Mutexes necessary
  pthread_mutex_t buf_mutex = PTHREAD_MUTEX_INITIALIZER;

  // Condition variables necessary
  pthread_cond_t can_produce = PTHREAD_COND_INITIALIZER;
  pthread_cond_t can_consume = PTHREAD_COND_INITIALIZER;

  // Lists of producer and consumer threads
  pthread_t producer_threads[producer_count];
  pthread_t consumer_threads[consumer_count];

  buf_info_t *buf_info = calloc(sizeof(buf_info_t), 1);
  buf_info->buffer = buffer;
  buf_info->buffer_elems = 0;
  buf_info->buffer_cap = buffer_size;
  buf_info->next_write = 0;
  buf_info->next_read = 0;
  buf_info->producers_done = 0;
  buf_info->total_producers = producer_count;
  buf_info->buf_mutex = &buf_mutex;
  buf_info->can_produce = &can_produce;
  buf_info->can_consume = &can_consume;

  for(int p = 0; p < producer_count; p++) {
    producer_t *producer_config = calloc(sizeof(producer_t), 1);
    producer_config->id = p;
    producer_config->elems_produced = elements_produced;
    producer_config->buf_info = buf_info;

    int producer_thread_err = pthread_create(&producer_threads[p], NULL,
      producer, (void *)producer_config);
    if (producer_thread_err) {
      printf("ERROR; return code from creating producer is %d\n",
       producer_thread_err);
      exit(-1);
    }
  }

  for(int c = 0; c < consumer_count; c++) {
    consumer_t *consumer_config = calloc(sizeof(consumer_t), 1);
    consumer_config->id = c;
    consumer_config->buf_info = buf_info;

    int consumer_thread_err = pthread_create(&consumer_threads[c], NULL,
      consumer, (void *)consumer_config);
    if (consumer_thread_err) {
      printf("ERROR; return code from creating producer is %d\n",
       consumer_thread_err);
      exit(-1);
    }
  }

  // when producer has finished writing, start "sending" sentinels?
  // that way consumers know when to quit.
  free(buf_info);
  pthread_exit(NULL);
}
