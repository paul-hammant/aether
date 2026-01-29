package main

import (
	"fmt"
	"os"
	"sync"
	"time"
)

const MESSAGES = 10000000

func main() {
	fmt.Println("=== Go Ping-Pong Benchmark ===")
	fmt.Printf("Messages: %d\n", MESSAGES)
	fmt.Println("Using goroutines with buffered channels")
	fmt.Println()

	chanA := make(chan int, 1)
	chanB := make(chan int, 1)

	var wg sync.WaitGroup
	wg.Add(2)

	start := time.Now()

	// Ping goroutine
	go func() {
		defer wg.Done()
		for i := 0; i < MESSAGES; i++ {
			chanA <- i
			received := <-chanB
			// VALIDATE: Must receive echo of what we sent
			if received != i {
				fmt.Fprintf(os.Stderr, "ERROR: Ping sent %d but got back %d\n", i, received)
			}
		}
	}()

	// Pong goroutine
	go func() {
		defer wg.Done()
		for i := 0; i < MESSAGES; i++ {
			received := <-chanA
			// VALIDATE: Must receive expected sequence
			if received != i {
				fmt.Fprintf(os.Stderr, "ERROR: Pong expected %d but got %d\n", i, received)
			}
			// Echo back what we received
			chanB <- received
		}
	}()

	wg.Wait()
	elapsed := time.Since(start)

	totalNs := float64(elapsed.Nanoseconds())
	nsPerMsg := totalNs / MESSAGES
	throughput := 1e9 / nsPerMsg
	cyclesPerMsg := nsPerMsg * 3.0

	fmt.Printf("Cycles/msg:     %.2f\n", cyclesPerMsg)
	fmt.Printf("Throughput:     %.2f M msg/sec\n", throughput/1e6)
}
