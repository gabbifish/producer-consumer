package main

import (
	"flag"
	"fmt"
	"sync"
)

var bufferSize int
var producerCount int
var consumerCount int
var elementsProduced int

func producer(buffer chan<- int, id int, elements int, wg *sync.WaitGroup) {
	defer wg.Done()
	for i := 0; i < elements; i++ {
		buffer <- id
		fmt.Printf("producer id %d added to buf\n", id)
	}
}

func consumer(buffer <-chan int, id int, wg *sync.WaitGroup) {
	defer wg.Done()
	for elem := range buffer {
		fmt.Printf("Consumer id %d read %d from buffer.\n", id, elem)
	}
}

func init() {
	flag.IntVar(&bufferSize, "b", 100, "Size of buffer shared by producers and consumers.")
	flag.IntVar(&producerCount, "p", 5, "Number of producers")
	flag.IntVar(&consumerCount, "c", 5, "Number of consumers")
	flag.IntVar(&elementsProduced, "e", 100, "Num elements produced by each producer")
	flag.Parse()
}

func main() {
	var wgProducer sync.WaitGroup
	var wgConsumer sync.WaitGroup

	buffer := make(chan int, bufferSize)

	// launch goroutines for producer
	for i := 0; i < producerCount; i++ {
		wgProducer.Add(1)
		go producer(buffer, i, elementsProduced, &wgProducer)
	}

	// launch goroutines for consumer
	for i := 0; i < consumerCount; i++ {
		wgConsumer.Add(1)
		go consumer(buffer, i, &wgConsumer)
	}

	// close channel when producers have stopped writing to it.
	wgProducer.Wait()
	close(buffer)

	// wait for consumers to finish reading
	wgConsumer.Wait()
}
