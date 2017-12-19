## CS 242 Final Project

**Comparing Producer-Consumer Implementations in Go, Rust, and C**

Authors
- Gabbi Fisher
- Christopher Yeh

Stanford CS 242: Programming Languages
- Instructor: Will Crichton
- Autumn, 2017-18

## Getting Started

We assume this is running on macOS or Linux, and that `gcc` is already installed.

- Install Rust (v1.20.0) - [link](https://www.rust-lang.org/en-US/install.html)
- Install Go (v1.9.2) - [link](https://golang.org/doc/install)
- Install libmill (v1.18) - [link](http://libmill.org/documentation.html)

To run the benchmark: `bash ./bench.sh`

## Running individual implementations

All of the executables support the following flags:

- `-h`: print the help menu and exit
- `-b BUF_SIZE`: buffer size in number of integers (default=100)
- `-p NUM_PRODUCERS`: number of producer threads (default=5)
- `-c NUM_CONSUMERS`: number of consumer threads (default=5)
- `-e ELEMS_PER_PRODUCER`: number of elements each producer adds to buffer (default=100)
- `-i`: have producer threads sleep for 1ms before adding int to buffer
- `-j`: have consumer threads sleep for 1ms after reading int from buffer

## Other

NOTE: the C-protothreads implementation is experimental and not completely functional yet.
