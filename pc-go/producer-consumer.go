package main

import (
	"flag"
	// "fmt"
	"sync"
	"time"
)

var bufferSize int
var producerCount int
var consumerCount int
var elementsProduced int
var producer_sleep bool
var consumer_sleep bool

func producer(buffer chan<- int, id int, elements int, sleep bool, wg *sync.WaitGroup) {
	defer wg.Done()
	for i := 0; i < elements; i++ {
		buffer <- id
		// fmt.Printf("producer id %d added to buf\n", id)
		if sleep {
			// simulate a blocking disk I/O read, sleep for 1ms
			time.Sleep(time.Duration(1)*time.Millisecond)
		}
	}
}

func consumer(buffer <-chan int, id int, sleep bool, wg *sync.WaitGroup) {
	defer wg.Done()
	for elem := range buffer {
		// fmt.Printf("Consumer id %d read %d from buffer.\n", id, elem)
		if sleep {
			// simulate an expensive computation, sleep for 1ms
			time.Sleep(time.Duration(1)*time.Millisecond)
		}
		elem = elem*2 // "data computation"
	}
}

func init() {
	flag.IntVar(&bufferSize, "b", 100, "Size of buffer shared by producers and consumers, default=100")
	flag.IntVar(&producerCount, "p", 5, "Number of producers, default=5")
	flag.IntVar(&consumerCount, "c", 5, "Number of consumers, default=5")
	flag.IntVar(&elementsProduced, "e", 100, "Num elements produced by each producer, default=100")
	flag.BoolVar(&producer_sleep, "i", false, "have producer threads sleep for 1ms before adding int to buffer")
	flag.BoolVar(&consumer_sleep, "j", false, "have consumer threads sleep for 1ms after reading int from buffer")
	flag.Parse()
}

func main() {
	var wgProducer sync.WaitGroup
	var wgConsumer sync.WaitGroup

	buffer := make(chan int, bufferSize)

	// launch goroutines for producer
	for i := 0; i < producerCount; i++ {
		wgProducer.Add(1)
		go producer(buffer, i, elementsProduced, producer_sleep, &wgProducer)
	}

	// launch goroutines for consumer
	for i := 0; i < consumerCount; i++ {
		wgConsumer.Add(1)
		go consumer(buffer, i, consumer_sleep, &wgConsumer)
	}

	// close channel when producers have stopped writing to it.
	wgProducer.Wait()
	close(buffer)

	// wait for consumers to finish reading
	wgConsumer.Wait()
}
