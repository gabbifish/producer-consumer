#include <libmill.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void producer() {

}

void consumer() {
  
}

int main (int argc, char *argv[]) {
  // Establish default values
  int bufferSize = 100;
  int producerCount = 5;
  int consumerCount = 5;
  int elementsProduced = 100;

  while ((c = getopt(argc, argv, "hb:p:c:e:")) != EOF)
  {
    switch (c)
    {
      case 'h':
        usage(argv[0]);
        exit(0);
        break;
      case 'b':
        bufferSize = atoi((char *) optarg);
        break;
      case 'p':
        producerCount = atoi((char *) optarg);
        break;
      case 'c':
        consumerCount = atoi((char *) optarg);
        break;
      case 'e':
        elementsProduced = atoi((char *) optarg);
        break;
    } /* switch */
  } /* -- while -- */


}
