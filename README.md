# Asynchronous File Processing Server

## Project basis

This implementation follows the submitted Operating Systems and Systems Programming project abstract:

- Linux-based server written in C
- TCP socket communication
- Concurrent client connections
- Pending request queue
- Linux `io_uring` asynchronous file read/write
- Completion queue
- POSIX mutexes and condition variables
- High-resolution timing with `clock_gettime()`
- Throughput and latency measurements
- GCC / Ubuntu build environment

The project abstract specifically proposes `io_uring`, request/completion queues, synchronization, socket programming, and timing APIs.

## Architecture

```text
                   +----------------------+
Clients ---------->| TCP Socket Server    |
                   +----------+-----------+
                              |
                              v
                    +-------------------+
                    | Client Handlers   |
                    | pthreads          |
                    +---------+---------+
                              |
                              v
                    +-------------------+
                    | Request Queue      |
                    | mutex + cond var   |
                    +---------+---------+
                              |
                              v
                    +-------------------+
                    | I/O Submitter     |
                    | io_uring SQ       |
                    +---------+---------+
                              |
                              v
                    +-------------------+
                    | Linux io_uring    |
                    | async read/write  |
                    +---------+---------+
                              |
                              v
                    +-------------------+
                    | Completion Queue  |
                    | mutex + cond var  |
                    +---------+---------+
                              |
                              v
                    +-------------------+
                    | Response Worker   |
                    +---------+---------+
                              |
                              v
                         Client reply
```

## Ubuntu installation

```bash
sudo apt update
sudo apt install build-essential liburing-dev python3
```

Check:

```bash
gcc --version
python3 --version
pkg-config --modversion liburing
```

## Build

```bash
make
```

Or:

```bash
gcc -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE -o server server.c queue.c -luring -lpthread
gcc -Wall -Wextra -O2 -std=c11 -D_GNU_SOURCE -o client client.c
```

## Run

Terminal 1:

```bash
./server
```

Terminal 2:

```bash
./client
```

The server uses port `9090` by default.

## Commands

Write a file:

```text
WRITE notes.txt Hello from the asynchronous server
```

Read a file:

```text
READ notes.txt
```

Show measurements:

```text
STATS
```

Close the client:

```text
QUIT
```

Files are stored inside the server's `storage/` directory.

## Concurrent testing

Run the server:

```bash
./server
```

Then:

```bash
python3 load_test.py 10 20
```

This creates 10 concurrent clients. Each client performs 20 writes and 20 reads.

Try a larger test:

```bash
python3 load_test.py 50 20
```

## OS concepts demonstrated

1. Process and thread concurrency
2. POSIX threads
3. Mutexes
4. Condition variables
5. Producer-consumer queues
6. TCP sockets
7. Linux system calls
8. Asynchronous I/O with io_uring
9. File descriptors
10. File system operations
11. High-resolution timing
12. Resource management
13. Race-condition prevention
14. Throughput and latency measurement

## Performance metrics

For each completed request:

```text
latency = completion_time - request_start_time
```

The `STATS` command reports:

- completed requests
- failed requests
- average latency in microseconds

The load-test script reports:

- number of clients
- number of file operations
- wall-clock execution time
- operations/second

## Notes

The server uses a pthread for each connected client, not for each file operation. File read/write operations are submitted through `io_uring`.

For a production system, the socket layer could also be converted to an event-driven `io_uring` network layer, but this academic implementation keeps the socket and queue components easy to demonstrate and test.
